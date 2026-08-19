// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_cdev_priv.h"
#include "yk3_doe_process.h"
#include "yk3_qset.h"

static struct miscdevice yk3_mdev;

static int yk3_cfg_save(struct yk3_cdev_priv *priv, int type, void *data, size_t size)
{
	size_t total_size;
	struct yk3_cfgsave *cfgsave;

	if (size && !data)
		return -EINVAL;

	total_size = sizeof(*cfgsave) + size;

	cfgsave = kzalloc(total_size, GFP_KERNEL);
	if (!cfgsave)
		return -ENOMEM;

	INIT_LIST_HEAD(&cfgsave->node);
	cfgsave->type = type;
	if (size)
		memcpy(cfgsave->data, data, size);
	list_add_tail(&cfgsave->node, &priv->cfgsave_head);

	return 0;
}

static void yk3_cfg_restore(struct yk3_cdev_priv *priv,
			    void (*cb)(struct yk3_cdev_priv *priv, int type, void *data))
{
	struct yk3_cfgsave *cfgsave, *cfgsave_tmp;

	list_for_each_entry_safe(cfgsave, cfgsave_tmp, &priv->cfgsave_head, node) {
		if (priv->cfgchange_flags & (1 << cfgsave->type))
			cb(priv, cfgsave->type, cfgsave->data);
		list_del(&cfgsave->node);
		kfree(cfgsave);
	}
}

static void yk3_cfg_flags_set(struct yk3_cdev_priv *priv, int type)
{
	priv->cfgchange_flags |= (1 << type);
}

static int yk3_open(struct inode *inode, struct file *file)
{
	struct yk3_cdev_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	file->private_data = priv;
	INIT_LIST_HEAD(&priv->cfgsave_head);

	return 0;
}

static int yk3_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret, i;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_pdev_priv *pdev_priv = priv->pdev_priv;

	size_t req_size = vma->vm_end - vma->vm_start;
	u64 req_addr = vma->vm_pgoff << PAGE_SHIFT;

	u64 bar_addr = 0;
	u64 bar_size = 0;

	for (i = 0; i < YK3_BAR_MAX; i++) {
		if (req_addr == pdev_priv->bar_pa[i] &&
		    req_size == pdev_priv->bar_size[i]) {
			bar_addr = pdev_priv->bar_pa[i];
			bar_size = pdev_priv->bar_size[i];
			break;
		}
	}

	if (!bar_addr || !bar_size)
		return -EINVAL;

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, req_size,
			      pgprot_noncached(vma->vm_page_prot));

	return ret;
}

static long yk3_comm_dev_bind(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_comm_devid devid;
	int ret;
	enum yk3_ndev_type type;
	struct yk3_pci_addr pci_addr;

	if (copy_from_user(&devid, (void __user *)arg, sizeof(devid)))
		return -EFAULT;

	if (priv->pdev_priv)
		return -EEXIST;

	switch (devid.type) {
	case YK3_COMM_DEVTYPE_PCI:
		pci_addr = devid.pci;
		priv->pdev_priv = yk3_get_pdev_priv(&pci_addr);
		if (!priv->pdev_priv) {
			ret = -ENODEV;
			goto err_find;
		}

		if (yk3_pdev_is_mgr(priv->pdev_priv)) {
			// doe bind on manager-pf
			return 0;
		}

		type = yk3_pdev_is_pf(priv->pdev_priv) ? YK3_NDEV_T_PF : YK3_NDEV_T_VF;

		priv->ndev_priv = yk3_pdev_get_ndev_priv(priv->pdev_priv, type, -1);
		if (!priv->ndev_priv) {
			ret = -ENODEV;
			goto err_find;
		}
		break;
	case YK3_COMM_DEVTYPE_NDEV:
		priv->ndev_priv = yk3_get_ndev_priv(devid.ndev.ifindex);
		if (!priv->ndev_priv) {
			ret = -ENODEV;
			goto err_find;
		}
		priv->pdev_priv = priv->ndev_priv->pdev_priv;
		break;
	default:
		ret = -EOPNOTSUPP;
		goto err_find;
	}

	priv->domain = iommu_get_domain_for_dev(priv->pdev_priv->dev) ? true : false;

	if (yk3_iommu_present(priv->pdev_priv->dev)) {
		priv->dmamap_table = yk3_dmamap_table_create(priv->pdev_priv->dev);
		if (!priv->dmamap_table)
			yk3_err("dmamap table create failed!");
	}

	return 0;

err_find:
	priv->ndev_priv = NULL;
	priv->pdev_priv = NULL;
	return ret;
}

static long yk3_comm_sysinfo(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_comm_sysinfo sysinfo = {0};
	struct iommu_domain *domain;
	struct iommu_group *group;

	domain = iommu_get_domain_for_dev(priv->pdev_priv->dev);
	group = iommu_group_get(priv->pdev_priv->dev);
	sysinfo.iommu = (domain || group) ? true : false;
	sysinfo.domain = domain ? true : false;
	if (domain && domain->type == IOMMU_DOMAIN_IDENTITY)
		sysinfo.pt = true;

	if (copy_to_user((void __user *)arg, &sysinfo, sizeof(sysinfo)))
		return -EFAULT;

	return 0;
}

static long yk3_comm_cardinfo(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_comm_cardinfo cardinfo = {0};
	struct yk3_card *card = priv->pdev_priv->card;

	cardinfo.id = card->id;
	cardinfo.chip = card->chip;
	cardinfo.mode = card->mode;
	cardinfo.numa = card->numa;
	cardinfo.qnum = card->qnum;
	cardinfo.pf_num = card->pf_num;
	cardinfo.vf_maxnum = pci_sriov_get_totalvfs(priv->pdev_priv->pdev);

	if (copy_to_user((void __user *)arg, &cardinfo, sizeof(cardinfo)))
		return -EFAULT;

	return 0;
}

static long yk3_net_devinfo(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_net_devinfo devinfo;
	struct yk3_pdev_priv *pdev_priv = priv->pdev_priv;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_queuebase qbase;
	int ret;

	devinfo.pdev.pf_id = pdev_priv->pf_id;
	devinfo.pdev.vf_id = pdev_priv->vf_id;
	devinfo.pdev.vf_num = pdev_priv->sriov_priv ? pdev_priv->sriov_priv->num_vfs : 0;
	ret = yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_LOCAL, &qbase);
	devinfo.pdev.own_qbase[YK3_QUEUE_T_LOCAL] = qbase;
	ret |= yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_FUNC, &qbase);
	devinfo.pdev.own_qbase[YK3_QUEUE_T_FUNC] = qbase;
	ret |= yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_PF, &qbase);
	devinfo.pdev.own_qbase[YK3_QUEUE_T_PF] = qbase;
	ret |= yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_GLOBAL, &qbase);
	devinfo.pdev.own_qbase[YK3_QUEUE_T_GLOBAL] = qbase;
	if (ret)
		return ret;

	devinfo.ndev.ifindex = ndev_priv->ndev->ifindex;
	devinfo.ndev.type = ndev_priv->type;
	devinfo.ndev.qsetid = ndev_priv->qsetid;
#ifdef YK3_HAVE_MAX_MTU
	devinfo.ndev.min_mtu = ndev_priv->ndev->min_mtu;
	devinfo.ndev.max_mtu = ndev_priv->ndev->max_mtu;
#else
	devinfo.ndev.min_mtu = ndev_priv->ndev->extended->min_mtu;
	devinfo.ndev.max_mtu = ndev_priv->ndev->extended->max_mtu;
#endif /* YK3_HAVE_MAX_MTU */
	devinfo.ndev.min_qdepth = YK3_N_MIN_QDEPTH;
	devinfo.ndev.max_qdepth = YK3_N_MAX_QDEPTH;

	qbase.start = 0;
	qbase.num = ndev_priv->ndev->real_num_tx_queues;
	devinfo.ndev.tx_qbase = qbase;
	qbase.start = 0;
	qbase.num = ndev_priv->ndev->real_num_rx_queues;
	devinfo.ndev.rx_qbase = qbase;

	devinfo.ndev.own_qbase[YK3_QUEUE_T_LOCAL] = ndev_priv->qbase[YK3_QUEUE_T_LOCAL];
	devinfo.ndev.own_qbase[YK3_QUEUE_T_FUNC] = ndev_priv->qbase[YK3_QUEUE_T_FUNC];
	devinfo.ndev.own_qbase[YK3_QUEUE_T_PF] = ndev_priv->qbase[YK3_QUEUE_T_PF];
	devinfo.ndev.own_qbase[YK3_QUEUE_T_GLOBAL] = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];

	if (copy_to_user((void __user *)arg, &devinfo, sizeof(devinfo)))
		return -EFAULT;

	return 0;
}

