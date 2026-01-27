// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config dn200 sriov
 */
#include <linux/bitrev.h>
#include <linux/crc32.h>
#include <linux/printk.h>
#include <linux/iopoll.h>
#include <linux/pci.h>
#include "dn200.h"
#include "dn200_sriov.h"

void dn200_sriov_reconfig_hw_feature(struct dn200_priv *priv,
				     struct dma_features *dma_cap)
{
	int tx_fifo_size = 0;
	bool port_is_0_and_1 = false;

	if (!PRIV_IS_VF(priv)) {
		if (priv->plat_ex->pf_id < 2)
			port_is_0_and_1 = true;
		else
			port_is_0_and_1 = false;
	}

	if (PRIV_SRIOV_SUPPORT(priv)) {
		if (port_is_0_and_1) {
			tx_fifo_size = DN200_SRIOV_01_TX_FIFO_SIZE;
			priv->tx_fifo_queue_0 = DN200_SRIOV_Q0_TX_FIFO_SIZE;
			dma_cap->tx_fifo_size =
			DN200_01_MAX_FIFO_SIZE - tx_fifo_size * priv->plat_ex->max_vfs;
			if (priv->plat_ex->max_speed == SPEED_10000) {
				priv->mtl_queue_fifo_avg = DN200_RX_TC_FIFO_SIZE;
				priv->mtl_queue_fifo_more = DN200_RX_SUPER_FIFO_SIZE;
			} else {
				priv->mtl_queue_fifo_avg = DN200_RX_TC_FIFO_SIZE_1G;
				priv->mtl_queue_fifo_more = DN200_RX_SUPER_FIFO_SIZE_1G;
			}
			priv->vf_tx_fifo_size = tx_fifo_size;
		} else {
			priv->mtl_queue_fifo_avg = DN200_RX_FIFO_SIZE_PORT2_3;
			priv->mtl_queue_fifo_more = DN200_RX_FIFO_SIZE_PORT2_3;
			priv->tx_fifo_queue_0 = DN200_TX_FIFO_SIZE_PORT2_3;
			dma_cap->tx_fifo_size = DN200_TX_FIFO_SIZE_PORT2_3;
			dma_cap->rx_fifo_size = DN200_RX_FIFO_SIZE_PORT2_3;
			priv->vf_tx_fifo_size = DN200_TX_FIFO_SIZE_PORT2_3;
		}
	} else if (PRIV_IS_PUREPF(priv)) {
		if (port_is_0_and_1) {
			tx_fifo_size = DN200_PURE_01_TX_FIFO_SIZE;
			priv->tx_fifo_queue_0 = DN200_PURE_Q0_TX_FIFO_SIZE;
			dma_cap->tx_fifo_size =
				tx_fifo_size * (priv->plat_ex->default_tx_queue_num - 1) + priv->tx_fifo_queue_0;
			if (priv->plat_ex->max_speed == SPEED_10000) {
				/* no need give more fifo to last queue */
				priv->mtl_queue_fifo_avg = DN200_PURE_RX_TC_FIFO_SIZE;
				priv->mtl_queue_fifo_more = DN200_PURE_RX_SUPER_FIFO_SIZE;
			} else {
				/* no need give more fifo to last queue */
				priv->mtl_queue_fifo_avg = DN200_PURE_RX_TC_FIFO_SIZE;
				priv->mtl_queue_fifo_more = DN200_PURE_RX_SUPER_FIFO_SIZE;
			}
		} else {
			priv->mtl_queue_fifo_avg = DN200_23_MAX_FIFO_SIZE;
			priv->mtl_queue_fifo_more = 0;
			priv->tx_fifo_queue_0 = DN200_TX_FIFO_SIZE_PORT2_3;
			dma_cap->tx_fifo_size = DN200_23_MAX_FIFO_SIZE;
			dma_cap->rx_fifo_size = DN200_23_MAX_FIFO_SIZE;
		}
	}
	dma_cap->vlhash = 0;
}

