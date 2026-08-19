// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_lan.h"
#include "yk3_np.h"
#include "yk3_emp_priv.h"

struct yk3_ndev_param {
	struct yk3_queuebase qbase[YK3_QUEUE_T_MAX];
	enum yk3_ndev_type type;
};

static void yk3_lro_set_enable(struct yk3_pdev_priv *pdev_priv, bool enable);

static int yk3_ndo_open(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	int ret;

	mutex_lock(&ndev_priv->state_mlock);

	if (ndev_priv->umd_enable) {
		ret = -EBUSY;
		goto err_umd;
	}

	if (!pdev_priv->card->linkup_dbg && yk3_ndev_is_pf(ndev_priv)) {
		ret = yk3_mac_ndev_set_phy_state(ndev_priv, false);
		if (ret) {
			yk3_net_err("Set mac phy state failed: %d", ret);
			goto err_set_phy_state;
		}
	}

	ret = yk3_edma_start(ndev_priv, txqnum, rxqnum);
	if (ret) {
		yk3_net_err("edma_start failed: %d", ret);
		goto err_edma_start;
	}

	ndev_priv->txq_real_num = txqnum;
	ndev_priv->rxq_real_num = rxqnum;
	netif_tx_start_all_queues(ndev);
	netif_device_attach(ndev);
	netif_tx_schedule_all(ndev);
	yk3_mac_ndev_start(ndev_priv);
	__dev_uc_sync(ndev, yk3_lan_ndev_sync_uc, yk3_lan_ndev_unsync_uc);
	__dev_mc_sync(ndev, yk3_lan_ndev_sync_mc, yk3_lan_ndev_unsync_mc);
	mutex_unlock(&ndev_priv->state_mlock);

	return 0;

err_edma_start:
err_set_phy_state:
err_umd:
	mutex_unlock(&ndev_priv->state_mlock);
	return ret;
}

static int yk3_ndo_stop(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret = 0;

	mutex_lock(&ndev_priv->state_mlock);
	ndev_priv->txq_real_num = 0;
	ndev_priv->rxq_real_num = 0;
	netif_tx_disable(ndev);
	yk3_edma_stop(ndev_priv);
	if (yk3_ndev_is_pf(ndev_priv)) {
		if (ndev_priv->ethtool_priv_flags & BIT(YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE)) {
			ret = yk3_mac_ndev_set_phy_state(ndev_priv, true);
			if (ret) {
				mutex_unlock(&ndev_priv->state_mlock);
				yk3_net_err("Set mac phy state failed: %d", ret);
				return -EFAULT;
			}
		}
	}
	__dev_uc_unsync(ndev, yk3_lan_ndev_unsync_uc);
	__dev_mc_unsync(ndev, yk3_lan_ndev_unsync_mc);
	mutex_unlock(&ndev_priv->state_mlock);

	return 0;
}

static netdev_tx_t
yk3_ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	return yk3_edma_start_xmit(skb, ndev);
}

static int yk3_ndo_set_mac(struct net_device *ndev, void *addr)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_sriov_priv *sriov_priv;
	struct sockaddr *saddr = addr;
	u8 old_dev_addr[ETH_ALEN];
	u8 dev_addr[ETH_ALEN];
	int ret;

	if (ether_addr_equal(ndev->dev_addr, saddr->sa_data) &&
	    ether_addr_equal(ndev_priv->intf_mac, saddr->sa_data))
		return 0;

	pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	memcpy(dev_addr, saddr->sa_data, ETH_ALEN);
	memcpy(old_dev_addr, ndev_priv->intf_mac, ETH_ALEN);

	if (!is_valid_ether_addr(dev_addr))
		return -EADDRNOTAVAIL;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (sriov_priv) {
		if (yk3_check_dup_pf_mac(sriov_priv, dev_addr) == -EEXIST) {
			yk3_err("PF recv set mac msg duplicate mac addr with other VF!\n");
			yk3_err("exist mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
				dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3],
				dev_addr[4], dev_addr[5]);
			yk3_sriov_put_priv(sriov_priv);
			return -EADDRINUSE;
		}
		yk3_sriov_put_priv(sriov_priv);
	}

	ret = yk3_lan_ndev_set_mac(ndev, old_dev_addr, dev_addr);
	if (ret != 0) {
		if (ret == -EEXIST) {
			ret = -EADDRINUSE;
			yk3_net_err("Set MAC address duplicate with other PF/VF!");
		} else {
			yk3_net_err("Set MAC address failed with error: %d", ret);
			ret = -EFAULT;
		}
		return ret;
	}

	eth_hw_addr_set(ndev, dev_addr);
	memcpy(ndev_priv->intf_mac, dev_addr, ETH_ALEN);

	if (yk3_set_pause_srcaddr(pdev_priv, dev_addr))
		yk3_dev_warn("Failed to set pause srcaddr.\n");

	yk3_dev_info("Set MAC address to %02x:%02x:%02x:%02x:%02x:%02x",
		     dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3],
		     dev_addr[4], dev_addr[5]);

	return 0;
}

static int yk3_ndo_change_mtu(struct net_device *ndev, int new_mtu)
{
	int ret;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	bool running = netif_running(ndev);

#ifdef YK3_HAVE_MAX_MTU
	if (new_mtu < ndev->min_mtu || new_mtu > ndev->max_mtu) {
		yk3_net_err("Bad MTU: %d", new_mtu);
		return -EPERM;
	}
#else
	if (new_mtu < ndev->extended->min_mtu || new_mtu > ndev->extended->max_mtu) {
		yk3_net_err("Bad MTU: %d", new_mtu);
		return -EPERM;
	}
#endif /* YK3_HAVE_MAX_MTU */

	if (running && ndev->netdev_ops->ndo_stop) {
		ndev->netdev_ops->ndo_stop(ndev);
		ndev->flags &= ~IFF_UP;
	}

	mutex_lock(&ndev_priv->state_mlock);
	ret = yk3_lan_ndev_set_mtu(ndev, new_mtu < YK3_N_RX_MIN_MTU ?
				   YK3_N_RX_MIN_MTU : new_mtu);
	mutex_unlock(&ndev_priv->state_mlock);
	if (ret) {
		yk3_net_err("set_mtu failed: %d", ret);
		return ret;
	}

	ndev->mtu = new_mtu;
	if (running && ndev->netdev_ops->ndo_open) {
		ret = ndev->netdev_ops->ndo_open(ndev);
		ndev->flags |= IFF_UP;
	}

	return 0;
}

static void yk3_ndo_get_stats64(struct net_device *ndev, struct rtnl_link_stats64 *stats)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (!(ndev->flags & IFF_UP))
		return;

	spin_lock_bh(&ndev_priv->statistics_lock);
	yk3_edma_update_stat(ndev_priv);
	netdev_stats_to_stats64(stats, &ndev->stats);
	spin_unlock_bh(&ndev_priv->statistics_lock);
}

static void yk3_ndo_change_rx_flags(struct net_device *ndev, int flags)
{
}

static void yk3_ndo_set_rx_mode(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u32 changed = 0;
	int ret;

	changed = ndev_priv->netdev_flags ^ ndev->flags;
	ret = yk3_lan_ndev_set_rx_mode(ndev, changed, ndev->flags);
	if (ret)
		yk3_net_err("lan set_rx_mode failed: %d", ret);
	ndev_priv->netdev_flags = ndev->flags;
}

