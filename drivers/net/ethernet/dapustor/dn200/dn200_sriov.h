/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config dn200 sriov
 */

#ifndef _DN200_SRIOV_H
#define _DN200_SRIOV_H

#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/ethtool.h>
#include "common.h"
#include "dn200_ctrl.h"
#define DN200_MAX_VF_NUM		8
#define DN200_MAX_QPS_PER_VF	1
#define DN200_MAX_VF_LOSS_HB_CNT 120
#define LRAM_UPGRADE_OFFSET 0x10000
#define HW_VF_NUM(hw) ((hw)->priv->plat_ex->pf.registered_vfs)

/******************
 * rxp_entry		used_for			comment
 * 0-1				BC					broadcast_macaddr
 * 2-34				PF_UC			    PF owns 11 uc mac_addr
 * ENTRY 0: UC first
 * ENTRY 1: UC SECOND
 * ENTRY 2: VLAN　ENTRY
 *
 * 35-41			PF_vlan				PF has Five fixed vlan_ids
 * 42-57			VF_UC				VF has only one uc mac_addr
 * 58-123			MC					PF VF own MC
 * 124				ALL Multicast
 * 125				PROMISC
 * 126				all drop			all drop entry(for vlan filter)
 * 127				all bypass			all bypass entry(for rss)
 *
 */
#define DN200_PF_SELF_UC_NUM	(1)
#define DN200_PF_OTHER_UC_NUM	(15)
#define DN200_PF_VLAN_ENTRY_NUM	(7)
#define DN200_VF_UC_OFF	(1 + DN200_PF_SELF_UC_NUM + DN200_PF_OTHER_UC_NUM)
#define DN200_VF_UC_NUM	(8)
#define DN200_PFVF_MC_NUM (33)
#define DN200_BC_RXP_OFF	(1 * 2 + 1 + DN200_PF_VLAN_ENTRY_NUM + (DN200_PF_SELF_UC_NUM + DN200_PF_OTHER_UC_NUM + DN200_VF_UC_NUM + DN200_PFVF_MC_NUM) * 2 + 1 + 1 + 1)
#define DN200_ALL_DROP_OFF		(DN200_BC_RXP_OFF - 1)
#define DN200_MAX_USED_RXP_NUM	(DN200_BC_RXP_OFF + 1)
#define DN200_ALL_PROMISC_OFF		(DN200_ALL_DROP_OFF - 1)
#define DN200_ALL_MULTCAST_OFF		(DN200_ALL_PROMISC_OFF - 1)
#define DN200_MAX_UC_MAC_ADDR_NUM	15
#define DN200_PF_UC_ADDR_START	2
#define DN200_PF_UC_ADDR_END	16
#define DN200_MC_ADDR_START	(1 + DN200_PF_SELF_UC_NUM + DN200_PF_OTHER_UC_NUM + 8)
#define DN200_MC_ADDR_END	(33 + 1 + DN200_PF_SELF_UC_NUM + DN200_PF_OTHER_UC_NUM + 8 - 1)

#define DN200_VLAN_ADDR_START	(1 * 2 + 2 * 16 + 1)

#define DMA_CHA_NO_OFFSET(id)	((id) * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) + offsetof(struct RXP_FPR_ENTRY, dma_ch_no) / sizeof(u32))
#define OK_INDEX_ENTRY_OFFSET(id)	((id) * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) + offsetof(struct RXP_FPR_ENTRY, ok_index) / sizeof(u32))
#define AFRFNC_ENTRY_OFFSET(id)	((id) * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) + 2)
#define OK_INDEX_MASK GENMASK(23, 16)
#define OK_INDEX_OFFSET 16
#define AF_ENABLE BIT(0)
#define RF_ENABLE BIT(1)
#define IM_ENABLE BIT(2)
#define NC_ENABLE BIT(3)

