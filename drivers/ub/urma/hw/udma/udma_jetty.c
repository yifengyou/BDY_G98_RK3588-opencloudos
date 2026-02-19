// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt
#define pr_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <ub/urma/ubcore_uapi.h>
#include "udma_dev.h"
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_jfr.h"
#include "udma_jfs.h"
#include "udma_jfc.h"
#include "udma_jetty.h"

bool well_known_jetty_pgsz_check = true;

const char *state_name[] = {
	"RESET",
	"READY",
	"SUSPENDED",
	"ERROR",
	"INVALID"
};

const char *to_state_name(enum ubcore_jetty_state state)
{
	if ((int)state >= (int)STATE_NUM)
		return state_name[STATE_NUM];

	return state_name[state];
}

static int udma_get_user_jetty_cmd(struct udma_dev *dev, struct udma_jetty *jetty,
				   struct ubcore_udata *udata,
				   struct udma_create_jetty_ucmd *ucmd)
{
	struct udma_context *uctx;
	unsigned long byte;

	if (!udata) {
		jetty->sq.jetty_type = (enum udma_jetty_type)UDMA_URMA_NORMAL_JETTY_TYPE;
		return 0;
	}

	if (!udata->udrv_data) {
		dev_err(dev->dev, "jetty udata udrv_data is null.\n");
		return -EINVAL;
	}

	if (!udata->udrv_data->in_addr || udata->udrv_data->in_len < sizeof(*ucmd)) {
		dev_err(dev->dev, "jetty in_len (%u) or addr is invalid.\n",
			udata->udrv_data->in_len);
		return -EINVAL;
	}

	byte = copy_from_user(ucmd, (void *)(uintptr_t)udata->udrv_data->in_addr,
			      sizeof(*ucmd));
	if (byte) {
		dev_err(dev->dev,
			"failed to copy jetty udata, byte = %lu.\n", byte);
		return -EFAULT;
	}

	uctx = to_udma_context(udata->uctx);
	jetty->sq.udma_ctx = uctx;
	jetty->sq.tid = uctx->tid;
	jetty->jetty_addr = ucmd->jetty_addr;
	jetty->pi_type = ucmd->pi_type;
	jetty->sq.jetty_type = (enum udma_jetty_type)ucmd->jetty_type;
	jetty->sq.non_pin = ucmd->non_pin;

	return 0;
}

static int udma_get_jetty_buf(struct udma_dev *dev, struct udma_jetty *jetty,
			      struct ubcore_udata *udata,
			      struct ubcore_jetty_cfg *cfg,
			      struct udma_create_jetty_ucmd *ucmd)
{
	struct ubcore_jfs_cfg jfs_cfg = {
		.depth = cfg->jfs_depth,
		.trans_mode = cfg->trans_mode,
		.priority = cfg->priority,
		.max_sge = cfg->max_send_sge,
		.max_rsge = cfg->max_send_rsge,
		.max_inline_data = cfg->max_inline_data,
		.rnr_retry = cfg->rnr_retry,
		.err_timeout = cfg->err_timeout,
		.jfs_context = cfg->jetty_context,
		.jfc = cfg->send_jfc,
	};
	int ret;

	jfs_cfg.flag.bs.lock_free = cfg->flag.bs.lock_free;
	if (!udata)
		jetty->jetty_addr = (uintptr_t)&jetty->sq;

	jetty->jfr = to_udma_jfr(cfg->jfr);

	ret = udata ? udma_alloc_u_sq_buf(dev, &jetty->sq, ucmd) :
		udma_alloc_k_sq_buf(dev, &jetty->sq, &jfs_cfg);
	if (ret) {
		dev_err(dev->dev, "failed to get sq buf, ret = %d.\n", ret);
		return ret;
	}
	jetty->sq.trans_mode = jfs_cfg.trans_mode;
	jetty->sq.is_jetty = true;

	return ret;
}

static void udma_init_jettyc(struct udma_dev *dev, struct ubcore_jetty_cfg *cfg,
			     struct udma_jetty *jetty, void *mb_buf)
{
	struct udma_jetty_ctx *ctx = (struct udma_jetty_ctx *)mb_buf;
	struct udma_jfc *receive_jfc = to_udma_jfc(cfg->recv_jfc);
	uint8_t i;

	ctx->state = JETTY_READY;
	ctx->jfs_mode = JETTY;
	ctx->type = to_udma_type(cfg->trans_mode);
	ctx->sl = dev->udma_sl[UDMA_DEFAULT_SL_NUM];
	if (ctx->type == JETTY_RM || ctx->type == JETTY_RC) {
		for (i = 0; i < dev->udma_total_sl_num; i++) {
			if (cfg->priority == dev->udma_sl[i]) {
				ctx->sl = cfg->priority;
				break;
			}
		}
	} else if (ctx->type == JETTY_UM) {
		ctx->sl = dev->unic_sl[UDMA_DEFAULT_SL_NUM];
		for (i = 0; i < dev->unic_sl_num; i++) {
			if (cfg->priority == dev->unic_sl[i]) {
				ctx->sl = cfg->priority;
				break;
			}
		}
	}
	ctx->sqe_base_addr_l = (jetty->sq.buf.addr >> SQE_VA_L_OFFSET) &
			       (uint32_t)SQE_VA_L_VALID_BIT;
	ctx->sqe_base_addr_h = (jetty->sq.buf.addr >> SQE_VA_H_OFFSET) &
			       (uint32_t)SQE_VA_H_VALID_BIT;
	ctx->sqe_token_id_l = jetty->sq.tid & (uint32_t)SQE_TOKEN_ID_L_MASK;
	ctx->sqe_token_id_h = (jetty->sq.tid >> SQE_TOKEN_ID_H_OFFSET) &
			      (uint32_t)SQE_TOKEN_ID_H_MASK;
	ctx->sqe_bb_shift = ilog2(roundup_pow_of_two(jetty->sq.buf.entry_cnt));
	ctx->tx_jfcn = cfg->send_jfc->id;
	ctx->ta_timeout = to_ta_timeout(cfg->err_timeout);

	if (!!(dev->caps.feature & UDMA_CAP_FEATURE_RNR_RETRY))
		ctx->rnr_retry_num = cfg->rnr_retry;

	ctx->jfrn_l = jetty->jfr->rq.id;
	ctx->jfrn_h = jetty->jfr->rq.id >> JETTY_CTX_JFRN_H_OFFSET;
	ctx->rx_jfcn = cfg->recv_jfc->id;
	ctx->user_data_l = jetty->jetty_addr;
	ctx->user_data_h = jetty->jetty_addr >> UDMA_USER_DATA_H_OFFSET;
	ctx->seid_idx = cfg->eid_index;
	ctx->pi_type = jetty->pi_type ? 1 : 0;

	if (!!(dev->caps.feature & UDMA_CAP_FEATURE_JFC_INLINE))
		ctx->cqe_ie = receive_jfc->inline_en;

	ctx->err_mode = cfg->flag.bs.error_suspend;
	ctx->cmp_odr = cfg->flag.bs.outorder_comp;
	ctx->avail_sgmt_ost = AVAIL_SGMT_OST_INIT;
	ctx->sqe_pld_tokenid = jetty->sq.tid & (uint32_t)SQE_PLD_TOKEN_ID_MASK;
	ctx->next_send_ssn = get_random_u16();
	ctx->next_rcv_ssn = ctx->next_send_ssn;
}

static int update_jetty_grp_ctx_valid(struct udma_dev *udma_dev,
				      struct udma_jetty_grp *jetty_grp)
{
	struct udma_jetty_grp_ctx ctx[UDMA_CTX_NUM];
	struct ubase_mbx_attr mbox_attr = {};
	int ret;

