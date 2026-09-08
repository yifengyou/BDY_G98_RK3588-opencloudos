/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_LAN_H
#define _YK3_LAN_H

#include "yk3.h"

int yk3_lan_init(struct yk3_pdev_priv *pdev_priv);
void yk3_lan_exit(struct yk3_pdev_priv *pdev_priv);
void yk3_lan_sriov_init(struct yk3_pdev_priv *pdev_priv, bool enable);

int yk3_lan_ndev_init(struct net_device *ndev);
void yk3_lan_ndev_exit(struct net_device *ndev);
int yk3_lan_ndev_set_mtu(struct net_device *ndev, int new_mtu);
int yk3_lan_ndev_set_mac(struct net_device *ndev, u8 *old_mac, u8 *mac);
int yk3_lan_ndev_set_rx_mode(struct net_device *ndev, u32 changed, u32 flags);
int yk3_lan_ndev_set_feature(struct net_device *ndev, netdev_features_t changed,
			     netdev_features_t features);
int yk3_lan_ndev_set_vf_mac(struct net_device *ndev, int vf, u16 vf_qset,
			    u8 *old_mac, u8 *mac);
int yk3_lan_ndev_set_vf_vlan(struct net_device *ndev, u16 vf_qset, u16 vlan,
			     u8 qos, __be16 proto);
int yk3_lan_ndev_set_vf_trust(struct net_device *ndev, int vf, bool trust);
int yk3_lan_ndev_set_vf_spoofchk(struct net_device *ndev, u16 vf_qset, bool check);
int yk3_lan_ndev_vlan(struct net_device *ndev, __be16 proto, u16 vlan_id, bool enable);
int yk3_lan_ndev_sync_mc(struct net_device *ndev, const u8 *addr);
int yk3_lan_ndev_sync_uc(struct net_device *ndev, const u8 *addr);
int yk3_lan_ndev_unsync_mc(struct net_device *ndev, const u8 *addr);
int yk3_lan_ndev_unsync_uc(struct net_device *ndev, const u8 *addr);
void yk3_lan_ndev_set_evb_mode(struct net_device *ndev, u16 mode);
int yk3_lan_ethtool_set_rxnfc(struct net_device *ndev, const u32 flow_type, const u8 fields);
int yk3_lan_ethtool_set_priv_flags(struct net_device *ndev, const u32 flags, bool enable);
int yk3_lan_et_get_sset_count(struct yk3_ndev_priv *ndev_priv);
void yk3_lan_et_get_stats_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail);
void yk3_lan_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data);

int yk3_lan_umd_set_pvid(struct yk3_ndev_priv *ndev_priv, u16 pvid, __be16 proto, u8 qos,
			 bool tx_only);
int yk3_lan_umd_set_spoofchk(struct yk3_ndev_priv *ndev_priv, bool enable);
int yk3_lan_umd_restore_vf_conf(struct yk3_ndev_priv *ndev_priv);

static inline int yk3_check_dup_vf_mac(struct yk3_ndev_priv *ndev_priv, u8 *mac,
				       u16 vlan, __be16 vlan_proto)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv = pdev_priv->sriov_priv;
	struct yk3_vf_info *vf_info;
	int i;

	if (IS_ERR_OR_NULL(sriov_priv))
		return 0;

	/* check new vf mac is duplicate with pf mac */
	if (ndev_priv->ndev->dev_addr[0] == mac[0] && ndev_priv->ndev->dev_addr[1] == mac[1] &&
	    ndev_priv->ndev->dev_addr[2] == mac[2] && ndev_priv->ndev->dev_addr[3] == mac[3] &&
	    ndev_priv->ndev->dev_addr[4] == mac[4] && ndev_priv->ndev->dev_addr[5] == mac[5])
		return -EEXIST;

	for (i = 0; i < sriov_priv->num_vfs; i++) {
		vf_info = &sriov_priv->vf_info[i];
		if (vf_info->vf_vlan != vlan || vf_info->vf_vlan_tpid != vlan_proto)
			continue;

		if (vf_info->mac_addr[0] == mac[0] && vf_info->mac_addr[1] == mac[1] &&
		    vf_info->mac_addr[2] == mac[2] && vf_info->mac_addr[3] == mac[3] &&
		    vf_info->mac_addr[4] == mac[4] && vf_info->mac_addr[5] == mac[5]) {
			return -EEXIST;
		}
	}

	return 0;
}

static inline int yk3_check_dup_pf_mac(struct yk3_sriov_priv *sriov_priv, u8 *mac)
{
	struct yk3_vf_info *vf_info;
	int i;

	if (IS_ERR_OR_NULL(sriov_priv))
		return 0;

	for (i = 0; i < sriov_priv->num_vfs; i++) {
		vf_info = &sriov_priv->vf_info[i];
		if (vf_info->mac_addr[0] == mac[0] && vf_info->mac_addr[1] == mac[1] &&
		    vf_info->mac_addr[2] == mac[2] && vf_info->mac_addr[3] == mac[3] &&
		    vf_info->mac_addr[4] == mac[4] && vf_info->mac_addr[5] == mac[5]) {
			return -EEXIST;
		}
	}

	return 0;
}

#endif /* _YK3_LAN_H */