#define DA_DMA_CHA_NO_OFFSET_AMPM(id)	DMA_CHA_NO_OFFSET((id))
#define DA_DMA_CHA_NO_OFFSET(id)	DMA_CHA_NO_OFFSET(((id) + 1))
#define DA_OK_INDEX_FIRST_ENTRY_OFFSET(id)	OK_INDEX_ENTRY_OFFSET((id))
#define DA_OK_INDEX_SEC_ENTRY_OFFSET(id)	OK_INDEX_ENTRY_OFFSET(((id) + 1))
#define DN200_MAX_TC_ENTRY_NUM	16

#define DN200_VLAN_NUM 6

#define DN200_FIFO_SIZE_CHANGE_SIGN 0x14

/*SRIOV PF q0 tx fifo 11.5K, other q 3.5K*/
#define DN200_SRIOV_Q0_TX_FIFO_SIZE	11776
/*PUREPF PF q0 tx fifo 16K other 4K*/
#define DN200_PURE_Q0_TX_FIFO_SIZE	15360
#define DN200_OTHER_Q_TX_FIFO_SIZE	5120
#define DN200_SRIOV_01_TX_FIFO_SIZE	3584
#define DN200_PURE_01_TX_FIFO_SIZE	7168
#define DN200_01_VF_RX_FIFO_SIZE	16384
#define DN200_RX_TC_FIFO_SIZE		11264 /* 11KB */
#define DN200_RX_SUPER_FIFO_SIZE	20480 /* 20KB */
#define DN200_RX_TC_FIFO_SIZE_1G	12800 /* 12.5KB */
#define DN200_RX_SUPER_FIFO_SIZE_1G	14366 /* 14KB */
#define DN200_PURE_RX_TC_FIFO_SIZE	16384 /* 16KB */
#define DN200_PURE_RX_SUPER_FIFO_SIZE	0 /* 0 */
#define DN200_RX_UNTAG_PKT_FIFO_INDEX	0
#define DN200_TX_FIFO_SIZE_PORT2_3 16384
#define DN200_RX_FIFO_SIZE_PORT2_3 16384

#define DN200_23_VF_FIFO_SIZE		16384
#define DN200_01_MAX_FIFO_SIZE		65536
#define DN200_23_MAX_FIFO_SIZE		32768

#define PRIV_SRIOV_SUPPORT(priv) ((priv)->plat_ex->sriov_supported)
#define PRIV_NVME_SUPPORT(priv) ((priv)->plat_ex->nvme_supported)
#define DN200_VF_OFFSET_GET(hw) ((hw)->priv->plat_ex->vf_offset)
struct mac_addr_route {
	u8 mac_addr[ETH_ALEN];
	u16 channel;
	int rxp_offset;
} __aligned(8);

struct dn200_vf_info {
	u8 rx_queue_start;
	u8 tx_queue_start;
	u8 rx_queues_num;
	u8 tx_queues_num;
	u8 max_vfs;
	u8 registered_vfs;
	u16 max_vlan_num;
	u8 iatu_num;
}  __aligned(8);

struct dn200_pf_info {
	u16	active_vfs;
	u16	registered_vfs;
	u16	max_vfs;
	u16 vlan_num_per_vf;
	void __iomem *ioaddr;
	void __iomem *ctrl_addr;
	unsigned long	*vf_event_bmap;
	struct dn200_vf_info *vfs;
};

struct dn200_sriov_phy_info {
	u8 media_type;
	u8 an;
	u8 pause;
	u8 phy_interface;
	/*link status*/
	int speed;
	u8 dup;
	u16 link_modes;
} __aligned(8);

union l3l4_info_t {
	struct {
		u32 funcid:4;
		u32 entry_idx:5;
		u32 offset:5;
	};
	u32 l3l4_info;
};

struct RXP_FPR_ENTRY {
	u32 match_data;
	u32 match_en;
	u8 af:1;
	u8 rf:1;
	u8 im:1;
	u8 nc:1;
	u8 res1:4;
	u8 frame_offset:6;
	u8 res2:2;
	u8 ok_index;
	u8 giv:1;
	u8 gid:3;
	u8 res3:4;
	u16 dma_ch_no;
	u16 res4;
} __packed;

