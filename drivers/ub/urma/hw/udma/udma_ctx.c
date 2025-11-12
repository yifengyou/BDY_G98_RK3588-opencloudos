// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt

#include <asm/current.h>
#include <ub/urma/ubcore_api.h>
#include "udma_jfs.h"
#include "udma_jetty.h"
#include "udma_ctrlq_tp.h"
#include "udma_ctx.h"

static int udma_init_ctx_resp(struct udma_dev *dev, struct ubcore_udrv_priv *udrv_data)
{
	struct udma_create_ctx_resp resp;
	unsigned long byte;

	if (!udrv_data->out_addr ||
	    udrv_data->out_len < sizeof(resp)) {
		dev_err(dev->dev,
			"Invalid ctx resp out: len %d or addr is invalid.\n",
			udrv_data->out_len);
		return -EINVAL;
	}

	resp.cqe_size = dev->caps.cqe_size;
	resp.dwqe_enable = !!(dev->caps.feature & UDMA_CAP_FEATURE_DIRECT_WQE);
	resp.reduce_enable = !!(dev->caps.feature & UDMA_CAP_FEATURE_REDUCE);
	resp.ue_id = dev->ue_id;
	resp.chip_id = dev->chip_id;
	resp.die_id = dev->die_id;
	resp.dump_aux_info = dump_aux_info;
	resp.jfr_sge = dev->caps.jfr_sge;
	resp.hugepage_enable = ubase_adev_prealloc_supported(dev->comdev.adev);

	byte = copy_to_user((void *)(uintptr_t)udrv_data->out_addr, &resp,
			   (uint32_t)sizeof(resp));
	if (byte) {
		dev_err(dev->dev,
			"copy ctx resp to user failed, byte = %lu.\n", byte);
		return -EFAULT;
	}

	return 0;
}

struct ubcore_ucontext *udma_alloc_ucontext(struct ubcore_device *ub_dev,
					    uint32_t eid_index,
					    struct ubcore_udrv_priv *udrv_data)
{
	struct udma_dev *dev = to_udma_dev(ub_dev);
	struct udma_context *ctx;
	int ret;

	ctx = kzalloc(sizeof(struct udma_context), GFP_KERNEL);
	if (ctx == NULL)
		return NULL;

	ctx->sva = ummu_sva_bind_device(dev->dev, current->mm, NULL);
	if (!ctx->sva) {
		dev_err(dev->dev, "SVA failed to bind device.\n");
		goto err_free_ctx;
	}

	ret = ummu_get_tid(dev->dev, ctx->sva, &ctx->tid);
	if (ret) {
		dev_err(dev->dev, "Failed to get tid.\n");
		goto err_unbind_dev;
	}

	ctx->dev = dev;
	INIT_LIST_HEAD(&ctx->pgdir_list);
	mutex_init(&ctx->pgdir_mutex);
	INIT_LIST_HEAD(&ctx->hugepage_list);
	mutex_init(&ctx->hugepage_lock);

	ret = udma_init_ctx_resp(dev, udrv_data);
	if (ret) {
		dev_err(dev->dev, "Init ctx resp failed.\n");
		goto err_init_ctx_resp;
	}

	return &ctx->base;

err_init_ctx_resp:
	mutex_destroy(&ctx->hugepage_lock);
	mutex_destroy(&ctx->pgdir_mutex);
err_unbind_dev:
	ummu_sva_unbind_device(ctx->sva);
err_free_ctx:
	kfree(ctx);
	return NULL;
}

int udma_free_ucontext(struct ubcore_ucontext *ucontext)
{
	struct udma_dev *udma_dev = to_udma_dev(ucontext->ub_dev);
	struct udma_hugepage_priv *priv;
	struct vm_area_struct *vma;
	struct udma_context *ctx;
	int ret;
	int i;

	ctx = to_udma_context(ucontext);

	ret = ummu_core_invalidate_cfg_table(ctx->tid);
	if (ret)
		dev_err(udma_dev->dev, "invalidate cfg_table failed, ret=%d.\n", ret);

	mutex_destroy(&ctx->pgdir_mutex);
	ummu_sva_unbind_device(ctx->sva);

	mutex_lock(&ctx->hugepage_lock);
	list_for_each_entry(priv, &ctx->hugepage_list, list) {
		if (current->mm) {
			mmap_write_lock(current->mm);
			vma = find_vma(current->mm, (unsigned long)priv->va_base);
			if (vma != NULL && vma->vm_start <= (unsigned long)priv->va_base &&
			    vma->vm_end >= (unsigned long)(priv->va_base + priv->va_len))
				zap_vma_ptes(vma, (unsigned long)priv->va_base, priv->va_len);
			mmap_write_unlock(current->mm);
		}

		dev_info(udma_dev->dev, "unmap_hugepage, 2m_page_num=%u.\n", priv->page_num);
		for (i = 0; i < priv->page_num; i++)
			__free_pages(priv->pages[i], get_order(UDMA_HUGEPAGE_SIZE));
		kfree(priv->pages);
		kfree(priv);
	}
	mutex_unlock(&ctx->hugepage_lock);
	mutex_destroy(&ctx->hugepage_lock);

	kfree(ctx);

	return 0;
}

