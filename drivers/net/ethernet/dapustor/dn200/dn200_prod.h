/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 *
 */

#ifndef __DN200_PROD_H__
#define __DN200_PROD_H__

#define XGE_NUM 4
#define DN200_PORT_TYPE 2
#define DN200_NUM_PCB_VER 8
#define DN200_PORT_NUM 2

/**
 * bit0-tx_disable,		bit1-tx_fault,		bit4-sfp_loss,
 * bit5-sfp_mod_detect,	bit6-sfp_rs0,		bit7-sfp_rs1,
 * bit8-led0 from PWM,	bit9-led1 for 1G,	bit10-led2 for 10G
 */
struct dn200_gpio_data
	gpio_data_prod_type[DN200_PORT_TYPE][DN200_NUM_PCB_VER][DN200_PORT_NUM] = {
	/*4p is the same */
	[0] = {
			[0] = {
				[0] = {
					.gpio_addr_offset = DN200_SFPCTRL_MODE0_BAROFF,
					.sfp_detect_pin = (0 << 16) | 5,	/*0 is valid */
					.sfp_tx_disable_pin = (0 << 16) | 0,
					.sfp_tx_fault_pin = (0 << 16) | 1,
					.sfp_rx_los_pin = (0 << 16) | 4,
					.sfp_rs0_pin = (0 << 16) | 6,
					.sfp_rs1_pin = (0 << 16) | 7,
					.sfp_led1_pin = (0 << 16) | 10,	// low active
					.sfp_led2_pin = (0 << 16) | 9,
					.reg_off_set_write = 0xc,
					.reg_off_set_read = 0x8,
					},
				},
			},
	/*2p */
	[1] = {
			/*2p old pcb */
			[0] = {
				/*xgmac0 */
				[0] = {
					.gpio_addr_offset = DN200_SFPCTRL_MODE1_BAROFF,
					.sfp_detect_pin = (0 << 16) | 10,
					.sfp_tx_disable_pin = (0 << 16) | 7,
					.sfp_tx_fault_pin = (0 << 16) | 8,
					.sfp_rx_los_pin = (0 << 16) | 9,
					.sfp_rs0_pin = (0 << 16) | 11,
					.sfp_rs1_pin = (0 << 16) | 11,
					.sfp_led1_pin = (0 << 16) | 21,	// low active
					.sfp_led2_pin = (0 << 16) | 14,
					.reg_off_set_write = 0x10,
					.reg_off_set_read = 0x0,
					},
				/*xgmac1 */
				[1] = {
					.gpio_addr_offset = DN200_SFPCTRL_MODE1_BAROFF,
					.sfp_detect_pin = (0 << 16) | 25,
					.sfp_tx_disable_pin = (0 << 16) | 22,
					.sfp_tx_fault_pin = (0 << 16) | 23,
					.sfp_rx_los_pin = (0 << 16) | 24,
					.sfp_rs0_pin = (0 << 16) | 26,
					.sfp_rs1_pin = (0 << 16) | 26,
					.sfp_led1_pin = (0 << 16) | 49,	// low active
					.sfp_led2_pin = (0 << 16) | 37,
					.reg_off_set_write = 0x10,
					.reg_off_set_read = 0x0,
					},
				},
			/*2p new pcb */
			[1] = {
				/*xgmac0 */
				[0] = {
					.gpio_addr_offset = DN200_SFPCTRL_MODE1_BAROFF,
					.sfp_detect_pin = (0 << 16) | 14,
					.sfp_tx_disable_pin = (1 << 16) | 7,
					.sfp_tx_fault_pin = (1 << 16) | 8,
					.sfp_rx_los_pin = (1 << 16) | 9,
					.sfp_rs0_pin = (1 << 16) | 11,
					.sfp_rs1_pin = (1 << 16) | 11,
					.sfp_led1_pin = (1 << 16) | 21,	// low active
					.sfp_led2_pin = (1 << 16) | 10,
					.reg_off_set_write = 0x10,
					.reg_off_set_read = 0x0,
					},
				/*xgmac1 */
				[1] = {
					.gpio_addr_offset = DN200_SFPCTRL_MODE1_BAROFF,
					.sfp_detect_pin = (0 << 16) | 24,
					.sfp_tx_disable_pin = (1 << 16) | 22,
					.sfp_tx_fault_pin = (1 << 16) | 23,
					.sfp_rx_los_pin = (1 << 16) | 25,
					.sfp_rs0_pin = (1 << 16) | 26,
					.sfp_rs1_pin = (1 << 16) | 26,
					.sfp_led1_pin = (1 << 16) | 32,	// low active
					.sfp_led2_pin = (1 << 16) | 37,
					.reg_off_set_write = 0x10,
					.reg_off_set_read = 0x0,
					},
				},
			},
};

