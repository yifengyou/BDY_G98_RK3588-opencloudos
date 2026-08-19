// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_emp_priv.h"

static const char yk3_ethtool_priv_strings[YK3_ET_PFLAG_MAX][ETH_GSTRING_LEN] = {
	"rxhash_tun_inner_tuple",
	"pvid_miss_upload",
	"fc_pause_filter",
	"pfc_pause_filter",
	"prio_vlan_mode",
	"lldp_fw_mode",
	"link-down-on-close",
	"src_eq_dst_drop",
};

static void yk3_get_drvinfo(struct net_device *ndev,
			    struct ethtool_drvinfo *drvinfo)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	strscpy(drvinfo->bus_info, pci_name(pdev_priv->pdev),
		sizeof(drvinfo->bus_info));
	strscpy(drvinfo->driver, dev_driver_string(pdev_priv->dev),
		sizeof(drvinfo->driver));
	strscpy(drvinfo->version, YK3_GIT_VERSION, sizeof(drvinfo->version));
	snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
		 "%02u.%02u.%02u.%02u",
		 ((u8 *)&pdev_priv->card->emp_info->vpd.img_version)[0],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.img_version)[1],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.img_version)[2],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.img_version)[3]);
#ifdef YK3_HAVE_ETHTOOL_EROM_VERSION
	snprintf(drvinfo->erom_version, sizeof(drvinfo->erom_version),
		 "%02u.%02u.%02u.%02u",
		 ((u8 *)&pdev_priv->card->emp_info->vpd.pxe_version)[0],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.pxe_version)[1],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.pxe_version)[2],
		 ((u8 *)&pdev_priv->card->emp_info->vpd.pxe_version)[3]);
#endif
}

static const char yk3_self_test_strings[][ETH_GSTRING_LEN] = {
	"Link test ",
	"Speed test ",
	"Registers test ",
	"Interrupt test ",
	"Loopback test ",
};

#define YK3_SELF_TEST_LEN ARRAY_SIZE(yk3_self_test_strings)

static int yk3_get_selftest_count(struct yk3_ndev_priv *ndev_priv)
{
	if (!yk3_ndev_is_pf(ndev_priv))
		return -EOPNOTSUPP;
	return YK3_SELF_TEST_LEN;
}

static void yk3_get_selftest_strings(u8 *data)
{
	memcpy(data, yk3_self_test_strings, sizeof(yk3_self_test_strings));
}

static void yk3_get_ethtool_stats(struct net_device *ndev,
				  struct ethtool_stats *stats, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int len = 0;

	yk3_edma_et_get_stats(ndev_priv, data);
	len += yk3_edma_et_get_sset_count(ndev_priv);
	yk3_mac_et_get_stats(ndev_priv, &data[len]);
	len += yk3_mac_et_get_sset_count(ndev_priv);
	yk3_lan_et_get_stats(ndev_priv, &data[len]);
}

static void yk3_get_strings(struct net_device *ndev, u32 stringset, u8 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u8 *cur;

	switch (stringset) {
	case ETH_SS_STATS:
		yk3_edma_et_get_strings(ndev_priv, data, &cur);
		yk3_mac_et_get_stats_strings(ndev_priv, cur, &cur);
		yk3_lan_et_get_stats_strings(ndev_priv, cur, &cur);
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, yk3_ethtool_priv_strings,
		       sizeof(yk3_ethtool_priv_strings));
		break;
	case ETH_SS_TEST:
		yk3_get_selftest_strings(data);
		break;
	default:
		break;
	}
	yk3_net_debug("stringset %u\n", stringset);
}

static int yk3_get_sset_count(struct net_device *ndev, int sset)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret;

	switch (sset) {
	case ETH_SS_STATS:
		ret = yk3_edma_et_get_sset_count(ndev_priv);
		ret += yk3_mac_et_get_sset_count(ndev_priv);
		ret += yk3_lan_et_get_sset_count(ndev_priv);
		break;
	case ETH_SS_PRIV_FLAGS:
		ret = YK3_ET_PFLAG_MAX;
		break;
	case ETH_SS_TEST:
		ret = yk3_get_selftest_count(ndev_priv);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}
	yk3_net_debug("sset %u sset_count %d\n", sset, ret);
	return ret;
}

static int yk3_get_lldp_priv_flags(struct yk3_ndev_priv *ndev_priv, bool *enabled)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	mbox_msg.data[0] = pdev_priv->mac->mac_ch;
	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_GET_LLDP_MODE;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data_length = 2;
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		yk3_dev_err("Send mbox message errno %d failed!\n", ret);
		return ret;
	}

	if (pdev_priv->mac->mac_ch == recv_msg.data[0])
		*enabled = recv_msg.data[1];

	return 0;
}

static u32 yk3_get_priv_flags(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	bool lldp_enable = false;

	if (!yk3_get_lldp_priv_flags(ndev_priv, &lldp_enable)) {
		if (lldp_enable)
			ndev_priv->ethtool_priv_flags |= BIT(YK3_ET_PFLAG_LLDP_MODE);
		else
			ndev_priv->ethtool_priv_flags &= ~BIT(YK3_ET_PFLAG_LLDP_MODE);
	}

	return ndev_priv->ethtool_priv_flags;
}

static int yk3_set_priv_flags(struct net_device *ndev, u32 flag)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	u32 changed = 0;
	bool enable;
	int ret = 0;

	changed = ndev_priv->ethtool_priv_flags ^ flag;

	if (!changed)
		return 0;

	if (changed & BIT(YK3_ET_PFLAG_PVID_MISS_UPLOAD)) {
		if (!yk3_pdev_is_pf(pdev_priv))
			return -EOPNOTSUPP;
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_PVID_MISS_UPLOAD))
			enable = false;
		else
			enable = true;
		ret = yk3_lan_ethtool_set_priv_flags(ndev,
						     YK3_ET_PFLAG_PVID_MISS_UPLOAD,
						     enable);
		if (ret) {
			yk3_net_err("lan set ethtool priv pvid miss upload flags failed: %d",
				    ret);
			return -EFAULT;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_PVID_MISS_UPLOAD);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_PVID_MISS_UPLOAD);
	}

	if (changed & BIT(YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY))
			enable = false;
		else
			enable = true;
		ret = yk3_lan_ethtool_set_priv_flags(ndev,
						     YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY,
						     enable);
		if (ret) {
			yk3_net_err("lan set ethtool priv rss sel overlay flags failed: %d",
				    ret);
			return -EFAULT;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY);
	}

	if (changed & BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER))
			enable = false;
		else
			enable = true;
		ret = yk3_mac_set_priv_flags(ndev_priv,
					     YK3_ET_PFLAG_FC_PAUSE_FILTER,
					     enable);
		if (ret) {
			yk3_net_debug("umac set ethtool priv fc pause filter flags failed: %d",
				      ret);
			return -EOPNOTSUPP;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER);
	}

	if (changed & BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER))
			enable = false;
		else
			enable = true;
		ret = yk3_mac_set_priv_flags(ndev_priv,
					     YK3_ET_PFLAG_PFC_PAUSE_FILTER,
					     enable);
		if (ret) {
			yk3_net_debug("umac set ethtool priv pfc pause filter flags failed: %d",
				      ret);
			return -EOPNOTSUPP;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER);
	}

	if (changed & BIT(YK3_ET_PFLAG_PRIO_VLAN_MODE)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_PRIO_VLAN_MODE))
			enable = false;
		else
			enable = true;
		ret = yk3_mac_set_priv_flags(ndev_priv,
					     YK3_ET_PFLAG_PRIO_VLAN_MODE,
					     enable);
		if (ret) {
			yk3_net_debug("ppp set ethtool priv prio vlan mode flags failed: %d",
				      ret);
			return -EOPNOTSUPP;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_PRIO_VLAN_MODE);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_PRIO_VLAN_MODE);
	}

