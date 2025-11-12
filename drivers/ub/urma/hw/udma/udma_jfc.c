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

static int udma_alloc_u_cq(struct udma_dev *dev, struct udma_create_jfc_ucmd *ucmd,
			   struct udma_jfc *jfc)
{
	int ret;

	if (ucmd->is_hugepage) {
		jfc->buf.addr = ucmd->buf_addr;
		if (udma_occupy_u_hugepage(jfc->ctx, (void *)jfc->buf.addr)) {
			dev_err(dev->dev, "failed to create cq, va not map.\n");
			return -EINVAL;
		}
		jfc->buf.is_hugepage = true;
	} else {
		ret = pin_queue_addr(dev, ucmd->buf_addr, ucmd->buf_len, &jfc->buf);
		if (ret) {
			dev_err(dev->dev, "failed to pin queue for jfc, ret = %d.\n", ret);
			return ret;
		}
	}
	jfc->tid = jfc->ctx->tid;

	ret = udma_pin_sw_db(jfc->ctx, &jfc->db);
	if (ret) {
		dev_err(dev->dev, "failed to pin sw db for jfc, ret = %d.\n", ret);
		goto err_pin_db;
	}

	return 0;
err_pin_db:
	if (ucmd->is_hugepage)
		udma_return_u_hugepage(jfc->ctx, (void *)jfc->buf.addr);
	else
		unpin_queue_addr(jfc->buf.umem);

	return ret;
}

static int udma_alloc_k_cq(struct udma_dev *dev, struct udma_jfc *jfc)
{
	int ret;

	if (!jfc->lock_free)
		spin_lock_init(&jfc->lock);

	jfc->buf.entry_size = dev->caps.cqe_size;
	jfc->tid = dev->tid;
	ret = udma_k_alloc_buf(dev, &jfc->buf);
	if (ret) {
		dev_err(dev->dev, "failed to alloc cq buffer, id=%u.\n", jfc->jfcn);
		return ret;
	}

	ret = udma_alloc_sw_db(dev, &jfc->db, UDMA_JFC_TYPE_DB);
	if (ret) {
		dev_err(dev->dev, "failed to alloc sw db for jfc(%u).\n", jfc->jfcn);
		udma_k_free_buf(dev, &jfc->buf);
	}

	return ret;
}

static void udma_free_cq(struct udma_dev *dev, struct udma_jfc *jfc)
{
	if (jfc->mode != UDMA_NORMAL_JFC_TYPE) {
		udma_free_sw_db(dev, &jfc->db);
		return;
	}

	if (jfc->buf.kva) {
		udma_k_free_buf(dev, &jfc->buf);
		udma_free_sw_db(dev, &jfc->db);
	} else {
		if (jfc->buf.is_hugepage)
			udma_return_u_hugepage(jfc->ctx, (void *)jfc->buf.addr);
		else
			unpin_queue_addr(jfc->buf.umem);
		udma_unpin_sw_db(jfc->ctx, &jfc->db);
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

	ret = udata ? udma_alloc_u_cq(dev, &ucmd, jfc) : udma_alloc_k_cq(dev, jfc);
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
	udma_free_cq(dev, jfc);
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

	udma_free_cq(dev, ujfc);
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
		xa_unlock(&udma_dev->jfc_table.xa);
		dev_warn(udma_dev->dev,
			 "Completion event for bogus jfcn %lu.\n", jfcn);
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

int udma_rearm_jfc(struct ubcore_jfc *jfc, bool solicited_only)
{
	struct udma_dev *dev = to_udma_dev(jfc->ub_dev);
	struct udma_jfc *udma_jfc = to_udma_jfc(jfc);
	struct udma_jfc_db db;

	db.ci = udma_jfc->ci & (uint32_t)UDMA_JFC_DB_CI_IDX_M;
	db.notify = solicited_only;
	db.arm_sn = udma_jfc->arm_sn;
	db.type = UDMA_CQ_ARM_DB;
	db.jfcn = udma_jfc->jfcn;

	udma_write64(dev, (uint64_t *)&db, (void __iomem *)(dev->k_db_base +
		UDMA_JFC_HW_DB_OFFSET));

	return 0;
}

static enum jfc_poll_state udma_get_cr_status(struct udma_dev *dev,
					      uint8_t src_status,
					      uint8_t substatus,
					      enum ubcore_cr_status *dst_status)
{
#define UDMA_SRC_STATUS_NUM 7
#define UDMA_SUB_STATUS_NUM 5

struct udma_cr_status {
	bool is_valid;
	enum ubcore_cr_status cr_status;
};

	static struct udma_cr_status map[UDMA_SRC_STATUS_NUM][UDMA_SUB_STATUS_NUM] = {
		{{true, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}},
		{{true, UBCORE_CR_UNSUPPORTED_OPCODE_ERR}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}},
		{{false, UBCORE_CR_SUCCESS}, {true, UBCORE_CR_LOC_LEN_ERR},
		 {true, UBCORE_CR_LOC_ACCESS_ERR}, {true, UBCORE_CR_REM_RESP_LEN_ERR},
		 {true, UBCORE_CR_LOC_DATA_POISON}},
		{{false, UBCORE_CR_SUCCESS}, {true, UBCORE_CR_REM_UNSUPPORTED_REQ_ERR},
		 {true, UBCORE_CR_REM_ACCESS_ABORT_ERR}, {false, UBCORE_CR_SUCCESS},
		 {true, UBCORE_CR_REM_DATA_POISON}},
		{{true, UBCORE_CR_RNR_RETRY_CNT_EXC_ERR}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}},
		{{true, UBCORE_CR_ACK_TIMEOUT_ERR}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}},
		{{true, UBCORE_CR_FLUSH_ERR}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}, {false, UBCORE_CR_SUCCESS},
		 {false, UBCORE_CR_SUCCESS}}
	};

	if ((src_status < UDMA_SRC_STATUS_NUM) && (substatus < UDMA_SUB_STATUS_NUM) &&
	    map[src_status][substatus].is_valid) {
		*dst_status = map[src_status][substatus].cr_status;
		return JFC_OK;
	}

	dev_err(dev->dev, "cqe status is error, status = %u, substatus = %u.\n",
		src_status, substatus);

	return JFC_POLL_ERR;
}