static int yk3_save_rss_config(struct yk3_cdev_priv *priv)
{
	struct net_device *ndev = priv->ndev_priv->ndev;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	int ret = 0;
	struct ethtool_rxnfc rxnfc;
#ifdef YK3_HAVE_ETHTOOL_GET_RXFH_PARAM
	struct ethtool_rxfh_param rxfh_param;
#endif
	u8 *key = NULL;
	u32 *indir = NULL;
	struct yk3_rss_cfg cfg = {0};
	int i;
	u32 flows[] = {
		TCP_V4_FLOW,
		TCP_V6_FLOW,
		UDP_V4_FLOW,
		UDP_V6_FLOW,
	};

	if (ndev->ethtool_ops->get_rxnfc) {
		for (i = 0; i < ARRAY_SIZE(flows); i++) {
			memset(&rxnfc, 0, sizeof(rxnfc));
			rxnfc.cmd = ETHTOOL_GRXFH;
			rxnfc.flow_type = flows[i];

			if (ndev->ethtool_ops->get_rxnfc &&
			    !ndev->ethtool_ops->get_rxnfc(ndev, &rxnfc, NULL)) {
				cfg.entry[cfg.num].flow_type = flows[i];
				cfg.entry[cfg.num].data = rxnfc.data;
				cfg.num++;
			}
		}
		ret = yk3_cfg_save(priv, YK3_NET_RSS_HF, &cfg, sizeof(cfg));
		if (ret) {
			yk3_net_err("save rss_rxnfc failed");
			return ret;
		}
	}

	key = kmalloc(YK3_RSS_KEY_SIZE, GFP_KERNEL);
	if (!key)
		return -ENOMEM;

	indir = kmalloc_array(YK3_MAX_RSS_RETA_SIZE, sizeof(u32), GFP_KERNEL);
	if (!indir) {
		kfree(key);
		return -ENOMEM;
	}

#ifdef YK3_HAVE_ETHTOOL_GET_RXFH_PARAM
	rxfh_param.key_size = YK3_RSS_KEY_SIZE;
	rxfh_param.indir_size = YK3_MAX_RSS_RETA_SIZE;

	rxfh_param.key = key;
	rxfh_param.indir = indir;
	if (ndev->ethtool_ops->get_rxfh) {
		ndev->ethtool_ops->get_rxfh(ndev, &rxfh_param);
		ret = yk3_cfg_save(priv, YK3_NET_RSS_HASH_KEY, key,
				   rxfh_param.key_size);
		if (ret) {
			yk3_net_err("save rss hash key failed");
			goto out;
		}
		ret = yk3_cfg_save(priv, YK3_NET_RSS_RETA, indir,
				   rxfh_param.indir_size * sizeof(u32));
		if (ret) {
			yk3_net_err("save rss hash reta failed");
			goto out;
		}
	}
#else
	if (ndev->ethtool_ops->get_rxfh) {
		ndev->ethtool_ops->get_rxfh(ndev, indir, key, NULL);
		ret = yk3_cfg_save(priv, YK3_NET_RSS_HASH_KEY, key, YK3_RSS_KEY_SIZE);
		if (ret) {
			yk3_net_err("save rss_hash_key failed");
			goto out;
		}
		ret = yk3_cfg_save(priv, YK3_NET_RSS_RETA, indir,
				   YK3_MAX_RSS_RETA_SIZE * sizeof(u32));
		if (ret) {
			yk3_net_err("save rss_reta failed");
			goto out;
		}
	}
#endif /* YK3_HAVE_ETHTOOL_GET_RXFH_PARAM */

out:
	kfree(key);
	kfree(indir);
	return ret;
}

static int yk3_save_pfc_config(struct yk3_cdev_priv *priv)
{
	struct net_device *ndev = priv->ndev_priv->ndev;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = priv->pdev_priv;
	struct yk3_pfc_pcie_l1 pfc_pcie;
	u32 l1xoff = 512;
	u32 l1xon = 352;
	int ret = 0;

	if (yk3_pdev_is_vf(pdev_priv))
		return ret;

	ret = yk3_get_pfc_pcie_l1xoff(ndev, &l1xoff);
	ret |= yk3_get_pfc_pcie_l1xon(ndev, &l1xon);
	if (ret) {
		yk3_net_err("get pfc_pcie l1 failed");
		return ret;
	}

	pfc_pcie.pcie_l1_xoff = l1xoff;
	pfc_pcie.pcie_l1_xon = l1xon;

	ret = yk3_cfg_save(priv, YK3_NET_PFC_PCIE_L1, &pfc_pcie, sizeof(pfc_pcie));
	if (ret) {
		yk3_net_err("save pfc pcie l1 failed");
		return ret;
	}

	priv->cfgchange_flags |= (1 << YK3_NET_PFC_PCIE_L1);

	return ret;
}

static int yk3_restore_pfc_config(struct yk3_ndev_priv *ndev_priv, struct yk3_pfc_pcie_l1 *pfc_pcie)
{
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	int ret = 0;

	if (yk3_pdev_is_vf(pdev_priv))
		return ret;

	ret = yk3_set_pfc_pcie_l1vq(ndev, pfc_pcie->pcie_l1_xoff, pfc_pcie->pcie_l1_xon);
	if (ret) {
		yk3_net_err("restore pfc pcie l1 failed");
		return ret;
	}

	return ret;
}