int dn200_sriov_vlan_entry_update(struct dn200_priv *priv)
{
	int ret = 0;
	int i = 0;
	u16 vid = 0;

	if (priv->plat_ex->vlan_num > (priv->hw->max_vlan_num + 1))
		return ret;
	for_each_set_bit(vid, priv->active_vlans, VLAN_N_VID) {
		__le16 vid_le = cpu_to_le16(vid);

		ret =
		    dn200_add_hw_vlan_rx_fltr(priv, NULL, priv->hw, 0,
					      vid_le, i,
					      i == (priv->plat_ex->vlan_num - 1));
		if (ret)
			return ret;
		i++;
	}
	return ret;
}

int dn200_get_prev_used_bit(unsigned long *bitmap, u8 offset)
{
	int i = 0;

	if (offset > DN200_MC_ADDR_END)
		return -1;

	if (offset)
		i = offset - 1;
	else
		return -1;

	for (; i >= 0; i--) {
		if (bitmap[0] & (1ULL << (i)))
			return i;
	}
	return -1;
}

int dn200_get_next_used_bit(unsigned long *bitmap, u8 offset, u8 last)
{
	int i = offset + 1;

	if (offset > DN200_MC_ADDR_END)
		return -1;
	if (last > DN200_MC_ADDR_END)
		return -1;

	for (; i < last; i++) {
		if (bitmap[0] & (1ULL << (i)))
			return i;
	}

	if (i == last)
		i = -1;
	return i;
}

int dn200_get_unused_bit(unsigned long *bitmap, u8 last, u8 first)
{
	int i = first;

	if (last > DN200_MC_ADDR_END)
		return -1;
	for (; i <= last; i++) {
		if (!(bitmap[0] & (1ULL << (i))))
			return i;
	}
	return -1;
}

void dn200_get_func_uc_mac_addr(struct mac_device_info *hw, u8 offset,
				struct mac_addr_route *mac_addr)
{
	int i = 0;

	for (; i < sizeof(struct mac_addr_route); i++) {
		*((u8 *) mac_addr + i) =
		    readb(LRAM_MAC_PF_OFFSET(hw) +
			  offsetof(struct dn200_mailbox_info,
				   pf_uc_mac_addr[offset]) + i);
	}
}

void dn200_set_func_uc_mac_addr(struct mac_device_info *hw, u8 offset,
				struct mac_addr_route *mac_addr)
{
	int i = 0;

	for (; i < sizeof(struct mac_addr_route); i++) {
		writeb(*((u8 *) mac_addr + i),
		       LRAM_MAC_PF_OFFSET(hw) +
		       offsetof(struct dn200_mailbox_info,
				pf_uc_mac_addr[offset]) + i);
	}
}

void dn200_get_func_mac_addr(struct mac_device_info *hw, u8 offset,
			     struct mac_addr_route *mac_addr)
{
	int i = 0;

	for (; i < sizeof(struct mac_addr_route); i++) {
		*((u8 *) mac_addr + i) =
		    readb(LRAM_MAC_PF_OFFSET(hw) +
			  offsetof(struct dn200_mailbox_info,
				   mac_addr[offset]) + i);
	}
}

void dn200_set_func_mac_addr(struct mac_device_info *hw, u8 offset,
			     struct mac_addr_route *mac_addr)
{
	int i = 0;

	for (; i < sizeof(struct mac_addr_route); i++) {
		writeb(*((u8 *) mac_addr + i),
		       LRAM_MAC_PF_OFFSET(hw) +
		       offsetof(struct dn200_mailbox_info,
				mac_addr[offset]) + i);
	}
}

u16 dn200_get_vxlan_status(struct mac_device_info *hw)
{
	u16 data;

	data =
	    readw(LRAM_MAC_PF_OFFSET(hw) +
		  offsetof(struct dn200_mailbox_info, vxlan_status));
	return data;
}

void dn200_set_vxlan_status(struct mac_device_info *hw, u16 vxlan_status)
{
	writew(vxlan_status,
	       LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info,
						 vxlan_status));
}