struct xge_link_config link_config[XGE_LINK_MODE_MAX - 1] = {
	/*EXTERNAL_PHY_1000BASEX */
	{
		.has_xpcs = 0,
		.clk_ptp_rate = 250000000, // 250MHz for XGMAC, 50MHz for GMAC
		.clk_csr = 0x8, // DN200_CSR_I_4
		.phy_interface = PHY_INTERFACE_MODE_1000BASEX,
		.max_speed = 1000,
		.clk_ref_rate = 500000000,
	},
	/*EXTERNAL_PHY_SGMII */
	{
		.has_xpcs = 0,
		.clk_ptp_rate = 250000000,
		.clk_csr = 0x8,
		.phy_interface = PHY_INTERFACE_MODE_SGMII,
		.max_speed = 1000,
		.clk_ref_rate = 500000000,
	},
	/*EXTERNAL_PHY_RGMII */
	{
		.has_xpcs = 0,
		.clk_ptp_rate = 250000000,
		.clk_csr = 0x5, /*Clock Range 400-500M, refer to MDIO_Single_Command_Control_Data */
		.phy_interface = PHY_INTERFACE_MODE_RGMII_ID,
		.max_speed = 1000,
		.clk_ref_rate = 500000000,
	},
	/*XPCS_PHY_1000BASEX */
	{
		.has_xpcs = 1,
		.clk_ptp_rate = 250000000,
		.clk_csr = 0x8,
		.phy_interface = PHY_INTERFACE_MODE_GMII,
		.max_speed = 1000,
		.clk_ref_rate = 500000000,
	},
	/*XPCS_PHY_10GBASER */
	{
		.has_xpcs = 1,
		.clk_ptp_rate = 250000000,
		.clk_csr = 0x5,
		.phy_interface = PHY_INTERFACE_MODE_XGMII,
		.max_speed = 10000,
		.clk_ref_rate = 500000000,
	},
};

const struct xge_private_data xge_private_data_table[XGE_NUM] = {
	[0] = {
			/* phy address is just used by external MDIO phy,
			 * xpcs do not use it
			 */
			.phy_addr = 1,
			.bus_id = 1,
			.tx_queues_to_use = XGE01_QUEUES_TO_USE,
			.rx_queues_to_use = XGE01_QUEUES_TO_USE,
			.rx_queues_reserved = 8,
			.tx_queues_reserved = 8,
			.max_vfs = 8,
			.rx_queues_total = 16,
			.tx_queues_total = 16,
			.rx_used_mtl_queues = 4,
			},
	[1] = {
			.phy_addr = 2,
			.bus_id = 2,
			.tx_queues_to_use = XGE01_QUEUES_TO_USE,
			.rx_queues_to_use = XGE01_QUEUES_TO_USE,
			.rx_queues_reserved = 8,
			.tx_queues_reserved = 8,
			.max_vfs = 8,
			.rx_queues_total = 16,
			.tx_queues_total = 16,
			.rx_used_mtl_queues = 4,
			},
	[2] = {
			.phy_addr = 3,
			.bus_id = 3,
			.tx_queues_to_use = XGE23_QUEUES_TO_USE,
			.rx_queues_to_use = XGE23_QUEUES_TO_USE,
			.rx_queues_reserved = 1,
			.tx_queues_reserved = 1,
			.max_vfs = 1,
			.rx_queues_total = 2,
			.tx_queues_total = 2,
			.rx_used_mtl_queues = 2,
			},
	[3] = {
			.phy_addr = 4,
			.bus_id = 4,
			.tx_queues_to_use = XGE23_QUEUES_TO_USE,
			.rx_queues_to_use = XGE23_QUEUES_TO_USE,
			.rx_queues_reserved = 1,
			.tx_queues_reserved = 1,
			.max_vfs = 1,
			.rx_queues_total = 2,
			.tx_queues_total = 2,
			.rx_used_mtl_queues = 2,
			},
};