static long yk3_net_umd(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_sriov_mbox_umd_vf_msg *umd_vf;
	struct yk3_sriov_mbox_ack_msg *ack;
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_net_umd umd = { 0 };
	struct yk3_cfgsave *cfgsave, *cfgsave_tmp;
	int ret = 0;
	void *data;
	struct ethtool_pauseparam pause = {0};
	struct ethtool_coalesce ec = {0};
	u8 default_ifg = 12;

	if (copy_from_user(&umd, (void __user *)arg, sizeof(umd)))
		return -EFAULT;

	mutex_lock(&ndev_priv->state_mlock);
	if (umd.enable && ndev_priv->umd_enable) {
		ret = -EBUSY;
		goto unlock_out;
	}
	if (!umd.enable && !ndev_priv->umd_enable) {
		priv->umd_enabled = false;
		goto unlock_out;
	}

	if (!list_empty(&ndev_priv->cvlan_list) || !list_empty(&ndev_priv->svlan_list)) {
		yk3_net_err("vlan should be cleared before umd");
		ret = -EOPNOTSUPP;
		goto unlock_out;
	}

	if (ndev_priv->ethtool_priv_flags & BIT(YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE)) {
		yk3_net_err("LINK_DOWN_ON_CLOSE should be off before umd");
		ret = -EOPNOTSUPP;
		goto unlock_out;
	}

	if (yk3_ndev_bw_limited(ndev_priv)) {
		yk3_net_err("bw limit should be cleared before umd");
		ret = -EOPNOTSUPP;
		goto unlock_out;
	}

	data = (void *)(ndev->dev_addr);
	ret = yk3_cfg_save(priv, YK3_NET_MACADDR, data, ETH_ALEN);
	if (ret) {
		yk3_net_err("save macaddr failed");
		goto unlock_out;
	}

	ret = yk3_cfg_save(priv, YK3_NET_MTU, &ndev->mtu, sizeof(ndev->mtu));
	if (ret) {
		yk3_net_err("save mtu failed");
		goto unlock_out;
	}

	ret = yk3_cfg_save(priv, YK3_NET_PROMISC, &ndev->flags, sizeof(ndev->flags));
	if (ret) {
		yk3_net_err("save promisc failed");
		goto unlock_out;
	}

	ret = yk3_cfg_save(priv, YK3_NET_ALLMULTI, &ndev->flags, sizeof(ndev->flags));
	if (ret) {
		yk3_net_err("save allmulti failed");
		goto unlock_out;
	}

	ret = yk3_cfg_save(priv, YK3_NET_OFFLOADCAP, &ndev->features, sizeof(ndev->features));
	if (ret) {
		yk3_net_err("save offloadcap failed");
		goto unlock_out;
	}

	ret = yk3_cfg_save(priv, YK3_NET_VF_CONF, NULL, 0);
	if (ret) {
		yk3_net_err("save vf info failed");
		goto unlock_out;
	}
	priv->cfgchange_flags |= (1 << YK3_NET_VF_CONF);

	if (ndev->ethtool_ops->get_pauseparam) {
		ndev->ethtool_ops->get_pauseparam(ndev, &pause);
		ret = yk3_cfg_save(priv, YK3_NET_FLOW_CTRL, &pause, sizeof(pause));
		if (ret) {
			yk3_net_err("save flow control failed");
			goto unlock_out;
		}
	}

	if (ndev->ethtool_ops->get_coalesce) {
#ifdef YK3_HAVE_ETHTOOL_COALESCE_CQE
		ret = ndev->ethtool_ops->get_coalesce(ndev, &ec, NULL, NULL);
#else
		ret = ndev->ethtool_ops->get_coalesce(ndev, &ec);
#endif /* YK3_HAVE_ETHTOOL_COALESCE_CQE */
		if (ret) {
			yk3_net_err("get coalesce failed");
			goto unlock_out;
		}

		ret = yk3_cfg_save(priv, YK3_NET_COALESCE, &ec, sizeof(ec));
		if (ret) {
			yk3_net_err("save coalesce failed");
			goto unlock_out;
		}
	}

	mutex_unlock(&ndev_priv->state_mlock);

	ret = yk3_cfg_save(priv, YK3_NET_SET_IFG, &default_ifg, sizeof(default_ifg));
	if (ret) {
		yk3_net_err("save ifg failed");
		goto out;
	}

	ret = yk3_save_rss_config(priv);
	if (ret) {
		yk3_net_err("save rss config failed");
		goto out;
	}

	if (yk3_ndev_is_vf(ndev_priv)) {
		umd_vf = (struct yk3_sriov_mbox_umd_vf_msg *)mbox_msg.data;
		umd_vf->vf_id = ndev_priv->vf_id - 1;
		umd_vf->enable = umd.enable ? 1 : 0;

		mbox_msg.opcode = YK3_MBOX_OPCODE_UMD_VF_ENABLE;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 3000;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg);
		if (ret != 0) {
			yk3_dev_err("cdev umd send mbox message errno %d failed!\n", ret);
		} else {
			ack = (struct yk3_sriov_mbox_ack_msg *)ack_msg.data;
			if (ack->ret != YK3_SRIOV_MBOX_OK)
				yk3_dev_err("sriov pf set vf umd enable failed ret code %d!\n",
					    ack->ret);
		}
	}

	ret = yk3_save_pfc_config(priv);
	if (ret) {
		yk3_net_err("save pfc config failed");
		goto out;
	}

	rtnl_lock();
	if (umd.enable) {
		mutex_lock(&ndev_priv->state_mlock);
		ndev_priv->umd_enable = true;
		mutex_unlock(&ndev_priv->state_mlock);
		dev_close(ndev_priv->ndev);
		priv->umd_enabled = true;
		netif_device_detach(ndev_priv->ndev);
		yk3_net_debug("umd enable");
	} else {
		netif_device_attach(ndev_priv->ndev);
#ifdef YK3_HAVE_DEV_OPEN_NETLINK
		dev_open(ndev_priv->ndev, NULL);
#else
		dev_open(ndev_priv->ndev);
#endif /* YK3_HAVE_DEV_OPEN_NETLINK */
		mutex_lock(&ndev_priv->state_mlock);
		ndev_priv->umd_enable = false;
		mutex_unlock(&ndev_priv->state_mlock);
		priv->umd_enabled = false;
		yk3_net_debug("umd disable");
	}
	rtnl_unlock();

	return 0;

unlock_out:
	mutex_unlock(&ndev_priv->state_mlock);
out:
	list_for_each_entry_safe(cfgsave, cfgsave_tmp, &priv->cfgsave_head, node) {
		list_del(&cfgsave->node);
		kfree(cfgsave);
	}
	return ret;
}

static long yk3_net_pcibar(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_net_pcibar pcibar;
	int bar;

	if (copy_from_user(&pcibar, (void __user *)arg, sizeof(pcibar)))
		return -EFAULT;

	bar = pcibar.req.bar_idx;
	if (bar < 0 || bar >= YK3_BAR_MAX)
		return -EINVAL;

	bar = array_index_nospec(bar, YK3_BAR_MAX);
	pcibar.rsp.bar_addr = priv->pdev_priv->bar_pa[bar];
	pcibar.rsp.bar_size = priv->pdev_priv->bar_size[bar];
	pcibar.rsp.bar_offset = priv->pdev_priv->bar_offset[bar];

	if (copy_to_user((void __user *)arg, &pcibar, sizeof(pcibar)))
		return -EFAULT;

	return 0;
}

static int yk3_net_macaddr_set(struct yk3_ndev_priv *ndev_priv, void *addr)
{
	struct sockaddr sa;

	memcpy(sa.sa_data, addr, ETH_ALEN);
	return ndev_priv->ndev->netdev_ops->ndo_set_mac_address(ndev_priv->ndev, &sa);
}

