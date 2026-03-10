#ifndef _EN_AUX_CMD_H_
#define _EN_AUX_CMD_H_

#include <linux/ethtool.h>
#include "../msg_common.h"

#define ZXDH_QRES_TBL_LEN (300)
#define ZXDH_QS_PAIRS (2)

#define INVALID_PHY_PORT 0xff
#define ZXDH_PHY_PORT_MAX 9
#define ZXDH_MAX_HASH_INDEX 6 //TODO:should is 5

/* HASH_FUNC TYPE */
#define ZXDH_FUNC_TOP 0x04
#define ZXDH_FUNC_XOR 0x02
#define ZXDH_FUNC_CRC32 0x01

/* RX_NFC */
#define ZXDH_NET_RX_FLOW_HASH_MV 4
#define ZXDH_NET_RX_FLOW_HASH_SDT 2
#define ZXDH_NET_RX_FLOW_HASH_SDFNT 1

/* RISCV OPCODE */
#define RISC_TYPE_READ 0
#define RISC_FIELD_PANEL_ID 5
#define RISC_FIELD_PHYPORT_CHANNEL 6
#define RISC_FIELD_HASHID_CHANNEL 10
#define RISC_SERVER_TIME 0xF0

#define MAX_PANEL_ID 9

/* SPM STATS */
#define ZXDH_SPM_STATS_OFFSET (0x24000 + 0x1000 + 408)
#define ZXDH_SPM_BYTES_OFFSET (0x24000 + 0xb000)

/* ZF bar address bit[16:63] needs to be shifted left by 4 bits */
#define TO_ZF_ADDR(addr) (((addr & 0xFFFFFFFFFFFF0000) << 4) | (addr & 0xFFFF))

#define ZXDH_NP_GLOBAL_PSN_ENABLE_BIT (28)

//#define MAC_CONFIG_DEBUG 1
enum riscv_op_code {
	OP_CODE_WRITE = 1,
	OP_CODE_MSGQ_CHAN = 2,
	OP_CODE_DATA_CHAN = 3,
	OP_CODE_MAX,
};

#define OP_CODE_TBL_STAT (0xaa)
#define MSG_STRUCT_HD_LEN 8

struct queue_index_message {
	uint8_t type;
	uint8_t field;
	uint16_t ep_bdf;
	uint16_t write_bytes;
	uint16_t rsv;
	uint16_t write_data[0];
} __attribute__((packed));

struct cmd_hdr_recv {
	uint8_t check;
	uint8_t rsv;
	uint16_t data_len_bytes;
};

struct cmd_tbl_ack {
	struct cmd_hdr_recv hdr;
	uint8_t phy_port;
	uint8_t rsv[3];
} __attribute__((packed));

enum zxdh_msg_chan_opc {
	ZXDH_VPORT_GET = 4,
	ZXDH_PHYPORT_GET = 6,
};

struct zxdh_debug_msg {
	uint8_t opcode;
	uint8_t phyport;
	bool lldp_enable;
} __attribute__((packed));

struct zxdh_debug_rcv_msg {
	uint8_t reps_states;
	uint8_t lldp_enable;
} __attribute__((packed));

enum zxdh_en_link_speed_bit_indices {
	SPM_SPEED_1X_1G = 2,
	SPM_SPEED_1X_10G = 5,
	SPM_SPEED_1X_25G = 6,
	SPM_SPEED_1X_50G = 7,
	SPM_SPEED_2X_100G = 8,
	SPM_SPEED_4X_40G = 9,
	SPM_SPEED_4X_100G = 10,
	SPM_SPEED_4X_200G = 11,
};

enum zxdh_en_fec_mode_bit_indices {
	SPM_FEC_NONE = 0,
	SPM_FEC_BASER = 1,
	SPM_FEC_RS528 = 2,
	SPM_FEC_RS544 = 3,
};

enum zxdh_en_fc_mode_bit_indices {
	SPM_FC_NONE = 0,
	SPM_FC_PAUSE_RX = 1,
	SPM_FC_PAUSE_TX = 2,
	SPM_FC_PAUSE_FULL = 3,
	SPM_FC_PFC_FULL = 4,
};

struct zxdh_en_module_eeprom_param {
	uint8_t i2c_addr;
	uint8_t bank;
	uint8_t page;
	uint8_t offset;
	uint8_t length;
};

#define SFF_I2C_ADDRESS_LOW (0x50)
#define SFF_I2C_ADDRESS_HIGH (0x51)

enum zxdh_module_id {
	ZXDH_MODULE_ID_SFP = 0x3,
	ZXDH_MODULE_ID_QSFP = 0xC,
	ZXDH_MODULE_ID_QSFP_PLUS = 0xD,
	ZXDH_MODULE_ID_QSFP28 = 0x11,
	ZXDH_MODULE_ID_QSFP_DD = 0x18,
	ZXDH_MODULE_ID_OSFP = 0x19,
	ZXDH_MODULE_ID_DSFP = 0x1B,
	ZXDH_MODULE_ID_QSFP_PLUS_WITH_CMIS = 0x1E,
	ZXDH_MODULE_ID_SFP_DD_WITH_CMIS = 0x1F,
	ZXDH_MODULE_ID_SFP_PLUS_WITH_CMIS = 0x20,
};

