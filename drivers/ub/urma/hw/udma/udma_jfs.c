// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt
#define pr_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/random.h>
#include <linux/mm.h>
#include <ub/urma/ubcore_uapi.h>
#include "udma_common.h"
#include "udma_dev.h"
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_jetty.h"
#include "udma_segment.h"
#include "udma_jfs.h"

int udma_alloc_u_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct udma_create_jetty_ucmd *ucmd)
{
	int ret;

	if (ucmd->sqe_bb_cnt == 0 || ucmd->buf_len == 0) {
		dev_err(dev->dev, "invalid param, sqe_bb_cnt=%u, buf_len=%u.\n",
			ucmd->sqe_bb_cnt, ucmd->buf_len);
		return -EINVAL;
	}

	sq->sqe_bb_cnt = ucmd->sqe_bb_cnt;
	sq->buf.entry_cnt = ucmd->buf_len >> WQE_BB_SIZE_SHIFT;
	if (sq->non_pin) {
		sq->buf.addr = ucmd->buf_addr;
	} else {
		ret = pin_queue_addr(dev, ucmd->buf_addr, ucmd->buf_len, &sq->buf);
		if (ret) {
			dev_err(dev->dev,
				"failed to pin jetty/jfs queue addr, ret = %d.\n",
				ret);
			return ret;
		}
	}

	return 0;
}

int udma_alloc_k_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct ubcore_jfs_cfg *jfs_cfg)
{
	uint32_t wqe_bb_depth;
	uint32_t sqe_bb_cnt;
	uint32_t size;
	int ret;

	if (!jfs_cfg->flag.bs.lock_free)
		spin_lock_init(&sq->lock);

	sq->max_inline_size = jfs_cfg->max_inline_data;
	sq->max_sge_num = jfs_cfg->max_sge;
	sq->tid = dev->tid;
	sq->lock_free = jfs_cfg->flag.bs.lock_free;

	sqe_bb_cnt = sq_cal_wqebb_num(SQE_WRITE_NOTIFY_CTL_LEN, jfs_cfg->max_sge);
	sq->sqe_bb_cnt = sqe_bb_cnt > (uint32_t)MAX_WQEBB_NUM ? (uint32_t)MAX_WQEBB_NUM :
			 sqe_bb_cnt;

	wqe_bb_depth = roundup_pow_of_two(sq->sqe_bb_cnt * jfs_cfg->depth);
	sq->buf.entry_size = UDMA_JFS_WQEBB_SIZE;
	size = ALIGN(wqe_bb_depth * sq->buf.entry_size, UDMA_HW_PAGE_SIZE);
	sq->buf.entry_cnt = size >> WQE_BB_SIZE_SHIFT;

	ret = udma_k_alloc_buf(dev, size, &sq->buf);
	if (ret) {
		dev_err(dev->dev,
			"failed to alloc jetty (%u) sq buf when size = %u.\n", sq->id, size);
		return ret;
	}

	sq->wrid = kcalloc(1, sq->buf.entry_cnt * sizeof(uint64_t), GFP_KERNEL);
	if (!sq->wrid) {
		udma_k_free_buf(dev, size, &sq->buf);
		dev_err(dev->dev,
			"failed to alloc wrid for jfs id = %u when entry cnt = %u.\n",
			sq->id, sq->buf.entry_cnt);
		return -ENOMEM;
	}

	udma_alloc_kernel_db(dev, sq);
	sq->kva_curr = sq->buf.kva;

	return 0;
}

void udma_free_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq)
{
	uint32_t size;

	if (sq->buf.kva) {
		size = sq->buf.entry_cnt * sq->buf.entry_size;
		udma_k_free_buf(dev, size, &sq->buf);
		kfree(sq->wrid);
		return;
	}
	if (sq->non_pin)
		return;

	unpin_queue_addr(sq->buf.umem);
}

void udma_init_jfsc(struct udma_dev *dev, struct ubcore_jfs_cfg *cfg,
		    struct udma_jfs *jfs, void *mb_buf)
{
	struct udma_jetty_ctx *ctx = (struct udma_jetty_ctx *)mb_buf;
	uint8_t i;

