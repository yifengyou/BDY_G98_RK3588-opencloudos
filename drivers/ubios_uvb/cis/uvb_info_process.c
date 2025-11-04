// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: UVB info processing module, handles init and window polling.
 * Author: zhangrui
 * Create: 2025-04-18
 */

#include <asm/barrier.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/hashtable.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/cred.h>
#include <linux/kernel.h>

#include "cis_info_process.h"
#include "io_param.h"
#include "uvb_info_process.h"

DECLARE_HASHTABLE(uvb_lock_table, MAX_UVB_LOCK_IN_BITS);

int cis_call_uvb_sync(u8 index, struct udfi_para *para)
{
	int err;
	struct uvb_window *window = NULL;
	struct uvb_window_description *wd = NULL;
	u64 *wd_obtain = NULL;

	if (!g_uvb_info) {
		pr_err(ERR_PRE "uvb unsupported\n");
		return -EOPNOTSUPP;
	}

	if (index >= g_uvb_info->uvb_count) {
		pr_err(ERR_PRE "cis call sync use uvb index exceed\n");
		return -EOVERFLOW;
	}

	pr_info(LOG_PRE "start to do sync cis call by uvb\n");
	wd = uvb_occupy_window(g_uvb_info->uvbs[index], para->sender_id, wd_obtain);
	if (!wd) {
		pr_err(ERR_PRE "obtain window failed\n");
		err = -EBUSY;
		goto free_resources;
	}

	window = (struct uvb_window *)memremap(wd->address, sizeof(struct uvb_window), MEMREMAP_WC);
	if (!window) {
		pr_err(ERR_PRE "convert window addr from pa to va failed\n");
		err = -ENOMEM;
		goto free_resources;
	}

	err = uvb_fill_window(wd, window, io_param_sync, para);
	if (err) {
		pr_err(ERR_PRE "fill uvb window failed\n");
		goto free_resources;
	}

	err = uvb_poll_window_call_sync(window, para->message_id);
	if (err) {
		pr_err(ERR_PRE "sync call by uvb failed\n");
		goto free_resources;
	}

	err = uvb_get_output_data(window, io_param_sync, para->output, para->output_size);
	if (err)
		pr_err(ERR_PRE "uvb get output data failed\n");

free_resources:
	if (wd->buffer && io_param_sync->input)
		memunmap(io_param_sync->input);

	memset(io_param_sync, 0, sizeof(struct cis_message));

	if (window)
		uvb_free_window(window);

	if (wd_obtain) {
		*wd_obtain = 0;
		memunmap(wd_obtain);
	}
	pr_info(LOG_PRE "finish cis call by uvb sync\n");

	return err;
}

int cis_call_uvb(u8 index, struct udfi_para *para)
{
	int err;
	struct uvb_window *window = NULL;
	struct uvb_window_description *wd = NULL;
	struct cis_message *io_param = NULL;
	u64 *wd_obtain = NULL;

	if (!g_uvb_info) {
		pr_err(ERR_PRE "uvb unsupported\n");
		return -EOPNOTSUPP;
	}

	if (index >= g_uvb_info->uvb_count) {
		pr_err(ERR_PRE "cis call use uvb index exceed\n");
		return -EOVERFLOW;
	}

	pr_info(LOG_PRE "start to do cis call by uvb\n");
	wd = uvb_occupy_window(g_uvb_info->uvbs[index], para->sender_id, wd_obtain);
	if (!wd) {
		pr_err(ERR_PRE "obtain window failed\n");
		err = -EBUSY;
		goto free_resources;
	}

	window = (struct uvb_window *)memremap(wd->address, sizeof(struct uvb_window), MEMREMAP_WC);
	if (!window) {
		pr_err(ERR_PRE "convert window addr from pa to va failed\n");
		err = -ENOMEM;
		goto free_resources;
	}

	io_param = kzalloc(sizeof(struct cis_message), GFP_KERNEL);
	if (!io_param) {
		err = -ENOMEM;
		goto free_resources;
	}
	err = uvb_fill_window(wd, window, io_param, para);
	if (err) {
		pr_err(ERR_PRE "fill uvb window failed\n");
		goto free_resources;
	}

	err = uvb_poll_window_call(window, para->message_id);
	if (err) {
		pr_err(ERR_PRE "call by uvb failed\n");
		goto free_resources;
	}

	err = uvb_get_output_data(window, io_param, para->output, para->output_size);
	if (err)
		pr_err(ERR_PRE "uvb get output data failed\n");

free_resources:
	if (io_param)
		ubios_uvb_free_io_param(io_param, (wd->buffer == 0));

	if (window) {
		uvb_free_window(window);
		memunmap(window);
	}
	if (wd_obtain) {
		*wd_obtain = 0;
		memunmap(wd_obtain);
	}
	pr_info(LOG_PRE "finish cis call by uvb\n");

	return err;
}

