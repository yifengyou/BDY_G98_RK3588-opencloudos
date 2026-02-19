/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config extern phy or xpcs phy
 */

#ifndef __DN200_PHY_H__
#define __DN200_PHY_H__

#define DN200_PXE_USED 0
#define DN200_JUST_FOR_NORMAL_DRIVER 1
#define DN200_NORMAL_DRIVER 1

#include <linux/netdevice.h>
#include "dn200.h"


#define DN200_XPCS_BAR_OffSET (0x40200)
#define IC_READ 0
#define IC_WRITE 1
#define DN200_SOFT_AN_LINK_TIMES 20

/* EEPROM (dev_addr = 0xA0) */
#define DN200_I2C_EEPROM_DEV_ADDR	0x50

#define DN200_SFF_IDENTIFIER		0x00
#define DN200_SFF_IDENTIFIER_SFP	0x03

#define DN200_SFF_VENDOR_OUI_BYTE0	0x25
#define DN200_SFF_VENDOR_OUI_BYTE1	0x26
#define DN200_SFF_VENDOR_OUI_BYTE2	0x27

#define DN200_SFF_1GBE_COMP_CODES	0x06
#define DN200_SFP_BASE_1GBE_CC_SX		BIT(0)
#define DN200_SFP_BASE_1GBE_CC_LX		BIT(1)
#define DN200_SFP_BASE_1GBE_CC_CX		BIT(2)
#define DN200_SFP_BASE_1GBE_CC_T			BIT(3)

#define DN200_SFF_10GBE_COMP_CODES	0x03
#define DN200_SFP_BASE_10GBE_CC_SR		BIT(4)
#define DN200_SFP_BASE_10GBE_CC_LR		BIT(5)
#define DN200_SFP_BASE_10GBE_CC_LRM		BIT(6)
#define DN200_SFP_BASE_10GBE_CC_ER		BIT(7)

#define DN200_SFF_CABLE_TECHNOLOGY	0x08
#define DN200_SFP_BASE_CABLE_PASSIVE		BIT(2)
#define DN200_SFP_BASE_CABLE_ACTIVE		BIT(3)

#define DN200_SFP_BASE_BR			12
#define DN200_SFP_BASE_BR_1GBE_MIN		0x0a
#define DN200_SFP_BASE_BR_10GBE_MIN		0x64

#define DN200_SFF_SFF_8472_SWAP		0x5C
#define DN200_SFF_ADDRESSING_MODE		BIT(2)
#define DN200_SFF_DDM_IMPLEMENTED		BIT(6)

#define DN200_SFF_SFF_8472_COMP		0x5E
#define DN200_SFF_SFF_8472_UNSUP      0x00

#define BLINK_ENABLE 1
#define BLINK_DISABLE 0
/* SFF8472 A2 (dev_addr = 0xA2) */
#define DN200_I2C_SFF8472_DEV_ADDR	0x51
/* SFF GPIO PINs */
#define DN200_SFPCTRL_BAROFF 0x840130	// 0x840100-0x84012c is reserved

/**
 * bit0-tx_disable,		bit1-tx_fault,		bit4-sfp_loss,
 * bit5-sfp_mod_detect,	bit6-sfp_rs0,		bit7-sfp_rs1,
 * bit8-led0 from PWM,	bit9-led1 for 1G,	bit10-led2 for 10G
 */
#define XGMAC_SFP_DETECT_PIN		5
#define XGMAC_SFP_TX_DIS_PIN		0
#define XGMAC_SFP_TX_FAULT_PIN		1
#define XGMAC_SFP_RX_LOS_PIN		4
//#define XGMAC_SFP_SDA_PIN                     2
//#define XGMAC_SFP_SCL_PIN                     3
#define XGMAC_SFP_RS0_PIN			6
#define XGMAC_SFP_RS1_PIN			7
#define XGMAC_SFP_LED1_PIN			9	// low active
#define XGMAC_SFP_LED2_PIN			10	// low active

/*XPCS C45 MMD REG ADDR*/
#define SR_PMA_KR_PMD_CTRL	0x96
#define TR_EN				BIT(1)
#define TR_EN_S				1
#define RS_TR				BIT(0)
#define RS_TR_S				0
#define SR_PMA_KR_PMD_STS	0x97
#define SR_PMA_KR_LP_CEU	0x98
#define LP_PRST				BIT(13)
#define LP_INIT				BIT(12)

