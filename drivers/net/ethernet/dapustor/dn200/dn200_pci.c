// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include <linux/clk-provider.h>
#include <linux/pci.h>
#include <linux/dmi.h>
#include <linux/msi.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/aer.h>
#include "dn200.h"
#include "dn200_self.h"
#include "linux/delay.h"
#include "dn200_sriov.h"
#include "dn200_ctrl.h"
#include "dn200_prod.h"

#define DRV_SUMMARY	"DapuStor(R) Ethernet Connection DN200 Series Linux Driver"
static const char dn200_driver_str[] = DRV_SUMMARY;
static const char dn200_copyright[] = "Copyright (c) 2024, DapuStor Corporation.";

MODULE_AUTHOR("DapuStor Corporation, <fae@dapustor.com>");
MODULE_DESCRIPTION(DRV_SUMMARY);
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRV_MODULE_VERSION);

static int queue_max_set = 8;
module_param(queue_max_set, int, 0644);
MODULE_PARM_DESC(queue_max_set,
		 "PF0 and PF1 channel number set.range from 1 to 8");

#define XGE_NUM 4
#define DN200_PCI_BAR_NUM 6

struct pcb_type dn200_pcb_type[] = {
	{ 0b000, DN200_HW_PCB_TYPE_3 },
	{ 0b001, DN200_HW_PCB_TYPE_0 },
	{ 0b010, DN200_HW_PCB_TYPE_2 },
	{ 0b011, DN200_HW_PCB_TYPE_1 },
};

static void dn200_set_clk_conf(struct dn200_ctrl_resource *ctrl, u32 off,
			       u32 val)
{
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (plat_ex->speed_cmd)
		fw_reg_write(ctrl, 0x24300000 + off, val);
	else
		writel(val, plat_ex->io_addr + DN200_PCIE_BAROFF + off);
}

static void dn200_get_clk_conf(struct dn200_ctrl_resource *ctrl, u32 off,
			       u32 *val)
{
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (plat_ex->speed_cmd)
		fw_reg_read(ctrl, 0x24300000 + off, val);
	else
		*val = readl(plat_ex->io_addr + DN200_PCIE_BAROFF + off);
}

/*xgmac use PLLA*/
static bool dn200_clock_phy_detect(struct pci_dev *pdev,
				   struct dn200_ctrl_resource *ctrl)
{
	u32 val_refa0 = 0, val_refa1 = 0;
	int retry = 0;
	int val_judge = 0;
	u32 val[2];
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	dn200_get_clk_conf(ctrl, PCIE_PHY0_REFA_CLKDET_EN_REG, &val[0]);
	dn200_get_clk_conf(ctrl, PCIE_PHY1_REFA_CLKDET_EN_REG, &val[1]);

	dn200_set_clk_conf(ctrl, PCIE_PHY0_REFA_CLKDET_EN_REG, val[0] | 0x1);
	dn200_set_clk_conf(ctrl, PCIE_PHY1_REFA_CLKDET_EN_REG, val[1] | 0x1);
	do {
		dn200_get_clk_conf(ctrl, PCIE_PHY0_REFA_CLKDET_RESULT_REG,
				&val_refa0);
		if (plat_ex->raid_supported) {
			if (!val_refa0) {
				val_judge = 0;
			} else {
				val_judge = 1;
				break;
			}
		} else {
			dn200_get_clk_conf(ctrl, PCIE_PHY1_REFA_CLKDET_RESULT_REG,
				&val_refa1);
			if (!val_refa0 || !val_refa1)
				val_judge = 0;
			else {
				val_judge = 1;
				break;
			}
		}
		usleep_range(100, 200);
		retry++;
	} while (retry < 200);

	dn200_set_clk_conf(ctrl, PCIE_PHY0_REFA_CLKDET_EN_REG, val[0]);
	dn200_set_clk_conf(ctrl, PCIE_PHY1_REFA_CLKDET_EN_REG, val[1]);

	if (val_judge)
		return 1;

	dev_err(&pdev->dev, "func = %s, phy clock not found\n", __func__);
	return 0;
}

struct dn200_pci_info {
	int (*setup)(struct pci_dev *pdev, struct plat_dn200enet_data *plat,
		     struct plat_dn200_data *plat_ex);
};

static int dn200_xgmac_gpio_data_set(struct pci_dev *pdev,
				     struct plat_dn200_data *plat_ex,
				     const struct dn200_gpio_data *gpio_data,
				     u8 off)
{
	struct dn200_gpio_data *gpio;

	plat_ex->gpio_data =
	    devm_kzalloc(&pdev->dev, sizeof(struct dn200_gpio_data),
			 GFP_KERNEL);
	if (!plat_ex->gpio_data)
		return -ENOMEM;

	gpio = plat_ex->gpio_data;
	gpio->gpio_addr_offset = gpio_data[off].gpio_addr_offset;
	gpio->sfp_detect_pin = gpio_data[off].sfp_detect_pin;
	gpio->sfp_tx_disable_pin = gpio_data[off].sfp_tx_disable_pin;
	gpio->sfp_tx_fault_pin = gpio_data[off].sfp_tx_fault_pin;
	gpio->sfp_rx_los_pin = gpio_data[off].sfp_rx_los_pin;
	gpio->sfp_rs0_pin = gpio_data[off].sfp_rs0_pin;
	gpio->sfp_rs1_pin = gpio_data[off].sfp_rs1_pin;
	gpio->sfp_led1_pin = gpio_data[off].sfp_led1_pin;
	gpio->sfp_led2_pin = gpio_data[off].sfp_led2_pin;
	gpio->reg_off_set_write = gpio_data[off].reg_off_set_write;
	gpio->reg_off_set_read = gpio_data[off].reg_off_set_read;
	return 0;
}

static void dn200_xgmac_private_data_set(struct pci_dev *pdev,
					 struct plat_dn200enet_data *plat,
					 struct plat_dn200_data *plat_ex,
					 const struct xge_private_data *xge_data,
					 const struct xge_link_config *link_config)
{

	plat->phy_addr = xge_data->phy_addr;
	plat->bus_id = xge_data->bus_id;

	plat->clk_ptp_rate = link_config->clk_ptp_rate;	// 250MHz for XGMAC, 50MHz for GMAC
	plat->clk_csr = link_config->clk_csr;	// DN200_CSR_I_4
	plat->phy_interface = link_config->phy_interface;
	plat->max_speed = link_config->max_speed;
	plat_ex->max_speed = link_config->max_speed;
	plat->clk_ref_rate = link_config->clk_ref_rate;
	plat_ex->has_xpcs = link_config->has_xpcs;
	switch (plat->bus_id) {
	case 1:
	case 2:
		plat->tx_queues_to_use = queue_max_set;
		plat->rx_queues_to_use = queue_max_set;
		plat_ex->rx_queues_reserved =
		    xge_data->rx_queues_total - queue_max_set;
		plat_ex->tx_queues_reserved =
		    xge_data->tx_queues_total - queue_max_set;
		plat_ex->max_vfs = plat_ex->tx_queues_reserved;
		break;
	case 3:
	case 4:
		if (plat_ex->sriov_supported) {
			plat->tx_queues_to_use =
			    xge_data->tx_queues_total -
			    xge_data->tx_queues_reserved;
			plat->rx_queues_to_use =
			    xge_data->rx_queues_total -
			    xge_data->rx_queues_reserved;
		} else {
			plat->tx_queues_to_use = xge_data->tx_queues_to_use;
			plat->rx_queues_to_use = xge_data->rx_queues_to_use;
		}
		plat_ex->rx_queues_reserved = xge_data->rx_queues_reserved;
		plat_ex->tx_queues_reserved = xge_data->tx_queues_reserved;
		plat_ex->max_vfs = xge_data->max_vfs;
		break;
	default:
		dev_err(&pdev->dev, "Invalid bus id %x\n", plat->bus_id);
		break;
	}
	plat_ex->tx_queues_total = xge_data->tx_queues_total;
	plat_ex->rx_queues_total = xge_data->rx_queues_total;
	plat_ex->rx_used_mtl_queues = xge_data->rx_used_mtl_queues;
	plat_ex->default_rx_queue_num = plat->rx_queues_to_use;
	plat_ex->default_tx_queue_num = plat->tx_queues_to_use;
	/*init address bits limit(e.g. 40, 32) and forbidden bits for different driver or product */
	if (!(plat_ex->is_vf || plat_ex->sriov_supported)) {
		plat_ex->addr_bits_limit =
		    addr_limit[DRV_PURE_PF].addr_bits_limit;
		plat_ex->addr_forbid_bits =
		    addr_limit[DRV_PURE_PF].addr_forbid_bits;
	} else if (plat_ex->sriov_supported) {
		plat_ex->addr_bits_limit =
		    addr_limit[DRV_SRIOV_PF].addr_bits_limit;
		plat_ex->addr_forbid_bits =
		    addr_limit[DRV_SRIOV_PF].addr_forbid_bits;
	} else {
		plat_ex->addr_bits_limit = addr_limit[DRV_VF].addr_bits_limit;
		plat_ex->addr_forbid_bits = addr_limit[DRV_VF].addr_forbid_bits;
	}

}