#ifdef CONFIG_DCB
	if (changed & BIT(YK3_ET_PFLAG_LLDP_MODE)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_PFLAG_LLDP_MODE))
			enable = false;
		else
			enable = true;
		ret = yk3_mac_set_priv_flags(ndev_priv,
					     YK3_ET_PFLAG_LLDP_MODE,
					     enable);
		if (ret) {
			yk3_net_debug("ppp set ethtool priv lldp fw mode flags failed: %d",
				      ret);
			return -EOPNOTSUPP;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_PFLAG_LLDP_MODE);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_PFLAG_LLDP_MODE);
	}
#endif

	if (changed & BIT(YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE)) {
		if (!yk3_pdev_is_pf(pdev_priv))
			return -EOPNOTSUPP;
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE);
	}

	if (changed & BIT(YK3_ET_FFLAG_SRC_EQ_DST_DROP)) {
		if (ndev_priv->ethtool_priv_flags &
		    BIT(YK3_ET_FFLAG_SRC_EQ_DST_DROP))
			enable = false;
		else
			enable = true;
		ret = yk3_lan_ethtool_set_priv_flags(ndev,
						     YK3_ET_FFLAG_SRC_EQ_DST_DROP,
						     enable);
		if (ret) {
			yk3_net_err("lan set ethtool priv src equal dst drop flags failed: %d",
				    ret);
			return -EFAULT;
		}
		ndev_priv->ethtool_priv_flags &=
			~BIT(YK3_ET_FFLAG_SRC_EQ_DST_DROP);
		ndev_priv->ethtool_priv_flags |=
			flag & BIT(YK3_ET_FFLAG_SRC_EQ_DST_DROP);
	}

	return 0;
}

#ifdef YK3_HAVE_ETHTOOL_COALESCE_CQE

static int yk3_get_coalesce(struct net_device *ndev, struct ethtool_coalesce *ec,
			    struct kernel_ethtool_coalesce *kec,
			    struct netlink_ext_ack *ack)
#else
static int yk3_get_coalesce(struct net_device *ndev, struct ethtool_coalesce *ec)
#endif /* YK3_HAVE_ETHTOOL_COALESCE_CQE */
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	ec->use_adaptive_rx_coalesce = ndev_priv->itr_rx_enable;
	ec->use_adaptive_tx_coalesce = ndev_priv->itr_tx_enable;

	ec->rx_coalesce_usecs       = ndev_priv->rx_coalesce_usecs;
	ec->rx_max_coalesced_frames = ndev_priv->rx_max_coalesced_frames;
	ec->tx_coalesce_usecs       = ndev_priv->tx_coalesce_usecs;
	ec->tx_max_coalesced_frames = ndev_priv->tx_max_coalesced_frames;

	return 0;
}

#ifdef YK3_HAVE_ETHTOOL_COALESCE_CQE
static int yk3_set_coalesce(struct net_device *ndev, struct ethtool_coalesce *ec,
			    struct kernel_ethtool_coalesce *kec,
			    struct netlink_ext_ack *ack)
#else
static int yk3_set_coalesce(struct net_device *ndev, struct ethtool_coalesce *ec)
#endif /* YK3_HAVE_ETHTOOL_COALESCE_CQE */
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (ec->rx_coalesce_usecs > ndev_priv->pdev_priv->card->coalesce_max_usecs ||
	    ec->tx_coalesce_usecs > ndev_priv->pdev_priv->card->coalesce_max_usecs)
		return -EINVAL;
	if (!ec->rx_max_coalesced_frames || !ec->tx_max_coalesced_frames)
		return -EINVAL;
	if (ec->rx_max_coalesced_frames > ndev_priv->pdev_priv->card->coalesce_max_frames ||
	    ec->tx_max_coalesced_frames > ndev_priv->pdev_priv->card->coalesce_max_frames)
		return -EINVAL;
	if (ec->rx_max_coalesced_frames > (ndev_priv->rxq_depth >> 1) ||
	    ec->tx_max_coalesced_frames > (ndev_priv->txq_depth >> 1))
		return -EINVAL;

	ndev_priv->itr_rx_enable = ec->use_adaptive_rx_coalesce;
	ndev_priv->itr_tx_enable = ec->use_adaptive_tx_coalesce;

	ndev_priv->rx_coalesce_usecs       = ec->rx_coalesce_usecs;
	ndev_priv->rx_max_coalesced_frames = ec->rx_max_coalesced_frames;
	ndev_priv->tx_coalesce_usecs       = ec->tx_coalesce_usecs;
	ndev_priv->tx_max_coalesced_frames = ec->tx_max_coalesced_frames;

	yk3_edma_set_coal(ndev_priv);

	return 0;
}

#ifdef YK3_HAVE_ETHTOOL_MAC_STATS
static void yk3_get_eth_mac_stats(struct net_device *ndev,
				  struct ethtool_eth_mac_stats *mac_stats)
{
}
#endif /* YK3_HAVE_ETHTOOL_MAC_STATS */

static u32 yk3_get_rxfh_indir_size(struct net_device *ndev)
{
	return 4 * ndev->real_num_rx_queues;
}

static u32 yk3_get_rxfh_key_size(struct net_device *ndev)
{
	return yk3_rss_key_size();
}

#ifdef YK3_HAVE_ETHTOOL_GET_RXFH_PARAM
static int yk3_get_rxfh(struct net_device *ndev, struct ethtool_rxfh_param *param)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	mutex_lock(&ndev_priv->state_mlock);

	if (param->indir)
		yk3_rss_indir_table_get(ndev, param->indir);

	if (param->hfunc)
		param->hfunc = ETH_RSS_HASH_TOP;

	if (param->key)
		yk3_rss_key_get(ndev, param->key);

	mutex_unlock(&ndev_priv->state_mlock);

	return 0;
}
#else
static int yk3_get_rxfh(struct net_device *ndev, u32 *indir, u8 *key, u8 *hfunc)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	mutex_lock(&ndev_priv->state_mlock);
	if (indir)
		yk3_rss_indir_table_get(ndev, indir);

	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	if (key)
		yk3_rss_key_get(ndev, key);

	mutex_unlock(&ndev_priv->state_mlock);

	return 0;
}
#endif /* YK3_HAVE_ETHTOOL_GET_RXFH_PARAM */

#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_PARAM
static int yk3_set_rxfh(struct net_device *ndev,
			struct ethtool_rxfh_param *param,
			struct netlink_ext_ack *extack)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (param->hfunc != ETH_RSS_HASH_NO_CHANGE && param->hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;

	mutex_lock(&ndev_priv->state_mlock);
	if (param->key)
		yk3_rss_key_set(ndev, param->key);

	if (param->indir)
		yk3_rss_indir_table_set(ndev, param->indir, ndev->real_num_rx_queues);

	mutex_unlock(&ndev_priv->state_mlock);

	return 0;
}
#else
static int yk3_set_rxfh(struct net_device *ndev, const u32 *indir,
			const u8 *key, const u8 hfunc)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;

	mutex_lock(&ndev_priv->state_mlock);
	if (key)
		yk3_rss_key_set(ndev, key);

	if (indir)
		yk3_rss_indir_table_set(ndev, indir, ndev->real_num_rx_queues);
	mutex_unlock(&ndev_priv->state_mlock);

	return 0;
}
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_PARAM */