#define SPEED_MODES_TO_SPEED(speed_modes, speed)                     \
	do {                                                         \
		if (((speed_modes)&BIT(SPM_SPEED_1X_1G)) ==          \
		    BIT(SPM_SPEED_1X_1G)) {                          \
			(speed) = SPEED_1000;                        \
		} else if (((speed_modes)&BIT(SPM_SPEED_1X_10G)) ==  \
			   BIT(SPM_SPEED_1X_10G)) {                  \
			(speed) = SPEED_10000;                       \
		} else if (((speed_modes)&BIT(SPM_SPEED_1X_25G)) ==  \
			   BIT(SPM_SPEED_1X_25G)) {                  \
			(speed) = SPEED_25000;                       \
		} else if (((speed_modes)&BIT(SPM_SPEED_4X_40G)) ==  \
			   BIT(SPM_SPEED_4X_40G)) {                  \
			(speed) = SPEED_40000;                       \
		} else if (((speed_modes)&BIT(SPM_SPEED_1X_50G)) ==  \
			   BIT(SPM_SPEED_1X_50G)) {                  \
			(speed) = SPEED_50000;                       \
		} else if (((speed_modes)&BIT(SPM_SPEED_2X_100G)) == \
			   BIT(SPM_SPEED_2X_100G)) {                 \
			(speed) = SPEED_100000;                      \
		} else if (((speed_modes)&BIT(SPM_SPEED_4X_100G)) == \
			   BIT(SPM_SPEED_4X_100G)) {                 \
			(speed) = SPEED_100000;                      \
		} else if (((speed_modes)&BIT(SPM_SPEED_4X_200G)) == \
			   BIT(SPM_SPEED_4X_200G)) {                 \
			(speed) = SPEED_200000;                      \
		} else {                                             \
			(speed) = SPEED_UNKNOWN;                     \
		}                                                    \
	} while (0)

#define GET_VFID(vport)                                                     \
	((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) ? \
		       (PF_VQM_VFID_OFFSET + EPID(vport) * 8 + FUNC_NUM(vport)) : \
		       (EPID(vport) * 256 + VFUNC_NUM(vport)))

#define DH_AUX_PF_ID_OFFSET(vport) (EPID(vport) * 8 + FUNC_NUM(vport))

#define NP_GET_PKT_CNT 0
#define NP_CLEAR_PKT_CNT 1

struct zxdh_en_device;
int32_t get_common_table_msg(struct zxdh_en_device *en_dev, uint16_t pcie_id,
			     uint8_t field, void *ack);
int32_t zxdh_common_tbl_init(struct net_device *netdev,
			     union zxdh_msg *old_msg);
int32_t zxdh_en_phyport_init(struct zxdh_en_device *en_dev);
int32_t zxdh_en_autoneg_set(struct zxdh_en_device *en_dev, uint8_t enable,
			    uint32_t speed_modes);
int32_t zxdh_vport_stats_get(struct zxdh_en_device *en_dev);
int32_t zxdh_en_vport_pre_stats_get(struct zxdh_en_device *en_dev);
int32_t zxdh_mac_stats_get(struct zxdh_en_device *en_dev);
int32_t zxdh_en_udp_pkt_stats_get(struct zxdh_en_device *en_dev);
int32_t zxdh_mac_stats_clear(struct zxdh_en_device *en_dev);
int32_t zxdh_hash_id_get(struct zxdh_en_device *en_dev);
int32_t zxdh_en_fec_mode_set(struct zxdh_en_device *en_dev, uint32_t fec_cfg);
int32_t zxdh_en_fec_mode_get(struct zxdh_en_device *en_dev, uint32_t *fec_cap,
			     uint32_t *fec_cfg, uint32_t *fec_active);
int32_t zxdh_en_fc_mode_set(struct zxdh_en_device *en_dev, uint32_t fc_mode);
int32_t zxdh_en_fc_mode_get(struct zxdh_en_device *en_dev, uint32_t *fc_mode);
uint32_t zxdh_en_module_eeprom_read(struct zxdh_en_device *en_dev,
				    struct zxdh_en_module_eeprom_param *query,
				    uint8_t *data);
int32_t zxdh_lldp_enable_set(struct zxdh_en_device *en_dev, bool lldp_enable);
int32_t zxdh_sshd_enable_set(struct zxdh_en_device *en_dev, bool sshd_enable);
int32_t zxdh_vf_dpp_add_mac(struct zxdh_en_device *en_dev,
			    const uint8_t *dev_addr, uint8_t filter_flag);
int32_t zxdh_vf_dpp_del_mac(struct zxdh_en_device *en_dev,
			    const uint8_t *dev_addr, uint8_t filter_flag,
			    bool mac_flag);
int32_t zxdh_vf_dpp_dump_mac(struct zxdh_en_device *en_dev,
			     const uint8_t *dev_addr);