#define SR_PMA_KR_LP_CESTS	0x99
#define LP_RR				BIT(15)
#define LP_CFF_STS1			GENMASK(5, 4)
#define LP_CFF_STS0			GENMASK(3, 2)
#define LP_CFF_STSM0		GENMASK(1, 0)
#define SR_PMA_KR_LD_CEU	0x9a
#define PMA_CTRL1_LB		BIT(1)
#define SR_PMA_KR_LD_CESTS	0x9b
#define SR_PMA_KR_FEC_CTRL	0xab
#define VR_XS_PMA_RX_LSTS	0x8020
#define VR_XS_PMA_SRAM		0x809b
#define SRAM_BTLD_BYP		BIT(2)
#define SRAM_EXT_LD_DN		BIT(1)
#define VR_TX_GENCTRL		0x8030
#define TX_RST_0			BIT(8)
#define TX_DT_EN			BIT(12)
#define TX_EQ_CTRL0			0x8036
#define TX_EQ_MAIN			GENMASK(13, 8)
#define TX_EQ_MAIN_SHIFT	(8)
#define VR_RX_GENCTRL0			0x8050
#define RX_DT_EN_0_MASK		BIT(8)
#define RX_DT_EN_0			BIT(8)
#define RX_DT_EN_0_S		(8)
#define VR_RX_GENCTRL1      0x8051
#define RX_RST_0            BIT(4)

#define VR_PMA_KRTR_TIMER_CTRL1 0x8007
#define VR_PMA_KRTR_TIMER_CTRL2 0x8008
#define VR_PMA_KRTR_RX_EQ_CTRL	0x8009
#define RX_EQ_MM			BIT(15)
#define RR_RDY				BIT(8)
#define VR_PMA_KRTR_TX_EQ_STS_CTRL	0x800B
#define TX_EQ_MM			BIT(15)
#define VR_PMA_KRTR_TX_EQ_CFF_CTRL	0x800C
#define VR_PMA_PHY_TX_EQ_STS	0x800D

#define VR_PMA_PHY_RX_EQ_CEU			0x800e
#define CFF_UPDT_VLD_ALL				GENMASK(10, 8)
#define CFF_UPDT1_VLD					BIT(10)
#define CFF_UPDT0_VLD					BIT(9)
#define CFF_UPDTM1_VLD					BIT(8)
#define CFF_UPDT1						GENMASK(5, 4)
#define CFF_UPDT0						GENMASK(3, 2)
#define CFF_UPDTM1						GENMASK(1, 0)
#define CFF_UPDTM1_MASK		GENMASK(1, 0)
#define CFF_UPDT0_MASK		GENMASK(3, 2)
#define CFF_UPDT1_MASK		GENMASK(5, 4)

#define VR_XS_PMA_MP32G_RXCNTX_CTRL0	0x8092
#define VR_XS_PMA_MP_12G_16G_25G_MISC_STS 0x8098
#define RX_ADPT_ACK						BIT(12)
#define RX_ADPT_ACK_S					12
#define VR_XS_PMA_MP32G_TXCNTX_CTRL0	0x803E
#define VR_XS_PMA_MP32G_TXCMCNTX_SEL	0x803C
#define VR_XS_PMA_MP25G_TXWIDTH_CTRL	0x8046
#define VR_XS_PMA_MP25G_RXWIDTH_CTRL   0x80B0
#define VR_XS_PMA_MP25G_TX_EQ_CTRL0		0x8036
#define DN200_TX_MAIN_SHIFT				8
#define DN200_TX_MAIN_MASK				GENMASK(13, 8)
#define DN200_TX_PRE_SHIFT				0
#define DN200_TX_PRE_MASK				GENMASK(5, 0)
#define VR_XS_PMA_MP25G_TX_EQ_CTRL1		0x8037
#define DN200_TX_POST_SHIFT				0
#define DN200_TX_POST_MASK				GENMASK(5, 0)
#define DN200_TX_EQ_OVRD				BIT(6)
#define VR_XS_PMA_MP_32G_RX_EQ_CTRL4	0x805C
#define RX_AD_REQ						BIT(12)
#define RX_AD_REQ_S						12
#define RX_CDR_CTRL						0x8056

