// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved. */

#define dev_fmt(fmt) "CDMA: " fmt

#include <linux/idr.h>
#include <linux/ummu_core.h>
#include "cdma.h"
#include "cdma_queue.h"
#include "cdma_jfc.h"
#include "cdma_jfs.h"
#include "cdma_tp.h"
#include "cdma_segment.h"
#include "cdma_context.h"

static void cdma_ctx_handle_free(struct cdma_dev *cdev,
				 struct cdma_context *ctx)
{
	spin_lock(&cdev->ctx_lock);
	idr_remove(&cdev->ctx_idr, ctx->handle);
	spin_unlock(&cdev->ctx_lock);
}

static int cdma_ctx_handle_alloc(struct cdma_dev *cdev,
				 struct cdma_context *ctx)
{
#define CDMA_CTX_START 0
#define CDMA_CTX_END 0xff
	int id;

	idr_preload(GFP_KERNEL);
	spin_lock(&cdev->ctx_lock);
	id = idr_alloc(&cdev->ctx_idr, ctx, CDMA_CTX_START, CDMA_CTX_END,
		       GFP_NOWAIT);
	spin_unlock(&cdev->ctx_lock);
	idr_preload_end();

	return id;
}

struct cdma_context *cdma_find_ctx_by_handle(struct cdma_dev *cdev, int handle)
{
	struct cdma_context *ctx;

	spin_lock(&cdev->ctx_lock);
	ctx = idr_find(&cdev->ctx_idr, handle);
	spin_unlock(&cdev->ctx_lock);

	return ctx;
}

static int cdma_ctx_alloc_tid(struct cdma_dev *cdev, struct cdma_context *ctx)
{
	struct ummu_param drvdata = { .mode = MAPT_MODE_TABLE };
	int ret;

	if (ctx->is_kernel)
		ctx->sva = ummu_ksva_bind_device(cdev->dev, &drvdata);
	else
		ctx->sva = ummu_sva_bind_device(cdev->dev, current->mm, NULL);

	if (!ctx->sva) {
		dev_err(cdev->dev, "%s bind device failed.\n",
			ctx->is_kernel ? "KSVA" : "SVA");
		return -EFAULT;
	}

	ret = ummu_get_tid(cdev->dev, ctx->sva, &ctx->tid);
	if (ret) {
		dev_err(cdev->dev, "get tid failed, ret = %d.\n", ret);
		if (ctx->is_kernel)
			ummu_ksva_unbind_device(ctx->sva);
		else
			ummu_sva_unbind_device(ctx->sva);
	}

	return ret;
}

static void cdma_ctx_free_tid(struct cdma_dev *cdev, struct cdma_context *ctx)
{
	if (ctx->is_kernel)
		ummu_ksva_unbind_device(ctx->sva);
	else
		ummu_sva_unbind_device(ctx->sva);
}

struct cdma_context *cdma_alloc_context(struct cdma_dev *cdev, bool is_kernel)
{
	struct cdma_context *ctx;
	int ret;

	if (!cdev)
		return ERR_PTR(-EINVAL);

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->handle = cdma_ctx_handle_alloc(cdev, ctx);
	if (ctx->handle < 0) {
		dev_err(cdev->dev,
			"Alloc context handle failed, ret = %d.\n", ctx->handle);
		ret = ctx->handle;
		goto free_ctx;
	}

	ctx->cdev = cdev;
	ctx->is_kernel = is_kernel;
	ret = cdma_ctx_alloc_tid(cdev, ctx);
	if (ret) {
		dev_err(cdev->dev, "alloc ctx tid failed, ret = %d.\n", ret);
		goto free_handle;
	}

	spin_lock_init(&ctx->lock);
	INIT_LIST_HEAD(&ctx->pgdir_list);
	mutex_init(&ctx->pgdir_mutex);
	INIT_LIST_HEAD(&ctx->queue_list);
	INIT_LIST_HEAD(&ctx->seg_list);

	return ctx;

free_handle:
	cdma_ctx_handle_free(cdev, ctx);
free_ctx:
	kfree(ctx);
	return ERR_PTR(ret);
}

void cdma_free_context(struct cdma_dev *cdev, struct cdma_context *ctx)
{
	if (!cdev || !ctx)
		return;

	if (ctx->jfae)
		ctx->jfae->ctx = NULL;

	cdma_ctx_free_tid(cdev, ctx);
	cdma_ctx_handle_free(cdev, ctx);
	mutex_destroy(&ctx->pgdir_mutex);
	kfree(ctx);
}

static void cdma_cleanup_queue_res(struct cdma_dev *cdev, struct cdma_context *ctx)
{
	struct cdma_table *queue_tbl = &cdev->queue_table;
	struct cdma_queue *queue, *next_queue;

	list_for_each_entry_safe(queue, next_queue, &ctx->queue_list, list) {
		list_del(&queue->list);

		if (queue->jfs)
			cdma_delete_jfs(cdev, queue->jfs->id);

		if (queue->tp)
			cdma_delete_ctp(cdev, queue->tp->tp_id, ctx->invalid);

		if (queue->jfc)
			cdma_delete_jfc(cdev, queue->jfc->id, NULL);

		spin_lock(&queue_tbl->lock);
		idr_remove(&queue_tbl->idr_tbl.idr, queue->id);
		spin_unlock(&queue_tbl->lock);
		kfree(queue);
	}
}

static void cdma_cleanup_segment_res(struct cdma_dev *cdev, struct cdma_context *ctx)
{
	struct cdma_segment *segment, *next_segment;

	list_for_each_entry_safe(segment, next_segment, &ctx->seg_list, list) {
		list_del(&segment->list);
		cdma_unregister_seg(cdev, segment);
	}
}

void cdma_cleanup_context_res(struct cdma_context *ctx)
{
	cdma_cleanup_queue_res(ctx->cdev, ctx);
	cdma_cleanup_segment_res(ctx->cdev, ctx);
	cdma_free_context(ctx->cdev, ctx);
}
