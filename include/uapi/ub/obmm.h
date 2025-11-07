/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifndef UAPI_OBMM_H
#define UAPI_OBMM_H

#include <linux/types.h>

#if defined(__cplusplus)
extern "C" {
#endif


#define OBMM_MAX_LOCAL_NUMA_NODES   16
#define MAX_NUMA_DIST               254
#define OBMM_MAX_PRIV_LEN           512
#define OBMM_MAX_VENDOR_LEN         128

enum obmm_query_key_type {
	OBMM_QUERY_BY_PA,
	OBMM_QUERY_BY_ID_OFFSET
};

struct obmm_cmd_addr_query {
	/* key type decides the input and output */
	enum obmm_query_key_type key_type;
	__u64 mem_id;
	__u64 offset;
	__u64 pa;
} __attribute__((aligned(8)));


#define OBMM_CMD_ADDR_QUERY  _IOWR('x', 4, struct obmm_cmd_addr_query)

/* cache maintenance operations (not states) */
/* no cache maintenance (nops) */
#define OBMM_SHM_CACHE_NONE             0x0
/* invalidate only (in-cache modifications may not be written back to DRAM) */
#define OBMM_SHM_CACHE_INVAL            0x1
/* write back and invalidate */
#define OBMM_SHM_CACHE_WB_INVAL         0x2
/* write back only */
#define OBMM_SHM_CACHE_WB_ONLY         0x3
/* Automatically choose the cache maintenance action depending on the memory
 * state. The resulting choice always make sure no data would be lost, and might
 * be more conservative than necessary.
 */
#define OBMM_SHM_CACHE_INFER            0x4

#if defined(__cplusplus)
}
#endif

#endif /* UAPI_OBMM_H */
