// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Call ID Service (CIS) info processing module, handles CIS init,
 *              func register/lookup and group info retrieval.
 * Author: zhangrui
 * Create: 2025-04-18
 */
#define pr_fmt(fmt) "[UVB]: " fmt

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/hashtable.h>
#include <linux/delay.h>
#include "cis_info_process.h"
#include "uvb_info_process.h"

LIST_HEAD(g_local_cis_list);
DEFINE_SPINLOCK(cis_register_lock);
struct cis_message *io_param_sync;

void ubios_prepare_output_data(struct cis_message *io_param, void *output, u32 *output_size)
{
	memcpy(output, io_param->output, *(io_param->p_output_size));
	*output_size = *(io_param->p_output_size);
}

static bool is_call_id_supported(struct cis_group *group, u32 call_id)
{
	u32 i;

	for (i = 0; i < group->cis_count; i++) {
		pr_debug("cia call_id: %08x\n", group->call_id[i]);
		if (group->call_id[i] == call_id)
			return true;
	}

	return false;
}

int get_cis_group_info(u32 call_id, u32 receiver_id,
					u8 *usage, u8 *index,
					u32 *exact_receiver_id, u32 *forwarder_id)
{
	u32 i;

	if (!g_cis_info) {
		pr_err("can't get cis_info from odf\n");
		return -EOPNOTSUPP;
	}

	for (i = 0; i < g_cis_info->group_count; i++) {
		if (receiver_id != g_cis_info->groups[i]->owner_user_id &&
			receiver_id != ubios_get_user_type(g_cis_info->groups[i]->owner_user_id))
			continue;
		if (is_call_id_supported(g_cis_info->groups[i], call_id)) {
			*usage = g_cis_info->groups[i]->usage;
			*index = g_cis_info->groups[i]->index;
			*exact_receiver_id = g_cis_info->groups[i]->owner_user_id;
			*forwarder_id = g_cis_info->groups[i]->forwarder_id;
			return 0;
		}
	}

	if (ubios_get_user_type(receiver_id) == UBIOS_USER_ID_UB_DEVICE) {
		*usage = g_cis_info->ub.usage;
		*index = g_cis_info->ub.index;
		*exact_receiver_id = receiver_id;
		*forwarder_id = g_cis_info->ub.forwarder_id;
		pr_info("refresh info, usage=%d, index=%d, forward_id=%08x\n",
			*usage, *index, *forwarder_id);
		return 0;
	}

	pr_err("call id: %08x not supported\n", call_id);

	return -EOPNOTSUPP;
}

/*
Search Call ID Service owned by this component, return the function.
*/
struct cis_func_node *search_local_cis_func_node(u32 call_id, u32 receiver_id)
{
	struct cis_func_node *cis_node = NULL;
	struct cis_func_node *tmp;

	rcu_read_lock();
	list_for_each_entry_rcu(tmp, &g_local_cis_list, link) {
		if ((tmp->call_id == call_id) && (tmp->receiver_id == receiver_id)) {
			cis_node = tmp;
			break;
		}
	}
	rcu_read_unlock();

	return cis_node;
}

/*
Search local Call ID Service Functon according Call ID, return the function.
*/
msg_handler search_local_cis_func(u32 call_id, u32 receiver_id)
{
	struct cis_func_node *cis_node;

	cis_node = search_local_cis_func_node(call_id, receiver_id);
	if (cis_node)
		return cis_node->func;

	return NULL;
}

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

static atomic_t *find_uvb_window_lock(u64 window_address)
{
	struct uvb_window_lock *entry;

	if (hash_empty(uvb_lock_table))
		return NULL;

	hash_for_each_possible(uvb_lock_table, entry, node, window_address) {
		if (entry->window_address == window_address)
			return &entry->lock;
	}

	return NULL;
}

static int try_obtain_uvb_window(u64 *wd_obtain, u32 sender_id)
{
	if (*wd_obtain == 0) {
		*wd_obtain = sender_id;
		return 1;
	}
	return 0;
}

struct uvb_window_description *uvb_occupy_window(struct uvb *uvb, u32 sender_id, u64 *wd_obtain)
{
	struct uvb_window_description *wd = NULL;
	ktime_t start;
	ktime_t now;
	atomic_t *lock;
	s64 time_interval;
	u32 i;
	u32 round;