/* apps use hw interrupt to sync with peer(pf or vf)*/
enum ITR_SYNC_APP_ID {
	HW_RESET_ID,
	FLOW_STATE_ID,
	RXP_TASK,

	/* MAX_APP_ID is last one, put others before it */
	MAX_APP_ID
};

enum HW_RESET_EVENT {
	VF2PF_ERR_RST_NOTIFY = 0,
	/* MAX_RESET_EVENT is last one, put others before it */
	MAX_RESET_EVENT
};

enum dn200_lram_lock {
	DN200_LRAM_LOCK,
	DN200_UC_LRAM_LOCK,
	DN200_MC_LRAM_LOCK,
	DN200_L3L4_LRAM_LOCK,
	DN200_BC_LRAM_LOCK,
};

enum VF_FLOW_STATE_EVENT {
	FLOW_OPEN_START = 1,
	FLOW_OPEN_DONE,
	FLOW_CLOSE_START,
	FLOW_CLOSE_DONE,
};

struct dn200_itr_sync {
	/* all app (e.g. hw reset) which use hw interrupt to sync between pf & vf
	 * one byte represet one event, 1: app exist 0: not exist
	 */
	u8 itr_sync_app[MAX_APP_ID];
	/* e.g. vf notify pf to do reset; or pf notify vf to do reset,
	 * one byte represet one event, 1: event exist 0: not exist
	 */
	u8 reset_event[MAX_RESET_EVENT];
	/* record vf function that have done reset
	 * one byte represet one func, 1: fun done reset 0: not done
	 */
	u8 reset_func_list[DN200_MAX_VF_NUM + 1];
	u8 vf_flow_state_event[DN200_MAX_VF_NUM];
	u8 vf_reset_mac_list[DN200_MAX_VF_NUM];
	u8 vf_link_list[DN200_MAX_VF_NUM];
	u8 vf_carrier[DN200_MAX_VF_NUM];
	u8 vf_probe[DN200_MAX_VF_NUM];
	u8 pf_carrier;
} __aligned(8);

enum DN200_REG_VF_STATE {
	DN200_VF_REG_STATE_NONE = 0,
	DN200_VF_REG_STATE_OPENED = BIT(0),
	DN200_VF_REG_STATE_IN_RST = BIT(1),
};

struct dn200_heartbeat_info {
	/*vf should do : heatbeat = heatbeat ^ last_heatbeat*/
	u8 heartbeat[DN200_MAX_VF_NUM + 1];		/*registered_vf_state set headbeat */
	/*Pf should check last_heatbeat != heatbeat && set last_heatbeat = heatbeat */
	u8 last_heartbeat[DN200_MAX_VF_NUM + 1];
	/* all probed vf should set registered_vf_state to true,
	 * PF will clear this when vf's rxp reset
	 * VF should check this to judge whether itself has been reset by PF
	 */
	u8 registered_vf_state[DN200_MAX_VF_NUM + 1];
};

/* ===================================
 * SRIOV LRAM MAILBOX STRUCT INFO
 */

#define SRIOV_MAX_SIZE_PER_PF 4096

#define DN200_SRIOV_MAC_OFFSET 0x0
//store mac addr info to mailbox bar mem
struct dn200_mailbox_info {

	unsigned long bitmap_rxp;
	unsigned long bitmap_uc;
	unsigned long bitmap_l3l4;
	unsigned long bitmap_mac;
	u8 link_status;
	u8 vf_link_notify[DN200_MAX_VF_NUM];
	u16 vxlan_status;
	u8 uc_mac_addr[DN200_MAX_VF_NUM][ETH_ALEN];
	struct dn200_itr_sync itr_sync_info;
	struct dn200_sriov_phy_info sriov_phy_info;
	struct mac_addr_route mac_addr[DN200_MAX_MC_ADDR_NUM];
	union l3l4_info_t l3l4_info[32];
	struct dn200_heartbeat_info heartbeat;
	unsigned long bitmap_allmucast;
	unsigned long bitmap_promisc;
	struct mac_addr_route pf_uc_mac_addr[DN200_MAX_UC_MAC_ADDR_NUM];
	u8 pf_states;
	u8 pf_fw_err_states;
	u8 vf_upgrade_state[DN200_MAX_VF_NUM];
} __aligned(8);

