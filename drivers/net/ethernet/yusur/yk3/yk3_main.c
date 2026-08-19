// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"

static void yk3_pdev_unmap(struct yk3_pdev_priv *pdev_priv);

static const struct devlink_ops yk3_devlink_ops = {
};

static int yk3_pdev_remap(struct yk3_pdev_priv *pdev_priv)
{
	struct pci_dev *pdev = pdev_priv->pdev;
	resource_size_t bar_start, bar_end, bar_offset = 0;
	unsigned long bar_flags;
	int i;

	for (i = 0; i < YK3_BAR_MAX; i++) {
		bar_start = pci_resource_start(pdev, i);
		bar_end = pci_resource_end(pdev, i);
		bar_flags = pci_resource_flags(pdev, i);

		pdev_priv->bar_size[i] = pci_resource_len(pdev, i);
		if (!pdev_priv->bar_size[i]) {
			pdev_priv->bar_addr[i] = NULL;
			continue;
		}

		pdev_priv->bar_addr[i] = ioremap(bar_start, pdev_priv->bar_size[i]);
		if (!pdev_priv->bar_addr[i]) {
			yk3_dev_err("could not map BAR_%d[0x%08llx-0x%08llx] flag[0x%08lx]",
				    i, bar_start, bar_end, bar_flags);
			goto failed;
		}

		pdev_priv->bar_pa[i] = bar_start;

		pdev_priv->bar_offset[i] = bar_offset;
		bar_offset += pdev_priv->bar_size[i];
	}

	return 0;

failed:
	yk3_pdev_unmap(pdev_priv);
	return -EIO;
}

static void yk3_pdev_unmap(struct yk3_pdev_priv *pdev_priv)
{
	int i;

	for (i = 0; i < YK3_BAR_MAX; i++) {
		if (pdev_priv->bar_addr[i])
			iounmap(pdev_priv->bar_addr[i]);
		pdev_priv->bar_addr[i] = NULL;
		pdev_priv->bar_size[i] = 0;
		pdev_priv->bar_offset[i] = 0;
		pdev_priv->bar_pa[i] = 0;
	}
}

#ifndef PCIE_IRQ_TIMEOUT
#define PCIE_IRQ_TIMEOUT 0x3fffff
#endif

static int yk3_early_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		void __iomem *bar = pdev_priv->bar_addr[YK3_BAR0];
		// TODO: only support k3?
		// set pcie irq timeout
		yk3_wr32(bar, 0x200000, PCIE_IRQ_TIMEOUT); // ch0 timeout_rd_para
		yk3_wr32(bar, 0x200004, PCIE_IRQ_TIMEOUT); // ch0 timeout_wr_para
		yk3_wr32(bar, 0x200040, PCIE_IRQ_TIMEOUT); // ch1 timeout_rd_para
		yk3_wr32(bar, 0x200044, PCIE_IRQ_TIMEOUT); // ch1 timeout_wr_para
		yk3_wr32(bar, 0x200080, PCIE_IRQ_TIMEOUT); // ch2 timeout_rd_para
		yk3_wr32(bar, 0x200084, PCIE_IRQ_TIMEOUT); // ch2 timeout_wr_para
		yk3_wr32(bar, 0x2000c0, PCIE_IRQ_TIMEOUT); // ch3 timeout_rd_para
		yk3_wr32(bar, 0x2000c4, PCIE_IRQ_TIMEOUT); // ch3 timeout_wr_para
	}

	ret = yk3_edma_early_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_edma_early_init() failed\n");
		return ret;
	}

	return 0;
}