	i = 0;
	round = 0;
	start = ktime_get();
	while (1) {
		if (i >= uvb->window_count) {
			i = 0;
			round++;
		}
		wd = &(uvb->wd[i]);
		wd_obtain = memremap(wd->obtain, wd->size, MEMREMAP_WC);
		if (!wd_obtain) {
			pr_err("uvb window obtain map failed\n");
			return NULL;
		}
		lock = find_uvb_window_lock(wd->address);
		if (!lock) {
			pr_err("uvb window lock not found\n");
			goto free_resources;
		}

		if (atomic_cmpxchg(lock, 0, 1) == 0
			&& try_obtain_uvb_window(wd_obtain, sender_id)) {
			atomic_set(lock, 0);
			udelay(uvb->delay);
			if (*wd_obtain == sender_id) {
				now = ktime_get();
				time_interval = ktime_to_us(ktime_sub(now, start));
				pr_info("occupy uvb window successfully, elapsed time: %lldus\n",
					time_interval);
				return wd;
			}
		}

		now = ktime_get();
		time_interval = ktime_to_us(ktime_sub(now, start));
		if (round > 1 && time_interval > UVB_TIMEOUT_WINDOW_OBTAIN) {
			pr_err("obtain window timeout, tried %u * %u = %u times\n",
			round, (u32)(uvb->window_count), round * (u32)(uvb->window_count));
			goto free_resources;
		}
		i++;
		memunmap(wd_obtain);
		wd_obtain = NULL;
	}

free_resources:
	memunmap(wd_obtain);
	wd_obtain = NULL;

	return NULL;
}

void uvb_free_wd_obtain(u64 *wd_obtain)
{
	if (!wd_obtain)
		return;
	*wd_obtain = 0;
	memunmap(wd_obtain);
}

int uvb_free_window(struct uvb_window *window)
{
	if (!window)
		return 0;
	window->input_data_address = 0;
	window->input_data_size = 0;
	window->input_data_checksum = 0;

	window->output_data_address = 0;
	window->output_data_size = 0;
	window->output_data_checksum = 0;
	window->returned_status = 0;
	window->message_id = 0;

	dsb(sy);
	isb();

	window->receiver_id = 0;
	window->sender_id = 0;

	return 0;
}

static int fill_uvb_window_with_buffer(struct uvb_window_description *wd,
			struct uvb_window *window_address,
			struct cis_message *io_params,
			void *input, u32 input_size,
			void *output, u32 *output_size)
{
	struct uvb_window *window;
	void *new_input = NULL;
	void *new_output = NULL;

	window = window_address;
	if (output_size) {
		if (wd->size < (u64)*output_size + (u64)input_size) {
			pr_err("check wd size failed for output size\n");
			return -EOVERFLOW;
		}
		window->output_data_size = *output_size;
	} else {
		window->output_data_size = UVB_OUTPUT_SIZE_NULL;
	}

	if (input) {
		if (wd->size < input_size) {
			pr_err("check wd size failed for input size\n");
			return -EOVERFLOW;
		}
		new_input = memremap(wd->buffer, wd->size, MEMREMAP_WC);
		if (!new_input) {
			pr_err("memremap for wd_buffer_virt_addr failed\n");
			return -ENOMEM;
		}
		memcpy(new_input, input, input_size);
		window->input_data_checksum = checksum32(input, input_size);
	}

	if (output)
		new_output = (void *)(new_input + ALIGN(input_size, sizeof(u64)));

	io_params->input = new_input;
	io_params->input_size = input_size;
	io_params->output = new_output;
	io_params->p_output_size = &(window->output_data_size);

	window->input_data_address = new_input ? wd->buffer : 0;
	window->input_data_size = input_size;
	window->output_data_address = new_output ? wd->buffer + ALIGN(input_size, sizeof(u64)) : 0;

	return 0;
}

int uvb_fill_window(struct uvb_window_description *wd, struct uvb_window *wd_addr,
			struct cis_message *io_params, struct udfi_para *para)
{
	int err;
	struct uvb_window *window;

	window = wd_addr;
	window->message_id = para->message_id;
	window->sender_id = para->sender_id;

	err = fill_uvb_window_with_buffer(wd, window, io_params, para->input,
		para->input_size, para->output, para->output_size);
	if (err) {
		pr_err("fill uvb window with buffer failed\n");
		uvb_free_window(window);
		return err;
	}

	window->receiver_id = para->receiver_id;
	window->forwarder_id = para->forwarder_id;
	pr_info("uvb fill window success\n");

	return 0;
}

int uvb_poll_window_call(struct uvb_window *window, u32 call_id)
{
	ktime_t start;
	ktime_t now;
	s64 time_interval;

	start = ktime_get();
	while (1) {
		if (window->message_id == ~call_id)
			return (int)window->returned_status;

		now = ktime_get();
		time_interval = ktime_to_ms(ktime_sub(now, start));
		if (time_interval > UVB_POLL_TIMEOUT)
			break;
	}

	pr_err("uvb poll window call timeout,wait=%lld ms\n", time_interval);

	return -ETIMEDOUT;
}