	ctx[0].valid = jetty_grp->valid;
	/* jetty number indicates the location of the jetty with the largest ID. */
	ctx[0].jetty_number = fls(jetty_grp->valid) - 1;
	memset(ctx + 1, 0xff, sizeof(ctx[1]));
	ctx[1].valid = 0;
	ctx[1].jetty_number = 0;

	mbox_attr.tag = jetty_grp->jetty_grp_id;
	mbox_attr.op = UDMA_CMD_MODIFY_JETTY_GROUP_CONTEXT;
	ret = post_mailbox_update_ctx(udma_dev, ctx, sizeof(ctx), &mbox_attr);
	if (ret)
		dev_err(udma_dev->dev,
			"post mailbox update jetty grp ctx failed, ret = %d.\n",
			ret);

	return ret;
}

static uint32_t udma_get_jetty_grp_jetty_id(uint32_t *valid, uint32_t *next)
{
	uint32_t bit_idx;

	bit_idx = find_next_zero_bit((unsigned long *)valid, UDMA_BITS_PER_INT, *next);
	if (bit_idx >= UDMA_BITS_PER_INT)
		bit_idx = find_next_zero_bit((unsigned long *)valid, UDMA_BITS_PER_INT, 0);

	*next = (*next + 1) >= UDMA_BITS_PER_INT ? 0 : *next + 1;

	return bit_idx;
}

static int add_jetty_to_grp(struct udma_dev *udma_dev, struct ubcore_jetty_group *jetty_grp,
			    struct udma_jetty_queue *sq, uint32_t cfg_id)
{
	struct udma_jetty_grp *udma_jetty_grp = to_udma_jetty_grp(jetty_grp);
	uint32_t bit_idx = cfg_id - udma_jetty_grp->start_jetty_id;
	int ret = 0;

	mutex_lock(&udma_jetty_grp->valid_lock);

	if (cfg_id == 0)
		bit_idx = udma_get_jetty_grp_jetty_id(&udma_jetty_grp->valid,
						      &udma_jetty_grp->next_jetty_id);

	if (bit_idx >= UDMA_BITS_PER_INT || (udma_jetty_grp->valid & BIT(bit_idx))) {
		dev_err(udma_dev->dev,
			"jg(%u.%u) vallid %u is full or user id(%u) error",
			udma_jetty_grp->jetty_grp_id, udma_jetty_grp->start_jetty_id,
			udma_jetty_grp->valid, cfg_id);
		ret = -ENOMEM;
		goto out;
	}

	udma_jetty_grp->valid |= BIT(bit_idx);
	sq->id = udma_jetty_grp->start_jetty_id + bit_idx;
	sq->jetty_grp = udma_jetty_grp;

	ret = update_jetty_grp_ctx_valid(udma_dev, udma_jetty_grp);
	if (ret) {
		dev_err(udma_dev->dev,
			"update jetty grp ctx valid failed, jetty_grp id is %u.\n",
			udma_jetty_grp->jetty_grp_id);

		udma_jetty_grp->valid &= ~BIT(bit_idx);
	}
out:
	mutex_unlock(&udma_jetty_grp->valid_lock);

	return ret;
}

static void remove_jetty_from_grp(struct udma_dev *udma_dev,
				  struct udma_jetty *jetty)
{
	struct udma_jetty_grp *jetty_grp = jetty->sq.jetty_grp;
	uint32_t bit_idx;
	int ret;

	bit_idx = jetty->sq.id - jetty_grp->start_jetty_id;
	if (bit_idx >= UDMA_BITS_PER_INT) {
		dev_err(udma_dev->dev,
			"jetty_id(%u) is not in jetty grp, start_jetty_id(%u).\n",
			jetty->sq.id, jetty_grp->start_jetty_id);
		return;
	}

	mutex_lock(&jetty_grp->valid_lock);
	jetty_grp->valid &= ~BIT(bit_idx);
	jetty->sq.jetty_grp = NULL;

	ret = update_jetty_grp_ctx_valid(udma_dev, jetty_grp);
	if (ret)
		dev_err(udma_dev->dev,
			"update jetty grp ctx valid failed, jetty_grp id is %u.\n",
			jetty_grp->jetty_grp_id);

	mutex_unlock(&jetty_grp->valid_lock);
}

static int udma_specify_rsvd_jetty_id(struct udma_dev *udma_dev, uint32_t cfg_id)
{
	struct udma_ida *ida_table = &udma_dev->rsvd_jetty_ida_table;
	int id;

	id = ida_alloc_range(&ida_table->ida, cfg_id, cfg_id, GFP_KERNEL);
	if (id < 0) {
		dev_err(udma_dev->dev, "user specify id %u has been used, ret = %d.\n", cfg_id, id);
		return id;
	}

	return 0;
}

static int udma_user_specify_jetty_id(struct udma_dev *udma_dev, uint32_t cfg_id)
{
	if (cfg_id < udma_dev->caps.jetty.start_idx)
		return udma_specify_rsvd_jetty_id(udma_dev, cfg_id);

	return udma_specify_adv_id(udma_dev, &udma_dev->jetty_table.bitmap_table,
				   cfg_id);
}

int udma_alloc_jetty_id(struct udma_dev *udma_dev, uint32_t *idx,
			struct udma_res *jetty_res)
{
	struct udma_group_bitmap *bitmap = &udma_dev->jetty_table.bitmap_table;
	struct ida *ida = &udma_dev->rsvd_jetty_ida_table.ida;
	uint32_t min = jetty_res->start_idx;
	uint32_t next = jetty_res->next_idx;
	uint32_t max;
	int ret;

	if (jetty_res->max_cnt == 0) {
		dev_err(udma_dev->dev, "ida alloc failed max_cnt is 0.\n");
		return -EINVAL;
	}

	max = jetty_res->start_idx + jetty_res->max_cnt - 1;

	if (jetty_res != &udma_dev->caps.jetty) {
		ret = ida_alloc_range(ida, next, max, GFP_KERNEL);
		if (ret < 0) {
			ret = ida_alloc_range(ida, min, max, GFP_KERNEL);
			if (ret < 0) {
				dev_err(udma_dev->dev,
					"ida alloc failed %d.\n", ret);
				return ret;
			}
		}

		*idx = (uint32_t)ret;
	} else {
		ret = udma_adv_id_alloc(udma_dev, bitmap, idx, false, next);
		if (ret) {
			ret = udma_adv_id_alloc(udma_dev, bitmap, idx, false, min);
			if (ret) {
				dev_err(udma_dev->dev,
					"bitmap alloc failed %d.\n", ret);
				return ret;
			}
		}
	}

	jetty_res->next_idx = (*idx + 1) > max ? min : (*idx + 1);

	return 0;
}

static int udma_alloc_normal_jetty_id(struct udma_dev *udma_dev, uint32_t *idx)
{
	int ret;

	ret = udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.jetty);
	if (ret == 0)
		return 0;

	ret = udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.user_ctrl_normal_jetty);
	if (ret == 0)
		return 0;

	return udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.public_jetty);
}

#define CFGID_CHECK(a, b) ((a) >= (b).start_idx && (a) < (b).start_idx + (b).max_cnt)