static int yk3_rdma_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret;
	struct yk3_rdma_data *rdma_data;
	struct yk3_ndev_priv *ndev_priv;
	struct platform_device *platform_dev;
	int vec_start;
	int vec_num;

	if (!yk3_pdev_is_pf(pdev_priv) ||
	    pdev_priv->card->mode != YK3_MODE_RCARD)
		return 0;

	yk3_irq_rdma_init(pdev_priv);

	rdma_data = kzalloc(sizeof(*rdma_data), GFP_KERNEL);
	if (!rdma_data)
		return -ENOMEM;

	ret = yk3_irq_rdma_get(pdev_priv, &vec_start, &vec_num);
	if (ret) {
		yk3_dev_err("yk3_irq_rdma_get() failed\n");
		goto err_get_irq;
	}
	rdma_data->vector_start = vec_start;
	rdma_data->vector_num = vec_num;
	rdma_data->version_major = 1;
	rdma_data->version_minor = 0;
	rdma_data->pf_num = pdev_priv->card->pf_num;

	pdev_priv->rdma_data = rdma_data;

	rdma_data->pdev = pdev_priv->pdev;
	rdma_data->pdev_priv = pdev_priv;

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv) {
		yk3_dev_err("yk3_pdev_get_ndev_priv() failed\n");
		ret = -ENODEV;
		goto err_get_ndev_priv;
	}

	rdma_data->ndev = ndev_priv->ndev;

	// doe
	rdma_data->ops.create_tbl = yk3_doe_create_tbl;
	rdma_data->ops.delete_tbl = yk3_doe_delete_tbl;
	rdma_data->ops.entry_add = yk3_doe_entry_add;
	rdma_data->ops.entry_delete = yk3_doe_entry_delete;
	rdma_data->ops.entry_update = yk3_doe_entry_update;
	rdma_data->ops.entry_query = yk3_doe_entry_query;
	rdma_data->ops.ctl_get = yk3_doe_ctl_get;
	rdma_data->ops.ctl_set = yk3_doe_ctl_set;

	rdma_data->ops.ppp_rdma_enable = yk3_ppp_rdma_enable;
	rdma_data->ops.ppp_rdma_disable = yk3_ppp_rdma_disable;
	rdma_data->ops.ppp_action_modify = yk3_ppp_action_modify;
	rdma_data->ops.ppp_action_query = yk3_ppp_action_query;

	rdma_data->ops.ppp_eme_create_table = yk3_ppp_eme_create_table;
	rdma_data->ops.ppp_eme_delete_table = yk3_ppp_eme_delete_table;
	rdma_data->ops.ppp_eme_add_rule = yk3_ppp_eme_add_rule;
	rdma_data->ops.ppp_eme_del_rule = yk3_ppp_eme_del_rule;
	rdma_data->ops.ppp_eme_mod_rule = yk3_ppp_eme_mod_rule;
	rdma_data->ops.ppp_eme_get_rule = yk3_ppp_eme_get_rule;

	rdma_data->ops.set_qp_tc = yk3_qos_set_qp_tc;
	rdma_data->ops.set_qp_rate = yk3_qos_set_qp_rate;
	rdma_data->ops.set_rt_dst = yk3_qos_set_rt_dst;

	rdma_data->ops.rdma_set_pfc = yk3_rdma_setpfc;
	rdma_data->ops.rdma_get_pfc = yk3_rdma_getpfc;

	platform_dev = platform_device_alloc(YK3_RDMA_DEV_NAME, pdev_priv->pf_id);
	if (!platform_dev) {
		yk3_dev_err("platform_device_alloc() failed\n");
		ret = -ENOMEM;
		goto err_platform_device_alloc;
	}

	platform_dev->dev.parent = pdev_priv->dev;

	ret = platform_device_add_data(platform_dev, rdma_data, sizeof(*rdma_data));
	if (ret) {
		yk3_dev_err("platform_device_add_data() failed\n");
		goto err_platform_device_add_data;
	}

	ret = platform_device_add(platform_dev);
	if (ret) {
		yk3_dev_err("platform_device_add() failed\n");
		goto err_platform_device_add;
	}

	pdev_priv->platform_dev = platform_dev;

	return 0;

err_platform_device_add:
err_platform_device_add_data:
	platform_device_put(platform_dev);
	pdev_priv->platform_dev = NULL;
err_platform_device_alloc:
err_get_ndev_priv:
err_get_irq:
	kfree(rdma_data);
	pdev_priv->rdma_data = NULL;
	return ret;
}