void dn200_reset_lram_rxp_async_info(struct mac_device_info *hw)
{
	int i = 0;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_info);

	for (; i < info_size; i++)
		writeb(0, LRAM_VF_RXP_INFO_OFFSET(hw, DN200_VF_OFFSET_GET(hw)) + i);
}

void dn200_set_lram_rxp_async_info(struct mac_device_info *hw, u8 *info)
{
	int i = 0;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_info);

	for (; i < info_size; i++)
		writeb(*((u8 *)info + i), LRAM_VF_RXP_INFO_OFFSET(hw, DN200_VF_OFFSET_GET(hw)) + i);
}

void dn200_get_lram_rxp_async_info(struct mac_device_info *hw, u8 *info, u8 vf_num)
{
	int i = 0;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_info);

	for (; i < info_size; i++)
		*(info + i) = readb(LRAM_VF_RXP_INFO_OFFSET(hw, (vf_num)) + i);
}

void dn200_get_lram_rxp_async_crc32(struct mac_device_info *hw, u8 vf_num, u8 *info)
{
	int i = 0;
	size_t info_size = sizeof(u32);

	for (; i < info_size; i++)
		*(info + i) = readb(LRAM_VF_RXP_INFO_OFFSET(hw, (vf_num)) + i);
}

void dn200_set_lram_rxp_wb_info(struct mac_device_info *hw, u8 *info)
{
	int i = 0;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_wb);

	for (; i < info_size; i++)
		writeb(*((u8 *)info + i), LRAM_VF_RXP_WB_INFO_OFFSET(hw, DN200_VF_OFFSET_GET(hw)) + i);
}

void dn200_get_lram_rxp_wb_info(struct mac_device_info *hw, u8 *info, u8 vf_num)
{
	int i = 0;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_wb);

	for (; i < info_size; i++)
		*(info + i) = readb(LRAM_VF_RXP_WB_INFO_OFFSET(hw, (vf_num)) + i);
}
/**
 *  dn200_hw_lock - set gloabl spin lock in firmware
 *  @hw: hw mac device information
 *  @is_locked: used as input and output, if it is true just return; if false get the lock
 *  Description: this function is used by lram & share table(e.g. rxp),
 *  it will set global spin lock in firmare,
 *  if suceess, output var is_locked will be true, and return 0
 *  if failure, output var is_locked will be false, and return err code
 */
int dn200_hw_lock(struct mac_device_info *hw, bool *is_locked)
{
	int ret = 0;
	u32 retry = 0;
	u32 ret_val = 0;
	struct dn200_priv *priv = hw->priv;

	/* when vf flow is stoped, don't allow to set rxp */
	if (HW_IS_VF(hw) && test_bit(DN200_VF_IN_STOP, &priv->state))
		return -1;

#define MAX_SCHED_TIMES 10
#define MAX_RETRY_TIMES 10000
	if (*is_locked)
		return 0;

	while (retry++ < MAX_RETRY_TIMES) {
		ret = lram_and_rxp_lock_and_unlock(&priv->plat_ex->ctrl,
						 BIT(0), &ret_val);
		if (ret) {
			netdev_err(priv->dev,
				   "get lock fail, for cq not return\n");
			return ret;
		}
		if (ret_val && ret_val == 1) {
			udelay(5);
			continue;
		} else {
			break;
		}
	}

	if (retry > MAX_RETRY_TIMES) {
		priv->swc.hw_lock_timeout++;
		ret = -1;
	}

	if (!ret) {
		*is_locked = true;
		priv->swc.hw_lock_recfgs = 0;
	} else {
		*is_locked = false;
		priv->swc.hw_lock_fail_cnt++;
		/* just reconfig limit times, stop schedule the work when always lock failure */
		if (++priv->swc.hw_lock_recfgs < MAX_SCHED_TIMES)
			queue_work(priv->wq, &priv->reconfig_task);
	}
	return ret;
}

/**
 *  dn200_hw_unlock - free the gloabl spin lock in firmware
 *  @hw: hw mac device information
 *  @is_locked: used as input and output, if it is true need to unlock; if false do nothing
 *  Description: this function is used by lram & share table(e.g. rxp),
 *  it will free global spin lock in firmare,
 *  if suceess, output var is_locked will be false
 *  if failure, output var is_locked will be unchanged
 */