static int udma_verify_jetty_type_dwqe(struct udma_dev *udma_dev,
				       uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.stars_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, cache lock st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.stars_jetty.start_idx,
			udma_dev->caps.stars_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_ccu(struct udma_dev *udma_dev,
				      uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.ccu_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, ccu st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.ccu_jetty.start_idx,
			udma_dev->caps.ccu_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_normal(struct udma_dev *udma_dev,
					 uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.user_ctrl_normal_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, user ctrl normal st idx %u cnt %u.\n",
			cfg_id,
			udma_dev->caps.user_ctrl_normal_jetty.start_idx,
			udma_dev->caps.user_ctrl_normal_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_urma_normal(struct udma_dev *udma_dev,
					      uint32_t cfg_id)
{
	if (!(CFGID_CHECK(cfg_id, udma_dev->caps.public_jetty) ||
		CFGID_CHECK(cfg_id, udma_dev->caps.hdc_jetty) ||
		CFGID_CHECK(cfg_id, udma_dev->caps.jetty))) {
		dev_err(udma_dev->dev,
			"user id %u error, ccu st idx %u cnt %u, stars st idx %u, normal st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.ccu_jetty.start_idx,
			udma_dev->caps.ccu_jetty.max_cnt,
			udma_dev->caps.stars_jetty.start_idx,
			udma_dev->caps.jetty.start_idx,
			udma_dev->caps.jetty.max_cnt);
		return -EINVAL;
	}

	if (well_known_jetty_pgsz_check && PAGE_SIZE != UDMA_HW_PAGE_SIZE) {
		dev_err(udma_dev->dev, "Does not support specifying Jetty ID on non-4KB page systems.\n");
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type(struct udma_dev *udma_dev,
				  enum udma_jetty_type jetty_type, uint32_t cfg_id)
{
	int (*udma_cfg_id_check[UDMA_JETTY_TYPE_MAX])(struct udma_dev *udma_dev,
						      uint32_t cfg_id) = {
		udma_verify_jetty_type_dwqe,
		udma_verify_jetty_type_ccu,
		udma_verify_jetty_type_normal,
		udma_verify_jetty_type_urma_normal
	};

	if (jetty_type < UDMA_JETTY_TYPE_MAX) {
		if (!cfg_id)
			return 0;

		return udma_cfg_id_check[jetty_type](udma_dev, cfg_id);
	}

	dev_err(udma_dev->dev, "invalid jetty type 0x%x.\n", jetty_type);
	return -EINVAL;
}

static int udma_alloc_jetty_id_own(struct udma_dev *udma_dev, uint32_t *id,
				   enum udma_jetty_type jetty_type)
{
	int ret;

	switch (jetty_type) {
	case UDMA_CACHE_LOCK_DWQE_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id,
			&udma_dev->caps.stars_jetty);
		break;
	case UDMA_NORMAL_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id,
			&udma_dev->caps.user_ctrl_normal_jetty);
		break;
	case UDMA_CCU_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id, &udma_dev->caps.ccu_jetty);
		break;
	default:
		ret = udma_alloc_normal_jetty_id(udma_dev, id);
		break;
	}

	if (ret)
		dev_err(udma_dev->dev,
			"udma alloc jetty id own failed, type = %d, ret = %d.\n",
			jetty_type, ret);

	return ret;
}

int alloc_jetty_id(struct udma_dev *udma_dev, struct udma_jetty_queue *sq,
		   uint32_t cfg_id, struct ubcore_jetty_group *jetty_grp)
{
	int ret;

	if (udma_verify_jetty_type(udma_dev, sq->jetty_type, cfg_id))
		return -EINVAL;

	if (cfg_id > 0 && !jetty_grp) {
		ret = udma_user_specify_jetty_id(udma_dev, cfg_id);
		if (ret)
			return ret;

		sq->id = cfg_id;
	} else if (jetty_grp) {
		ret = add_jetty_to_grp(udma_dev, jetty_grp, sq, cfg_id);
		if (ret) {
			dev_err(udma_dev->dev,
				"add jetty to grp failed, ret = %d.\n", ret);
			return ret;
		}
	} else {
		ret = udma_alloc_jetty_id_own(udma_dev, &sq->id, sq->jetty_type);
	}

	return ret;
}

static void free_jetty_id(struct udma_dev *udma_dev,
			  struct udma_jetty *udma_jetty, bool is_grp)
{
	if (udma_jetty->sq.id < udma_dev->caps.jetty.start_idx)
		udma_id_free(&udma_dev->rsvd_jetty_ida_table, udma_jetty->sq.id);
	else if (is_grp)
		remove_jetty_from_grp(udma_dev, udma_jetty);
	else
		udma_adv_id_free(&udma_dev->jetty_table.bitmap_table,
				 udma_jetty->sq.id, false);
}

static void udma_dfx_store_jetty_id(struct udma_dev *udma_dev,
				    struct udma_jetty *udma_jetty)
{
	struct udma_dfx_jetty *jetty;
	int ret;

	jetty = (struct udma_dfx_jetty *)xa_load(&udma_dev->dfx_info->jetty.table,
						 udma_jetty->sq.id);
	if (jetty) {
		dev_warn(udma_dev->dev, "jetty_id(%u) already exists in dfx.\n",
			 udma_jetty->sq.id);
		return;
	}

	jetty = kzalloc(sizeof(*jetty), GFP_KERNEL);
	if (!jetty)
		return;

	jetty->id = udma_jetty->sq.id;
	jetty->jfs_depth = udma_jetty->sq.buf.entry_cnt / udma_jetty->sq.sqe_bb_cnt;

	write_lock(&udma_dev->dfx_info->jetty.rwlock);
	ret = xa_err(xa_store(&udma_dev->dfx_info->jetty.table, udma_jetty->sq.id,
			      jetty, GFP_KERNEL));
	if (ret) {
		write_unlock(&udma_dev->dfx_info->jetty.rwlock);
		dev_err(udma_dev->dev, "store jetty_id(%u) to jetty_table failed in dfx.\n",
			udma_jetty->sq.id);
		kfree(jetty);
		return;
	}

	++udma_dev->dfx_info->jetty.cnt;
	write_unlock(&udma_dev->dfx_info->jetty.rwlock);
}

static int
udma_alloc_jetty_sq(struct udma_dev *udma_dev, struct udma_jetty *jetty,
		    struct ubcore_jetty_cfg *cfg, struct ubcore_udata *udata)
{
	struct udma_create_jetty_ucmd ucmd = {};
	int ret;

	ret = udma_get_user_jetty_cmd(udma_dev, jetty, udata, &ucmd);
	if (ret) {
		dev_err(udma_dev->dev,
			"udma get user jetty ucmd failed, ret = %d.\n", ret);
		return ret;
	}

	ret = alloc_jetty_id(udma_dev, &jetty->sq, cfg->id, cfg->jetty_grp);
	if (ret) {
		dev_err(udma_dev->dev, "alloc jetty id failed, ret = %d.\n", ret);
		return ret;
	}
	jetty->ubcore_jetty.jetty_id.id = jetty->sq.id;
	jetty->ubcore_jetty.jetty_cfg = *cfg;

	ret = udma_get_jetty_buf(udma_dev, jetty, udata, cfg, &ucmd);
	if (ret)
		free_jetty_id(udma_dev, jetty, !!cfg->jetty_grp);

	return ret;
}

static void udma_free_jetty_id_buf(struct udma_dev *udma_dev,
				   struct udma_jetty *udma_jetty,
				   struct ubcore_jetty_cfg *cfg)
{
	udma_free_sq_buf(udma_dev, &udma_jetty->sq);
	free_jetty_id(udma_dev, udma_jetty, !!cfg->jetty_grp);
}

void udma_reset_sw_k_jetty_queue(struct udma_jetty_queue *sq)
{
	sq->kva_curr = sq->buf.kva;
	sq->pi = 0;
	sq->ci = 0;
	sq->flush_flag = false;
}

static int udma_create_hw_jetty_ctx(struct udma_dev *dev, struct udma_jetty *udma_jetty,
				    struct ubcore_jetty_cfg *cfg)
{
	struct ubase_mbx_attr attr = {};
	struct udma_jetty_ctx ctx = {};
	int ret;

	if (cfg->priority >= UDMA_MAX_PRIORITY) {
		dev_err(dev->dev, "kernel mode jetty priority is out of range, priority is %u.\n",
			cfg->priority);
		return -EINVAL;
	}

	udma_init_jettyc(dev, cfg, udma_jetty, &ctx);

	attr.tag = udma_jetty->sq.id;
	attr.op = UDMA_CMD_CREATE_JFS_CONTEXT;
	ret = post_mailbox_update_ctx(dev, &ctx, sizeof(ctx), &attr);
	if (ret)
		dev_err(dev->dev,
			"post mailbox create jetty ctx failed, ret = %d.\n", ret);

	return ret;
}

void udma_set_query_flush_time(struct udma_jetty_queue *sq, uint8_t err_timeout)
{
#define UDMA_TA_TIMEOUT_MAX_INDEX 3
	uint32_t time[] = {
				UDMA_TA_TIMEOUT_128MS,
				UDMA_TA_TIMEOUT_1000MS,
				UDMA_TA_TIMEOUT_8000MS,
				UDMA_TA_TIMEOUT_64000MS,
			};
	uint8_t index;

	index = to_ta_timeout(err_timeout);
	if (index > UDMA_TA_TIMEOUT_MAX_INDEX)
		index = UDMA_TA_TIMEOUT_MAX_INDEX;

	sq->ta_timeout = time[index];
}

struct ubcore_jetty *udma_create_jetty(struct ubcore_device *ub_dev,
				       struct ubcore_jetty_cfg *cfg,
				       struct ubcore_udata *udata)
{
	struct udma_dev *udma_dev = to_udma_dev(ub_dev);
	struct udma_jetty *udma_jetty;
	int ret;

	udma_jetty = kzalloc(sizeof(*udma_jetty), GFP_KERNEL);
	if (!udma_jetty)
		return NULL;

	ret = udma_alloc_jetty_sq(udma_dev, udma_jetty, cfg, udata);
	if (ret) {
		dev_err(udma_dev->dev,
			"udma alloc jetty id buf failed, ret = %d.\n", ret);
		goto err_alloc_jetty;
	}

	ret = xa_err(xa_store(&udma_dev->jetty_table.xa, udma_jetty->sq.id,
			      &udma_jetty->sq, GFP_KERNEL));
	if (ret) {
		dev_err(udma_dev->dev,
			"store jetty sq(%u) to sq table failed, ret = %d.\n",
			udma_jetty->sq.id, ret);
		goto err_store_jetty_sq;
	}

	ret = udma_create_hw_jetty_ctx(udma_dev, udma_jetty, cfg);
	if (ret) {
		dev_err(udma_dev->dev,
			"post mailbox create jetty ctx failed, ret = %d.\n", ret);
		goto err_create_hw_jetty;
	}

	udma_set_query_flush_time(&udma_jetty->sq, cfg->err_timeout);
	udma_jetty->sq.state = UBCORE_JETTY_STATE_READY;
	refcount_set(&udma_jetty->ae_refcount, 1);
	init_completion(&udma_jetty->ae_comp);

	if (dfx_switch)
		udma_dfx_store_jetty_id(udma_dev, udma_jetty);

	return &udma_jetty->ubcore_jetty;
err_create_hw_jetty:
	xa_erase(&udma_dev->jetty_table.xa, udma_jetty->sq.id);
err_store_jetty_sq:
	udma_free_jetty_id_buf(udma_dev, udma_jetty, cfg);
err_alloc_jetty:
	kfree(udma_jetty);

	return NULL;
}

int udma_destroy_hw_jetty_ctx(struct udma_dev *dev, uint32_t jetty_id)
{
	struct ubase_mbx_attr attr = {};
	int ret;

	attr.tag = jetty_id;
	attr.op = UDMA_CMD_DESTROY_JFS_CONTEXT;
	ret = post_mailbox_update_ctx(dev, NULL, 0, &attr);
	if (ret)
		dev_err(dev->dev,
			"post mailbox destroy jetty ctx failed, ret = %d.\n", ret);

	return ret;
}

int udma_set_jetty_state(struct udma_dev *dev, uint32_t jetty_id,
			 enum jetty_state state)
{
	struct udma_jetty_ctx *ctx, *ctx_mask;
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;
	int ret;

	mailbox = udma_alloc_cmd_mailbox(dev);
	if (!mailbox) {
		dev_err(dev->dev, "failed to alloc mailbox for jettyc.\n");
		return -EINVAL;
	}

	ctx = (struct udma_jetty_ctx *)mailbox->buf;

	/* Optimize chip access performance. */
	ctx_mask = (struct udma_jetty_ctx *)((char *)ctx + UDMA_JFS_MASK_OFFSET);
	memset(ctx_mask, 0xff, sizeof(struct udma_jetty_ctx));
	ctx->state = state;
	ctx_mask->state = 0;

	mbox_attr.tag = jetty_id;
	mbox_attr.op = UDMA_CMD_MODIFY_JFS_CONTEXT;
	ret = udma_post_mbox(dev, mailbox, &mbox_attr);
	if (ret)
		dev_err(dev->dev,
			"failed to upgrade jettyc, ret = %d.\n", ret);
	udma_free_cmd_mailbox(dev, mailbox);

	return ret;
}

static int udma_query_jetty_ctx(struct udma_dev *dev,
				struct udma_jetty_ctx *jfs_ctx,
				uint32_t jetty_id)
{
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;

	mbox_attr.tag = jetty_id;
	mbox_attr.op = UDMA_CMD_QUERY_JFS_CONTEXT;
	mailbox = udma_mailbox_query_ctx(dev, &mbox_attr);
	if (!mailbox)
		return -ENOMEM;
	memcpy((void *)jfs_ctx, mailbox->buf, sizeof(*jfs_ctx));

	udma_free_cmd_mailbox(dev, mailbox);

	return 0;
}

void udma_clean_cqe_for_jetty(struct udma_dev *dev, struct udma_jetty_queue *sq,
			      struct ubcore_jfc *send_jfc,
			      struct ubcore_jfc *recv_jfc)
{
	if (sq->buf.kva) {
		if (send_jfc)
			udma_clean_jfc(send_jfc, sq->id, dev);

		if (recv_jfc && recv_jfc != send_jfc)
			udma_clean_jfc(recv_jfc, sq->id, dev);
	}
}

static bool udma_wait_timeout(uint32_t *sum_times, uint32_t times, uint32_t ta_timeout)
{
	uint32_t wait_time;

	if (*sum_times > ta_timeout)
		return true;

	wait_time = 1 << times;
	msleep(wait_time);
	*sum_times += wait_time;

	return false;
}

static void udma_mask_jetty_ctx(struct udma_jetty_ctx *ctx)
{
	ctx->sqe_base_addr_l = 0;
	ctx->sqe_base_addr_h = 0;
	ctx->user_data_l = 0;
	ctx->user_data_h = 0;
}

static bool udma_query_jetty_fd(struct udma_dev *dev, struct udma_jetty_queue *sq)
{
	struct udma_jetty_ctx ctx = {};
	uint16_t rcv_send_diff = 0;
	uint32_t sum_times = 0;
	uint32_t times = 0;

	while (true) {
		if (udma_query_jetty_ctx(dev, &ctx, sq->id))
			return false;

		if (ctx.flush_cqe_done)
			return true;

		if (udma_wait_timeout(&sum_times, times, UDMA_TA_TIMEOUT_64000MS))
			break;

		times++;
	}

	/* In the flip scenario, ctx.next_rcv_ssn - ctx.next_send_ssn value is less than 512. */
	rcv_send_diff = ctx.next_rcv_ssn - ctx.next_send_ssn;
	if (ctx.flush_ssn_vld && rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF)
		return true;

	udma_mask_jetty_ctx(&ctx);
	udma_dfx_ctx_print(dev, "Flush Failed Jetty", sq->id, sizeof(ctx) / sizeof(uint32_t),
			   (uint32_t *)&ctx);

	return false;
}

int udma_modify_jetty_precondition(struct udma_dev *dev, struct udma_jetty_queue *sq)
{
	struct udma_jetty_ctx ctx = {};
	uint16_t rcv_send_diff = 0;
	uint32_t sum_times = 0;
	uint32_t times = 0;
	int ret;

	while (true) {
		ret = udma_query_jetty_ctx(dev, &ctx, sq->id);
		if (ret) {
			dev_err(dev->dev, "query jetty ctx failed, id = %u, ret = %d.\n",
				sq->id, ret);
			return ret;
		}

		rcv_send_diff = ctx.next_rcv_ssn - ctx.next_send_ssn;
		if (ctx.PI == ctx.CI && rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF &&
		    ctx.state == JETTY_READY)
			break;

		if (rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF &&
		    ctx.state == JETTY_ERROR)
			break;

		if (udma_wait_timeout(&sum_times, times, sq->ta_timeout)) {
			dev_warn(dev->dev, "TA timeout, id = %u. PI = %d, CI = %d, nxt_send_ssn = %d nxt_rcv_ssn = %d state = %d.\n",
				 sq->id, ctx.PI, ctx.CI, ctx.next_send_ssn,
				 ctx.next_rcv_ssn, ctx.state);
			break;
		}
		times++;
	}

	return 0;
}

static bool udma_destroy_jetty_precondition(struct udma_dev *dev, struct udma_jetty_queue *sq)
{
#define UDMA_DESTROY_JETTY_DELAY_TIME 100U

	if (sq->state != UBCORE_JETTY_STATE_READY && sq->state != UBCORE_JETTY_STATE_SUSPENDED)
		goto query_jetty_fd;

	if (dev->caps.feature & UDMA_CAP_FEATURE_UE_RX_CLOSE)
		goto modify_to_err;

	if (udma_modify_jetty_precondition(dev, sq))
		return false;

modify_to_err:
	if (udma_set_jetty_state(dev, sq->id, JETTY_ERROR)) {
		dev_err(dev->dev, "modify jetty to error failed, id: %u.\n",
			sq->id);
		return false;
	}

	sq->state = UBCORE_JETTY_STATE_ERROR;

query_jetty_fd:
	if (!udma_query_jetty_fd(dev, sq))
		return false;

	udelay(UDMA_DESTROY_JETTY_DELAY_TIME);

	return true;
}

int udma_modify_and_destroy_jetty(struct udma_dev *dev,
				  struct udma_jetty_queue *sq)
{
	int ret;

	if (!udma_destroy_jetty_precondition(dev, sq))
		return -EFAULT;

	if (sq->state != UBCORE_JETTY_STATE_RESET) {
		ret = udma_destroy_hw_jetty_ctx(dev, sq->id);
		if (ret) {
			dev_err(dev->dev, "jetty destroyed failed, id: %u.\n",
				sq->id);
			return ret;
		}
	}

	return 0;
}

static void udma_free_jetty(struct ubcore_jetty *jetty)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);

	udma_clean_cqe_for_jetty(udma_dev, &udma_jetty->sq, jetty->jetty_cfg.send_jfc,
				 jetty->jetty_cfg.recv_jfc);

	if (dfx_switch)
		udma_dfx_delete_id(udma_dev, &udma_dev->dfx_info->jetty,
				   udma_jetty->sq.id);

	xa_erase(&udma_dev->jetty_table.xa, udma_jetty->sq.id);

	if (refcount_dec_and_test(&udma_jetty->ae_refcount))
		complete(&udma_jetty->ae_comp);
	wait_for_completion(&udma_jetty->ae_comp);

	udma_free_sq_buf(udma_dev, &udma_jetty->sq);
	free_jetty_id(udma_dev, udma_jetty, !!udma_jetty->sq.jetty_grp);
	kfree(udma_jetty);
}

