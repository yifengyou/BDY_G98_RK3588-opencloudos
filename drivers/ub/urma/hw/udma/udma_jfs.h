/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#ifndef __UDMA_JFS_H__
#define __UDMA_JFS_H__

#include "udma_common.h"

#define MAX_WQEBB_NUM 4
#define UDMA_SQE_RMT_EID_SIZE 16
#define SQE_WRITE_IMM_CTL_LEN 64
#define SQE_NORMAL_CTL_LEN 48
#define ATOMIC_WQEBB_CNT 2
#define NOP_WQEBB_CNT 1
#define UDMA_JFS_WQEBB_SIZE 64
#define UDMA_JFS_SGE_SIZE 16
#define UDMA_JFS_MAX_SGE_READ 6
#define UDMA_JFS_MAX_SGE_WRITE_IMM 12
#define UDMA_ATOMIC_SGE_NUM 1
#define UDMA_ATOMIC_LEN_4 4
#define UDMA_ATOMIC_LEN_8 8
#define UDMA_ATOMIC_LEN_16 16
#define SQE_CTL_RMA_ADDR_OFFSET 32
#define SQE_CTL_RMA_ADDR_BIT GENMASK(31, 0)
#define SQE_ATOMIC_DATA_FIELD 64
#define SQE_SEND_IMM_FIELD 40
#define WRITE_IMM_TOKEN_FIELD 56
#define SQE_WRITE_IMM_FIELD 48

#define SQE_WRITE_NOTIFY_CTL_LEN 80
#define SQE_WRITE_IMM_INLINE_SIZE 192

#define UINT8_MAX 0xff

enum udma_jfs_type {
	UDMA_NORMAL_JFS_TYPE,
	UDMA_KERNEL_STARS_JFS_TYPE,
};

struct udma_jfs {
	struct ubcore_jfs ubcore_jfs;
	struct udma_jetty_queue sq;
	uint64_t jfs_addr;
	refcount_t ae_refcount;
	struct completion ae_comp;
	uint32_t mode;
	bool pi_type;
	bool ue_rx_closed;
};

/* thanks to include/rdma/ib_verbs.h */
enum udma_sq_opcode {
	UDMA_OPC_SEND,
	UDMA_OPC_SEND_WITH_IMM,
	UDMA_OPC_SEND_WITH_INVALID,
	UDMA_OPC_WRITE,
	UDMA_OPC_WRITE_WITH_IMM,
	UDMA_OPC_READ = 0x6,
	UDMA_OPC_CAS,
	UDMA_OPC_FAA = 0xb,
	UDMA_OPC_NOP = 0x11,
	UDMA_OPC_INVALID = 0x12,
};

struct udma_jfs_wqebb {
	uint32_t value[16];
};

struct udma_sqe_ctl {
	uint32_t sqe_bb_idx : 16;
	uint32_t place_odr : 2;
	uint32_t comp_order : 1;
	uint32_t fence : 1;
	uint32_t se : 1;
	uint32_t cqe : 1;
	uint32_t inline_en : 1;
	uint32_t rsv : 5;
	uint32_t token_en : 1;
	uint32_t rmt_jetty_type : 2;
	uint32_t owner : 1;
	uint32_t target_hint : 8;
	uint32_t opcode : 8;
	uint32_t rsv1 : 6;
	uint32_t inline_msg_len : 10;
	uint32_t tpn : 24;
	uint32_t sge_num : 8;
	uint32_t rmt_obj_id : 20;
	uint32_t rsv2 : 12;
	uint8_t rmt_eid[UDMA_SQE_RMT_EID_SIZE];
	uint32_t rmt_token_value;
	uint32_t rsv3;
	uint32_t rmt_addr_l_or_token_id;
	uint32_t rmt_addr_h_or_token_value;
};

struct udma_normal_sge {
	uint32_t length;
	uint32_t token_id;
	uint64_t va;
};

struct udma_token_info {
	uint32_t token_id : 20;
	uint32_t rsv : 12;
	uint32_t token_value;
};

static inline struct udma_jfs *to_udma_jfs(struct ubcore_jfs *jfs)
{
	return container_of(jfs, struct udma_jfs, ubcore_jfs);
}

static inline struct udma_jfs *to_udma_jfs_from_queue(struct udma_jetty_queue *queue)
{
	return container_of(queue, struct udma_jfs, sq);
}

static inline bool to_check_sq_overflow(struct udma_jetty_queue *sq,
					uint32_t wqebb_cnt)
{
	return sq->pi - sq->ci + wqebb_cnt > sq->buf.entry_cnt;
}

static inline uint32_t sq_cal_wqebb_num(uint32_t sqe_ctl_len, uint32_t sge_num)
{
	return (sqe_ctl_len + (sge_num - 1) * UDMA_JFS_SGE_SIZE) /
		UDMA_JFS_WQEBB_SIZE + 1;
}

static inline uint32_t get_ctl_len(uint8_t opcode)
{
	return opcode == UDMA_OPC_WRITE_WITH_IMM ? SQE_WRITE_IMM_CTL_LEN : SQE_NORMAL_CTL_LEN;
}

struct ubcore_jfs *udma_create_jfs(struct ubcore_device *ub_dev,
				   struct ubcore_jfs_cfg *cfg,
				   struct ubcore_udata *udata);
int udma_destroy_jfs(struct ubcore_jfs *jfs);
int udma_destroy_jfs_batch(struct ubcore_jfs **jfs_arr, int jfs_num, int *bad_jfs_index);
int udma_alloc_u_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct udma_create_jetty_ucmd *ucmd);
int udma_alloc_k_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct ubcore_jfs_cfg *jfs_cfg);
void udma_free_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq);
int udma_modify_jfs(struct ubcore_jfs *jfs, struct ubcore_jfs_attr *attr,
		    struct ubcore_udata *udata);
int udma_flush_jfs(struct ubcore_jfs *jfs, int cr_cnt, struct ubcore_cr *cr);
int udma_post_sq_wr(struct udma_dev *udma_dev, struct udma_jetty_queue *sq,
		    struct ubcore_jfs_wr *wr, struct ubcore_jfs_wr **bad_wr);
int udma_post_jfs_wr(struct ubcore_jfs *jfs, struct ubcore_jfs_wr *wr,
		     struct ubcore_jfs_wr **bad_wr);
void udma_flush_sq(struct udma_dev *udma_dev,
		   struct udma_jetty_queue *sq, struct ubcore_cr *cr);

#endif /* __UDMA_JFS_H__ */
