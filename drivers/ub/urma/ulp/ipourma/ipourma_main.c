// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ip over urma kernel module main entry
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include "ipourma_netdev.h"
#include "ipourma_res.h"
#include "ipourma_ub.h"
#include "ipourma_sysfs.h"
#include "ub/urma/ubcore_api.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_main.h"

static int ipourma_ubcore_add_device(struct ubcore_device *ubc_dev);
static void ipourma_ubcore_remove_device(struct ubcore_device *ubc_dev, void *client_ctx);
static void ipourma_unregister_netdev(struct ipourma_dev_priv *priv);
static void ipourma_do_eid_change_handler(struct ubcore_event *event,
					struct ubcore_event_handler *handler);

struct ubcore_client g_ipourma_ubcore_client = {
	.list_node = LIST_HEAD_INIT(g_ipourma_ubcore_client.list_node),
	.client_name = "ipourma",
	.add = ipourma_ubcore_add_device,
	.remove = ipourma_ubcore_remove_device,
};

// alloc netdev for the first legal eid on ubcore_device
// set client ctx data in ubcore
STATIC int ipourma_ubcore_add_device(struct ubcore_device *ubc_dev)
{
	struct net_device *ipou_ndev = NULL;
	struct ipourma_dev_priv *priv = NULL;

	/* need to skip unsupported device */
	if (strstr(ubc_dev->dev_name, "udma") == NULL)
		return -EOPNOTSUPP;

	ipou_ndev = ipourma_alloc_netdev(ubc_dev);
	if (IS_ERR_OR_NULL(ipou_ndev)) {
		pr_err("alloc netdev failed.\n");
		return -EOPNOTSUPP;
	}
	priv = netdev_priv(ipou_ndev);
	ubcore_set_client_ctx_data(ubc_dev, &g_ipourma_ubcore_client, priv);

	queue_work(priv->register_wq, &priv->register_netdev);

	return 0;
}

STATIC void ipourma_ubcore_remove_device(struct ubcore_device *ubc_dev, void *client_ctx)
{
	struct ipourma_dev_priv *priv = client_ctx;
	struct net_device *dev;

	if (IS_ERR_OR_NULL(priv))
		return;

	dev = priv->dev;
	if (IS_ERR_OR_NULL(dev))
		pr_err("ubcore_remove_device:priv->dev is NULL.\n");
	else
		ipourma_unregister_netdev(priv);
}

/*
 * Definition of a series of functions that support debugfs.
 */
struct ipourma_address_iter *ipourma_address_iter_init(struct ipourma_dev_priv *priv)
{
	struct ipourma_address_iter *iter;

	if (IS_ERR_OR_NULL(priv) || IS_ERR_OR_NULL(priv->urma_dev)) {
		pr_err("address_iter_init: priv or priv->urma_dev is NULL\n");
		return NULL;
	}
	iter = kzalloc(sizeof(*iter), GFP_KERNEL);
	if (iter) {
		iter->priv = priv;
		iter->index = 0;
	}
	return iter;
}

int ipourma_address_iter_next(struct ipourma_address_iter *iter)
{
	struct ubcore_eid_table *eid_table;
	uint32_t eid_cnt;

	if (!iter || !iter->priv || !iter->priv->urma_dev) {
		pr_err("address_iter_next: debugfs iter next NULL\n");
		return 1;
	}
	eid_table = &iter->priv->urma_dev->eid_table;
	spin_lock(&eid_table->lock);
	eid_cnt = eid_table->eid_cnt;
	spin_unlock(&eid_table->lock);
	if (iter->index >= eid_cnt - 1)
		return 1; // return 1 means stop

	iter->index++;
	return 0;
}

void ipourma_address_iter_read(struct ipourma_address_iter *iter, union ubcore_eid *eid)
{
	struct ubcore_eid_table *eid_table;
	struct ubcore_eid_entry *entry;
	uint32_t eid_cnt;

	if (!iter || !iter->priv || !iter->priv->urma_dev || !eid) {
		if (eid != NULL)
			memset(eid, 0, UBCORE_EID_SIZE);
		pr_err("address_iter_read: ptr NULL\n");
		return;
	}
	eid_table = &iter->priv->urma_dev->eid_table;
	spin_lock(&eid_table->lock);
	eid_cnt = eid_table->eid_cnt;

	if (iter->index < 0 || iter->index >= eid_cnt || !eid_table->eid_entries) {
		spin_unlock(&eid_table->lock);
		pr_err("address_iter_read: invalid index %d (address_cnt=%d)\n",
			iter->index, eid_cnt);
		return;
	}
	entry = &eid_table->eid_entries[iter->index];
	if (entry->valid)
		memcpy(eid->raw, entry->eid.raw, sizeof(entry->eid.raw));
	else
		memset(eid, 0, UBCORE_EID_SIZE);
	spin_unlock(&eid_table->lock);
}

