/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __DN200_IATU_H__
#define __DN200_IATU_H__

#include "common.h"

#define DN200_MAX_IATU_TBL_SIZE_PER_FUNC  16
#define DN200_MAX_IATU_MAP_SHIFT 4
#define DN200_MAX_IATU_TBL_SIZE  32
#define DN200_MAX_IATU_MAP_PER_QUEUE BIT(DN200_MAX_IATU_MAP_SHIFT)
#define DN200_MAX_IATU_MAP_RX 7
#define DN200_MAX_IATU_MAP_TX BIT(DN200_MAX_IATU_MAP_SHIFT - 1)
#define DN200_BASE_IATU_ADDR 0x00E000000000ULL
#define DN200_RAID_BASE_IATU_ADDR 0x80E000000000ULL
#define DN200_AXI_HIGH_ADDR_REG_BASE 0x20000344
#define DN200_AXI_HIGH_24BIT_VAL 0x000080

/*record per iatu mapped info for a tx_q*/
struct iatu_con_hashmap {
	struct list_head node;
	u16 iatu_index;
	u64 target_addr;
	atomic_t ref_count;
	atomic_t *global_ref_ptr;
	bool is_rx;
};

struct iatu_map_info {
	struct list_head list;
};

struct iatu_cached_target {
	u64 cached_target;
	u16 iatu_index;
	atomic_t *ref_count_ptr;
	bool is_rx;
};

struct dn200_queue_iatu_info {
	struct iatu_map_info map_info[DN200_MAX_IATU_MAP_PER_QUEUE];
	struct iatu_cached_target cached;
};

struct dn200_iatu_tbl_entry {
	u64 tgt_addr;		/* target address used as high address */
	u64 base_addr;		/* base address used to compare */
	u16 iatu_offset;	/*iatu global offset */
	u64 limit_range;	/*limit_range maximum BIT(32) */
	u64 limit_mask;		/*limit_mask maximum BIT(32) - 1 */
	u8 pf_id;
	u8 is_vf;
	u8 vf_offset;
};

struct dn200_func_iatu_basic_info {
	u16 max_tx_iatu_num;
	u16 max_rx_iatu_num;
	struct dn200_iatu_tbl_entry tbl[DN200_MAX_IATU_TBL_SIZE];
};

struct dn200_priv_iatu_map {
	u16 iatu_index;
	u64 target_addr;
	atomic_t global_ref;
	bool is_rx;
};

struct dn200_priv_iatu_info {
	spinlock_t rx_lock; /* rx iatu lock */
	spinlock_t tx_lock; /* tx iatu lock */
	struct dn200_priv_iatu_map dma32_info;
	struct dn200_priv_iatu_map rx_info[DN200_MAX_IATU_MAP_RX];
	struct dn200_priv_iatu_map tx_info[DN200_MAX_IATU_MAP_TX +
					   DN200_MAX_IATU_MAP_RX];
	struct dn200_func_iatu_basic_info basic_info;
};

#define atomic_wait_sch(v) \
do {} while (atomic_cmpxchg(v, 0, 1))

#define atomic_wait_trysch(v) !atomic_cmpxchg(v, 0, 1)
#define atomic_free_sch(v) atomic_set(v, 0)

enum dn200_iatu_type {
	IATU_TX,
	IATU_RX,
	IATU_DMA32,
};

#define DN200_IATU_BASE_ADDR_SET(addr, index)
#define DN200_IATU_TAR_ADDR_SET(addr, index)
#define MAX_LIMIT_RANGE_SHIFT 32
#define MAX_LIMIT_RANGE_SIZE  BIT(MAX_LIMIT_RANGE_SHIFT)
#define MAX_LIMIT_MASK (MAX_LIMIT_RANGE_SIZE - 1)

/* e.g. change LIMIT_RANGE_SHIFT to 26 bit, each iatu just cover 64MB */
#define LIMIT_RANGE_SHIFT 32	//MAX_LIMIT_RANGE_SHIFT
#define LIMIT_RANGE_SIZE  BIT(LIMIT_RANGE_SHIFT)
#define LIMIT_MASK (LIMIT_RANGE_SIZE - 1)
#define CMP_ADDR_SHIFT (LIMIT_RANGE_SHIFT)

#endif