static void yk3_rdma_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (!yk3_pdev_is_pf(pdev_priv) ||
	    pdev_priv->card->mode != YK3_MODE_RCARD)
		return;

	platform_device_unregister(pdev_priv->platform_dev);
	pdev_priv->platform_dev = NULL;
	kfree(pdev_priv->rdma_data);
	pdev_priv->rdma_data = NULL;
}

static void yk3_reset_init(struct yk3_pdev_priv *pdev_priv)
{
	if (yk3_pdev_is_mgr(pdev_priv)) {
		void __iomem *bar = pdev_priv->bar_addr[YK3_BAR0];
		// set emp reset status for init
		yk3_wr32(bar, 0x2200154, 0x1);
	}
}

static void yk3_reset_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (yk3_pdev_is_mgr(pdev_priv)) {
		void __iomem *bar = pdev_priv->bar_addr[YK3_BAR0];
		// set emp reset status for exit
		yk3_wr32(bar, 0x2200154, 0x2);
	}
}

static int yk3_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct yk3_pdev_priv *pdev_priv = NULL;
	struct device *dev = &pdev->dev;
	int ret;
	u32 val;
#ifdef YK3_HAVE_DEVLINK_PARAM_DRIVER
	struct devlink *devlink = NULL;

#ifdef YK3_HAVE_DEVLINK_ALLOC_DEV
	devlink = devlink_alloc(&yk3_devlink_ops, sizeof(*pdev_priv), dev);
#else
	devlink = devlink_alloc(&yk3_devlink_ops, sizeof(*pdev_priv));
#endif
	if (!devlink) {
		dev_err(dev, "pci_devlink_alloc() failed\n");
		ret = -ENOMEM;
		goto err_priv_alloc;
	}

	pdev_priv = devlink_priv(devlink);
#else
	pdev_priv = kzalloc(sizeof(*pdev_priv), GFP_KERNEL);
	if (!pdev_priv) {
		ret = -ENOMEM;
		goto err_priv_alloc;
	}
#endif /* YK3_HAVE_DEVLINK_PARAM_DRIVER */

	pci_set_drvdata(pdev, pdev_priv);
	snprintf(pdev_priv->name, sizeof(pdev_priv->name), "%04x:%02x:%02x.%d",
		 pci_domain_nr(pdev->bus), pdev->bus->number,
		 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	pdev_priv->dev = dev;
	pdev_priv->pdev = pdev;
	INIT_LIST_HEAD(&pdev_priv->card_node);
	INIT_LIST_HEAD(&pdev_priv->ndev_priv_head);
	mutex_init(&pdev_priv->ndev_priv_head_mlock);

	ret = pci_enable_device(pdev);
	if (ret) {
		yk3_dev_err("pci_enable_device() failed\n");
		goto err_pci_enable;
	}
	pci_set_master(pdev);

	/* Request MMIO/IOP resources */
	ret = pci_request_regions(pdev, pdev->driver->name);
	if (ret) {
		yk3_dev_err("pci_request_regions() failed\n");
		goto err_regions;
	}

	ret = yk3_pdev_remap(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_pdev_remap() failed\n");
		goto err_remap;
	}

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		yk3_dev_err("dma_set_mask_and_coherent() failed\n");
		goto err_dma_mask_coherent;
	}
	dma_set_max_seg_size(&pdev->dev, DMA_BIT_MASK(32));

	pdev_priv->vendor = id->vendor;
	pdev_priv->device = id->device;
	val = yk3_rd32(pdev_priv->bar_addr[YK3_BAR0], YK3_RP_PFVFID);
	pdev_priv->pf_id = FIELD_GET(YK3_RP_PFID_GMASK, val);
	pdev_priv->vf_id = FIELD_GET(YK3_RP_VFID_GMASK, val);

	ret = yk3_early_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_early_init() failed\n");
		goto err_early_init;
	}

	yk3_reset_init(pdev_priv);

	ret = yk3_debug_pdev_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_debug_pdev_init() failed\n");
		goto err_debug_pdev_init;
	}

	ret = yk3_irq_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_irq_init() failed\n");
		goto err_irq_init;
	}

	/* yk3_mbox init */
	ret = yk3_mbox_init(pdev_priv);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			yk3_dev_err("yk3_mbox_init() failed\n");
		goto err_mbox_init;
	}

	ret = yk3_card_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_card_init() failed\n");
		goto err_card_init;
	}

	ret = yk3_qset_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_qset_init() failed\n");
		goto err_qset_init;
	}

	ret = yk3_qos_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_qos_init() failed\n");
		goto err_qos_init;
	}

	ret = yk3_edma_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_edma_init() failed\n");
		goto err_edma_init;
	}

	/* doe init */
	ret = yk3_doe_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_doe_init() failed\n");
		goto err_doe_init;
	}

	ret = yk3_lan_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_lan_init() failed\n");
		goto err_lan_init;
	}

	/* mac init */
	ret = yk3_mac_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_mac_init() failed\n");
		goto err_mac_init;
	}

	/* ppp init */
	ret = yk3_ppp_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_ppp_init() failed\n");
		goto err_ppp_init;
	}

	/* np init */
	ret = yk3_np_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_np_init() failed.\n");
		goto err_np_init;
	}

	/* ptp init */
	ret = yk3_ptp_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_ptp_init failed. ret=%d\n", ret);
		goto err_ptp_init;
	}

	/* ndev init */
	ret = yk3_ndev_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_ndev_init() failed\n");
		goto err_ndev_init;
	}

	ret = yk3_sriov_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_sriov_init() failed\n");
		goto err_sriov_init;
	}

	ret = yk3_rdma_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_rdma_init() failed\n");
		goto err_rdma_init;
	}

	return 0;

