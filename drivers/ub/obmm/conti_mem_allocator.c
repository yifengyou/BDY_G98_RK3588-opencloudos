// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 * Description：OBMM Framework's implementations.
 */
#define pr_fmt(fmt) "OBMM: conti_mem:" fmt
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "conti_mem_allocator.h"

static atomic_t pool_thread_should_pause = ATOMIC_INIT(0);

static int conti_clear_memseg(struct conti_mem_allocator *a, struct memseg_node *node)
{
	if (a->ops->clear_memseg)
		return a->ops->clear_memseg(a, node);
	return -EOPNOTSUPP;
}

static void conti_pool_free_memseg(struct conti_mem_allocator *a, struct memseg_node *node)
{
	if (a->ops->pool_free_memseg) {
		pr_debug("free memseg: nid %d\n", a->nid);
		a->ops->pool_free_memseg(a, node);
	}
}

static struct memseg_node *conti_pool_alloc_memseg(struct conti_mem_allocator *a)
{
	if (a->ops->pool_alloc_memseg) {
		pr_debug("alloc memseg: nid %d\n", a->nid);
		return a->ops->pool_alloc_memseg(a);
	} else {
		return NULL;
	}
}

static bool conti_has_poisoned_memseg(struct conti_mem_allocator *a)
{
	/* this lockless read is safe and is intended */
	return !list_empty(&a->memseg_poisoned);
}

static bool conti_need_contract(struct conti_mem_allocator *a)
{
	if (a->ops->need_contract)
		return a->ops->need_contract(a);
	else
		return false;
}

static size_t conti_contract_size(struct conti_mem_allocator *a)
{
	if (a->ops->contract_size)
		return a->ops->contract_size(a);
	else
		return 0;
}

static bool conti_need_expand(struct conti_mem_allocator *a)
{
	if (a->ops->need_expand)
		return a->ops->need_expand(a);
	else
		return false;
}

static size_t conti_expand_size(struct conti_mem_allocator *a)
{
	if (a->ops->expand_size)
		return a->ops->expand_size(a);
	else
		return 0;
}

size_t conti_mem_allocator_expand(struct conti_mem_allocator *allocator, size_t size)
{
	unsigned long count, flags;
	struct memseg_node *node;
	size_t expand_size;

	if (size == 0 || size % allocator->granu) {
		pr_err("size %#zx is zero or not aligned with allocator->granu.\n", size);
		return 0;
	}

	count = size / allocator->granu;
	while (count > 0 && atomic_read(&pool_thread_should_pause) == 0) {
		node = conti_pool_alloc_memseg(allocator);
		if (!node)
			break;

		spin_lock_irqsave(&allocator->lock, flags);
		list_add_tail(&node->list, &allocator->memseg_uncleared);
		spin_unlock_irqrestore(&allocator->lock, flags);
		count--;
	}

	if (allocator->clear_work)
		wake_up_interruptible(&allocator->clear_wq);

	expand_size = size - count * allocator->granu;
	atomic64_add(expand_size, &allocator->pooled_mem_size);
	if (expand_size > 0)
		pr_debug("%s: expand expect size %#zx, actual size %#zx\n", current->comm, size,
			expand_size);

	return expand_size;
}

size_t conti_mem_allocator_contract(struct conti_mem_allocator *allocator, size_t size)
{
	struct list_head contract_list;
	struct memseg_node *node, *tmp;
	unsigned long count, flags;
	size_t contract_size;

	if (size == 0 || size % allocator->granu) {
		pr_err_ratelimited("size %#zx is zero or not aligned with allocator->granu.\n",
				   size);
		return 0;
	}

	count = size / allocator->granu;
	if (allocator->ops->pool_free_memseg == NULL)
		return 0;

	INIT_LIST_HEAD(&contract_list);
	spin_lock_irqsave(&allocator->lock, flags);
	list_for_each_entry_safe(node, tmp, &allocator->memseg_uncleared, list) {
		list_move_tail(&node->list, &contract_list);
		count--;
		if (count == 0)
			goto done;
	}

	list_for_each_entry_safe(node, tmp, &allocator->memseg_ready, list) {
		list_move_tail(&node->list, &contract_list);
		count--;
		if (count == 0)
			goto done;
	}

done:
	spin_unlock_irqrestore(&allocator->lock, flags);
	list_for_each_entry_safe(node, tmp, &contract_list, list) {
		list_del(&node->list);
		conti_pool_free_memseg(allocator, node);
	}
	contract_size = size - count * allocator->granu;
	atomic64_sub(contract_size, &allocator->pooled_mem_size);
	if (contract_size > 0)
		pr_debug("%s: nid: %d, contract expect size %#zx, actual size %#zx\n",
			 current->comm, allocator->nid, size, contract_size);

	return contract_size;
}

