// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/ummu_core.h>
#include <ub/urma/ubcore_uapi.h>
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_common.h"
#include "udma_jetty.h"
#include "udma_jfr.h"
#include "udma_jfs.h"
#include "udma_ctx.h"
#include "udma_db.h"
#include <ub/urma/udma/udma_ctl.h>
#include "udma_jfc.h"

static void udma_construct_jfc_ctx(struct udma_dev *dev,
				   struct udma_jfc *jfc,
				   struct udma_jfc_ctx *ctx)
{
	memset(ctx, 0, sizeof(struct udma_jfc_ctx));

	ctx->state = UDMA_JFC_STATE_VALID;
	if (jfc_arm_mode)
		ctx->arm_st = UDMA_CTX_NO_ARMED;
	else
		ctx->arm_st = UDMA_CTX_ALWAYS_ARMED;
	ctx->shift = jfc->cq_shift - UDMA_JFC_DEPTH_SHIFT_BASE;
	ctx->jfc_type = UDMA_NORMAL_JFC_TYPE;
	if (!!(dev->caps.feature & UDMA_CAP_FEATURE_JFC_INLINE))
		ctx->inline_en = jfc->inline_en;
	ctx->cqe_va_l = jfc->buf.addr >> CQE_VA_L_OFFSET;
	ctx->cqe_va_h = jfc->buf.addr >> CQE_VA_H_OFFSET;
	ctx->cqe_token_id = jfc->tid;

	if (cqe_mode)
		ctx->cq_cnt_mode = UDMA_CQE_CNT_MODE_BY_CI_PI_GAP;
	else
		ctx->cq_cnt_mode = UDMA_CQE_CNT_MODE_BY_COUNT;

	ctx->ceqn = jfc->ceqn;
	if (jfc->stars_en) {
		ctx->stars_en = UDMA_STARS_SWITCH;
		ctx->record_db_en = UDMA_NO_RECORD_EN;
	} else {
		ctx->record_db_en = UDMA_RECORD_EN;
		ctx->record_db_addr_l = jfc->db.db_addr >> UDMA_DB_L_OFFSET;
		ctx->record_db_addr_h = jfc->db.db_addr >> UDMA_DB_H_OFFSET;
	}
}

void udma_init_jfc_param(struct ubcore_jfc_cfg *cfg,
			 struct udma_jfc *jfc)
{
	jfc->base.id = jfc->jfcn;
	jfc->base.jfc_cfg = *cfg;
	jfc->ceqn = cfg->ceqn;
	jfc->lock_free = cfg->flag.bs.lock_free;
	jfc->inline_en = cfg->flag.bs.jfc_inline;
	jfc->cq_shift = ilog2(jfc->buf.entry_cnt);
}

int udma_check_jfc_cfg(struct udma_dev *dev, struct udma_jfc *jfc,
		       struct ubcore_jfc_cfg *cfg)
{
	if (!jfc->buf.entry_cnt || jfc->buf.entry_cnt > dev->caps.jfc.depth) {
		dev_err(dev->dev, "invalid jfc depth = %u, cap depth = %u.\n",
			jfc->buf.entry_cnt, dev->caps.jfc.depth);
		return -EINVAL;
	}

	if (jfc->buf.entry_cnt < UDMA_JFC_DEPTH_MIN)
		jfc->buf.entry_cnt = UDMA_JFC_DEPTH_MIN;