static int yk3_ndo_set_features(struct net_device *ndev, netdev_features_t features)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	netdev_features_t changed;
	int ret;

	changed = ndev->features ^ features;

	if (!!(changed & NETIF_F_LRO) && yk3_ndev_is_pf(ndev_priv)) {
		mutex_lock(&ndev_priv->state_mlock);
		if (ndev_priv->lro_enable) {
			yk3_net_info("lro disable");
			//yk3_lro_set_enable(ndev_priv->pdev_priv, false);
			ndev_priv->lro_enable = false;
		} else {
			yk3_net_info("lro enable");
			//yk3_lro_set_enable(ndev_priv->pdev_priv, true);
			ndev_priv->lro_enable = true;
		}
		mutex_unlock(&ndev_priv->state_mlock);
	}

	ret = yk3_lan_ndev_set_feature(ndev, changed, features);

	return ret;
}

static netdev_features_t
yk3_ndo_fix_features(struct net_device *ndev, netdev_features_t features)
{
	if (features & NETIF_F_GSO_PARTIAL) {
		ndev->gso_partial_features = NETIF_F_GSO_ENCAP_ALL;
		ndev->gso_partial_features |= NETIF_F_GSO_UDP_L4;
	}

	if (!(features & NETIF_F_GSO_PARTIAL))
		ndev->gso_partial_features = 0;

	if ((features & NETIF_F_GSO_UDP_L4) && !(features & NETIF_F_HW_CSUM))
		features &= ~NETIF_F_GSO_UDP_L4;

	return features;
}

static int yk3_ndo_set_vf_mac(struct net_device *ndev, int vf, u8 *mac)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	int ret;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if (!is_valid_ether_addr(mac)) {
		yk3_net_err("Invalid MAC address for VF %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EADDRNOTAVAIL;
	}

	if (yk3_check_dup_vf_mac(ndev_priv, mac, vf_info->vf_vlan,
				 vf_info->vf_vlan_tpid) == -EEXIST) {
		yk3_err("PF recv vf %d set mac but address duplicate with other PF/VF!\n",
			vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EADDRINUSE;
	}

	ret = yk3_lan_ndev_set_vf_mac(ndev, vf, vf_info->qsetid,
				      vf_info->mac_addr,
				      mac);
	if (ret) {
		yk3_net_err("Set VF %d mac failed: %d", vf, ret);
		yk3_sriov_put_priv(sriov_priv);
		return ret;
	}
	memcpy(vf_info->mac_addr, mac, ETH_ALEN);
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

/**
 * yk3_ndo_set_vf_rate - set min/max VF bandwidth
 * @ndev: network interface device structure
 * @vf: VF identifier
 * @minrate: Minimum Tx rate in Mbps
 * @maxrate: Maximum Tx rate in Mbps
 */
static int
yk3_ndo_set_vf_rate(struct net_device *ndev, int vf, int minrate, int maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	int ret;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if (minrate > maxrate) {
		yk3_net_err("minrate %d bigger than maxrate:%d for the vf %d\n",
			    minrate, maxrate, vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if (maxrate > ndev_priv->link_speed) {
		yk3_net_err("Invalid maxrate %d specified for the vf %d\n",
			    maxrate, vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	vf_info->min_tx_rate = minrate;
	vf_info->max_tx_rate = maxrate;
	ret = yk3_qos_set_vf_rate(ndev, vf, minrate, maxrate);
	if (ret) {
		yk3_net_err("Set VF %d rate failed: %d", vf, ret);
		yk3_sriov_put_priv(sriov_priv);
		return -EFAULT;
	}
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

#ifdef YK3_HAVE_NDO_SET_TX_MAXRATE
/**
 * yk3_ndo_set_tx_maxrate - NDO callback to set the maximum per-queue bitrate
 * @ndev: network interface device structure
 * @queue_index: Queue ID
 * @maxrate: maximum bandwidth in Mbps
 */
static int
yk3_ndo_set_tx_maxrate(struct net_device *ndev, int queue_index, u32 maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (maxrate > ndev_priv->link_speed) {
		yk3_net_err("Invalid max rate %d specified for the queue %d\n",
			    maxrate, queue_index);
		return -EINVAL;
	}

	return yk3_qos_set_queue_rate(ndev, queue_index, maxrate);
}
#endif

#ifdef YK3_HAVE_SET_BRIDGE_LINK_NL_EXT_ACK
static int yk3_ndo_bridge_setlink(struct net_device *ndev, struct nlmsghdr *nlh,
				  u16 flags, struct netlink_ext_ack *extack)
#else
static int yk3_ndo_bridge_setlink(struct net_device *ndev, struct nlmsghdr *nlh,
				  u16 flags)
#endif
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_sriov_priv *sriov_priv;
	struct nlattr *attr, *br_spec;
	int rem;
	u16 mode = BRIDGE_MODE_UNDEF;

	if (!yk3_pdev_is_pf(pdev_priv) ||
	    pdev_priv->card->mode != YK3_MODE_TCARD)
		return -EOPNOTSUPP;

	if (IS_ERR_OR_NULL(pdev_priv->sriov_priv))
		return -EOPNOTSUPP;

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);
	if (!br_spec)
		return -EINVAL;

	nla_for_each_nested(attr, br_spec, rem) {
		if (nla_type(attr) != IFLA_BRIDGE_MODE)
			continue;
		if (nla_len(attr) < sizeof(mode))
			return -EINVAL;
		mode = nla_get_u16(attr);
		if (mode != BRIDGE_MODE_VEPA && mode != BRIDGE_MODE_VEB)
			return -EINVAL;
		break;
	}

	if (mode == pdev_priv->evb_mode)
		return 0;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	yk3_lan_ndev_set_evb_mode(ndev, mode);
	pdev_priv->evb_mode = mode;
	if (sriov_priv)
		yk3_sriov_put_priv(sriov_priv);

	return 0;
}

static int yk3_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
				  struct net_device *ndev, u32 filter_mask,
				  int nlflags)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!yk3_pdev_is_pf(pdev_priv) ||
	    pdev_priv->card->mode != YK3_MODE_TCARD)
		return -EOPNOTSUPP;

	if (IS_ERR_OR_NULL(pdev_priv->sriov_priv))
		return -EOPNOTSUPP;

	return ndo_dflt_bridge_getlink(skb, pid, seq, ndev,
				       pdev_priv->evb_mode,
				       0, 0, nlflags, filter_mask, NULL);
}

static int yk3_ndo_set_vf_vlan(struct net_device *ndev, int vf, u16 vlan, u8 qos,
			       __be16 proto)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	int ret;

	if (pdev_priv->card->mode != YK3_MODE_TCARD &&
	    pdev_priv->card->mode != YK3_MODE_RCARD)
		return -EOPNOTSUPP;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if (proto != htons(ETH_P_8021Q) && proto != htons(ETH_P_8021AD)) {
		yk3_sriov_put_priv(sriov_priv);
		return -EPROTONOSUPPORT;
	}

	if (vlan == 0 && vf_info->vf_vlan == 0) {
		yk3_sriov_put_priv(sriov_priv);
		return 0;
	}

	if (vlan == 0)
		proto = 0;

	if (qos == vf_info->vf_vlan_qos &&
	    yk3_check_dup_vf_mac(ndev_priv, vf_info->mac_addr, vlan, proto) == -EEXIST) {
		yk3_err("PF set vf vlan but mac addr duplicate with other VF!\n");
		yk3_err("exist mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
			vf_info->mac_addr[0], vf_info->mac_addr[1], vf_info->mac_addr[2],
			vf_info->mac_addr[3], vf_info->mac_addr[4], vf_info->mac_addr[5]);
		yk3_sriov_put_priv(sriov_priv);
		return -EADDRINUSE;
	}

	vf_info->vf_vlan = vlan;
	if (vlan == 0) {
		vf_info->vf_vlan_tpid = 0;
		vf_info->vf_vlan_qos = 0;
	} else {
		vf_info->vf_vlan_tpid = proto;
		vf_info->vf_vlan_qos = qos;
	}

	ret = yk3_lan_ndev_set_vf_vlan(ndev, vf_info->qsetid, vlan, qos, proto);
	if (ret) {
		yk3_net_err("Set vf vlan failed: %d", ret);
		yk3_sriov_put_priv(sriov_priv);
		return -EFAULT;
	}
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

static int yk3_ndo_get_vf_config(struct net_device *ndev, int vf,
				 struct ifla_vf_info *ivf)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	ivf->vf = vf;
	memcpy(&ivf->mac, vf_info->mac_addr, ETH_ALEN);
	ivf->vlan = vf_info->vf_vlan;
	ivf->qos = vf_info->vf_vlan_qos;
	ivf->vlan_proto = vf_info->vf_vlan_tpid;
	ivf->trusted = vf_info->trusted;
	ivf->spoofchk = vf_info->spoofchk;
	ivf->min_tx_rate = vf_info->min_tx_rate;
	ivf->max_tx_rate = vf_info->max_tx_rate;
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

static int yk3_ndo_vlan_rx_add_vid(struct net_device *ndev,
				   __be16 proto, u16 vlan_id)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct list_head *vlan_list;
	struct yk3_vlan *vlan_node, *temp;
	int ret;

	if (vlan_id == 0)
		return 0;

	if (proto == htons(ETH_P_8021Q)) {
		vlan_list = &ndev_priv->cvlan_list;
	} else if (proto == htons(ETH_P_8021AD)) {
		vlan_list = &ndev_priv->svlan_list;
	} else {
		yk3_net_warn("proto only support ETH_P_8021Q or ETH_P_8021AD\n");
		return -EOPNOTSUPP;
	}

	list_for_each_entry_safe(vlan_node, temp, vlan_list, list) {
		if (vlan_node->vlan_id == vlan_id)
			return 0;
	}
	vlan_node = kzalloc(sizeof(*vlan_node), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(vlan_node))
		return -ENOMEM;
	vlan_node->vlan_id = vlan_id;
	list_add(&vlan_node->list, vlan_list);

	ret = yk3_lan_ndev_vlan(ndev, proto, vlan_id, true);
	if (ret) {
		list_del(&vlan_node->list);
		kfree(vlan_node);
		yk3_net_err("lan add vlan id %d failed: %d", vlan_id, ret);
		return -EFAULT;
	}

	return 0;
}

static int yk3_ndo_vlan_rx_kill_vid(struct net_device *ndev,
				    __be16 proto, u16 vlan_id)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct list_head *vlan_list;
	struct yk3_vlan *vlan_node, *temp;
	bool found = false;
	int ret;

	if (vlan_id == 0)
		return 0;

	if (proto == htons(ETH_P_8021Q)) {
		vlan_list = &ndev_priv->cvlan_list;
	} else if (proto == htons(ETH_P_8021AD)) {
		vlan_list = &ndev_priv->svlan_list;
	} else {
		yk3_net_warn("proto only support ETH_P_8021Q or ETH_P_8021AD\n");
		return -EOPNOTSUPP;
	}

	if (list_empty(vlan_list)) {
		yk3_net_warn("remove vlan id %d but vlan list empty return directly!",
			     vlan_id);
		return 0;
	}

	list_for_each_entry_safe(vlan_node, temp, vlan_list, list) {
		if (vlan_node->vlan_id == vlan_id) {
			list_del(&vlan_node->list);
			kfree(vlan_node);
			vlan_node = NULL;
			found = true;
		}
	}

	if (!found) {
		yk3_net_warn("remove vlan id %d but not found in list!", vlan_id);
		return 0;
	}

	ret = yk3_lan_ndev_vlan(ndev, proto, vlan_id, false);
	if (ret) {
		yk3_net_err("lan del vlan id %d failed: %d", vlan_id, ret);
		return -EFAULT;
	}

	return 0;
}