err_rdma_init:
	yk3_sriov_exit(pdev_priv);
err_sriov_init:
	yk3_ndev_exit(pdev_priv);
err_ndev_init:
	yk3_ptp_exit(pdev_priv);
err_ptp_init:
	yk3_np_exit(pdev_priv);
err_np_init:
	yk3_ppp_exit(pdev_priv);
err_ppp_init:
	yk3_mac_exit(pdev_priv);
err_mac_init:
	yk3_lan_exit(pdev_priv);
err_lan_init:
	yk3_doe_exit(pdev_priv);
err_doe_init:
	yk3_edma_exit(pdev_priv);
err_edma_init:
	yk3_qos_exit(pdev_priv);
err_qos_init:
	yk3_qset_exit(pdev_priv);
err_qset_init:
	yk3_card_exit(pdev_priv);
err_card_init:
	yk3_mbox_exit(pdev_priv);
err_mbox_init:
	yk3_irq_exit(pdev_priv);
err_irq_init:
	yk3_debug_pdev_exit(pdev_priv);
err_debug_pdev_init:
err_early_init:
err_dma_mask_coherent:
	yk3_pdev_unmap(pdev_priv);
err_remap:
	pci_release_regions(pdev);
err_regions:
	pci_clear_master(pdev);
	pci_disable_device(pdev);
err_pci_enable:
	mutex_destroy(&pdev_priv->ndev_priv_head_mlock);
#ifdef YK3_HAVE_DEVLINK_PARAM_DRIVER
	devlink_free(devlink);
#else
	kfree(pdev_priv);
#endif
	pci_set_drvdata(pdev, NULL);
err_priv_alloc:
	return ret;
}

static void yk3_remove(struct pci_dev *pdev)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(pdev);
#ifdef YK3_HAVE_DEVLINK_PARAM_DRIVER
	struct devlink *devlink;
#endif
	if (!pdev_priv)
		return;
	yk3_rdma_exit(pdev_priv);
	yk3_sriov_exit(pdev_priv);
	yk3_ndev_exit(pdev_priv);
	yk3_ptp_exit(pdev_priv);
	/* np exit */
	yk3_np_exit(pdev_priv);
	/* ppp exit */
	yk3_ppp_exit(pdev_priv);
	/* mac exit */
	yk3_mac_exit(pdev_priv);
	yk3_lan_exit(pdev_priv);
	/* doe exit */
	yk3_doe_exit(pdev_priv);
	yk3_edma_exit(pdev_priv);
	yk3_qos_exit(pdev_priv);
	yk3_qset_exit(pdev_priv);
	yk3_card_exit(pdev_priv);
	yk3_mbox_exit(pdev_priv);
	yk3_irq_exit(pdev_priv);
	yk3_debug_pdev_exit(pdev_priv);
	yk3_reset_exit(pdev_priv);
	yk3_pdev_unmap(pdev_priv);
	pci_release_regions(pdev);
	pci_clear_master(pdev);
	pci_disable_device(pdev);

	mutex_destroy(&pdev_priv->ndev_priv_head_mlock);