static int udma_mmap_jetty_dsqe(struct udma_dev *dev, struct ubcore_ucontext *uctx,
				struct vm_area_struct *vma)
{
	struct ubcore_ucontext *jetty_uctx;
	struct udma_jetty_queue *sq;
	uint64_t address;
	uint64_t j_id;

	j_id = get_mmap_idx(vma);

	xa_lock(&dev->jetty_table.xa);
	sq = xa_load(&dev->jetty_table.xa, j_id);
	if (!sq) {
		dev_err(dev->dev,
			"mmap failed, j_id: %llu not exist\n", j_id);
		xa_unlock(&dev->jetty_table.xa);
		return -EINVAL;
	}

	if (sq->is_jetty)
		jetty_uctx = to_udma_jetty_from_queue(sq)->ubcore_jetty.uctx;
	else
		jetty_uctx = to_udma_jfs_from_queue(sq)->ubcore_jfs.uctx;

	if (jetty_uctx != uctx) {
		dev_err(dev->dev,
			"mmap failed, j_id: %llu, uctx invalid\n", j_id);
		xa_unlock(&dev->jetty_table.xa);
		return -EINVAL;
	}
	xa_unlock(&dev->jetty_table.xa);

	address = (uint64_t)dev->db_base + JETTY_DSQE_OFFSET + j_id * UDMA_HW_PAGE_SIZE;