void dn200_hw_unlock(struct mac_device_info *hw, bool *is_locked)
{
	u32 ret_val = 0;
	int ret = 0;

	if (*is_locked) {
		ret = lram_and_rxp_lock_and_unlock(&hw->priv->plat_ex->ctrl, 0, &ret_val);
		if (ret_val)
			netdev_err(hw->priv->dev, "lram and rxp unlock failed\n");

		if (ret)
			netdev_err(hw->priv->dev,
				"unlock failed, for cq not return func =%s, line = %d\n",
				__func__, __LINE__);

		if ((ret == 0) && (ret_val == 0))
			*is_locked = false;
	}
}

/* idx : l3l4_entry idx
 */
int dn200_get_l3l4_filter_offset(struct mac_device_info *hw, int idx)
{
	int i = 0;
	union l3l4_info_t l3l4_info;
	u32 funcid = hw->priv->plat_ex->vf_offset + HW_IS_VF(hw) + 1;

	for (; i < 32; i++) {
		l3l4_info.l3l4_info =
		    readl(LRAM_MAC_PF_OFFSET(hw) +
			  offsetof(struct dn200_mailbox_info,
				   l3l4_info) + i * sizeof(u32));
		if (l3l4_info.funcid == funcid && l3l4_info.entry_idx == idx)
			return i;
	}
	return -1;
}

int dn200_set_l3l4_filter_info(struct mac_device_info *hw, int idx, int offset,
			       bool clear)
{
	union l3l4_info_t l3l4_info;
	u32 funcid = hw->priv->plat_ex->vf_offset + HW_IS_VF(hw) + 1;

	if (clear) {
		l3l4_info.l3l4_info = 0;
	} else {
		l3l4_info.funcid = funcid;
		l3l4_info.entry_idx = idx;
		l3l4_info.offset = offset;
	}
	writel(l3l4_info.l3l4_info,
	       LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info,
						 l3l4_info) +
	       offset * sizeof(u32));
	return 0;
}

u8 dn200_vf_get_link_status(struct mac_device_info *hw)
{
	return readb(LRAM_MAC_PF_OFFSET(hw) +
		     offsetof(struct dn200_mailbox_info, link_status));
}

void dn200_pf_set_link_status(struct mac_device_info *hw, u8 link_status)
{
	writeb(link_status,
	       LRAM_MAC_PF_OFFSET(hw) + offsetof(struct dn200_mailbox_info,
						 link_status));
}

static void dn200_clear_vf_queue_info(struct dn200_priv *priv)
{
	u8 queue_info_size = sizeof(struct dn200_vf_info) / sizeof(u32);
	int i, k;

	for (i = 0; i < DN200_MAX_VF_NUM; i++) {
		for (k = 0; k < queue_info_size; k++) {
			writel(0,
			       LRAM_QUEUE_PF_OFFSET(priv->hw) +
			       offsetof(struct dn200_sriov_queue_info,
					vf_info[i]) + k * sizeof(u32));
		}
	}
}

static void dn200_set_vf_queue_info(struct dn200_priv *priv)
{
	u8 queue_info_size = sizeof(struct dn200_vf_info) / sizeof(u32);
	int i, k;
	u32 value;

	for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++) {
		for (k = 0; k < queue_info_size; k++) {
			value = *((u32 *) (&priv->plat_ex->pf.vfs[i]) + k);
			writel(value,
			       LRAM_QUEUE_PF_OFFSET(priv->hw) +
			       offsetof(struct dn200_sriov_queue_info,
					vf_info[i]) + k * sizeof(u32));
		}
	}
}

