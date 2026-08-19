/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_PPP_H
#define _YK3_PPP_H

#include "yk3.h"

enum yk3_ppp_id {
	PPP_ALL  = 0,
	PPP_LITE = 1,
	PPP_MAX  = 2
};

enum yk3_ppp_primap_mode {
	PRIMAP_VLAN = 0,
	PRIMAP_IP   = 1
};

enum yk3_ppp_ivar_type {
	IVAR_SUMMARY_INFO   = 0x00,  // 信息区域顶层汇总
	IVAR_VLAN_IF0       = 0x10,  // VLAN动态修改接口0
	IVAR_VLAN_IF1       = 0x11,  // VLAN动态修改接口1
	IVAR_VXLAN_PORT_0   = 0x12,  // VXLAN自定义端口0
	IVAR_VXLAN_PORT_1   = 0x13,  // VXLAN自定义端口1
	IVAR_VXLAN_PORT_2   = 0x14,  // VXLAN自定义端口2
	IVAR_VXLAN_PORT_3   = 0x15,  // VXLAN自定义端口3
	IVAR_GENEVE_PORT    = 0x16,  // GENEVE自定义端口
	IVAR_MIRROR         = 0x20,  // 镜像功能开关
	IVAR_FUZZY_TABLE    = 0x21,  // 模糊表开关
	IVAR_HASH_TABLE     = 0x22,  // 哈希表开关
	IVAR_EXACT_TABLE    = 0x23,  // 精确表开关
	IVAR_NP_RDMA_ENABLE = 0x30,  // NP RDMA动态使能开关
};

enum yk3_ppp_action_type {
	YK3_PPP_PTP_ACTION     = 0,
	YK3_PPP_RDMA_RTDEST    = 1,
	YK3_PPP_RDMA_QBASE     = 2,
};

struct yk3_ppp_rdma_val {
	u8 vport;
	union {
		u8 rtdest;
		u8 qbase;
	};
} __packed;

struct yk3_ppp_vxlan_val {
	u8 enable;
	u16 protocol_num;
	u16 mask;
};

struct yk3_ppp_geneve_val {
	u8 enable;
	u16 protocol_num;
	u16 mask;
};

/* action */
int yk3_ppp_action_modify(struct yk3_pdev_priv *pdev_priv, u8 act_type, u16 act_id, void *val);
int yk3_ppp_action_query(struct yk3_pdev_priv *pdev_priv, u8 act_type, u16 act_id, void *val);

/* interactive variable */
int yk3_ppp_ivar_modify(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val);
int yk3_ppp_ivar_query(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val);
int yk3_ppp_rdma_enable(struct yk3_pdev_priv *pdev_priv);
int yk3_ppp_rdma_disable(struct yk3_pdev_priv *pdev_priv);

/* priority map */
int yk3_ppp_primap_mode_get(struct yk3_pdev_priv *pdev_priv);
int yk3_ppp_primap_mode_set(struct yk3_pdev_priv *pdev_priv, u8 primap_mode);
int yk3_ppp_primap_dscp_map_set(struct yk3_pdev_priv *pdev_priv, u8 *primap);
int yk3_ppp_primap_dscp_map_get(struct yk3_pdev_priv *pdev_priv, u8 *primap);

int yk3_ppp_init(struct yk3_pdev_priv *pdev_priv);
void yk3_ppp_exit(struct yk3_pdev_priv *pdev_priv);
#endif /* _YK3_PPP_H */
