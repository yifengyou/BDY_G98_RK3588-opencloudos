/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_EMP_INFO_H
#define _YK3_EMP_INFO_H

enum {
	YK3_BOARD_TYPE_INVALID = 0,
	YK3_BOARD_TYPE_FPGA = 1,
	YK3_BOARD_TYPE_ASIC = 2,

	YK3_BOARD_TYPE_END
};

enum {
	YK3_NIC_TYPE_INVALID = 0,
	YK3_NIC_TYPE_3100T = 1,

	YK3_NIC_TYPE_END
};

enum yk3_chiptype {
	YK3_CHIPTYPE_ASIC = 0,
	YK3_CHIPTYPE_FPGA = 1,
};

enum yk3_porttype_speed {
	YK3_PORTTYPE_SPEED_10G = 0,
	YK3_PORTTYPE_SPEED_25G = 1,
	YK3_PORTTYPE_SPEED_40G = 2,
	YK3_PORTTYPE_SPEED_100G = 3,
};

enum yk3_board_type {
	BOARD_TYPE_MIN = 0,
	K3_PROTOTYPE = 1,
	K3_2XSFP = 2,
	K3_1XQSFP = 3,
	K3_R_PROTOTYPE = 4,
	K3_4XSFP = 5,
	BOARD_TYPE_MAX,
};

#define SERIAL_NUM_LEN_MAX 20
#define PRODUCT_NUM_LEN_MAX 20
#define PRODUCT_NAME_LEN_MAX 24
#define VENDOR_NAME_LEN_MAX 16
#define MODEL_NAME_LEN_MAX 16

struct yk3_vpd_info {
	u8 magic_id; /* fixed to 'I' */
	u8 ver;
	u8 cksum;
	u8 flag;
	u32 img_version; /* IMAGE version */
	u8 fw_version[4]; /* EMP version */
	u32 np_version; /* NP version */
	u32 ppp0_version; /* PPP version */
	u32 ppp1_version; /* PPP version */
	u32 ppp2_version; /* PPP version */
	u32 ppp3_version; /* PPP version */
	u32 pxe_version; /* PXE version */
	u8 chip_type; /* chip type, 0 = ASIC , 1 = FPGA, ... , */
	u8 board_type;
	/* Board type:
	 * 1 = K3_PROTOTYPE
	 * 2 = K3_2XSFP
	 * 3 = K3_1XQSFP
	 * 4 = K3_R_PROTOTYPE
	 * 5 = K3_4XSFP
	 */
	u8 nic_type; /* board subtype, 0 = 3100T,  */
	u8 porttype; /* MAC port type, 0 = None, 0x02 = 10Gx2, 0x12=25Gx2 ... */
	u8 runmode; /* 0=dummy, 1=Tmode, 2=Rmode */
	u8 pxe_enable; /* 1=enable, 0=disable */
	u8 pxe_type;
	u8 reserved;
	u8 mac_addr[8];
	u8 sn[SERIAL_NUM_LEN_MAX]; /* string */
	u8 pn[PRODUCT_NUM_LEN_MAX]; /* string */
	u8 board_id;
	u8 pcb_ver;
	u8 reserved_a[2];
	u8 default_mac_addr[8];
	u8 product_name[PRODUCT_NAME_LEN_MAX];
	u8 vendor_name[VENDOR_NAME_LEN_MAX];
	u8 model_name[MODEL_NAME_LEN_MAX];
};

struct yk3_runtime_info {
	u32 temp_core; /* = (temperature + 100) x100 */
};

struct yk3_emp_info {
	struct yk3_vpd_info vpd;
	struct yk3_runtime_info runtime;
};