void dn200_get_vf_queue_info(void __iomem *mailbox, struct dn200_vf_info *info,
			     u8 pf_id, u8 funcid)
{
	u8 queue_info_size = sizeof(struct dn200_vf_info) / sizeof(u32);
	int k;
	u32 *value;

	for (k = 0; k < queue_info_size; k++) {
		value = (u32 *) (info) + k;

		*value =
		    readl(mailbox + DN200_SRIOV_QUEUE_OFFSET +
			  pf_id * SRIOV_MAX_SIZE_PER_PF +
			  offsetof(struct dn200_sriov_queue_info,
				   vf_info[funcid]) + k * sizeof(u32));
	}
}

void dn200_set_phy_info(struct mac_device_info *hw,
			struct dn200_sriov_phy_info *info)
{
	u8 phy_info_size = sizeof(struct dn200_sriov_phy_info) / sizeof(u32);
	int k;
	u32 value;

	for (k = 0; k < phy_info_size; k++) {
		value = *((u32 *) (info) + k);
		writel(value,
		       LRAM_MAC_PF_OFFSET(hw) +
		       offsetof(struct dn200_mailbox_info,
				sriov_phy_info) + k * sizeof(u32));
	}
}

void dn200_get_phy_info(struct mac_device_info *hw,
			struct dn200_sriov_phy_info *info)
{
	u8 phy_info_size = sizeof(struct dn200_sriov_phy_info) / sizeof(u32);
	int k;
	u32 *value;

	for (k = 0; k < phy_info_size; k++) {
		value = (u32 *) (info) + k;
		*value =
		    readl(LRAM_MAC_PF_OFFSET(hw) +
			  offsetof(struct dn200_mailbox_info,
				   sriov_phy_info) + k * sizeof(u32));
	}
}

void dn200_sriov_mail_init(struct dn200_priv *priv)
{
	int k;
	int info_size = sizeof(struct dn200_mailbox_info) / sizeof(u32);

	if (!PRIV_IS_VF(priv)) {
		for (k = 0; k < info_size; k++)
			writel(0, LRAM_MAC_PF_OFFSET(priv->hw) + k * sizeof(u32));
	}
}

void dn200_sriov_ver_get(struct dn200_priv *priv, struct dn200_ver *ver_info)
{
	int k;
	int info_size = sizeof(struct dn200_ver) / sizeof(u32);
	u32 *value;

	for (k = 0; k < info_size; k++) {
		value = ((u32 *) (ver_info) + k);
		*value = readl(LRAM_VER_PF_OFFSET(priv->hw) +
				offsetof(struct dn200_sriov_ver_info,
				   dn200_ver_info) + k * sizeof(u32));
	}
}

void dn200_sriov_ver_set(struct dn200_priv *priv, struct dn200_ver *ver_info)
{
	int k;
	int info_size = sizeof(struct dn200_ver) / sizeof(u32);
	u32 value;

	for (k = 0; k < info_size; k++) {
		value = *((u32 *) (ver_info) + k);
		writel(value, LRAM_VER_PF_OFFSET(priv->hw) +
				offsetof(struct dn200_sriov_ver_info,
				   dn200_ver_info) + k * sizeof(u32));
	}
}

void dn200_sriov_static_init(struct dn200_priv *priv)
{
	int k;
	int info_size = sizeof(struct dn200_sriov_mbx_info) / sizeof(u32);

	if (!PRIV_IS_VF(priv)) {
		for (k = 0; k < info_size; k++)
			writel(0, LRAM_MBX_PF_OFFSET(priv->hw) + k * sizeof(u32));

		DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 0);
		DN200_SET_LRAM_UPGRADE_PF_FINISH(priv->hw, 0, priv->plat_ex->pf_id);
	}
}