#ifdef YK3_HAVE_NDO_SETUP_TC
static int yk3_ndo_setup_tc(struct net_device *ndev, enum tc_setup_type type,
			    void *type_data)
{
	int ret = 0;

	switch (type) {
#ifdef YK3_MQPRIO_OFFLOAD
	case TC_SETUP_QDISC_MQPRIO:
		ret = yk3_qos_setup_tc_mqprio_qdisc(ndev, type_data);
		break;
#endif
	default:
		return -EOPNOTSUPP;
	}

	return ret;
}
#endif

static int yk3_ndo_set_vf_trust(struct net_device *ndev, int vf, bool setting)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	int ret;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if ((setting && vf_info->trusted == 1) ||
	    (!setting && vf_info->trusted == 0)) {
		yk3_sriov_put_priv(sriov_priv);
		return 0;
	}

	if (setting)
		vf_info->trusted = 1;
	else
		vf_info->trusted = 0;

	ret = yk3_lan_ndev_set_vf_trust(ndev, vf, setting);
	if (ret) {
		yk3_net_err("Set vf %d trust failed: %d", vf, ret);
		yk3_sriov_put_priv(sriov_priv);
		return -EFAULT;
	}
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
static netdev_features_t yk3_tunnel_features_check(struct yk3_ndev_priv *ndev_priv,
						   struct sk_buff *skb,
						   netdev_features_t features)
{
	unsigned int offset = 0;
	struct udphdr *udph;
	u8 proto;
	u16 port;

	switch (vlan_get_protocol(skb)) {
	case htons(ETH_P_IP):
		if (!pskb_may_pull(skb, sizeof(struct iphdr)))
			goto out;
		proto = ip_hdr(skb)->protocol;
		break;
	case htons(ETH_P_IPV6):
		if (!pskb_may_pull(skb, sizeof(struct ipv6hdr)))
			goto out;
		proto = ipv6_find_hdr(skb, &offset, -1, NULL, NULL);
		break;
	default:
		goto out;
	}

	switch (proto) {
	case IPPROTO_GRE:
	case IPPROTO_IPIP:
	case IPPROTO_IPV6:
		return features;
	case IPPROTO_UDP:
		if (!pskb_may_pull(skb, sizeof(struct udphdr)))
			goto out;

		udph = udp_hdr(skb);
		port = be16_to_cpu(udph->dest);

		/* Support Geneve offload for default UDP port */
		if (ndev_priv->udp_tnl_port->geneve_entry[0].port == YK3_GENEVE_DEFAULT_PORT)
			return features;

		if (yk3_vxlan_lookup_port(ndev_priv, port))
			return vxlan_features_check(skb, features);
		break;
	}

out:
	/* Disable CSUM and GSO if skb cannot be offloaded by HW */
	return features & ~(NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK);
}

static netdev_features_t yk3_features_check(struct sk_buff *skb,
					    struct net_device *ndev,
					    netdev_features_t features)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	features = vlan_features_check(skb, features);

	/* Validate if the tunneled packet is being offloaded by HW */
	if (skb->encapsulation &&
	    (features & NETIF_F_CSUM_MASK || features & NETIF_F_GSO_MASK))
		return yk3_tunnel_features_check(ndev_priv, skb, features);

	return features;
}
#endif

