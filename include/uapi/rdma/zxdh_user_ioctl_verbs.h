/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR Linux-OpenIB */
/* Copyright (c) 2024 ZTE Corporation.  All rights reserved. */
#ifndef ZXDH_USER_IOCTL_VERBS_H
#define ZXDH_USER_IOCTL_VERBS_H

#include <linux/types.h>

struct zxdh_query_qpc_resp {
	__u8 retry_flag;
	__u8 rnr_retry_flag;
	__u8 read_retry_flag;
	__u8 cur_retry_count;
	__u8 retry_cqe_sq_opcode;
	__u8 err_flag;
	__u8 ack_err_flag;
	__u8 package_err_flag;
	__u8 recv_err_flag;
	__u8 retry_count;
	__u32 tx_last_ack_psn;
};

struct zxdh_modify_qpc_req {
	__u8 retry_flag;
	__u8 rnr_retry_flag;
	__u8 read_retry_flag;
	__u8 cur_retry_count;
	__u8 retry_cqe_sq_opcode;
	__u8 err_flag;
	__u8 ack_err_flag;
	__u8 package_err_flag;
};

#endif