/**
Calculate checksum in 4bytes, if size not aligned with 4bytes, padding with 0.
*/
static u32 checksum32(const void *data, u32 size)
{
	u64 i;
	u64 sum = 0;
	u32 remainder = size % sizeof(u32);
	u32 *p = (u32 *)data;
	u32 restsize = size - remainder;

	if (!data)
		return (u32)-1;

	for (i = 0; i < restsize; i += sizeof(u32)) {
		sum += *p;
		p++;
	}

	switch (remainder) {
	case 1:
		sum += (*p) & 0x000000FF;
		break;
	case 2:
		sum += (*p) & 0x0000FFFF;
		break;
	case 3:
		sum += (*p) & 0x00FFFFFF;
		break;
	default:
		break;
	}

	return (u32)(sum);
}

static void free_uvb_window_lock(void)
{
	struct uvb_window_lock *entry;
	struct hlist_node *tmp;
	u32 bkt;

	if (hash_empty(uvb_lock_table))
		return;

	hash_for_each_safe(uvb_lock_table, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
}

static int uvb_window_lock_init(void)
{
	struct uvb *uvb;
	struct uvb_window_lock *lock_node;
	u16 i;
	u16 j;

	for (i = 0; i < g_uvb_info->uvb_count; i++) {
		uvb = g_uvb_info->uvbs[i];
		for (j = 0; j < uvb->window_count; j++) {
			lock_node = kzalloc(sizeof(struct uvb_window_lock), GFP_KERNEL);
			if (!lock_node) {
				free_uvb_window_lock();
				return -ENOMEM;
			}
			lock_node->lock.counter = 0;
			lock_node->window_address = uvb->wd[j].address;
			hash_add(uvb_lock_table, &lock_node->node, uvb->wd[j].address);
		}
	}
	pr_info(LOG_PRE "uvb window lock init success.\n");

	return 0;
}

static void uvb_return_status(struct uvb_window *window, int status)
{
	window->returned_status = (u32)status;
	window->message_id = ~window->message_id;
}

bool search_local_receiver_id(u32 receiver_id)
{
	bool found = false;
	struct cis_func_node *cis_node;

	rcu_read_lock();
	list_for_each_entry_rcu(cis_node, &g_local_cis_list, link) {
		if (cis_node->receiver_id == receiver_id) {
			found = true;
			break;
		}
	}
	rcu_read_unlock();

	return found;
}

void uninit_uvb(void)
{
	free_uvb_window_lock();
}

int init_uvb(void)
{
	int err = 0;

	if (!g_uvb_info) {
		pr_warn(LOG_PRE "uvb is invalid, please try to use smc\n");
		return -EOPNOTSUPP;
	}

	err = uvb_window_lock_init();
	if (err) {
		pr_err(ERR_PRE "Init uvb window lock failed\n");
		return err;
	}

	return err;
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
			pr_err(ERR_PRE "uvb window obtain map failed\n");
			return NULL;
		}
		lock = find_uvb_window_lock(wd->address);
		if (!lock) {
			pr_err(ERR_PRE "uvb window lock not found\n");
			goto free_resources;
		}

		if (atomic_cmpxchg(lock, 0, 1) == 0
			&& try_obtain_uvb_window(wd_obtain, sender_id)) {
			atomic_set(lock, 0);
			udelay(uvb->delay);
			if (*wd_obtain == sender_id) {
				now = ktime_get();
				time_interval = ktime_to_us(ktime_sub(now, start));
				pr_info(LOG_PRE "occupy uvb window successfully, elapsed time: %lldus\n",
					time_interval);
				return wd;
			}
		}

		now = ktime_get();
		time_interval = ktime_to_us(ktime_sub(now, start));
		if (round > 1 && time_interval > UVB_TIMEOUT_WINDOW_OBTAIN) {
			pr_err(ERR_PRE "obtain window timeout, tried %u * %u = %u times\n",
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

int uvb_free_window(struct uvb_window *window)
{
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
	if (input) {
		new_input = memremap(wd->buffer, wd->size, MEMREMAP_WC);
		if (!new_input) {
			pr_err(ERR_PRE "memremap for wd_buffer_virt_addr failed\n");
			return -ENOMEM;
		}
		memcpy(new_input, input, input_size);
		window->input_data_checksum = checksum32(input, input_size);
	}

	if (output)
		new_output = (void *)(new_input + input_size);

	if (output_size) {
		if (wd->size < *output_size + input_size)
			return -EOVERFLOW;
		window->output_data_size = *output_size;
	}

	io_params->input = new_input;
	io_params->input_size = input_size;
	io_params->output = new_output;
	io_params->p_output_size = &(window->output_data_size);

	window->input_data_address = new_input ? wd->buffer : 0;
	window->input_data_size = input_size;
	window->output_data_address = new_output ? wd->buffer + input_size : 0;

	return 0;
}

static int fill_uvb_window_without_buffer(struct uvb_window *window_address,
			struct cis_message *io_params,
			void *input, u32 input_size,
			void *output, u32 *output_size)
{
	int err = 0;
	struct uvb_window *window;
	void *input_kloc = NULL;
	void *output_kloc = NULL;

	window = window_address;
	if (input) {
		input_kloc = kzalloc(input_size, GFP_KERNEL);
		if (!input_kloc) {
			err = -ENOMEM;
			goto fail;
		}
		memcpy(input_kloc, input, input_size);
		window->input_data_checksum = checksum32(input, input_size);
	}
	if (output) {
		output_kloc = kzalloc(*output_size, GFP_KERNEL);
		if (!output_kloc) {
			err = -ENOMEM;
			goto fail;
		}
		memcpy(output_kloc, output, *output_size);
	}
	if (output_size)
		window->output_data_size = *output_size;

	io_params->input = input_kloc;
	io_params->input_size = input_size;
	io_params->output = output_kloc;
	io_params->p_output_size = &(window->output_data_size);

	window->input_data_address = input_kloc ? (u64)virt_to_phys(input_kloc) : 0;
	window->input_data_size = input_size;
	window->output_data_address = output_kloc ? virt_to_phys(output_kloc) : 0;
	return 0;

fail:
	kfree(input_kloc);
	kfree(output_kloc);

	return err;
}

int uvb_fill_window(struct uvb_window_description *wd, struct uvb_window *wd_addr,
			struct cis_message *io_params, struct udfi_para *para)
{
	int err;
	struct uvb_window *window;

	window = wd_addr;
	window->message_id = para->message_id;
	window->sender_id = para->sender_id;

	if (wd->buffer == 0) {
		err = fill_uvb_window_without_buffer(window, io_params, para->input,
			para->input_size, para->output, para->output_size);
		if (err) {
			pr_err(ERR_PRE "fill uvb window without buffer failed\n");
			goto fail;
		}
	} else {
		err = fill_uvb_window_with_buffer(wd, window, io_params, para->input,
			para->input_size, para->output, para->output_size);
		if (err) {
			pr_err(ERR_PRE "fill uvb window with buffer failed\n");
			goto fail;
		}
	}

	window->receiver_id = para->receiver_id;
	window->forwarder_id = para->forwarder_id;
	pr_info(LOG_PRE "uvb fill window success\n");

	return 0;
fail:
	return err;
}

int uvb_poll_window_call(struct uvb_window *window, u32 call_id)
{
	ktime_t start;
	ktime_t now;
	s64 time_interval;

	start = ktime_get();
	while (1) {
		if (window->message_id == ~call_id) {
			pr_info(LOG_PRE "window message id seted to 0x%08x\n", window->message_id);
			return (int)window->returned_status;
		}
		now = ktime_get();
		time_interval = ktime_to_ms(ktime_sub(now, start));
		if (time_interval > UVB_POLL_TIMEOUT)
			break;
	}

	pr_err(ERR_PRE "uvb poll window call timeout,wait=%lld ms\n", time_interval);

	return -ETIMEDOUT;
}

int uvb_poll_window_call_sync(struct uvb_window *window, u32 call_id)
{
	int i;

	pr_info(LOG_PRE "start uvb window polling\n");
	for (i = 0; i < UVB_POLL_TIMEOUT_TIMES; i++) {
		if (window->message_id == ~call_id) {
			pr_info(LOG_PRE "window message id seted to 0x%08x\n", window->message_id);
			return (int)window->returned_status;
		}
		udelay(UVB_POLL_TIME_INTERVAL);
	}

	pr_err(ERR_PRE "uvb poll window call sync timeout\n");

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
		pr_warn(LOG_PRE "returned data checksum error\n");
		return -EINVAL;
	}
	ubios_prepare_output_data(io_param, output, output_size);

	return 0;
}
