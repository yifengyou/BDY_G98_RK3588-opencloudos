// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Call ID Service (CIS) info processing module, handles CIS init,
 *              func register/lookup and group info retrieval.
 * Author: zhangrui
 * Create: 2025-04-18
 */

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include "cis_info_process.h"

LIST_HEAD(g_local_cis_list);
DEFINE_SPINLOCK(cis_register_lock);
struct cis_message *io_param_sync;

int init_cis_table(void)
{
	if (!g_cis_info) {
		pr_err(ERR_PRE "failed to get cis info from odf\n");
		return -EOPNOTSUPP;
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

void free_global_vars(void)
{
	kfree(io_param_sync);
	io_param_sync = NULL;
}

static bool is_call_id_supported(struct cis_group *group, u32 call_id)
{
	u32 i;

	for (i = 0; i < group->cis_count; i++) {
		pr_debug(LOG_PRE "cia call_id: %08x\n", group->call_id[i]);
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
		pr_err(ERR_PRE "can't get cis_info from odf\n");
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
		pr_info(LOG_PRE "refresh info, usage=%d, index=%d, forward_id=%08x\n",
			*usage, *index, *forwarder_id);
		return 0;
	}

	pr_err(ERR_PRE "call id: %08x not supported\n", call_id);

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

/*
Search Call ID Service owned by this component, return the function.
*/
msg_handler search_my_cis_func(u32 call_id)
{
	return search_local_cis_func(call_id, UBIOS_MY_USER_ID);
}

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

	pr_info(LOG_PRE "cis register: call_id[%08x], receiver_id[%08x]\n", call_id, receiver_id);
	if (UBIOS_GET_MESSAGE_FLAG(call_id) != UBIOS_CALL_ID_FLAG) {
		pr_err(ERR_PRE "register is not uvb call\n");
		return -EINVAL;
	}
	if (!func) {
		pr_err(ERR_PRE "register func is NULL\n");
		return -EINVAL;
	}

	/* check is this Call ID already has a funciton */
	if (search_local_cis_func_node(call_id, receiver_id)) {
		pr_err(ERR_PRE "cis register: call_id[%08x], receiver_id[%08x], already register func\n",
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
	pr_info(LOG_PRE "register cis func success\n");

	return 0;
}
EXPORT_SYMBOL(register_local_cis_func);

/*
Register a Call ID Service owned by this component
@call_id     - UBIOS Interface ID
@func   - Callback function of Call ID
*/
int register_my_cis_func(u32 call_id, msg_handler func)
{
	return register_local_cis_func(call_id, UBIOS_MY_USER_ID, func);
}
EXPORT_SYMBOL(register_my_cis_func);


/*
Unregister a Call ID Service
@call_id         - UBIOS Interface ID
@receiver_id     - UBIOS User ID who own this CIS
*/
int unregister_local_cis_func(u32 call_id, u32 receiver_id)
{
	struct cis_func_node *p;
	unsigned long flags;

	pr_info(LOG_PRE "cis unregister: call_id[%08x], receiver_id[%08x]\n", call_id, receiver_id);
	if (UBIOS_GET_MESSAGE_FLAG(call_id) != UBIOS_CALL_ID_FLAG) {
		pr_err(ERR_PRE "register is not uvb call\n");
		return -EINVAL;
	}

	p = search_local_cis_func_node(call_id, receiver_id);
	if (!p) {
		pr_err(ERR_PRE "cis unregister: call_id[%08x], receiver_id[%08x] not find func node.\n",
			call_id, receiver_id);
		return -EINVAL;
	}

	spin_lock_irqsave(&cis_register_lock, flags);
	list_del_rcu(&p->link);
	spin_unlock_irqrestore(&cis_register_lock, flags);
	synchronize_rcu();

	kfree(p);
	pr_info(LOG_PRE "unregister cis func success\n");

	return 0;
}
EXPORT_SYMBOL(unregister_local_cis_func);

/*
Unregister a Call ID Service owned by this component
@call_id     - UBIOS Interface ID
*/
int unregister_my_cis_func(u32 call_id)
{
	return unregister_local_cis_func(call_id, UBIOS_MY_USER_ID);
}
EXPORT_SYMBOL(unregister_my_cis_func);