static int yk3_get_rxnfc_eth(struct net_device *ndev,
			     struct ethtool_rxnfc *info,
			     u32 *rule_locs __always_unused)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u32 yk3_flow_type;
	u8 flow_config;
	int ret = 0;

	switch (info->cmd) {
#ifndef YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS
	case ETHTOOL_GRXFH:
		switch (info->flow_type) {
		case TCP_V4_FLOW:
			yk3_flow_type = YK3_RXFH_TCP_V4_FLOW;
			break;
		case TCP_V6_FLOW:
			yk3_flow_type = YK3_RXFH_TCP_V6_FLOW;
			break;
		case UDP_V4_FLOW:
			yk3_flow_type = YK3_RXFH_UDP_V4_FLOW;
			break;
		case UDP_V6_FLOW:
			yk3_flow_type = YK3_RXFH_UDP_V6_FLOW;
			break;
		default:
			return -EOPNOTSUPP;
		}
		/* compatible with kylin*/
		info->data = 0;
		flow_config = yk3_get_rxfh_conf(ndev_priv, yk3_flow_type);
		if (flow_config & YK3_RXH_IP_SRC)
			info->data |= RXH_IP_SRC;
		if (flow_config & YK3_RXH_IP_DST)
			info->data |= RXH_IP_DST;
		if (flow_config & YK3_RXH_L4_SRC)
			info->data |= RXH_L4_B_0_1;
		if (flow_config & YK3_RXH_L4_DST)
			info->data |= RXH_L4_B_2_3;
		if (flow_config & YK3_RXH_L3_PROTO)
			info->data |= RXH_L3_PROTO;
		break;
#endif /* YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS */
	case ETHTOOL_GRXRINGS:
		info->data = ndev->real_num_rx_queues;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		info->rule_cnt = 0;
		break;
	case ETHTOOL_GRXCLSRULE:
		ret = -EOPNOTSUPP;
		break;
	case ETHTOOL_GRXCLSRLALL:
		info->data = 0;
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static int yk3_set_rxnfc_eth(struct net_device *ndev,
			     struct ethtool_rxnfc *rxnfc)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u32 yk3_flow_type;
	u8 fields = 0;
	int ret = 0;

	switch (rxnfc->cmd) {
#ifndef YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS
	case ETHTOOL_SRXFH:
		switch (rxnfc->flow_type) {
		case TCP_V4_FLOW:
			yk3_flow_type = YK3_RXFH_TCP_V4_FLOW;
			break;
		case TCP_V6_FLOW:
			yk3_flow_type = YK3_RXFH_TCP_V6_FLOW;
			break;
		case UDP_V4_FLOW:
			yk3_flow_type = YK3_RXFH_UDP_V4_FLOW;
			break;
		case UDP_V6_FLOW:
			yk3_flow_type = YK3_RXFH_UDP_V6_FLOW;
			break;
		default:
			return -EOPNOTSUPP;
		}
		if (rxnfc->data & RXH_IP_SRC)
			fields |= YK3_RXH_IP_SRC;
		if (rxnfc->data & RXH_IP_DST)
			fields |= YK3_RXH_IP_DST;
		if (rxnfc->data & RXH_L4_B_0_1)
			fields |= YK3_RXH_L4_SRC;
		if (rxnfc->data & RXH_L4_B_2_3)
			fields |= YK3_RXH_L4_DST;
		if (rxnfc->data & RXH_L3_PROTO)
			fields |= YK3_RXH_L3_PROTO;

		/* if rxhash disable only save config */
		if (ndev->features & NETIF_F_RXHASH)
			ret = yk3_lan_ethtool_set_rxnfc(ndev, yk3_flow_type, fields);

		if (ret) {
			yk3_net_err("failed to set rss flow %d hash field!",
				    rxnfc->flow_type);
			ret = -EFAULT;
		} else {
			yk3_set_rxfh_conf(ndev_priv, yk3_flow_type, fields);
		}
		break;
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS */
	case ETHTOOL_SRXCLSRLINS:
		ret = -EOPNOTSUPP;
		break;
	case ETHTOOL_SRXCLSRLDEL:
		ret = -EOPNOTSUPP;
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

#ifdef YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS
static int yk3_get_rxfh_fields(struct net_device *ndev,
			       struct ethtool_rxfh_fields *info)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u32 yk3_flow_type;
	u8 flow_config;

	switch (info->flow_type) {
	case TCP_V4_FLOW:
		yk3_flow_type = YK3_RXFH_TCP_V4_FLOW;
		break;
	case TCP_V6_FLOW:
		yk3_flow_type = YK3_RXFH_TCP_V6_FLOW;
		break;
	case UDP_V4_FLOW:
		yk3_flow_type = YK3_RXFH_UDP_V4_FLOW;
		break;
	case UDP_V6_FLOW:
		yk3_flow_type = YK3_RXFH_UDP_V6_FLOW;
		break;
	default:
		return -EOPNOTSUPP;
	}
	flow_config = yk3_get_rxfh_conf(ndev_priv, yk3_flow_type);
	if (flow_config & YK3_RXH_IP_SRC)
		info->data |= RXH_IP_SRC;
	if (flow_config & YK3_RXH_IP_DST)
		info->data |= RXH_IP_DST;
	if (flow_config & YK3_RXH_L4_SRC)
		info->data |= RXH_L4_B_0_1;
	if (flow_config & YK3_RXH_L4_DST)
		info->data |= RXH_L4_B_2_3;
	if (flow_config & YK3_RXH_L3_PROTO)
		info->data |= RXH_L3_PROTO;

	return 0;
}
#endif /* YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS */

#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS
static int yk3_set_rxfh_fields(struct net_device *ndev,
			       const struct ethtool_rxfh_fields *nfc,
			       struct netlink_ext_ack *extack)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u32 yk3_flow_type;
	u8 fields = 0;
	int ret = 0;

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
		yk3_flow_type = YK3_RXFH_TCP_V4_FLOW;
		break;
	case TCP_V6_FLOW:
		yk3_flow_type = YK3_RXFH_TCP_V6_FLOW;
		break;
	case UDP_V4_FLOW:
		yk3_flow_type = YK3_RXFH_UDP_V4_FLOW;
		break;
	case UDP_V6_FLOW:
		yk3_flow_type = YK3_RXFH_UDP_V6_FLOW;
		break;
	default:
		return -EOPNOTSUPP;
	}

	if (nfc->data & RXH_IP_SRC)
		fields |= YK3_RXH_IP_SRC;
	if (nfc->data & RXH_IP_DST)
		fields |= YK3_RXH_IP_DST;
	if (nfc->data & RXH_L4_B_0_1)
		fields |= YK3_RXH_L4_SRC;
	if (nfc->data & RXH_L4_B_2_3)
		fields |= YK3_RXH_L4_DST;
	if (nfc->data & RXH_L3_PROTO)
		fields |= YK3_RXH_L3_PROTO;

	/* if rxhash disable only save config */
	if (ndev->features & NETIF_F_RXHASH)
		ret = yk3_lan_ethtool_set_rxnfc(ndev, yk3_flow_type, fields);

	if (ret) {
		yk3_net_err("failed to set rss flow %d hash field!",
			    nfc->flow_type);
		return -EFAULT;
	}
	yk3_set_rxfh_conf(ndev_priv, yk3_flow_type, fields);

	return 0;
}
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS */

static int yk3_get_eeprom_len(struct net_device *ndev)
{
	return -EOPNOTSUPP;
}

static int yk3_get_eeprom(struct net_device *ndev,
			  struct ethtool_eeprom *eeep, u8 *data)
{
	return -EOPNOTSUPP;
}

static int yk3_set_eeprom(struct net_device *ndev,
			  struct ethtool_eeprom *eeep, u8 *data)
{
	return -EOPNOTSUPP;
}

static int yk3_set_phys_id(struct net_device *ndev,
			   enum ethtool_phys_id_state state)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	enum yk3_mac_channel mac_ch;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = pdev_priv->mac->mac_ch;

	yk3_net_debug("state %u mac_ch %u\n", state, mac_ch);

	switch (state) {
	case ETHTOOL_ID_INACTIVE:
		msg.opcode = YK3_MBOX_OPCODE_EMP_DISABLE_BLINK;
		break;
	case ETHTOOL_ID_ACTIVE:
		msg.opcode = YK3_MBOX_OPCODE_EMP_ENABLE_BLINK;
		break;
	default:
		yk3_net_err("%s state %u not supported, mac_ch %u\n", __func__, state, mac_ch);
		return -EINVAL;
	}

	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 1;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_net_err("send mbox msg err\n");
		return -EIO;
	}
	if (ack_msg.data[0]) {
		yk3_net_err("mbox resp result 0x%02x reason 0x%02x\n",
			    ack_msg.data[0], ack_msg.data[1]);
		return -EIO;
	}
	return 0;
}

static int yk3_get_module_info(struct net_device *ndev,
			       struct ethtool_modinfo *modinfo)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	enum yk3_mac_channel mac_ch;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = pdev_priv->mac->mac_ch;

	yk3_net_debug("get module info, cmd %u mac_ch %u\n", modinfo->cmd, mac_ch);

	msg.opcode = YK3_MBOX_OPCODE_EMP_GET_MODULE_BASIC;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 1;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_net_err("send mbox msg err\n");
		return -EIO;
	}

	switch (ack_msg.data[0]) {
	case YK3_MODULE_ID_QSFP:
		modinfo->type = ETH_MODULE_SFF_8436;
		modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
		break;
	case YK3_MODULE_ID_QSFP_PLUS:
	case YK3_MODULE_ID_QSFP28:
		if (ack_msg.data[0] == YK3_MODULE_ID_QSFP28 || ack_msg.data[1] >= 0x3) {
			modinfo->type = ETH_MODULE_SFF_8636;
			modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
		} else {
			modinfo->type = ETH_MODULE_SFF_8436;
			modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
		}
		break;
	case YK3_MODULE_ID_SFP:
		modinfo->type = ETH_MODULE_SFF_8472;
		modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
		break;
	default:
		yk3_net_err("%s: cable type not recognized: 0x%x\n", __func__, ack_msg.data[0]);
		return -EINVAL;
	}

	yk3_net_debug("get module info, modtype %u modeeprom_len %u\n",
		      modinfo->type, modinfo->eeprom_len);

	return 0;
}

static int yk3_get_module_eeprom(struct net_device *ndev,
				 struct ethtool_eeprom *ee, u8 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	u32 offset, len;
	u32 read_len = 0;
	const u8 room = YK3_MBOX_DATA_LEN - 4;
	u8 req_len, response_len;
	u16 remain_len;
	u8 mac_ch;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	if (!ee || !ee->len || !data) {
		yk3_net_err("%s Invalid argument\n", __func__);
		return -EINVAL;
	}

	offset = ee->offset;
	len = ee->len;
	mac_ch = pdev_priv->mac->mac_ch;

	yk3_net_debug("get module eeprom, cmd %u ee->offset %u ee->len %u  mac_ch %u\n",
		      ee->cmd, ee->offset, ee->len, mac_ch);

	msg.opcode = YK3_MBOX_OPCODE_EMP_GET_EERPOM;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 4;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	memset(data, 0, ee->len);

	while (len > 0) {
		req_len = (len > room) ? room : len;

		msg.data[1] = req_len;
		put_unaligned_le16(offset, &msg.data[2]);

		memset(&ack_msg, 0, sizeof(ack_msg));
		if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
			yk3_net_err("get module eeprom, emp response error\n");
			return -EIO;
		}
		response_len = ack_msg.data[1];
		remain_len = le16_to_cpu(get_unaligned((__le16 *)&ack_msg.data[2]));

		if (response_len > req_len) {
			yk3_net_err("get module eeprom, exceeds, req_len %u response_len %u\n",
				    req_len, response_len);
			return -EIO;
		}

		yk3_net_debug("get module eeprom, req_len %u response_len %u remain_len %u\n",
			      req_len, response_len, remain_len);

		memcpy(data + read_len, &ack_msg.data[4], response_len);
		len -= response_len;
		offset += response_len;
		read_len += response_len;

		if (len > remain_len) {
			yk3_net_err("get module eeprom ERROR, len %u remain_len %u\n",
				    len, remain_len);
			return -EIO;
		}

		yk3_net_debug("get module eeprom, len %u offset %u read_len %u\n",
			      len, offset, read_len);
	}

	return 0;
}

static int yk3_get_fecparam(struct net_device *ndev, struct ethtool_fecparam *fp)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	void __iomem *addr;
	enum yk3_mac_channel mac_ch;
	u32 fec_mode;
	u8 link_fec_cfg;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	link_fec_cfg = ndev_priv->link_fec_cfg;

	switch (link_fec_cfg) {
	case YK3_OFFFEC:
		fp->fec = ETHTOOL_FEC_OFF;
		break;
	case YK3_FCFEC:
		fp->fec = ETHTOOL_FEC_BASER;
		break;
	case YK3_RSFEC:
		fp->fec = ETHTOOL_FEC_RS;
		break;
	case YK3_AUTOFEC:
		fp->fec = ETHTOOL_FEC_AUTO;
		break;
	default:
		fp->fec = ETHTOOL_FEC_AUTO;
		break;
	}

	if (netif_carrier_ok(ndev_priv->ndev)) {
		addr = pdev_priv->bar_addr[YK3_BAR0];
		mac_ch = pdev_priv->mac->mac_ch;
		fec_mode = yk3_rd32(addr, UMAC_CHMODE_L(0, mac_ch)) & CHMODE_MODE_MASK;
		switch (fec_mode) {
		case MAC_MODE_SPEED_10GBASE:
		case MAC_MODE_SPEED_25GBASE:
		case MAC_MODE_SPEED_40GBASE:
		case MAC_MODE_SPEED_100GBASE:
			fp->active_fec = ETHTOOL_FEC_OFF;
			break;
		case MAC_MODE_SPEED_10GBASE_FC:
		case MAC_MODE_SPEED_25GBASE_FC:
		case MAC_MODE_SPEED_40GBASE_FC:
			fp->active_fec = ETHTOOL_FEC_BASER;
			break;
		case MAC_MODE_SPEED_25GBASE_RS_IEEE:
		case MAC_MODE_SPEED_25GBASE_RS_CONS:
		case MAC_MODE_SPEED_100GBASE_RS:
			fp->active_fec = ETHTOOL_FEC_RS;
			break;
		default:
			yk3_net_err("%s error, fec_mode %u\n", __func__, fec_mode);
			break;
		}
	} else {
		fp->active_fec = ETHTOOL_FEC_NONE;
	}

	return 0;
}