	if (cfg->ceqn >= dev->caps.comp_vector_cnt) {
		dev_err(dev->dev, "invalid ceqn = %u, cap ceq cnt = %u.\n",
			cfg->ceqn, dev->caps.comp_vector_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_get_cmd_from_user(struct udma_create_jfc_ucmd *ucmd,
				  struct udma_dev *dev,
				  struct ubcore_udata *udata,
				  struct udma_jfc *jfc)
{
#define UDMA_JFC_CQE_SHIFT 6
	unsigned long byte;

	if (!udata->udrv_data || !udata->udrv_data->in_addr) {
		dev_err(dev->dev, "jfc udrv_data or in_addr is null.\n");
		return -EINVAL;
	}

	byte = copy_from_user(ucmd, (void *)(uintptr_t)udata->udrv_data->in_addr,
			      min(udata->udrv_data->in_len,
				  (uint32_t)sizeof(*ucmd)));
	if (byte) {
		dev_err(dev->dev,
			"failed to copy udata from user, byte = %lu.\n", byte);
		return -EFAULT;
	}

	jfc->mode = ucmd->mode;
	jfc->ctx = to_udma_context(udata->uctx);
	if (jfc->mode > UDMA_NORMAL_JFC_TYPE && jfc->mode < UDMA_KERNEL_STARS_JFC_TYPE) {
		jfc->buf.entry_cnt = ucmd->buf_len;
		return 0;
	}

	jfc->db.db_addr = ucmd->db_addr;
	jfc->buf.entry_cnt = ucmd->buf_len >> UDMA_JFC_CQE_SHIFT;

	return 0;
}

static int udma_get_jfc_buf(struct udma_dev *dev, struct udma_create_jfc_ucmd *ucmd,
			    struct ubcore_udata *udata, struct udma_jfc *jfc)
{
	struct udma_context *uctx;
	uint32_t size;
	int ret = 0;

	if (udata) {
		ret = pin_queue_addr(dev, ucmd->buf_addr, ucmd->buf_len, &jfc->buf);
		if (ret) {
			dev_err(dev->dev, "failed to pin queue for jfc, ret = %d.\n", ret);
			return ret;
		}
		uctx = to_udma_context(udata->uctx);
		jfc->tid = uctx->tid;
		ret = udma_pin_sw_db(uctx, &jfc->db);
		if (ret) {
			dev_err(dev->dev, "failed to pin sw db for jfc, ret = %d.\n", ret);
			unpin_queue_addr(jfc->buf.umem);
		}

		return ret;
	}

	if (!jfc->lock_free)
		spin_lock_init(&jfc->lock);
	jfc->buf.entry_size = dev->caps.cqe_size;
	jfc->tid = dev->tid;
	size = jfc->buf.entry_size * jfc->buf.entry_cnt;

	ret = udma_k_alloc_buf(dev, size, &jfc->buf);
	if (ret) {
		dev_err(dev->dev, "failed to alloc buffer for jfc.\n");
		return ret;
	}

	ret = udma_alloc_sw_db(dev, &jfc->db, UDMA_JFC_TYPE_DB);
	if (ret) {
		dev_err(dev->dev, "failed to alloc sw db for jfc(%u).\n", jfc->jfcn);
		udma_k_free_buf(dev, size, &jfc->buf);
		return -ENOMEM;
	}

	return ret;
}

static void udma_free_jfc_buf(struct udma_dev *dev, struct udma_jfc *jfc)
{
	struct udma_context *uctx;
	uint32_t size;

	if (jfc->buf.kva) {
		size = jfc->buf.entry_size * jfc->buf.entry_cnt;
		udma_k_free_buf(dev, size, &jfc->buf);
	} else if (jfc->buf.umem) {
		uctx = to_udma_context(jfc->base.uctx);
		unpin_queue_addr(jfc->buf.umem);
	}

	if (jfc->db.page) {
		uctx = to_udma_context(jfc->base.uctx);
		udma_unpin_sw_db(uctx, &jfc->db);
	} else if (jfc->db.kpage) {
		udma_free_sw_db(dev, &jfc->db);
	}
}

int udma_post_create_jfc_mbox(struct udma_dev *dev, struct udma_jfc *jfc)
{
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;
	int ret;

	mailbox = udma_alloc_cmd_mailbox(dev);
	if (!mailbox) {
		dev_err(dev->dev, "failed to alloc mailbox for JFCC.\n");
		return -ENOMEM;
	}

	if (jfc->mode == UDMA_STARS_JFC_TYPE || jfc->mode == UDMA_CCU_JFC_TYPE ||
	    jfc->mode == UDMA_KERNEL_STARS_JFC_TYPE)
		jfc->stars_en = true;
	udma_construct_jfc_ctx(dev, jfc, (struct udma_jfc_ctx *)mailbox->buf);

	mbox_attr.tag = jfc->jfcn;
	mbox_attr.op = UDMA_CMD_CREATE_JFC_CONTEXT;
	ret = udma_post_mbox(dev, mailbox, &mbox_attr);
	if (ret)
		dev_err(dev->dev,
			"failed to post create JFC mailbox, ret = %d.\n", ret);

	udma_free_cmd_mailbox(dev, mailbox);

	return ret;
}

static int udma_verify_stars_jfc_param(struct udma_dev *dev,
				       struct udma_ex_jfc_addr *jfc_addr,
				       struct udma_jfc *jfc)
{
	uint32_t size;

	if (!jfc_addr->cq_addr) {
		dev_err(dev->dev, "CQE addr is wrong.\n");
		return -ENOMEM;
	}
	if (!jfc_addr->cq_len) {
		dev_err(dev->dev, "CQE len is wrong.\n");
		return -EINVAL;
	}

	size = jfc->buf.entry_cnt * dev->caps.cqe_size;

	if (size != jfc_addr->cq_len) {
		dev_err(dev->dev, "cqe buff size is wrong, buf size = %u.\n", size);
		return -EINVAL;
	}

	return 0;
}

static int udma_get_stars_jfc_buf(struct udma_dev *dev, struct udma_jfc *jfc)
{
	struct udma_ex_jfc_addr *jfc_addr = &dev->cq_addr_array[jfc->mode];
	int ret;

	jfc->tid = dev->tid;

	ret = udma_verify_stars_jfc_param(dev, jfc_addr, jfc);
	if (ret)
		return ret;

	jfc->buf.addr = (dma_addr_t)(uintptr_t)jfc_addr->cq_addr;

	ret = udma_alloc_sw_db(dev, &jfc->db, UDMA_JFC_TYPE_DB);
	if (ret) {
		dev_err(dev->dev, "failed to alloc sw db for jfc(%u).\n", jfc->jfcn);
		return -ENOMEM;
	}

	return ret;
}

static int udma_create_stars_jfc(struct udma_dev *dev,
				 struct udma_jfc *jfc,
				 struct ubcore_jfc_cfg *cfg,
				 struct ubcore_udata *udata,
				 struct udma_create_jfc_ucmd *ucmd)
{
	unsigned long flags_store;
	unsigned long flags_erase;
	int ret;

	ret = udma_id_alloc_auto_grow(dev, &dev->jfc_table.ida_table, &jfc->jfcn);
	if (ret) {
		dev_err(dev->dev, "failed to alloc id for stars JFC.\n");
		return -ENOMEM;
	}

	udma_init_jfc_param(cfg, jfc);
	xa_lock_irqsave(&dev->jfc_table.xa, flags_store);
	ret = xa_err(__xa_store(&dev->jfc_table.xa, jfc->jfcn, jfc, GFP_ATOMIC));
	xa_unlock_irqrestore(&dev->jfc_table.xa, flags_store);
	if (ret) {
		dev_err(dev->dev,
			"failed to stored stars jfc id to jfc_table, jfcn: %u.\n",
			jfc->jfcn);
		goto err_store_jfcn;
	}

	ret = udma_get_stars_jfc_buf(dev, jfc);
	if (ret)
		goto err_alloc_cqc;

	ret = udma_post_create_jfc_mbox(dev, jfc);
	if (ret)
		goto err_get_jfc_buf;

	refcount_set(&jfc->event_refcount, 1);
	init_completion(&jfc->event_comp);

	if (dfx_switch)
		udma_dfx_store_id(dev, &dev->dfx_info->jfc, jfc->jfcn, "jfc");

	return 0;

err_get_jfc_buf:
	udma_free_sw_db(dev, &jfc->db);
err_alloc_cqc:
	xa_lock_irqsave(&dev->jfc_table.xa, flags_erase);
	__xa_erase(&dev->jfc_table.xa, jfc->jfcn);
	xa_unlock_irqrestore(&dev->jfc_table.xa, flags_erase);
err_store_jfcn:
	udma_id_free(&dev->jfc_table.ida_table, jfc->jfcn);

	return -ENOMEM;
}

struct ubcore_jfc *udma_create_jfc(struct ubcore_device *ubcore_dev,
				   struct ubcore_jfc_cfg *cfg,
				   struct ubcore_udata *udata)
{
	struct udma_dev *dev = to_udma_dev(ubcore_dev);
	struct udma_create_jfc_ucmd ucmd = {};
	unsigned long flags_store;
	unsigned long flags_erase;
	struct udma_jfc *jfc;
	int ret;

	jfc = kzalloc(sizeof(struct udma_jfc), GFP_KERNEL);
	if (!jfc)
		return NULL;

	if (udata) {
		ret = udma_get_cmd_from_user(&ucmd, dev, udata, jfc);
		if (ret)
			goto err_get_cmd;
	} else {
		jfc->arm_sn = 1;
		jfc->buf.entry_cnt = cfg->depth ? roundup_pow_of_two(cfg->depth) : cfg->depth;
	}

	ret = udma_check_jfc_cfg(dev, jfc, cfg);
	if (ret)
		goto err_get_cmd;

	if (jfc->mode == UDMA_STARS_JFC_TYPE || jfc->mode == UDMA_CCU_JFC_TYPE) {
		if (udma_create_stars_jfc(dev, jfc, cfg, udata, &ucmd))
			goto err_get_cmd;
		return &jfc->base;
	}

	ret = udma_id_alloc_auto_grow(dev, &dev->jfc_table.ida_table,
				      &jfc->jfcn);
	if (ret)
		goto err_get_cmd;

	udma_init_jfc_param(cfg, jfc);

	xa_lock_irqsave(&dev->jfc_table.xa, flags_store);
	ret = xa_err(__xa_store(&dev->jfc_table.xa, jfc->jfcn, jfc, GFP_ATOMIC));
	xa_unlock_irqrestore(&dev->jfc_table.xa, flags_store);
	if (ret) {
		dev_err(dev->dev,
			"failed to stored jfc id to jfc_table, jfcn: %u.\n",
			jfc->jfcn);
		goto err_store_jfcn;
	}

	ret = udma_get_jfc_buf(dev, &ucmd, udata, jfc);
	if (ret)
		goto err_get_jfc_buf;

	ret = udma_post_create_jfc_mbox(dev, jfc);
	if (ret)
		goto err_alloc_cqc;

	refcount_set(&jfc->event_refcount, 1);
	init_completion(&jfc->event_comp);

	if (dfx_switch)
		udma_dfx_store_id(dev, &dev->dfx_info->jfc, jfc->jfcn, "jfc");

	return &jfc->base;

err_alloc_cqc:
	jfc->base.uctx = (udata == NULL ? NULL : udata->uctx);
	udma_free_jfc_buf(dev, jfc);
err_get_jfc_buf:
	xa_lock_irqsave(&dev->jfc_table.xa, flags_erase);
	__xa_erase(&dev->jfc_table.xa, jfc->jfcn);
	xa_unlock_irqrestore(&dev->jfc_table.xa, flags_erase);
err_store_jfcn:
	udma_id_free(&dev->jfc_table.ida_table, jfc->jfcn);
err_get_cmd:
	kfree(jfc);
	return NULL;
}

static int udma_post_destroy_jfc_mbox(struct udma_dev *dev, uint32_t jfcn)
{
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;
	struct udma_jfc_ctx *ctx;
	int ret;

	mailbox = udma_alloc_cmd_mailbox(dev);
	if (!mailbox) {
		dev_err(dev->dev, "failed to alloc mailbox for JFCC.\n");
		return -ENOMEM;
	}

	ctx = (struct udma_jfc_ctx *)mailbox->buf;

	mbox_attr.tag = jfcn;
	mbox_attr.op = UDMA_CMD_DESTROY_JFC_CONTEXT;
	ret = udma_post_mbox(dev, mailbox, &mbox_attr);
	if (ret)
		dev_err(dev->dev,
			"failed to post destroy JFC mailbox, ret = %d.\n",
			ret);

	udma_free_cmd_mailbox(dev, mailbox);

	return ret;
}

static int udma_query_jfc_destroy_done(struct udma_dev *dev, uint32_t jfcn)
{
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;
	struct udma_jfc_ctx *jfc_ctx;
	int ret;

	mbox_attr.tag = jfcn;
	mbox_attr.op = UDMA_CMD_QUERY_JFC_CONTEXT;
	mailbox = udma_mailbox_query_ctx(dev, &mbox_attr);
	if (!mailbox)
		return -ENOMEM;

	jfc_ctx = (struct udma_jfc_ctx *)mailbox->buf;
	ret = jfc_ctx->pi == jfc_ctx->wr_cqe_idx ? 0 : -EAGAIN;

	jfc_ctx->cqe_token_value = 0;
	jfc_ctx->remote_token_value = 0;
	udma_free_cmd_mailbox(dev, mailbox);

	return ret;
}

static int udma_destroy_and_flush_jfc(struct udma_dev *dev, uint32_t jfcn)
{
#define QUERY_MAX_TIMES 5
	uint32_t wait_times = 0;
	int ret;

	ret = udma_post_destroy_jfc_mbox(dev, jfcn);
	if (ret) {
		dev_err(dev->dev, "failed to post mbox to destroy jfc, id: %u.\n", jfcn);
		return ret;
	}

	while (true) {
		if (udma_query_jfc_destroy_done(dev, jfcn) == 0)
			return 0;
		if (wait_times > QUERY_MAX_TIMES)
			break;
		msleep(1 << wait_times);
		wait_times++;
	}
	dev_err(dev->dev, "jfc flush timed out, id: %u.\n", jfcn);

	return -EFAULT;
}

int udma_destroy_jfc(struct ubcore_jfc *jfc)
{
	struct udma_dev *dev = to_udma_dev(jfc->ub_dev);
	struct udma_jfc *ujfc = to_udma_jfc(jfc);
	unsigned long flags;
	int ret;

	ret = udma_destroy_and_flush_jfc(dev, ujfc->jfcn);
	if (ret)
		return ret;

	xa_lock_irqsave(&dev->jfc_table.xa, flags);
	__xa_erase(&dev->jfc_table.xa, ujfc->jfcn);
	xa_unlock_irqrestore(&dev->jfc_table.xa, flags);

	if (refcount_dec_and_test(&ujfc->event_refcount))
		complete(&ujfc->event_comp);
	wait_for_completion(&ujfc->event_comp);

	if (dfx_switch)
		udma_dfx_delete_id(dev, &dev->dfx_info->jfc, jfc->id);

	udma_free_jfc_buf(dev, ujfc);
	udma_id_free(&dev->jfc_table.ida_table, ujfc->jfcn);
	kfree(ujfc);

	return 0;
}

int udma_jfc_completion(struct notifier_block *nb, unsigned long jfcn,
			void *data)
{
	struct auxiliary_device *adev = (struct auxiliary_device *)data;
	struct udma_dev *udma_dev = get_udma_dev(adev);
	struct ubcore_jfc *ubcore_jfc;
	struct udma_jfc *udma_jfc;

	xa_lock(&udma_dev->jfc_table.xa);
	udma_jfc = (struct udma_jfc *)xa_load(&udma_dev->jfc_table.xa, jfcn);
	if (!udma_jfc) {
		dev_warn(udma_dev->dev,
			 "Completion event for bogus jfcn %lu.\n", jfcn);
		xa_unlock(&udma_dev->jfc_table.xa);
		return -EINVAL;
	}

	++udma_jfc->arm_sn;

	ubcore_jfc = &udma_jfc->base;
	if (ubcore_jfc->jfce_handler) {
		refcount_inc(&udma_jfc->event_refcount);
		xa_unlock(&udma_dev->jfc_table.xa);
		ubcore_jfc->jfce_handler(ubcore_jfc);
		if (refcount_dec_and_test(&udma_jfc->event_refcount))
			complete(&udma_jfc->event_comp);
	} else {
		xa_unlock(&udma_dev->jfc_table.xa);
	}

	return 0;
}

static int udma_get_cqe_period(uint16_t cqe_period)
{
	uint16_t period[] = {
		UDMA_CQE_PERIOD_0,
		UDMA_CQE_PERIOD_4,
		UDMA_CQE_PERIOD_16,
		UDMA_CQE_PERIOD_64,
		UDMA_CQE_PERIOD_256,
		UDMA_CQE_PERIOD_1024,
		UDMA_CQE_PERIOD_4096,
		UDMA_CQE_PERIOD_16384
	};
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(period); ++i) {
		if (cqe_period == period[i])
			return i;
	}

	return -EINVAL;
}

static int udma_check_jfc_attr(struct udma_dev *udma_dev, struct ubcore_jfc_attr *attr)
{
	if (!(attr->mask & (UBCORE_JFC_MODERATE_COUNT | UBCORE_JFC_MODERATE_PERIOD))) {
		dev_err(udma_dev->dev,
			"udma modify jfc mask is not set or invalid.\n");
		return -EINVAL;
	}

	if ((attr->mask & UBCORE_JFC_MODERATE_COUNT) &&
	    (attr->moderate_count >= UDMA_CQE_COALESCE_CNT_MAX)) {
		dev_err(udma_dev->dev, "udma cqe coalesce cnt %u is invalid.\n",
			attr->moderate_count);
		return -EINVAL;
	}

	if ((attr->mask & UBCORE_JFC_MODERATE_PERIOD) &&
	    (udma_get_cqe_period(attr->moderate_period) == -EINVAL)) {
		dev_err(udma_dev->dev, "udma cqe coalesce period %u is invalid.\n",
			attr->moderate_period);
		return -EINVAL;
	}

	return 0;
}

static int udma_modify_jfc_attr(struct udma_dev *dev, uint32_t jfcn,
				struct ubcore_jfc_attr *attr)
{
	struct udma_jfc_ctx *jfc_context, *ctx_mask;
	struct ubase_mbx_attr mbox_attr = {};
	struct ubase_cmd_mailbox *mailbox;
	int ret;

	mailbox = udma_alloc_cmd_mailbox(dev);
	if (!mailbox) {
		dev_err(dev->dev, "failed to alloc mailbox for modify jfc.\n");
		return -ENOMEM;
	}

	jfc_context = &((struct udma_jfc_ctx *)mailbox->buf)[0];
	ctx_mask = &((struct udma_jfc_ctx *)mailbox->buf)[1];
	memset(ctx_mask, 0xff, sizeof(struct udma_jfc_ctx));

	if (attr->mask & UBCORE_JFC_MODERATE_COUNT) {
		jfc_context->cqe_coalesce_cnt = attr->moderate_count;
		ctx_mask->cqe_coalesce_cnt = 0;
	}

	if (attr->mask & UBCORE_JFC_MODERATE_PERIOD) {
		jfc_context->cqe_coalesce_period =
			udma_get_cqe_period(attr->moderate_period);
		ctx_mask->cqe_coalesce_period = 0;
	}

	mbox_attr.tag = jfcn;
	mbox_attr.op = UDMA_CMD_MODIFY_JFC_CONTEXT;
	ret = udma_post_mbox(dev, mailbox, &mbox_attr);
	if (ret)
		dev_err(dev->dev,
			"failed to send post mbox in modify JFCC, ret = %d.\n",
			ret);

	udma_free_cmd_mailbox(dev, mailbox);

	return ret;
}

int udma_modify_jfc(struct ubcore_jfc *ubcore_jfc, struct ubcore_jfc_attr *attr,
		    struct ubcore_udata *udata)
{
	struct udma_dev *udma_device = to_udma_dev(ubcore_jfc->ub_dev);
	struct udma_jfc *udma_jfc = to_udma_jfc(ubcore_jfc);
	int ret;

	ret = udma_check_jfc_attr(udma_device, attr);
	if (ret)
		return ret;

	ret = udma_modify_jfc_attr(udma_device, udma_jfc->jfcn, attr);
	if (ret)
		dev_err(udma_device->dev,
			"failed to modify JFC, jfcn = %u, ret = %d.\n",
			udma_jfc->jfcn, ret);

	return ret;
}