#define DN200_MAX_SIZE_MAILBOX_INFO (1536)
_Static_assert((sizeof(struct dn200_mailbox_info) < DN200_MAX_SIZE_MAILBOX_INFO), "struct mailbox_info cannot exceed 2K!!");

#define DN200_SRIOV_QUEUE_OFFSET (DN200_MAX_SIZE_MAILBOX_INFO)

struct dn200_sriov_queue_info {
	struct dn200_vf_info vf_info[DN200_MAX_VF_NUM];
} __aligned(8);
#define DN200_MAX_SIZE_SRIOV_QUEUE_INFO (256)
_Static_assert((sizeof(struct dn200_sriov_queue_info) < DN200_MAX_SIZE_SRIOV_QUEUE_INFO), "struct sriov_queue cannot exceed 512!!");
#define DN200_LOCK_INFO_OFFSET (DN200_SRIOV_QUEUE_OFFSET + DN200_MAX_SIZE_SRIOV_QUEUE_INFO)
struct dn200_sriov_lock_info {
	/* BIT 0	LOCKED
	 * BIT 1-16	LOCK_FUNC
	 * else		reserved
	 */
	u32 lock_info;
} __aligned(8);
#define DN200_SIZE_MAX_LOCK_INFO (16)
_Static_assert((sizeof(struct dn200_sriov_lock_info) < DN200_SIZE_MAX_LOCK_INFO), "struct sriov_msix_info cannot exceed 128!!");
#define DN200_VFMAILBOX_SIZE	16 /* 16 32 bit words - 64 bytes */
#define DN200_MAILBOX_INFO_OFFSET (DN200_LOCK_INFO_OFFSET + DN200_SIZE_MAX_LOCK_INFO)
struct dn200_sriov_mbx_info {
	u32 msg_type;
	u32 msg_len;
	u32 msgbuf[DN200_VFMAILBOX_SIZE];
	struct dn200_heartbeat_info heartbeat;
} __aligned(8);
#define DN200_SIZE_SRIOV_MBX_INFO (192)
_Static_assert((sizeof(struct dn200_sriov_mbx_info) < DN200_SIZE_SRIOV_MBX_INFO), "struct sriov_mbx_info cannot exceed 256!!");

#define DN200_VER_INFO_OFFSET (DN200_MAILBOX_INFO_OFFSET + DN200_SIZE_SRIOV_MBX_INFO)
struct dn200_sriov_ver_info {
	struct dn200_ver dn200_ver_info;
} __aligned(8);
#define DN200_SIZE_SRIOV_VER_INFO (16)
_Static_assert((sizeof(struct dn200_sriov_ver_info) < DN200_SIZE_SRIOV_VER_INFO), "struct sriov_ver_info cannot exceed 128!!");

#define DN200_RXP_INFO_OFFSET (DN200_VER_INFO_OFFSET + DN200_SIZE_SRIOV_VER_INFO)

#define LRAM_PRIV_MAC_PF_OFFSET(priv) \
	((priv)->plat_ex->pf.ioaddr + DN200_SRIOV_MAC_OFFSET + (priv)->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)
#define LRAM_MAC_PF_OFFSET(hw) \
	((hw)->pmail + DN200_SRIOV_MAC_OFFSET + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)
#define LRAM_QUEUE_PF_OFFSET(hw) \
	((hw)->pmail + DN200_SRIOV_QUEUE_OFFSET + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)
#define LRAM_LOCK_PF_OFFSET(hw) \
	((hw)->pmail + DN200_LOCK_INFO_OFFSET + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)
#define LRAM_MBX_PF_OFFSET(hw) \
	((hw)->pmail + DN200_MAILBOX_INFO_OFFSET + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)