static int yk3_set_fecparam(struct net_device *ndev, struct ethtool_fecparam *fp)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	struct yk3_mac *mac = pdev_priv->mac;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	u8 fec_cfg;
	u8 porttype;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!card || !mac)
		return -EIO;

	switch (fp->fec) {
	case ETHTOOL_FEC_OFF:
		fec_cfg = YK3_OFFFEC;
		break;
	case ETHTOOL_FEC_BASER:
		fec_cfg = YK3_FCFEC;
		break;
	case ETHTOOL_FEC_RS:
		fec_cfg = YK3_RSFEC;
		break;
	case ETHTOOL_FEC_AUTO:
		fec_cfg = YK3_AUTOFEC;
		break;
	default:
		yk3_net_err("%s error, fec 0x%x\n", __func__, fp->fec);
		return -EOPNOTSUPP;
	}

	yk3_net_debug("cmd %u active_fec %u fec %u fec_cfg %u\n",
		      fp->cmd, fp->active_fec, fp->fec, fec_cfg);

	porttype = pdev_priv->card->port_type;
	switch (porttype) {
	case YK3_10G:
		if (fec_cfg != YK3_AUTOFEC &&
		    fec_cfg != YK3_OFFFEC &&
		    fec_cfg != YK3_FCFEC) {
			yk3_net_err("not supported, porttype %u fec_cfg %u\n",
				    porttype, fec_cfg);
			return -EOPNOTSUPP;
		}
		break;
	case YK3_25G:
		if (fec_cfg != YK3_AUTOFEC &&
		    fec_cfg != YK3_OFFFEC &&
		    fec_cfg != YK3_FCFEC &&
		    fec_cfg != YK3_RSFEC) {
			yk3_net_err("not supported, porttype %u fec_cfg %u\n",
				    porttype, fec_cfg);
			return -EOPNOTSUPP;
		}
		break;
	case YK3_40G:
		if (fec_cfg != YK3_AUTOFEC &&
		    fec_cfg != YK3_OFFFEC &&
		    fec_cfg != YK3_FCFEC) {
			yk3_net_err("not supported, porttype %u fec_cfg %u\n",
				    porttype, fec_cfg);
			return -EOPNOTSUPP;
		}
		break;
	case YK3_100G:
		if (fec_cfg != YK3_AUTOFEC &&
		    fec_cfg != YK3_OFFFEC &&
		    fec_cfg != YK3_RSFEC) {
			yk3_net_err("not supported, porttype %u fec_cfg %u\n",
				    porttype, fec_cfg);
			return -EOPNOTSUPP;
		}
		break;
	default:
		yk3_net_err("card porttype error, %u\n", porttype);
		return -EOPNOTSUPP;
	}

	msg.opcode = YK3_MBOX_OPCODE_EMP_SET_FEC;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac->mac_ch;
	msg.data[1] = fec_cfg;
	msg.data_length = 2;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_net_err("send mbox msg err\n");
		return -EIO;
	}
	if (ack_msg.data[0]) {
		yk3_net_err("mbox resp result 0x%02x reason 0x%02x\n",
			    ack_msg.data[0], ack_msg.data[1]);
		return -EIO;
	}
	ndev_priv->link_fec_cfg = fec_cfg;
	mac->fec_cfg = fec_cfg;

	return 0;
}

static void yk3_build_advertising(struct yk3_ethtool_ksetting *cmd,
				  enum ethtool_link_mode_bit_indices link_mode)
{
	const unsigned int modes = link_mode;
	unsigned int bit, idx;

	bit = modes % 64;
	idx = modes / 64;
	__set_bit(bit, &cmd->advertising[idx]);
}

static void yk3_build_supported(struct yk3_ethtool_ksetting *cmd,
				enum ethtool_link_mode_bit_indices link_mode)
{
	const unsigned int modes = link_mode;
	unsigned int bit, idx;

	bit = modes % 64;
	idx = modes / 64;
	__set_bit(bit, &cmd->supported[idx]);
}

static void yk3_mac_get_supported_advertising(struct net_device *ndev,
					      struct yk3_ethtool_ksetting *cmd)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	u16 port_type;
	u32 cfg_speed;
	u8 cfg_speed_autoneg;
	u8 cfg_fec;

	if (!card)
		return;

	port_type = pdev_priv->card->port_type;
	cfg_speed = card->link_speed_cfg;
	cfg_speed_autoneg = card->link_speed_cfg_autoneg;
	cfg_fec = ndev_priv->link_fec_cfg;

	switch (port_type) {
	case YK3_10G:
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv)) {
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
		}
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
		if (cfg_speed_autoneg == AUTONEG_ENABLE)
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv))
			switch (cfg_fec) {
			case YK3_OFFFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				break;
			case YK3_FCFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				break;
			case YK3_AUTOFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				break;
			default:
				break;
			}
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		break;
	case YK3_25G:
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv)) {
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
		}
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		switch (cfg_speed) {
		case SPEED_25G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			break;
		case SPEED_10G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			break;
		default:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			break;
		}
		if (cfg_speed_autoneg == AUTONEG_ENABLE)
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv))
			switch (cfg_fec) {
			case YK3_OFFFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				break;
			case YK3_FCFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				break;
			case YK3_RSFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
				break;
			case YK3_AUTOFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
				break;
			default:
				break;
			}
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		break;
	case YK3_40G:
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv)) {
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
		}
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		switch (cfg_speed) {
		case SPEED_40G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
			break;
		case SPEED_25G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			break;
		case SPEED_10G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			break;
		default:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
			break;
		}
		if (cfg_speed_autoneg == AUTONEG_ENABLE)
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv))
			switch (cfg_fec) {
			case YK3_OFFFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				break;
			case YK3_FCFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				break;
			case YK3_AUTOFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_BASER_BIT);
				break;
			default:
				break;
			}
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		break;
	case YK3_100G:
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv)) {
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
			yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
		}
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_supported(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		switch (cfg_speed) {
		case SPEED_100G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT);
			break;
		case SPEED_40G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
			break;
		case SPEED_25G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			break;
		case SPEED_10G:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			break;
		default:
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_10000baseLR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseCR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseKR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_25000baseSR_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT);
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT);
			break;
		}
		if (cfg_speed_autoneg == AUTONEG_ENABLE)
			yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Autoneg_BIT);
		if (yk3_ndev_is_pf(ndev_priv))
			switch (cfg_fec) {
			case YK3_OFFFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				break;
			case YK3_RSFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
				break;
			case YK3_AUTOFEC:
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_NONE_BIT);
				yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FEC_RS_BIT);
				break;
			default:
				break;
			}
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_FIBRE_BIT);
		yk3_build_advertising(cmd, ETHTOOL_LINK_MODE_Pause_BIT);
		break;
	default:
		break;
	}
}