	ctx->state = JETTY_READY;
	ctx->jfs_mode = JFS;
	ctx->type = to_udma_type(cfg->trans_mode);
	ctx->sl = dev->udma_sl[UDMA_DEFAULT_SL_NUM];
	if (ctx->type == JETTY_RM || ctx->type == JETTY_RC) {
		for (i = 0; i < dev->udma_total_sl_num; i++)
			if (cfg->priority == dev->udma_sl[i])
				ctx->sl = cfg->priority;
	} else if (ctx->type == JETTY_UM) {
		ctx->sl = dev->unic_sl[UDMA_DEFAULT_SL_NUM];
		for (i = 0; i < dev->unic_sl_num; i++)
			if (cfg->priority == dev->unic_sl[i])
				ctx->sl = cfg->priority;
	}
	ctx->sqe_base_addr_l = (jfs->sq.buf.addr >> SQE_VA_L_OFFSET) &
			       (uint32_t)SQE_VA_L_VALID_BIT;
	ctx->sqe_base_addr_h = (jfs->sq.buf.addr >> SQE_VA_H_OFFSET) &
			       (uint32_t)SQE_VA_H_VALID_BIT;
	ctx->sqe_token_id_l = jfs->sq.tid & (uint32_t)SQE_TOKEN_ID_L_MASK;
	ctx->sqe_token_id_h = (jfs->sq.tid >> SQE_TOKEN_ID_H_OFFSET) &
			      (uint32_t)SQE_TOKEN_ID_H_MASK;
	ctx->sqe_bb_shift = ilog2(roundup_pow_of_two(jfs->sq.buf.entry_cnt));
	ctx->tx_jfcn = cfg->jfc->id;
	ctx->ta_timeout = to_ta_timeout(cfg->err_timeout);

	if (!!(dev->caps.feature & UDMA_CAP_FEATURE_RNR_RETRY))
		ctx->rnr_retry_num = cfg->rnr_retry;

	ctx->user_data_l = jfs->jfs_addr;
	ctx->user_data_h = jfs->jfs_addr >> UDMA_USER_DATA_H_OFFSET;
	ctx->seid_idx = cfg->eid_index;
	ctx->err_mode = cfg->flag.bs.error_suspend;
	ctx->cmp_odr = cfg->flag.bs.outorder_comp;
	ctx->avail_sgmt_ost = AVAIL_SGMT_OST_INIT;
	ctx->pi_type = jfs->pi_type;
	ctx->sqe_pld_tokenid = jfs->sq.tid & (uint32_t)SQE_PLD_TOKEN_ID_MASK;
	ctx->next_send_ssn = get_random_u16();
	ctx->next_rcv_ssn = ctx->next_send_ssn;
}

void udma_dfx_store_jfs_id(struct udma_dev *udma_dev, struct udma_jfs *udma_jfs)
{
	struct udma_dfx_jfs *jfs;
	int ret;

	jfs = (struct udma_dfx_jfs *)xa_load(&udma_dev->dfx_info->jfs.table,
					     udma_jfs->sq.id);
	if (jfs) {
		dev_warn(udma_dev->dev, "jfs_id(%u) already exists in DFX.\n",
			 udma_jfs->sq.id);
		return;
	}

	jfs = kzalloc(sizeof(*jfs), GFP_KERNEL);
	if (!jfs)
		return;

	jfs->id = udma_jfs->sq.id;
	jfs->depth = udma_jfs->sq.buf.entry_cnt / udma_jfs->sq.sqe_bb_cnt;

	write_lock(&udma_dev->dfx_info->jfs.rwlock);
	ret = xa_err(xa_store(&udma_dev->dfx_info->jfs.table, udma_jfs->sq.id,
			      jfs, GFP_KERNEL));
	if (ret) {
		write_unlock(&udma_dev->dfx_info->jfs.rwlock);
		dev_err(udma_dev->dev, "store jfs_id(%u) to table failed in DFX.\n",
			udma_jfs->sq.id);
		kfree(jfs);
		return;
	}

	++udma_dev->dfx_info->jfs.cnt;
	write_unlock(&udma_dev->dfx_info->jfs.rwlock);
}

static int udma_create_hw_jfs_ctx(struct udma_dev *dev, struct udma_jfs *jfs,
				    struct ubcore_jfs_cfg *cfg)
{
	struct ubase_mbx_attr attr = {};
	struct udma_jetty_ctx ctx = {};
	int ret;