static size_t conti_mem_allocator_free_poisoned(struct conti_mem_allocator *allocator)
{
	LIST_HEAD(free_list);
	struct memseg_node *node, *tmp;
	size_t free_size = 0;
	unsigned long flags;

	if (allocator->ops->pool_free_memseg == NULL) {
		pr_debug("%s: no means to free poisoned memseg.\n", __func__);
		return 0;
	}

	spin_lock_irqsave(&allocator->lock, flags);
	list_splice_init(&allocator->memseg_poisoned, &free_list);
	spin_unlock_irqrestore(&allocator->lock, flags);

	list_for_each_entry_safe(node, tmp, &free_list, list) {
		list_del(&node->list);
		conti_pool_free_memseg(allocator, node);
		free_size += allocator->granu;
	}
	/* The memory freed by this function has already been subtracted from pooled memory size
	 * when isolated.
	 */
	if (free_size > 0)
		pr_debug("%s: nid: %d, %#zx poisoned memory freed\n", current->comm, allocator->nid,
			 free_size);

	return free_size;
}

void conti_free_memory(struct conti_mem_allocator *allocator, struct list_head *head)
{
	size_t freed_size = 0;
	struct memseg_node *node, *tmp;

	list_for_each_entry_safe(node, tmp, head, list) {
		freed_size += allocator->granu;
		list_del(&node->list);
		conti_pool_free_memseg(allocator, node);
		pr_debug("allocator: freed: %d: 0x%llx + 0x%lx\n", allocator->nid, node->addr,
			 node->size);
	}

	atomic64_sub(freed_size, &allocator->pooled_mem_size);
	atomic64_sub(freed_size, &allocator->used_mem_size);
	pr_debug("%s: freed_size %#zx on node %d.\n", current->comm, freed_size, allocator->nid);
}

static size_t conti_alloc_memory_slow(struct conti_mem_allocator *allocator, size_t size,
				      struct list_head *head, bool clear)
{
	struct memseg_node *node;
	size_t allocated = 0;
	int ret;

	while (size) {
		node = conti_pool_alloc_memseg(allocator);
		if (!node)
			break;

		if (clear) {
			ret = conti_clear_memseg(allocator, node);
			if (ret < 0) {
				conti_pool_free_memseg(allocator, node);
				break;
			}
		}
		allocated += allocator->granu;
		list_add_tail(&node->list, head);
		size -= allocator->granu;
	}

	atomic64_add(allocated, &allocator->pooled_mem_size);
	atomic64_add(allocated, &allocator->used_mem_size);
	pr_info("%s: slow allocated %#zx from node %d\n", current->comm, allocated, allocator->nid);
	return allocated;
}

size_t conti_alloc_memory(struct conti_mem_allocator *allocator, size_t size,
			  struct list_head *head, bool clear, bool allow_slow)
{
	struct list_head *first, *second, *entry, temp_list;
	struct memseg_node *node;
	size_t allocated = 0, available;
	unsigned long flags;

	atomic_inc(&pool_thread_should_pause);
	INIT_LIST_HEAD(&temp_list);
	if (clear) {
		first = &allocator->memseg_ready;
		second = &allocator->memseg_uncleared;
	} else {
		second = &allocator->memseg_ready;
		first = &allocator->memseg_uncleared;
	}

	spin_lock_irqsave(&allocator->lock, flags);
	available = conti_get_avail(allocator);
	if (!allow_slow && available < size) {
		pr_err("%s:fast alloc failed. nid: %d, request: 0x%lx, available: 0x%lx\n",
			__func__, allocator->nid, size, available);
		spin_unlock_irqrestore(&allocator->lock, flags);
		goto out_continue_pool;
	}
	list_for_each(entry, first) {
		if (allocated >= size)
			break;
		allocated += allocator->granu;
		pr_debug("alloc 1 node from %s list.\n", clear ? "cleared" : "uncleared");
	}
	list_cut_before(head, first, entry);

	list_for_each(entry, second) {
		if (allocated >= size)
			break;
		allocated += allocator->granu;
		pr_debug("alloc 1 node from %s list.\n", !clear ? "cleared" : "uncleared");
	}
	list_cut_before(&temp_list, second, entry);
	spin_unlock_irqrestore(&allocator->lock, flags);

	atomic64_add(allocated, &allocator->used_mem_size);

	/* now: head collects elements from the first list, temp_list holds elements form the
	 * second list and clearing node. When the caller requests for cleared data, all nodes in
	 * temp_list should be cleared synchronously.
	 */
	if (clear)
		list_for_each_entry(node, &temp_list, list)
			conti_clear_memseg(allocator, node);
	list_splice(&temp_list, head);

	if (allocated < size)
		allocated += conti_alloc_memory_slow(allocator, size - allocated, head, clear);