static int dn200_vf_plat_info_set(struct pci_dev *pdev,
				  struct plat_dn200enet_data *plat,
				  struct plat_dn200_data *plat_ex)
{
	struct dn200_vf_info info;

	dn200_get_vf_queue_info(pcim_iomap_table(pdev)[SRIOV_LRAM_BAR_OFF] +
				0x10000, &info, plat_ex->pf_id,
				plat_ex->vf_offset);
	plat_ex->tx_queue_start = info.tx_queue_start;
	plat_ex->rx_queue_start = info.rx_queue_start;
	plat->rx_queues_to_use = info.rx_queues_num;
	plat->tx_queues_to_use = info.tx_queues_num;
	plat_ex->default_rx_queue_num = info.rx_queues_num;
	plat_ex->default_tx_queue_num = info.tx_queues_num;
	plat_ex->max_vfs = info.max_vfs;
	plat_ex->pf.registered_vfs = info.registered_vfs;
	plat_ex->pf.vlan_num_per_vf = info.max_vlan_num;
	plat_ex->max_num_vlan = info.max_vlan_num;
	plat->rss_en = 0;
	return 0;
}

static int dn200_xge_info_set(struct pci_dev *pdev,
			      struct plat_dn200enet_data *plat,
			      struct plat_dn200_data *plat_ex)
{
	int i = 0, j = 0;
	int max_pf;
	int found_pf = 0;	/*0: vf device; 1: pf device */
	int found_vf = 0;	/*1: vf device; 0: pf device */
	int dev_func = plat_ex->funcid;
	int ret = 0;

	for (; i < ARRAY_SIZE(device_map); i++) {
		if (pdev->device == device_map[i].pf_deviceid) {
			max_pf = device_map[i].max_pf;
			for (j = 0; j < max_pf; j++) {
				if ((dev_func & 0xff) ==
				    device_map[i].pci_funcid[j]) {
					found_pf = 1;
					goto found;
				}
			}
		} else if (pdev->device == device_map[i].vf_deviceid) {
			max_pf = device_map[i].max_pf;
			for (j = 0; j < max_pf; j++) {
				if ((dev_func & 0xff) >=
				    device_map[i].min_vf_funcid[j]
				    && (dev_func & 0xff) <=
				    device_map[i].max_vf_funcid[j]) {
					found_vf = 1;
					goto found;
				}
			}
		}
	}

found:
	if (found_pf || found_vf) {
		plat_ex->total_pfs = device_map[i].max_pf;
		plat_ex->pf_id = device_map[i].pci_funcid[j];
		plat_ex->xpcs_index = device_map[i].xpcs_index[j];
		plat_ex->nvme_supported = device_map[i].nvme_supported;
		plat_ex->raid_supported = device_map[i].raid_supported;
		plat_ex->pf_max_iatu = device_map[i].pf_max_iatu;
		plat_ex->vf_total_iatu = device_map[i].vf_total_iatu;
		plat_ex->upgrade_with_flowing = device_map[i].upgrade_with_flowing;
		if (found_pf) {
			plat_ex->vf_offset = 0;
			plat_ex->is_vf = 0;
			plat_ex->sriov_supported =
			    device_map[i].sriov_supported;
			if (device_map[i].max_pf == 4) {
				ret = dn200_xgmac_gpio_data_set(pdev, plat_ex,
						gpio_data_prod_type[0][0], 0);
			} else {
				if (plat_ex->hw_pcb_ver_type >
				    DN200_2P_MAX_PCB_ID
				    || plat_ex->hw_pcb_ver_type <
				    DN200_2P_MIN_PCB_ID) {
					dev_err(&pdev->dev,
						"funid %x deviceid %x : Invalid hw_pcb_ver_type\n",
						dev_func, pdev->device);
					return -EINVAL;
				}
				ret = dn200_xgmac_gpio_data_set(pdev, plat_ex,
								gpio_data_prod_type[1][dn200_pcb_id_map2_gpio_type[plat_ex->hw_pcb_ver_type]],
								plat_ex->pf_id);
			}
			if (ret)
				return -ENOMEM;
		} else if (found_vf) {
			plat_ex->vf_offset =
			    (dev_func & 0xff) - device_map[i].min_vf_funcid[j];
			plat_ex->is_vf = 1;
			plat_ex->sriov_supported = 0;
			dn200_vf_plat_info_set(pdev, plat, plat_ex);
		}
		dn200_xgmac_private_data_set(pdev, plat, plat_ex,
					     &xge_private_data_table[j],
					     device_map[i].link_config);
	} else {
		dev_err(&pdev->dev, "Invalid funid %x deviceid %x\n", dev_func,
			pdev->device);
		return -EINVAL;
	}
	return 0;
}

static int dn200_default_data(struct pci_dev *pdev,
			      struct plat_dn200enet_data *plat,
			      struct plat_dn200_data *plat_ex)
{
	int i;
	int prio_cnt_per_queue, prio;

	plat->has_xgmac = 1;
	plat->has_gmac = 0;
	plat->has_gmac4 = 0;
	plat->force_sf_dma_mode = 1;

	/* Set default value for multicast hash bins */
	plat->multicast_filter_bins = HASH_TABLE_SIZE;

	/* Set default value for unicast filter entries */
	plat->unicast_filter_entries = 8;	// 1-31

	/* Set the maxmtu to a default of JUMBO_LEN */
	plat->maxmtu = JUMBO_LEN;

	/* pbl can be 1, 4, 8, 16, 32, when rxpbl assign to 2, the rx fifo overflow almost disapper
	 * as pblx8 is true, so the rx dma transfer data length is:
	 * 2 * 8 * 16(dma beats size is 16) = 256Btyes
	 * notes: must assign "rxpbl as 2" and "receive store & forward as 0",
	 * the overflow issue almost resolve and must assign "txpbl as 4",
	 * as the pcie interface max limit is 512(4 * 8 * 16) bytes
	 */
	plat->dma_cfg->pbl = 16;
	plat->dma_cfg->aal = false;
	plat->dma_cfg->onekbbe = false;
	plat->dma_cfg->txpbl = 4;
	/* to solve kunpen's 2 port iperf at the same,nedd change rxpbl from 2 to 4 */
	plat->dma_cfg->rxpbl = 4;
	plat->dma_cfg->pblx8 = true;
	plat->tso_en = true;
	/* To save memory, so set split head feature as disabled */
	plat->sph_disable = true;
	plat->rss_en = 1;