int udma_destroy_jetty(struct ubcore_jetty *jetty)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);
	int ret;

	if (!udma_jetty->ue_rx_closed && udma_close_ue_rx(udma_dev, true, true, false, 0)) {
		dev_err(udma_dev->dev, "close ue rx failed when destroying jetty.\n");
		return -EINVAL;
	}

	ret = udma_modify_and_destroy_jetty(udma_dev, &udma_jetty->sq);
	if (ret) {
		dev_err(udma_dev->dev, "udma modify error and destroy jetty failed, id: %u.\n",
			jetty->jetty_id.id);
		if (!udma_jetty->ue_rx_closed)
			udma_open_ue_rx(udma_dev, true, true, false, 0);
		return ret;
	}

	udma_free_jetty(jetty);
	udma_open_ue_rx(udma_dev, true, true, false, 0);

	return 0;
}

static int udma_batch_jetty_get_ack(struct udma_dev *dev,
				    struct udma_jetty_queue **sq_list,
				    uint32_t jetty_cnt, bool *jetty_flag,
				    int *bad_jetty_index)
{
	struct udma_jetty_ctx ctx = {};
	struct udma_jetty_queue *sq;
	uint16_t rcv_send_diff = 0;
	uint32_t i;
	int ret;

	for (i = 0; i < jetty_cnt; i++) {
		sq = sq_list[i];
		if (sq->state != UBCORE_JETTY_STATE_READY &&
		    sq->state != UBCORE_JETTY_STATE_SUSPENDED)
			continue;

		if (jetty_flag[i])
			continue;

		ret = udma_query_jetty_ctx(dev, &ctx, sq->id);
		if (ret) {
			dev_err(dev->dev,
				"query jetty ctx failed, id = %u, ret = %d.\n",
				sq->id, ret);
			*bad_jetty_index = 0;
			return ret;
		}

		rcv_send_diff = ctx.next_rcv_ssn - ctx.next_send_ssn;
		if (ctx.PI == ctx.CI && rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF &&
		    ctx.state == JETTY_READY) {
			jetty_flag[i] = true;
			continue;
		}

		if (rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF &&
		    ctx.state == JETTY_ERROR) {
			jetty_flag[i] = true;
			continue;
		}

		*bad_jetty_index = 0;
		break;
	}

	return (i == jetty_cnt) ? 0 : -EAGAIN;
}