/*MMD PCS*/
#define XS_PCS_STS2_TF	BIT(11)
#define XS_PCS_STS2_RF	BIT(10)
#define RST_DUP1			BIT(15)
#define XS_PCS_LSTS			24
#define XS_PCS_KR_STS1		32
#define XPCS_10G_PLU		BIT(12)
#define RPCS_BKLK			BIT(0)
#define XS_PCS_KR_STS2		33
#define VR_XS_PCS_DIG_CTRL1	0x8000
#define XPCS_PCS_RST				BIT(15)
#define XPCS_EN_2_5G_MODE	BIT(2)
#define XPCS_PCS_BYP_PWRUP_DUP1		BIT(1)
#define VR_XS_PCS_DIG_CTRL2 0x8001
#define XPCS_PCS_TX_POL_INV GENMASK(7, 4)
#define XPCS_PCS_RX_POL_INV GENMASK(3, 0)
#define VR_XS_PCS_DEBUG_CTRL	0x8005
#define RX_DT_EN_CTL				BIT(6)
#define SUPRESS_LOS_DET				BIT(4)

/*MMD AN*/
#define AN_CTRL_AN_EN				BIT(12)
#define AN_CTRL_AN_EN_S				12
#define AN_CTRL_AN_RESTART			BIT(9)
#define AN_CTRL_AN_RESTART_S			9

#define AN_CTRL_LOW_POWER			BIT(11)

#define SR_AN_COMP_STS	0x30

/* SR_AN */
#define SR_AN_ADV1			0x10
#define AN_ADV_RF_13_MASK	BIT(13)
#define AN_ADV_RF_13_SHIFT	13
#define AAN_ADV_ACK_MASK	BIT(14)
#define AN_ADV_ACK_SHIFT	14
#define AN_ADV_NP			BIT(15)
#define AN_ADV_NP_S			15
#define SR_AN_ADV2			0x11
#define SR_AN_ADV3			0x12
#define KR10G_FEC_ABL		BIT(14)
#define KR10G_FEC_ABL_S		14
#define KR10G_FEC_REQ		BIT(15)
#define KR10G_FEC_REQ_S		15
#define SR_AN_LP_ABL1		0x13
#define SR_AN_LP_ABL2		0x14
#define SR_AN_LP_ABL3		0x15
#define SR_AN_XNP_TX1		0x16
#define SR_AN_XNP_TX2		0x17
#define SR_AN_XNP_TX3		0x18
#define AN_LP_XNP_ABL1		0x19
#define VR_AN_DIG_CTRL1		0x8000
#define CL73_TMR_OVR_RIDE	BIT(3)
#define CL73_TMR_OVR_RIDE_S	3
#define VR_AN_INTR_MSK		0x8001
#define VR_AN_INTR			0x8002
#define AN_PG_RCV			BIT(2)
#define AN_INC_LINK			BIT(1)
#define AN_INT_CMPLT		BIT(0)
#define VR_AN_TIMER_CTRL0	0x8004
#define VR_AN_TIMER_CTRL1	0x8005

/* Clause 73 Defines */
/* AN_LP_ABL1 */
#define C73_PAUSE			BIT(10)
#define C73_ASYM_PAUSE		BIT(11)
#define C73_AN_ADV_SF		0x1
/* AN_LP_ABL2 */
#define C73_1000KX			BIT(5)
#define C73_10000KX4			BIT(6)
#define C73_10000KR			BIT(7)
/* AN_LP_ABL3 */
#define C73_2500KX			BIT(0)
#define C73_5000KR			BIT(1)
#define C73_LP_FEC_EN		BIT(14)
#define C73_NEED_FEC_EN		BIT(15)

/* EEE Mode Control Register */
#define VR_MII_EEE_MCTRL0              0x8006
#define VR_MII_EEE_MCTRL1              0x800b
#define VR_MII_DIG_CTRL2               0x80e1
/* VR MII EEE Control 0 defines */
#define VR_MII_EEE_LTX_EN                      BIT(0)	/* LPI Tx Enable */
#define VR_MII_EEE_LRX_EN                      BIT(1)	/* LPI Rx Enable */
#define VR_MII_EEE_TX_QUIET_EN         BIT(2)	/* Tx Quiet Enable */
#define VR_MII_EEE_RX_QUIET_EN         BIT(3)	/* Rx Quiet Enable */
#define VR_MII_EEE_TX_EN_CTRL          BIT(4)	/* Tx Control Enable */
#define VR_MII_EEE_RX_EN_CTRL          BIT(7)	/* Rx Control Enable */
/* VR MII EEE Control 1 defines */
#define VR_MII_EEE_TRN_LPI             BIT(0)	/* Transparent Mode Enable */

