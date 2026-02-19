/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#ifndef _UB_UBASE_COMM_HW_H_
#define _UB_UBASE_COMM_HW_H_

#include <linux/types.h>
#include <ub/ubase/ubase_comm_dev.h>

#define UBASE_AEQ_CTX_SIZE		64
#define UBASE_CEQ_CTX_SIZE		64
#define UBASE_JFS_CTX_SIZE		256
#define UBASE_JFR_CTX_SIZE		64
#define UBASE_JFC_CTX_SIZE		128
#define UBASE_RC_CTX_SIZE		256
#define UBASE_JTG_CTX_SIZE		8
#define UBASE_TP_CTX_SIZE		256
#define UBASE_TPG_CTX_SIZE		64

#define UBASE_DESC_DATA_LEN		6

/**
 * struct ubase_cmdq_desc - Command queue descriptor
 * @opcode: Command opcode
 * @flag: Command flag
 * @bd_num: bd number. One bd is 32 bytes, and the first db is 24 bytes.
 * @ret: Command return value
 * @rsv: reserved
 * @data: Command data
 */
struct ubase_cmdq_desc {
	__le16 opcode;
	u8 flag;
	u8 bd_num;
	__le16 ret;
	__le16 rsv;
	__le32 data[UBASE_DESC_DATA_LEN];
};

/**
 * struct ubase_cmdq_ring - Command ring queue information
 * @ci: consumer indicator
 * @pi: producer indicator
 * @desc_num: descriptors number
 * @tx_timeout: transmit timeout interval
 * @desc_dma_addr: dma address of descriptors
 * @desc: Command queue descriptor
 * @lock: spinlock
 */
struct ubase_cmdq_ring {
	u32 ci;
	u32 pi;
	u32 desc_num;
	u32 tx_timeout;
	dma_addr_t desc_dma_addr;
	struct ubase_cmdq_desc *desc;
	spinlock_t lock;
};

/**
 * struct ubase_cmdq - cmmand queue
 * @csq: command send queue
 * @crq: command receive queue
 */
struct ubase_cmdq {
	struct ubase_cmdq_ring csq;
	struct ubase_cmdq_ring crq;
};

/**
 * struct ubase_hw - hardware information
 * @rs0_base: resource0 space base addr
 * @io_base: io space base addr
 * @mem_base: memory space base addr
 * @cmdq: command queue
 * @state: state of the hardware
 */
struct ubase_hw {
	struct ubase_resource_space rs0_base;
	struct ubase_resource_space io_base;
	struct ubase_resource_space mem_base;
	struct ubase_cmdq cmdq;
	unsigned long state;
};

/**
 * struct ubase_mbx_event_context - mailbox event context
 * @done: completion object to wait for event
 * @result: mailbox execution result
 * @out_param: mailbox output parameter
 * @seq_num: mailbox sequence number
 */
struct ubase_mbx_event_context {
	struct completion	done;
	int			result;
	u64			out_param;
	u16			seq_num;
};

#endif