#ifdef YK3_HAVE_DEVLINK_PARAM_DRIVER
	devlink = priv_to_devlink(pdev_priv);
	devlink_free(devlink);
#else
	kfree(pdev_priv);
#endif
	pci_set_drvdata(pdev, NULL);
}

static void yk3_mgr_shutdown(struct pci_dev *pdev)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(pdev);

	yk3_reset_exit(pdev_priv);
}

static const struct pci_device_id yk3_pci_ids[] = {
	{ PCI_VDEVICE(YUSUR, YK3_DEV_ID_PF) },
	{ PCI_VDEVICE(YUSUR, YK3_DEV_ID_VF) },
	{ PCI_VDEVICE(YUSUR, YK3R_DEV_ID_PF) },
	{ PCI_VDEVICE(YUSUR, YK3R_DEV_ID_VF) },
	{ 0, },
};

static struct pci_driver yk3_driver = {
	.name = KBUILD_MODNAME,
	.id_table = yk3_pci_ids,
	.probe = yk3_probe,
	.remove = yk3_remove,
	.sriov_configure = yk3_sriov_configure,
	.shutdown = yk3_remove,
};

static const struct pci_device_id yk3_mgr_pci_ids[] = {
	{ PCI_VDEVICE(YUSUR, YK3_DEV_ID_MGR) },
	{ PCI_VDEVICE(YUSUR, YK3R_DEV_ID_MGR) },
	{ 0, },
};

static struct pci_driver yk3_mgr_driver = {
	.name = "yk3_mgr",
	.id_table = yk3_mgr_pci_ids,
	.probe = yk3_probe,
	.remove = yk3_remove,
	.shutdown = yk3_mgr_shutdown,
};

static int yk3_param_init(void)
{
	int ret;

	ret = yk3_doe_param_init();
	if (ret)
		return ret;

	return 0;
}

static int __init yk3_init(void)
{
	int ret;

	yk3_info("YUSUR K3 and K3MAX Driver %s Init ...\n", THIS_MODULE->name);

	ret = yk3_param_init();
	if (ret)
		goto err_mgr;

	yk3_debug_init();

	ret = pci_register_driver(&yk3_mgr_driver);
	if (ret) {
		yk3_err("pci_register_driver() mgr failed\n");
		goto err_mgr;
	}

	ret = pci_register_driver(&yk3_driver);
	if (ret) {
		yk3_err("pci_register_driver() failed\n");
		goto err_pci;
	}

	ret = yk3_cdev_init();
	if (ret) {
		yk3_err("yk3_cdev_init() failed\n");
		goto err_cdev;
	}

	yk3_info("YUSUR K3 and K3MAX Driver %s Init\n", THIS_MODULE->name);
	return 0;

err_cdev:
	pci_unregister_driver(&yk3_driver);
err_pci:
	pci_unregister_driver(&yk3_mgr_driver);
err_mgr:
	yk3_debug_exit();
	return ret;
}

static void __exit yk3_exit(void)
{
	yk3_info("YUSUR K3 and K3MAX Driver %s Exit ...\n", THIS_MODULE->name);
	yk3_cdev_exit();
	pci_unregister_driver(&yk3_driver);
	pci_unregister_driver(&yk3_mgr_driver);
	yk3_debug_exit();
	yk3_info("YUSUR K3 and K3MAX Driver %s Exit\n", THIS_MODULE->name);
}

module_init(yk3_init);
module_exit(yk3_exit);

MODULE_DESCRIPTION("Yusur K3 and K3MAX Pcie Device Driver");
MODULE_AUTHOR("YUSUR Technology Co., Ltd.");
MODULE_LICENSE("GPL");
MODULE_VERSION(YK3_GIT_VERSION);