	if (cfg->priority >= UDMA_MAX_PRIORITY) {
		dev_err(dev->dev, "kernel mode jfs priority is out of range, priority is %u.\n",
			cfg->priority);
		return -EINVAL;
	}

	udma_init_jfsc(dev, cfg, jfs, &ctx);
	attr.tag = jfs->sq.id;
	attr.op = UDMA_CMD_CREATE_JFS_CONTEXT;
	ret = post_mailbox_update_ctx(dev, &ctx, sizeof(ctx), &attr);
	if (ret) {
		dev_err(dev->dev, "failed to upgrade JFSC, ret = %d.\n", ret);
		return ret;
	}

	return 0;
}

static int udma_get_user_jfs_cmd(struct udma_dev *dev, struct udma_jfs *jfs,
				 struct ubcore_udata *udata,
				 struct udma_create_jetty_ucmd *ucmd)
{
	struct udma_context *uctx;
	unsigned long byte;

	if (udata) {
		if (!udata->udrv_data) {
			dev_err(dev->dev, "udrv_data is null.\n");
			return -EINVAL;
		}

		if (!udata->udrv_data->in_addr || udata->udrv_data->in_len < sizeof(*ucmd)) {
			dev_err(dev->dev, "jfs in_len %u or addr is invalid.\n",
				udata->udrv_data->in_len);
			return -EINVAL;
		}

		byte = copy_from_user(ucmd, (void *)(uintptr_t)udata->udrv_data->in_addr,
				      sizeof(*ucmd));
		if (byte) {
			dev_err(dev->dev,
				"failed to copy jfs udata, ret = %lu.\n", byte);
			return -EFAULT;
		}

		uctx = to_udma_context(udata->uctx);
		jfs->sq.tid = uctx->tid;
		jfs->jfs_addr = ucmd->jetty_addr;
		jfs->pi_type = ucmd->pi_type;
		jfs->sq.non_pin = ucmd->non_pin;
		jfs->sq.jetty_type = (enum udma_jetty_type)ucmd->jetty_type;
		jfs->sq.id = ucmd->jfs_id;
	} else {
		jfs->jfs_addr = (uintptr_t)&jfs->sq;
		jfs->sq.jetty_type = (enum udma_jetty_type)UDMA_URMA_NORMAL_JETTY_TYPE;
	}

	return 0;
}

static int udma_alloc_jfs_sq(struct udma_dev *dev, struct ubcore_jfs_cfg *cfg,
			      struct udma_jfs *jfs, struct ubcore_udata *udata)
{
	struct udma_create_jetty_ucmd ucmd = {};
	int ret;

	ret = udma_get_user_jfs_cmd(dev, jfs, udata, &ucmd);
	if (ret)
		goto err_get_user_cmd;

	ret = alloc_jetty_id(dev, &jfs->sq, jfs->sq.id, NULL);
	if (ret) {
		dev_err(dev->dev, "failed to alloc_id.\n");
		goto err_alloc_id;
	}
	jfs->ubcore_jfs.jfs_id.id = jfs->sq.id;
	jfs->ubcore_jfs.jfs_cfg = *cfg;
	udma_set_query_flush_time(&jfs->sq, cfg->err_timeout);

	ret = xa_err(xa_store(&dev->jetty_table.xa, jfs->sq.id, &jfs->sq, GFP_KERNEL));
	if (ret) {
		dev_err(dev->dev, "failed to store_sq(%u), ret=%d.", jfs->sq.id, ret);
		goto err_store_sq;
	}

	ret = udata ? udma_alloc_u_sq_buf(dev, &jfs->sq, &ucmd) :
		udma_alloc_k_sq_buf(dev, &jfs->sq, cfg);
	if (ret)
		goto err_alloc_sq_buf;

	jfs->sq.trans_mode = cfg->trans_mode;

	return ret;

err_alloc_sq_buf:
	xa_erase(&dev->jetty_table.xa, jfs->sq.id);
err_store_sq:
	if (jfs->sq.id < dev->caps.jetty.start_idx)
		udma_id_free(&dev->rsvd_jetty_ida_table, jfs->sq.id);
	else
		udma_adv_id_free(&dev->jetty_table.bitmap_table,
				 jfs->sq.id, false);
err_alloc_id:
err_get_user_cmd:
	return ret;
}