#ifdef YK3_HAVE_NDO_UDP_TUNNEL
static void yk3_add_udp_tunnel(struct net_device *ndev,
			       struct udp_tunnel_info *ti)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret = 0;

	if (ti->type != UDP_TUNNEL_TYPE_VXLAN && ti->type != UDP_TUNNEL_TYPE_GENEVE)
		return;

	ret = yk3_udp_tunnel_set(ndev, ti, YK3_SET_UDP_TUNNEL);
	if (ret)
		yk3_net_warn("Failed to add udp tunnel port, err is %d", ret);
}

static void yk3_del_udp_tunnel(struct net_device *ndev,
			       struct udp_tunnel_info *ti)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret = 0;

	if (ti->type != UDP_TUNNEL_TYPE_VXLAN && ti->type != UDP_TUNNEL_TYPE_GENEVE)
		return;

	ret = yk3_udp_tunnel_set(ndev, ti, YK3_UNSET_UDP_TUNNEL);
	if (ret)
		yk3_net_warn("Failed to delete udp tunnel port, err is %d", ret);
}
#endif

static int yk3_ndo_set_vf_link_state(struct net_device *ndev,
				     int vf, int link_state)
{
	return -EOPNOTSUPP;
}

static int yk3_ndo_set_vf_spoofchk(struct net_device *ndev, int vf, bool setting)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	int ret;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv))
		return -EOPNOTSUPP;

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_net_err("Invalid VF index %d\n", vf);
		yk3_sriov_put_priv(sriov_priv);
		return -EINVAL;
	}

	if ((setting && vf_info->spoofchk == 1) ||
	    (!setting && vf_info->spoofchk == 0)) {
		yk3_sriov_put_priv(sriov_priv);
		return 0;
	}

	if (setting)
		vf_info->spoofchk = 1;
	else
		vf_info->spoofchk = 0;

	ret = yk3_lan_ndev_set_vf_spoofchk(ndev, vf_info->qsetid, setting);
	if (ret) {
		yk3_net_err("Set vf %d spoofcheck failed: %d", vf, ret);
		yk3_sriov_put_priv(sriov_priv);
		return -EFAULT;
	}
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

#ifndef YK3_HAVE_SELECT_QUEUE_RH310
#ifdef CONFIG_DCB
static u8 yk3_get_dscp_up(struct yk3_ndev_priv *ndev_priv, struct sk_buff *skb)
{
	u8 dscp = 0;

	if (!ndev_priv || !ndev_priv->dcbx)
		return 0;

	if (skb->protocol == htons(ETH_P_IP))
		dscp = ipv4_get_dsfield(ip_hdr(skb)) >> 2;
	else if (skb->protocol == htons(ETH_P_IPV6))
		dscp = ipv6_get_dsfield(ipv6_hdr(skb)) >> 2;

	if (dscp >= YK3_MAX_DSCP)
		dscp = 0;

	return ndev_priv->dcbx->dscp2prio[dscp];
}
#endif

/**
 * yk3_get_up - Get User Priority from packet
 * @ndev: Network device
 * @skb: Socket buffer
 *
 * Priority sources (in order of precedence):
 * 1. DCB DSCP mode (if enabled) - from IP TOS/DSCP field
 * 2. VLAN PCP (if VLAN tagged) - from 802.1p priority
 * 3. skb->priority - from Socket SO_PRIORITY or iptables CLASSIFY
 *
 * Returns: User priority (0-7 for TC mapping)
 */
static u8 yk3_get_up(struct net_device *ndev, struct sk_buff *skb)
{
	u8 prio = 0;

#ifdef CONFIG_DCB
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	/* 1. DCB DSCP mode has highest priority */
	if (ndev_priv->dcbx &&
	    yk3_pdev_is_pf(pdev_priv) &&
	    READ_ONCE(ndev_priv->dcbx->prio) == YK3_PRIO_DSCP) {
		return yk3_get_dscp_up(ndev_priv, skb);
	}
#endif

	/* 2. VLAN PCP (802.1p priority) */
	if (skb_vlan_tag_present(skb)) {
		prio = (skb->vlan_tci & VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
		return prio & 0x07;
	}

	/* 3. skb->priority (Socket SO_PRIORITY, iptables CLASSIFY, etc.) */
	prio = skb->priority;

	return prio;
}

/**
 * yk3_pick_tx - Pick TX queue based on priority and TC mapping
 * @ndev: Network device
 * @skb: Socket buffer
 *
 * Queue selection flow:
 * 1. Get user priority from packet (DSCP/VLAN/skb->priority)
 * 2. Map priority to Traffic Class using netdev TC map
 * 3. Select queue within TC's queue range using hash
 *
 * Returns: TX queue index
 */
static u16 yk3_pick_tx(struct net_device *ndev, struct sk_buff *skb)
{
	struct yk3_ndev_priv *ndev_priv;
	int channel_ix;
	u8 up = 0;
	u8 tc = 0;
	u16 txqnum = 0;
	u16 qoffset, qcount;
	u32 hash;

	if (!ndev)
		return 0;

	ndev_priv = netdev_priv(ndev);
	if (!ndev_priv)
		return 0;

	txqnum = ndev->real_num_tx_queues;

	/* Calculate hash for queue selection within TC */
	if (skb->sk && skb->sk->sk_hash)
		hash = skb->sk->sk_hash;
	else
		hash = skb_get_hash(skb);

	channel_ix = reciprocal_scale(hash, txqnum);

	if (yk3_ndev_is_vf(ndev_priv))
		return channel_ix;

	/* If PFC not enabled, use simple hash-based selection */
#ifdef CONFIG_DCB
	if (!ndev_priv->tc_cfg.ena_tc && ndev_priv->dcbx && !ndev_priv->dcbx->pfc.pfcena)
		return channel_ix;
#else
	if (!ndev_priv->tc_cfg.ena_tc)
		return channel_ix;
#endif

	/* Get user priority from packet */
	up = yk3_get_up(ndev, skb);

	/* Update skb->priority for kernel statistics */
	skb->priority = up;

	/* Map priority to TC using netdev mapping */
	if (ndev_priv->tc_cfg.ena_tc) {
		tc = netdev_get_prio_tc_map(ndev, skb->priority);
	} else {
		channel_ix /= 8;
		return ndev_priv->tc_cfg.tc2txq[up][channel_ix];
	}

	/* Ensure TC is valid */
	if (tc >= ndev_priv->tc_cfg.numtc)
		tc = 0;

	/* Get TC's queue range */
	qoffset = ndev_priv->tc_cfg.tc_info[tc].qoffset;
	qcount = ndev_priv->tc_cfg.tc_info[tc].qcount_tx;

	if (qcount == 0)
		qcount = 1;

	/* Select queue within TC's range using hash */
	channel_ix = reciprocal_scale(hash, qcount);

	return qoffset + channel_ix;
}

static u16 yk3_ndo_select_queue(struct net_device *ndev, struct sk_buff *skb,
				#ifdef YK3_HAVE_SELECT_QUEUE_NOCB
				struct net_device *sb_dev)
				#else
				struct net_device *sb_dev, select_queue_fallback_t fallback)
				#endif
{
	return yk3_pick_tx(ndev, skb);
}
#endif /* YK3_HAVE_SELECT_QUEUE_RH310 */

#ifdef YK3_HAVE_NDO_HWTSTAMP_GET
static int yk3_ndo_hwtstamp_get(struct net_device *ndev,
				struct kernel_hwtstamp_config *kernel_config)
{
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct hwtstamp_config *hwts_config = (struct hwtstamp_config *)kernel_config;
	int ret;

	ndev_priv = netdev_priv(ndev);
	pdev_priv = pci_get_drvdata(ndev_priv->pdev);

