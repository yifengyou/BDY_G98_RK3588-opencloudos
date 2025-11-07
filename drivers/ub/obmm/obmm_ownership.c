// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * Description：OBMM Framework's implementations.
 */

#include <linux/bitmap.h>
#include <linux/vmalloc.h>

#include "obmm_core.h"
#include "obmm_ownership.h"

static inline uint32_t merge_counts(uint32_t read, uint32_t write)
{
	return (read << READ_SHIFT) | (write << WRITE_SHIFT);
}

/*
 * dirty -> non-dirty: INVAL_WB
 * non-dirty cacheable -> NC: INVAL
 * cache capability rise: NONE
 * cache operation coverage: INVAL_WB > INVAL > NONE
 */
uint8_t infer_cache_ops(uint8_t cur_state, uint8_t target_state)
{
	bool cur_dirty, cur_none, target_dirty, target_none, target_clean;
	uint8_t ops = OBMM_SHM_CACHE_NONE;

	cur_dirty = ((cur_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READWRITE &&
		     (cur_state & OBMM_SHM_MEM_CACHE_MASK) == OBMM_SHM_MEM_NORMAL);
	target_dirty = ((target_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READWRITE &&
			(target_state & OBMM_SHM_MEM_CACHE_MASK) == OBMM_SHM_MEM_NORMAL);
	target_clean = ((target_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READONLY &&
			(target_state & OBMM_SHM_MEM_CACHE_MASK) == OBMM_SHM_MEM_NORMAL);
	cur_none = ((cur_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_NO_ACCESS ||
		    (cur_state & OBMM_SHM_MEM_CACHE_MASK) != OBMM_SHM_MEM_NORMAL);
	target_none = ((target_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_NO_ACCESS ||
		       (target_state & OBMM_SHM_MEM_CACHE_MASK) != OBMM_SHM_MEM_NORMAL);
	if (cur_dirty && target_clean)
		ops = OBMM_SHM_CACHE_WB_ONLY;
	else if (cur_dirty && !target_dirty)
		ops = OBMM_SHM_CACHE_WB_INVAL;
	else if (!cur_none && target_none)
		ops = OBMM_SHM_CACHE_INVAL;

	pr_debug("%s: target_state = %u; ops = %u\n", __func__, target_state, ops);
	return ops;
}

/**
 * Calculate the local page state index corresponding to the VMA address
 */
int vma_addr_to_page_idx_local(struct vm_area_struct *vma, unsigned long addr)
{
	unsigned long offset_in_vma = addr - vma->vm_start;

	return offset_in_vma >> PAGE_SHIFT;
}

/**
 * Calculate the global page state index corresponding to the VMA address
 */
static int vma_addr_to_page_idx(struct vm_area_struct *vma,
				struct obmm_local_state_info *local_state_info, unsigned long addr)
{
	return local_state_info->orig_pgoff + vma_addr_to_page_idx_local(vma, addr);
}

/* Check if new permissions conflict with existing mappings */
static int check_target_state_allowed(uint32_t state_count, uint8_t target_mem_state)
{
	uint32_t read_count, write_count;

	read_count = GET_R_COUNTER(state_count);
	write_count = GET_W_COUNTER(state_count);

	switch (target_mem_state & OBMM_SHM_MEM_ACCESS_MASK) {
	case OBMM_SHM_MEM_READONLY:
		fallthrough;
	case OBMM_SHM_MEM_READEXEC:
		if (read_count == MAX_READ_COUNT) {
			pr_warn("%s: readonly map failed, read_count=%d\n", __func__, read_count);
			return -EBUSY;
		}
		break;
	case OBMM_SHM_MEM_READWRITE:
		if (write_count == MAX_WRITE_COUNT) {
			pr_warn("%s: readwrite map failed, write_count=%d\n", __func__,
				write_count);
			return -EBUSY;
		}
		break;
	default:
		break;
	}
	return 0;
}

/**
 * Check whether mmap operation is possible.
 * The caller holds region state_mutex lock.
 */
int check_mmap_allowed(struct obmm_region *reg, struct vm_area_struct *vma, uint8_t mem_state)
{
	int idx_offset, page_idx_start, page_count, ret;
	uint32_t state_count;
	struct obmm_local_state_info *local_state_info;
	struct obmm_ownership_info *info;

	info = reg->ownership_info;
	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;
	page_idx_start = vma_addr_to_page_idx(vma, local_state_info, vma->vm_start);
	page_count = local_state_info->npages;

	for (idx_offset = 0; idx_offset < page_count; idx_offset++) {
		state_count = info->mem_state_arr[page_idx_start + idx_offset];
		ret = check_target_state_allowed(state_count, mem_state);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * Update the count of the corresponding permission in the state.
 */
static uint32_t update_state_count(uint32_t state_count, uint8_t target_mem_state, bool inc)
{
	uint32_t read_count, write_count;
	int delta;

	delta = inc ? 1 : -1;
	read_count = GET_R_COUNTER(state_count);
	write_count = GET_W_COUNTER(state_count);

	/* inc new permission count */
	switch (target_mem_state & OBMM_SHM_MEM_ACCESS_MASK) {
	case OBMM_SHM_MEM_NO_ACCESS:
		break;
	case OBMM_SHM_MEM_READONLY:
		fallthrough;
	case OBMM_SHM_MEM_READEXEC:
		read_count += delta;
		break;
	case OBMM_SHM_MEM_READWRITE:
		write_count += delta;
		break;
	default:
		break;
	}
	return merge_counts(read_count, write_count);
}

/**
 * Check whether permissions can be modified.
 * The caller holds region state_mutex lock.
 */
int check_modify_ownership_allowed(struct obmm_region *reg, struct vm_area_struct *vma,
				   const struct obmm_cmd_update_range *update_info)
{
	int idx_offset, page_idx_start, page_count, local_page_idx_start, ret;
	uint32_t state_count;
	struct obmm_local_state_info *local_state_info;
	struct obmm_ownership_info *info;
	uint8_t old_state;

	info = reg->ownership_info;
	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;

	page_idx_start = vma_addr_to_page_idx(vma, local_state_info, update_info->start);
	local_page_idx_start = vma_addr_to_page_idx_local(vma, update_info->start);
	page_count = (update_info->end - update_info->start) >> PAGE_SHIFT;

	for (idx_offset = 0; idx_offset < page_count; idx_offset++) {
		old_state =
			local_state_info->local_mem_state_arr[local_page_idx_start + idx_offset];
		state_count = info->mem_state_arr[page_idx_start + idx_offset];

		/* Check for conflicts after simulating permission changes */
		/* Remove old permissions */
		state_count = update_state_count(state_count, old_state, false);
		ret = check_target_state_allowed(state_count, update_info->mem_state);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * Increase global page permission count (for mmap).
 * The caller holds region state_mutex lock.
 */
void add_mapping_permission(struct obmm_region *reg, struct vm_area_struct *vma, uint8_t mem_state)
{
	int idx_offset, page_idx_start, page_count;
	uint32_t state_count;
	struct obmm_local_state_info *local_state_info;
	struct obmm_ownership_info *info;

	info = reg->ownership_info;
	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;
	page_idx_start = vma_addr_to_page_idx(vma, local_state_info, vma->vm_start);
	page_count = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	for (idx_offset = 0; idx_offset < page_count; idx_offset++) {
		state_count = info->mem_state_arr[page_idx_start + idx_offset];
		state_count = update_state_count(state_count, mem_state, true);
		info->mem_state_arr[page_idx_start + idx_offset] = state_count;
	}
}

/**
 * Update global page permission count and VMA local permissions.
 * The caller holds region state_mutex lock.
 */
void update_ownership(struct obmm_region *reg, struct vm_area_struct *vma,
		      const struct obmm_cmd_update_range *update_info)
{
	int idx_offset, page_idx_start, page_count, local_page_idx_start;
	uint32_t state_count;
	uint8_t old_state;
	struct obmm_local_state_info *local_state_info;
	struct obmm_ownership_info *info;

	info = reg->ownership_info;
	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;

	page_idx_start = vma_addr_to_page_idx(vma, local_state_info, update_info->start);
	local_page_idx_start = vma_addr_to_page_idx_local(vma, update_info->start);
	page_count = (update_info->end - update_info->start) >> PAGE_SHIFT;

	for (idx_offset = 0; idx_offset < page_count; idx_offset++) {
		old_state =
			local_state_info->local_mem_state_arr[local_page_idx_start + idx_offset];

		state_count = info->mem_state_arr[page_idx_start + idx_offset];
		/* Remove old permissions */
		state_count = update_state_count(state_count, old_state, false);
		/* Add new permissions */
		state_count = update_state_count(state_count, update_info->mem_state, true);

		/* update mem_state_arr */
		info->mem_state_arr[page_idx_start + idx_offset] = state_count;
		/* update vma local_state_info */
		local_state_info->local_mem_state_arr[local_page_idx_start + idx_offset] =
			update_info->mem_state;
	}
}

/**
 * Remove global page permission count.
 * The caller holds region state_mutex lock.
 */
void remove_mapping_permission(struct obmm_region *reg, struct vm_area_struct *vma,
			       unsigned long start, unsigned long end)
{
	int idx_offset, page_idx_start, page_count, local_page_idx_start;
	uint32_t state_count;
	uint8_t old_state;
	struct obmm_local_state_info *local_state_info;
	struct obmm_ownership_info *info;

	info = reg->ownership_info;
	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;

	page_idx_start = vma_addr_to_page_idx(vma, local_state_info, start);
	local_page_idx_start = vma_addr_to_page_idx_local(vma, start);
	page_count = (end - start) >> PAGE_SHIFT;

	for (idx_offset = 0; idx_offset < page_count; idx_offset++) {
		old_state =
			local_state_info->local_mem_state_arr[local_page_idx_start + idx_offset];
		state_count = info->mem_state_arr[page_idx_start + idx_offset];

		/*  Remove permissions */
		state_count = update_state_count(state_count, old_state, false);
		info->mem_state_arr[page_idx_start + idx_offset] = state_count;
	}
}

int init_local_state_info(struct vm_area_struct *vma, uint8_t mem_state)
{
	struct obmm_local_state_info *local_state_info;
	unsigned long size;
	int ret, i;

	size = vma->vm_end - vma->vm_start;
	local_state_info = kzalloc(sizeof(struct obmm_local_state_info), GFP_KERNEL);
	if (local_state_info == NULL)
		return -ENOMEM;

	local_state_info->npages = size >> PAGE_SHIFT;
	local_state_info->local_mem_state_arr = vmalloc(sizeof(uint8_t) * local_state_info->npages);

	if (local_state_info->local_mem_state_arr == NULL) {
		ret = -ENOMEM;
		goto out_local_state_info;
	}
	for (i = 0; i < local_state_info->npages; i++)
		local_state_info->local_mem_state_arr[i] = mem_state;

	local_state_info->orig_pgoff = vma->vm_pgoff;
	vma->vm_private_data = local_state_info;

	pr_debug("init vma local state: npages=%d, state=%#x\n", local_state_info->npages,
		 mem_state);
	return 0;
out_local_state_info:
	kfree(local_state_info);
	return ret;
}

void release_local_state_info(struct vm_area_struct *vma)
{
	struct obmm_local_state_info *local_state_info;

	local_state_info = (struct obmm_local_state_info *)vma->vm_private_data;

	vma->vm_private_data = NULL;
	vfree(local_state_info->local_mem_state_arr);
	kfree(local_state_info);
}

/*
 * Initialize the global page permission count array.
 * The obmm_ownership_info is created when the region is mmapped for the first time,
 * so the caller need to hold region state_mutex lock.
 */
int init_ownership_info(struct obmm_region *reg)
{
	struct obmm_ownership_info *info;
	int i, ret;

	if (reg->ownership_info)
		return 0;
	info = kzalloc(sizeof(struct obmm_ownership_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	info->npages = reg->mem_size >> PAGE_SHIFT;
	info->mem_state_arr = vmalloc(sizeof(uint32_t) * info->npages);
	if (info->mem_state_arr == NULL) {
		ret = -ENOMEM;
		goto out_free_info;
	}
	for (i = 0; i < info->npages; i++)
		info->mem_state_arr[i] = 0;

	reg->ownership_info = info;

	pr_debug("init ownership: npages=%d, state=%#x\n", info->npages, 0U);
	return 0;
out_free_info:
	kfree(info);
	return ret;
}

void release_ownership_info(struct obmm_region *reg)
{
	struct obmm_ownership_info *info = reg->ownership_info;

	reg->ownership_info = NULL;
	vfree(info->mem_state_arr);
	kfree(info);
}