#ifdef YK3_HAVE_ETHTOOL_GET_LINK_SETTING
static int yk3_get_link_ksettings(struct net_device *ndev,
				  struct ethtool_link_ksettings *ksettings)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	struct yk3_ethtool_ksetting cmd;

	yk3_net_debug("cmd %u type %u pid %d comm %s\n",
		      ksettings->base.cmd, ndev_priv->type, current->pid, current->comm);

	memset(&cmd, 0, sizeof(struct yk3_ethtool_ksetting));
	ethtool_link_ksettings_zero_link_mode(ksettings, supported);
	ethtool_link_ksettings_zero_link_mode(ksettings, advertising);
	ethtool_link_ksettings_zero_link_mode(ksettings, lp_advertising);

	yk3_mac_get_supported_advertising(ndev, &cmd);
	bitmap_copy(ksettings->link_modes.supported,
		    cmd.supported,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_copy(ksettings->link_modes.advertising,
		    cmd.advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_copy(ksettings->link_modes.lp_advertising,
		    cmd.lp_advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	if (netif_carrier_ok(ndev_priv->ndev) && netif_running(ndev_priv->ndev)) {
		ksettings->base.speed = ndev_priv->link_speed;
		ksettings->base.duplex = DUPLEX_FULL;
		ksettings->base.port = ndev_priv->port;
	} else {
		ksettings->base.speed = SPEED_UNKNOWN;
		ksettings->base.duplex = DUPLEX_UNKNOWN;
		ksettings->base.port = PORT_NONE;
	}
	ksettings->base.autoneg = ndev_priv->link_speed_cfg_autoneg;
	if (card)
		ksettings->base.autoneg = card->link_speed_cfg_autoneg;
	return 0;
}
#else
static int yk3_get_link_ksettings(struct net_device *ndev,
				  struct ethtool_cmd *ksettings)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	struct yk3_ethtool_ksetting cmd;

	yk3_net_debug("cmd %u type %u pid %d comm %s\n",
		      ksettings->cmd, ndev_priv->type, current->pid, current->comm);

	memset(&cmd, 0, sizeof(struct yk3_ethtool_ksetting));
	ksettings->supported = 0;
	ksettings->advertising = 0;
	ksettings->lp_advertising = 0;
	yk3_mac_get_supported_advertising(ndev, &cmd);
	memcpy(&ksettings->supported, cmd.supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
	memcpy(&ksettings->advertising, cmd.advertising, __ETHTOOL_LINK_MODE_MASK_NBITS);
	memcpy(&ksettings->lp_advertising, cmd.lp_advertising, __ETHTOOL_LINK_MODE_MASK_NBITS);
	if (netif_carrier_ok(ndev_priv->ndev) && netif_running(ndev_priv->ndev)) {
		ksettings->speed = ndev_priv->link_speed;
		ksettings->duplex = DUPLEX_FULL;
		ksettings->port = ndev_priv->port;
	} else {
		ksettings->speed = SPEED_UNKNOWN;
		ksettings->duplex = DUPLEX_UNKNOWN;
		ksettings->port = PORT_NONE;
	}
	ksettings->autoneg = ndev_priv->link_speed_cfg_autoneg;
	if (card)
		ksettings->base.autoneg = card->link_speed_cfg_autoneg;
	return 0;
}
#endif /* YK3_HAVE_ETHTOOL_GET_LINK_SETTING */

static int yk3_set_link_ksettings(struct net_device *ndev,
				  const struct ethtool_link_ksettings *ksettings)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	struct yk3_mac *mac = pdev_priv->mac;
	bool autoneg_enable = ksettings->base.autoneg;
	u32 duplex = ksettings->base.duplex;
	u32 speed = ksettings->base.speed;
	u8 porttype;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!card || !mac)
		return -EIO;

	yk3_net_debug("cmd %u speed %u duplex %u autoneg %u\n",
		      ksettings->base.cmd,
		      speed,
		      duplex,
		      autoneg_enable);
	yk3_net_debug("speed_cfg %u cfg_autoneg %u\n",
		      ndev_priv->link_speed_cfg,
		      ndev_priv->link_speed_cfg_autoneg);

	/*
	 * When autoneg is on, speed/duplex is ignored and negotiation mode is used;
	 * When autoneg is off, the forced speed must be configured.
	 */
	porttype = pdev_priv->card->port_type;
	if (autoneg_enable == AUTONEG_ENABLE) {
		/* Filter not supported speed */
		switch (porttype) {
		case YK3_10G:
			if (speed != SPEED_10G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_25G:
			if (speed != SPEED_10G && speed != SPEED_25G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_40G:
			if (speed != SPEED_10G && speed != SPEED_25G &&
			    speed != SPEED_40G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_100G:
			if (speed != SPEED_10G && speed != SPEED_25G &&
			    speed != SPEED_40G && speed != SPEED_100G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		default:
			yk3_net_err("card porttype error, %u\n", porttype);
			return -EOPNOTSUPP;
		}
		if (yk3_mac_set_link_autoneg(ndev_priv)) {
			yk3_net_err("failed to set link autoneg\n");
			return -EIO;
		}
		ndev_priv->link_speed_cfg = SPEED_UNKNOWN;
		ndev_priv->link_speed_cfg_autoneg = AUTONEG_ENABLE;
		mac->speed_cfg = SPEED_UNKNOWN;
		mac->speed_cfg_autoneg = AUTONEG_ENABLE;
		card->link_speed_cfg = SPEED_UNKNOWN;
		card->link_speed_cfg_autoneg = AUTONEG_ENABLE;
	} else if (autoneg_enable == AUTONEG_DISABLE) {
		/*
		 * If the interface is linked, even if the command does not include speed,
		 * speed is not unknown, but the current link speed. Autoneg is also
		 */
		/* Filter incorrect configurations */
		if (speed == SPEED_UNKNOWN) {
			yk3_net_err("not supported, autoneg off must specify speed\n");
			return -EOPNOTSUPP;
		}
		if (duplex != DUPLEX_FULL && duplex != DUPLEX_UNKNOWN) {
			yk3_net_err("not supported, duplex %u\n", duplex);
			return -EOPNOTSUPP;
		}
		switch (porttype) {
		case YK3_10G:
			if (speed != SPEED_10G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_25G:
			if (speed != SPEED_10G && speed != SPEED_25G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_40G:
			if (speed != SPEED_10G && speed != SPEED_25G &&
			    speed != SPEED_40G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		case YK3_100G:
			if (speed != SPEED_10G && speed != SPEED_25G &&
			    speed != SPEED_40G && speed != SPEED_100G) {
				yk3_net_err("not supported, porttype %u speed %u\n",
					    porttype, speed);
				return -EOPNOTSUPP;
			}
			break;
		default:
			yk3_net_err("card porttype error, %u\n", porttype);
			return -EOPNOTSUPP;
		}

		/* set speed */
		if (yk3_mac_set_link_speed(ndev_priv, speed)) {
			yk3_net_err("failed to set link speed %u\n", speed);
			return -EIO;
		}
		ndev_priv->link_speed_cfg = speed;
		ndev_priv->link_speed_cfg_autoneg = AUTONEG_DISABLE;
		mac->speed_cfg = speed;
		mac->speed_cfg_autoneg = AUTONEG_DISABLE;
		card->link_speed_cfg = speed;
		card->link_speed_cfg_autoneg = AUTONEG_DISABLE;
		yk3_np_tm_speed_limit(pdev_priv, speed);
		yk3_qos_set_link_speed(pdev_priv, speed);
		yk3_edma_l2_vq(pdev_priv, speed);
	}

	return 0;
}

static void yk3_self_test(struct net_device *ndev, struct ethtool_test *eth_test, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);

	yk3_net_info("Self test out: status flags(0x%x)\n", eth_test->flags);

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	if (eth_test->flags == ETH_TEST_FL_OFFLINE)
		yk3_mac_self_offline_test(ndev, eth_test, data);
}

#ifdef YK3_HAVE_KERNEL_RING
static void yk3_get_ringparam(struct net_device *ndev,
			      struct ethtool_ringparam *ring,
			      struct kernel_ethtool_ringparam *kring,
			      struct netlink_ext_ack *ext_ack)
#else
static void yk3_get_ringparam(struct net_device *ndev,
			      struct ethtool_ringparam *ring)
#endif /* YK3_HAVE_KERNEL_RING */
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	ring->rx_max_pending = YK3_N_MAX_QDEPTH;
	ring->tx_max_pending = YK3_N_MAX_QDEPTH;

	ring->rx_pending = ndev_priv->rxq_depth;
	ring->tx_pending = ndev_priv->txq_depth;

	ndev->gso_max_segs = min_t(u16, YK3_N_TSO_MAXSEGS, (ndev_priv->txq_depth >> 2));
	if ((ndev->features & (NETIF_F_TSO | NETIF_F_TSO6)) != (NETIF_F_TSO | NETIF_F_TSO6)) {
		if (ndev_priv->txq_depth <= 128)
			ndev->gso_max_segs >>= 2;
	}
}

#ifdef YK3_HAVE_KERNEL_RING
static int yk3_set_ringparam(struct net_device *ndev,
			     struct ethtool_ringparam *ring,
			     struct kernel_ethtool_ringparam *kring,
			     struct netlink_ext_ack *ext_ack)
#else
static int yk3_set_ringparam(struct net_device *ndev,
			     struct ethtool_ringparam *ring)
#endif /* YK3_HAVE_KERNEL_RING */
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	bool running = netif_running(ndev);
	u8 log_tx_size;
	u8 log_rx_size;
	int ret;

	if (ring->rx_jumbo_pending || ring->rx_mini_pending)
		return -EINVAL;

	if (ring->rx_pending < YK3_N_MIN_QDEPTH ||
	    ring->rx_pending > YK3_N_MAX_QDEPTH)
		return -EINVAL;

	if (ring->tx_pending < YK3_N_MIN_QDEPTH ||
	    ring->tx_pending > YK3_N_MAX_QDEPTH)
		return -EINVAL;

	if (running && ndev->netdev_ops->ndo_stop) {
		ndev->netdev_ops->ndo_stop(ndev);
		ndev->flags &= ~IFF_UP;
	}

	log_tx_size = order_base_2(ring->tx_pending);
	log_rx_size = order_base_2(ring->rx_pending);

	mutex_lock(&ndev_priv->state_mlock);
	ndev_priv->rxq_depth = (1 << log_rx_size);
	ndev_priv->txq_depth = (1 << log_tx_size);
	if (ndev_priv->rx_max_coalesced_frames > (ndev_priv->rxq_depth >> 1))
		ndev_priv->rx_max_coalesced_frames = (ndev_priv->rxq_depth >> 1);
	if (ndev_priv->tx_max_coalesced_frames > (ndev_priv->txq_depth >> 1))
		ndev_priv->tx_max_coalesced_frames = (ndev_priv->txq_depth >> 1);
	yk3_edma_destroy_queues(ndev_priv);
	ret = yk3_edma_create_queues(ndev_priv);
	mutex_unlock(&ndev_priv->state_mlock);
	if (ret) {
		yk3_net_err("failed to set ringparam(on create queues)");
		return ret;
	}

	if (running && ndev->netdev_ops->ndo_open) {
		ret = ndev->netdev_ops->ndo_open(ndev);
		ndev->flags |= IFF_UP;
	}

	return ret;
}

static int yk3_get_regs_len(struct net_device *ndev)
{
	return -EOPNOTSUPP;
}

static void yk3_get_regs(struct net_device *ndev,
			 struct ethtool_regs *regs, void *p)
{
}

static void yk3_get_channels(struct net_device *ndev,
			     struct ethtool_channels *ch)
{
	ch->max_combined = ndev->num_rx_queues;
	ch->combined_count = ndev->real_num_tx_queues;
}

static int yk3_set_channels(struct net_device *ndev,
			    struct ethtool_channels *ch)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret;
	bool running = netif_running(ndev);

	if (!ch->combined_count)
		return -EINVAL;

	if (ch->combined_count > ndev->num_rx_queues)
		return -EINVAL;

	if (ch->other_count || ch->tx_count || ch->rx_count)
		return -EINVAL;

	if (yk3_ndev_is_pf(ndev_priv) && test_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags)) {
		yk3_net_err("Cannot change channels with tc configured.");
		return -EPERM;
	}

	if (running && ndev->netdev_ops->ndo_stop) {
		ndev->netdev_ops->ndo_stop(ndev);
		ndev->flags &= ~IFF_UP;
	}

	mutex_lock(&ndev_priv->state_mlock);
	yk3_edma_destroy_queues(ndev_priv);
	netif_set_real_num_tx_queues(ndev, ch->combined_count);
	netif_set_real_num_rx_queues(ndev, ch->combined_count);
	ret = yk3_edma_create_queues(ndev_priv);
	mutex_unlock(&ndev_priv->state_mlock);
	if (ret) {
		yk3_net_err("failed to set channel(on create queues)");
		return ret;
	}

	ret = yk3_rss_indir_table_init(ndev_priv, (u16)ndev->real_num_rx_queues);
	if (ret) {
		yk3_net_err("failed to set channel(on init rss-indir table)");
		return ret;
	}

	if (yk3_ndev_is_pf(ndev_priv) && test_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags))
		yk3_ets_set_tc_default(ndev, ndev_priv->etscfg);

	if (running && ndev->netdev_ops->ndo_open) {
		ret = ndev->netdev_ops->ndo_open(ndev);
		ndev->flags |= IFF_UP;
	}

	return ret;
}

int yk3_get_fc_xon(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13a3008 + 0x40 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_get_fc_xoff(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13a300c + 0x40 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_set_fc_xon(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	yk3_wr32(hw_addr, 0x13a3008 + 0x40 * pdev_priv->mac->mac_ch, val);

	return 0;
}

int yk3_set_fc_xoff(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	yk3_wr32(hw_addr, 0x13a300c + 0x40 * pdev_priv->mac->mac_ch, val);

	return 0;
}

static int yk3_np_set_fc(struct net_device *ndev, u8 mac_ch, bool enabled)
{
	int i;
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u32 port_speed;
	u8 port_num;
	u32 base;
	u32 mac_fc_xoff;
	u32 mac_fc_xon;
	u32 size;

	if (!pdev_priv)
		return -ENODEV;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	port_speed = ndev_priv->link_speed;
	port_num = pdev_priv->card->pf_num;

	if (port_speed == 25000 && port_num == 2)
		size = 96;
	else if (port_speed == 25000 && port_num == 4)
		size = 192;
	else
		size = 128;

	base = port_speed / (size * 8);

	mac_fc_xoff = 1536 / port_num - 320 - 3 * base;
	mac_fc_xon = mac_fc_xoff * 7 / 10;

	if (enabled) {
		for (i = 0; i < 32; i++)
			yk3_wr32(hw_addr, 0x14b1000 + 4 * mac_ch, 0x0); // L1 reserve
		yk3_wr32(hw_addr, 0x14b040c + 0x10 * mac_ch, (u32)(0xff << (8 * mac_ch)));

		if (yk3_set_fc_xon(ndev, mac_fc_xon) || yk3_set_fc_xoff(ndev, mac_fc_xoff)) {
			yk3_net_err("Set np fc xon / xoff failed.\n");
			return -EINVAL;
		}

		for (i = 0; i < 4; i++)
			yk3_wr32(hw_addr, 0x14b0600 + 4 * i, mac_fc_xoff + 0x90);  //mac l2 ovf

		/* edma pfc buffer and pkt threshold */
		for (i = 0; i < 32; i++) {
			yk3_wr32(hw_addr, 0x03a4 + 4 * i, 0x200);
			yk3_wr32(hw_addr, 0x0424 + 4 * i, 0x100);
		}
	} else {
		for (i = 0; i < 4; i++)
			yk3_wr32(hw_addr, 0x14b0600 + 4 * i, 0);  //mac l2 ovf

		for (i = 0; i < 32; i++) {
			yk3_wr32(hw_addr, 0x03a4 + 4 * i, 0x1800);
			yk3_wr32(hw_addr, 0x0424 + 4 * i, 0xd00);
		}
	}

	yk3_wr32(hw_addr, 0x13a3000 + 0x40 * mac_ch, enabled);  // FC enabled
	yk3_wr32(hw_addr, 0x13e3000, enabled);  //L2 enable

	return 0;
}

int yk3_clear_fc_register(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (!pdev_priv || !pdev_priv->card)
		return -ENODEV;

	ret = yk3_mac_set_fc(ndev_priv, pdev_priv->card->mac_chs[pdev_priv->pf_id], 0);
	if (ret)
		yk3_net_warn("Set umac fc failed, ret=%d\n", ret);

	ret = yk3_np_set_fc(ndev_priv->ndev, pdev_priv->card->mac_chs[pdev_priv->pf_id], false);
	if (ret)
		yk3_net_warn("Set np fc failed, ret=%d\n", ret);

	return ret;
}

static void yk3_get_pauseparam(struct net_device *ndev,
			       struct ethtool_pauseparam *pause)
{
	void __iomem *hw_addr;
	u32 tx_pause_mode = 0, rx_pause_mode = 0;
	u32 value = 0, value_bit8 = 0, value_bit10 = 0;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return;

	if (yk3_pdev_is_vf(pdev_priv))
		return;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return;
	}

	/* Default: no pause, no autoneg */
	pause->rx_pause = 0;
	pause->tx_pause = 0;
	pause->autoneg  = 0;

	value = yk3_rd32(hw_addr, 0xa00008 + 0x400 * pdev_priv->mac->mac_ch);
	value_bit8 = (value >> 8) & 0x1;
	value_bit10 = (value >> 10) & 0x1;
	if (value_bit8)
		tx_pause_mode = 1;
	if (value_bit10)
		rx_pause_mode = 1;

	pause->rx_pause = !!(rx_pause_mode > 0);
	pause->tx_pause = !!(tx_pause_mode > 0);
	pause->autoneg = 0;
}

static int yk3_set_pauseparam(struct net_device *ndev,
			      struct ethtool_pauseparam *pause)
{
	u8 enable_fc;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return -EOPNOTSUPP;

	if (pause->autoneg)
		return -EINVAL;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_DCB
	if (ndev_priv->dcbx && ndev_priv->dcbx->pfc.pfcena) {
		yk3_net_warn("dcbx pfc is enable, do not set fc\n");
		return -EOPNOTSUPP;
	}
#endif

	enable_fc = (pause->rx_pause << 1) | pause->tx_pause;
	yk3_net_info("Set flow control %s (rx_pause=%d, tx_pause=%d)",
		     enable_fc ? "enable" : "disable",
		     pause->rx_pause, pause->tx_pause);

	ret = yk3_mac_set_fc(ndev_priv, pdev_priv->mac->mac_ch, enable_fc);
	if (ret) {
		yk3_net_err("Set umac fc failed, error is %d.\n", ret);
		return ret;
	}

	ret = yk3_np_set_fc(ndev, pdev_priv->mac->mac_ch, enable_fc ? 1 : 0);
	if (ret) {
		yk3_net_err("Set np fc failed, error is %d.\n", ret);
		ret = yk3_mac_set_fc(ndev_priv, pdev_priv->mac->mac_ch, 0);
		return ret;
	}

	return ret;
}

#ifdef YK3_HAVE_KERNEL_ETHTOOL_TS_INFO
static int yk3_get_ts_info(struct net_device *ndev, struct kernel_ethtool_ts_info *eti)
#else
static int yk3_get_ts_info(struct net_device *ndev, struct ethtool_ts_info *eti)
#endif
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	return yk3_ptp_get_ts_info(pdev_priv, eti);
}

static int yk3_nway_reset(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int ret;

	if (!yk3_ndev_is_pf(ndev_priv))
		return -EOPNOTSUPP;

	ret = yk3_mac_ndev_set_phy_state(ndev_priv, true);
	if (ret) {
		yk3_net_err("Set mac phy step 1 failed: %d", ret);
		return -EIO;
	}
	msleep(20000);
	ret = yk3_mac_ndev_set_phy_state(ndev_priv, false);
	if (ret) {
		yk3_net_err("Set mac phy step 2 failed: %d", ret);
		return -EIO;
	}

	return ret;
}

const struct ethtool_ops yk3_ethtool_ops = {
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
				     ETHTOOL_COALESCE_MAX_FRAMES |
				     ETHTOOL_COALESCE_USE_ADAPTIVE,
#endif /* ETHTOOL_COALESCE_USECS */
	.get_drvinfo = yk3_get_drvinfo,
	.get_link = ethtool_op_get_link,
	.get_ethtool_stats = yk3_get_ethtool_stats,
	.get_strings = yk3_get_strings,
	.get_sset_count = yk3_get_sset_count,
	.get_module_info = yk3_get_module_info,
	.get_module_eeprom = yk3_get_module_eeprom,
	.self_test = yk3_self_test,
#ifdef YK3_HAVE_ETHTOOL_GET_LINK_SETTING
	.get_link_ksettings = yk3_get_link_ksettings,
#else
	.get_settings = yk3_get_link_ksettings,
#endif /* YK3_HAVE_ETHTOOL_GET_LINK_SETTING */
	.set_link_ksettings = yk3_set_link_ksettings,
	.get_priv_flags = yk3_get_priv_flags,
	.set_priv_flags = yk3_set_priv_flags,
	.get_coalesce = yk3_get_coalesce,
	.set_coalesce = yk3_set_coalesce,
	.get_ts_info = yk3_get_ts_info,
	.get_fecparam = yk3_get_fecparam,
	.set_fecparam = yk3_set_fecparam,
#ifdef YK3_HAVE_ETHTOOL_MAC_STATS
	.get_eth_mac_stats = yk3_get_eth_mac_stats,
#endif /* YK3_HAVE_ETHTOOL_MAC_STATS */
	.get_rxfh_indir_size = yk3_get_rxfh_indir_size,
	.get_rxfh_key_size = yk3_get_rxfh_key_size,
	.get_rxfh = yk3_get_rxfh,
	.set_rxfh = yk3_set_rxfh,
#ifdef YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS
	.get_rxfh_fields = yk3_get_rxfh_fields,
#endif /* YK3_HAVE_ETHTOOL_GET_RXFH_FIELDS */
#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS
	.set_rxfh_fields = yk3_set_rxfh_fields,
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_FIELDS */
	.get_rxnfc = yk3_get_rxnfc_eth,
	.set_rxnfc = yk3_set_rxnfc_eth,
	.get_eeprom_len = yk3_get_eeprom_len,
	.get_eeprom = yk3_get_eeprom,
	.set_eeprom = yk3_set_eeprom,
	.set_phys_id = yk3_set_phys_id,
	.get_ringparam = yk3_get_ringparam,
	.set_ringparam = yk3_set_ringparam,
	.get_regs_len = yk3_get_regs_len,
	.get_regs = yk3_get_regs,
	.get_channels = yk3_get_channels,
	.set_channels = yk3_set_channels,
	.get_pauseparam = yk3_get_pauseparam,
	.set_pauseparam = yk3_set_pauseparam,
	.nway_reset = yk3_nway_reset,
};