	ret = yk3_ptp_hwtstamp_get(pdev_priv, hwts_config);
	if (ret)
		return ret;

	return 0;
}
#endif /* YK3_HAVE_NDO_HWTSTAMP_GET */

#ifdef YK3_HAVE_NDO_HWTSTAMP_SET
static int yk3_ndo_hwtstamp_set(struct net_device *ndev,
				struct kernel_hwtstamp_config *kernel_config,
				struct netlink_ext_ack *extack)
{
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct hwtstamp_config *hwts_config = (struct hwtstamp_config *)kernel_config;
	int ret;

	ndev_priv = netdev_priv(ndev);
	pdev_priv = pci_get_drvdata(ndev_priv->pdev);

	ret = yk3_ptp_hwtstamp_set(pdev_priv, hwts_config);
	if (ret)
		return ret;

	return 0;
}
#endif /* YK3_HAVE_NDO_HWTSTAMP_SET */

static int yk3_ioctl_hwtstamp_get(struct net_device *ndev, struct ifreq *ifr)
{
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct hwtstamp_config hwts_config;
	int ret;

	ndev_priv = netdev_priv(ndev);
	pdev_priv = ndev_priv->pdev_priv;

	ret = yk3_ptp_hwtstamp_get(pdev_priv, &hwts_config);
	if (ret)
		return ret;

	if (copy_to_user(ifr->ifr_data, &hwts_config,
			 sizeof(hwts_config)))
		return -EFAULT;

	return 0;
}

static int yk3_ioctl_hwtstamp_set(struct net_device *ndev, struct ifreq *ifr)
{
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct hwtstamp_config hwts_config;
	int ret;

	ndev_priv = netdev_priv(ndev);
	pdev_priv = pci_get_drvdata(ndev_priv->pdev);

	if (copy_from_user(&hwts_config, ifr->ifr_data, sizeof(hwts_config)))
		return -EFAULT;

	ret = yk3_ptp_hwtstamp_set(pdev_priv, &hwts_config);
	if (ret)
		return ret;

	if (copy_to_user(ifr->ifr_data, &hwts_config, sizeof(hwts_config)))
		return -EFAULT;

	return 0;
}

static int yk3_ndo_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	int ret;

	switch (cmd) {
	case SIOCSHWTSTAMP:
		ret = yk3_ioctl_hwtstamp_set(ndev, ifr);
		break;
	case SIOCGHWTSTAMP:
		ret = yk3_ioctl_hwtstamp_get(ndev, ifr);
		break;
	default:
		return -EOPNOTSUPP;
	}

	return ret;
}

#ifdef YK3_HAVE_NDO_ETH_IOCTL
static int yk3_ndo_eth_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	int ret;

	switch (cmd) {
	case SIOCSHWTSTAMP:
		ret = yk3_ioctl_hwtstamp_set(ndev, ifr);
		break;
	case SIOCGHWTSTAMP:
		ret = yk3_ioctl_hwtstamp_get(ndev, ifr);
		break;
	default:
		return -EOPNOTSUPP;
	}

	return ret;
}
#endif /* YK3_HAVE_NDO_ETH_IOCTL */

static const struct net_device_ops yk3_ndev_ops = {
	.ndo_open = yk3_ndo_open,
	.ndo_stop = yk3_ndo_stop,
	.ndo_start_xmit = yk3_ndo_start_xmit,
	.ndo_set_mac_address = yk3_ndo_set_mac,
	.ndo_do_ioctl = yk3_ndo_ioctl,
#ifdef YK3_HAVE_NDO_ETH_IOCTL
	.ndo_eth_ioctl = yk3_ndo_eth_ioctl,
#endif /* YK3_HAVE_NDO_ETH_IOCTL */
#ifdef YK3_HAVE_NDO_HWTSTAMP_GET
	.ndo_hwtstamp_get = yk3_ndo_hwtstamp_get,
#endif /* YK3_HAVE_NDO_HWTSTAMP_GET */
#ifdef YK3_HAVE_NDO_HWTSTAMP_SET
	.ndo_hwtstamp_set = yk3_ndo_hwtstamp_set,
#endif /* YK3_HAVE_NDO_HWTSTAMP_SET */
#ifdef YK3_HAVE_NDO_EXT_CHANGE_MTU
	.extended.ndo_change_mtu = yk3_ndo_change_mtu,
#elif defined YK3_HAVE_CHANGE_MTU_RH74
	.ndo_change_mtu_rh74 = yk3_ndo_change_mtu,
#else
	.ndo_change_mtu = yk3_ndo_change_mtu,
#endif /* YK3_HAVE_NDO_EXT_CHANGE_MTU */
	.ndo_get_stats64 = yk3_ndo_get_stats64,
	.ndo_change_rx_flags = yk3_ndo_change_rx_flags,
	.ndo_set_rx_mode = yk3_ndo_set_rx_mode,
	.ndo_set_features = yk3_ndo_set_features,
	.ndo_fix_features = yk3_ndo_fix_features,
	.ndo_set_vf_mac = yk3_ndo_set_vf_mac,
	.ndo_set_vf_rate = yk3_ndo_set_vf_rate,
	.ndo_bridge_setlink = yk3_ndo_bridge_setlink,
	.ndo_bridge_getlink = yk3_ndo_bridge_getlink,
#ifdef YK3_HAVE_NDO_SET_TX_MAXRATE
	.ndo_set_tx_maxrate = yk3_ndo_set_tx_maxrate,
#endif
#ifdef YK3_HAVE_NDO_EXT_SET_VF_VLAN
	.extended.ndo_set_vf_vlan = yk3_ndo_set_vf_vlan,
#else
	.ndo_set_vf_vlan = yk3_ndo_set_vf_vlan,
#endif /* YK3_HAVE_NDO_EXT_SET_VF_VLAN */
	.ndo_get_vf_config = yk3_ndo_get_vf_config,
	.ndo_vlan_rx_add_vid = yk3_ndo_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = yk3_ndo_vlan_rx_kill_vid,
#ifdef YK3_HAVE_NDO_SETUP_TC
	.ndo_setup_tc		= yk3_ndo_setup_tc,
#endif
#ifdef YK3_HAVE_NDO_SET_VF_TRUST
	.ndo_set_vf_trust = yk3_ndo_set_vf_trust,
#else
	.extended.ndo_set_vf_trust = yk3_ndo_set_vf_trust,
#endif /* YK3_HAVE_NDO_SET_VF_TRUST */
#ifdef YK3_HAVE_NDO_UDP_TUNNEL
	.ndo_udp_tunnel_add = yk3_add_udp_tunnel,
	.ndo_udp_tunnel_del = yk3_del_udp_tunnel,
#endif
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	.ndo_features_check = yk3_features_check,
#endif
	.ndo_set_vf_link_state = yk3_ndo_set_vf_link_state,
	.ndo_set_vf_spoofchk = yk3_ndo_set_vf_spoofchk,
#ifndef YK3_HAVE_SELECT_QUEUE_RH310
	.ndo_select_queue = yk3_ndo_select_queue,
#endif
};

