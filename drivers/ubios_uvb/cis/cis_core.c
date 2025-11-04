// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Call ID Service (CIS) core module, manages inter-process communication
 *              via call identifiers with local/remote handling and UVB integration.
 * Author: zhangrui
 * Create: 2025-04-18
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "cis_info_process.h"
#include "uvb_info_process.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Call ID Service Framework");

static bool cis_call_for_me(u32 receiver_id)
{
	if ((receiver_id == UBIOS_USER_ID_ALL) ||
		(receiver_id == ubios_get_user_type(UBIOS_MY_USER_ID)) ||
		(receiver_id == UBIOS_MY_USER_ID)) {
		return true;
	}

	return false;
}

static bool cis_call_for_local(u32 receiver_id)
{
	if ((ubios_get_user_type(receiver_id) == UBIOS_USER_ID_INTERGRATED_UB_DEVICE) ||
		(ubios_get_user_type(receiver_id) == UBIOS_USER_ID_INTERGRATED_PCIE_DEVICE)) {
		return true;
	}

	return false;
}

int cis_call_remote(u32 call_id, u32 sender_id, u32 receiver_id,
					struct cis_message *msg,
					bool is_sync)
{
	u32 forwarder_id;
	u32 exact_receiver_id;
	u8 usage;
	u8 index;
	int res;
	struct udfi_para para = { 0 };

	pr_debug(LOG_PRE "cis remote call: call id %08x, sender id %08x, receiver id %08x\n",
			call_id, sender_id, receiver_id);
	res = get_cis_group_info(call_id, receiver_id,
		&usage, &index, &exact_receiver_id, &forwarder_id);
	if (res) {
		pr_err(ERR_PRE "can't get group info, call id=%08x, receiver id=%08x\n",
			call_id, receiver_id);
		return -EOPNOTSUPP;
	}

	para.input = msg->input;
	para.input_size = msg->input_size;
	para.output = msg->output;
	para.output_size = msg->p_output_size;
	para.message_id = call_id;
	para.receiver_id = exact_receiver_id;
	para.sender_id = sender_id;
	para.forwarder_id = forwarder_id;

	if (usage != CIS_USAGE_UVB) {
		pr_err(ERR_PRE "method not supported, call id=%08x, receiver id=%08x, usage=%d\n",
			call_id, receiver_id, usage);
		return -EOPNOTSUPP;
	}

	if (is_sync)
		return cis_call_uvb_sync(index, &para);

	return cis_call_uvb(index, &para);
}

/**
 * cis_call - Trigger a cis call with given aruguments.
 *
 * @call_id:     call id that identifies which cis call will be triggered.
 * @sender_id:   user id of sender.
 * @receiver_id: user id of receiver.
 * @msg:         the data that the user needs to transmit.
 * @is_sync:     whether to use a synchronous interface.
 *
 * Search for cia (call id attribute) in cis info with given call id and receiver id.
 * The `usage` property of cia determines which method to used (uvb/arch call).
 * Return 0 if cis call succeeds or communication method is not supported,
 * else return cis error code.
 */
int cis_call_by_uvb(u32 call_id, u32 sender_id, u32 receiver_id,
					struct cis_message *msg, bool is_sync)
{
	int ret;
	msg_handler func;

	pr_info(LOG_PRE "cis call: call id %08x, sender id %08x, receiver id %08x\n",
			call_id, sender_id, receiver_id);
	if (cis_call_for_me(receiver_id) || cis_call_for_local(receiver_id)) {
		func = search_local_cis_func(call_id, receiver_id);
		if (func) {
			ret = func(msg);
			if (ret) {
				pr_err(ERR_PRE "cis call execute registered cis func failed\n");
				return ret;
			}
			pr_info(LOG_PRE "cis call execute registered cis func success\n");
			return 0;
		}
		pr_err(ERR_PRE "can't found cis func for callid=%08x, receiver_id=%08x\n",
			call_id, receiver_id);
		return -EOPNOTSUPP;
	}

	return cis_call_remote(call_id, sender_id, receiver_id, msg, is_sync);
}
EXPORT_SYMBOL(cis_call_by_uvb);

int cis_module_lock_func(int lock)
{
	if (lock)
		return try_module_get(THIS_MODULE) ? 0 : -EINVAL;

	module_put(THIS_MODULE);

	return 0;
}
EXPORT_SYMBOL(cis_module_lock_func);

static int __init cis_init(void)
{
	int err = 0;

	err = init_cis_table();
	if (err) {
		pr_err(ERR_PRE "cis info init failed, err=%d\n", err);
		goto fail;
	}

	err = init_global_vars();
	if (err) {
		pr_err(ERR_PRE "global vars malloc failed, err=%d\n", err);
		goto free_global;
	}

	err = init_uvb();
	if (err) {
		pr_err(ERR_PRE "uvb init failed, err=%d\n", err);
		goto fail;
	}

	pr_info(LOG_PRE "cis init success\n");
	return 0;
fail:
	uninit_uvb();
free_global:
	free_global_vars();

	return err;
}

static void __exit cis_exit(void)
{
	uninit_uvb();
	free_global_vars();
	pr_info(LOG_PRE "cis exit success\n");
}

module_init(cis_init);
module_exit(cis_exit);
