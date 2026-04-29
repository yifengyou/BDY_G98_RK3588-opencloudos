#include <linux/device.h>
#include <linux/tcp.h>
#include <linux/if_vlan.h>
#include <net/geneve.h>
#include <net/dsfield.h>
#include "../en_aux.h"

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN

struct netdev_queue_attribute {
	struct attribute attr;
	ssize_t (*show)(struct netdev_queue *queue, char *buf);
	ssize_t (*store)(struct netdev_queue *queue, const char *buf,
			 size_t count);
};

enum {
	ATTR_DST_IP,
	ATTR_DST_PORT,
};

static ssize_t zxdh_flow_param_show(struct netdev_queue *queue, char *buf,
				    int32_t type)
{
	struct net_device *netdev = queue->dev;
	struct zxdh_en_priv *en_priv = netdev_priv(netdev);
	struct zxdh_en_device *en_dev = &en_priv->edev;

	unsigned int queue_index = queue - netdev->_tx;
	struct send_queue *sq = &en_dev->sq[queue_index];
	int32_t count;

	LOG_INFO("enter\n");
	switch (type) {
	case ATTR_DST_IP:
		count = sprintf(buf, "0x%8x\n", ntohl(sq->flow_map.dst_ip));
		break;
	case ATTR_DST_PORT:
		count = sprintf(buf, "%d\n", ntohs(sq->flow_map.dst_port));
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static ssize_t zxdh_flow_param_store(struct netdev_queue *queue,
				     const char *buf, size_t count,
				     int32_t type)
{
	struct net_device *netdev = queue->dev;
	struct zxdh_en_priv *en_priv = netdev_priv(netdev);
	struct zxdh_en_device *en_dev = &en_priv->edev;

	unsigned int queue_index = queue - netdev->_tx;
	struct send_queue *sq = &en_dev->sq[queue_index];
	int32_t rtn = 0;
	uint32_t key;

	LOG_INFO("enter\n");
	switch (type) {
	case ATTR_DST_IP:
		rtn = kstrtou32(buf, 16, &sq->flow_map.dst_ip);
		if (rtn < 0)
			return rtn;
		sq->flow_map.dst_ip = htonl(sq->flow_map.dst_ip);
		break;
	case ATTR_DST_PORT:
		rtn = kstrtou16(buf, 0, &sq->flow_map.dst_port);
		if (rtn < 0)
			return rtn;
		sq->flow_map.dst_port = htons(sq->flow_map.dst_port);
		break;
	default:
		return -EINVAL;
	}

	/* Each queue can only apear once in the hash table */
	hash_del_rcu(&sq->flow_map.hlist);

	sq->flow_map.queue_index = queue_index;
	if (sq->flow_map.dst_ip != 0 || sq->flow_map.dst_port != 0) {
		/* hash and add to hash table */
		key = sq->flow_map.dst_ip ^ sq->flow_map.dst_port;
		hash_add_rcu(en_dev->flow_map_hash, &sq->flow_map.hlist, key);
	}

	return count;
}

static ssize_t zxdh_dst_port_store(struct netdev_queue *queue, const char *buf,
				   size_t count)
{
	return zxdh_flow_param_store(queue, buf, count, ATTR_DST_PORT);
}

static ssize_t zxdh_dst_port_show(struct netdev_queue *queue, char *buf)
{
	return zxdh_flow_param_show(queue, buf, ATTR_DST_PORT);
}

static ssize_t zxdh_dst_ip_store(struct netdev_queue *queue, const char *buf,
				 size_t count)
{
	return zxdh_flow_param_store(queue, buf, count, ATTR_DST_IP);
}

static ssize_t zxdh_dst_ip_show(struct netdev_queue *queue, char *buf)
{
	return zxdh_flow_param_show(queue, buf, ATTR_DST_IP);
}

static struct netdev_queue_attribute dst_port = {
	.attr = { .name = "dst_port", .mode = (S_IWUSR | S_IRUGO) },
	.show = zxdh_dst_port_show,
	.store = zxdh_dst_port_store,
};

static struct netdev_queue_attribute dst_ip = {
	.attr = { .name = "dst_ip", .mode = (S_IWUSR | S_IRUGO) },
	.show = zxdh_dst_ip_show,
	.store = zxdh_dst_ip_store,
};

static struct attribute *zxdh_txmap_attrs[] = {
	&dst_port.attr,
	&dst_ip.attr,
	NULL,
};

static struct attribute_group zxdh_txmap_attr = {
	.name = "flow_map",
	.attrs = zxdh_txmap_attrs,
};

int32_t zxdh_flow_map_update_sysfs(struct net_device *netdev)
{
	int32_t rtn;
	int32_t i;
	struct zxdh_en_priv *en_priv = netdev_priv(netdev);
	struct zxdh_en_device *en_dev = &en_priv->edev;
	struct netdev_queue *txq;

	if (en_dev->old_queue_pairs > en_dev->curr_queue_pairs) {
		LOG_INFO("old_queue_pairs(%d) > curr_queue_pairs(%d)\n",
			 en_dev->old_queue_pairs, en_dev->curr_queue_pairs);
	} else {
		for (i = en_dev->old_queue_pairs; i < en_dev->curr_queue_pairs;
		     i++) {
			txq = netdev_get_tx_queue(netdev, i);
			rtn = sysfs_create_group(&txq->kobj, &zxdh_txmap_attr);
			if (rtn) {
				LOG_ERR("Failed to create flow_map for tx-%d (err=%d)\n",
					i, rtn);
				goto rollback;
			}
		}
	}

	return 0;

rollback:
	//配置回滚,删除已经创建的sysfs属性
	for (i--; i >= en_dev->old_queue_pairs; i--) {
		txq = netdev_get_tx_queue(netdev, i);
		sysfs_remove_group(&txq->kobj, &zxdh_txmap_attr);
	}
	return rtn;
}

int32_t zxdh_flow_map_init_sysfs(struct net_device *netdev)
{
	struct zxdh_en_priv *en_priv = netdev_priv(netdev);
	struct zxdh_en_device *en_dev = &en_priv->edev;
	struct netdev_queue *txq;
	int32_t rtn;
	int32_t qid;

	LOG_DEBUG("enter\n");
	for (qid = 0; qid < en_dev->curr_queue_pairs; qid++) {
		// qid = i + params.num_channels * params.num_tc;
		txq = netdev_get_tx_queue(netdev, qid);
		rtn = sysfs_create_group(&txq->kobj, &zxdh_txmap_attr);
		if (rtn)
			goto rtn;
	}
	return 0;

rtn:
	for (--qid; qid >= 0; qid--) {
		// qid = i + params.num_channels * params.num_tc;
		txq = netdev_get_tx_queue(netdev, qid);
		sysfs_remove_group(&txq->kobj, &zxdh_txmap_attr);
	}
	return rtn;
}

void zxdh_flow_map_remove_sysfs(struct zxdh_en_device *en_dev)
{
	struct netdev_queue *txq;
	struct kernfs_node *kfnode;
	int32_t qid;

	LOG_INFO("Entering zxdh_flow_map_remove_sysfs\n");

	for (qid = 0; qid < en_dev->curr_queue_pairs; qid++) {
		// qid = i + en_dev->channels.params.num_channels *
		//             en_dev->channels.params.num_tc;
		txq = netdev_get_tx_queue(en_dev->netdev, qid);
		if (!txq) {
			LOG_ERR("Failed to get TX queue for qid %d\n", qid);
			continue;
		}
		if (!kobject_get(&txq->kobj)) {
			LOG_WARN("Failed to get kobject for qid %d\n", qid);
			continue;
		}

		kfnode = sysfs_get_dirent(txq->kobj.sd, zxdh_txmap_attr.name);
		if (kfnode) {
			sysfs_remove_group(&txq->kobj, &zxdh_txmap_attr);
			kernfs_put(kfnode); // 释放引用计数，防止内存泄露
		} else {
			LOG_INFO("Directory entry not found for qid %d\n", qid);
		}

		kobject_put(&txq->kobj);
	}
}

static int32_t zxdh_select_queue_assigned(struct zxdh_en_device *en_dev,
					  struct sk_buff *skb,
					  uint32_t *queue_index)
{
	struct zxdh_sq_flow_map *flow_map;
	// int32_t sk_ix = sk_tx_queue_get(skb->sk);
	uint32_t key_all, key_dip, key_dport;
	uint16_t dport;
	uint32_t dip;
	__be16 protocol;
	uint8_t l4_proto = 0;

	// if (sk_ix >= en_dev->channels.params.num_channels)
	//     return sk_ix;
	if (hash_empty(en_dev->flow_map_hash)) {
		goto fallback;
	}

	protocol = vlan_get_protocol(skb);
	l4_proto = ip_hdr(skb)->protocol;

	if (protocol == htons(ETH_P_IP)) {
		dip = ip_hdr(skb)->daddr;

		if (l4_proto == IPPROTO_UDP || l4_proto == IPPROTO_TCP)
			dport = udp_hdr(skb)->dest;
		else
			goto fallback;
	} else {
		goto fallback;
	}

	// LOG_INFO("dst_ip = 0x%8x, dst_port = %d", ntohl(dip), htons(dport));
	key_all = dip ^ dport;
	hash_for_each_possible_rcu(en_dev->flow_map_hash, flow_map, hlist,
				   key_all)
		if (flow_map->dst_ip == dip && flow_map->dst_port == dport) {
			*queue_index = flow_map->queue_index;
			return 1;
		}

	key_dip = dip;
	hash_for_each_possible_rcu(en_dev->flow_map_hash, flow_map, hlist,
				   key_dip)
		if (flow_map->dst_ip == dip) {
			*queue_index = flow_map->queue_index;
			return 1;
		}

	key_dport = dport;
	hash_for_each_possible_rcu(en_dev->flow_map_hash, flow_map, hlist,
				   key_dport)
		if (flow_map->dst_port == dport) {
			*queue_index = flow_map->queue_index;
			return 1;
		}

fallback:
	return 0;
}

#ifdef HAVE_NDO_SELECT_QUEUE_FALLBACK_REMOVED
uint16_t zxdh_en_select_queue(struct net_device *netdev, struct sk_buff *skb,
			      struct net_device *sb_dev)
#else
uint16_t zxdh_en_select_queue(struct net_device *netdev, struct sk_buff *skb,
			      struct net_device *sb_dev,
			      select_queue_fallback_t fallback)
#endif
{
	struct zxdh_en_priv *en_priv = netdev_priv(netdev);
	struct zxdh_en_device *en_dev = &en_priv->edev;
	uint32_t queue_index;
	int32_t rtn = 0;

	if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
		return 0;

	rtn = zxdh_select_queue_assigned(en_dev, skb, &queue_index);
	if (rtn) {
		sk_tx_queue_set(skb->sk, queue_index);
		// LOG_INFO("queue_index = %d\n", queue_index);
		return queue_index;
	}

// #ifdef HAVE_QUEUE_SELECTION_HELPERS_RENAME
#ifdef HAVE_NDO_SELECT_QUEUE_FALLBACK_REMOVED
	queue_index = netdev_pick_tx(netdev, skb, NULL);
	return queue_index;
#else
	return fallback(netdev, skb, NULL);
#endif
}

//     txq_ix = netdev_pick_tx(dev, skb, NULL);
//     up = 0;

//     if (!netdev_get_num_tc(dev))
//         return txq_ix;

// #ifdef CONFIG_EN_DCB
//     if (en_dev->dcbx_dp.trust_state == QPTS_TRUST_DSCP)
//         up = zxdh_get_dscp_up(en_dev, skb);
//     else
// #endif
//         if (skb_vlan_tag_present(skb))
//             up = skb_vlan_tag_get_prio(skb);

//     /* txq_ix can be larger than num_channels since
//      * dev->num_real_tx_queues = num_channels * num_tc
//      */
//     num_channels = en_dev->channels.params.num_channels;
//     if (txq_ix >= num_channels)
//         txq_ix = en_dev->txq2sq[txq_ix]->ch_ix;

//     return en_dev->channel_tc2realtxq[txq_ix][up];

void zxdh_flow_map_cleanup(struct zxdh_en_priv *en_priv)
{
	struct zxdh_en_device *en_dev = &en_priv->edev;

	LOG_INFO("enter\n");
	zxdh_flow_map_remove_sysfs(en_dev);
	hash_init(en_dev->flow_map_hash);
}

int32_t zxdh_flow_map_init(struct zxdh_en_priv *en_priv)
{
	struct zxdh_en_device *en_dev = &en_priv->edev;
	int32_t rtn;

	LOG_DEBUG("enter\n");
	rtn = zxdh_flow_map_init_sysfs(en_dev->netdev);
	if (!rtn) {
		WARN_ON(!hash_empty(en_dev->flow_map_hash));
		hash_init(en_dev->flow_map_hash);
	} else {
		zxdh_flow_map_cleanup(en_priv);
		LOG_ERR("failed to init rate limit\n");
	}

	return rtn;
}

#endif /*ZXDH_CONFIG_SPECIAL_SQ_EN*/