int yk3_ndev_bw_limited(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct netdev_queue *tx_queue;
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	u16 queue = 0;

	if (test_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags)) {
		yk3_dev_err("tc enabled by tc qdisc,disable use cmd eg: %s",
			    "tc qdisc del dev eth0 root");
		return true;
	} else if ((test_bit(YK3_ETS_PENDING, ndev_priv->tc_cfg.flags) ||
		    test_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags)) &&
		    (yk3_qos_get_num_tc(ndev_priv->etscfg) > 1)) {
		yk3_dev_err("tc enabled by ETS, kill lldpad then clear ETS cfg eg: %s %s %s",
			    "dcb ets set dev eth0 prio-tc all:0",
			    "tc-bw 0:100 1:0 2:0 3:0 4:0 5:0 6:0 7:0",
			    "tc-tsa all:strict");

		return true;
	}

	if (ndev_priv->mdrop_rx_rate || ndev_priv->hqos_tx_rate)
		return true;

	for (queue = 0; queue < total_qnum; queue++) {
		tx_queue = netdev_get_tx_queue(ndev_priv->ndev, queue);
		if (tx_queue->tx_maxrate)
			return true;
	}

	if (yk3_ndev_is_vf(ndev_priv))
		return yk3_qos_get_vf_rate(ndev_priv);

	return false;
}

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO && !defined YK3_HAVE_NDO_UDP_TUNNEL
static int yk3_udp_tnl_set_port(struct net_device *ndev,
				unsigned int table, unsigned int entry,
				struct udp_tunnel_info *ti)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (yk3_pdev_is_vf(pdev_priv)) {
		if (ti->type == UDP_TUNNEL_TYPE_VXLAN) {
			if (ntohs(ti->port) != YK3_VXLAN_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default vxlan port!\n");
				return -EOPNOTSUPP;
			}
		} else if (ti->type == UDP_TUNNEL_TYPE_GENEVE) {
			if (ntohs(ti->port) != YK3_GENEVE_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default geneve port!\n");
				return -EOPNOTSUPP;
			}
		}
	}

	ret = yk3_udp_tunnel_set(ndev, ti, YK3_SET_UDP_TUNNEL);
	if (ret) {
		yk3_net_warn("Failed to set udp tunnel port, err is %d", ret);
		return -EFAULT;
	}

	return 0;
}

static int yk3_udp_tnl_unset_port(struct net_device *ndev,
				  unsigned int table, unsigned int entry,
				  struct udp_tunnel_info *ti)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (yk3_pdev_is_vf(pdev_priv)) {
		if (ti->type == UDP_TUNNEL_TYPE_VXLAN) {
			if (ntohs(ti->port) != YK3_VXLAN_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default vxlan port!\n");
				return -EOPNOTSUPP;
			}
		} else if (ti->type == UDP_TUNNEL_TYPE_GENEVE) {
			if (ntohs(ti->port) != YK3_GENEVE_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default geneve port!\n");
				return -EOPNOTSUPP;
			}
		}
	}

	ret = yk3_udp_tunnel_set(ndev, ti, YK3_UNSET_UDP_TUNNEL);
	if (ret) {
		yk3_net_warn("Failed to set udp tunnel port, err is %d", ret);
		return -EINVAL;
	}

	return 0;
}

static const struct udp_tunnel_nic_info yk3_udp_tunnels = {
	.set_port	= yk3_udp_tnl_set_port,
	.unset_port	= yk3_udp_tnl_unset_port,
	.flags          = UDP_TUNNEL_NIC_INFO_MAY_SLEEP,
	.tables         = {
		{
			.n_entries = YK3_VXLAN_PORT_MAX,
			.tunnel_types = UDP_TUNNEL_TYPE_VXLAN,
		},
		{
			.n_entries = YK3_GENEVE_PORT_MAX,
			.tunnel_types = UDP_TUNNEL_TYPE_GENEVE,
		},
	},
};

static const struct udp_tunnel_nic_info yk3_vf_udp_tunnels = {
	.set_port	= yk3_udp_tnl_set_port,
	.unset_port	= yk3_udp_tnl_unset_port,
	.flags          = UDP_TUNNEL_NIC_INFO_MAY_SLEEP,
	.tables         = {
		{
			.n_entries = YK3_VF_VXLAN_PORT_MAX,
			.tunnel_types = UDP_TUNNEL_TYPE_VXLAN,
		},
		{
			.n_entries = YK3_GENEVE_PORT_MAX,
			.tunnel_types = UDP_TUNNEL_TYPE_GENEVE,
		},
	},
};
#endif

static void yk3_ndev_create_name(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	switch (ndev_priv->type) {
	case YK3_NDEV_T_PF:
		snprintf(ndev_priv->name, sizeof(ndev_priv->name),
			 "%s-pf%d", pdev_priv->card->name, pdev_priv->pf_id);
		break;
	case YK3_NDEV_T_VF:
		snprintf(ndev_priv->name, sizeof(ndev_priv->name),
			 "%s-pf%dvf%d", pdev_priv->card->name, pdev_priv->pf_id,
			 pdev_priv->vf_id - 1);
		break;
	case YK3_NDEV_T_REP:
	default:
		yk3_net_err("unsupported ndev type %d", ndev_priv->type);
		break;
	}
}

/* temporary, should invoke np function */
#define YK3_RN_NP_BASE		(0x1000000)
#define YK3_RN_CLUSTER_BASE(i)	(YK3_RN_NP_BASE + 0x20000 * (i))
#define YK3_RN_SHM_ADDR		(0x10010)
#define YK3_RN_SHM_DATA		(0x10014)
#define YK3_RN_SHM_ATOM		(0x1001C)
#define YK3_RN_SHM_PFLRO	(0x1050)
#define YK3_N_NP_PPE_CLUSTER	(24)

static inline u32
yk3_np_shm_rd32(void __iomem *baddr, u32 shm_addr, u32 cluster)
{
	u32 addr, data;

	addr = YK3_RN_CLUSTER_BASE(cluster) + YK3_RN_SHM_ADDR;
	data = YK3_RN_CLUSTER_BASE(cluster) + YK3_RN_SHM_DATA;

	yk3_wr32(baddr, addr, shm_addr);

	return yk3_rd32(baddr, data);
}

static inline void
yk3_np_shm_wr32(void __iomem *baddr, u32 shm_addr, u32 cluster, u32 val)
{
	u32 addr, data;

	addr = YK3_RN_CLUSTER_BASE(cluster) + YK3_RN_SHM_ADDR;
	data = YK3_RN_CLUSTER_BASE(cluster) + YK3_RN_SHM_DATA;

	yk3_wr32(baddr, addr, shm_addr);
	yk3_wr32(baddr, data, val);
}

static void yk3_lro_set_enable(struct yk3_pdev_priv *pdev_priv, bool enable)
{
	void __iomem *hwaddr = pdev_priv->bar_addr[YK3_BAR0];
	int i;
	u32 val;

	val = yk3_np_shm_rd32(hwaddr, YK3_RN_SHM_PFLRO, 0);
	if (enable)
		val |= 1 << pdev_priv->pf_id;
	else
		val &= ~(1 << pdev_priv->pf_id);

	for (i = 0; i < YK3_N_NP_PPE_CLUSTER; i++)
		yk3_np_shm_wr32(hwaddr, YK3_RN_SHM_PFLRO, i, val);
}