static void udma_handle_inline_cqe(struct udma_jfc_cqe *cqe, uint8_t opcode,
				   struct udma_jetty_queue *queue,
				   struct ubcore_cr *cr)
{
	struct udma_jfr *jfr = to_udma_jfr_from_queue(queue);
	uint32_t rqe_idx, data_len, sge_idx, size;
	struct udma_wqe_sge *sge_list;
	void *cqe_inline_buf;

	rqe_idx = cqe->entry_idx;
	sge_list = (struct udma_wqe_sge *)(jfr->rq.buf.kva +
					   rqe_idx * jfr->rq.buf.entry_size);
	data_len = cqe->byte_cnt;
	cqe_inline_buf = opcode == HW_CQE_OPC_SEND ?
			 (void *)&cqe->data_l : (void *)&cqe->inline_data;

	for (sge_idx = 0; (sge_idx < jfr->max_sge) && data_len; sge_idx++) {
		size = sge_list[sge_idx].length < data_len ?
		       sge_list[sge_idx].length : data_len;
		memcpy((void *)(uintptr_t)sge_list[sge_idx].va,
		       cqe_inline_buf, size);
		data_len -= size;
		cqe_inline_buf += size;
	}
	cr->completion_len = cqe->byte_cnt - data_len;

	if (data_len) {
		cqe->status = UDMA_CQE_LOCAL_OP_ERR;
		cqe->substatus = UDMA_CQE_LOCAL_LENGTH_ERR;
	}
}

static void udma_parse_opcode_for_res(struct udma_dev *dev,
				      struct udma_jfc_cqe *cqe,
				      struct ubcore_cr *cr,
				      struct list_head *tid_list)
{
	uint8_t opcode = cqe->opcode;
	struct udma_inv_tid *inv_tid;

