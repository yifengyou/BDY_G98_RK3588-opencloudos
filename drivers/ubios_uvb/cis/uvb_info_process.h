/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: uvb info process header
 * Author: zhangrui
 * Create: 2025-04-18
 */

#ifndef UVB_INFO_PROCESS_H
#define UVB_INFO_PROCESS_H

#include <linux/atomic.h>
#include <linux/hashtable.h>
#include <linux/types.h>

#include "cis_uvb_interface.h"

#define CIS_USAGE_UVB                    2

#define UVB_POLL_TIME_INTERVAL           (100)                  /* 100us */
#define UVB_POLL_TIMEOUT                 (1000)                 /* 1000ms */
#define UVB_TIMEOUT_WINDOW_OBTAIN        (10000)                /* 10000ms */
#define UVB_POLL_TIMEOUT_TIMES           (10000)                /* 10000 times */

int init_uvb(void);
void uninit_uvb(void);
void uninit_uvb_sync(void);

#define MAX_UVB_LOCK_IN_BITS 8
struct uvb_window_lock {
	atomic_t lock;
	u64 window_address;
	struct hlist_node node;
};
extern DECLARE_HASHTABLE(uvb_lock_table, MAX_UVB_LOCK_IN_BITS);

struct uvb_window_description *uvb_occupy_window(struct uvb *uvb, u32 sender_id, u64 *wd_obtain);
int uvb_free_window(struct uvb_window *window);
int uvb_fill_window(struct uvb_window_description *wd, struct uvb_window *wd_addr,
			struct cis_message *io_params, struct udfi_para *para);
int uvb_poll_window_call(struct uvb_window *window, u32 call_id);
int uvb_poll_window_call_sync(struct uvb_window *window, u32 call_id);
int uvb_get_output_data(struct uvb_window *window,
	struct cis_message *io_param, void *output, u32 *output_size);

/* cis call by uvb */
int cis_call_uvb(u8 index, struct udfi_para *para);
int cis_call_uvb_sync(u8 index, struct udfi_para *para);
#endif