	list_for_each_entry(node, head, list) {
		pr_debug("allocator: allocated: %d: 0x%llx + 0x%lx\n", allocator->nid, node->addr,
			 node->size);
	}
	pr_info("%s: allocated %#zx from node %d\n", current->comm, allocated, allocator->nid);

out_continue_pool:
	atomic_dec(&pool_thread_should_pause);

	/* not aligned */
	WARN_ON(allocated > size);

	return allocated;
}

bool conti_mem_allocator_isolate_memseg(struct conti_mem_allocator *a, unsigned long addr)
{
	struct memseg_node *node;
	bool found = false;
	unsigned long flags;

	if (!a->initialized)
		return false;
	addr = ALIGN_DOWN(addr, a->granu);
	spin_lock_irqsave(&a->lock, flags);
	list_for_each_entry(node, &a->memseg_ready, list) {
		if (node->addr == addr) {
			pr_debug("isolate memseg from cleared pool.\n");
			list_move(&node->list, &a->memseg_poisoned);
			found = true;
			goto out;
		}
	}
	list_for_each_entry(node, &a->memseg_uncleared, list) {
		if (node->addr == addr) {
			pr_debug("isolate memseg from uncleared pool.\n");
			list_move(&node->list, &a->memseg_poisoned);
			found = true;
			goto out;
		}
	}
	if (a->memseg_clearing && a->memseg_clearing->addr == addr)
		pr_warn("memseg to isolate is being cleared; isolation failed.\n");
	else
		pr_debug("memseg to isolate not found in pooled allocator of nid=%d.\n", a->nid);

out:
	spin_unlock_irqrestore(&a->lock, flags);
	if (found)
		atomic64_sub(a->granu, &a->pooled_mem_size);
	return found;
}

static int conti_clear_thread(void *p)
{
	struct conti_mem_allocator *allocator = p;
	struct memseg_node *node;
	int ret;
	unsigned long flags;

	pr_debug("%s: nid=%d, start\n", __func__, allocator->nid);
	allocator->memseg_clearing = NULL;
	while (!kthread_should_stop()) {
		wait_event_interruptible(allocator->clear_wq,
					 !list_empty(&allocator->memseg_uncleared) ||
						 kthread_should_stop());

		if (kthread_should_stop())
			break;
		spin_lock_irqsave(&allocator->lock, flags);
		if (list_empty(&allocator->memseg_uncleared)) {
			spin_unlock_irqrestore(&allocator->lock, flags);
			continue;
		}

		node = list_first_entry(&allocator->memseg_uncleared, struct memseg_node, list);
		list_del(&node->list);
		allocator->memseg_clearing = node;

		pr_debug("clearing: %d: %pa + 0x%lx\n", allocator->nid, &node->addr, node->size);
		spin_unlock_irqrestore(&allocator->lock, flags);
		ret = conti_clear_memseg(allocator, node);
		pr_debug("%s: nid=%d, clear done node=%p, addr=%pa\n", __func__, allocator->nid,
			 node, &node->addr);

		spin_lock_irqsave(&allocator->lock, flags);
		allocator->memseg_clearing = NULL;
		if (ret)
			list_add(&node->list, &allocator->memseg_uncleared);
		else
			list_add(&node->list, &allocator->memseg_ready);
		spin_unlock_irqrestore(&allocator->lock, flags);
	}
	pr_debug("%s: nid=%d, exit\n", __func__, allocator->nid);

	return 0;
}

static int clear_thread_init(struct conti_mem_allocator *allocator)
{
	struct task_struct *work;

	work = kthread_create_on_node(conti_clear_thread, allocator, allocator->nid,
				      "conti_clear_%s", allocator->name);
	if (IS_ERR(work)) {
		pr_err("failed to init conti_clear task\n");
		return -ENODEV;
	}
	(void)wake_up_process(work);

	allocator->clear_work = work;

	return 0;
}

