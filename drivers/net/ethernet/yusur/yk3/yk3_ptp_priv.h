/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _YK3_PTP_PRIV_H
#define _YK3_PTP_PRIV_H

/*
 * UMAC need know some values which pass from driver by control desc.
 * The follow define is the pozition. This is used on onestep mode.
 */
#define YK3_PTP_META_EN_BYTE		(22)
#define YK3_PTP_META_TS_EN		BIT(0)
#define YK3_PTP_META_OS_EN		BIT(1)
#define YK3_PTP_META_CSUM_EN		BIT(2)
#define YK3_PTP_REFID_BYTE		(23)
#define YK3_PTP_TOD_BYTE		(24)
#define YK3_PTP_CSUM_LSB_BYTE		(25)
#define YK3_PTP_CSUM_MSB_BYTE		(26)
#define YK3_PTP_BYTE_TO_DRV(byte)	((byte) - 18)
#define YK3_PTP_BYTE_TO_HW(byte)	((byte) - 16)

/*
 * PTP registers. The PTP registers base is `0xa0f000`.
 */
#define YK3_PTP_REG_BASE		(0xa0f000)
/* the version of ptp hardware */
#define YK3_PTP_VERSION			(YK3_PTP_REG_BASE)
/* the hardware frequency of mac tod */
#define YK3_PTP_MAC_FREQ		(YK3_PTP_REG_BASE + 0x4)
/* the hardware frequency of app tod */
#define YK3_PTP_APP_FREQ		(YK3_PTP_REG_BASE + 0x8)

/*
 * the global config register.
 * `YK3_PTP_GLB_REG` is the address of this register.
 * Others is the macro define for setting.
 */
#define YK3_PTP_GLB_REG			(YK3_PTP_REG_BASE + 0xc)

#define YK3_PTP_GLB_EN_P		(YK3_PTP_BYTE_TO_HW(YK3_PTP_META_EN_BYTE) & 0xf)
#define YK3_PTP_GLB_REFLID_P		((YK3_PTP_BYTE_TO_HW(YK3_PTP_REFID_BYTE) & 0xf) << 4)
#define YK3_PTP_GLB_TOD_P		((YK3_PTP_BYTE_TO_HW(YK3_PTP_TOD_BYTE) & 0xf) << 8)
#define YK3_PTP_GLB_CSUML_P		((YK3_PTP_BYTE_TO_HW(YK3_PTP_CSUM_LSB_BYTE) & 0xf) << 12)
#define YK3_PTP_GLB_CSUMM_P		((YK3_PTP_BYTE_TO_HW(YK3_PTP_CSUM_MSB_BYTE) & 0xf) << 16)

/*
 * The value is set to `YK3_PTP_GLB_REG`.
 * If the ptp channel is mac mode, the value is set to `YK3_PTP_GLB_MAC_MODE`.
 * If the ptp channel is app mode, the value is set to `YK3_PTP_GLB_APP_MODE`.
 */
#define YK3_PTP_GLB_MAC_MODE		(YK3_PTP_GLB_EN_P | YK3_PTP_GLB_REFLID_P | \
					 YK3_PTP_GLB_TOD_P | YK3_PTP_GLB_CSUML_P | \
					 YK3_PTP_GLB_CSUMM_P)
#define YK3_PTP_GLB_APP_MODE		(0x1043210)
/*
 * PTP channel control
 */
#define YK3_PTP_CH_REG(ch)		(YK3_PTP_REG_BASE + 0x10 + (ch) * 0x4)
#define YK3_PTP_CH_TS_EN		BIT(0)
#define YK3_PTP_CH_OS_EN		BIT(4)
#define YK3_PTP_CH_CSUM_EN		BIT(8)
#define YK3_PTP_CH_FLAG_EN		BIT(12)
#define YK3_PTP_CH_REFL_EN		BIT(16)
#define YK3_PTP_CH_LEGACY_EN		BIT(20)
#define YK3_PTP_CH_RX_TOD_EN		BIT(24)
#define YK3_PTP_CH_RX_APP_EN		BIT(28)
#define YK3_PTP_CH_IDX(idx)		(((idx) & 0x3) << 30)

/*
 * PTP channel tod registers.
 */
#define YK3_PTP_TOD_BASE(idx)		(YK3_PTP_REG_BASE + 0x50 + (idx) * 0x30)
/* tod timestamp modify register */
#define YK3_PTP_TOD_CTRL		(0x0)
#define YK3_PTP_TOD_CTRL_INIT		BIT(0)
#define YK3_PTP_TOD_CTRL_UPDATE		BIT(4)
#define YK3_PTP_TOD_CTRL_FIND_CORSE	BIT(8)
#define YK3_PTP_TOD_CTRL_TS_EN		BIT(12)
#define YK3_PTP_TOD_CTRL_ROLLOVER	BIT(16)
#define YK3_PTP_TOD_CFG_L		(0x4)
#define YK3_PTP_TOD_CFG_M		(0x8)
#define YK3_PTP_TOD_CFG_H		(0xc)
/* */
#define YK3_PTP_TOD_SS_INCR		(0x10)
#define YK3_PTP_TOD_ADDEND		(0x14)
/* tod realtime timestamp */
#define YK3_PTP_TOD_RT_L		(0x18)
#define YK3_PTP_TOD_RT_M		(0x1c)
#define YK3_PTP_TOD_RT_H		(0x20)
/* tod reflect timestamp */
#define YK3_PTP_TOD_REFL_L		(0x24)
#define YK3_PTP_TOD_REFL_M		(0x28)
/* tod reflect timestamp control. such as clear or valid */
#define YK3_PTP_TOD_REFL_CTRL		(0x2c)
#define YK3_PTP_TOD_REFL_CTRL_CLR	BIT(16)
#define YK3_PTP_TOD_REFL_CTRL_VLD	BIT(20)
#define YK3_PTP_REFL_VLD(addr)		(yk3_rd32(addr, YK3_PTP_TOD_REFL_CTRL) & BIT(20))

struct pci_dev;

struct yk3_ptp {
	u8 idx;
	struct pci_dev *pdev;

#define YK3_PTP_TOD_APP 0
#define YK3_PTP_TOD_MAC 1
	u8 tod_mode;

#define YK3_PTP_TWOSTEP 0
#define YK3_PTP_ONESTEP 1
	u8 ptp_mode;

	u64 hw_freq;
	u64 req_freq;
	u32 default_addend;

	bool rx_hw_tstamp;
	bool tx_hw_tstamp;
	struct hwtstamp_config hwts_config;

	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_caps;

	void __iomem *glb_ctrl;
	void __iomem *ch_ctrl;
	void __iomem *tod_ctrl;
};

#endif /* _YK3_PTP_PRIV_H */