int uvb_poll_window_call_sync(struct uvb_window *window, u32 call_id)
{
	int i;

	pr_info("start uvb window polling\n");
	for (i = 0; i < UVB_POLL_TIMEOUT_TIMES; i++) {
		if (window->message_id == ~call_id)
			return (int)window->returned_status;

		udelay(UVB_POLL_TIME_INTERVAL);
	}

	pr_err("uvb poll window call sync timeout\n");

	return -ETIMEDOUT;
}

int uvb_get_output_data(struct uvb_window *window,
	struct cis_message *io_param, void *output, u32 *output_size)
{
	if (!output || !output_size)
		return 0;

	if (*output_size == 0)
		return 0;

	if (window->output_data_address == 0 || window->output_data_size == UVB_OUTPUT_SIZE_NULL)
		return 0;

	if (window->output_data_checksum !=
		checksum32(io_param->output, window->output_data_size)) {
		pr_warn("returned data checksum error\n");
		return -EINVAL;
	}
	ubios_prepare_output_data(io_param, output, output_size);

	return 0;
}

void free_io_param_with_buffer(struct cis_message *io_param)
{
	if (!io_param)
		return;

	if (io_param->input)
		memunmap(io_param->input);
	kfree(io_param);
}

int cis_call_uvb(u8 index, struct udfi_para *para)
{
	int err;
	struct uvb_window *window = NULL;
	struct uvb_window_description *wd = NULL;
	struct cis_message *io_param = NULL;
	u64 *wd_obtain = NULL;

	if (!g_uvb_info) {
		pr_err("uvb unsupported\n");
		return -EOPNOTSUPP;
	}

	if (index >= g_uvb_info->uvb_count) {
		pr_err("cis call uvb index exceed uvb count\n");
		return -EOVERFLOW;
	}

	wd = uvb_occupy_window(g_uvb_info->uvbs[index], para->sender_id, wd_obtain);
	if (!wd) {
		pr_err("obtain window failed\n");
		return -EBUSY;
	}

	if (!wd->buffer) {
		pr_err("no window buffer to save data\n");
		goto free_obtain;
	}

	io_param = kzalloc(sizeof(struct cis_message), GFP_KERNEL);
	if (!io_param) {
		err = -ENOMEM;
		goto free_obtain;
	}

	window = (struct uvb_window *)memremap(wd->address, sizeof(struct uvb_window), MEMREMAP_WC);
	if (!window) {
		pr_err("memremap uvb window failed\n");
		err = -ENOMEM;
		goto free_io_param;
	}

	err = uvb_fill_window(wd, window, io_param, para);
	if (err) {
		pr_err("fill uvb window failed\n");
		goto unmap_window;
	}

	err = uvb_poll_window_call(window, para->message_id);
	if (err) {
		pr_err("call by uvb failed\n");
		goto free_window;
	}

	err = uvb_get_output_data(window, io_param, para->output, para->output_size);
	if (err)
		pr_err("uvb get output data failed\n");

free_window:
	uvb_free_window(window);
unmap_window:
	memunmap(window);
free_io_param:
	free_io_param_with_buffer(io_param);
free_obtain:
	uvb_free_wd_obtain(wd_obtain);
	pr_info("finish cis call by uvb\n");

	return err;
}