	switch (opcode) {
	case HW_CQE_OPC_SEND:
		cr->opcode = UBCORE_CR_OPC_SEND;
		break;
	case HW_CQE_OPC_SEND_WITH_IMM:
		cr->imm_data = (uint64_t)cqe->data_h << UDMA_IMM_DATA_SHIFT |
			       cqe->data_l;
		cr->opcode = UBCORE_CR_OPC_SEND_WITH_IMM;
		break;
	case HW_CQE_OPC_SEND_WITH_INV:
		cr->invalid_token.token_id = cqe->data_l & (uint32_t)UDMA_CQE_INV_TOKEN_ID;
		cr->invalid_token.token_id <<= UDMA_TID_SHIFT;
		cr->invalid_token.token_value.token = cqe->data_h;
		cr->opcode = UBCORE_CR_OPC_SEND_WITH_INV;

		inv_tid = kzalloc(sizeof(*inv_tid), GFP_ATOMIC);
		if (!inv_tid)
			return;

		inv_tid->tid = cr->invalid_token.token_id >> UDMA_TID_SHIFT;
		list_add(&inv_tid->list, tid_list);

		break;
	case HW_CQE_OPC_WRITE_WITH_IMM:
		cr->imm_data = (uint64_t)cqe->data_h << UDMA_IMM_DATA_SHIFT |
			       cqe->data_l;
		cr->opcode = UBCORE_CR_OPC_WRITE_WITH_IMM;
		break;
	default:
		cr->opcode = (enum ubcore_cr_opcode)HW_CQE_OPC_ERR;
		dev_err(dev->dev, "receive invalid opcode :%u.\n", opcode);
		cr->status = UBCORE_CR_UNSUPPORTED_OPCODE_ERR;
		break;
	}
}

static struct udma_jfr *udma_get_jfr(struct udma_dev *udma_dev,
				     struct udma_jfc_cqe *cqe,
				     struct ubcore_cr *cr)
{
	struct udma_jetty_queue *udma_sq;
	struct udma_jetty *jetty = NULL;
	struct udma_jfr *jfr = NULL;
	uint32_t local_id;

	local_id = cr->local_id;
	if (cqe->is_jetty) {
		udma_sq = (struct udma_jetty_queue *)xa_load(&udma_dev->jetty_table.xa, local_id);
		if (!udma_sq) {
			dev_warn(udma_dev->dev,
				 "get jetty failed, jetty_id = %u.\n", local_id);
			return NULL;
		}
		jetty = to_udma_jetty_from_queue(udma_sq);
		jfr = jetty->jfr;
		cr->user_data = (uintptr_t)&jetty->ubcore_jetty;
	} else {
		jfr = (struct udma_jfr *)xa_load(&udma_dev->jfr_table.xa, local_id);
		if (!jfr) {
			dev_warn(udma_dev->dev,
				 "get jfr failed jfr id = %u.\n", local_id);
			return NULL;
		}
		cr->user_data = (uintptr_t)&jfr->ubcore_jfr;
	}

	return jfr;
}

static bool udma_update_jfr_idx(struct udma_dev *dev,
				struct udma_jfc_cqe *cqe,
				struct ubcore_cr *cr,
				bool is_clean)
{
	struct udma_jetty_queue *queue;
	uint8_t opcode = cqe->opcode;
	struct udma_jfr *jfr;
	uint32_t entry_idx;

	jfr = udma_get_jfr(dev, cqe, cr);
	if (!jfr)
		return true;

	queue = &jfr->rq;
	entry_idx = cqe->entry_idx;
	cr->user_ctx = queue->wrid[entry_idx & (queue->buf.entry_cnt - (uint32_t)1)];

	if (!is_clean && cqe->inline_en)
		udma_handle_inline_cqe(cqe, opcode, queue, cr);

	if (!jfr->ubcore_jfr.jfr_cfg.flag.bs.lock_free)
		spin_lock(&jfr->lock);

	udma_id_free(&jfr->idx_que.jfr_idx_table.ida_table, entry_idx);
	queue->ci++;

	if (!jfr->ubcore_jfr.jfr_cfg.flag.bs.lock_free)
		spin_unlock(&jfr->lock);

	return false;
}

static enum jfc_poll_state udma_parse_cqe_for_send(struct udma_dev *dev,
						   struct udma_jfc_cqe *cqe,
						   struct ubcore_cr *cr)
{
	struct udma_jetty_queue *queue;
	struct udma_jetty *jetty;
	struct udma_jfs *jfs;

	queue = (struct udma_jetty_queue *)(uintptr_t)(
		(uint64_t)cqe->user_data_h << UDMA_ADDR_SHIFT |
		cqe->user_data_l);
	if (!queue) {
		dev_err(dev->dev, "jetty queue addr is null, jetty_id = %u.\n", cr->local_id);
		return JFC_POLL_ERR;
	}

	if (unlikely(udma_get_cr_status(dev, cqe->status, cqe->substatus, &cr->status)))
		return JFC_POLL_ERR;