static int dn200_assign_vf_resources(struct dn200_priv *priv, int num_vfs)
{
	u8 iatu_per_vf;
	int free_iatu;
	u8 pf_id = priv->plat_ex->pf_id;
	int num_txq, num_rxq;
	int i = 0;
	struct plat_dn200_data *plat_ex = priv->plat_ex;
	struct dn200_vf_info *vfs;
	u8 qps_per_vf;
	u8 tx_queue_num =
	    plat_ex->tx_queues_total - plat_ex->tx_queues_reserved;
	u8 rx_queue_num =
	    plat_ex->rx_queues_total - plat_ex->rx_queues_reserved;
	u8 queue_num = min(tx_queue_num, rx_queue_num);

	vfs = kcalloc(num_vfs, sizeof(struct dn200_vf_info), GFP_KERNEL);
	if (!vfs)
		return -ENOMEM;

	priv->plat_ex->pf.vfs = vfs;
	num_txq = plat_ex->tx_queues_total - queue_num;
	num_rxq = plat_ex->rx_queues_total - queue_num;
	qps_per_vf = min_t(int, num_txq / num_vfs, num_rxq / num_vfs);
	qps_per_vf = min_t(int, DN200_MAX_QPS_PER_VF, qps_per_vf);
	priv->plat_ex->pf.registered_vfs = num_vfs;
	/*vlan 0 reserved, can not be use */
	iatu_per_vf = priv->plat_ex->vf_total_iatu[pf_id] / num_vfs;
	free_iatu = priv->plat_ex->vf_total_iatu[pf_id] % num_vfs;
	for (i = 0; i < num_vfs; i++) {
		priv->plat_ex->pf.vfs[i].rx_queue_start =
		    queue_num + i * qps_per_vf;
		priv->plat_ex->pf.vfs[i].tx_queue_start =
		    queue_num + i * qps_per_vf;
		priv->plat_ex->pf.vfs[i].rx_queues_num = qps_per_vf;
		priv->plat_ex->pf.vfs[i].tx_queues_num = qps_per_vf;
		priv->plat_ex->pf.vfs[i].max_vfs = plat_ex->max_vfs;
		priv->plat_ex->pf.vfs[i].registered_vfs = num_vfs;
		priv->plat_ex->pf.vfs[i].iatu_num = iatu_per_vf;
		priv->plat_ex->pf.vfs[i].max_vlan_num = 0;
		if (free_iatu-- > 0)
			priv->plat_ex->pf.vfs[i].iatu_num++;
	}

	dn200_set_vf_queue_info(priv);
	dn200_sriov_reconfig_hw_feature(priv, &priv->dma_cap);
	return 0;
}

static int dn200_free_vf_resources(struct dn200_priv *priv)
{
	dn200_clear_vf_queue_info(priv);
	kfree(priv->plat_ex->pf.vfs);
	priv->plat_ex->pf.vfs = NULL;
	priv->plat_ex->pf.registered_vfs = 0;
	dn200_sriov_reconfig_hw_feature(priv, &priv->dma_cap);
	return 0;
}

int dn200_datapath_close(struct dn200_priv *priv)
{
	int ret = 0;
	u8 flow_state;

	/* if netdev is not running, no need to close dataptath */
	if (!netif_running(priv->dev))
		return 0;

	/* stop mac rx */
	dn200_mac_rx_set(priv, priv->ioaddr, false);
	/* clean all tx & rx queues */
	ret = dn200_clean_all_tx_queues(priv, priv->plat->tx_queues_to_use);
	if (PRIV_IS_VF(priv)) {
		/* pf to vf, rx clean is no need*/
		DN200_ITR_SYNC_GET(priv->hw, vf_flow_state_event,
				DN200_VF_OFFSET_GET(priv->hw), &flow_state);

		if (flow_state == FLOW_CLOSE_START)
			return 0;
		if (test_bit(DN200_VF_FLOW_CLOSE, &priv->state))
			return 0;
	}
	if (ret == 0)
		ret = dn200_clean_all_rx_queues(priv);
	/* clear tx queues or mtl fifo */
	return ret;
}

void dn200_datapath_open(struct dn200_priv *priv)
{
	/* if netdev is not running, no need to open dataptath */
	if (!netif_running(priv->dev))
		return;

	if (priv->plat_ex->phy_info->link_status)
		netif_carrier_on(priv->dev);
	netif_tx_start_all_queues(priv->dev);

	/* start mac rx */
	dn200_mac_rx_set(priv, priv->ioaddr, true);
}