static long yk3_net_macaddr(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_macaddr macaddr;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (copy_from_user(&macaddr, (void __user *)arg, sizeof(macaddr)))
			return -EFAULT;
		ret = yk3_net_macaddr_set(ndev_priv, macaddr.addr);
		break;
	case _IOC_READ:
		memcpy(macaddr.addr, ndev_priv->ndev->dev_addr, ETH_ALEN);
		if (copy_to_user((void __user *)arg, &macaddr, sizeof(macaddr)))
			return -EFAULT;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int yk3_net_mtu_set(struct yk3_ndev_priv *ndev_priv, int mtu)
{
	struct net_device *ndev = ndev_priv->ndev;

#ifdef YK3_HAVE_NDO_EXT_CHANGE_MTU
	return ndev->netdev_ops->extended.ndo_change_mtu(ndev, mtu);
#elif defined YK3_HAVE_CHANGE_MTU_RH74
	return ndev->netdev_ops->ndo_change_mtu_rh74(ndev, mtu);
#else
	return ndev->netdev_ops->ndo_change_mtu(ndev, mtu);
#endif
}

static long yk3_net_mtu(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_mtu mtu;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (copy_from_user(&mtu, (void __user *)arg, sizeof(mtu)))
			return -EFAULT;
		ret = yk3_net_mtu_set(ndev_priv, mtu.size);
		break;
	case _IOC_READ:
		mtu.size = (u16)(ndev_priv->ndev->mtu);
		if (copy_to_user((void __user *)arg, &mtu, sizeof(mtu)))
			return -EFAULT;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static void yk3_net_offloadcap_set(struct yk3_ndev_priv *ndev_priv, netdev_features_t features)
{
	struct net_device *ndev = ndev_priv->ndev;

	if (!IS_ERR_OR_NULL(ndev->netdev_ops->ndo_set_features))
		ndev->netdev_ops->ndo_set_features(ndev, features);

	ndev->features = features;
}

static void yk3_net_offload_cap_get(struct yk3_ndev_priv *ndev_priv, struct yk3_net_offloadcap *cap)
{
	if (ndev_priv->cdev_features & NETIF_F_RXCSUM) {
		cap->rxsupport |= YK3_RXOLCAP_IPV4_CKSUM;
		cap->rxsupport |= YK3_RXOLCAP_UDP_CKSUM;
		cap->rxsupport |= YK3_RXOLCAP_TCP_CKSUM;
	}

	if (ndev_priv->cdev_features & NETIF_F_RXHASH)
		cap->rxsupport |= YK3_RXOLCAP_RSS_HASH;

	if (ndev_priv->cdev_features & NETIF_F_HW_CSUM) {
		cap->txsupport |= YK3_TXOLCAP_IPV4_CKSUM;
		cap->txsupport |= YK3_TXOLCAP_UDP_CKSUM;
		cap->txsupport |= YK3_TXOLCAP_TCP_CKSUM;
	}

	if (ndev_priv->cdev_features & NETIF_F_TSO) {
		cap->txsupport |= YK3_TXOLCAP_TCP_TSO;
		cap->txsupport |= YK3_TXOLCAP_UDP_TSO;
	}
}

static long yk3_net_offloadcap(struct file *file, unsigned int cmd, unsigned long arg)
{
	u64 tx_mask = 0;
	u64 rx_mask = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_offloadcap offloadcap = {0};

	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
		yk3_net_offload_cap_get(ndev_priv, &offloadcap);
		if (copy_to_user((void __user *)arg, &offloadcap, sizeof(offloadcap)))
			return -EFAULT;
		break;
	case _IOC_WRITE:
		if (copy_from_user(&offloadcap, (void __user *)arg, sizeof(offloadcap)))
			return -EFAULT;
		tx_mask = YK3_TXOLCAP_IPV4_CKSUM;
		tx_mask |= YK3_TXOLCAP_TCP_CKSUM;
		tx_mask |= YK3_TXOLCAP_UDP_CKSUM;

		rx_mask = YK3_RXOLCAP_IPV4_CKSUM;
		rx_mask |= YK3_RXOLCAP_UDP_CKSUM;
		rx_mask |= YK3_RXOLCAP_TCP_CKSUM;

		if (offloadcap.txconfig & tx_mask)
			ndev_priv->cdev_features |= NETIF_F_HW_CSUM;
		else
			ndev_priv->cdev_features &= ~NETIF_F_HW_CSUM;

		if (offloadcap.rxconfig & rx_mask)
			ndev_priv->cdev_features |= NETIF_F_RXCSUM;
		else
			ndev_priv->cdev_features &= ~NETIF_F_RXCSUM;

		if (offloadcap.txconfig & YK3_TXOLCAP_TCP_TSO)
			ndev_priv->cdev_features |= NETIF_F_TSO;
		else
			ndev_priv->cdev_features &= ~NETIF_F_TSO;

		if (offloadcap.rxconfig & YK3_RXOLCAP_SCATTER)
			ndev_priv->cdev_features |= NETIF_F_SG;
		else
			ndev_priv->cdev_features &= ~NETIF_F_SG;

		if (offloadcap.rxconfig & YK3_RXOLCAP_VLAN_STRIP)
			ndev_priv->cdev_features |= NETIF_F_HW_VLAN_CTAG_RX;
		else
			ndev_priv->cdev_features &= ~NETIF_F_HW_VLAN_CTAG_RX;

		if (offloadcap.rxconfig & YK3_RXOLCAP_VLAN_FILTER)
			ndev_priv->cdev_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
		else
			ndev_priv->cdev_features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;

		if (offloadcap.txconfig & YK3_TXOLCAP_VLAN_INSERT)
			ndev_priv->cdev_features |= NETIF_F_HW_VLAN_CTAG_TX;
		else
			ndev_priv->cdev_features &= ~NETIF_F_HW_VLAN_CTAG_TX;

		if (offloadcap.rxconfig & YK3_RXOLCAP_RSS_HASH)
			ndev_priv->cdev_features |= NETIF_F_RXHASH;
		else
			ndev_priv->cdev_features &= ~NETIF_F_RXHASH;

		yk3_net_offloadcap_set(ndev_priv, ndev_priv->cdev_features);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static long yk3_net_linkinfo(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_linkinfo linkinfo;

	linkinfo.link_speed = ndev_priv->link_speed;
	linkinfo.link_duplex = ndev_priv->link_duplex;
	/* only AUTONEG_DISABLE for now*/
	linkinfo.link_autoneg = AUTONEG_DISABLE;
	linkinfo.link_status = ndev_priv->link_status;

	if (copy_to_user((void __user *)arg, &linkinfo, sizeof(linkinfo)))
		return -EFAULT;
	return 0;
}

static void yk3_net_rxmode_set(struct yk3_ndev_priv *ndev_priv, u32 flag, bool enable)
{
	struct net_device *ndev = ndev_priv->ndev;

	rtnl_lock();
	netif_addr_lock_bh(ndev);
	if (enable)
		ndev->flags |= flag;
	else
		ndev->flags &= ~flag;
	ndev->netdev_ops->ndo_set_rx_mode(ndev);
	netif_addr_unlock_bh(ndev);
	rtnl_unlock();
}

#define yk3_net_promisc_set(ndev_priv, enable) \
	yk3_net_rxmode_set(ndev_priv, IFF_PROMISC, enable)
#define yk3_net_allmulti_set(ndev_priv, enable) \
	yk3_net_rxmode_set(ndev_priv, IFF_ALLMULTI, enable)

static long yk3_net_rxmode_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg, u32 flag)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_rxmode rxmode;

	if (copy_from_user(&rxmode, (void __user *)arg, sizeof(rxmode)))
		return -EFAULT;

	if (rxmode.enable && (ndev->flags & flag))
		return 0;
	if (!rxmode.enable && !(ndev->flags & flag))
		return 0;

	yk3_net_rxmode_set(ndev_priv, flag, rxmode.enable);
	return 0;
}

static long yk3_net_promisc(struct file *file, unsigned int cmd, unsigned long arg)
{
	return yk3_net_rxmode_ioctl(file, cmd, arg, IFF_PROMISC);
}

static long yk3_net_allmulti(struct file *file, unsigned int cmd, unsigned long arg)
{
	return yk3_net_rxmode_ioctl(file, cmd, arg, IFF_ALLMULTI);
}

static struct yk3_page_map *yk3_page_map_alloc(u64 vaddr, size_t size)
{
	struct yk3_page_map *page_map;
	size_t npages = size >> PAGE_SHIFT;

	page_map = vzalloc(sizeof(*page_map) + sizeof(struct page *) * npages);
	if (!page_map)
		return NULL;

	page_map->vaddr = vaddr;
	page_map->npages = npages;
	page_map->pages = (struct page **)((u8 *)page_map + sizeof(*page_map));

	return page_map;
}

static void yk3_page_map_free(struct yk3_page_map *page_map)
{
	if (!page_map)
		return;

	vfree(page_map);
}

static void yk3_unpin_user_pages(struct page **pages, unsigned long npages)
{
#ifdef YK3_HAVE_UNPIN_USER_PAGES
	unpin_user_pages(pages, npages);
#else
	unsigned long i;

	for (i = 0; i < npages; i++)
		put_page(pages[i]);
#endif /* YK3_HAVE_UNPIN_USER_PAGES */
}

static long yk3_net_dma_map(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	unsigned int flags;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_pdev_priv *pdev_priv = priv->pdev_priv;
	struct yk3_page_map *page_map;
	u64 paddr_start;
	u64 iova;
	struct yk3_net_dmamap dmamap;
	u64 length;

	if (copy_from_user(&dmamap, (void __user *)arg, sizeof(dmamap)))
		return -EFAULT;

	if (!priv->dmamap_table)
		return -EOPNOTSUPP;

	if (!dmamap.len || ((dmamap.len | dmamap.iova | dmamap.vaddr) & ~PAGE_MASK))
		return -EINVAL;

	if (dmamap.iova + dmamap.len - 1 < dmamap.iova ||
	    dmamap.vaddr + dmamap.len - 1 < dmamap.vaddr)
		return -EINVAL;

	page_map = yk3_page_map_alloc(dmamap.vaddr, dmamap.len);
	if (!page_map)
		return -ENOMEM;

	/* should be flags = FOLL_WRITE | FOLL_LONGTERM; */
	flags = FOLL_WRITE;
#ifdef YK3_HAVE_GUP_PIN_NO_VMAS
	ret = pin_user_pages(page_map->vaddr, page_map->npages, flags,
			     page_map->pages);
#else
	ret = pin_user_pages(page_map->vaddr, page_map->npages, flags,
			     page_map->pages, NULL);
#endif /* YK3_HAVE_GUP_PIN_NO_VMAS */
	if (ret <= 0) {
		yk3_page_map_free(page_map);
		return ret;
	}

	if (ret != page_map->npages) {
		yk3_unpin_user_pages(page_map->pages, ret);
		yk3_page_map_free(page_map);
		return -ENOMEM;
	}

	iova = dmamap.iova;
	paddr_start = page_to_pfn(page_map->pages[0]) << PAGE_SHIFT;
	length = (u64)page_map->npages * PAGE_SIZE;

	ret = yk3_dmamap_map(priv->dmamap_table, iova, length, paddr_start, page_map->pages);
	if (ret)
		goto failed;

	yk3_page_map_free(page_map);

	return 0;

failed:
	if (ret != -EEXIST) {
		yk3_dev_err("%s failed: ret=%d (%pe), iova=0x%llx, length=%llu\n",
			    __func__, ret, ERR_PTR(ret), iova, length);
		yk3_dmamap_unmap(priv->dmamap_table, dmamap.iova, dmamap.len);
	} else {
		yk3_dmamap_update_refcnt(priv->dmamap_table, dmamap.iova, dmamap.len);
	}

	yk3_unpin_user_pages(page_map->pages, page_map->npages);
	yk3_page_map_free(page_map);

	return ret;
}

static long yk3_net_dma_unmap(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_net_dmamap dmamap;

	if (copy_from_user(&dmamap, (void __user *)arg, sizeof(dmamap)))
		return -EFAULT;

	if (!priv->dmamap_table)
		return -EOPNOTSUPP;

	if (!dmamap.len || ((dmamap.len | dmamap.iova | dmamap.vaddr) & ~PAGE_MASK))
		return -EINVAL;

	if (dmamap.iova + dmamap.len - 1 < dmamap.iova ||
	    dmamap.vaddr + dmamap.len - 1 < dmamap.vaddr)
		return -EINVAL;

	yk3_dmamap_unmap(priv->dmamap_table, dmamap.iova, dmamap.len);

	return 0;
}

static long yk3_net_start(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_net_start start;
	int ret;

	if (copy_from_user(&start, (void __user *)arg, sizeof(start)))
		return -EFAULT;

	if (start.enable) {
		ret = yk3_qset_start(priv->ndev_priv, start.txqnum, start.rxqnum);
		if (!ret) {
			priv->ndev_priv->txq_real_num = start.txqnum;
			priv->ndev_priv->rxq_real_num = start.rxqnum;
		}

		yk3_rss_indir_table_init(priv->ndev_priv, start.rxqnum);
		return ret;
	}

	priv->ndev_priv->txq_real_num = 0;
	priv->ndev_priv->rxq_real_num = 0;
	yk3_rss_indir_table_init(priv->ndev_priv, (u16)priv->ndev_priv->ndev->real_num_rx_queues);
	yk3_qset_stop(priv->ndev_priv);
	return 0;
}

static long yk3_meter_rx(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct ysc_meter_rate rx;
	int ret = 0;

	if (copy_from_user(&rx, (void __user *)arg, sizeof(rx)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_edma_set_rx_rate(priv->ndev_priv, rx.rate);
		break;
	case _IOC_READ:
		rx.rate = priv->ndev_priv->mdrop_rx_rate;
		if (copy_to_user((void __user *)arg, &rx, sizeof(rx)))
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static long yk3_meter_tx(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct ysc_meter_rate tx;
	int ret = 0;

	if (copy_from_user(&tx, (void __user *)arg, sizeof(tx)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_qos_set_tx_rate(priv->ndev_priv, tx.rate);
		break;
	case _IOC_READ:
		tx.rate = priv->ndev_priv->hqos_tx_rate;
		if (copy_to_user((void __user *)arg, &tx, sizeof(tx)))
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static long yk3_net_vlan_filter(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_vlan_config vlan_filter;
	int ret = 0;

	if (copy_from_user(&vlan_filter, (void __user *)arg, sizeof(vlan_filter)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (vlan_filter.enable == 0)
			ret = ndev->netdev_ops->ndo_vlan_rx_kill_vid(ndev,
								     (__be16)htons(ETH_P_8021Q),
								     vlan_filter.vlan_id);
		else
			ret = ndev->netdev_ops->ndo_vlan_rx_add_vid(ndev,
								    (__be16)htons(ETH_P_8021Q),
								    vlan_filter.vlan_id);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

enum {
	YK3_SET_RX_RSS_HASH_V4_TUPLE_3 = (1ULL << 0),
	YK3_SET_RX_RSS_HASH_V6_TUPLE_3 = (1ULL << 1),
	YK3_SET_RX_RSS_HASH_UDP_V4_TUPLE_5 = (1ULL << 2),
	YK3_SET_RX_RSS_HASH_TCP_V4_TUPLE_5 = (1ULL << 3),
	YK3_SET_RX_RSS_HASH_UDP_V6_TUPLE_5 = (1ULL << 4),
	YK3_SET_RX_RSS_HASH_TCP_V6_TUPLE_5 = (1ULL << 5),
};

static int yk3_net_rss_hf_set(struct net_device *ndev, unsigned long rss_hf)
{
	int ret = 0;
	struct ethtool_rxnfc rxnfc = {0};

	if (!ndev->ethtool_ops->set_rxnfc)
		return -EOPNOTSUPP;

	rxnfc.cmd = ETHTOOL_SRXFH;

	if (rss_hf & YK3_SET_RX_RSS_HASH_V4_TUPLE_3) {
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L3_PROTO);
		rxnfc.flow_type = TCP_V4_FLOW;
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
		rxnfc.flow_type = UDP_V4_FLOW;
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}
	if (rss_hf & YK3_SET_RX_RSS_HASH_V6_TUPLE_3) {
		rxnfc.flow_type = TCP_V6_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
		rxnfc.flow_type = UDP_V6_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}
	if (rss_hf & YK3_SET_RX_RSS_HASH_UDP_V4_TUPLE_5) {
		rxnfc.flow_type = UDP_V4_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 |
				RXH_L4_B_2_3 | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}
	if (rss_hf & YK3_SET_RX_RSS_HASH_TCP_V4_TUPLE_5) {
		rxnfc.flow_type = TCP_V4_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 |
				RXH_L4_B_2_3 | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}
	if (rss_hf & YK3_SET_RX_RSS_HASH_UDP_V6_TUPLE_5) {
		rxnfc.flow_type = UDP_V6_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 |
				RXH_L4_B_2_3 | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}
	if (rss_hf & YK3_SET_RX_RSS_HASH_TCP_V6_TUPLE_5) {
		rxnfc.flow_type = TCP_V6_FLOW;
		rxnfc.data = (RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 |
				RXH_L4_B_2_3 | RXH_L3_PROTO);
		ret = ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		if (ret)
			return ret;
	}

	return 0;
}

static long yk3_net_rss_hf(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_rss_hf rss_hf;

	if (copy_from_user(&rss_hf, (void __user *)arg, sizeof(rss_hf)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_net_rss_hf_set(ndev, rss_hf.rss_hf);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_rss_key(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_PARAM
	struct ethtool_rxfh_param param = { 0 };
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_PARAM */
	struct yk3_net_rss_key rss_key;

	if (copy_from_user(&rss_key, (void __user *)arg, sizeof(rss_key)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_PARAM
	case _IOC_WRITE:
		if (!ndev->ethtool_ops->set_rxfh)
			return -EOPNOTSUPP;

		param.rss_context = 0;
		param.hfunc = ETH_RSS_HASH_TOP;
		/* RSS key */
		param.key_size = rss_key.key_len;
		param.key = rss_key.rss_key;

		ret = ndev->ethtool_ops->set_rxfh(ndev, &param, NULL);
		break;
#else
	case _IOC_WRITE:
		if (!ndev->ethtool_ops->set_rxfh)
			return -EOPNOTSUPP;

		ret = ndev->ethtool_ops->set_rxfh(ndev, NULL, rss_key.rss_key, 0);
		break;
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_PARAM */
	default:
		ret = -EINVAL;
		break;
	}

	yk3_net_debug("ysc set rss hash key: %d %x:%x:%x:%x...%x:%x ret %d\n",
		      rss_key.key_len, rss_key.rss_key[0], rss_key.rss_key[1],
		      rss_key.rss_key[2], rss_key.rss_key[3], rss_key.rss_key[38],
		      rss_key.rss_key[39], ret);

	return ret;
}

static long yk3_net_rss_reta(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_rss_reta rss_reta;
	u32 indir_buf[YK3_MAX_RSS_RETA_SIZE];
	u32 indir_size;

	if (copy_from_user(&rss_reta, (void __user *)arg, sizeof(rss_reta)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		/* indir */
		indir_size = rss_reta.qcount * 4;
		if (indir_size > YK3_MAX_RSS_RETA_SIZE)
			return -EINVAL;

		memcpy(indir_buf, rss_reta.indir, indir_size * sizeof(u32));
		/* we need qcount as a parameter, cannot use ndo_ops set_rxfh */
		yk3_rss_indir_table_set(ndev, indir_buf, rss_reta.qcount);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static long yk3_net_flow_ctrl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_flow_ctrl flow_ctrl;
	struct ethtool_pauseparam pause = {0};
	int ret = 0;

	if (copy_from_user(&flow_ctrl, (void __user *)arg, sizeof(flow_ctrl)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (IS_ERR_OR_NULL(ndev->ethtool_ops->set_pauseparam))
			return -EOPNOTSUPP;

		pause.cmd = ETHTOOL_SPAUSEPARAM;
		pause.rx_pause = flow_ctrl.rx_pause;
		pause.tx_pause = flow_ctrl.tx_pause;
		pause.autoneg = flow_ctrl.autoneg;
		ret = ndev->ethtool_ops->set_pauseparam(ndev, &pause);
		if (ret) {
			yk3_net_debug("set flow ctrl failed: ret=%d\n", ret);
			return ret;
		}
		break;
	case _IOC_READ:
		if (IS_ERR_OR_NULL(ndev->ethtool_ops->get_pauseparam))
			return -EOPNOTSUPP;

		pause.cmd = ETHTOOL_GPAUSEPARAM;
		ndev->ethtool_ops->get_pauseparam(ndev, &pause);
		flow_ctrl.rx_pause = pause.rx_pause;
		flow_ctrl.tx_pause = pause.tx_pause;
		flow_ctrl.autoneg = pause.autoneg;

		if (copy_to_user((void __user *)arg, &flow_ctrl, sizeof(flow_ctrl)))
			return -EFAULT;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_vf_mac_anti_spoof(struct file *file,  unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_vf_mac_anti_spoof spoof;
	int ret = 0;

	if (copy_from_user(&spoof, (void __user *)_arg, sizeof(spoof)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_lan_umd_set_spoofchk(ndev_priv, spoof.enable);
		if (ret) {
			yk3_net_debug("set vf mac anti spoof failed: ret=%d\n", ret);
			return ret;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_pvid(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_pvid_config pvid;

	int ret = 0;

	if (copy_from_user(&pvid, (void __user *)_arg, sizeof(pvid)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_lan_umd_set_pvid(ndev_priv, pvid.pvid, pvid.tpid, 0, false);
		if (ret) {
			yk3_net_debug("set pvid vlan failed: ret=%d\n", ret);
			return ret;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_tx_pvid(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_pvid_config pvid;

	int ret = 0;

	if (copy_from_user(&pvid, (void __user *)_arg, sizeof(pvid)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		ret = yk3_lan_umd_set_pvid(ndev_priv, pvid.pvid, pvid.tpid, 0, true);
		if (ret) {
			yk3_net_debug("set tx pvid vlan failed: ret=%d\n", ret);
			return ret;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_queue_rate_limit(struct file *file, unsigned int cmd, unsigned long _arg)
{
#ifdef YK3_HAVE_NDO_SET_TX_MAXRATE
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_queue_rate_limit qrl;
	int ret = 0;

	if (copy_from_user(&qrl, (void __user *)_arg, sizeof(qrl)))
		return -EFAULT;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (IS_ERR_OR_NULL(ndev->netdev_ops->ndo_set_tx_maxrate))
			return -EOPNOTSUPP;

		ret = ndev->netdev_ops->ndo_set_tx_maxrate(ndev, qrl.queue_idx, qrl.tx_rate);
		if (ret) {
			yk3_net_debug("set queue rate limit failed: ret=%d\n", ret);
			return ret;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
#else
	(void)file;
	(void)cmd;
	(void)_arg;

	return -EOPNOTSUPP;
#endif /* YK3_HAVE_NDO_SET_TX_MAXRATE */
}

static long yk3_net_module_eeprom(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_module_eeprom eeprom;
	struct ethtool_eeprom ee;
	u8 *buf;
	int ret;

	if (IS_ERR_OR_NULL(ndev->ethtool_ops->get_module_eeprom))
		return -EOPNOTSUPP;

	if (copy_from_user(&eeprom, (void __user *)arg, sizeof(eeprom)))
		return -EFAULT;

	if (!eeprom.len || !eeprom.data)
		return -EINVAL;

	buf = memdup_user(eeprom.data, eeprom.len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	ee.cmd = ETHTOOL_GEEPROM;
	ee.offset = eeprom.offset;
	ee.len = eeprom.len;

	ret = ndev->ethtool_ops->get_module_eeprom(ndev, &ee, buf);
	if (ret)
		goto out;

	if (copy_to_user(eeprom.data, buf, eeprom.len))
		ret = -EFAULT;

out:
	kfree(buf);
	return ret;
}

static long yk3_net_module_info(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct ethtool_modinfo modinfo;
	struct yk3_net_module_info info;
	int ret;

	if (IS_ERR_OR_NULL(ndev->ethtool_ops->get_module_info))
		return -EOPNOTSUPP;

	ret = ndev->ethtool_ops->get_module_info(ndev, &modinfo);
	if (ret)
		return ret;

	info.type = modinfo.type;
	info.eeprom_len = modinfo.eeprom_len;
	return copy_to_user((void __user *)arg, &info, sizeof(info)) ? -EFAULT : 0;
}

static long yk3_net_phys_id(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_phys_id phys_id;
	enum ethtool_phys_id_state state;

	if (IS_ERR_OR_NULL(ndev->ethtool_ops->set_phys_id))
		return -EOPNOTSUPP;

	if (copy_from_user(&phys_id, (void __user *)arg, sizeof(phys_id)))
		return -EFAULT;

	switch (phys_id.state) {
	case 0:
		state = ETHTOOL_ID_INACTIVE;
		priv->phys_id_active = false;
		break;
	case 1:
		state = ETHTOOL_ID_ACTIVE;
		priv->phys_id_active = true;
		break;
	default:
		return -EINVAL;
	}

	return ndev->ethtool_ops->set_phys_id(ndev, state);
}

static long yk3_net_coalesce(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_net_coalesce ne = {0};
	struct ethtool_coalesce ec = {0};
	int ret = 0;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (!ndev->ethtool_ops->set_coalesce)
			return -EOPNOTSUPP;

		if (copy_from_user(&ne, (void __user *)arg, sizeof(ne)))
			return -EFAULT;

		ec.rx_coalesce_usecs = ne.rx_coalesce_usecs;
		ec.rx_max_coalesced_frames = ne.rx_max_coalesced_frames;
		ec.tx_coalesce_usecs = ne.tx_coalesce_usecs;
		ec.tx_max_coalesced_frames = ne.tx_max_coalesced_frames;
#ifdef YK3_HAVE_ETHTOOL_COALESCE_CQE
		ret = ndev->ethtool_ops->set_coalesce(ndev, &ec, NULL, NULL);
#else
		ret = ndev->ethtool_ops->set_coalesce(ndev, &ec);
#endif
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_set_ifg(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_set_ifg ifg = {0};
	int ret = 0;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (copy_from_user(&ifg, (void __user *)arg, sizeof(ifg)))
			return -EFAULT;

		ret = yk3_mac_set_ifg(ndev_priv, ifg.ifg_num);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static long yk3_net_set_l1vq(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_net_set_l1vq l1vq = {0};
	int ret = 0;

	switch (_IOC_DIR(cmd)) {
	case _IOC_WRITE:
		if (copy_from_user(&l1vq, (void __user *)arg, sizeof(l1vq)))
			return -EFAULT;

		if (l1vq.mode == DPDK_MODE_PERFORMANCE)
			ret = yk3_set_pfc_pcie_l1vq(ndev_priv->ndev, 0x200, 0x160);
		else if (l1vq.mode == DPDK_MODE_LATENCY)
			ret = yk3_set_pfc_pcie_l1vq(ndev_priv->ndev, 0x100, 0xB0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static unlocked_ioctl yk3_comm_ioctls[] = {
	[YK3_COMM_DEVBIND] = yk3_comm_dev_bind,
	[YK3_COMM_SYSINFO] = yk3_comm_sysinfo,
	[YK3_COMM_CARDINFO] = yk3_comm_cardinfo,
};

static size_t yk3_comm_arg_sizes[] = {
	[YK3_COMM_DEVBIND] = sizeof(struct yk3_comm_devid),
	[YK3_COMM_SYSINFO] = sizeof(struct yk3_comm_sysinfo),
	[YK3_COMM_CARDINFO] = sizeof(struct yk3_comm_cardinfo),
};

static unlocked_ioctl yk3_net_ioctls[] = {
	[YK3_NET_DEVINFO] = yk3_net_devinfo,
	[YK3_NET_UMD] = yk3_net_umd,
	[YK3_NET_PCIBAR] = yk3_net_pcibar,
	[YK3_NET_MACADDR] = yk3_net_macaddr,
	[YK3_NET_MTU] = yk3_net_mtu,
	[YK3_NET_LINKINFO] = yk3_net_linkinfo,
	[YK3_NET_PROMISC] = yk3_net_promisc,
	[YK3_NET_ALLMULTI] = yk3_net_allmulti,
	[YK3_NET_DMAMAP] = yk3_net_dma_map,
	[YK3_NET_DMAUNMAP] = yk3_net_dma_unmap,
	[YK3_NET_START] = yk3_net_start,
	[YK3_NET_OFFLOADCAP] = yk3_net_offloadcap,
	[YK3_NET_VLAN_FILTER] = yk3_net_vlan_filter,
	[YK3_NET_RSS_HF] = yk3_net_rss_hf,
	[YK3_NET_RSS_HASH_KEY] = yk3_net_rss_key,
	[YK3_NET_RSS_RETA] = yk3_net_rss_reta,
	[YK3_NET_FLOW_CTRL] = yk3_net_flow_ctrl,
	[YK3_NET_VF_MAC_ANTI_SPOOF] = yk3_net_vf_mac_anti_spoof,
	[YK3_NET_PVID] = yk3_net_pvid,
	[YK3_NET_TX_PVID] = yk3_net_tx_pvid,
	[YK3_NET_QUEUE_RATE_LIMIT] = yk3_net_queue_rate_limit,
	[YK3_NET_MODULE_EEPROM] = yk3_net_module_eeprom,
	[YK3_NET_MODULE_INFO] = yk3_net_module_info,
	[YK3_NET_PHYS_ID] = yk3_net_phys_id,
	[YK3_NET_COALESCE] = yk3_net_coalesce,
	[YK3_NET_SET_IFG] = yk3_net_set_ifg,
	[YK3_NET_PFC_PCIE_L1] = yk3_net_set_l1vq,
};

static size_t yk3_net_arg_sizes[] = {
	[YK3_NET_DEVINFO] = sizeof(struct yk3_net_devinfo),
	[YK3_NET_UMD] = sizeof(struct yk3_net_umd),
	[YK3_NET_PCIBAR] = sizeof(struct yk3_net_pcibar),
	[YK3_NET_MACADDR] = sizeof(struct yk3_net_macaddr),
	[YK3_NET_MTU] = sizeof(struct yk3_net_mtu),
	[YK3_NET_LINKINFO] = sizeof(struct yk3_net_linkinfo),
	[YK3_NET_PROMISC] = sizeof(struct yk3_net_rxmode),
	[YK3_NET_ALLMULTI] = sizeof(struct yk3_net_rxmode),
	[YK3_NET_DMAMAP] = sizeof(struct yk3_net_dmamap),
	[YK3_NET_DMAUNMAP] = sizeof(struct yk3_net_dmamap),
	[YK3_NET_START] = sizeof(struct yk3_net_start),
	[YK3_NET_OFFLOADCAP] = sizeof(struct yk3_net_offloadcap),
	[YK3_NET_VLAN_FILTER] = sizeof(struct yk3_net_vlan_config),
	[YK3_NET_RSS_HF] = sizeof(struct yk3_net_rss_hf),
	[YK3_NET_RSS_HASH_KEY] = sizeof(struct yk3_net_rss_key),
	[YK3_NET_RSS_RETA] = sizeof(struct yk3_net_rss_reta),
	[YK3_NET_FLOW_CTRL] = sizeof(struct yk3_net_flow_ctrl),
	[YK3_NET_VF_MAC_ANTI_SPOOF] = sizeof(struct yk3_net_vf_mac_anti_spoof),
	[YK3_NET_PVID] = sizeof(struct yk3_net_pvid_config),
	[YK3_NET_TX_PVID] = sizeof(struct yk3_net_pvid_config),
	[YK3_NET_QUEUE_RATE_LIMIT] = sizeof(struct yk3_net_queue_rate_limit),
	[YK3_NET_MODULE_EEPROM] = sizeof(struct yk3_net_module_eeprom),
	[YK3_NET_MODULE_INFO] = sizeof(struct yk3_net_module_info),
	[YK3_NET_PHYS_ID] = sizeof(struct yk3_net_phys_id),
	[YK3_NET_COALESCE] = sizeof(struct yk3_net_coalesce),
	[YK3_NET_SET_IFG] = sizeof(struct yk3_net_set_ifg),
	[YK3_NET_PFC_PCIE_L1] = sizeof(struct yk3_net_set_l1vq),
};

static unlocked_ioctl yk3_meter_ioctls[] = {
	[YK3_METER_RX] = yk3_meter_rx,
	[YK3_METER_TX] = yk3_meter_tx,
};

static size_t yk3_meter_arg_sizes[] = {
	[YK3_METER_RX] = sizeof(struct ysc_meter_rate),
	[YK3_METER_TX] = sizeof(struct ysc_meter_rate),
};

static long yk3_comm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_COMM_MAX)
		return -EINVAL;

	if (_IOC_SIZE(cmd) != yk3_comm_arg_sizes[nr])
		return -EINVAL;

	if (nr != YK3_COMM_DEVBIND && !priv->pdev_priv)
		return -ENODEV;

	return yk3_comm_ioctls[nr](file, cmd, arg);
}

static long yk3_net_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_NET_MAX)
		return -EINVAL;

	if (!priv->pdev_priv || !priv->ndev_priv)
		return -ENODEV;

	if (nr != YK3_NET_UMD && nr != YK3_NET_DEVINFO && !priv->umd_enabled)
		return -EPERM;

	if (_IOC_SIZE(cmd) != yk3_net_arg_sizes[nr])
		return -EINVAL;

	if (_IOC_DIR(cmd) & _IOC_WRITE)
		yk3_cfg_flags_set(priv, _IOC_NR(cmd));

	return yk3_net_ioctls[nr](file, cmd, arg);
}

static long yk3_meter_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_METER_MAX)
		return -EINVAL;

	if (!priv->pdev_priv || !priv->ndev_priv)
		return -ENODEV;

	if (_IOC_SIZE(cmd) != yk3_meter_arg_sizes[nr])
		return -EINVAL;

	return yk3_meter_ioctls[nr](file, cmd, arg);
}

static long yk3_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (_IOC_TYPE(cmd)) {
	case YK3_IOCTL_COMM_TYPE:
		return yk3_comm_ioctl(file, cmd, arg);

	case YK3_IOCTL_NET_TYPE:
		return yk3_net_ioctl(file, cmd, arg);

	case YK3_IOCTL_METER_TYPE:
		return yk3_meter_ioctl(file, cmd, arg);

	case YK3_IOCTL_DOE_TYPE:
		return yk3_doe_ioctl(file, cmd, arg);

	default:
		return -EINVAL;
	};
}

static void yk3_release_restore_cfg(struct yk3_cdev_priv *priv, int type, void *data)
{
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_PARAM
	struct ethtool_rxfh_param rxfh = { 0 };
#endif
	struct ethtool_pauseparam *pause;
	struct ethtool_rxnfc rxnfc;
	struct yk3_rss_cfg *cfg;
	struct yk3_pfc_pcie_l1 *pfc_pcie_l1;
	int i;

	switch (type) {
	case YK3_NET_MACADDR:
		yk3_net_macaddr_set(ndev_priv, data);
		break;
	case YK3_NET_MTU:
		yk3_net_mtu_set(ndev_priv, *(int *)data);
		break;
	case YK3_NET_PROMISC:
		yk3_net_promisc_set(ndev_priv, !!(*((unsigned int *)data) & IFF_PROMISC));
		break;
	case YK3_NET_ALLMULTI:
		yk3_net_allmulti_set(ndev_priv, !!(*((unsigned int *)data) & IFF_ALLMULTI));
		break;
	case YK3_NET_OFFLOADCAP:
		yk3_net_offloadcap_set(ndev_priv, *(netdev_features_t *)data);
		break;
	case YK3_NET_RSS_HF:
		cfg = (struct yk3_rss_cfg *)data;
		for (i = 0; i < cfg->num; i++) {
			memset(&rxnfc, 0, sizeof(rxnfc));
			rxnfc.cmd = ETHTOOL_SRXFH;
			rxnfc.flow_type = cfg->entry[i].flow_type;
			rxnfc.data = cfg->entry[i].data;

			ndev->ethtool_ops->set_rxnfc(ndev, &rxnfc);
		}
		break;
#ifdef YK3_HAVE_ETHTOOL_SET_RXFH_PARAM
	case YK3_NET_RSS_HASH_KEY:
		rxfh.key = (u8 *)data;
		rxfh.key_size = YK3_RSS_KEY_SIZE;
		if (ndev->ethtool_ops->set_rxfh)
			ndev->ethtool_ops->set_rxfh(ndev, &rxfh, NULL);
		break;
	case YK3_NET_RSS_RETA:
		rxfh.indir = (u32 *)data;
		rxfh.key_size = YK3_MAX_RSS_RETA_SIZE;
		if (ndev->ethtool_ops->set_rxfh)
			ndev->ethtool_ops->set_rxfh(ndev, &rxfh, NULL);
		break;
#else
	case YK3_NET_RSS_HASH_KEY:
		if (ndev->ethtool_ops->set_rxfh)
			ndev->ethtool_ops->set_rxfh(ndev, NULL, (u8 *)data, 0);
		break;
	case YK3_NET_RSS_RETA:
		if (ndev->ethtool_ops->set_rxfh)
			ndev->ethtool_ops->set_rxfh(ndev, (u32 *)data, NULL, 0);
		break;
#endif /* YK3_HAVE_ETHTOOL_SET_RXFH_PARAM */
	case YK3_NET_FLOW_CTRL:
		pause = (struct ethtool_pauseparam *)data;
		if (ndev->ethtool_ops->set_pauseparam)
			ndev->ethtool_ops->set_pauseparam(ndev, pause);
		break;
	case YK3_NET_COALESCE:
		if (ndev->ethtool_ops->set_coalesce) {
#ifdef YK3_HAVE_ETHTOOL_COALESCE_CQE
			ndev->ethtool_ops->set_coalesce(ndev, data, NULL, NULL);
#else
			ndev->ethtool_ops->set_coalesce(ndev, data);
#endif
		}
		break;
	case YK3_NET_VF_CONF:
		if (yk3_pdev_is_pf(priv->pdev_priv)) {
			yk3_lan_umd_set_spoofchk(ndev_priv, false);
			yk3_lan_umd_set_pvid(ndev_priv, 0, 0, 0, false);
		} else if (yk3_pdev_is_vf(priv->pdev_priv)) {
			yk3_lan_umd_restore_vf_conf(ndev_priv);
		}
		break;
	case YK3_NET_SET_IFG:
		yk3_mac_set_ifg(ndev_priv, *(u8 *)data);
		break;
	case YK3_NET_PFC_PCIE_L1:
		pfc_pcie_l1 = (struct yk3_pfc_pcie_l1 *)data;
		yk3_restore_pfc_config(ndev_priv, pfc_pcie_l1);
		break;
	default:
		break;
	}
}

static void yk3_remove_all_cvlan(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_vlan *vlan;

	while (!list_empty(&ndev_priv->cvlan_list)) {
		vlan = list_first_entry(&ndev_priv->cvlan_list,
					struct yk3_vlan, list);

		ndev->netdev_ops->ndo_vlan_rx_kill_vid(ndev,
						       htons(ETH_P_8021Q),
						       vlan->vlan_id);
	}
}

static void yk3_clear_all_queue_txrate_limit(struct net_device *ndev)
{
#ifdef YK3_HAVE_NDO_SET_TX_MAXRATE
	int i;

	for (i = 0; i < ndev->real_num_tx_queues; i++)
		ndev->netdev_ops->ndo_set_tx_maxrate(ndev, i, 0);
#else
	(void)ndev;
#endif /* YK3_HAVE_NDO_SET_TX_MAXRATE */
}

static int yk3_release(struct inode *inode, struct file *file)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_ndev_priv *ndev_priv = priv->ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct net_device *ndev;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_sriov_mbox_umd_vf_msg *umd_vf;
	struct yk3_sriov_mbox_ack_msg *ack;
	struct yk3_mbox_option mbox_opt = {0};
	int ret;

	if (priv->dmamap_table)
		yk3_dmamap_table_destroy(priv->dmamap_table);

	if (!priv->umd_enabled)
		goto out;

	if (!ndev_priv)
		goto out;

	pdev_priv = ndev_priv->pdev_priv;
	if (!pdev_priv)
		goto out;

	if (yk3_ndev_is_vf(ndev_priv)) {
		umd_vf = (struct yk3_sriov_mbox_umd_vf_msg *)mbox_msg.data;
		umd_vf->vf_id = ndev_priv->vf_id - 1;
		umd_vf->enable = 0;

		mbox_msg.opcode = YK3_MBOX_OPCODE_UMD_VF_ENABLE;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 3000;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg);
		if (ret != 0) {
			yk3_dev_err("cdev umd send mbox message errno %d failed!\n", ret);
		} else {
			ack = (struct yk3_sriov_mbox_ack_msg *)ack_msg.data;
			if (ack->ret != YK3_SRIOV_MBOX_OK)
				yk3_dev_err("sriov pf set vf umd disable failed ret code %d!\n",
					    ack->ret);
		}
	}

	/* turn off led if phys_id was active */
	ndev = ndev_priv->ndev;
	if (priv->phys_id_active && ndev->ethtool_ops->set_phys_id)
		ndev->ethtool_ops->set_phys_id(ndev, ETHTOOL_ID_INACTIVE);

	/* restore config */
	yk3_cfg_restore(priv, yk3_release_restore_cfg);
	yk3_remove_all_cvlan(ndev_priv->ndev);
	yk3_clear_all_queue_txrate_limit(ndev_priv->ndev);

	if (ndev_priv->umd_enable) {
		rtnl_lock();
		/* Change umd state before to call dev_open -> ys_ndo_open.*/
		ndev_priv->umd_enable = false;
		netif_device_attach(ndev_priv->ndev);
#ifdef YK3_HAVE_DEV_OPEN_NETLINK
		dev_open(ndev_priv->ndev, NULL);
#else
		dev_open(ndev_priv->ndev);
#endif /* YK3_HAVE_DEV_OPEN_NETLINK */
		rtnl_unlock();
		yk3_net_debug("umd disable");
	}
out:
	kfree(priv);
	file->private_data = NULL;
	return 0;
}

static const struct file_operations yk3_fops = {
	.owner = THIS_MODULE,
	.open = yk3_open,
	.mmap = yk3_mmap,
	.unlocked_ioctl = yk3_ioctl,
	.release = yk3_release,
};

int yk3_cdev_init(void)
{
	int ret;

	yk3_mdev.minor = MISC_DYNAMIC_MINOR;
	yk3_mdev.name = "yk3";
	yk3_mdev.fops = &yk3_fops;

	ret = misc_register(&yk3_mdev);
	if (ret) {
		yk3_err("misc_register failed: %d\n", ret);
		return ret;
	}

	return 0;
}

void yk3_cdev_exit(void)
{
	misc_deregister(&yk3_mdev);
}
