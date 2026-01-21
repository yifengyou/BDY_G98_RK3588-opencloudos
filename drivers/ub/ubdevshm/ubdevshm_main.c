// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 * Description：ubdevshm core function
 */
#define pr_fmt(fmt) "UBDEVSHM: " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/mutex.h>
#include <linux/idr.h>
#include <linux/kthread.h>
#include <ub/ubdevshm/ubdevshm.h>
#include "ubdevshm_attr.h"
#include "ubdevshm_main.h"

#define UBDEVSHM_IDR_MIN_ID	0
#define UBDEVSHM_IDR_MAX_ID	INT_MAX

static bool ubdevshm_init_state;

/* To protect the global resource *_list and *_idr */
struct rw_semaphore ubdevshm_rw_semlock;

struct list_head provider_list = LIST_HEAD_INIT(provider_list);
struct list_head container_list = LIST_HEAD_INIT(container_list);

struct idr shm_container_idr;
struct idr mem_provider_idr;
static struct idr access_ctx_idr;

bool ubdevshm_is_inited(void)
{
	return ubdevshm_init_state;
}
EXPORT_SYMBOL_GPL(ubdevshm_is_inited);

static bool check_provider_registered(struct ubdevshm_mem_ops *ops)
{
	struct mem_provider *pos = NULL;
	bool found = false;

	list_for_each_entry(pos, &provider_list, node) {
		if (pos->ops == ops || pos->ops->acquire == ops->acquire ||
		    pos->ops->release == ops->release) {
			found = true;
			break;
		}
	}

	return found;
}

int ubdevshm_register_ops(struct ubdevshm_mem_ops *ops, unsigned long *handle)
{
	struct mem_provider *provider;
	int ret;

	if (!ops || !ops->acquire || !ops->release || !handle) {
		pr_err("invalid param\n");
		return -EINVAL;
	}

	down_write(&ubdevshm_rw_semlock);
	if (check_provider_registered(ops)) {
		pr_err("ops already registered\n");
		up_write(&ubdevshm_rw_semlock);
		return -EEXIST;
	}

	provider = kzalloc(sizeof(*provider), GFP_KERNEL);
	if (!provider) {
		up_write(&ubdevshm_rw_semlock);
		return -ENOMEM;
	}

	refcount_set(&provider->refcnt, 1);
	provider->ops = ops;
	INIT_LIST_HEAD(&provider->node);
	ret = idr_alloc_cyclic(&mem_provider_idr, provider, UBDEVSHM_IDR_MIN_ID,
			       UBDEVSHM_IDR_MAX_ID, GFP_ATOMIC);
	if (ret < 0) {
		pr_err("shm provider id_alloc err=%d\n", ret);
		kfree(provider);
	} else {
		provider->handle_id = ret;
		list_add_tail(&provider->node, &provider_list);
		*handle = (unsigned long)ret;
		ret = 0;
	}
	up_write(&ubdevshm_rw_semlock);

	return ret;
}
EXPORT_SYMBOL_GPL(ubdevshm_register_ops);

int ubdevshm_unregister_ops(unsigned long *handle)
{
	struct mem_provider *provider;
	int handle_id;
	int ret;

	if (!handle) {
		pr_err("handle is NULL\n");
		return -EINVAL;
	}
	handle_id = (int)*handle;

	down_write(&ubdevshm_rw_semlock);
	provider = idr_find(&mem_provider_idr, handle_id);
	if (!provider) {
		pr_err("invalid handle[%d] without matching provider\n", handle_id);
		ret = -EINVAL;
		goto out;
	}

	if (refcount_dec_if_one(&provider->refcnt)) {
		(void)idr_remove(&mem_provider_idr, (unsigned long)handle_id);
		list_del(&provider->node);
		ret = 0;
	} else {
		pr_err("provider is still used by others\n");
		ret = -EBUSY;
		goto out;
	}

	up_write(&ubdevshm_rw_semlock);
	kfree(provider);
	return ret;

out:
	up_write(&ubdevshm_rw_semlock);
	return ret;
}
EXPORT_SYMBOL_GPL(ubdevshm_unregister_ops);

static int __init ubdevshm_init(void)
{
	int ret;

	idr_init(&shm_container_idr);
	idr_init(&mem_provider_idr);
	idr_init(&access_ctx_idr);

	ret = ubdevshm_attr_file_init();
	if (ret)
		return ret;

	init_rwsem(&ubdevshm_rw_semlock);
	ubdevshm_init_state = true;
	return ret;
}

static void __exit ubdevshm_exit(void)
{
	ubdevshm_init_state = false;

	ubdevshm_attr_file_uninit();
	idr_destroy(&access_ctx_idr);
	idr_destroy(&mem_provider_idr);
	idr_destroy(&shm_container_idr);
}

module_init(ubdevshm_init);
module_exit(ubdevshm_exit);

MODULE_DESCRIPTION("Hisilicon ubdevshm");
MODULE_LICENSE("GPL");