int dn200_sriov_enable(struct dn200_priv *priv, int num_vfs)
{
	int rc = 0;
	bool dev_running = netif_running(priv->dev);

	if (num_vfs > priv->plat_ex->pf.max_vfs) {
		netdev_err(priv->dev,
			   "Can't enable %d VFs, max VFs supported is %d\n",
			   num_vfs, priv->plat_ex->pf.max_vfs);
		return -EOPNOTSUPP;
	}

	netdev_info(priv->dev, "Enabling %d VFs\n", num_vfs);
	/* close tx & rx datapath before assign resources for vfs that will cause stop flow */
	if (dev_running)
		dn200_datapath_close(priv);

	rc = dn200_assign_vf_resources(priv, num_vfs);
	if (rc)
		goto dev_open;

	rc = pci_enable_sriov(to_pci_dev(priv->device), num_vfs);
	if (rc)
		goto err_out1;
	priv->plat_ex->pf.active_vfs = num_vfs;
	priv->plat_ex->sriov_cfg = true;

	goto dev_open;

err_out1:
	priv->plat_ex->pf.registered_vfs = 0;
	dn200_free_vf_resources(priv);
dev_open:
	if (dev_running)
		dn200_datapath_open(priv);

	return rc;
}

void dn200_vf_flow_close(struct dn200_priv *priv)
{
	if (!priv->plat_ex->sriov_cfg)
		return;
	dn200_all_vf_flow_state_clear(priv);
	dn200_vf_flow_state_set(priv, FLOW_CLOSE_START);
	dn200_all_vf_flow_state_wait(priv, FLOW_CLOSE_DONE, false);
}

void _dn200_vf_flow_open(struct dn200_priv *priv)
{
	set_bit(DN200_VF_FLOW_OPEN, &priv->state);
	queue_work(priv->wq, &priv->service_task);
}

void dn200_vf_flow_open(struct dn200_priv *priv)
{
	if (!priv->plat_ex->sriov_cfg)
		return;
	dn200_all_vf_flow_state_clear(priv);
	dn200_vf_flow_state_set(priv, FLOW_OPEN_START);
	dn200_all_vf_flow_state_wait(priv, FLOW_OPEN_DONE, true);
	clear_bit(DN200_VF_FLOW_OPEN, &priv->state);
}

int dn200_sriov_disable(struct dn200_priv *priv)
{
	u16 num_vfs = pci_num_vf(to_pci_dev(priv->device));

	if (!num_vfs)
		return 0;
	if (pci_vfs_assigned(to_pci_dev(priv->device))) {
		netdev_warn(priv->dev,
			    "Unable to free %d VFs because some are assigned to VMs.\n",
			    num_vfs);
		return -EPERM;
	}
	/* wait all vfs to stop dev before disable sriov */
	pci_disable_sriov(to_pci_dev(priv->device));

	/* close datapath before free resources that will cause stop flow */
	dn200_datapath_close(priv);
	dn200_free_vf_resources(priv);
	/* open datapath */
	dn200_datapath_open(priv);
	priv->plat_ex->sriov_cfg = false;
	return 0;
}

int dn200_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	struct net_device *dev = pci_get_drvdata(pdev);
	struct dn200_priv *priv = netdev_priv(dev);
	int ret = 0;
	u32 flag = 0;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	if (!PRIV_SRIOV_SUPPORT(priv)) {
		netdev_err(dev,
			   "Reject SRIOV config request since device is not supported!\n");
		return -EOPNOTSUPP;
	}

	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, &flag);
	if (flag) {
		netdev_err(dev,
			   "Reject echo vfs after update fw!\n");
		return -EOPNOTSUPP;
	}

	priv->plat_ex->vf_flag = true;
	priv->plat_ex->pf.max_vfs = priv->plat_ex->max_vfs;
	if (pci_vfs_assigned(pdev)) {
		netdev_warn(dev,
			    "Unable to configure SRIOV since some VFs are assigned to VMs.\n");
		priv->plat_ex->vf_flag = false;
		return -EPERM;
	}

	/* if there are previous existing VFs, clean them up */
	dn200_sriov_disable(priv);
	if (!num_vfs)
		goto sriov_cfg_exit;

	ret = dn200_sriov_enable(priv, num_vfs);
	if (ret) {
		netdev_err(dev, "Sriov enable failed, error %d\n", ret);
		priv->plat_ex->vf_flag = false;
		return ret;
	}