#define LRAM_PRIV_MAC_VF_OFFSET(priv, vf_id, i) \
	(LRAM_PRIV_MAC_PF_OFFSET(priv) + offsetof(struct dn200_mailbox_info, uc_mac_addr) + vf_id * 6 + i)
#define LRAM_VER_PF_OFFSET(hw) \
	((hw)->pmail + DN200_VER_INFO_OFFSET + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF)

_Static_assert(((DN200_VER_INFO_OFFSET + DN200_SIZE_SRIOV_VER_INFO) < 2048), "LRAM size per PF cannot exceed 2K");

struct dn200_upgrade_info {
	u32 magic_num;
	u32 nic_st;
	u32 host_st;
	u32 rsv;
	u32 pf_stop[4];
	u32 pf_start[4];
	u32 upgrade_flag;
} __aligned(8);
_Static_assert(sizeof(struct dn200_upgrade_info) < 64, "LRAM upgrade info's size cannot exceed 64");

/* ==========================================
 */
#define DN200_ITR_SYNC_GET(hw, member, offset, u8data) \
do { \
	u8 *__data = u8data;	\
	*__data = readb(LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, itr_sync_info) + offsetof(struct dn200_itr_sync, member) + (offset)); \
} while (0)

#define DN200_ITR_SYNC_SET(hw, member, offset, u8data) \
do { \
	u8 __data = u8data;	\
	writeb(__data, LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, itr_sync_info) + offsetof(struct dn200_itr_sync, member) + (offset)); \
} while (0)

#define DN200_HEARTBEAT_GET(hw, member, vfoffset, u8data) \
do { \
	u8 *__data = u8data;	\
	*__data = readb(LRAM_MBX_PF_OFFSET(hw) + offsetof(struct dn200_sriov_mbx_info, heartbeat) + offsetof(struct dn200_heartbeat_info, member) + (vfoffset)); \
} while (0)

#define DN200_HEARTBEAT_SET(hw, member, vfoffset, u8data) \
do { \
	u8 __data = u8data;	\
	writeb(__data, LRAM_MBX_PF_OFFSET(hw) + offsetof(struct dn200_sriov_mbx_info, heartbeat) + offsetof(struct dn200_heartbeat_info, member) + (vfoffset)); \
} while (0)

#define DN200_VF_UPGRADE_GET(priv, vfoffset, u8data) \
do { \
	u8 *__data = u8data;	\
	*__data = readb(LRAM_PRIV_MAC_PF_OFFSET(priv) + offsetof(struct dn200_mailbox_info, vf_upgrade_state) + (vfoffset)); \
} while (0)

#define DN200_VF_UPGRADE_SET(priv, vfoffset, u8data) \
do { \
	u8 __data = u8data;	\
	writeb(__data, LRAM_PRIV_MAC_PF_OFFSET(priv) + offsetof(struct dn200_mailbox_info, vf_upgrade_state) + (vfoffset)); \
} while (0)

#define DN200_VF_LINK_GET(priv, vfoffset, u8data) \
do { \
	u8 *__data = u8data;	\
	*__data = readb(LRAM_PRIV_MAC_PF_OFFSET(priv) + offsetof(struct dn200_mailbox_info, vf_link_notify) + (vfoffset)); \
} while (0)

#define DN200_VF_LINK_SET(priv, vfoffset, u8data) \
do { \
	u8 __data = u8data;	\
	writeb(__data, LRAM_PRIV_MAC_PF_OFFSET(priv) + offsetof(struct dn200_mailbox_info, vf_link_notify) + (vfoffset)); \
} while (0)