#define POOL_THREAD_SLEEP_JIFFIES msecs_to_jiffies(5000)
static int conti_pool_thread(void *p)
{
	struct conti_mem_allocator *allocator = p;
	size_t size, ret_size;

	pr_debug("%s: nid=%d, start\n", __func__, allocator->nid);
	while (!kthread_should_stop()) {
		wait_event_interruptible_timeout(allocator->pool_wq,
						 atomic_read(&pool_thread_should_pause) == 0 &&
							 (conti_has_poisoned_memseg(allocator) ||
							  conti_need_contract(allocator) ||
							  conti_need_expand(allocator) ||
							  kthread_should_stop()),
						 POOL_THREAD_SLEEP_JIFFIES);

		if (kthread_should_stop())
			break;

		if (conti_has_poisoned_memseg(allocator)) {
			ret_size = conti_mem_allocator_free_poisoned(allocator);
			pr_debug("%s: nid=%d, free poisoned done, ret=%#zx\n", __func__,
				allocator->nid, ret_size);
		}

		if (conti_need_contract(allocator)) {
			size = conti_contract_size(allocator);
			if (size > 0) {
				pr_debug("%s: nid=%d, size=%#lx start contract\n", __func__,
					 allocator->nid, size);
				ret_size = conti_mem_allocator_contract(allocator, size);
				if (ret_size)
					pr_debug("%s: nid=%d, contract done, ret=%#zx\n", __func__,
						allocator->nid, ret_size);
			}
		}

		if (conti_need_expand(allocator)) {
			size = conti_expand_size(allocator);
			if (size > 0) {
				pr_debug("%s: nid=%d, start expand\n", __func__, allocator->nid);
				ret_size = conti_mem_allocator_expand(allocator, size);
				if (ret_size)
					pr_debug("%s: nid=%d, expand done, ret=%#zx\n", __func__,
						 allocator->nid, ret_size);
			}
		}
	}
	pr_debug("%s: nid=%d, exit\n", __func__, allocator->nid);

	return 0;
}

static int pool_thread_init(struct conti_mem_allocator *allocator)
{
	struct task_struct *work;

	init_waitqueue_head(&allocator->pool_wq);
	work = kthread_create_on_node(conti_pool_thread, allocator, allocator->nid, "conti_pool_%s",
				      allocator->name);
	if (IS_ERR(work)) {
		pr_err("failed to init conti_pool task\n");
		return -ENODEV;
	}
	(void)wake_up_process(work);

	allocator->pool_work = work;

	return 0;
}

int conti_mem_allocator_init(struct conti_mem_allocator *allocator, int nid, size_t granu,
			     const struct conti_mempool_ops *ops, const char *fmt, ...)
{
	va_list ap;
	int ret;

	if (!allocator || !ops) {
		pr_err("%s: null pointer.\n", __func__);
		return -EINVAL;
	}
	if (!ops->need_expand || !ops->expand_size) {
		pr_err("expand ops is required.\n");
		return -EINVAL;
	}
	if (!IS_ALIGNED(granu, PAGE_SIZE) || granu == 0) {
		pr_err("invalid granu size %#lx.\n", granu);
		return -EINVAL;
	}

	va_start(ap, fmt);
	allocator->name = kvasprintf(GFP_KERNEL, fmt, ap);
	va_end(ap);
	if (!allocator->name)
		return -ENOMEM;

	allocator->nid = nid;
	allocator->granu = granu;
	atomic64_set(&allocator->pooled_mem_size, 0);
	atomic64_set(&allocator->used_mem_size, 0);
	spin_lock_init(&allocator->lock);
	INIT_LIST_HEAD(&allocator->memseg_ready);
	init_waitqueue_head(&allocator->clear_wq);
	INIT_LIST_HEAD(&allocator->memseg_uncleared);
	allocator->memseg_clearing = NULL;
	INIT_LIST_HEAD(&allocator->memseg_poisoned);

	allocator->ops = ops;

	if (ops->clear_memseg) {
		ret = clear_thread_init(allocator);
		if (ret) {
			kfree(allocator->name);
			allocator->name = NULL;
			return ret;
		}
	}

	ret = pool_thread_init(allocator);
	if (ret) {
		if (allocator->clear_work)
			kthread_stop(allocator->clear_work);
		kfree(allocator->name);
		allocator->name = NULL;
		return ret;
	}

	allocator->initialized = true;

	return 0;
}

void conti_mem_allocator_deinit(struct conti_mem_allocator *allocator)
{
	struct memseg_node *node, *tmp;
	struct list_head free_list;
	unsigned long flags;

	INIT_LIST_HEAD(&free_list);
	if (allocator->pool_work)
		kthread_stop(allocator->pool_work);

	if (allocator->clear_work)
		kthread_stop(allocator->clear_work);

	kfree(allocator->name);
	if (!allocator->ops->pool_free_memseg) {
		pr_err("pool_free_memseg is not defined.\n");
		return;
	}

	/* Release all memory nodes chained in memseg_uncleared, memseg_ready
	 * and memseg_poisoned.
	 * NOTE: No memory node will be held in allocator->memseg_clearing after
	 * the clear worker stops working.
	 */
	spin_lock_irqsave(&allocator->lock, flags);
	list_splice(&allocator->memseg_uncleared, &free_list);
	list_splice(&allocator->memseg_ready, &free_list);
	list_splice(&allocator->memseg_poisoned, &free_list);
	spin_unlock_irqrestore(&allocator->lock, flags);

	list_for_each_entry_safe(node, tmp, &free_list, list) {
		list_del(&node->list);
		conti_pool_free_memseg(allocator, node);
	}
	memset(allocator, 0, sizeof(*allocator));
}