void zxdh_vport_uninit(struct net_device *netdev);
int32_t zxdh_pf_port_init(struct zxdh_en_device *en_dev, bool boot);
int32_t zxdh_vf_dpp_port_init(struct zxdh_en_device *en_dev);
int32_t zxdh_port_init(struct net_device *netdev);
int32_t zxdh_vf_egr_port_attr_set(struct zxdh_en_device *en_dev, uint32_t mode,
				  uint32_t value, uint8_t fow);
int32_t zxdh_vf_egr_port_attr_get(struct zxdh_en_device *en_dev,
				  ZXDH_SRIOV_VPORT_T *port_attr_entry);
int32_t zxdh_vf_rss_en_set(struct zxdh_en_device *en_dev, uint32_t enable);
int32_t zxdh_num_channels_changed(struct zxdh_en_device *en_dev,
				  uint16_t num_changed);
int32_t zxdh_pf_macpcs_num_get(struct zxdh_en_device *en_dev);
int32_t zxdh_lldp_enable_get(struct zxdh_en_device *en_dev,
			     uint32_t *lldp_enable);
int32_t zxdh_indir_to_queue_map(struct zxdh_en_device *en_dev,
				const uint32_t *indir);
int32_t zxdh_rxfh_set(struct zxdh_en_device *en_dev, uint32_t *queue_map);
void zxdh_rxfh_del(struct zxdh_en_device *en_dev);
void zxdh_u32_array_print(uint32_t *array, uint16_t size);
int32_t zxdh_en_firmware_version_get(struct zxdh_en_device *en_dev,
				     uint8_t *fw_version,
				     uint8_t *fw_version_len);
int32_t zxdh_panel_id_init(struct zxdh_en_device *en_dev);
int32_t zxdh_vf_port_promisc_set(struct zxdh_en_device *en_dev, uint8_t mode,
				 uint8_t value, uint8_t fow);
int32_t zxdh_phyport_get(struct zxdh_en_device *en_dev);
int32_t zxdh_vf_1588_call_np_interface(struct zxdh_en_device *en_dev);
int32_t zxdh_aux_alloc_pannel(struct zxdh_en_device *en_dev);
int8_t zxdh_debug_ip_get(struct zxdh_en_device *en_dev, int8_t *ip);
int32_t zxdh_vf_dpp_add_ipv6_mac(struct zxdh_en_device *en_dev,
				 const uint8_t *mac_addr);
int32_t zxdh_vf_dpp_del_ipv6_mac(struct zxdh_en_device *en_dev,
				 const uint8_t *mac_addr);
int32_t zxdh_vf_dpp_add_lacp_mac(struct zxdh_en_device *en_dev,
				 const uint8_t *mac_addr);
int32_t zxdh_vf_dpp_del_lacp_mac(struct zxdh_en_device *en_dev,
				 const uint8_t *mac_addr);
int32_t zxdh_spm_port_enable_cfg(struct zxdh_en_device *en_dev,
				 uint32_t enable);
uint32_t zxdh_uplink_phy_attr_set(DPP_PF_INFO_T *pf_info, uint8_t phy_port,
				  uint32_t attr, uint32_t value);
bool zxdh_en_is_panel_port(struct zxdh_en_device *en_dev);
int32_t zxdh_get_vf_err_stats(struct zxdh_en_device *en_dev,
			      zxdh_get_sw_stats *payload,
			      zxdh_sw_stats_reply *reply);
int32_t zxdh_cfg_misx_mode(struct zxdh_en_device *en_dev, uint16_t rx_msix_mode,
			   uint16_t tx_msix_mode);
int32_t zxdh_get_misx_mode(struct zxdh_en_device *en_dev,
			   uint16_t *rx_msix_mode, uint16_t *tx_msix_mode);
int32_t zxdh_get_coalesce_usecs(struct zxdh_en_device *en_dev,
				uint32_t *rx_coalesce_usecs,
				uint32_t *tx_coalesce_usecs);
int32_t zxdh_cfg_coalesce_usecs(struct zxdh_en_device *en_dev,
				uint32_t rx_coalesce_usecs,
				uint32_t tx_coalesce_usecs);
int32_t zxdh_dual_tor_switch(struct zxdh_en_device *en_dev, bool state);
int32_t zxdh_dual_tor_label_get(struct zxdh_en_device *en_dev);
int32_t zxdh_en_hash_key_recover(struct zxdh_en_device *en_dev);
int32_t zxdh_vf_add_fd(struct zxdh_en_device *en_dev,
		       struct ethtool_rx_flow_spec *fs, uint32_t *index);
int32_t zxdh_vf_get_fd(struct zxdh_en_device *en_dev, uint32_t index);
int32_t zxdh_vf_del_fd(struct zxdh_en_device *en_dev, uint32_t index);
int32_t zxdh_vf_fd_en_set(struct zxdh_en_device *en_dev, uint32_t enable);

#endif /* END __ZXDH_EN_COMMAND_H_ */
