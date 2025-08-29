/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved. */

#ifndef __CDMA_HANDLE_H__
#define __CDMA_HANDLE_H__

#include "cdma_segment.h"
#include "cdma_queue.h"
#include "cdma.h"

int cdma_write(struct cdma_dev *cdev, struct cdma_queue *queue,
	       struct dma_seg *local_seg, struct dma_seg *rmt_seg);

#endif /* CDMA_HANDLE_H */