#define VR_MII_EEE_MULT_FACT_100NS_SHIFT       8
#define VR_MII_EEE_MULT_FACT_100NS             GENMASK(11, 8)

#define SR_MII_CTRL 0x0
#define AN_ENABLE BIT(12)
#define AN_ENABLE_SHIFT 12
#define AN_ENABLE_MASK BIT(12)

#define XPCSCLKENABLE 0x20

enum rx_train_state {
	RX_EQ_NONE = 0,
	RX_EQ_SEND_INIT,
	RX_EQ_WAIT_UPDATE,
	RX_EQ_SEND_HOLD,
	RX_EQ_WAIT_NOTUPDATE,
	RX_EQ_SEND_COF,
	RX_EQ_POLL_COF,
	RX_EQ_LD_NOCMD,
	RX_EQ_READY,
};

enum tx_train_state {
	TX_EQ_NONE = 0,
	TX_EQ_POLL_LP_CMD,
	TX_EQ_WAIT_LD_VLD,
	TX_EQ_WAIT_HOLD_CMD,
	TX_EQ_WAIT_LD_INVLD,
	TX_EQ_LP_RDY,
};

struct xpcs_link_down_dump {
	char *reg_str;
	u8 dev;
	u16 reg;
};

#define GET_BITS(_var, _index, _width) \
	(((_var) >> (_index)) & ((0x1 << (_width)) - 1))

#define SET_BITS(_var, _index, _width, _val)                                \
	do {                                                                \
		__tmp_val = _val; \
		__tmp_index = _index; \
		__tmp_width = _width; \
		__tmp_unsed_var = _var; \
		(__tmp_val) &= ~(((0x1 << (__tmp_width)) - 1) << (__tmp_index));           \
		(__tmp_val) |= (((__tmp_val) & ((0x1 << (__tmp_width)) - 1)) << (__tmp_index)); \
	} while (0)

#define XGE_IOREAD(_pdata, _reg) ioread32((_pdata)->ioaddr 0x20000 + (_reg))

#define XGE_IOWRITE(_pdata, _reg, _val) \
	iowrite32((_val), (_pdata)->ioaddr + 0x20000 + (_reg))