STATIC int ipourma_netdev_event(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct ipourma_dev_priv *priv = NULL;

	if (!strstr(dev->name, "ipourma"))
		return NOTIFY_DONE;
	priv = (struct ipourma_dev_priv *)netdev_priv(dev);

	switch (event) {
	case NETDEV_REGISTER:
		/* create debugfs files here */
		break;
	case NETDEV_UNREGISTER:
		/* create debugfs files here */
		break;
	}
	return NOTIFY_DONE;
}

STATIC struct notifier_block ipourma_netdev_nb = {
	.notifier_call = ipourma_netdev_event,
};

STATIC int ipourma_priv_eid_init(struct net_device *dev)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct ubcore_device *urma_dev = priv->urma_dev;
	uint32_t eid_cnt;
	uint32_t i, j = 0;

	priv->eid_info = kcalloc(IPOURMA_MAX_EID_CNT,
		sizeof(struct ubcore_eid_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->eid_info)) {
		priv->eid_count = 0;
		pr_err("eid_info create failed.\n");
		return -1;
	}
	priv->tx_ring_is_full = kcalloc(IPOURMA_MAX_EID_CNT, sizeof(bool), GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->tx_ring_is_full)) {
		kfree(priv->eid_info);
		priv->eid_count = 0;
		pr_err("tx_ring_is_full create failed.\n");
		return -1;
	}

	priv->eid_change_handler.event_callback = ipourma_do_eid_change_handler;
	ubcore_register_event_handler(priv->urma_dev, &priv->eid_change_handler);

	spin_lock(&urma_dev->eid_table.lock);
	if (IS_ERR_OR_NULL(urma_dev->eid_table.eid_entries))
		goto eid_table_unlock;
	eid_cnt = urma_dev->eid_table.eid_cnt;
	for (i = 0; i < eid_cnt; i++) {
		if (eid_is_empty(&urma_dev->eid_table.eid_entries[i].eid))
			continue;
		priv->eid_count++;
	}

	for (i = 0, j = 0; i < eid_cnt && j < priv->eid_count; i++) {
		if (eid_is_empty(&urma_dev->eid_table.eid_entries[i].eid)
			|| urma_dev->eid_table.eid_entries[i].valid == false)
			continue;
		pr_debug("    eid:"EID_FMT"\n",
			EID_ARGS(urma_dev->eid_table.eid_entries[i].eid));

		priv->eid_info[i].eid = urma_dev->eid_table.eid_entries[i].eid;
		priv->eid_info[i].eid_index = i;
		priv->eid_info_exist[j++] = priv->eid_info[i];
	}
	if (j != priv->eid_count)
		pr_err("eid count error\n");
	pr_debug("eid_count:%d\n", priv->eid_count);

eid_table_unlock:
	spin_unlock(&urma_dev->eid_table.lock);
	return IPOURMA_OK;
}

STATIC void ipourma_cleanup_res(struct net_device *dev)
{
	if (IS_ERR_OR_NULL(dev))
		return;
	ipourma_urma_dev_uninit(dev);
}

STATIC void ipourma_proc_eid_exist(struct ipourma_dev_priv *priv)
{
	for (uint32_t i = 0; i < priv->eid_count; i++)
		ipourma_create_new_eid(priv, priv->eid_info_exist[i].eid_index);
}

/* register netdev */
void ipourma_register_netdev(struct work_struct *work)
{
	struct ipourma_dev_priv *priv;
	int ret = 0;

	priv = container_of(work, struct ipourma_dev_priv, register_netdev);
	ret = register_netdev(priv->dev);
	if (ret != 0) {
		pr_err("register %s failed, ret = %d\n", priv->dev->name, ret);
		goto REG_ERR;
	}
	if (IS_ERR_OR_NULL(priv->net_config_wq)) {
		pr_err("%s create work queue failed, ret = %d\n", priv->dev->name, ret);
		goto ALLOC_RESOURCE_ERR;
	}
	ret = ipourma_priv_eid_init(priv->dev);
	if (ret != IPOURMA_OK) {
		pr_err("%s init eid resources failed, ret = %d\n", priv->dev->name, ret);
		goto ALLOC_RESOURCE_ERR;
	}
	ipourma_proc_eid_exist(priv);
	queue_work(priv->net_config_wq, &(priv->set_dev_up));

	ipourma_register_sysfs(priv);
	return;

ALLOC_RESOURCE_ERR:
	unregister_netdev(priv->dev);
	ipourma_cleanup_res(priv->dev);
REG_ERR:
	return;
}