	if (io_remap_pfn_range(vma, vma->vm_start, address >> PAGE_SHIFT,
			       PAGE_SIZE, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int udma_mmap_hugepage(struct udma_dev *dev, struct ubcore_ucontext *uctx,
			      struct vm_area_struct *vma)
{
	uint32_t max_map_size = dev->caps.cqe_size * dev->caps.jfc.depth;
	uint32_t map_size = vma->vm_end - vma->vm_start;

	if (!IS_ALIGNED(map_size, UDMA_HUGEPAGE_SIZE)) {
		dev_err(dev->dev, "mmap size is not 2m alignment.\n");
		return -EINVAL;
	}

	if (map_size == 0) {
		dev_err(dev->dev, "mmap size is zero.\n");
		return -EINVAL;
	}

	if (map_size > max_map_size) {
		dev_err(dev->dev, "mmap size(%u) is greater than the max_size.\n",
			map_size);
		return -EINVAL;
	}

	vm_flags_set(vma, VM_IO | VM_LOCKED | VM_DONTEXPAND | VM_DONTDUMP | VM_DONTCOPY);
	vma->vm_page_prot = __pgprot(((~PTE_ATTRINDX_MASK) & vma->vm_page_prot.pgprot) |
				     PTE_ATTRINDX(MT_NORMAL));
	if (udma_alloc_u_hugepage(to_udma_context(uctx), vma)) {
		dev_err(dev->dev, "failed to alloc hugepage.\n");
		return -ENOMEM;
	}

	return 0;
}

int udma_mmap(struct ubcore_ucontext *uctx, struct vm_area_struct *vma)
{
#define JFC_DB_UNMAP_BOUND 1
	struct udma_dev *udma_dev = to_udma_dev(uctx->ub_dev);
	uint32_t cmd;

	if (((vma->vm_end - vma->vm_start) % PAGE_SIZE) != 0) {
		dev_err(udma_dev->dev,
			"mmap failed, unexpected vm area size.\n");
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	cmd = get_mmap_cmd(vma);
	switch (cmd) {
	case UDMA_MMAP_JFC_PAGE:
		if (io_remap_pfn_range(vma, vma->vm_start,
				       jfc_arm_mode > JFC_DB_UNMAP_BOUND ?
				       (uint64_t)udma_dev->db_base >> PAGE_SHIFT :
				       page_to_pfn(udma_dev->db_page),
				       PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		break;
	case UDMA_MMAP_JETTY_DSQE:
		return udma_mmap_jetty_dsqe(udma_dev, uctx, vma);
	case UDMA_MMAP_HUGEPAGE:
		return udma_mmap_hugepage(udma_dev, uctx, vma);
	default:
		dev_err(udma_dev->dev,
			"mmap failed, cmd(%u) not support\n", cmd);
		return -EINVAL;
	}

	return 0;
}

int udma_alloc_u_hugepage(struct udma_context *ctx, struct vm_area_struct *vma)
{
	uint32_t page_num = (vma->vm_end - vma->vm_start) >> UDMA_HUGEPAGE_SHIFT;
	struct udma_hugepage_priv *priv;
	int ret = -ENOMEM;
	int i;

	mutex_lock(&ctx->dev->hugepage_lock);
	if (page_num > ctx->dev->total_hugepage_num) {
		dev_err(ctx->dev->dev, "insufficient resources for mmap.\n");
		mutex_unlock(&ctx->dev->hugepage_lock);
		return -EINVAL;
	}
	ctx->dev->total_hugepage_num -= page_num;
	mutex_unlock(&ctx->dev->hugepage_lock);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		goto err_alloc_priv;

	priv->page_num = page_num;
	priv->pages = kcalloc(priv->page_num, sizeof(*priv->pages), GFP_KERNEL);
	if (!priv->pages)
		goto err_alloc_arr;

	for (i = 0; i < priv->page_num; i++) {
		priv->pages[i] = alloc_pages(GFP_KERNEL | __GFP_ZERO,
					     get_order(UDMA_HUGEPAGE_SIZE));
		if (!priv->pages[i]) {
			dev_err(ctx->dev->dev, "failed to alloc 2M pages.\n");
			goto err_alloc_pages;
		}
		ret = remap_pfn_range(vma, vma->vm_start + i * UDMA_HUGEPAGE_SIZE,
				      page_to_pfn(priv->pages[i]), UDMA_HUGEPAGE_SIZE,
				      vma->vm_page_prot);
		if (ret) {
			dev_err(ctx->dev->dev, "failed to remap_pfn_range, ret=%d.\n", ret);
			goto err_remap_pfn_range;
		}
	}

	priv->va_base = (void *)vma->vm_start;
	priv->va_len = priv->page_num << UDMA_HUGEPAGE_SHIFT;
	priv->left_va_len = priv->va_len;
	refcount_set(&priv->refcnt, 1);

	mutex_lock(&ctx->hugepage_lock);
	list_add(&priv->list, &ctx->hugepage_list);
	mutex_unlock(&ctx->hugepage_lock);

	if (dfx_switch)
		dev_info_ratelimited(ctx->dev->dev, "map_hugepage, 2m_page_num=%u.\n",
				     priv->page_num);
	return 0;

err_remap_pfn_range:
err_alloc_pages:
	for (i = 0; i < priv->page_num; i++) {
		if (priv->pages[i])
			__free_pages(priv->pages[i], get_order(UDMA_HUGEPAGE_SIZE));
		else
			break;
	}
	kfree(priv->pages);
err_alloc_arr:
	kfree(priv);
err_alloc_priv:
	mutex_lock(&ctx->dev->hugepage_lock);
	ctx->dev->total_hugepage_num += page_num;
	mutex_unlock(&ctx->dev->hugepage_lock);

	return ret;
}

static struct udma_hugepage_priv *udma_list_find_before(struct udma_context *ctx, void *va)
{
	struct udma_hugepage_priv *priv;

	list_for_each_entry(priv, &ctx->hugepage_list, list) {
		if (va >= priv->va_base && va < priv->va_base + priv->va_len)
			return priv;
	}

	return NULL;
}

int udma_occupy_u_hugepage(struct udma_context *ctx, void *va)
{
	struct udma_hugepage_priv *priv;

	mutex_lock(&ctx->hugepage_lock);
	priv = udma_list_find_before(ctx, va);
	if (priv) {
		if (dfx_switch)
			dev_info_ratelimited(ctx->dev->dev, "occupy_hugepage.\n");
		refcount_inc(&priv->refcnt);
	}
	mutex_unlock(&ctx->hugepage_lock);

	return priv ? 0 : -EFAULT;
}

void udma_return_u_hugepage(struct udma_context *ctx, void *va)
{
	struct udma_hugepage_priv *priv;
	struct vm_area_struct *vma;
	uint32_t i;

	mutex_lock(&ctx->hugepage_lock);
	priv = udma_list_find_before(ctx, va);
	if (!priv) {
		mutex_unlock(&ctx->hugepage_lock);
		dev_warn(ctx->dev->dev, "va is invalid addr.\n");
		return;
	}

	if (dfx_switch)
		dev_info_ratelimited(ctx->dev->dev, "return_hugepage.\n");
	refcount_dec(&priv->refcnt);
	if (!refcount_dec_if_one(&priv->refcnt)) {
		mutex_unlock(&ctx->hugepage_lock);
		return;
	}

	list_del(&priv->list);
	mutex_unlock(&ctx->hugepage_lock);

	if (current->mm) {
		mmap_write_lock(current->mm);
		vma = find_vma(current->mm, (unsigned long)priv->va_base);
		if (vma != NULL && vma->vm_start <= (unsigned long)priv->va_base &&
		    vma->vm_end >= (unsigned long)(priv->va_base + priv->va_len))
			zap_vma_ptes(vma, (unsigned long)priv->va_base, priv->va_len);
		mmap_write_unlock(current->mm);
	} else {
		dev_warn(ctx->dev->dev, "current mm released.\n");
	}

	if (dfx_switch)
		dev_info_ratelimited(ctx->dev->dev, "unmap_hugepage, 2m_page_num=%u.\n",
				     priv->page_num);
	mutex_lock(&ctx->dev->hugepage_lock);
	for (i = 0; i < priv->page_num; i++)
		__free_pages(priv->pages[i], get_order(UDMA_HUGEPAGE_SIZE));
	ctx->dev->total_hugepage_num += priv->page_num;
	mutex_unlock(&ctx->dev->hugepage_lock);
	kfree(priv->pages);
	kfree(priv);
}