struct pci_device_map_t {
	int pci_funcid[4];
	int xpcs_index[4];
	int pf_deviceid;
	int vf_deviceid;
	int max_pf;
	int min_vf_funcid[4];
	int max_vf_funcid[4];
	int sriov_supported;
	int nvme_supported;
	u8 pf_max_iatu[4]; /* max iatu of each pf */
	u8 vf_total_iatu[4]; /* vf total iatu of each pf */
	struct xge_link_config *link_config;
	bool raid_supported;
	bool upgrade_with_flowing;
};

static int dn200_pcb_id_map2_gpio_type[8] = { 0, 1, 1 };

#define DN200_2P_MIN_PCB_ID 0
#define DN200_2P_MAX_PCB_ID 2
struct pci_device_map_t device_map[] = {
	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_4P_SRIOV_PF,
	 .vf_deviceid = DN200_DEV_ID_SFP_10G_4P_SRIOV_VF,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3,
			},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   0x4, 0x13, 0x22, 0x23,
			   },
	 .max_vf_funcid = {
			   0x12, 0x21, 0x22, 0x23,
			   },
	 .sriov_supported = 1,
	 .nvme_supported = true,
	 .pf_max_iatu = {
			 4, 4, 4, 4},
	 .vf_total_iatu = {
			   8, 8, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_4P_PURE_PF,
	 .vf_deviceid = 0,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   -1, -1, -1, -1,
			   },
	 .max_vf_funcid = {
			   -1, -1, -1, -1,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 0,
	 .pf_max_iatu = {
			 8, 8, 8, 8},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_2P_SRIOV_PF,
	 .vf_deviceid = DN200_DEV_ID_SFP_10G_2P_SRIOV_VF,
	 .max_pf = 2,
	 .pci_funcid = {
			0, 1,
			},
	 .xpcs_index = {
			0, 1,
			},
	 .min_vf_funcid = {
			   0x2, 0xa,
			   },
	 .max_vf_funcid = {
			   0x9, 0x11,
			   },
	 .sriov_supported = 1,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 8, 8, 0, 0},
	 .vf_total_iatu = {
			   8, 8, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_2P_PURE_PF,
	 .vf_deviceid = 0,
	 .max_pf = 2,
	 .pci_funcid = {
			0, 1, 2, 3,
			},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   0x2, 0x11,
			   },
	 .max_vf_funcid = {
			   0x10, 0x1f,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 0,
	 .pf_max_iatu = {
			 0, 0, 0, 0},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_1G_4P_SRIOV_PF,
	 .vf_deviceid = DN200_DEV_ID_SFP_1G_4P_SRIOV_VF,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   0x4, 0x13, 0x22, 0x23,
			   },
	 .max_vf_funcid = {
			   0x12, 0x21, 0x22, 0x23,
			   },
	 .sriov_supported = 1,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 4, 4, 4, 4},
	 .vf_total_iatu = {
			   8, 8, 0, 0},
	 .link_config = &link_config[EXTERNAL_PHY_1000BASEX],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_1G_4P_PURE_PF,
	 .vf_deviceid = 0,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   0x4, 0x13, 0x22, 0x23,
			   },
	 .max_vf_funcid = {
			   0x12, 0x21, 0x22, 0x23,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 0,
	 .pf_max_iatu = {
			 8, 8, 8, 8},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[EXTERNAL_PHY_1000BASEX],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_2P_NVME_PUREPF,
	 .vf_deviceid = 0,
	 .max_pf = 2,
	 .pci_funcid = {
			0, 1},
	 .xpcs_index = {
			0, 1},
	 .min_vf_funcid = {
			   0, 0, 0, 0,
			   },
	 .max_vf_funcid = {
			   0, 0, 0, 0,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 16, 16, 0, 0},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF,
	 .vf_deviceid = 0,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3},
	 .xpcs_index = {
			0, 1, 2, 3,
			},
	 .min_vf_funcid = {
			   0, 0, 0, 0,
			   },
	 .max_vf_funcid = {
			   0, 0, 0, 0,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 8, 8, 8, 8},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = false,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_PF,
	 .vf_deviceid = DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_VF,
	 .max_pf = 2,
	 .pci_funcid = {
			0, 1,
			},
	 .xpcs_index = {
			0, 1,
			},
	 .min_vf_funcid = {
			   0x3, 0xb,
			   },
	 .max_vf_funcid = {
			   0xa, 0x12,
			   },
	 .sriov_supported = 1,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 8, 8, 0, 0},
	 .vf_total_iatu = {
			   8, 8, 0, 0},
	 .link_config = &link_config[XPCS_PHY_10GBASER],
	 .raid_supported = true,
	 .upgrade_with_flowing = false,
	  },

	{
	 .pf_deviceid = DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF,
	 .vf_deviceid = 0,
	 .max_pf = 4,
	 .pci_funcid = {
			0, 1, 2, 3},
	 .xpcs_index = {
			-1, -1, -1, -1,
			},
	 .min_vf_funcid = {
			   -1, -1, -1, -1,
			   },
	 .max_vf_funcid = {
			   -1, -1, -1, -1,
			   },
	 .sriov_supported = 0,
	 .nvme_supported = 1,
	 .pf_max_iatu = {
			 8, 8, 8, 8},
	 .vf_total_iatu = {
			   0, 0, 0, 0},
	 .link_config = &link_config[EXTERNAL_PHY_RGMII],
	 .raid_supported = true,
	 .upgrade_with_flowing = true,
	  },

};