sriov_cfg_exit:
	priv->plat_ex->vf_flag = false;
	return num_vfs;
}

int dn200_vf_glb_err_rst_notify(struct dn200_priv *priv)
{
	if (!PRIV_IS_VF(priv))
		return -EINVAL;

	/* for sriov vf noitfy pf */
	DN200_ITR_SYNC_SET(priv->hw, reset_event, VF2PF_ERR_RST_NOTIFY, 1);
	priv->xstats.rst_start_count++;
	dev_dbg(priv->device, "%s, %d, vf to pf err rst request.\n",
		__func__, __LINE__);

	DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, HW_RESET_ID, 1);
	irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
	return 0;
}

int dn200_pf_glb_err_rst_process(struct dn200_priv *priv)
{
	int ret = 0;
	u8 reset_event = 0;

	if (PRIV_IS_VF(priv))
		return 0;

	/* pf process the err reset request from vf */
	if (PRIV_SRIOV_SUPPORT(priv)) {
		DN200_ITR_SYNC_GET(priv->hw, reset_event, VF2PF_ERR_RST_NOTIFY,
				   &reset_event);
		if (reset_event) {
			priv->xstats.rst_start_accept_count++;
			dev_dbg(priv->device,
				"%s, %d, pf received vf err rst request.\n",
				__func__, __LINE__);
			dn200_global_err(priv, DN200_VF_TO_PF);
			/* received vf to pf err reset request, clear it */
			DN200_ITR_SYNC_SET(priv->hw, reset_event,
					   VF2PF_ERR_RST_NOTIFY, 0);
		}
	}

	return ret;
}

void dn200_vf_flow_state_set(struct dn200_priv *priv, u8 flow_sate)
{
	int i = 0;

	for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++)
		DN200_ITR_SYNC_SET(priv->hw, vf_flow_state_event, i, flow_sate);

	DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, FLOW_STATE_ID, 1);

	irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
}

bool dn200_all_vf_flow_state_wait(struct dn200_priv *priv, u8 wait_state, bool in_task)
{
	int i, retries = 20;	/* max retry 10times, 1s */
	u8 vf_flow_state = 0;
	u8 reg_info;
	bool all_fun_state;

	while (retries-- > 0) {
		all_fun_state = true;
		for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++) {
			DN200_HEARTBEAT_GET(priv->hw, registered_vf_state, i,
					    &reg_info);
			if (!((reg_info & DN200_VF_REG_STATE_OPENED)
			     || (reg_info & DN200_VF_REG_STATE_IN_RST))) {
				/*vf not register now, skip it */
				continue;
			}
			DN200_ITR_SYNC_GET(priv->hw, vf_flow_state_event, i,
					   &vf_flow_state);
			dev_dbg(priv->device,
				"%s, %d, retry %d, vf:%d, vf_flow_state:%d, wait_state:%d, reg_info:%d",
				__func__, __LINE__, 20 - retries, i,
				vf_flow_state, wait_state, reg_info);
			if (vf_flow_state != wait_state) {
				all_fun_state = false;
				break;
			}
		}

		if (all_fun_state) {
			dev_dbg(priv->device,
				"%s, %d, retry %d to wait all vfs flow state:%d",
				__func__, __LINE__, 20 - retries,
				vf_flow_state);
			return true;
		}

		/* Sleep then retry */
		if (in_task)
			usleep_range(100000, 110000);
		else
			msleep(100);
	}
	dev_err(priv->device,
		"%s, %d, retry timeout to wait all vfs flow state", __func__,
		__LINE__);
	return false;
}

void dn200_all_vf_flow_state_clear(struct dn200_priv *priv)
{
	int i;

	for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++)
		DN200_ITR_SYNC_SET(priv->hw, vf_flow_state_event, i, 0);

	DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, FLOW_STATE_ID, 0);
}
