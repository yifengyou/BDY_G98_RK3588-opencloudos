/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#ifndef __UDMA_JFS_H__
#define __UDMA_JFS_H__

#include "udma_common.h"

#define MAX_WQEBB_NUM 4
#define UDMA_JFS_WQEBB_SIZE 64
#define UDMA_JFS_SGE_SIZE 16

#define SQE_WRITE_NOTIFY_CTL_LEN 80

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

static inline struct udma_jfs *to_udma_jfs(struct ubcore_jfs *jfs)
{
	return container_of(jfs, struct udma_jfs, ubcore_jfs);
}

static inline struct udma_jfs *to_udma_jfs_from_queue(struct udma_jetty_queue *queue)
{
	return container_of(queue, struct udma_jfs, sq);
}

static inline uint32_t sq_cal_wqebb_num(uint32_t sqe_ctl_len, uint32_t sge_num)
{
	return (sqe_ctl_len + (sge_num - 1) * UDMA_JFS_SGE_SIZE) /
		UDMA_JFS_WQEBB_SIZE + 1;
}

struct ubcore_jfs *udma_create_jfs(struct ubcore_device *ub_dev,
				   struct ubcore_jfs_cfg *cfg,
				   struct ubcore_udata *udata);
int udma_alloc_u_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct udma_create_jetty_ucmd *ucmd);
int udma_alloc_k_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq,
			struct ubcore_jfs_cfg *jfs_cfg);
void udma_free_sq_buf(struct udma_dev *dev, struct udma_jetty_queue *sq);

#endif /* __UDMA_JFS_H__ */