	if (!!cqe->fd) {
		cr->status = UBCORE_CR_WR_FLUSH_ERR_DONE;
		queue->flush_flag = true;
	} else {
		queue->ci += (cqe->entry_idx - queue->ci) & (queue->buf.entry_cnt - 1);
		cr->user_ctx = queue->wrid[queue->ci & (queue->buf.entry_cnt - 1)];
		queue->ci++;
	}

	if (!!cr->flag.bs.jetty) {
		jetty = to_udma_jetty_from_queue(queue);
		cr->user_data = (uintptr_t)&jetty->ubcore_jetty;
	} else {
		jfs = container_of(queue, struct udma_jfs, sq);
		cr->user_data = (uintptr_t)&jfs->ubcore_jfs;
	}

	return JFC_OK;
}

static enum jfc_poll_state udma_parse_cqe_for_recv(struct udma_dev *dev,
						   struct udma_jfc_cqe *cqe,
						   struct ubcore_cr *cr,
						   struct list_head *tid_list)
{
	uint8_t substatus;
	uint8_t status;

	if (unlikely(udma_update_jfr_idx(dev, cqe, cr, false)))
		return JFC_POLL_ERR;

	udma_parse_opcode_for_res(dev, cqe, cr, tid_list);
	status = cqe->status;
	substatus = cqe->substatus;
	if (unlikely(udma_get_cr_status(dev, status, substatus, &cr->status)))
		return JFC_POLL_ERR;

	return JFC_OK;
}

static enum jfc_poll_state parse_cqe_for_jfc(struct udma_dev *dev,
					     struct udma_jfc_cqe *cqe,
					     struct ubcore_cr *cr,
					     struct list_head *tid_list)
{
	enum jfc_poll_state ret;

	cr->flag.bs.s_r = cqe->s_r;
	cr->flag.bs.jetty = cqe->is_jetty;
	cr->completion_len = cqe->byte_cnt;
	cr->tpn = cqe->tpn;
	cr->local_id = cqe->local_num_h << UDMA_SRC_IDX_SHIFT | cqe->local_num_l;
	cr->remote_id.id = cqe->rmt_idx;
	udma_swap_endian((uint8_t *)(cqe->rmt_eid), cr->remote_id.eid.raw, UBCORE_EID_SIZE);

	if (cqe->s_r == CQE_FOR_RECEIVE)
		ret = udma_parse_cqe_for_recv(dev, cqe, cr, tid_list);
	else
		ret = udma_parse_cqe_for_send(dev, cqe, cr);

	return ret;
}

static struct udma_jfc_cqe *get_next_cqe(struct udma_jfc *jfc, uint32_t n)
{
	struct udma_jfc_cqe *cqe;
	uint32_t valid_owner;

	cqe = (struct udma_jfc_cqe *)get_buf_entry(&jfc->buf, n);

	valid_owner = (n >> jfc->cq_shift) & UDMA_JFC_DB_VALID_OWNER_M;
	if (!(cqe->owner ^ valid_owner))
		return NULL;

	return cqe;
}

static void dump_cqe_aux_info(struct udma_dev *dev, struct ubcore_cr *cr)
{
	struct ubcore_user_ctl_out out = {};
	struct ubcore_user_ctl_in in = {};
	struct udma_cqe_info_in info_in;

	info_in.status = cr->status;
	info_in.s_r = cr->flag.bs.s_r;
	in.addr = (uint64_t)&info_in;
	in.len = sizeof(struct udma_cqe_info_in);
	in.opcode = UDMA_USER_CTL_QUERY_CQE_AUX_INFO;

	(void)udma_query_cqe_aux_info(&dev->ub_dev, NULL, &in, &out);
}

static enum jfc_poll_state udma_poll_one(struct udma_dev *dev,
					 struct udma_jfc *jfc,
					 struct ubcore_cr *cr,
					 struct list_head *tid_list)
{
	struct udma_jfc_cqe *cqe;

	cqe = get_next_cqe(jfc, jfc->ci);
	if (!cqe)
		return JFC_EMPTY;

	++jfc->ci;
	/* Memory barrier */
	rmb();

	if (parse_cqe_for_jfc(dev, cqe, cr, tid_list))
		return JFC_POLL_ERR;

	if (unlikely(cr->status != UBCORE_CR_SUCCESS) && dump_aux_info)
		dump_cqe_aux_info(dev, cr);

	return JFC_OK;
}

