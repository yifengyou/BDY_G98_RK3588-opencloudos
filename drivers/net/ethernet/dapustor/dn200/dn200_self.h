/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 *
 */
#ifndef __DN200_SELF_H__
#define __DN200_SELF_H__

#include "dn200.h"
#include "dwxgmac_comm.h"
#include "dn200_phy.h"
#include "dn200_sriov.h"
#include "dn200_ctrl.h"

struct pcb_type {
	u8 index;
	u8 value;
};

enum dn200_hw_pcb_gpio_val {
	DN200_HW_PCB_GPIO_VAL_0 = 0b000,
	DN200_HW_PCB_GPIO_VAL_1 = 0b001,
	DN200_HW_PCB_GPIO_VAL_2 = 0b010,
	DN200_HW_PCB_GPIO_VAL_3 = 0b011,
};

enum dn200_hw_pcb_type {
	DN200_HW_PCB_TYPE_0 = 0x0,
	DN200_HW_PCB_TYPE_1 = 0x1,
	DN200_HW_PCB_TYPE_2 = 0x2,
	DN200_HW_PCB_TYPE_3 = 0x3,
};
/* DAPUSOTR vendor and device ID */
#define PCI_VENDOR_ID_DAPUSTOR 0x1E3B
#define DN200_DEV_ID_SFP_10G_2P_PURE_PF 0x3000
#define DN200_DEV_ID_SFP_10G_2P_SRIOV_PF 0x3001
#define DN200_DEV_ID_SFP_10G_2P_SRIOV_VF 0x3002
#define DN200_DEV_ID_SFP_10G_4P_PURE_PF 0x3003
#define DN200_DEV_ID_SFP_10G_4P_SRIOV_PF 0x3004
#define DN200_DEV_ID_SFP_10G_4P_SRIOV_VF 0x3005
#define DN200_DEV_ID_SFP_1G_4P_PURE_PF 0x3006
#define DN200_DEV_ID_SFP_1G_4P_SRIOV_PF 0x3007
#define DN200_DEV_ID_SFP_1G_4P_SRIOV_VF 0x3008
#define DN200_DEV_ID_COPP_1G_4P_PURE_PF 0x3009
#define DN200_DEV_ID_SFP_10G_2P_NVME_PUREPF 0x300A
#define DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF 0x300B
#define DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF 0x300C
#define DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_PF 0x3100
#define DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_VF 0x3101

#define DN200_FAILURE (-1)
#define INVALID_FUNCID (-2)

#define SRIOV_LRAM_BAR_OFF 4

/*dwmac-dn200.c*/
#define DN200_SFPCTRL_MODE0_BAROFF 0x40030
#define DN200_SFPCTRL_MODE1_BAROFF 0x40000
#define XGE_TOP_CONFIG_OFFSET 0x30000
#define DN200_PCIE_BAROFF 0x20000

#define XGE_MSI_CONFIG_OFFSET (XGE_TOP_CONFIG_OFFSET + 0x600)

#define XGE_XGMAC_CLK_MUX_CTRL(x) (0xC + XGE_TOP_CONFIG_OFFSET + (x) * 0x50)
#define XGE_XGMAC_CLK_TX_CTRL(x) (0x10 + XGE_TOP_CONFIG_OFFSET + (x) * 0x50)
#define XGE_XGMAC_CLK_MUX_ENABLE_CTRL(x) (0x1C + XGE_TOP_CONFIG_OFFSET + (x) * 0x50)
#define XGE_XGMAC_XPCS_SW_RST(x) (0x34 + XGE_TOP_CONFIG_OFFSET + (x) * 0x50)

#define XGE_PCIE_PCS_PHY1_TX_RATE_REG(x) (0x2440 + DN200_PCIE_BAROFF - (x) * 0xd0)
#define XGE_PCIE_PCS_PHY1_RX_RATE_REG(x) (0x2130 + DN200_PCIE_BAROFF - (x) * 0x110)

#define XGE_MSI_INTR_MASK_LOW(x) (0x10 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_MASK_HIGH(x) (0x14 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_EN_LOW(x) (0x18 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_EN_HIGH(x) (0x1C + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)

#define XGE_MSIX_INTR_EN_LOW(x) (0x20 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSIX_INTR_EN_HIGH(x) (0x24 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSIX_INTR_MASK_LOW(x) (0x28 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSIX_INTR_MASK_HIGH(x) (0x2C + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)

#define XGE_INTR_INFO_CONFIG(x) (0x30 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)

#define XGE_MSI_INTR_SRCMAP0_4(x) (0x34 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP5_9(x) (0x38 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP10_14(x) (0x3C + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP15_19(x) (0x40 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP20_24(x) (0x44 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP25_29(x) (0x48 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP30_34(x) (0x4C + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP35_39(x) (0x50 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP40_44(x) (0x54 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP45_49(x) (0x58 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP50_54(x) (0x5C + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP55_59(x) (0x60 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)
#define XGE_MSI_INTR_SRCMAP60_64(x) (0x64 + XGE_MSI_CONFIG_OFFSET + (x) * 0x80)