STATIC inline void ipourma_flush_register_wq(struct ipourma_dev_priv *priv)
{
	if (!IS_ERR_OR_NULL(priv->register_wq)) {
		flush_workqueue(priv->register_wq);
		destroy_workqueue(priv->register_wq);
	}
}

STATIC void ipourma_unregister_netdev(struct ipourma_dev_priv *priv)
{
	ubcore_unregister_event_handler(priv->urma_dev, &priv->eid_change_handler);
	ipourma_flush_register_wq(priv);
	ipourma_unregister_sysfs(priv);
	unregister_netdev(priv->dev);
	ipourma_reset_rings(priv);
	ipourma_cleanup_res(priv->dev);
	ipourma_unalloc_netdev(priv->dev);
}

STATIC void ipourma_do_eid_change_handler(struct ubcore_event *event,
					struct ubcore_event_handler *handler)
{
	if (event->event_type != UBCORE_EVENT_EID_CHANGE)
		return;

	struct ubcore_device *ub_dev = event->ub_dev;
	uint32_t eid_idx = event->element.eid_idx;
	struct ipourma_dev_priv *priv;
	union ubcore_eid eid;
	uint32_t eid_idx_tmp;
	uint32_t eid_cnt = 0;

	spin_lock(&ub_dev->eid_table.lock);
	eid_cnt = ub_dev->eid_table.eid_cnt;
	if (eid_idx >= eid_cnt ||
		eid_idx >= IPOURMA_MAX_EID_CNT ||
		IS_ERR_OR_NULL(ub_dev->eid_table.eid_entries))
		goto unlock_table_out;
	eid_idx_tmp = ub_dev->eid_table.eid_entries[eid_idx].eid_index;
	if (eid_idx_tmp != event->element.eid_idx ||
		ub_dev->eid_table.eid_entries[eid_idx].valid == false)
		goto unlock_table_out;
	eid = ub_dev->eid_table.eid_entries[eid_idx].eid;
	spin_unlock(&ub_dev->eid_table.lock);

	priv = ubcore_get_client_ctx_data(ub_dev, &g_ipourma_ubcore_client);
	if (!IS_ERR_OR_NULL(priv)) {
		if (!eid_is_empty(&priv->eid_info[eid_idx].eid)) {
			netdev_dbg(priv->dev, "get an old eid. eid index:%d, eid:"EID_FMT"\n",
			eid_idx, EID_ARGS(priv->eid_info[eid_idx].eid));
			return;
		}
		priv->eid_info[eid_idx].eid = eid;
		priv->eid_info[eid_idx].eid_index = eid_idx;
		netdev_dbg(priv->dev, " get a new eid. eid index:%d, eid:"EID_FMT"\n",
			eid_idx, EID_ARGS(priv->eid_info[eid_idx].eid));
		atomic_add(1, &(priv->need_set_ip_route));
		ipourma_create_new_eid(priv, eid_idx);
		priv->eid_count++;
	}
	return;
unlock_table_out:
	spin_unlock(&ub_dev->eid_table.lock);
}

STATIC int __init ipourma_init(void)
{
	int ret = 0;

	ret = ubcore_register_client(&g_ipourma_ubcore_client);
	if (ret != 0) {
		pr_err("Register ubcore client failed.\n");
		return ret;
	}
	pr_info("Register ubcore client success.\n");

	register_netdevice_notifier(&ipourma_netdev_nb);

	return 0;
}


STATIC void __exit ipourma_exit(void)
{
	ubcore_unregister_client(&g_ipourma_ubcore_client);
	unregister_netdevice_notifier(&ipourma_netdev_nb);
}
#if !defined(CONFIG_IPOURMA_TEST) && !defined(CONFIG_IPOURMA_KFUZZ)

module_init(ipourma_init);
module_exit(ipourma_exit);

MODULE_DESCRIPTION("ip over urma kernel module");
MODULE_AUTHOR("huawei");
MODULE_LICENSE("GPL");
#endif
