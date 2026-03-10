#ifndef _ZXDH_HARDWARE_BOND_H_
#define _ZXDH_HARDWARE_BOND_H_

#include "../en_aux.h"

struct zxdh_bond_group;

#define ZXDH_SPECIAL_LGA_ID 0

struct upper_info_struct {
	struct net_device *upper_dev;
	struct netdev_lag_upper_info lag_upper_info;
};

// 每个事件对应一个独立的节点
struct event_node {
	struct list_head list; // 内核链表节点
	unsigned long event; // 事件类型（如NETDEV_XXX）
	struct upper_info_struct upper_info;
	bool linking;
	uint8_t link_up;
	uint8_t tx_enabled;
	uint32_t idx;
	int32_t group_slave_num;
};

// 全局上下文
struct event_ctx {
	struct delayed_work bond_work;
	spinlock_t lock; // 保护队列的锁
	struct list_head event_list; // 事件链表头（FIFO队列）
	uint32_t idx;
};

struct zxdh_bond_device {
	struct dh_core_dev *pf_core_dev; /* backlink to PF core dev struct */
	struct net_device *netdev; /* this PF's netdev */
	struct net_device *upper_netdev; /* upper bonding netdev */
	struct notifier_block notif_block;

	struct event_ctx ctx;
	struct workqueue_struct *wq;

	uint8_t bonded : 1; /* currently bonded */
	uint8_t tx_enabled : 1;
	uint8_t link_up : 1;

	bool primary; /* this is a primary port */
	bool is_special_bond_dev;

	uint16_t rxq;
	uint16_t txq;

	uint16_t slot;
	uint16_t vport;

	uint16_t vfid;
	uint16_t ovs_pf_vfid;
	uint8_t phy_port;
	// uint16_t primary_vfid;
	bool linking;
	struct list_head node;

	struct upper_info_struct upper_info;
	struct zxdh_bond_group *group;
	struct sockaddr last_mac_addr;
};

struct zxdh_bond_group {
	char name[IFNAMSIZ];

	int32_t group_ida;

	uint8_t lag_tx_type; /* enum zxdh_netdev_lag_tx_type */
	uint8_t hash_policy;
	uint8_t num_slaves;

	bool configured;

	struct list_head node;
};

static inline bool zxdh_netdev_is_hwbond(const struct net_device *netdev)
{
	return (&((struct zxdh_en_priv *)netdev_priv(netdev))->edev)->is_hwbond;
}

static inline uint16_t
zxdh_bond_device_get_vport(const struct zxdh_bond_device *bond_dev)
{
	return bond_dev->vport;
}

static inline const char *
zxdh_bond_group_name(const struct zxdh_bond_group *group)
{
	return group->name;
}

void zxdh_lag_lock_init(void);
void zxdh_lag_lock_deinit(void);

#endif /* END _ZXDH_HARDWARE_BOND_H_ */