static uint32_t get_max_jetty_ta_timeout(struct udma_jetty_queue **sq_list,
					 uint32_t jetty_cnt)
{
	uint32_t max_timeout = 0;
	uint32_t i;

	for (i = 0; i < jetty_cnt; i++) {
		if (sq_list[i]->ta_timeout > max_timeout)
			max_timeout = sq_list[i]->ta_timeout;
	}

	return max_timeout;
}

static bool udma_batch_query_jetty_fd(struct udma_dev *dev,
				      struct udma_jetty_queue **sq_list,
				      uint32_t jetty_cnt, int *bad_jetty_index)
{
	uint32_t ta_timeout = get_max_jetty_ta_timeout(sq_list, jetty_cnt);
	struct udma_jetty_ctx ctx = {};
	struct udma_jetty_queue *sq;
	uint16_t rcv_send_diff = 0;
	uint32_t sum_times = 0;
	uint32_t flush_cnt = 0;
	bool all_query_done;
	uint32_t times = 0;
	bool *jetty_flag;
	uint32_t i;

	jetty_flag = kcalloc(jetty_cnt, sizeof(bool), GFP_KERNEL);
	if (!jetty_flag) {
		*bad_jetty_index = 0;
		return false;
	}

	while (true) {
		for (i = 0; i < jetty_cnt; i++) {
			if (jetty_flag[i])
				continue;

			sq = sq_list[i];
			if (udma_query_jetty_ctx(dev, &ctx, sq->id)) {
				kfree(jetty_flag);
				*bad_jetty_index = 0;
				return false;
			}

			if (!ctx.flush_cqe_done)
				continue;

			flush_cnt++;
			jetty_flag[i] = true;
		}

		if (flush_cnt == jetty_cnt) {
			kfree(jetty_flag);
			return true;
		}

		if (udma_wait_timeout(&sum_times, times, ta_timeout))
			break;

		times++;
	}

	all_query_done = true;

	for (i = 0; i < jetty_cnt; i++) {
		if (jetty_flag[i])
			continue;

		sq = sq_list[i];
		if (udma_query_jetty_ctx(dev, &ctx, sq->id)) {
			kfree(jetty_flag);
			*bad_jetty_index = 0;
			return false;
		}

		rcv_send_diff = ctx.next_rcv_ssn - ctx.next_send_ssn;
		if (ctx.flush_cqe_done || (ctx.flush_ssn_vld &&
		    rcv_send_diff < UDMA_RCV_SEND_MAX_DIFF))
			continue;

		*bad_jetty_index = 0;
		all_query_done = false;

		udma_mask_jetty_ctx(&ctx);
		udma_dfx_ctx_print(dev, "Flush Failed Jetty", sq->id,
				   sizeof(ctx) / sizeof(uint32_t), (uint32_t *)&ctx);
		break;
	}

	kfree(jetty_flag);

	return all_query_done;
}

