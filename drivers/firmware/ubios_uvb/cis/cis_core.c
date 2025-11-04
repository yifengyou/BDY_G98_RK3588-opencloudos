// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Call ID Service (CIS) core module, manages inter-process communication
 *              via call identifiers with local/remote handling and UVB integration.
 * Author: zhangrui
 * Create: 2025-04-18
 */
#define pr_fmt(fmt) "[UVB]: " fmt

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/hashtable.h>
#include "cis_info_process.h"
#include "uvb_info_process.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Call ID Service Framework");

static struct task_struct *uvb_poll_window_thread;
DECLARE_HASHTABLE(uvb_lock_table, MAX_UVB_LOCK_IN_BITS);

int create_uvb_poll_window_thread(void)
{
	uvb_poll_window_thread = kthread_run(uvb_poll_window, NULL, "uvb_poll_window_thread");
	if (IS_ERR(uvb_poll_window_thread)) {
		pr_err("Failed to create uvb polling thread\n");
		return PTR_ERR(uvb_poll_window_thread);
	}

	pr_info("create uvb poll window thread successfully\n");

	return 0;
}

void uvb_poll_window_thread_stop(void)
{
	if (uvb_poll_window_thread) {
		kthread_stop(uvb_poll_window_thread);
		uvb_poll_window_thread = NULL;
	}
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
	pr_info("uvb window lock init success.\n");

	return 0;
}

int init_uvb(void)
{
	int err = 0;

	if (!g_uvb_info) {
		pr_err("uvb is invalid, please try to use smc\n");
		return -EOPNOTSUPP;
	}

	err = uvb_window_lock_init();
	if (err) {
		pr_err("Init uvb window lock failed\n");
		return err;
	}

	err = create_uvb_poll_window_thread();
	if (err) {
		pr_err("create uvb poll thread did failed, err=%d\n", err);
		free_uvb_window_lock();
		return err;
	}

	return 0;
}

int init_global_vars(void)
{
	io_param_sync = kzalloc(sizeof(struct cis_message), GFP_KERNEL);
	if (!io_param_sync)
		return -ENOMEM;

	return 0;
}

int init_cis_table(void)
{
	if (!g_cis_info) {
		pr_err("failed to get cis info from odf\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

void free_global_vars(void)
{
	kfree(io_param_sync);
	io_param_sync = NULL;
}

void uninit_uvb(void)
{
	uvb_poll_window_thread_stop();
	msleep(UVB_POLL_TIMEOUT);
	free_uvb_window_lock();
}

static int __init cis_init(void)
{
	int err = 0;

	err = init_cis_table();
	if (err) {
		pr_err("cis info init failed, err=%d\n", err);
		return err;
	}

	err = init_global_vars();
	if (err) {
		pr_err("global vars malloc failed, err=%d\n", err);
		return err;
	}

	err = init_uvb();
	if (err) {
		pr_err("uvb init failed, err=%d\n", err);
		free_global_vars();
		return err;
	}

	pr_info("cis init success\n");

	return 0;
}

static void __exit cis_exit(void)
{
	uninit_uvb();
	free_global_vars();
	pr_info("cis exit success\n");
}

module_init(cis_init);
module_exit(cis_exit);