static int PURE_PF_DEVICE[] = {
	DN200_DEV_ID_SFP_10G_2P_PURE_PF,
	DN200_DEV_ID_SFP_10G_4P_PURE_PF,
	DN200_DEV_ID_SFP_1G_4P_PURE_PF,
	DN200_DEV_ID_COPP_1G_4P_PURE_PF,
};

static int NVME_PURE_PF_DEVICE[] = {
	DN200_DEV_ID_SFP_10G_2P_NVME_PUREPF,
	DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF,
	DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF,
};

static int SRIOV_VF_DEVICE[] = {
	DN200_DEV_ID_SFP_10G_2P_SRIOV_VF,
	DN200_DEV_ID_SFP_10G_4P_SRIOV_VF,
	DN200_DEV_ID_SFP_1G_4P_SRIOV_VF,
	DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_VF,
};

static int SRIOV_4P_DEVICE[] = {
	DN200_DEV_ID_SFP_10G_4P_SRIOV_PF,
	DN200_DEV_ID_SFP_10G_4P_PURE_PF,
	DN200_DEV_ID_SFP_1G_4P_SRIOV_PF,
	DN200_DEV_ID_SFP_1G_4P_PURE_PF,
	DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF,
};

static int EXTERN_PHY_DEVICE[] = {
	DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF,
};
/*address limit for different product or driver*/
struct dn200_addr_limit {
	int drv_type;
	int addr_bits_limit;
	u64 addr_forbid_bits;
} addr_limit[] = {
	{
	 .drv_type = DRV_PURE_PF,
	 .addr_bits_limit = 40,	/* support 1TB memory normal memory access */
	 .addr_forbid_bits = DN200_BASE_IATU_ADDR,
	},
	{
	 .drv_type = DRV_SRIOV_PF, /* highest 3 bits used for iATU VF high addr map */
	 .addr_bits_limit = 40,
	 .addr_forbid_bits = DN200_BASE_IATU_ADDR,	/* higher 3 bits of 40bits used for iATU VF map */
	},
	{
	 .drv_type = DRV_VF,
	 .addr_bits_limit = 32,	/* vf just support dma32 memory access */
	 .addr_forbid_bits = 0,
	},
};

#define DN200_FIRST_VF_FUNC_ID	0x04
#define DN200_IATU_ADDR_START	0xE0

#endif /* __DN200_PROD_H__ */