static int batch_modify_jetty_to_error(struct udma_dev *dev,
				       struct udma_jetty_queue **sq_list,
				       uint32_t jetty_cnt, int *bad_jetty_index)
{
	struct udma_jetty_queue *sq;
	uint32_t i;
	int ret;

	for (i = 0; i < jetty_cnt; i++) {
		sq = sq_list[i];
		if (sq->state == UBCORE_JETTY_STATE_ERROR ||
		    sq->state == UBCORE_JETTY_STATE_RESET)
			continue;

		ret = udma_set_jetty_state(dev, sq->id, JETTY_ERROR);
		if (ret) {
			dev_err(dev->dev, "modify jetty to error failed, id: %u.\n",
				sq->id);
			*bad_jetty_index = 0;
			return ret;
		}

		sq->state = UBCORE_JETTY_STATE_ERROR;
	}

	return 0;
}

static int udma_batch_modify_jetty_precondition(struct udma_dev *dev,
						struct udma_jetty_queue **sq_list,
						uint32_t jetty_cnt, int *bad_jetty_index)
{
	uint32_t ta_timeout = get_max_jetty_ta_timeout(sq_list, jetty_cnt);
	uint32_t sum_times = 0;
	uint32_t times = 0;
	bool *jetty_flag;
	int ret;

	jetty_flag = kcalloc(jetty_cnt, sizeof(bool), GFP_KERNEL);
	if (!jetty_flag) {
		*bad_jetty_index = 0;
		return -ENOMEM;
	}

	while (true) {
		ret = udma_batch_jetty_get_ack(dev, sq_list, jetty_cnt,
					       jetty_flag, bad_jetty_index);
		if (ret != -EAGAIN) {
			kfree(jetty_flag);
			return ret;
		}

		if (udma_wait_timeout(&sum_times, times, ta_timeout)) {
			dev_warn(dev->dev,
				 "timeout after %u ms, not all jetty get ack.\n",
				 sum_times);
			break;
		}
		times++;
	}

	kfree(jetty_flag);

	return 0;
}

static bool udma_batch_destroy_jetty_precondition(struct udma_dev *dev,
						  struct udma_jetty_queue **sq_list,
						  uint32_t jetty_cnt, int *bad_jetty_index)
{
	if (!(dev->caps.feature & UDMA_CAP_FEATURE_UE_RX_CLOSE) &&
	    udma_batch_modify_jetty_precondition(dev, sq_list, jetty_cnt, bad_jetty_index))
		return false;

	if (batch_modify_jetty_to_error(dev, sq_list, jetty_cnt, bad_jetty_index)) {
		dev_err(dev->dev, "batch md jetty err failed.\n");
		return false;
	}

	if (!udma_batch_query_jetty_fd(dev, sq_list, jetty_cnt, bad_jetty_index))
		return false;

	udelay(UDMA_DESTROY_JETTY_DELAY_TIME);

	return true;
}

int udma_batch_modify_and_destroy_jetty(struct udma_dev *dev,
					struct udma_jetty_queue **sq_list,
					uint32_t jetty_cnt, int *bad_jetty_index)
{
	uint32_t i;
	int ret;

	if (!udma_batch_destroy_jetty_precondition(dev, sq_list, jetty_cnt, bad_jetty_index))
		return -EFAULT;

	for (i = 0; i < jetty_cnt; i++) {
		if (sq_list[i]->state != UBCORE_JETTY_STATE_RESET) {
			ret = udma_destroy_hw_jetty_ctx(dev, sq_list[i]->id);
			if (ret) {
				dev_err(dev->dev,
					"jetty destroyed failed, id: %u.\n",
					sq_list[i]->id);
				*bad_jetty_index = 0;
				return ret;
			}

			sq_list[i]->state = UBCORE_JETTY_STATE_RESET;
		}
	}

	return 0;
}

int udma_destroy_jetty_batch(struct ubcore_jetty **jetty, int jetty_cnt, int *bad_jetty_index)
{
	struct udma_jetty_queue **sq_list;
	struct udma_dev *udma_dev;
	uint32_t i;
	int ret;

	if (!jetty) {
		pr_err("jetty array is null.\n");
		return -EINVAL;
	}

	if (!jetty_cnt) {
		pr_err("jetty cnt is 0.\n");
		return -EINVAL;
	}

	udma_dev = to_udma_dev(jetty[0]->ub_dev);

	sq_list = kcalloc(1, sizeof(*sq_list) * jetty_cnt, GFP_KERNEL);
	if (!sq_list) {
		*bad_jetty_index = 0;
		return -ENOMEM;
	}

	for (i = 0; i < jetty_cnt; i++)
		sq_list[i] = &(to_udma_jetty(jetty[i])->sq);

	ret = udma_batch_modify_and_destroy_jetty(udma_dev, sq_list, jetty_cnt, bad_jetty_index);

	kfree(sq_list);

	if (ret) {
		dev_err(udma_dev->dev,
			 "udma batch modify error and destroy jetty failed.\n");
		return ret;
	}

	for (i = 0; i < jetty_cnt; i++)
		udma_free_jetty(jetty[i]);

	return 0;
}