#define DN200_GET_LRAM_MAILBOX_MEMBER(hw, member, data) \
do { \
	void *__mptr = (void *)(data); \
	switch (sizeof_field(struct dn200_mailbox_info, member)) { \
	case 1:	\
		*(u8 *)__mptr = readb(LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 2:	\
		*(u16 *)__mptr = readw(LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 4:	\
		*(u32 *)__mptr = readl(LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 8:	\
		*(u64 *)__mptr = readq(LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	default:	\
		*(u8 *)__mptr = 0;	\
			break;	\
	}	\
} while (0)

#define DN200_SET_LRAM_MAILBOX_MEMBER(hw, member, data) \
do { \
	switch (sizeof_field(struct dn200_mailbox_info, member)) { \
	case 1:	\
		writeb((u8)data, LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 2:	\
		writew((u16)data, LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 4:	\
		writel((u32)data, LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	case 8:	\
		writeq((u64)data, LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info, member));	\
		break;	\
	default:	\
		break;	\
	}	\
} while (0)

#define DN200_GET_LRAM_UPGRADE_MEMBER(hw, member, data) \
do { \
	void *__mptr = (void *)(data); \
	switch (sizeof_field(struct dn200_upgrade_info, member)) { \
	case 1:	\
		*(u8 *)__mptr = readb((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 2:	\
		*(u16 *)__mptr = readw((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 4:	\
		*(u32 *)__mptr = readl((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 8:	\
		*(u64 *)__mptr = readq((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	default:	\
		*(u8 *)__mptr = 0;	\
			break;	\
	}	\
} while (0)

#define DN200_SET_LRAM_UPGRADE_MEMBER(hw, member, data) \
do { \
	switch (sizeof_field(struct dn200_upgrade_info, member)) { \
	case 1:	\
		writeb((u8)data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 2:	\
		writew((u16)data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 4:	\
		writel((u32)data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	case 8:	\
		writeq((u64)data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, member));	\
		break;	\
	default:	\
		break;	\
	}	\
} while (0)

#define DN200_SET_LRAM_UPGRADE_PF(hw, u32data, pf_id) \
	writel((u32)u32data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, pf_stop) + ((pf_id) * 4)) \

#define DN200_GET_LRAM_UPGRADE_PF(hw, u32data, pf_id) \
do { \
	u32 *__data = u32data;	\
	*__data = readl((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, pf_stop) + ((pf_id) * 4)); \
} while (0)

#define DN200_SET_LRAM_UPGRADE_PF_FINISH(hw, u32data, pf_id) \
	writel((u32)u32data, (hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, pf_start) + ((pf_id) * 4)) \

#define DN200_GET_LRAM_UPGRADE_PF_FINISH(hw, u32data, pf_id) \
do { \
	u32 *__data = u32data;	\
	*__data = readl((hw)->pmail - LRAM_UPGRADE_OFFSET + \
			offsetof(struct dn200_upgrade_info, pf_start) + ((pf_id) * 4)); \
} while (0)

enum dn200_upgrade_state {
	DN200_UNFINISH_FLAG = 1,
	DN200_STOP_FLAG,
	DN200_JMP_FAIL_FLAG,
	DN200_START_FLAG,
};
/* dn200 vf dev info*/
/* =========================== */

#define DN200_VF_RXP_ASYNC_INFO_SIZE \
	sizeof(struct dn200_vf_rxp_async_info)

#define DN200_VF_RXP_WB_INFO_SIZE \
	sizeof(struct dn200_vf_rxp_async_wb)

#define DN200_VF_TOTAL_RXP_INFO_SIZE (256)
_Static_assert(((DN200_VF_RXP_ASYNC_INFO_SIZE + DN200_VF_RXP_WB_INFO_SIZE) < DN200_VF_TOTAL_RXP_INFO_SIZE), "RXP INFO exceed 256");

#define DN200_VF_RXP_BASE (16384)
#define LRAM_VF_RXP_INFO_OFFSET(hw, vf_num) \
	((hw)->pmail + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF + DN200_RXP_INFO_OFFSET \
		+ DN200_VF_TOTAL_RXP_INFO_SIZE * (vf_num))

#define LRAM_VF_RXP_WB_INFO_OFFSET(hw, vf_num) \
	((hw)->pmail + (hw)->priv->plat_ex->pf_id * SRIOV_MAX_SIZE_PER_PF + DN200_RXP_INFO_OFFSET \
		+ DN200_VF_TOTAL_RXP_INFO_SIZE * (vf_num) + DN200_VF_RXP_ASYNC_INFO_SIZE)

void dn200_sriov_reconfig_hw_feature(struct dn200_priv *priv, struct dma_features *dma_cap);
int dn200_sriov_configure(struct pci_dev *pdev, int num_vfs);
int dn200_sriov_disable(struct dn200_priv *priv);
int dn200_sriov_enable(struct dn200_priv *priv, int num_vfs);
void dn200_get_func_mac_addr(struct mac_device_info *hw, u8 offset, struct mac_addr_route *mac_addr);
void dn200_set_func_mac_addr(struct mac_device_info *hw, u8 offset, struct mac_addr_route *mac_addr);
void dn200_get_func_uc_mac_addr(struct mac_device_info *hw, u8 offset, struct mac_addr_route *mac_addr);
void dn200_set_func_uc_mac_addr(struct mac_device_info *hw, u8 offset, struct mac_addr_route *mac_addr);
int dn200_hw_lock(struct mac_device_info *hw, bool *is_locked);
void dn200_hw_unlock(struct mac_device_info *hw, bool *is_locked);
void dn200_get_vf_queue_info(void __iomem *mailbox, struct dn200_vf_info *info, u8 pf_id, u8 funcid);
int dn200_get_prev_used_bit(unsigned long *bitmap, u8 offset);
int dn200_get_next_used_bit(unsigned long *bitmap, u8 offset, u8 last);
int dn200_get_unused_bit(unsigned long *bitmap, u8 last, u8 first);
void dn200_pf_set_link_status(struct mac_device_info *hw, u8 link_status);
u8 dn200_vf_get_link_status(struct mac_device_info *hw);
void dn200_set_phy_info(struct mac_device_info *hw, struct dn200_sriov_phy_info *info);
void dn200_get_phy_info(struct mac_device_info *hw, struct dn200_sriov_phy_info *info);
int dn200_get_l3l4_filter_offset(struct mac_device_info *hw, int idx);
int dn200_set_l3l4_filter_info(struct mac_device_info *hw, int idx, int offset, bool clear);
void dn200_sriov_mail_init(struct dn200_priv *priv);
void dn200_sriov_static_init(struct dn200_priv *priv);
int dn200_sriov_vlan_entry_update(struct dn200_priv *priv);
u16 dn200_get_vxlan_status(struct mac_device_info *hw);
void dn200_set_vxlan_status(struct mac_device_info *hw, u16 vxlan_status);
int dn200_vf_glb_err_rst_notify(struct dn200_priv *priv);
int dn200_pf_glb_err_rst_process(struct dn200_priv *priv);
void dn200_vf_flow_state_set(struct dn200_priv *priv, u8 flow_sate);
bool dn200_all_vf_flow_state_wait(struct dn200_priv *priv, u8 wait_state, bool in_task);
void dn200_all_vf_flow_state_clear(struct dn200_priv *priv);
void dn200_vf_flow_close(struct dn200_priv *priv);
void dn200_vf_flow_open(struct dn200_priv *priv);
void _dn200_vf_flow_open(struct dn200_priv *priv);
void dn200_sriov_ver_get(struct dn200_priv *priv, struct dn200_ver *ver_info);
void dn200_sriov_ver_set(struct dn200_priv *priv, struct dn200_ver *ver_info);
void dn200_reset_lram_rxp_async_info(struct mac_device_info *hw);
void dn200_get_lram_rxp_async_crc32(struct mac_device_info *hw, u8 vf_num, u8 *info);
void dn200_set_lram_rxp_async_info(struct mac_device_info *hw, u8 *info);
void dn200_get_lram_rxp_async_info(struct mac_device_info *hw, u8 *info, u8 vf_num);
void dn200_set_lram_rxp_wb_info(struct mac_device_info *hw, u8 *info);
void dn200_get_lram_rxp_wb_info(struct mac_device_info *hw, u8 *info, u8 vf_num);
#endif