	if (plat_ex->is_vf) {
		dn200_vf_plat_info_set(pdev, plat, plat_ex);
		if (!plat->tx_queues_to_use || plat->tx_queues_to_use > 8)
			return -EIO;
	}

	/* MTL Configuration */
	plat->tx_sched_algorithm = MTL_TX_ALGORITHM_WRR;
	for (i = 0; i < plat->tx_queues_to_use; i++) {
		plat->tx_queues_cfg[i].prio = BIT(i);
		plat->tx_queues_cfg[i].use_prio = true;
		plat->tx_queues_cfg[i].mode_to_use = MTL_QUEUE_DCB;
		plat->tx_queues_cfg[i].weight = 10 + i * 5;
	}

	plat->rx_sched_algorithm = MTL_RX_ALGORITHM_WSP;

	prio_cnt_per_queue = 8 / plat_ex->rx_used_mtl_queues;
	for (i = 0; i < plat_ex->rx_used_mtl_queues; i++) {
		/* e.g. use four mtl queues, every queue map two priorities,
		 * queue 0 map to prio 0 & 1, queue 1 map to prio 2 & 3,
		 * queue 2 map to prio 4 & 5, queue 3 map to prio 6 & 7,
		 * 1. L2 tag packets will route to mtl queue 0 ~ 3:
		 *    prio 0 & 1 route to queue 0, prio 2 & 3 route to queue 1,
		 *    prio 4 & 5 route to queue 2, prio 6 & 7 route to queue 3.
		 * 2. untag packets will route to default queue (e.g. queue 0)
		 */
		for (prio = i * prio_cnt_per_queue;
		     prio < (i + 1) * prio_cnt_per_queue; prio++) {
			plat->rx_queues_cfg[i].prio |= BIT(prio);	/* one bit per prioity */
		}
		plat->rx_queues_cfg[i].use_prio = true;
		plat->rx_queues_cfg[i].mode_to_use = MTL_QUEUE_DCB;
		plat->rx_queues_cfg[i].pkt_route = 0x0;
		plat->rx_queues_cfg[i].chan = i;
		/* for 4 mtl queues, weights are 1, 3, 5, 7, maximum rx weight is 7 */
		plat->rx_queues_cfg[i].weight = 2 * i + 1;
	}

	/* AXI Configuration */
	plat->axi = devm_kzalloc(&pdev->dev, sizeof(*plat->axi), GFP_KERNEL);
	if (!plat->axi)
		return -ENOMEM;

	plat->axi->axi_wr_osr_lmt = 0xf;
	plat->axi->axi_rd_osr_lmt = 0x1f;

	plat->axi->axi_fb = false;
	plat->axi->axi_blen[0] = 4;
	plat->axi->axi_blen[1] = 8;
	plat->axi->axi_blen[2] = 16;
	plat->axi->axi_blen[3] = 32;

	/* safety configuration */
	plat->safety_feat_cfg->tsoee = 1;
	plat->safety_feat_cfg->mrxpee = 1;
	plat->safety_feat_cfg->mestee = 1;
	plat->safety_feat_cfg->mrxee = 1;
	plat->safety_feat_cfg->mtxee = 1;
	plat->safety_feat_cfg->epsi = 1;
	plat->safety_feat_cfg->edpp = 1;
	plat->safety_feat_cfg->prtyen = 1;
	plat->safety_feat_cfg->tmouten = 1;

	if (plat_ex->sriov_supported)
		plat_ex->max_num_vlan = 5;
	else if (plat_ex->is_vf)
		plat_ex->max_num_vlan = 0;
	else
		plat_ex->max_num_vlan = 4094;

	return 0;
}

static const struct dn200_pci_info dn200_pci_info = {
	.setup = dn200_default_data,
};

static bool dn200_is_queue_input_supported(struct pci_dev *pdev,
					   struct plat_dn200_data *plat_ex)
{
	if (plat_ex->funcid == 0 || plat_ex->funcid == 1) {
		if (queue_max_set < 1) {
			queue_max_set = 1;
			dev_err(&pdev->dev,
				"PF0 and PF1 queue set < 1!,change queue_max_set form illegal value to 1\n");
			return 0;
		} else if (queue_max_set > DN200_CH_MAX) {
			queue_max_set = 8;
			dev_err(&pdev->dev,
				"PF0 and PF1 queue set exceed 8, change queue_max_set form iilegal value to 8!\n");
			return 0;
		}
	}
	return true;
}

static int dn200_config_multi_msix(struct pci_dev *pdev,
				   struct plat_dn200enet_data *plat,
				   struct plat_dn200_data *plat_ex,
				   struct dn200_resources *res, bool is_purepf)
{
	int i, j, ret;
	struct dn200_ctrl_resource *ctrl = &plat_ex->ctrl;

	ret =
	    dn200_ena_msix_range(pdev, &plat_ex->ctrl, plat_ex->default_tx_queue_num,
				 plat_ex->default_rx_queue_num, is_purepf);
	if (ret) {
		dev_err(&pdev->dev,
			"func %s, line %d:  dn200_ena_msix_range fail %d.\n",
			__func__, __LINE__, ret);
		return ret;
	}
	ret =
	    irq_queue_map(pdev, &plat_ex->ctrl, plat_ex->default_tx_queue_num,
			  plat_ex->default_rx_queue_num, true);
	if (ret) {
		dev_err(&pdev->dev,
			"func %s, line %d:  irq_queue_map fail %d.\n", __func__,
			__LINE__, ret);
		goto err_irq_config;
	}
	/* For TX MSIX */
	for (i = 0; i < plat_ex->default_tx_queue_num; i++)
		res->tx_irq[i] = plat_ex->ctrl.msix_entries[i + 1].vector;

	/* For RX MSIX */
	for (j = 0; j < plat_ex->default_rx_queue_num; j++)
		res->rx_irq[j] = plat_ex->ctrl.msix_entries[j + 1 + i].vector;

	if (plat_ex->sriov_supported ||
		plat_ex->pdev->device == DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF) {
		res->lpi_irq = plat_ex->ctrl.msix_entries[j + 1 + i + 0].vector;
		res->irq = plat_ex->ctrl.msix_entries[j + 1 + i + 1].vector;
		res->sfty_ce_irq =
		    plat_ex->ctrl.msix_entries[j + 1 + i + 2].vector;
		res->sfty_ue_irq =
		    plat_ex->ctrl.msix_entries[j + 1 + i + 3].vector;
	}
	plat->multi_msi_en = 1;
	return 0;
err_irq_config:
	devm_free_irq(ctrl->dev,
		      ctrl->msix_entries[plat_ex->total_irq - 2].vector,
		      plat_ex);
	devm_free_irq(ctrl->dev,
		      ctrl->msix_entries[plat_ex->total_irq - 1].vector,
		      plat_ex);
	devm_free_irq(ctrl->dev, ctrl->msix_entries[0].vector, ctrl);
	pci_disable_msix(pdev);
	return ret;
}

#define DN200_IRQ(num0, num1, num2, num3, num4) \
	((num0) | ((num1) << 6) | ((num2) << 12) | ((num3) << 18) | ((num4) << 24))