static int udma_check_jetty_grp_info(struct ubcore_tjetty_cfg *cfg, struct udma_dev *dev)
{
	if (cfg->type == UBCORE_JETTY_GROUP) {
		if (cfg->trans_mode != UBCORE_TP_RM) {
			dev_err(dev->dev, "import jg only support RM, transmode is %u.\n",
				cfg->trans_mode);
			return -EINVAL;
		}

		if (cfg->policy != UBCORE_JETTY_GRP_POLICY_HASH_HINT) {
			dev_err(dev->dev, "import jg only support hint, policy is %u.\n",
				cfg->policy);
			return -EINVAL;
		}
	}

	return 0;
}

int udma_unimport_jetty(struct ubcore_tjetty *tjetty)
{
	struct udma_target_jetty *udma_tjetty = to_udma_tjetty(tjetty);
	struct udma_dev *udma_dev = to_udma_dev(tjetty->ub_dev);

	if (!IS_ERR_OR_NULL(tjetty->vtpn)) {
		dev_err(udma_dev->dev,
			"the target jetty is still being used, id = %u.\n",
			tjetty->cfg.id.id);
		return -EINVAL;
	}

	udma_tjetty->token_value = 0;
	tjetty->cfg.token_value.token = 0;
	kfree(udma_tjetty);

	return 0;
}

bool verify_modify_jetty(enum ubcore_jetty_state jetty_state,
			 enum ubcore_jetty_state attr_state)
{
	switch (jetty_state) {
	case UBCORE_JETTY_STATE_RESET:
		return attr_state == UBCORE_JETTY_STATE_READY;
	case UBCORE_JETTY_STATE_READY:
		return attr_state == UBCORE_JETTY_STATE_ERROR ||
		       attr_state == UBCORE_JETTY_STATE_SUSPENDED;
	case UBCORE_JETTY_STATE_SUSPENDED:
		return attr_state == UBCORE_JETTY_STATE_ERROR;
	case UBCORE_JETTY_STATE_ERROR:
		return attr_state == UBCORE_JETTY_STATE_RESET;
	default:
		break;
	}

	return false;
}

enum jetty_state to_jetty_state(enum ubcore_jetty_state state)
{
	switch (state) {
	case UBCORE_JETTY_STATE_ERROR:
		return JETTY_ERROR;
	case UBCORE_JETTY_STATE_SUSPENDED:
		return JETTY_SUSPEND;
	default:
		break;
	}

	return STATE_NUM;
}

static int udma_modify_jetty_state(struct udma_dev *udma_dev, struct udma_jetty *udma_jetty,
				   struct ubcore_jetty_attr *attr)
{
	int ret;

	switch (attr->state) {
	case UBCORE_JETTY_STATE_RESET:
		ret = udma_destroy_hw_jetty_ctx(udma_dev, udma_jetty->sq.id);
		break;
	case UBCORE_JETTY_STATE_READY:
		ret = udma_create_hw_jetty_ctx(udma_dev, udma_jetty,
					       &udma_jetty->ubcore_jetty.jetty_cfg);
		if (ret)
			break;

		udma_reset_sw_k_jetty_queue(&udma_jetty->sq);
		break;
	default:
		ret = udma_close_ue_rx(udma_dev, true, true, false, 0);
		if (ret)
			break;

		if (!(udma_dev->caps.feature & UDMA_CAP_FEATURE_UE_RX_CLOSE)) {
			if (udma_modify_jetty_precondition(udma_dev, &udma_jetty->sq)) {
				ret = -ENOMEM;
				udma_open_ue_rx(udma_dev, true, true, false, 0);
				break;
			}
		}

		ret = udma_set_jetty_state(udma_dev, udma_jetty->sq.id,
					   to_jetty_state(attr->state));
		if (ret)
			udma_open_ue_rx(udma_dev, true, true, false, 0);
		else
			udma_jetty->ue_rx_closed = true;
		break;
	}

	return ret;
}

int udma_modify_jetty(struct ubcore_jetty *jetty, struct ubcore_jetty_attr *attr,
		      struct ubcore_udata *udata)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);
	int ret;

	if (!(attr->mask & UBCORE_JETTY_STATE)) {
		dev_err(udma_dev->dev, "modify jetty mask is error or not set, jetty_id = %u.\n",
			udma_jetty->sq.id);
		return -EINVAL;
	}

	if (udma_jetty->sq.state == attr->state) {
		dev_info(udma_dev->dev, "jetty state has been %s.\n", to_state_name(attr->state));
		return 0;
	}

	if (!verify_modify_jetty(udma_jetty->sq.state, attr->state)) {
		dev_err(udma_dev->dev, "not support modify jetty state from %s to %s.\n",
			to_state_name(udma_jetty->sq.state), to_state_name(attr->state));
		return -EINVAL;
	}

	ret = udma_modify_jetty_state(udma_dev, udma_jetty, attr);
	if (ret) {
		dev_err(udma_dev->dev, "modify jetty %u state to %s failed.\n",
			udma_jetty->sq.id, to_state_name(attr->state));
		return ret;
	}
	udma_jetty->sq.state = attr->state;

	return 0;
}

static int udma_alloc_group_start_id(struct udma_dev *udma_dev,
				     struct udma_group_bitmap *bitmap_table,
				     uint32_t *start_jetty_id)
{
	int ret;

	ret = udma_adv_id_alloc(udma_dev, bitmap_table, start_jetty_id, true,
				bitmap_table->grp_next);
	if (ret) {
		ret = udma_adv_id_alloc(udma_dev, bitmap_table, start_jetty_id,
					true, bitmap_table->min);
		if (ret)
			return ret;
	}

	bitmap_table->grp_next = (*start_jetty_id + NUM_JETTY_PER_GROUP) >
				 bitmap_table->max ? bitmap_table->min :
				 (*start_jetty_id + NUM_JETTY_PER_GROUP);

	return 0;
}

static int udma_alloc_jetty_grp_id(struct udma_dev *udma_dev,
				   struct udma_jetty_grp *jetty_grp)
{
	int ret;

	ret = udma_alloc_group_start_id(udma_dev, &udma_dev->jetty_table.bitmap_table,
					&jetty_grp->start_jetty_id);
	if (ret) {
		dev_err(udma_dev->dev,
			"alloc jetty id for grp failed, ret = %d.\n", ret);
		return ret;
	}

	ret = udma_id_alloc_auto_grow(udma_dev, &udma_dev->jetty_grp_table.ida_table,
				      &jetty_grp->jetty_grp_id);
	if (ret) {
		dev_err(udma_dev->dev,
			"alloc jetty grp id failed, ret = %d.\n", ret);
		udma_adv_id_free(&udma_dev->jetty_table.bitmap_table,
				 jetty_grp->start_jetty_id, true);
		return ret;
	}

	jetty_grp->ubcore_jetty_grp.jetty_grp_id.id = jetty_grp->jetty_grp_id;

	return 0;
}

struct ubcore_jetty_group *udma_create_jetty_grp(struct ubcore_device *dev,
						 struct ubcore_jetty_grp_cfg *cfg,
						 struct ubcore_udata *udata)
{
	struct udma_dev *udma_dev = to_udma_dev(dev);
	struct ubase_mbx_attr mbox_attr = {};
	struct udma_jetty_grp_ctx ctx = {};
	struct udma_jetty_grp *jetty_grp;
	int ret;

	if (cfg->policy != UBCORE_JETTY_GRP_POLICY_HASH_HINT) {
		dev_err(udma_dev->dev, "policy %u not support.\n", cfg->policy);
		return NULL;
	}

	jetty_grp = kzalloc(sizeof(*jetty_grp), GFP_KERNEL);
	if (!jetty_grp)
		return NULL;