#define XPCS_GET_BITS(_var, _prefix, _field)				\
	GET_BITS((_var),                                                \
		 _prefix##_##_field##_INDEX,                            \
		 _prefix##_##_field##_WIDTH)

#define XPCS_SET_BITS(_var, _prefix, _field, _val)                      \
	SET_BITS((_var),                                                \
		 _prefix##_##_field##_INDEX,                            \
		 _prefix##_##_field##_WIDTH, (_val))

#define XPCS32_IOWRITE(_pdata, _off, _val)	writel(_val, (_pdata)->xpcs_regs_base + DN200_XPCS_BAR_OffSET + (_off))

#define XPCS32_IOREAD(_pdata, _off)					\
	readl((_pdata)->xpcs_regs_base + DN200_XPCS_BAR_OffSET + (_off))

enum dn200_media_type {
	DN200_MEDIA_TYPE_UNKNOWN = 0,
	DN200_MEDIA_TYPE_XPCS_1000BASEX,
	DN200_MEDIA_TYPE_XPCS_10GBASEKR,
	DN200_MEDIA_TYPE_PHY_1000BASEX,
	DN200_MEDIA_TYPE_PHY_COPPER,
	DN200_MEDIA_TYPE_VIRTUAL,
	DN200_MEDIA_TYPE_MAX,
};

enum dn200_link_status {
	DN200_LINK_DOWN,
	DN200_LINK_UP,
};

enum dn200_duplex_info {
	DN200_DUP_HALF,
	DN200_DUP_FULL,
};

enum dn200_speed_info {
	DN200_SPEED_UNKOWN,
	DN200_SPEED_SGMII_10,
	DN200_SPEED_SGMII_100,
	DN200_SPEED_SGMII_1000,
	DN200_SPEED_1000BASEX,
	DN200_SPEED_10GKR,
	DN200_SPEED_2500,
};

enum dn200_sfp_type {
	DN200_SFP_TYPE_UNKNOWN = 0,
	DN200_SFP_TYPE_SR,
	DN200_SFP_TYPE_LR,
	DN200_SFP_TYPE_NOT_PRESENT = 0XFFFE,
	DN200_SFP_TYPE_NOT_KNOWN = 0XFFFF
};

enum dn200_sfp_module_type {
	DN200_PHY_SFP_MODULE_UNKNOWN = 0,
	DN200_PHY_SFP_MODULE_AVAGO,
	DN200_PHY_SFP_MODULE_INTEL,
	DN200_PHY_SFP_MODULE_GENERIC
};

#define DN200_SFF_VENDOR_OUI_AVAGO	0x00176A00

enum an_state {
	DN200_AN_DISABLE = 0,
	DN200_AN_ENABLE,
};

enum dn200_sfp_cable {
	DN200_SFP_CABLE_UNKNOWN = 0,
	DN200_SFP_CABLE_ACTIVE,
	DN200_SFP_CABLE_PASSIVE,
	DN200_SFP_CABLE_FIBRE,
};

enum dn200_sfp_speed {
	DN200_SFP_SPEED_UNKNOWN = 0,
	DN200_SFP_SPEED_100 = BIT(1),
	DN200_SFP_SPEED_1000 = BIT(2),
	DN200_SFP_SPEED_10000 = BIT(3),
};

enum dn200_sfp_speed_type {
	DN200_SFP_TYPE_SPEED_UNKNOWN = 0,
	DN200_SFP_TYPE_1000 = BIT(0),
	DN200_SFP_TYPE_10000 = BIT(1),
};

enum dn200_sfp_base {
	DN200_SFP_BASE_UNKNOWN = 0,
	DN200_SFP_BASE_1000_T = BIT(1),
	DN200_SFP_BASE_1000_SX = BIT(2),
	DN200_SFP_BASE_1000_LX = BIT(3),
	DN200_SFP_BASE_1000_CX = BIT(4),
	DN200_SFP_BASE_10000_SR = BIT(5),
	DN200_SFP_BASE_10000_LR = BIT(6),
	DN200_SFP_BASE_10000_LRM = BIT(7),
	DN200_SFP_BASE_10000_ER = BIT(8),
	DN200_SFP_BASE_10000_CR = BIT(9),
};

static const int dn200_xpcs_usxgmii_features[] = {
	ETHTOOL_LINK_MODE_Pause_BIT,
	ETHTOOL_LINK_MODE_Asym_Pause_BIT,
	ETHTOOL_LINK_MODE_Autoneg_BIT,
	ETHTOOL_LINK_MODE_1000baseKX_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseKX4_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_2500baseX_Full_BIT,
	__ETHTOOL_LINK_MODE_MASK_NBITS,
};

static const int dn200_xpcs_10gkr_features[] = {
	ETHTOOL_LINK_MODE_Pause_BIT,
	ETHTOOL_LINK_MODE_Asym_Pause_BIT,
	ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
	__ETHTOOL_LINK_MODE_MASK_NBITS,
};

static const int dn200_xpcs_sgmii_features[] = {
	ETHTOOL_LINK_MODE_Pause_BIT,
	ETHTOOL_LINK_MODE_Asym_Pause_BIT,
	ETHTOOL_LINK_MODE_Autoneg_BIT,
	ETHTOOL_LINK_MODE_10baseT_Half_BIT,
	ETHTOOL_LINK_MODE_10baseT_Full_BIT,
	ETHTOOL_LINK_MODE_100baseT_Half_BIT,
	ETHTOOL_LINK_MODE_100baseT_Full_BIT,
	ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
	ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
	__ETHTOOL_LINK_MODE_MASK_NBITS,
};

enum DN200_MDIO_MODE {
	DN200_MDIO_MODE_NONE = 0,
	DN200_MDIO_MODE_CL22,
	DN200_MDIO_MODE_CL45,
};

/* Link mode bit operations */
#define DN200_ZERO_SUP(_ls)		\
	ethtool_link_ksettings_zero_link_mode((_ls), supported)

#define DN200_SET_SUP(_ls, _mode)	\
	ethtool_link_ksettings_add_link_mode((_ls), supported, _mode)

#define DN200_CLR_SUP(_ls, _mode)	\
	ethtool_link_ksettings_del_link_mode((_ls), supported, _mode)

#define DN200_IS_SUP(_ls, _mode)	\
	ethtool_link_ksettings_test_link_mode((_ls), supported, _mode)

#define DN200_ZERO_ADV(_ls)		\
	ethtool_link_ksettings_zero_link_mode((_ls), advertising)

#define DN200_SET_ADV(_ls, _mode)	\
	ethtool_link_ksettings_add_link_mode((_ls), advertising, _mode)

#define DN200_CLR_ADV(_ls, _mode)	\
	ethtool_link_ksettings_del_link_mode((_ls), advertising, _mode)

#define DN200_ADV(_ls, _mode)		\
	ethtool_link_ksettings_test_link_mode((_ls), advertising, _mode)

#define DN200_ZERO_LP_ADV(_ls)		\
	ethtool_link_ksettings_zero_link_mode((_ls), lp_advertising)

#define DN200_SET_LP_ADV(_ls, _mode)	\
	ethtool_link_ksettings_add_link_mode((_ls), lp_advertising, _mode)

#define DN200_CLR_LP_ADV(_ls, _mode)	\
	ethtool_link_ksettings_del_link_mode((_ls), lp_advertising, _mode)

#define DN200_LP_ADV(_ls, _mode)		\
	ethtool_link_ksettings_test_link_mode((_ls), lp_advertising, _mode)

#define DN200_LM_COPY(_dst, _dname, _src, _sname)	\
	bitmap_copy((_dst)->link_modes._dname,		\
		    (_src)->link_modes._sname,		\
		    __ETHTOOL_LINK_MODE_MASK_NBITS)

static inline void dn200_linkmode_and(unsigned long *dst,
				      const unsigned long *a,
				      const unsigned long *b)
{
	bitmap_and(dst, a, b, __ETHTOOL_LINK_MODE_MASK_NBITS);
}

static inline int dn200_linkmode_test_bit(int nr,
					  const unsigned long *addr)
{
	return test_bit(nr, addr);
}

struct dn200_phy_info;

struct dn200_xpcs_info {
	int (*xpcs_read)(struct dn200_phy_info *phy_info, u32 addr, u32 devad,
			 u32 reg);
	int (*xpcs_write)(struct dn200_phy_info *phy_info, u32 addr, u32 devad,
			  u32 reg, u32 val);
	void __iomem *xpcs_regs_base;
};
#define DN200_MAX_PHY_DUMP_NUM (ARRAY_SIZE(xpcs_link_down_dump_regs))

/*reset sfp info per 2 seconds*/
#define DN200_SFP_RESET_TIME msecs_to_jiffies(3 * 1000)
#define DN200_KT_TRAIN_TIME msecs_to_jiffies(600)
struct dn200_phy_ops {
	int (*init)(struct dn200_phy_info *phy_info);
	int (*set_link)(struct dn200_phy_info *phy_info);
	int (*start)(struct dn200_phy_info *phy_info);
	int (*stop)(struct dn200_phy_info *phy_info);
	int (*reset)(struct dn200_phy_info *phy_info);
	int (*set_speeds)(struct dn200_phy_info *phy_info);
	void (*led_control)(struct dn200_phy_info *phy_info, bool enable);
	void (*blink_control)(struct dn200_phy_info *phy_info,
			      bool link_status);
	int (*link_status)(struct dn200_phy_info *phy_info);
	int (*media_type_get)(struct dn200_phy_info *phy_info);
	int (*identity)(struct dn200_phy_info *phy_info);
	int (*an_config)(struct dn200_phy_info *phy_info);

	int (*read_i2c_byte)(struct dn200_phy_info *phy_info, u8 byte_offset,
			     u8 dev_addr, u8 *data);
	int (*write_i2c_byte)(struct dn200_phy_info *phy_info, u8 byte_offset,
			      u8 dev_addr, u8 data);
	int (*read_i2c_eeprom)(struct dn200_phy_info *phy_info, u8 byte_offset,
			       u8 *eeprom_data);
	int (*read_i2c_sff8472)(struct dn200_phy_info *phy_info,
				u8 byte_offset, u8 *eeprom_data);
	int (*write_i2c_eeprom)(struct dn200_phy_info *phy_info,
				u8 byte_offset, u8 eeprom_data);
	void (*init_phy_timer)(struct dn200_phy_info *phy_info);
	void (*start_phy_timer)(struct dn200_phy_info *phy_info);
	void (*stop_phy_timer)(struct dn200_phy_info *phy_info);
	void (*phy_timer_del)(struct dn200_phy_info *phy_info);
	int (*set_link_ksettings)(struct net_device *netdev,
				  const struct ethtool_link_ksettings *cmd);
	int (*get_link_ksettings)(struct net_device *netdev,
				  struct ethtool_link_ksettings *cmd);
	int (*init_eee)(struct dn200_phy_info *phy_info, bool clk_stop_enable);
	int (*set_eee)(struct dn200_phy_info *phy_info,
		       struct ethtool_eee *data);
	int (*get_eee)(struct dn200_phy_info *phy_info,
		       struct ethtool_eee *data);
	int (*get_phy_pauseparam)(struct dn200_phy_info *phy_info,
				  struct ethtool_pauseparam *pause);
	int (*set_phy_pauseparam)(struct dn200_phy_info *phy_info,
				  struct ethtool_pauseparam *pause);
	int (*nway_reset)(struct dn200_phy_info *phy_info);
	int (*phy_loopback)(struct dn200_phy_info *phy_info, bool enable);
};

enum dn200_phy_state {
	DN200_PHY_EMPTY = 0,
	DN200_PHY_SFP_INITED = 1,
	DN200_PHY_SFP_NEED_RESET = 2,
	DN200_PHY_STARTED = 3,
	DN200_PHY_MULTISPEED_SETUP = 4,
	DN200_PHY_IN_SFP_INIT = 5,
	DN200_PHY_IN_RESET = 6,
	DN200_PHY_IN_TRAIN = 7,
};

#define DN200_MAX_BLK_ERR_CNT 3

struct dn200_phy_info {
	/*phy link modes */
	struct ethtool_link_ksettings lks;

	struct net_device *dev;
	/*phy ops */
	const struct dn200_phy_ops *phy_ops;
	const struct dn200_mac_ops *mac_ops;
	/*extern phy info */
	struct mii_bus *mii_bus;
	struct phy_device *phydev;
	/*inband xpcs phy info */
	struct dn200_xpcs_info *xpcs;
	struct dn200_gpio_data *gpio_data;
	unsigned long phy_state;

	/*common phy type */
	enum DN200_MDIO_MODE phydev_mode;
	enum dn200_media_type media_type;
	enum an_state an;
	enum an_state cur_an;
	bool an_sucess;
	enum rx_train_state rx_eq_states;
	enum tx_train_state tx_eq_states;
	unsigned long tr_timeout;
	int pause;
	u32 eee_broken_modes;
	phy_interface_t phy_interface;

	/*link status */
	u32 last_link_speed;	/*last link speed state <= setting_speed */
	u32 speed;		/*current used, <= setting_speed */
	u32 setting_speed;	/*set by user, <= max_speed */
	u32 max_speed;		/*set by pcie deviceid */
	bool self_adap_reset;
	unsigned long speed_reset_time;
	u8 port_type;
	enum dn200_duplex_info dup;
	enum dn200_link_status link_status;
	/*gpio base addr */
	void __iomem *gpio_base;

	u8 phy_addr;
	u8 xpcs_idx;
	/*phy sfp info */
	u8 sfp_type;
	u8 sfp_id;
	u8 sfp_module_type;

	u8 xpcs_sfp_valid:1;
	u8 sfp_has_gpio:1;
	u8 sfp_setup_needed:1;
	u8 sfp_rx_los:1;
	u8 sfp_tx_disable:1;
	u8 sfp_mod_absent:1;
	u8 sfp_tx_falut:1;
	u8 sfp_changed:1;
	u32 sfp_base;
	enum dn200_sfp_cable sfp_cable;
	u32 sfp_speed;
	bool multispeed_sfp;
	u8 blk_err_ck;
	u8 blk_err_cnt;
	/* Service routine support */
	struct workqueue_struct *dev_workqueue;
	struct work_struct phy_status_work;
	struct work_struct kr_train_work;
	struct delayed_work phy_multispeed_work;
	struct timer_list phy_status_timer;
	u32 phy_status_time_intr;
	bool phy_loopback_flag;
	bool mac_debug_active;
	bool recfg_an;
	u16 link_modes;
#define DN300_100BASET_Full BIT(0)
#define DN300_1000BASET_Full BIT(1)
#define DN300_1000BASEX_Full BIT(2)
#define DN300_10000baseSR_Full BIT(3)
#define DN300_10000baseLRM_Full BIT(4)
#define DN300_10000baseLR_Full BIT(5)
#define DN300_10000baseKR_Full BIT(6)
#define DN300_10000baseCR_Full BIT(7)
#define DN300_10000baseER_Full BIT(8)
};

struct dn200_mac_ops {
	void (*mac_link_down)(struct dn200_priv *config, unsigned int mode,
			      phy_interface_t interface);
	void (*mac_link_up)(struct dn200_priv *config,
			    struct phy_device *phy, unsigned int mode,
			    phy_interface_t interface, int speed, int duplex,
			    bool tx_pause, bool rx_pause);
	void (*mac_speed_set)(struct dn200_priv *priv,
			      phy_interface_t interface, int speed);
};

/*already link up, 500ms interval*/
#define DN200_PHY_STATUS_NINTR (500)
/*link down, 100ms interval*/
#define DN200_PHY_STATUS_DINTR (100)

#define DN200_ERR_BASE (0x100)

/* Error Codes */
#define DN200_ERR_EEPROM			-(DN200_ERR_BASE + 1)
#define DN200_ERR_EEPROM_CHECKSUM		-(DN200_ERR_BASE + 2)
#define DN200_ERR_PHY				-(DN200_ERR_BASE + 3)
#define DN200_ERR_CONFIG			-(DN200_ERR_BASE + 4)
#define DN200_ERR_PARAM				-(DN200_ERR_BASE + 5)
#define DN200_ERR_MAC_TYPE			-(DN200_ERR_BASE + 6)
#define DN200_ERR_UNKNOWN_PHY			-(DN200_ERR_BASE + 7)
#define DN200_ERR_LINK_SETUP			-(DN200_ERR_BASE + 8)
#define DN200_ERR_ADAPTER_STOPPED		-(DN200_ERR_BASE + 9)
#define DN200_ERR_INVALID_MAC_ADDR		-(DN200_ERR_BASE + 10)
#define DN200_ERR_DEVICE_NOT_SUPPORTED		-(DN200_ERR_BASE + 11)
#define DN200_ERR_MASTER_REQUESTS_PENDING	-(DN200_ERR_BASE + 12)
#define DN200_ERR_INVALID_LINK_SETTINGS		-(DN200_ERR_BASE + 13)
#define DN200_ERR_AUTONEG_NOT_COMPLETE		-(DN200_ERR_BASE + 14)
#define DN200_ERR_RESET_FAILED			-(DN200_ERR_BASE + 15)
#define DN200_ERR_SWFW_SYNC			-(DN200_ERR_BASE + 16)
#define DN200_ERR_PHY_ADDR_INVALID		-(DN200_ERR_BASE + 17)
#define DN200_ERR_I2C				-(DN200_ERR_BASE + 18)
#define DN200_ERR_SFP_NOT_SUPPORTED		-(DN200_ERR_BASE + 19)
#define DN200_ERR_SFP_NOT_PRESENT		-(DN200_ERR_BASE + 20)
#define DN200_ERR_SFP_NO_INIT_SEQ_PRESENT	-(DN200_ERR_BASE + 21)

int dn200_phy_info_init(struct net_device *dev,
			const struct dn200_mac_ops *mac_ops);
void dn200_hw_sideband_init(struct dn200_phy_info *phy_info);
int dn200_phy_info_remove(struct net_device *dev);
int dn200_phy_fec_enable(struct net_device *dev, bool enable);
int dn200_xpcs_config_eee(struct dn200_phy_info *phy_info, int mult_fact_100ns,
			  int enable);
irqreturn_t dn200_phy_status_isr(int irq, void *dev_id);
int dn200_phy_clock_stable_judge(struct dn200_phy_info *phy_info);
#define PRIV_PHY_OPS_CHECK(priv) \
	((priv)->plat_ex->phy_info ? !!(priv)->plat_ex->phy_info->phy_ops : 0)
#define PRIV_PHY_INFO(priv) ((priv)->plat_ex->phy_info)
#define PRIV_PHY_OPS(priv) ((priv)->plat_ex->phy_info->phy_ops)
#endif