static int dn200_config_multi_msi(struct pci_dev *pdev,
				  struct plat_dn200enet_data *plat,
				  struct plat_dn200_data *plat_ex,
				  struct dn200_resources *res,
				  bool support_msix)
{
#define MAX_RX_VECT_NUMB 16
#define MAX_TX_VECT_NUMB 16
	int ret;
	int i;
	int msi_num;
	int intr_msi_mod = 0, intr_src_mask = 0;
	int rx_v[MAX_RX_VECT_NUMB];
	int tx_v[MAX_TX_VECT_NUMB];

	memset(rx_v, 0, sizeof(rx_v));
	memset(tx_v, 0, sizeof(tx_v));

	plat->msi_rx_base_vec = 1;
	for (i = 0; i < plat->rx_queues_to_use; ++i)
		rx_v[i] = plat->msi_rx_base_vec + i;

	plat->msi_tx_base_vec = plat->msi_rx_base_vec + plat->rx_queues_to_use;
	for (i = 0; i < plat->tx_queues_to_use; ++i)
		tx_v[i] = plat->msi_tx_base_vec + i;

	plat->msi_sfty_ce_vec = plat->msi_tx_base_vec + plat->tx_queues_to_use;
	plat->msi_sfty_ue_vec = plat->msi_sfty_ce_vec + 1;
	plat->msi_mac_vec = plat->msi_sfty_ue_vec + 1;
	plat->msi_lpi_vec = plat->msi_mac_vec + 1;
	msi_num = plat->msi_lpi_vec;
	ret =
	    pci_alloc_irq_vectors(pdev, roundup_pow_of_two(msi_num + 1),
				  roundup_pow_of_two(msi_num + 1), PCI_IRQ_MSI);
	if (ret < 0) {
		if (!support_msix)
			dev_err(&pdev->dev,
				"func = %s, line = %d: multi vectors alloc fail! %d\n",
				__func__, __LINE__, ret);
		return ret;
	}
	dev_info(&pdev->dev, "Succeed to allocate %d MSI vectors\n", ret);
	switch (plat->bus_id) {
	case 1:
	case 2:
		/*remap each interrupt source to MSI vector:
		 *  Rx INT source 1~8  (RX DMA Channel 1~16 INT) to vector 1 ~ RX_QUEUES
		 *  Tx INT source 17~24(TX DMA Channel 1~16 INT) to
		 *  vector (RX_QUEUES + 1) ~ (RX_QUEUES + TX_QUEUES)
		 *  Sfty RE INT source 33 to vector msi_sfty_ce_vec
		 *  Sfty UE INT source 34 to vector msi_sfty_ue_vec
		 *  PMT INT source 35 to vector msi_mac_vec
		 *  LPI INT source 36 to vector msi_lpi_vec
		 *  other INT source 37 to vector 0 => of no use and mask
		 */
		writel(DN200_IRQ(0, rx_v[0], rx_v[1], rx_v[2], rx_v[3]),
		       res->addr + XGE_MSI_INTR_SRCMAP0_4(plat->bus_id - 1));
		writel(DN200_IRQ(rx_v[4], rx_v[5], rx_v[6], rx_v[7], rx_v[8]),
		       res->addr + XGE_MSI_INTR_SRCMAP5_9(plat->bus_id - 1));
		writel(DN200_IRQ
		       (rx_v[9], rx_v[10], rx_v[11], rx_v[12], rx_v[13]),
		       res->addr + XGE_MSI_INTR_SRCMAP10_14(plat->bus_id - 1));
		writel(DN200_IRQ(rx_v[14], rx_v[15], tx_v[0], tx_v[1], tx_v[2]),
		       res->addr + XGE_MSI_INTR_SRCMAP15_19(plat->bus_id - 1));
		writel(DN200_IRQ(tx_v[3], tx_v[4], tx_v[5], tx_v[6], tx_v[7]),
		       res->addr + XGE_MSI_INTR_SRCMAP20_24(plat->bus_id - 1));
		writel(DN200_IRQ
		       (tx_v[8], tx_v[9], tx_v[10], tx_v[11], tx_v[12]),
		       res->addr + XGE_MSI_INTR_SRCMAP25_29(plat->bus_id - 1));
		writel(DN200_IRQ
		       (tx_v[13], tx_v[14], tx_v[15], plat->msi_sfty_ce_vec,
			plat->msi_sfty_ue_vec),
		       res->addr + XGE_MSI_INTR_SRCMAP30_34(plat->bus_id - 1));
		writel(DN200_IRQ
		       (plat->msi_mac_vec, plat->msi_lpi_vec, 0x0, 0x0, 0x0),
		       res->addr + XGE_MSI_INTR_SRCMAP35_39(plat->bus_id - 1));
		writel(DN200_IRQ(0, 0, plat_ex->msi_xpcs_vec, 0, 0),
		       res->addr + XGE_MSI_INTR_SRCMAP40_44(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP45_49(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP50_54(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP55_59(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP60_64(plat->bus_id - 1));

		/* 1. Enable xge interrupt source work at msi mode, 0: not msi mode; 1: msi mode
		 * 2. Config interrupt vector mask for or msi, 0: use or no mask; 1: no use or mask
		 * e.g.
		 * 1-8, 17-28 msi mode enable:
		 * writel(0x01FE01FE, res->addr + XGE_MSI_INTR_EN_LOW(plat->bus_id - 1));
		 * 33-36 msi mode enable:
		 * writel(0x0000001E, res->addr + XGE_MSI_INTR_EN_HIGH(plat->bus_id - 1));
		 * 1-8, 17-24 no mask，vector 0 no use and mask:
		 * writel(0xFE01FE01, res->addr + XGE_MSI_INTR_MASK_LOW(plat->bus_id - 1));
		 * 33-36 no mask:
		 * writel(0xFFFFFFE1, res->addr + XGE_MSI_INTR_MASK_HIGH(plat->bus_id - 1));
		 */
		intr_src_mask = 0xFFFFFFFF;
		for (i = 0; i < MAX_RX_VECT_NUMB; ++i) {
			if (rx_v[i]) {
				intr_msi_mod |= 1 << (i + 1);
				intr_src_mask &= ~(1 << (i + 1));
			}
		}
		for (i = 0; i < MAX_TX_VECT_NUMB - 1; ++i) {
			if (tx_v[i]) {
				intr_msi_mod |= 1 << (MAX_RX_VECT_NUMB + i + 1);
				intr_src_mask &=
				    ~(1 << (MAX_RX_VECT_NUMB + i + 1));
			}
		}
		writel(intr_msi_mod,
		       res->addr + XGE_MSI_INTR_EN_LOW(plat->bus_id - 1));
		writel(intr_src_mask,
		       res->addr + XGE_MSI_INTR_MASK_LOW(plat->bus_id - 1));
		if (plat_ex->msi_xpcs_vec) {
			intr_msi_mod = 0x0000041E;
			intr_src_mask = 0xFFFFFBE1;
		} else {
			intr_msi_mod = 0x0000001E;
			intr_src_mask = 0xFFFFFFE1;
		}

		if (tx_v[MAX_TX_VECT_NUMB - 1]) {
			intr_msi_mod |= 0x01;
			intr_src_mask &= ~0x01;
		}
		writel(intr_msi_mod,
		       res->addr + XGE_MSI_INTR_EN_HIGH(plat->bus_id - 1));
		writel(intr_src_mask,
		       res->addr + XGE_MSI_INTR_MASK_HIGH(plat->bus_id - 1));
		break;
	case 3:
	case 4:
		/*remap each interrupt source to MSI vector:
		 * Rx INT source 1~2(RX DMA Channel 1~2 INT) to vector 1 ~ RX_QUEUES
		 * Tx INT source 3~4(TX DMA Channel 1~2 INT) to
		 * vector (RX_QUEUES + 1) ~ (RX_QUEUES + TX_QUEUES)
		 * Sfty RE INT source 5 to vector msi_sfty_ce_vec
		 * Sfty UE INT source 6 to vector msi_sfty_ue_vec
		 * PMT INT source 7 to vector msi_mac_vec
		 * LPI INT source 8 to vector msi_lpi_vec
		 * other INT source to vector 0 => of no use and mask
		 */
		writel(DN200_IRQ(0, rx_v[0], rx_v[1], tx_v[0], tx_v[1]),
		       res->addr + XGE_MSI_INTR_SRCMAP0_4(plat->bus_id - 1));
		writel(DN200_IRQ
		       (plat->msi_sfty_ce_vec, plat->msi_sfty_ue_vec,
			plat->msi_mac_vec, plat->msi_lpi_vec, 0x0),
		       res->addr + XGE_MSI_INTR_SRCMAP5_9(plat->bus_id - 1));
		writel(DN200_IRQ(0, 0, 0, 0, plat_ex->msi_xpcs_vec),
		       res->addr + XGE_MSI_INTR_SRCMAP10_14(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP15_19(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP20_24(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP25_29(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP30_34(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP35_39(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP40_44(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP45_49(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP50_54(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP55_59(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP60_64(plat->bus_id - 1));

		// platform interrupt source mask
		if (plat_ex->msi_xpcs_vec) {
			writel(0x000041FE,
			       res->addr + XGE_MSI_INTR_EN_LOW(plat->bus_id -
							       1));
			writel(0xFFFFBE01,
			       res->addr + XGE_MSI_INTR_MASK_LOW(plat->bus_id -
								 1));
		} else {
			writel(0x000001FE,
			       res->addr + XGE_MSI_INTR_EN_LOW(plat->bus_id -
							       1));
			writel(0xFFFFFE01,
			       res->addr + XGE_MSI_INTR_MASK_LOW(plat->bus_id -
								 1));
		}
		writel(0x00000000,
		       res->addr + XGE_MSI_INTR_EN_HIGH(plat->bus_id - 1));
		writel(0xFFFFFFFF,
		       res->addr + XGE_MSI_INTR_MASK_HIGH(plat->bus_id - 1));
		break;
	default:
		pci_free_irq_vectors(pdev);
		dev_info(&pdev->dev,
			 "func = %s, line = %d:plat->bus_id = %d,Failed to enable multi MSI retval.\n",
			 __func__, __LINE__, plat->bus_id);
		return DN200_FAILURE;
	}
	/* For Rx INT */
	for (i = 0; i < plat->rx_queues_to_use; i++) {
		res->rx_irq[i] =
		    pci_irq_vector(pdev, plat->msi_rx_base_vec + i);
	}
	/* For Tx INT */
	for (i = 0; i < plat->tx_queues_to_use; i++) {
		res->tx_irq[i] =
		    pci_irq_vector(pdev, plat->msi_tx_base_vec + i);
	}
	/* For PMT INT */
	if (plat->msi_mac_vec < DN200_MSI_VEC_MAX)
		res->irq = pci_irq_vector(pdev, plat->msi_mac_vec);
	/* For LPI INT */
	if (plat->msi_lpi_vec < DN200_MSI_VEC_MAX)
		res->lpi_irq = pci_irq_vector(pdev, plat->msi_lpi_vec);
	/* For CE Safety INT */
	if (plat->msi_sfty_ce_vec < DN200_MSI_VEC_MAX)
		res->sfty_ce_irq = pci_irq_vector(pdev, plat->msi_sfty_ce_vec);
	/* For UE Safety INT */
	if (plat->msi_sfty_ue_vec < DN200_MSI_VEC_MAX)
		res->sfty_ue_irq = pci_irq_vector(pdev, plat->msi_sfty_ue_vec);

	/* For XPCS LINK INT */
	if (plat_ex->msi_xpcs_vec &&
	    plat_ex->msi_xpcs_vec < DN200_MSI_VEC_MAX)
		res->xpcs_vec = pci_irq_vector(pdev, plat_ex->msi_xpcs_vec);

	plat->multi_msi_en = 1;
	return 0;
}

static int dn200_config_single_msi(struct pci_dev *pdev,
				   struct plat_dn200enet_data *plat,
				   struct dn200_resources *res)
{
	int ret = 0;

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI);
	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to enable single IRQ\n");
		return ret;
	}

	if (pdev->msix_enabled) {
		/* platform MSIX enable vector 0 */
		writel(0x1, res->addr + XGE_MSIX_INTR_EN_LOW(plat->bus_id - 1));
		writel(0x0,
		       res->addr + XGE_MSIX_INTR_EN_HIGH(plat->bus_id - 1));
		writel(0xFFFFFFFE,
		       res->addr + XGE_MSIX_INTR_MASK_LOW(plat->bus_id - 1));
		writel(0xFFFFFFFF,
		       res->addr + XGE_MSIX_INTR_MASK_HIGH(plat->bus_id - 1));

		dev_info(&pdev->dev, "Succeed to enable MSI-X single IRQ\n");
	} else if (pdev->msi_enabled) {
		/* map each interrupt source to MSI vector 0 */
		writel(0, res->addr + XGE_MSI_INTR_SRCMAP0_4(plat->bus_id - 1));
		writel(0, res->addr + XGE_MSI_INTR_SRCMAP5_9(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP10_14(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP15_19(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP20_24(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP25_29(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP30_34(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP35_39(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP40_44(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP45_49(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP50_54(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP55_59(plat->bus_id - 1));
		writel(0,
		       res->addr + XGE_MSI_INTR_SRCMAP60_64(plat->bus_id - 1));
		/* enable and unmask int0 */
		writel(0x1, res->addr + XGE_MSI_INTR_EN_LOW(plat->bus_id - 1));
		writel(0, res->addr + XGE_MSI_INTR_EN_HIGH(plat->bus_id - 1));
		writel(0xFFFFFFFE,
		       res->addr + XGE_MSI_INTR_MASK_LOW(plat->bus_id - 1));
		writel(0xFFFFFFFF,
		       res->addr + XGE_MSI_INTR_MASK_HIGH(plat->bus_id - 1));

		dev_info(&pdev->dev, "Succeed to enable MSI single IRQ\n");
	} else {
		/* Legacy Pin interrupt, do nothing */
		dev_info(&pdev->dev, "Succeed to enable Legacy single IRQ\n");
	}

	res->irq = pci_irq_vector(pdev, 0);
	plat->multi_msi_en = 0;
	return 0;
}

int dn200_config_interrupt(struct pci_dev *pdev,
				  struct plat_dn200enet_data *plat,
				  struct plat_dn200_data *plat_ex,
				  struct dn200_resources *res, bool is_nvme_pf)
{
	int ret = 0;

	if (!plat_ex->is_vf && !plat_ex->sriov_supported && !is_nvme_pf) {
		plat_ex->use_msi = true;
		ret =
		    dn200_config_multi_msi(pdev, plat, plat_ex, res,
					   plat_ex->nvme_supported);
		if (ret && plat_ex->nvme_supported) {
			ret =
			    dn200_config_multi_msix(pdev, plat, plat_ex, res,
						    is_nvme_pf);
			plat_ex->use_msi = false;
		} else if (ret) {
			ret = dn200_config_single_msi(pdev, plat, res);
			if (ret) {
				dev_err(&pdev->dev,
					"%s: ERROR, failed to enable single IRQ\n",
					__func__);
				return ret;
			}
		}
	} else {
		ret =
		    dn200_config_multi_msix(pdev, plat, plat_ex, res,
					    is_nvme_pf);
		plat_ex->use_msi = false;
	}
	if (plat_ex->use_msi) {
		ret = dn200_ctrl_ccena(pdev, 1, 1, false);
		if (ret) {
			dev_err(&pdev->dev,
				"func %s, line %d: ctrl cc enable timeout\n",
				__func__, __LINE__);
		}
	}
	return ret;
}

#define PURE_PF_NO_NVME 1
#define PURE_PF_NVME 2
static u8 dn200_pure_pf_type(struct pci_dev *pdev)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(PURE_PF_DEVICE); i++) {
		if (pdev->device == PURE_PF_DEVICE[i])
			return PURE_PF_NO_NVME;
	}
	for (i = 0; i < ARRAY_SIZE(NVME_PURE_PF_DEVICE); i++) {
		if (pdev->device == NVME_PURE_PF_DEVICE[i])
			return PURE_PF_NVME;
	}
	return 0;
}

static bool dn200_is_sriov_vf(struct pci_dev *pdev)
{
	int i = 0;

	for (; i < ARRAY_SIZE(SRIOV_VF_DEVICE); i++) {
		if (pdev->device == SRIOV_VF_DEVICE[i])
			return true;
	}
	return false;
}

static bool dn200_is_extern_phy(struct pci_dev *pdev)
{
	int i = 0;

	for (; i < ARRAY_SIZE(EXTERN_PHY_DEVICE); i++) {
		if (pdev->device == EXTERN_PHY_DEVICE[i])
			return true;
	}
	return false;
}

static bool dn200_is_4_port(struct pci_dev *pdev)
{
	int i = 0;

	for (; i < ARRAY_SIZE(SRIOV_4P_DEVICE); i++) {
		if (pdev->device == SRIOV_4P_DEVICE[i])
			return true;
	}
	return false;
}

static u8 dn200_gpio_read(struct pci_dev *pdev, struct dn200_resources *res,
			  u8 offset)
{
	u32 value = 0;
	u8 offset_pin = 0;
	u8 val = 0;
	u32 gpio_offset = 0;
	u32 reg_off_read = 0;

	if (dn200_is_4_port(pdev)) {
		gpio_offset += DN200_SFPCTRL_MODE0_BAROFF;
		reg_off_read = 8;
	} else {
		gpio_offset += DN200_SFPCTRL_MODE1_BAROFF;
		reg_off_read = 0;
	}

	/*may be offset is greater than 31 */
	offset_pin = (offset >> 5) << 2;
	offset = offset & 0x1f;
	value = ioread32(res->addr + gpio_offset + offset_pin + reg_off_read);
	val = (value >> (offset)) & 0x1;

	return val;
}

#define DN200_PCB_FW_GPIO_PIN0 49
#define DN200_PCB_FW_GPIO_PIN1 38
#define DN200_PCB_FW_GPIO_PIN2 29
static int dn200_hw_pcb_version_from_gpio(struct pci_dev *pdev,
					  struct plat_dn200_data *plat_ex,
					  struct dn200_resources *res,
					  bool is_no_nvme)
{
	int ret = 0;
	u8 val1;
	u32 val;
	/*vf not support gpio */
	if (dn200_is_sriov_vf(pdev))
		return ret;

	if (!is_no_nvme) {
		ret = get_pcb_type(&plat_ex->ctrl, &val);
		plat_ex->hw_pcb_ver_type = dn200_pcb_type[val].value;
		return ret;
	}

	/*vf not support gpio */
	if (dn200_is_sriov_vf(pdev))
		return ret;
	/*gpio 49 for ver */
	val1 = (dn200_gpio_read(pdev, res, DN200_PCB_FW_GPIO_PIN0) << 2);
	/*gpio 29 for ver */
	val1 |= (dn200_gpio_read(pdev, res, DN200_PCB_FW_GPIO_PIN1) << 1);
	/*gpio 38 for ver */
	val1 |= dn200_gpio_read(pdev, res, DN200_PCB_FW_GPIO_PIN2);

	plat_ex->hw_pcb_ver_type = dn200_pcb_type[val1].value;
	return ret;

}

/**
 * dn200_pci_probe
 *
 * @pdev: pci device pointer
 * @id: pointer to table of device id/id's.
 *
 * Description: This probing function gets called for all PCI devices which
 * match the ID table and are not "owned" by other driver yet. This function
 * gets passed a "struct pci_dev *" for each device whose entry in the ID table
 * matches the device. The probe functions returns zero when the driver choose
 * to take "ownership" of the device or an error code(-ve no) otherwise.
 */
static int dn200_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct dn200_pci_info *info = (struct dn200_pci_info *)id->driver_data;
	struct plat_dn200enet_data *plat = NULL;
	struct plat_dn200_data *plat_ex = NULL;
	struct dn200_resources res;
	int i;
	int ret;
	u8 pf_type = dn200_pure_pf_type(pdev);

	plat = devm_kzalloc(&pdev->dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		return -ENOMEM;

	plat_ex = devm_kzalloc(&pdev->dev, sizeof(*plat_ex), GFP_KERNEL);
	if (!plat)
		return -ENOMEM;

	plat->mdio_bus_data = devm_kzalloc(&pdev->dev,
					   sizeof(*plat->mdio_bus_data),
					   GFP_KERNEL);
	if (!plat->mdio_bus_data)
		return -ENOMEM;

	plat->dma_cfg = devm_kzalloc(&pdev->dev, sizeof(*plat->dma_cfg),
				     GFP_KERNEL);
	if (!plat->dma_cfg)
		return -ENOMEM;

	plat->safety_feat_cfg = devm_kzalloc(&pdev->dev,
					     sizeof(*plat->safety_feat_cfg),
					     GFP_KERNEL);
	if (!plat->safety_feat_cfg)
		return -ENOMEM;
	/* Enable pci device */
	ret = pci_enable_device(pdev);
	if (ret) {
		dev_err(&pdev->dev, "%s: ERROR: failed to enable device\n",
			__func__);
		return ret;
	}
	if (!dn200_is_sriov_vf(pdev))
		pci_aer_clear_nonfatal_status(pdev);
	/* set up for high or low dma */
	if (!dn200_is_sriov_vf(pdev)) {
		plat->addr64 = 64;
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
		if (ret) {
			ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
			if (ret) {
				dev_err(&pdev->dev,
					"DMA configuration failed: 0x%x\n", ret);
				goto err_dma;
			}
			plat->addr64 = 32;
		}
	} else {
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (ret) {
			dev_err(&pdev->dev,
				"DMA configuration failed: 0x%x\n", ret);
			goto err_dma;
		}
		plat->addr64 = 32;
	}
	plat_ex->ctrl.addr64 = plat->addr64;
	/* Get the base address of device */
	for (i = 0; i < DN200_PCI_BAR_NUM; i++) {
		if (pci_resource_len(pdev, i) == 0)
			continue;

		ret = pcim_iomap_regions(pdev, BIT(i), pci_name(pdev));
		if (ret)
			goto err_iomap;
	}
	pci_set_master(pdev);
	memset(&res, 0, sizeof(res));

	if (pci_resource_len(pdev, 0) != 0)
		res.ctrl_addr = pcim_iomap_table(pdev)[0];

	if (pci_resource_len(pdev, 2) != 0) {
		res.addr = pcim_iomap_table(pdev)[2];
		plat_ex->io_addr = res.addr;
	}
	if (pci_resource_len(pdev, 4) != 0)
		res.mail = pcim_iomap_table(pdev)[4] + 0x10000;

	if (!dn200_hwif_id_check(res.addr)) {
		dev_err(&pdev->dev, "func %s: %s\n", __func__,
			DN200_PCIE_BAR_ERR);
		goto err_ctrl_cc;
	}

	if (!(pf_type == PURE_PF_NO_NVME || (dn200_is_sriov_vf(pdev)))) {
		ret = dn200_ctrl_ccena(pdev, 0, 1, false);
		if (ret) {
			dev_err(&pdev->dev,
				"func %s, line %d: ctrl cc enable timeout\n",
				__func__, __LINE__);
			goto err_ctrl_cc;
		}
		set_bit(ADMIN_QUEUE_INITED, &plat_ex->ctrl.admin_state);
	}

	plat_ex->pdev = pdev;

	if (!(pf_type == PURE_PF_NO_NVME)) {
		ret =
		    admin_queue_configure(pdev, &plat_ex->ctrl,
					  pf_type == PURE_PF_NVME, dn200_is_extern_phy(pdev));
		if (ret)
			goto err_admin_queue;
	} else {
		plat_ex->funcid = pdev->devfn;
	}
	ret = irq_info_pfvf_release(pdev, &plat_ex->ctrl, true);
	if (ret) {
		dev_err(&pdev->dev,
			"func %s, line %d: vf_release fail\n",
			__func__, __LINE__);
		goto err_admin_queue;
	}
	ret = dn200_is_queue_input_supported(pdev, plat_ex);
	if (!ret) {
		dev_err(&pdev->dev,
			"func %s, line %d: PF0 and PF1 queue set is illegal\n",
			__func__, __LINE__);
		goto err_admin_queue;
	}
	ret = dn200_hw_pcb_version_from_gpio(pdev, plat_ex, &res,
					   pf_type == PURE_PF_NO_NVME);
	if (ret)
		goto err_hw_get;

	ret = dn200_xge_info_set(pdev, plat, plat_ex);
	if (ret)
		goto err_info_set;
	ret = info->setup(pdev, plat, plat_ex);
	if (ret) {
		dev_err(&pdev->dev, "func %s, line %d: info setup fail\n",
			__func__, __LINE__);
		goto err_info_set;
	}

	ret =
	    dn200_config_interrupt(pdev, plat, plat_ex, &res,
				   pf_type == PURE_PF_NVME);
	if (ret)
		goto err_alloc_irq;
	if (pf_type == (u8) PURE_PF_NO_NVME)
		plat_ex->speed_cmd = 0;
	else
		plat_ex->speed_cmd = 1;

	if (!plat_ex->is_vf && plat_ex->has_xpcs) {
		ret = dn200_clock_phy_detect(pdev, &plat_ex->ctrl);
		if (!ret)
			goto err_clk_detect;
	}

	ret = dn200_dvr_probe(&pdev->dev, plat, plat_ex, &res);
	if (ret)
		goto err_dvr_probe;
	if (!plat_ex->is_vf)
		dn200_configure_timestamp(&plat_ex->ctrl);
	if (!plat_ex->is_vf)
		dn200_register_nvme_device(&plat_ex->ctrl);
	return 0;

err_dvr_probe:
err_clk_detect:
	if (!plat_ex->use_msi) {
		dn200_ctrl_res_free(pdev, &plat_ex->ctrl);
		pci_disable_msix(pdev);
	} else {
		pci_free_irq_vectors(pdev);
	}
err_alloc_irq:
err_admin_queue:
err_info_set:
err_ctrl_cc:
err_hw_get:
	for (i = 0; i < DN200_PCI_BAR_NUM; i++) {
		if (pci_resource_len(pdev, i) == 0)
			continue;
		pcim_iounmap_regions(pdev, BIT(i));
	}
err_iomap:
err_dma:
	pci_disable_device(pdev);

	return ret;
}

/**
 * dn200_pci_remove
 *
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and releases the PCI resources.
 */
static void dn200_pci_remove(struct pci_dev *pdev)
{
	int i;
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv;
	struct plat_dn200_data *plat_ex;
	bool pcie_avalid = false;

	if (!ndev)
		return;

	priv = netdev_priv(ndev);
	if (!priv)
		return;

	if (test_bit(DN200_PCIE_UNAVAILD, &priv->state) || !dn200_hwif_id_check(priv->ioaddr))
		pcie_avalid = false;
	else
		pcie_avalid = true;

	/*test DN200_RESETING to avoid reset by other subtask */
	while (test_bit(DN200_RESETING, &priv->state))
		usleep_range(1000, 2000);
	set_bit(DN200_IN_REMOVE, &priv->state);
	plat_ex = priv->plat_ex;
	plat_ex->ctrl.pcie_ava = pcie_avalid;
	if (!PRIV_IS_VF(priv))
		dn200_unregister_nvme_device(&plat_ex->ctrl);
	if (pcie_avalid)
		dn200_sriov_disable(priv);
	dn200_dvr_remove(&pdev->dev);
	/* just sriov supported pf or vf need ctrl feature */
	if (!plat_ex->use_msi) {
		dn200_ctrl_res_free(pdev, &plat_ex->ctrl);
		pci_disable_msix(pdev);
	} else {
		pci_free_irq_vectors(pdev);
	}

	for (i = 0; i < DN200_PCI_BAR_NUM; i++) {
		if (pci_resource_len(pdev, i) == 0)
			continue;
		pcim_iounmap_regions(pdev, BIT(i));
	}
	pci_disable_device(pdev);
}

static int __maybe_unused dn200_pci_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;
	struct plat_dn200_data *plat_ex = NULL;
	int ret;

	if (!ndev)
		return -EINVAL;
	priv = netdev_priv(ndev);
	if (!priv)
		return -EINVAL;
	plat_ex = priv->plat_ex;

	/*Already suspended */
	if (test_and_set_bit(DN200_SUSPENDED, &priv->state))
		return 0;

	/* We need to hold the RTNL lock prior to restoring interrupt schemes,
	 * since we're going to be restoring queues
	 */
	set_bit(DN200_SYS_SUSPENDED, &priv->state);
	ret = dn200_suspend(dev);
	if (ret) {
		clear_bit(DN200_SUSPENDED, &priv->state);
		return ret;
	}
	clear_bit(ADMIN_QUEUE_INITED, &plat_ex->ctrl.admin_state);
	return 0;
}

static int __maybe_unused dn200_pci_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;
	int ret = 0;

	if (!ndev)
		return -EINVAL;
	priv = netdev_priv(ndev);
	if (!priv)
		return -EINVAL;

	if (priv->mii)
		msleep(10000);
	if (!test_and_clear_bit(DN200_SUSPENDED, &priv->state))
		return 0;
	schedule_work(&priv->retask);
	return ret;
}

/**
 * dn200_shutdown - PCI callback for shutting down
 * @pdev: PCI device information struct
 **/
static void dn200_shutdown(struct pci_dev *pdev)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;

	/* After fw abnormal, user remove and then insert driver,
	 * although driver probe failure, but it will not be removed
	 * now reboot the system, and will trigger shutdown,
	 * but netdevice is not init, so can't access it
	 */
	if (!ndev)
		return;

	priv = netdev_priv(ndev);
	if (!priv)
		return;

	/*Already suspended */
	if (test_and_set_bit(DN200_SUSPENDED, &priv->state))
		return;

	/* We need to hold the RTNL lock prior to restoring interrupt schemes,
	 * since we're going to be restoring queues
	 */
	dn200_suspend(&pdev->dev);
	if (!priv->plat_ex->use_msi) {
		dn200_ctrl_res_free(pdev, &priv->plat_ex->ctrl);
		pci_disable_msix(pdev);
	} else
		pci_free_irq_vectors(pdev);
}

static SIMPLE_DEV_PM_OPS(dn200_pm_ops, dn200_pci_suspend, dn200_pci_resume);

static const struct pci_device_id dn200_id_table[] = {
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_4P_PURE_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_4P_SRIOV_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_4P_SRIOV_VF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_PURE_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_SRIOV_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_SRIOV_VF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_1G_4P_PURE_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_1G_4P_SRIOV_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_1G_4P_SRIOV_VF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_COPP_1G_4P_PURE_PF), .driver_data =
	 (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_NVME_PUREPF),
	.driver_data = (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF),
	.driver_data = (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF),
	.driver_data = (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_PF),
	.driver_data = (kernel_ulong_t) &dn200_pci_info },
	{ PCI_VDEVICE(DAPUSTOR, DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_VF),
	.driver_data = (kernel_ulong_t) &dn200_pci_info },
	{ }
};

/**
 * dn200_pci_error_detected - warning that something funky happened in PCI land
 * @pdev: PCI device information struct
 * @error: the type of PCI error
 *
 * Called to warn that something happened and the error handling steps
 * are in progress.  Allows the driver to quiesce things, be ready for
 * remediation.
 **/
static pci_ers_result_t dn200_pci_error_detected(struct pci_dev *pdev,
						 pci_channel_state_t error)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;

	dev_info(&pdev->dev, "%s: error %d\n", __func__, error);
	if (!ndev) {
		dev_info(&pdev->dev,
			 "Cannot recover - error happened during device probe\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	priv = netdev_priv(ndev);
	if (!priv)
		return PCI_ERS_RESULT_DISCONNECT;

	if (PRIV_IS_VF(priv))
		return PCI_ERS_RESULT_NO_AER_DRIVER;
	/* shutdown all operations */
	if (!test_and_set_bit(DN200_SUSPENDED, &priv->state))
		dn200_suspend(&pdev->dev);
	/* Request a slot reset */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * dn200_pci_error_slot_reset - a PCI slot reset just happened
 * @pdev: PCI device information struct
 *
 * Called to find if the driver can work with the device now that
 * the pci slot has been reset.  If a basic connection seems good
 * (registers are readable and have sane content) then return a
 * happy little PCI_ERS_RESULT_xxx.
 **/
static pci_ers_result_t dn200_pci_error_slot_reset(struct pci_dev *pdev)
{
	pci_ers_result_t result;
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;
	int err;

	if (!ndev)
		return PCI_ERS_RESULT_DISCONNECT;
	priv = netdev_priv(ndev);
	if (!priv)
		return PCI_ERS_RESULT_DISCONNECT;
	dev_info(&pdev->dev, "%s enter reset\n", __func__);
	if (PRIV_IS_VF(priv))
		return PCI_ERS_RESULT_NO_AER_DRIVER;
	if (pci_enable_device_mem(pdev)) {
		dev_warn(&pdev->dev,
			 "Cannot re-enable PCI device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		pci_set_master(pdev);
		pci_restore_state(pdev);
		pci_save_state(pdev);
		result = PCI_ERS_RESULT_RECOVERED;
	}

	err = pci_aer_clear_nonfatal_status(pdev);
	if (err) {
		dev_info(&pdev->dev,
			 "pci_aer_clear_nonfatal_status() failed, error %d\n",
			 err);
		result = PCI_ERS_RESULT_DISCONNECT;
	}
	return result;
}

/**
 * dn200_pci_error_reset_prepare - prepare device driver for pci reset
 * @pdev: PCI device information struct
 */
static void dn200_pci_error_reset_prepare(struct pci_dev *pdev)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;

	if (!ndev)
		return;
	priv = netdev_priv(ndev);
	if (!priv)
		return;

	if (PRIV_IS_VF(priv))
		return;
	dev_info(&pdev->dev, "%s enter reset prepare\n", __func__);
	/* shutdown all operations */
	if (!test_and_set_bit(DN200_SUSPENDED, &priv->state))
		dn200_suspend(&pdev->dev);
}

/**
 * dn200_pci_error_reset_done - pci reset done, device driver reset can begin
 * @pdev: PCI device information struct
 */
static void dn200_pci_error_reset_done(struct pci_dev *pdev)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = NULL;

	if (!ndev)
		return;
	priv = netdev_priv(ndev);
	if (!priv)
		return;

	if (PRIV_IS_VF(priv))
		return;
	dev_info(&pdev->dev, "%s enter reset done\n", __func__);
	if (!test_and_clear_bit(DN200_SUSPENDED, &priv->state))
		return;
	ctrl_reset(&priv->plat_ex->ctrl, true);
	dn200_resume(&pdev->dev);
}

/**
 * dn200_pci_error_resume - restart operations after PCI error recovery
 * @pdev: PCI device information struct
 *
 * Called to allow the driver to bring things back up after PCI error
 * and/or reset recovery has finished.
 **/
static void dn200_pci_error_resume(struct pci_dev *pdev)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);
	struct dn200_priv *priv = netdev_priv(ndev);

	if (!ndev)
		return;
	priv = netdev_priv(ndev);
	if (!priv)
		return;

	dev_info(&pdev->dev, "%s pci error detect!\n", __func__);
	if (PRIV_IS_VF(priv))
		return;
	if (!test_bit(DN200_SUSPENDED, &priv->state))
		dn200_suspend(&pdev->dev);
	dn200_resume(&pdev->dev);
	clear_bit(DN200_SUSPENDED, &priv->state);
}

static const struct pci_error_handlers dn200_err_handler = {
	.error_detected = dn200_pci_error_detected,
	.slot_reset = dn200_pci_error_slot_reset,
	.reset_prepare = dn200_pci_error_reset_prepare,
	.reset_done = dn200_pci_error_reset_done,
	.resume = dn200_pci_error_resume,
};

MODULE_DEVICE_TABLE(pci, dn200_id_table);

static struct pci_driver dn200_pci_driver = {
	.name = DN200_RESOURCE_NAME,
	.id_table = dn200_id_table,
	.probe = dn200_pci_probe,
	.remove = dn200_pci_remove,
	.driver = {
		   .pm = &dn200_pm_ops,
		    },
	.shutdown = dn200_shutdown,
	.err_handler = &dn200_err_handler,
	.sriov_configure = dn200_sriov_configure,
};

static struct notifier_block dn200_notifier = {
	.notifier_call = dn200_dev_event,
};

/**
 * dn200_module_init - Driver registration routine
 *
 * dn200_module_init is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 */
static int __init dn200_module_init(void)
{
	int status;

	pr_info("%s\n", dn200_driver_str);
	pr_info("%s\n", dn200_copyright);
	status = register_netdevice_notifier(&dn200_notifier);
	if (status) {
		pr_err("failed to register netdevice_notifier, err %d\n",
		       status);
		return status;
	}
	status = pci_register_driver(&dn200_pci_driver);
	if (status)
		pr_err("failed to register PCI driver, err %d\n", status);

	return status;
}

module_init(dn200_module_init);

/**
 * dn200_module_exit - Driver exit cleanup routine
 *
 * dn200_module_exit is called just before the driver is removed
 * from memory.
 */
static void __exit dn200_module_exit(void)
{
	unregister_netdevice_notifier(&dn200_notifier);
	pci_unregister_driver(&dn200_pci_driver);
	pr_info("module unloaded\n");
}

module_exit(dn200_module_exit);
