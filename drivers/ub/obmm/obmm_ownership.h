/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifndef OBMM_OWNERSHIP_H
#define OBMM_OWNERSHIP_H

#include "obmm_core.h"

#define WRITE_COUNT_BIT 16
#define READ_COUNT_BIT 16

#define WRITE_MASK ((1 << WRITE_COUNT_BIT) - 1) /* 16-bit mask */
#define READ_MASK  ((1 << READ_COUNT_BIT) - 1)  /* 16-bit mask */

#define MAX_WRITE_COUNT WRITE_MASK
#define MAX_READ_COUNT READ_MASK

#define WRITE_SHIFT   0
#define READ_SHIFT    (WRITE_COUNT_BIT)

#define GET_W_COUNTER(val)   (((val) >> WRITE_SHIFT) & WRITE_MASK)
#define GET_R_COUNTER(val)   (((val) >> READ_SHIFT) & READ_MASK)

/*
 *       [ 16-31  : 0-15  ]
 * state:[ Read   : Write ]
 *       [ 65535  : 65535 ]
 */
struct obmm_ownership_info {
	uint32_t *mem_state_arr;
	int npages;
};

struct obmm_local_state_info {
	uint8_t *local_mem_state_arr;
	/* Original file offset in vma */
	unsigned long orig_pgoff;
	int npages;
};
int vma_addr_to_page_idx_local(struct vm_area_struct *vma, unsigned long addr);
uint8_t infer_cache_ops(uint8_t cur_state, uint8_t target_state);
int init_ownership_info(struct obmm_region *reg);
int init_local_state_info(struct vm_area_struct *vma, uint8_t mem_state);
void release_ownership_info(struct obmm_region *reg);
void release_local_state_info(struct vm_area_struct *vma);
void add_mapping_permission(struct obmm_region *reg, struct vm_area_struct *vma, uint8_t mem_state);
void update_ownership(struct obmm_region *reg, struct vm_area_struct *vma,
		      const struct obmm_cmd_update_range *update_info);
int check_modify_ownership_allowed(struct obmm_region *reg, struct vm_area_struct *vma,
				   const struct obmm_cmd_update_range *update_info);
int check_mmap_allowed(struct obmm_region *reg, struct vm_area_struct *vma, uint8_t mem_state);
void remove_mapping_permission(struct obmm_region *reg, struct vm_area_struct *vma,
			       unsigned long start, unsigned long end);
#endif