int cis_call_uvb_sync(u8 index, struct udfi_para *para)
{
	int err;
	struct uvb_window *window = NULL;
	struct uvb_window_description *wd = NULL;
	u64 *wd_obtain = NULL;

	memset(io_param_sync, 0, sizeof(struct cis_message));

	if (!g_uvb_info) {
		pr_err("sync call uvb unsupported\n");
		return -EOPNOTSUPP;
	}

	if (index >= g_uvb_info->uvb_count) {
		pr_err("sync call use uvb index exceed\n");
		return -EOVERFLOW;
	}

	wd = uvb_occupy_window(g_uvb_info->uvbs[index], para->sender_id, wd_obtain);
	if (!wd) {
		pr_err("sync call obtain window failed\n");
		return -EBUSY;
	}

	if (!wd->buffer) {
		pr_err("sync call no window buffer to save data\n");
		goto free_obtain;
	}

	window = (struct uvb_window *)memremap(wd->address, sizeof(struct uvb_window), MEMREMAP_WC);
	if (!window) {
		pr_err("sync call memremap window failed\n");
		err = -ENOMEM;
		goto free_obtain;
	}

	err = uvb_fill_window(wd, window, io_param_sync, para);
	if (err) {
		pr_err("sync call fill uvb window failed\n");
		goto unmap_window;
	}

	err = uvb_poll_window_call_sync(window, para->message_id);
	if (err) {
		pr_err("sync call by uvb failed\n");
		goto free_window;
	}

	err = uvb_get_output_data(window, io_param_sync, para->output, para->output_size);
	if (err)
		pr_err("sync call uvb get output data failed\n");

free_window:
	uvb_free_window(window);
	if (io_param_sync->input)
		memunmap(io_param_sync->input);
unmap_window:
	memunmap(window);
free_obtain:
	uvb_free_wd_obtain(wd_obtain);
	pr_info("finish cis sync call by uvb\n");

	return err;
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

	res = get_cis_group_info(call_id, receiver_id,
		&usage, &index, &exact_receiver_id, &forwarder_id);
	if (res) {
		pr_err("can't get group info, call id=%08x, receiver id=%08x\n",
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
		pr_err("method not supported, call id=%08x, receiver id=%08x, usage=%d\n",
			call_id, receiver_id, usage);
		return -EOPNOTSUPP;
	}

	if (is_sync)
		return cis_call_uvb_sync(index, &para);

	return cis_call_uvb(index, &para);
}

static bool check_msg_vaild(struct cis_message *msg)
{
	if (!msg)
		return false;

	if (msg->input && !msg->input_size)
		return false;

	if (!msg->input && msg->input_size)
		return false;

	if (msg->output && (!msg->p_output_size || !*msg->p_output_size))
		return false;

	if (!msg->output && msg->p_output_size && *msg->p_output_size)
		return false;

	return true;
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

	pr_debug("cis call: call id %08x, sender id %08x, receiver id %08x\n",
			call_id, sender_id, receiver_id);

	if (!sender_id || !receiver_id) {
		pr_err("senderid or receiverid can't be null\n");
		return -EINVAL;
	}

	if (!check_msg_vaild(msg)) {
		pr_err("check cis message invalid\n");
		return -EINVAL;
	}

	if (cis_call_for_me(receiver_id) || cis_call_for_local(receiver_id)) {
		func = search_local_cis_func(call_id, receiver_id);
		if (func) {
			ret = func(msg);
			if (ret) {
				pr_err("cis call execute registered cis func failed\n");
				return ret;
			}
			pr_info("cis call execute registered cis func success\n");
			return 0;
		}
		pr_err("can't found cis func for callid=%08x, receiver_id=%08x\n",
			call_id, receiver_id);
		return -EOPNOTSUPP;
	}

	return cis_call_remote(call_id, sender_id, receiver_id, msg, is_sync);
}
EXPORT_SYMBOL(cis_call_by_uvb);

/*
Register a Call ID Service
@call_id         - UBIOS Interface ID
@receiver_id     - UBIOS User ID who own this CIS
@func            - Callback function of Call ID
*/
int register_local_cis_func(u32 call_id, u32 receiver_id, msg_handler func)
{
	struct cis_func_node *p;
	unsigned long flags;

	if (UBIOS_GET_MESSAGE_FLAG(call_id) != UBIOS_CALL_ID_FLAG) {
		pr_err("register is not uvb call\n");
		return -EINVAL;
	}
	if (!func) {
		pr_err("register func is NULL\n");
		return -EINVAL;
	}

	/* check is this Call ID already has a funciton */
	if (search_local_cis_func_node(call_id, receiver_id)) {
		pr_err("cis register: call_id[%08x], receiver_id[%08x], already register func\n",
			call_id, receiver_id);
		return -EINVAL;
	}

	p = kcalloc(1, sizeof(struct cis_func_node), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->call_id = call_id;
	p->receiver_id = receiver_id;
	p->func = func;

	spin_lock_irqsave(&cis_register_lock, flags);
	list_add_tail_rcu(&p->link, &g_local_cis_list);
	spin_unlock_irqrestore(&cis_register_lock, flags);
	pr_info("register cis func success\n");

	return 0;
}
EXPORT_SYMBOL(register_local_cis_func);

/*
Unregister a Call ID Service
@call_id         - UBIOS Interface ID
@receiver_id     - UBIOS User ID who own this CIS
*/
int unregister_local_cis_func(u32 call_id, u32 receiver_id)
{
	struct cis_func_node *p;
	unsigned long flags;

	if (UBIOS_GET_MESSAGE_FLAG(call_id) != UBIOS_CALL_ID_FLAG) {
		pr_err("unregister is not uvb call\n");
		return -EINVAL;
	}

	p = search_local_cis_func_node(call_id, receiver_id);
	if (!p) {
		pr_err("cis unregister: call_id[%08x], receiver_id[%08x] not find func node.\n",
			call_id, receiver_id);
		return -EINVAL;
	}

	spin_lock_irqsave(&cis_register_lock, flags);
	list_del_rcu(&p->link);
	spin_unlock_irqrestore(&cis_register_lock, flags);
	synchronize_rcu();

	kfree(p);
	pr_info("unregister cis func success\n");

	return 0;
}
EXPORT_SYMBOL(unregister_local_cis_func);