static int
yk3_ndev_create(struct yk3_pdev_priv *pdev_priv, struct yk3_ndev_param *param)
{
	struct net_device *ndev;
	struct yk3_ndev_priv *ndev_priv;
	u16 qnum = param->qbase[YK3_QUEUE_T_LOCAL].num;
	u8 mac_addr[ETH_ALEN];
	struct yk3_card *card = pdev_priv->card;
	int ret;

	ndev = alloc_etherdev_mq(sizeof(*ndev_priv), qnum);
	if (IS_ERR_OR_NULL(ndev)) {
		yk3_dev_err("ndev alloc failed");
		return -ENOMEM;
	}

	ndev_priv = netdev_priv(ndev);
	memset(ndev_priv, 0, sizeof(*ndev_priv));
	ndev_priv->ndev = ndev;
	ndev_priv->pdev = pdev_priv->pdev;
	ndev_priv->pdev_priv = pdev_priv;
	ndev_priv->type = param->type;

	INIT_LIST_HEAD(&ndev_priv->cvlan_list);
	INIT_LIST_HEAD(&ndev_priv->svlan_list);
	mutex_init(&ndev_priv->state_mlock);
	spin_lock_init(&ndev_priv->statistics_lock);

	ndev_priv->qbase[YK3_QUEUE_T_LOCAL] = param->qbase[YK3_QUEUE_T_LOCAL];
	ndev_priv->qbase[YK3_QUEUE_T_FUNC] = param->qbase[YK3_QUEUE_T_FUNC];
	ndev_priv->qbase[YK3_QUEUE_T_PF] = param->qbase[YK3_QUEUE_T_PF];
	ndev_priv->qbase[YK3_QUEUE_T_GLOBAL] = param->qbase[YK3_QUEUE_T_GLOBAL];

	yk3_ndev_create_name(ndev_priv);

	switch (param->type) {
	case YK3_NDEV_T_PF:
	case YK3_NDEV_T_VF:
		ndev->dev_port = 0;	/* wait */
		SET_NETDEV_DEV(ndev, pdev_priv->dev);
		break;
	case YK3_NDEV_T_REP:
	default:
		/* should use SET_NETDEV_DEVLINK_PORT */
		/* not supported; */
		ret = -EOPNOTSUPP;
		goto err_dev_port;
	}

	ret = yk3_qsetid_alloc(pdev_priv, param->type);
	if (ret < 0) {
		yk3_dev_err("qsetid alloc failed, ret %d", ret);
		goto err_qsetid_alloc;
	}

	ndev_priv->qsetid = (u16)ret;

	/* rep should modified */
	ndev_priv->pf_id = pdev_priv->pf_id;
	ndev_priv->vf_id = pdev_priv->vf_id;

	if (yk3_pdev_is_pf(pdev_priv) && param->type == YK3_NDEV_T_PF) {
		if (!is_valid_ether_addr(card->emp_info->vpd.mac_addr)) {
			ndev->addr_len = ETH_ALEN;
			eth_hw_addr_random(ndev);
		} else {
			ether_addr_copy(mac_addr, card->emp_info->vpd.mac_addr);
			mac_addr[ETH_ALEN - 1] += pdev_priv->pf_id;
			eth_hw_addr_set(ndev, mac_addr);
		}
		memcpy(ndev_priv->intf_mac, ndev->dev_addr, ETH_ALEN);
	}

	ndev->mtu = ETH_DATA_LEN;
#ifdef YK3_HAVE_MAX_MTU
	ndev->min_mtu = YK3_N_MIN_MTU;
	ndev->max_mtu = YK3_N_MAX_MTU;
#else
	ndev->extended->min_mtu = YK3_N_MIN_MTU;
	ndev->extended->max_mtu = YK3_N_MAX_MTU;
#endif /* YK3_HAVE_MAX_MTU */

	/* ethtool -k offload default value */
	ndev->features |= NETIF_F_HIGHDMA;
	ndev->features |= NETIF_F_SG;
	ndev->features |= NETIF_F_GSO;
	ndev->features |= NETIF_F_GRO;
	ndev->features |= NETIF_F_GSO_ENCAP_ALL;
	ndev->features |= NETIF_F_HW_CSUM;
	ndev->features |= NETIF_F_RXCSUM;
	ndev->features |= NETIF_F_TSO;
	ndev->features |= NETIF_F_TSO6;
	ndev->features |= NETIF_F_RXHASH;
	ndev->features |= NETIF_F_GSO_UDP_L4;
	ndev->features |= NETIF_F_HW_VLAN_CTAG_RX;
	ndev->features |= NETIF_F_HW_VLAN_CTAG_TX;
	ndev->features |= NETIF_F_HW_VLAN_STAG_RX;
	ndev->features |= NETIF_F_HW_VLAN_STAG_TX;
	ndev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	ndev->features |= NETIF_F_HW_VLAN_STAG_FILTER;
#ifndef YK3_TC_DISABLE
	ndev->features |= NETIF_F_HW_TC;
#endif

	/* ethtool -k offload option */
	ndev->hw_features |= NETIF_F_SG;
	ndev->hw_features |= NETIF_F_HW_CSUM;
	ndev->hw_features |= NETIF_F_RXCSUM;
	ndev->hw_features |= NETIF_F_GSO;
	ndev->hw_features |= NETIF_F_GRO;
	ndev->hw_features |= NETIF_F_RXALL;
	ndev->hw_features |= NETIF_F_RXHASH;
	ndev->hw_features |= NETIF_F_TSO;
	ndev->hw_features |= NETIF_F_TSO6;
	ndev->hw_features |= NETIF_F_GSO_ENCAP_ALL;
	ndev->hw_features |= NETIF_F_TSO_MANGLEID;
	ndev->hw_features |= NETIF_F_GSO_UDP_L4;
	ndev->hw_features |= NETIF_F_GSO_PARTIAL;
	ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
	ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
	ndev->hw_features |= NETIF_F_HW_VLAN_STAG_RX;
	ndev->hw_features |= NETIF_F_HW_VLAN_STAG_TX;
	ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	ndev->hw_features |= NETIF_F_HW_VLAN_STAG_FILTER;
#ifndef YK3_TC_DISABLE
	ndev->hw_features |= NETIF_F_HW_TC;
#endif

	/* vlan features */
	ndev->vlan_features = ndev->hw_features;

	/* enc features */
	ndev->hw_enc_features |= NETIF_F_GSO_ENCAP_ALL;
	ndev->hw_enc_features |= NETIF_F_TSO_MANGLEID;
	ndev->hw_enc_features |= NETIF_F_SG;
	ndev->hw_enc_features |= NETIF_F_HW_CSUM;
	ndev->hw_enc_features |= NETIF_F_TSO;
	ndev->hw_enc_features |= NETIF_F_TSO6;
	ndev->hw_enc_features |= NETIF_F_GSO_UDP_L4;

	/* irq coal */
	ndev_priv->itr_rx_enable = 1;
	ndev_priv->itr_tx_enable = 1;
	ndev_priv->rx_coalesce_usecs = 128;
	ndev_priv->rx_max_coalesced_frames = 128;
	ndev_priv->tx_coalesce_usecs = 128;
	ndev_priv->tx_max_coalesced_frames = 128;

	/* ethtool priv features */

	/* lro, wait */
	if (yk3_ndev_is_pf(ndev_priv)) {
		ndev->hw_features |= NETIF_F_LRO;
		ndev_priv->lro_enable = false;
		yk3_lro_set_enable(pdev_priv, false);
	}

	ndev_priv->txq_depth = YK3_N_DEFAULT_DEPTH;
	ndev_priv->rxq_depth = YK3_N_DEFAULT_DEPTH;

	netif_set_real_num_tx_queues(ndev, ndev_priv->qbase[YK3_QUEUE_T_LOCAL].num);
	netif_set_real_num_rx_queues(ndev, ndev_priv->qbase[YK3_QUEUE_T_LOCAL].num);

	ndev->gso_max_size = YK3_N_TSO_MAXSIZE;
	ndev->gso_max_segs = min_t(u16, YK3_N_TSO_MAXSEGS, (ndev_priv->txq_depth >> 2));

	ret = yk3_debug_ndev_init(ndev_priv);
	if (ret) {
		yk3_dev_err("debug ndev init failed, ret %d", ret);
		goto err_debug_ndev_init;
	}

	ret = yk3_debug_fc_init(ndev_priv);
	if (ret) {
		yk3_net_err("debug fc init failed, ret %d", ret);
		goto err_debug_fc_init;
	}

	ret = yk3_edma_create_queues(ndev_priv);
	if (ret) {
		yk3_dev_err("edma create queues failed, ret %d", ret);
		goto err_edma_create_queues;
	}

	ret = yk3_edma_set_rx_rate(ndev_priv, 0);
	if (ret) {
		yk3_dev_err("edma set rx rate failed, ret %d", ret);
		goto err_set_rx_rate;
	}

	ret = yk3_qos_ndev_init(ndev);
	if (ret) {
		yk3_dev_err("qos init ndev failed, ret %d", ret);
		goto err_qos_init;
	}

	/* lan init */
	ret = yk3_lan_ndev_init(ndev);
	if (ret) {
		yk3_dev_err("lan init ndev failed, ret %d", ret);
		goto err_lan_init;
	}

	/* mac init */
	ret = yk3_mac_ndev_init(ndev);
	if (ret) {
		yk3_dev_err("mac init ndev failed, ret %d", ret);
		goto err_mac_init;
	}

	/* tc init */

	ret = yk3_rss_ndev_init(ndev_priv);
	if (ret) {
		yk3_dev_err("rss netdev init failed, ret %d", ret);
		goto err_rss_init;
	}

	/* dcbx init */
	yk3_dcbnl_init(ndev_priv);

	ndev->netdev_ops = &yk3_ndev_ops;
	ndev->ethtool_ops = &yk3_ethtool_ops;
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	ndev_priv->udp_tnl_port =  kzalloc(sizeof(*ndev_priv->udp_tnl_port), GFP_KERNEL);
	if (!ndev_priv->udp_tnl_port) {
		ret = -ENOMEM;
		goto err_udp_tnl_port;
	}
#endif
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO && !defined YK3_HAVE_NDO_UDP_TUNNEL
	if (yk3_ndev_is_pf(ndev_priv))
		ndev->udp_tunnel_nic_info = &yk3_udp_tunnels;
	else if (yk3_ndev_is_vf(ndev_priv))
		ndev->udp_tunnel_nic_info = &yk3_vf_udp_tunnels;
#endif

	netif_carrier_off(ndev);

	ret = register_netdev(ndev);
	if (ret) {
		yk3_dev_err("register netdev failed, ret %d", ret);
		goto err_register_netdev;
	}

	mutex_lock(&pdev_priv->ndev_priv_head_mlock);
	list_add_tail(&ndev_priv->pdev_node, &pdev_priv->ndev_priv_head);
	mutex_unlock(&pdev_priv->ndev_priv_head_mlock);

	return 0;

err_register_netdev:
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	kfree(ndev_priv->udp_tnl_port);
err_udp_tnl_port:
#endif
err_rss_init:
err_mac_init:
	yk3_lan_ndev_exit(ndev);
err_lan_init:
err_qos_init:
err_set_rx_rate:
	yk3_edma_destroy_queues(ndev_priv);
err_debug_fc_init:
	yk3_debug_fc_exit(ndev_priv);
err_edma_create_queues:
	yk3_debug_ndev_exit(ndev_priv);
err_debug_ndev_init:
	yk3_qsetid_free(pdev_priv, ndev_priv->qsetid);
err_qsetid_alloc:
err_dev_port:
	free_netdev(ndev);
	return ret;
}