#define YK3_EMP_INFO_ADDR (0x330000 + 0x800) /* Mailbox LF8 RAM */
#define YK3_RUNTIME_INFO_ADDR (YK3_EMP_INFO_ADDR + sizeof(struct yk3_vpd_info))
#define YK3_EMP_TICK_ADDR 0x02200110
#define YK3_TEMP_CODE_START 0x0000010
#define YK3_TEMP_CODE_VALID_END 0x000001B
#define YK3_TEMP_CODE_END 0x000001F
#define YK3_VOL_CODE_START 0x00000020
#define YK3_VOL_CODE_VALID_END 0x0000002B
#define YK3_VOL_CODE_END 0x0000002F
#define YK3_ECC_CODE_START 0x0000030
#define YK3_ECC_CODE_END 0x0000031
#define YK3_I2C_CODE_START 0x00000040
#define YK3_I2C_CODE_END 0x0000004F
#define YK3_I2C_NUM 4
#define YK3_CHIP_CODE_START 0x000050
#define YK3_CHIP_CODE_END 0x000050
#define YK3_CHIP_DATA_PCIE_STRAT_BIT 0
#define YK3_CHIP_DATA_PCIE_END_BIT 31
#define YK3_CHIP_DATA_DOE_STRAT_BIT 64
#define YK3_CHIP_DATA_DOE_END_BIT 64
#define YK3_CHIP_DATA_EMP_STRAT_BIT 65
#define YK3_CHIP_DATA_EMP_END_BIT 65
#define YK3_CHIP_DATA_BAR_STRAT_BIT 66
#define YK3_CHIP_DATA_BAR_END_BIT 69
#define YK3_CHIP_DATA_HQOS_STRAT_BIT 70
#define YK3_CHIP_DATA_HQOS_END_BIT 70
#define YK3_CHIP_DATA_EDMA_STRAT_BIT 71
#define YK3_CHIP_DATA_EDMA_END_BIT 71
#define YK3_CHIP_DATA_NP_STRAT_BIT 72
#define YK3_CHIP_DATA_NP_END_BIT 103
#define YK3_CHIP_DATA_NOC_STRAT_BIT 104
#define YK3_CHIP_DATA_NOC_END_BIT 316
#define YK3_CHIP_DATA_MBOX_STRAT_BIT 317
#define YK3_CHIP_DATA_MBOX_END_BIT 319
#define YK3_CHIP_DATA_CLK_STRAT_BIT 320
#define YK3_CHIP_DATA_CLK_END_BIT 327
#define YK3_CHIP_DATA_MAX_NUM 512
#define YK3_CHIP_ALARM_DATA_SIZE ((YK3_CHIP_DATA_MAX_NUM + 31) / 32)

enum yk3_ecc_alarm_data_bit {
	YK3_ECC_DATA_EMP_BIT = 0,
	YK3_ECC_DATA_M0PPP_BIT = 1,
	YK3_ECC_DATA_M1PPP_BIT = 2,
	YK3_ECC_DATA_HPPP_BIT = 3,
	YK3_ECC_DATA_SPPP_BIT = 4,
	YK3_ECC_DATA_HDMA_BIT = 5,
	YK3_ECC_DATA_SDMA_BIT = 6,
	YK3_ECC_DATA_MAC0_BIT = 7,
	YK3_ECC_DATA_MAC1_BIT = 8,
	YK3_ECC_DATA_NP_BIT = 9,
	YK3_ECC_DATA_DOE_BIT = 10,
	YK3_ECC_DATA_HQOS_BIT = 11,
	YK3_ECC_DATA_HOST_BIT = 12,
	YK3_ECC_DATA_SOC_BIT = 13,
	YK3_ECC_DATA_LAN_BIT = 14,
	YK3_ECC_DATA_NVME_BIT = 15,
	YK3_ECC_DATA_FDMA_BIT = 16,
	YK3_ECC_DATA_HMAIL_BIT = 17,
	YK3_ECC_DATA_SMAIL_BIT = 18,
	YK3_ECC_DATA_MEMS_BIT = 19,
	YK3_ECC_DATA_ADAP_BIT = 20
};

#define YK3_ALARM_URGENT_MAX 3

#define YK3_SPEED_MASK 0xf0
#define YK3_SPEED_SHIFT 4
#define YK3_PORTNUM_MASK 0x0f

#endif /* _YK3_EMP_INFO_H */