struct ubcore_jfs *udma_create_jfs(struct ubcore_device *ub_dev,
				   struct ubcore_jfs_cfg *cfg,
				   struct ubcore_udata *udata)
{
	struct udma_dev *dev = to_udma_dev(ub_dev);
	struct udma_jfs *jfs;
	int ret;

	if (cfg->trans_mode == UBCORE_TP_RC) {
		dev_err(dev->dev, "jfs not support RC transmode.\n");
		return NULL;
	}

	jfs = kcalloc(1, sizeof(*jfs), GFP_KERNEL);
	if (!jfs)
		return NULL;

	ret = udma_alloc_jfs_sq(dev, cfg, jfs, udata);
	if (ret) {
		dev_err(dev->dev, "failed to alloc_jfs_sq, ret = %d.\n", ret);
		goto err_alloc_sq;
	}

	ret = udma_create_hw_jfs_ctx(dev, jfs, cfg);
	if (ret) {
		dev_err(dev->dev,
			"post mailbox create jfs ctx failed, ret = %d.\n", ret);
		goto err_create_hw_jfs;
	}

	jfs->mode = UDMA_NORMAL_JFS_TYPE;
	jfs->sq.state = UBCORE_JETTY_STATE_READY;
	refcount_set(&jfs->ae_refcount, 1);
	init_completion(&jfs->ae_comp);
	if (dfx_switch)
		udma_dfx_store_jfs_id(dev, jfs);

	return &jfs->ubcore_jfs;

err_create_hw_jfs:
	udma_free_sq_buf(dev, &jfs->sq);
	xa_erase(&dev->jetty_table.xa, jfs->sq.id);
	if (jfs->sq.id < dev->caps.jetty.start_idx)
		udma_id_free(&dev->rsvd_jetty_ida_table, jfs->sq.id);
	else
		udma_adv_id_free(&dev->jetty_table.bitmap_table,
				 jfs->sq.id, false);
err_alloc_sq:
	kfree(jfs);
	return NULL;
}

static void udma_free_jfs(struct ubcore_jfs *jfs)
{
	struct udma_dev *dev = to_udma_dev(jfs->ub_dev);
	struct udma_jfs *ujfs = to_udma_jfs(jfs);

	xa_erase(&dev->jetty_table.xa, ujfs->sq.id);

	if (refcount_dec_and_test(&ujfs->ae_refcount))
		complete(&ujfs->ae_comp);
	wait_for_completion(&ujfs->ae_comp);

	if (dfx_switch)
		udma_dfx_delete_id(dev, &dev->dfx_info->jfs, jfs->jfs_id.id);

	if (ujfs->mode == UDMA_NORMAL_JFS_TYPE)
		udma_free_sq_buf(dev, &ujfs->sq);
	else
		kfree(ujfs->sq.wrid);

	if (ujfs->sq.id < dev->caps.jetty.start_idx)
		udma_id_free(&dev->rsvd_jetty_ida_table, ujfs->sq.id);
	else
		udma_adv_id_free(&dev->jetty_table.bitmap_table,
				 ujfs->sq.id, false);

	kfree(ujfs);
}

int udma_destroy_jfs(struct ubcore_jfs *jfs)
{
	struct udma_dev *dev = to_udma_dev(jfs->ub_dev);
	struct udma_jfs *ujfs = to_udma_jfs(jfs);
	int ret;

	if (!ujfs->ue_rx_closed && udma_close_ue_rx(dev, true, true, false, 0)) {
		dev_err(dev->dev, "close ue rx failed when destroying jfs.\n");
		return -EINVAL;
	}

	ret = udma_modify_and_destroy_jetty(dev, &ujfs->sq);
	if (ret) {
		dev_info(dev->dev, "udma modify error and destroy jfs failed, id: %u.\n",
			 jfs->jfs_id.id);
		if (!ujfs->ue_rx_closed)
			udma_open_ue_rx(dev, true, true, false, 0);
		return ret;
	}

	udma_free_jfs(jfs);
	udma_open_ue_rx(dev, true, true, false, 0);

	return 0;
}