static void udma_inv_tid(struct udma_dev *dev, struct list_head *tid_list)
{
	struct udma_inv_tid *tid_node;
	struct udma_inv_tid *tmp;
	struct iommu_sva *ksva;
	uint32_t tid;

	mutex_lock(&dev->ksva_mutex);
	list_for_each_entry_safe(tid_node, tmp, tid_list, list) {
		tid = tid_node->tid;
		ksva = (struct iommu_sva *)xa_load(&dev->ksva_table, tid);
		if (!ksva) {
			dev_warn(dev->dev, "tid may have been released.\n");
		} else {
			ummu_ksva_unbind_device(ksva);
			__xa_erase(&dev->ksva_table, tid);
		}

		list_del(&tid_node->list);
		kfree(tid_node);
	}
	mutex_unlock(&dev->ksva_mutex);
}

/* thanks to drivers/infiniband/hw/bnxt_re/ib_verbs.c */
int udma_poll_jfc(struct ubcore_jfc *jfc, int cr_cnt, struct ubcore_cr *cr)
{
	struct udma_dev *dev = to_udma_dev(jfc->ub_dev);
	struct udma_jfc *udma_jfc = to_udma_jfc(jfc);
	enum jfc_poll_state err = JFC_OK;
	struct list_head tid_list;
	unsigned long flags;
	uint32_t ci;
	int npolled;

	INIT_LIST_HEAD(&tid_list);

	if (!jfc->jfc_cfg.flag.bs.lock_free)
		spin_lock_irqsave(&udma_jfc->lock, flags);

	for (npolled = 0; npolled < cr_cnt; ++npolled) {
		err = udma_poll_one(dev, udma_jfc, cr + npolled, &tid_list);
		if (err != JFC_OK)
			break;
	}

	if (npolled) {
		ci = udma_jfc->ci;
		*udma_jfc->db.db_record = ci & (uint32_t)UDMA_JFC_DB_CI_IDX_M;
	}

	if (!jfc->jfc_cfg.flag.bs.lock_free)
		spin_unlock_irqrestore(&udma_jfc->lock, flags);

	if (!list_empty(&tid_list))
		udma_inv_tid(dev, &tid_list);

	return err == JFC_POLL_ERR ? -UDMA_INTER_ERR : npolled;
}

void udma_clean_jfc(struct ubcore_jfc *jfc, uint32_t jetty_id, struct udma_dev *udma_dev)
{
	struct udma_jfc *udma_jfc = to_udma_jfc(jfc);
	struct udma_jfc_cqe *dest;
	struct udma_jfc_cqe *cqe;
	struct ubcore_cr cr;
	uint32_t nfreed = 0;
	uint32_t local_id;
	uint8_t owner_bit;
	uint32_t pi;

	if (udma_jfc->mode != (uint32_t)UDMA_NORMAL_JFC_TYPE)
		return;

	if (!jfc->jfc_cfg.flag.bs.lock_free)
		spin_lock(&udma_jfc->lock);

	for (pi = udma_jfc->ci; get_next_cqe(udma_jfc, pi) != NULL; ++pi) {
		if (pi > udma_jfc->ci + udma_jfc->buf.entry_cnt)
			break;
	}
	while ((int) --pi - (int) udma_jfc->ci >= 0) {
		cqe = get_buf_entry(&udma_jfc->buf, pi);
		/* make sure cqe buffer is valid */
		rmb();
		local_id = (cqe->local_num_h << UDMA_SRC_IDX_SHIFT) | cqe->local_num_l;
		if (local_id == jetty_id) {
			if (cqe->s_r == CQE_FOR_RECEIVE) {
				cr.local_id = local_id;
				(void)udma_update_jfr_idx(udma_dev, cqe, &cr, true);
			}

			++nfreed;
		} else if (!!nfreed) {
			dest = get_buf_entry(&udma_jfc->buf, pi + nfreed);
			/* make sure owner bit is valid */
			rmb();
			owner_bit = dest->owner;
			(void)memcpy(dest, cqe, udma_dev->caps.cqe_size);
			dest->owner = owner_bit;
		}
	}

	if (!!nfreed) {
		udma_jfc->ci += nfreed;
		wmb(); /* be sure software get cqe data before update doorbell */
		*udma_jfc->db.db_record = udma_jfc->ci & (uint32_t)UDMA_JFC_DB_CI_IDX_M;
	}

	if (!jfc->jfc_cfg.flag.bs.lock_free)
		spin_unlock(&udma_jfc->lock);
}