	ret = udma_alloc_jetty_grp_id(udma_dev, jetty_grp);
	if (ret)
		goto err_alloc_jetty_grp_id;

	ctx.start_jetty_id = jetty_grp->start_jetty_id;

	ret = xa_err(xa_store(&udma_dev->jetty_grp_table.xa, jetty_grp->jetty_grp_id,
			      jetty_grp, GFP_KERNEL));
	if (ret) {
		dev_err(udma_dev->dev, "store jetty group(%u) failed, ret = %d.\n",
			jetty_grp->jetty_grp_id, ret);
		goto err_store_jetty_grp;
	}

	mbox_attr.tag = jetty_grp->jetty_grp_id;
	mbox_attr.op = UDMA_CMD_CREATE_JETTY_GROUP_CONTEXT;
	ret = post_mailbox_update_ctx(udma_dev, &ctx, sizeof(ctx), &mbox_attr);
	if (ret) {
		dev_err(udma_dev->dev,
			"post mailbox update jetty ctx failed, ret = %d.\n", ret);
		goto err_post_mailbox;
	}

	mutex_init(&jetty_grp->valid_lock);
	refcount_set(&jetty_grp->ae_refcount, 1);
	init_completion(&jetty_grp->ae_comp);

	if (dfx_switch)
		udma_dfx_store_id(udma_dev, &udma_dev->dfx_info->jetty_grp,
				  jetty_grp->jetty_grp_id, "jetty_grp");

	return &jetty_grp->ubcore_jetty_grp;
err_post_mailbox:
	xa_erase(&udma_dev->jetty_grp_table.xa, jetty_grp->jetty_grp_id);
err_store_jetty_grp:
	udma_id_free(&udma_dev->jetty_grp_table.ida_table,
		     jetty_grp->jetty_grp_id);
err_alloc_jetty_grp_id:
	kfree(jetty_grp);

	return NULL;
}

int udma_delete_jetty_grp(struct ubcore_jetty_group *jetty_grp)
{
	struct udma_jetty_grp *udma_jetty_grp = to_udma_jetty_grp(jetty_grp);
	struct udma_dev *udma_dev = to_udma_dev(jetty_grp->ub_dev);
	struct ubase_mbx_attr mbox_attr = {};
	int ret;

	mbox_attr.tag = udma_jetty_grp->jetty_grp_id;
	mbox_attr.op = UDMA_CMD_DESTROY_JETTY_GROUP_CONTEXT;
	ret = post_mailbox_update_ctx(udma_dev, NULL, 0, &mbox_attr);
	if (ret) {
		dev_err(udma_dev->dev,
			"post mailbox destroy jetty group failed, ret = %d.\n", ret);
		return ret;
	}

	xa_erase(&udma_dev->jetty_grp_table.xa, udma_jetty_grp->jetty_grp_id);

	if (refcount_dec_and_test(&udma_jetty_grp->ae_refcount))
		complete(&udma_jetty_grp->ae_comp);
	wait_for_completion(&udma_jetty_grp->ae_comp);

	if (dfx_switch)
		udma_dfx_delete_id(udma_dev, &udma_dev->dfx_info->jetty_grp,
				   udma_jetty_grp->jetty_grp_id);

	if (udma_jetty_grp->valid != 0)
		dev_err(udma_dev->dev,
			"jetty group been used, jetty valid is 0x%x.\n",
			udma_jetty_grp->valid);

	mutex_destroy(&udma_jetty_grp->valid_lock);
	udma_id_free(&udma_dev->jetty_grp_table.ida_table,
		     udma_jetty_grp->jetty_grp_id);
	udma_adv_id_free(&udma_dev->jetty_table.bitmap_table,
			 udma_jetty_grp->start_jetty_id, true);
	kfree(udma_jetty_grp);

	return ret;
}

int udma_flush_jetty(struct ubcore_jetty *jetty, int cr_cnt, struct ubcore_cr *cr)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);
	struct udma_jetty_queue *sq = &udma_jetty->sq;
	int n_flushed;

	if (!sq->flush_flag)
		return 0;

	if (!sq->lock_free)
		spin_lock(&sq->lock);

	for (n_flushed = 0; n_flushed < cr_cnt; n_flushed++) {
		if (sq->ci == sq->pi)
			break;
		udma_flush_sq(udma_dev, sq, cr + n_flushed);
	}

	if (!sq->lock_free)
		spin_unlock(&sq->lock);

	return n_flushed;
}

int udma_post_jetty_send_wr(struct ubcore_jetty *jetty, struct ubcore_jfs_wr *wr,
			    struct ubcore_jfs_wr **bad_wr)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);
	int ret;

	ret = udma_post_sq_wr(udma_dev, &udma_jetty->sq, wr, bad_wr);
	if (ret)
		dev_err(udma_dev->dev,
			"jetty post sq wr failed, ret = %d, jetty id = %u.\n",
			ret, udma_jetty->sq.id);

	return ret;
}

int udma_post_jetty_recv_wr(struct ubcore_jetty *jetty, struct ubcore_jfr_wr *wr,
			    struct ubcore_jfr_wr **bad_wr)
{
	struct udma_dev *udma_dev = to_udma_dev(jetty->ub_dev);
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);
	struct ubcore_jfr *jfr;
	int ret;

	jfr = &udma_jetty->jfr->ubcore_jfr;
	ret = udma_post_jfr_wr(jfr, wr, bad_wr);
	if (ret)
		dev_err(udma_dev->dev,
			"jetty post jfr wr failed, ret = %d, jetty id = %u.\n",
			ret, udma_jetty->sq.id);

	return ret;
}

int udma_unbind_jetty(struct ubcore_jetty *jetty)
{
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);

	udma_jetty->sq.rc_tjetty = NULL;

	return 0;
}

struct ubcore_tjetty *udma_import_jetty_ex(struct ubcore_device *ub_dev,
					   struct ubcore_tjetty_cfg *cfg,
					   struct ubcore_active_tp_cfg *active_tp_cfg,
					   struct ubcore_udata *udata)
{
	struct udma_dev *udma_dev = to_udma_dev(ub_dev);
	struct udma_target_jetty *tjetty;
	int ret = 0;

	if (cfg->type != UBCORE_JETTY_GROUP && cfg->type != UBCORE_JETTY) {
		dev_err(udma_dev->dev,
			"the jetty of the type %u cannot be imported in exp.\n",
			cfg->type);
		return NULL;
	}

	ret = udma_check_jetty_grp_info(cfg, udma_dev);
	if (ret)
		return NULL;

	tjetty = kzalloc(sizeof(*tjetty), GFP_KERNEL);
	if (!tjetty)
		return NULL;

	if (cfg->flag.bs.token_policy != UBCORE_TOKEN_NONE) {
		tjetty->token_value = cfg->token_value.token;
		tjetty->token_value_valid = true;
	}

	udma_swap_endian(cfg->id.eid.raw, tjetty->le_eid.raw, UBCORE_EID_SIZE);

	return &tjetty->ubcore_tjetty;
}

int udma_bind_jetty_ex(struct ubcore_jetty *jetty,
		       struct ubcore_tjetty *tjetty,
		       struct ubcore_active_tp_cfg *active_tp_cfg,
		       struct ubcore_udata *udata)
{
	struct udma_jetty *udma_jetty = to_udma_jetty(jetty);

	udma_jetty->sq.rc_tjetty = tjetty;

	return 0;
}

module_param(well_known_jetty_pgsz_check, bool, 0444);
MODULE_PARM_DESC(well_known_jetty_pgsz_check,
		"Whether check the system page size. default: true(true:check; false: not check)");
