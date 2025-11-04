/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: cis info process header
 * Author: zhangrui
 * Create: 2025-04-18
 */

#ifndef CIS_INFO_PROCESS_H
#define CIS_INFO_PROCESS_H

#include <ubios/cis.h>
#include "cis_uvb_interface.h"

extern struct cis_message *io_param_sync;
extern struct list_head  g_local_cis_list;
extern spinlock_t cis_register_lock;

struct udfi_para {
	u32 message_id;
	u32 sender_id;
	u32 receiver_id;
	u32 forwarder_id;
	void *input;
	u32 input_size;
	void *output;
	u32 *output_size;
};

struct cis_func_node {
	struct list_head link;
	u32 call_id;
	u32 receiver_id;
	msg_handler func;
};

int init_cis_table(void);
int init_global_vars(void);
void free_global_vars(void);

int get_cis_group_info(u32 call_id, u32 receiver_id,
						u8 *usage, u8 *index,
						u32 *exact_receiver_id, u32 *forwarder_id);
int cis_call_remote(u32 call_id, u32 sender_id, u32 receiver_id,
					struct cis_message *msg,
					bool is_sync);
msg_handler search_my_cis_func(u32 call_id);
msg_handler search_local_cis_func(u32 call_id, u32 receiver_id);

static inline u32 ubios_get_user_type(u32 user_id)
{
	return user_id & UBIOS_USER_TYPE_MASK;
}
static inline u32 ubios_get_user_index(u32 user_id)
{
	return user_id & UBIOS_USER_INDEX_MASK;
}

#endif