#define PCIE_PHY1_REFA_CLK_SEL_REG  0x17b8
#define PCIE_PHY1_REFB_CLK_SEL_REG  0x17c4
#define PCIE_PHY0_REFA_CLKDET_EN_REG  0x1794
#define PCIE_PHY0_REFB_CLKDET_EN_REG  0x17a0
#define PCIE_PHY1_REFA_CLKDET_EN_REG  0x17bc
#define PCIE_PHY1_REFB_CLKDET_EN_REG  0x17c8
#define PCIE_PHY1_REFA_CLKDET_RESULT_REG  0x17c0
#define PCIE_PHY1_REFB_CLKDET_RESULT_REG  0x17cc
#define PCIE_PHY0_REFA_CLKDET_RESULT_REG  0x1798
#define PCIE_PHY0_REFB_CLKDET_RESULT_REG  0x17A4

#define XPCS_VR_XS_PMA_MP_12G_16G_25G_SRAM 0x6026c
#define XPCA_SR_AN_CTRL 0x1C0000
#define XPCS_REG_BASE 0x2c000000
#define XPCS_REG_OFFSET 0x800000
#define XPCS_12G_16G_25G_TX_GENCTRL0 0x600c0
#define XGE01_QUEUES_TO_USE 8
#define XGE23_QUEUES_TO_USE 2

struct xge_private_data {
	unsigned char phy_addr;
	unsigned char bus_id;
	unsigned char tx_queues_to_use;
	unsigned char rx_queues_to_use;
	unsigned int clk_ptp_rate;
	unsigned int clk_csr;
	unsigned char rx_queues_reserved;
	unsigned char tx_queues_reserved;
	unsigned char rx_queues_total;
	unsigned char tx_queues_total;
	unsigned char rx_used_mtl_queues;
	unsigned char max_vfs;
	phy_interface_t phy_interface;
	int max_speed;
	void __iomem *topaddr;
	void __iomem *xpcsaddr;
};

enum XGE_LINK_MODE {
	EXTERNAL_PHY_1000BASEX,
	EXTERNAL_PHY_SGMII,
	EXTERNAL_PHY_RGMII,
	XPCS_PHY_1000BASEX,
	XPCS_PHY_10GBASER,
	XGE_LINK_MODE_UNKOWN,
	XGE_LINK_MODE_MAX,
};

struct xge_link_config {
	unsigned char has_xpcs;
	unsigned int clk_ptp_rate;
	unsigned int clk_csr;
	phy_interface_t phy_interface;
	int max_speed;
	unsigned int clk_ref_rate;
};

struct dn200_gpio_data {
	u32 gpio_addr_offset;
	int sfp_detect_pin;
	int sfp_tx_disable_pin;
	int sfp_tx_fault_pin;
	int sfp_rx_los_pin;
	int sfp_rs0_pin;
	int sfp_rs1_pin;
	int sfp_led1_pin; // low active
	int sfp_led2_pin; // low active
	u16 reg_off_set_read;
	u16 reg_off_set_write;
};

struct xge_private_data_id {
	unsigned char pf_id;
	const struct xge_private_data compat;
};

struct plat_dn200_data {
	struct dn200_priv *priv_back;
	struct pci_dev *pdev;
	void __iomem *io_addr;
	u8 rx_queues_total;
	u8 tx_queues_total;
	u8 rx_used_mtl_queues; //mtl queues used for rx
	u8 rx_queues_reserved; //rx queues reserved for all VFs per PF
	u8 tx_queues_reserved; //tx queues reserved for all VFs per PF
	u8 rx_queue_start;
	u8 tx_queue_start;
	u8 *pf_max_iatu;
	u8 *vf_total_iatu;
	bool raid_supported; /* true: support raid; flase: not support raid */
	bool use_msi;
	bool is_vf;
	bool sriov_cfg;
	bool vf_flag;
	bool sriov_supported;
	bool nvme_supported;
	bool vf_in_rst;
	u8 max_vfs;
	u8 pf_id;
	u8 total_pfs;
	int xpcs_index;
	u8 vf_offset; /*vf index within a pf */
	u8 funcid;
	bool has_xpcs;
	u8 total_irq;
	u8 speed_cmd;
	u8 vf_loss_hb_cnt[DN200_MAX_VF_NUM + 1];
	struct dn200_ctrl_resource ctrl;
	struct dn200_phy_info *phy_info;
	struct dn200_pf_info pf;
	struct dn200_gpio_data *gpio_data;
	int msi_xpcs_vec;
	u32 bitmap_vlan;
	u16 max_num_vlan; /*Maximum number of vlans that can be configured */
	u16 vlan_num; /*Number of configured vlans */
	u16 vlan_id[64]; /* VF supports a maximum of 48 vlan_id */
	int addr_bits_limit;
	u64 addr_forbid_bits;
	u8 hw_pcb_ver_type;
	int max_speed;
	u8 default_rx_queue_num;
	u8 default_tx_queue_num;
	u8 hw_rj45_type;
	bool upgrade_with_flowing;
};

#define HW_IS_VF(hw) ((hw)->priv->plat_ex->is_vf)
#define PRIV_IS_VF(priv) ((priv)->plat_ex->is_vf)
#define PRIV_IS_PUREPF(priv) \
	(!((priv)->plat_ex->is_vf || (priv)->plat_ex->sriov_supported))
#define HW_IS_PUREPF(hw) \
	(!((hw)->priv->plat_ex->is_vf || (hw)->priv->plat_ex->sriov_supported))
#define DN200_LAST_QUEUE(priv) ((priv)->plat_ex->rx_queues_total - 1)
#define DN200_MTL_QUEUE_IS_VALID(priv, queue)           \
	(((queue) < (priv)->plat_ex->rx_used_mtl_queues) || \
	 (!PRIV_IS_VF(priv) && (queue == DN200_LAST_QUEUE(priv))))
#endif