static void
yk3_ndev_destroy(struct yk3_pdev_priv *pdev_priv, struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_vlan *vlan_node, *temp;
	u16 temp_txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 temp_rxqnum = ndev_priv->ndev->real_num_rx_queues;

	rtnl_lock();
	if (netif_running(ndev_priv->ndev))
		dev_close(ndev_priv->ndev);
	rtnl_unlock();

	mutex_lock(&pdev_priv->ndev_priv_head_mlock);
	list_del(&ndev_priv->pdev_node);
	mutex_unlock(&pdev_priv->ndev_priv_head_mlock);

	yk3_qos_ndev_exit(ndev_priv->ndev);

	yk3_lan_ndev_exit(ndev_priv->ndev);

	unregister_netdev(ndev_priv->ndev);

	yk3_dcbnl_exit(ndev_priv);
	yk3_rss_ndev_exit(ndev_priv);
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	kfree(ndev_priv->udp_tnl_port);
#endif

	/* delete ndev vlan */
	list_for_each_entry_safe(vlan_node, temp, &ndev_priv->cvlan_list, list) {
		list_del(&vlan_node->list);
		kfree(vlan_node);
	}
	list_for_each_entry_safe(vlan_node, temp, &ndev_priv->svlan_list, list) {
		list_del(&vlan_node->list);
		kfree(vlan_node);
	}

	netif_set_real_num_tx_queues(ndev_priv->ndev, temp_txqnum);
	netif_set_real_num_rx_queues(ndev_priv->ndev, temp_rxqnum);
	yk3_edma_destroy_queues(ndev_priv);

	yk3_debug_fc_exit(ndev_priv);
	yk3_debug_ndev_exit(ndev_priv);
	yk3_qsetid_free(pdev_priv, ndev_priv->qsetid);
	free_netdev(ndev_priv->ndev);
}

int yk3_ndev_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ndev_param param;
	struct yk3_queuebase qbase, p_qbase;
	int ret;

	ret = yk3_rss_init(pdev_priv);
	if (ret) {
		yk3_dev_err("rss init failed, ret %d", ret);
		return ret;
	}

	if (yk3_pdev_is_mgr(pdev_priv))
		return 0;

	if (yk3_pdev_is_pf(pdev_priv)) {
		param.type = YK3_NDEV_T_PF;

		ret = yk3_edma_alloc_p_qbase(pdev_priv, pdev_priv->card->pf_ndev_qnum, &p_qbase);
		if (ret) {
			yk3_dev_err("edma alloc p qbase failed, ret %d", ret);
			return ret;
		}

		param.qbase[YK3_QUEUE_T_PF] = p_qbase;
		qbase = yk3_edma_p_qbase_cast(pdev_priv, YK3_QUEUE_T_LOCAL, p_qbase);
		param.qbase[YK3_QUEUE_T_LOCAL] = qbase;
		qbase = yk3_edma_p_qbase_cast(pdev_priv, YK3_QUEUE_T_FUNC, p_qbase);
		param.qbase[YK3_QUEUE_T_FUNC] = qbase;
		qbase = yk3_edma_p_qbase_cast(pdev_priv, YK3_QUEUE_T_GLOBAL, p_qbase);
		param.qbase[YK3_QUEUE_T_GLOBAL] = qbase;
	}

	if (yk3_pdev_is_vf(pdev_priv)) {
		param.type = YK3_NDEV_T_VF;

		yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_LOCAL, &param.qbase[YK3_QUEUE_T_LOCAL]);
		yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_FUNC, &param.qbase[YK3_QUEUE_T_FUNC]);
		yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_PF, &param.qbase[YK3_QUEUE_T_PF]);
		yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_GLOBAL, &param.qbase[YK3_QUEUE_T_GLOBAL]);
	}

	return yk3_ndev_create(pdev_priv, &param);
}

void yk3_ndev_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ndev_priv *ndev_priv, *pos;

	list_for_each_entry_safe(ndev_priv, pos, &pdev_priv->ndev_priv_head, pdev_node)
		yk3_ndev_destroy(pdev_priv, ndev_priv);

	yk3_rss_exit(pdev_priv);
}
