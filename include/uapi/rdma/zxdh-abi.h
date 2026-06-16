/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR Linux-OpenIB */
/* Copyright (c) 2024 ZTE Corporation.  All rights reserved. */

#ifndef ZXDH_ABI_H
#define ZXDH_ABI_H

#include <linux/types.h>

#define ZXDH_ABI_VER 5

enum zxdh_memreg_type {
	ZXDH_MEMREG_TYPE_MEM = 0,
	ZXDH_MEMREG_TYPE_QP = 1,
	ZXDH_MEMREG_TYPE_CQ = 2,
	ZXDH_MEMREG_TYPE_SRQ = 3,
};

enum zxdh_db_addr_type {
	ZXDH_DB_ADDR_PHY = 0,
	ZXDH_DB_ADDR_BAR = 1,
};

struct zxdh_alloc_ucontext_req {
	__u32 rsvd32;
	__u8 userspace_ver;
	__u8 rsvd8[3];
};

struct zxdh_alloc_ucontext_resp {
	__u32 max_pds;
	__u32 max_qps;
	__u32 wq_size; /* size of the WQs (SQ+RQ) in the mmaped area */
	__u8 kernel_ver;
	__u8 db_addr_type;
	__u8 rsvd[2];
	__aligned_u64 feature_flags;
	__aligned_u64 sq_db_mmap_key;
	__aligned_u64 cq_db_mmap_key;
	__aligned_u64 sq_db_pa;
	__aligned_u64 cq_db_pa;
	__u32 max_hw_wq_frags;
	__u32 max_hw_read_sges;
	__u32 max_hw_inline;
	__u32 max_hw_rq_quanta;
	__u32 max_hw_srq_quanta;
	__u32 max_hw_wq_quanta;
	__u32 max_hw_srq_wr;
	__u32 min_hw_cq_size;
	__u32 max_hw_cq_size;
	__u16 max_hw_sq_chunk;
	__u8 hw_rev;
	__u8 rsvd2;
};

struct zxdh_alloc_pd_resp {
	__u32 pd_id;
	__u8 rsvd[4];
};

struct zxdh_resize_cq_req {
	__aligned_u64 user_cq_buffer;
};

struct zxdh_create_cq_req {
	__aligned_u64 user_cq_buf;
	__aligned_u64 user_shadow_area;
};

struct zxdh_create_qp_req {
	__aligned_u64 user_wqe_bufs;
	__aligned_u64 user_compl_ctx;
};

struct zxdh_create_srq_req {
	__aligned_u64 user_wqe_bufs;
	__aligned_u64 user_compl_ctx;
	__aligned_u64 user_wqe_list;
	__aligned_u64 user_wqe_db;
};

struct zxdh_mem_reg_req {
	__u16 reg_type; /* enum zxdh_memreg_type */
	__u16 cq_pages;
	__u16 rq_pages;
	__u16 sq_pages;
	__u16 srq_pages;
	__u16 srq_list_pages;
	__u8 rsvd[4];
};

struct zxdh_reg_mr_resp {
	__u32 mr_pa_low;
	__u32 mr_pa_hig;
	__u16 host_page_size;
	__u16 leaf_pbl_size;
	__u8 rsvd[4];
};

struct zxdh_modify_qp_req {
	__u8 sq_flush;
	__u8 rq_flush;
	__u8 rsvd[6];
};

struct zxdh_create_cq_resp {
	__u32 cq_id;
	__u32 cq_size;
};

struct zxdh_create_qp_resp {
	__u32 qp_id;
	__u32 actual_sq_size;
	__u32 actual_rq_size;
	__u32 zxdh_drv_opt;
	__u16 push_idx;
	__u8 lsmm;
	__u8 rsvd;
	__u32 qp_caps;
};

struct zxdh_create_srq_resp {
	__u32 srq_id;
	__u32 actual_srq_size;
	__u32 actual_srq_list_size;
	__u8 rsvd[4];
};

struct zxdh_modify_qp_resp {
	__aligned_u64 push_wqe_mmap_key;
	__aligned_u64 push_db_mmap_key;
	__u16 push_offset;
	__u8 push_valid;
	__u8 rsvd[5];
};

struct zxdh_create_ah_resp {
	__u32 ah_id;
	__u8 rsvd[4];
};
#endif /* ZXDH_ABI_H */