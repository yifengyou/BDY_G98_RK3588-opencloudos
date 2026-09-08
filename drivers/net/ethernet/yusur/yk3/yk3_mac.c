// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"

static const char yk3_mac_stats_strings[][ETH_GSTRING_LEN] = {
	"tx_crc_error_packets_phy",
	"tx_less_than_64_bytes_phy",
	"tx_equal_to_64_bytes_phy",
	"tx_65_to_127_bytes_phy",
	"tx_128_to_255_bytes_phy",
	"tx_256_to_511_bytes_phy",
	"tx_512_to_1023_bytes_phy",
	"tx_1024_to_1518_bytes_phy",
	"tx_1519_to_2047_bytes_phy",
	"tx_2048_to_4095_bytes_phy",
	"tx_4096_to_8191_bytes_phy",
	"tx_8192_to_9215_bytes_phy",
	"tx_greater_than_9216_bytes_phy",
	"tx_pause_phy",
	"tx_pripause_phy",
	"tx_prio_0_pause_phy",
	"tx_prio_1_pause_phy",
	"tx_prio_2_pause_phy",
	"tx_prio_3_pause_phy",
	"tx_prio_4_pause_phy",
	"tx_prio_5_pause_phy",
	"tx_prio_6_pause_phy",
	"tx_prio_7_pause_phy",

	"rx_crc_error_packets_phy",
	"rx_less_than_64_bytes_phy",
	"rx_equal_to_64_bytes_phy",
	"rx_65_to_127_bytes_phy",
	"rx_128_to_255_bytes_phy",
	"rx_256_to_511_bytes_phy",
	"rx_512_to_1023_bytes_phy",
	"rx_1024_to_1518_bytes_phy",
	"rx_1519_to_2047_bytes_phy",
	"rx_2048_to_4095_bytes_phy",
	"rx_4096_to_8191_bytes_phy",
	"rx_8192_to_9215_bytes_phy",
	"rx_greater_than_9216_bytes_phy",
	"rx_pause_phy",
	"rx_pripause_phy",
	"rx_prio_0_pause_phy",
	"rx_prio_1_pause_phy",
	"rx_prio_2_pause_phy",
	"rx_prio_3_pause_phy",
	"rx_prio_4_pause_phy",
	"rx_prio_5_pause_phy",
	"rx_prio_6_pause_phy",
	"rx_prio_7_pause_phy",

	"tx_discard_phy",
	"rx_discard_phy",
};

void yk3_mac_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data)
{
	struct yk3_pdev_priv *pdev_priv;
	void __iomem *addr;
	enum yk3_mac_channel mac_ch;
	struct yk3_mac *mac;
	u64 val;

	if (!yk3_ndev_is_pf(ndev_priv))
		return;

	pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	mac = pdev_priv->mac;

	if (!mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return;
	}

	addr = pdev_priv->bar_addr[YK3_BAR0];
	mac_ch = mac->mac_ch;

	val = yk3_rd64(addr, UMAC_FRAMES_XMIT_OK(0, mac_ch));
	if (val != UMAC_STAT_ERROR) {
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_ERROR(0, mac_ch));
		data[MAC_FRAMES_XMIT_ERROR] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_ERROR] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZELT64(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZELT64] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZELT64] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZEEQ64(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZEEQ64] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZEEQ64] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE65TO127(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE65TO127] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE65TO127] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE128TO255(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE128TO255] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE128TO255] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE256TO511(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE256TO511] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE256TO511] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE512TO1023(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE512TO1023] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE512TO1023] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE1024TO1518(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE1024TO1518] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE1024TO1518] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE1519TO2047(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE1519TO2047] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE1519TO2047] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE2048TO4095(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE2048TO4095] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE2048TO4095] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZE4096TO8191(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE4096TO8191] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE4096TO8191] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_SMIT_SIZE8192TO9215(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZE8192TO9215] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZE8192TO9215] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_SIZEGT9216(0, mac_ch));
		data[MAC_FRAMES_XMIT_SIZEGT9216] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_SIZEGT9216] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PAUSE(0, mac_ch));
		data[MAC_FRAMES_XMIT_PAUSE] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PAUSE] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRIPAUSE(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRIPAUSE] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRIPAUSE] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI0(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI0] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI0] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI1(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI1] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI1] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI2(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI2] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI2] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI3(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI3] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI3] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI4(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI4] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI4] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI5(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI5] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI5] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI6(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI6] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI6] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_XMIT_PRI7(0, mac_ch));
		data[MAC_FRAMES_XMIT_PRI7] = val;
		mac->phy_stat[MAC_FRAMES_XMIT_PRI7] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_CRCERROR(0, mac_ch));
		data[MAC_FRAMES_RCVD_CRCERROR] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_CRCERROR] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZELT64(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZELT64] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZELT64] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZEEQ64(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZEEQ64] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZEEQ64] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE65TO127(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE65TO127] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE65TO127] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE128TO255(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE128TO255] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE128TO255] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE256TO511(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE256TO511] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE256TO511] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE512TO1023(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE512TO1023] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE512TO1023] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE1024TO1518(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE1024TO1518] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE1024TO1518] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE1419TO2047(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE1419TO2047] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE1419TO2047] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE2048TO4095(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE2048TO4095] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE2048TO4095] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE4096TO8191(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE4096TO8191] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE4096TO8191] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZE8192TO9215(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZE8192TO9215] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZE8192TO9215] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_SIZEGT9216(0, mac_ch));
		data[MAC_FRAMES_RCVD_SIZEGT9216] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_SIZEGT9216] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PAUSE(0, mac_ch));
		data[MAC_FRAMES_RCVD_PAUSE] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PAUSE] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRIPAUSE(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRIPAUSE] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRIPAUSE] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI0(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI0] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI0] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI1(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI1] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI1] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI2(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI2] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI2] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI3(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI3] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI3] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI4(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI4] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI4] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI5(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI5] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI5] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI6(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI6] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI6] += val;
		val = yk3_rd64(addr, UMAC_FRAMES_RCVD_PRI7(0, mac_ch));
		data[MAC_FRAMES_RCVD_PRI7] = val;
		mac->phy_stat[MAC_FRAMES_RCVD_PRI7] += val;
	} else {
		/* hw error, clear and restart, debug stat */
		val = yk3_rd64(addr, UMAC_PHY_STAT_SET_REG(0, mac_ch));
		val |= FIELD_PREP(UMAC_PHY_STAT_SET_MASK, 1);
		yk3_wr64(addr, UMAC_PHY_STAT_SET_REG(0, mac_ch), val);
		val &= ~UMAC_PHY_STAT_SET_MASK;
		yk3_wr64(addr, UMAC_PHY_STAT_SET_REG(0, mac_ch), val);
		mac->phy_stat_err++;
		data[MAC_FRAMES_XMIT_ERROR] = mac->phy_stat[MAC_FRAMES_XMIT_ERROR];
		data[MAC_FRAMES_XMIT_SIZELT64] = mac->phy_stat[MAC_FRAMES_XMIT_SIZELT64];
		data[MAC_FRAMES_XMIT_SIZEEQ64] = mac->phy_stat[MAC_FRAMES_XMIT_SIZEEQ64];
		data[MAC_FRAMES_XMIT_SIZE65TO127] = mac->phy_stat[MAC_FRAMES_XMIT_SIZE65TO127];
		data[MAC_FRAMES_XMIT_SIZE128TO255] = mac->phy_stat[MAC_FRAMES_XMIT_SIZE128TO255];
		data[MAC_FRAMES_XMIT_SIZE256TO511] = mac->phy_stat[MAC_FRAMES_XMIT_SIZE256TO511];
		data[MAC_FRAMES_XMIT_SIZE512TO1023] = mac->phy_stat[MAC_FRAMES_XMIT_SIZE512TO1023];
		data[MAC_FRAMES_XMIT_SIZE1024TO1518] =
			mac->phy_stat[MAC_FRAMES_XMIT_SIZE1024TO1518];
		data[MAC_FRAMES_XMIT_SIZE1519TO2047] =
			mac->phy_stat[MAC_FRAMES_XMIT_SIZE1519TO2047];
		data[MAC_FRAMES_XMIT_SIZE2048TO4095] =
			mac->phy_stat[MAC_FRAMES_XMIT_SIZE2048TO4095];
		data[MAC_FRAMES_XMIT_SIZE4096TO8191] =
			mac->phy_stat[MAC_FRAMES_XMIT_SIZE4096TO8191];
		data[MAC_FRAMES_XMIT_SIZE8192TO9215] =
			mac->phy_stat[MAC_FRAMES_XMIT_SIZE8192TO9215];
		data[MAC_FRAMES_XMIT_SIZEGT9216] = mac->phy_stat[MAC_FRAMES_XMIT_SIZEGT9216];
		data[MAC_FRAMES_XMIT_PAUSE] = mac->phy_stat[MAC_FRAMES_XMIT_PAUSE];
		data[MAC_FRAMES_XMIT_PRIPAUSE] = mac->phy_stat[MAC_FRAMES_XMIT_PRIPAUSE];
		data[MAC_FRAMES_XMIT_PRI0] = mac->phy_stat[MAC_FRAMES_XMIT_PRI0];
		data[MAC_FRAMES_XMIT_PRI1] = mac->phy_stat[MAC_FRAMES_XMIT_PRI1];
		data[MAC_FRAMES_XMIT_PRI2] = mac->phy_stat[MAC_FRAMES_XMIT_PRI2];
		data[MAC_FRAMES_XMIT_PRI3] = mac->phy_stat[MAC_FRAMES_XMIT_PRI3];
		data[MAC_FRAMES_XMIT_PRI4] = mac->phy_stat[MAC_FRAMES_XMIT_PRI4];
		data[MAC_FRAMES_XMIT_PRI5] = mac->phy_stat[MAC_FRAMES_XMIT_PRI5];
		data[MAC_FRAMES_XMIT_PRI6] = mac->phy_stat[MAC_FRAMES_XMIT_PRI6];
		data[MAC_FRAMES_XMIT_PRI7] = mac->phy_stat[MAC_FRAMES_XMIT_PRI7];
		data[MAC_FRAMES_RCVD_CRCERROR] = mac->phy_stat[MAC_FRAMES_RCVD_CRCERROR];
		data[MAC_FRAMES_RCVD_SIZELT64] = mac->phy_stat[MAC_FRAMES_RCVD_SIZELT64];
		data[MAC_FRAMES_RCVD_SIZEEQ64] = mac->phy_stat[MAC_FRAMES_RCVD_SIZEEQ64];
		data[MAC_FRAMES_RCVD_SIZE65TO127] = mac->phy_stat[MAC_FRAMES_RCVD_SIZE65TO127];
		data[MAC_FRAMES_RCVD_SIZE128TO255] = mac->phy_stat[MAC_FRAMES_RCVD_SIZE128TO255];
		data[MAC_FRAMES_RCVD_SIZE256TO511] = mac->phy_stat[MAC_FRAMES_RCVD_SIZE256TO511];
		data[MAC_FRAMES_RCVD_SIZE512TO1023] = mac->phy_stat[MAC_FRAMES_RCVD_SIZE512TO1023];
		data[MAC_FRAMES_RCVD_SIZE1024TO1518] =
			mac->phy_stat[MAC_FRAMES_RCVD_SIZE1024TO1518];
		data[MAC_FRAMES_RCVD_SIZE1419TO2047] =
			mac->phy_stat[MAC_FRAMES_RCVD_SIZE1419TO2047];
		data[MAC_FRAMES_RCVD_SIZE2048TO4095] =
			mac->phy_stat[MAC_FRAMES_RCVD_SIZE2048TO4095];
		data[MAC_FRAMES_RCVD_SIZE4096TO8191] =
			mac->phy_stat[MAC_FRAMES_RCVD_SIZE4096TO8191];
		data[MAC_FRAMES_RCVD_SIZE8192TO9215] =
			mac->phy_stat[MAC_FRAMES_RCVD_SIZE8192TO9215];
		data[MAC_FRAMES_RCVD_SIZEGT9216] = mac->phy_stat[MAC_FRAMES_RCVD_SIZEGT9216];
		data[MAC_FRAMES_RCVD_PAUSE] = mac->phy_stat[MAC_FRAMES_RCVD_PAUSE];
		data[MAC_FRAMES_RCVD_PRIPAUSE] = mac->phy_stat[MAC_FRAMES_RCVD_PRIPAUSE];
		data[MAC_FRAMES_RCVD_PRI0] = mac->phy_stat[MAC_FRAMES_RCVD_PRI0];
		data[MAC_FRAMES_RCVD_PRI1] = mac->phy_stat[MAC_FRAMES_RCVD_PRI1];
		data[MAC_FRAMES_RCVD_PRI2] = mac->phy_stat[MAC_FRAMES_RCVD_PRI2];
		data[MAC_FRAMES_RCVD_PRI3] = mac->phy_stat[MAC_FRAMES_RCVD_PRI3];
		data[MAC_FRAMES_RCVD_PRI4] = mac->phy_stat[MAC_FRAMES_RCVD_PRI3];
		data[MAC_FRAMES_RCVD_PRI5] = mac->phy_stat[MAC_FRAMES_RCVD_PRI5];
		data[MAC_FRAMES_RCVD_PRI6] = mac->phy_stat[MAC_FRAMES_RCVD_PRI6];
		data[MAC_FRAMES_RCVD_PRI7] = mac->phy_stat[MAC_FRAMES_RCVD_PRI7];
	}

	/* Generate a dummy counter. Simulate discarding packet by phy layer */
	data[MAC_DUMMY_TX_DISCARD_PHY] = 0;
	data[MAC_DUMMY_RX_DISCARD_PHY] = 0;
}

void yk3_mac_et_get_stats_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail)
{
	u8 *cur = data;
	int i;

	if (!yk3_ndev_is_pf(ndev_priv))
		return;

	for (i = 0; i < ARRAY_SIZE(yk3_mac_stats_strings); i++) {
		memcpy(cur, yk3_mac_stats_strings[i], ETH_GSTRING_LEN);
		cur += ETH_GSTRING_LEN;
	}

	*tail = cur;
}

int yk3_mac_et_get_sset_count(struct yk3_ndev_priv *ndev_priv)
{
	if (!yk3_ndev_is_pf(ndev_priv))
		return 0;

	return ARRAY_SIZE(yk3_mac_stats_strings);
}

inline u32 yk3_mac_get_status_reg(void __iomem *hw_addr,
					   u64 hw_bits, u8 macx, u8 mac_ch)
{
	if (hw_bits & YK3_HW_BIT_UMAC)
		return yk3_rd32(hw_addr, UMACX_CHY_STATUS(0, mac_ch));
	else
		return yk3_rd32(hw_addr, XMACX_CHY_STATUS(0, mac_ch));
}

inline void yk3_mac_set_ndev_status(u64 hw_bits, struct yk3_ndev_priv *ndev_priv,
					     u32 status_reg_val)
{
	if (hw_bits & YK3_HW_BIT_UMAC)
		if ((status_reg_val & UMAC_STATUS_MASK) == UMAC_STATUS_EN) {
			netif_carrier_on(ndev_priv->ndev);
			ndev_priv->link_status = 1;
		} else {
			netif_carrier_off(ndev_priv->ndev);
			ndev_priv->link_status = 0;
		}
	else
		if ((status_reg_val & XMAC_STATUS_MASK) == XMAC_STATUS_EN) {
			netif_carrier_on(ndev_priv->ndev);
			ndev_priv->link_status = 1;
		} else {
			netif_carrier_off(ndev_priv->ndev);
			ndev_priv->link_status = 0;
		}
}

u32 yk3_mac_get_speed(struct yk3_pdev_priv *pdev_priv, u32 *speed)
{
	struct yk3_mac *mac = pdev_priv->mac;
	enum yk3_mac_channel mac_ch;
	void __iomem *addr;
	u32 rt_speed;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!mac)
		return -EIO;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC) {
		*speed = SPEED_10G;
		return 0;
	}

	addr = pdev_priv->bar_addr[YK3_BAR0];
	mac_ch = mac->mac_ch;

	rt_speed = yk3_rd32(addr, UMAC_CHMODE_L(0, mac_ch)) & CHMODE_MODE_MASK;
	switch (rt_speed) {
	case MAC_MODE_SPEED_10GBASE:
	case MAC_MODE_SPEED_10GBASE_FC:
		*speed = SPEED_10G;
		break;
	case MAC_MODE_SPEED_25GBASE:
	case MAC_MODE_SPEED_25GBASE_FC:
	case MAC_MODE_SPEED_25GBASE_RS_IEEE:
	case MAC_MODE_SPEED_25GBASE_RS_CONS:
		*speed = SPEED_25G;
		break;
	case MAC_MODE_SPEED_40GBASE:
	case MAC_MODE_SPEED_40GBASE_FC:
		*speed = SPEED_40G;
		break;
	case MAC_MODE_SPEED_100GBASE:
	case MAC_MODE_SPEED_100GBASE_RS:
		*speed = SPEED_100G;
		break;
	default:
		*speed = SPEED_UNKNOWN;
	}

	return 0;
}

void yk3_np_tm_speed_limit(struct yk3_pdev_priv *pdev_priv, u32 speed)
{
	u32 cycles = 400;
	u32 bytes = 0;
	u8 lane;
	void __iomem *addr;

	if (!pdev_priv || !pdev_priv->mac)
		return;

	lane = pdev_priv->mac->mac_ch;
	addr = pdev_priv->bar_addr[YK3_BAR0];

	switch (speed) {
	case SPEED_10G:
		bytes = 1280;
		break;
	case SPEED_25G:
		bytes = 3200;
		break;
	case SPEED_40G:
		bytes = 5120;
		break;
	case SPEED_100G:
		bytes = 12800;
		break;
	default:
		bytes = 3200;
		break;
	}
	yk3_wr32(addr, NP_TM_QOS_TK_WD(lane), cycles);
	yk3_wr32(addr, NP_TM_QOS_TK_BYTES(lane), bytes);
	yk3_wr32(addr, NP_TM_QOS_TK_BS(lane), 12000);
	yk3_wr32(addr, NP_TM_QOS_ENABLE(lane), 0);
	yk3_wr32(addr, NP_TM_QOS_ENABLE(lane), 1);
	yk3_wr32(addr, NP_TM_QOS_ENABLE(lane), 0);
	yk3_wr32(addr, NP_TM_QOS_ENABLE(lane), 1);
}

void yk3_edma_l2_vq(struct yk3_pdev_priv *pdev_priv, u32 speed)
{
	void __iomem *hw_addr;
	u32 l2xon, l2xoff;

	if (!pdev_priv)
		return;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];

	switch (speed) {
	case SPEED_10G:
		l2xoff = 0x1000;
		l2xon = 0x500;
		break;
	case SPEED_25G:
		l2xoff = 0x2000;
		l2xon = 0x1000;
		break;
	case SPEED_40G:
		l2xoff = 0x2000;
		l2xon = 0x1000;
		break;
	case SPEED_100G:
		l2xoff = 0x2000;
		l2xon = 0x1000;
		break;
	default:
		l2xoff = 0x2000;
		l2xon = 0x1000;
		break;
	}

	/* pcie l1xoff, l1xon, enable */
	yk3_wr32(hw_addr, 0x13e300c, l2xoff); // l2 xoff, must config
	yk3_wr32(hw_addr, 0x13e3008, l2xon); // l2 xon, must config
	yk3_wr32(hw_addr, 0x014b0680, l2xoff + 0x90); // l2 overflow, must config
}

static void yk3_set_hqos_bp(struct yk3_pdev_priv *pdev_priv)
{
	void __iomem *hw_addr;
	u8 i, j;
	u32 val;

	if (!pdev_priv)
		return;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];

	for (i = 0; i < 4; i++)
		for (j = 0; j < 8; j++)
			yk3_wr32(hw_addr, 0x13c3100 + 0x40 * i + 0x100 * j, 1);

	/* pcie l1 overflow, must config */
	for (i = 0; i < 32; i++)
		yk3_wr32(hw_addr, 0x14b0300 + 4 * i, 0x290);

	val = yk3_rd32(hw_addr, 0x1308014); // read ingress1 pb val, from np microcode
	yk3_wr32(hw_addr, 0x14b0e08, val); // set vq pb using, must same to ingress1 pb val
	yk3_wr32(hw_addr, 0x14b0640, 0x1500); // hqos cfg space
	yk3_wr32(hw_addr, 0x14b0484, 0); // disable l2 map, slice
	yk3_wr32(hw_addr, 0x13e3000, 0); // l2 disable, signal
	yk3_wr32(hw_addr, 0x1465000, 1); // enable, signal
	yk3_wr32(hw_addr, 0x14b0e00, 1); // enable, report slice
	yk3_wr32(hw_addr, 0x230, 0xffffffff); // edma bp enable np slice
	yk3_wr32(hw_addr, 0x234, 0x0); // edma bp enable np signal

	yk3_wr32(hw_addr, 0x4e4, 1);
	yk3_wr32(hw_addr, 0x4e0, 0x2000);
	yk3_wr32(hw_addr, 0x800190, 0);
	yk3_wr32(hw_addr, 0x8070f4, 1);
	for (i = 0; i < 32; i++)
		yk3_wr32(hw_addr, 0x80717c + i * 4, 0xa0);

#ifdef CONFIG_X86
	if (strncmp(boot_cpu_data.x86_vendor_id, "HygonGenuine", 12) == 0) {
		if (strstr(boot_cpu_data.x86_model_id, "C86-4G") ||
		    strstr(boot_cpu_data.x86_model_id, "74"))
			for (i = 0; i < 32; i++)
				yk3_wr32(hw_addr, 0x80717c + i * 4, 0x68);
	}
#endif
}

u32 yk3_mac_set_link_speed(struct yk3_ndev_priv *ndev_priv, u32 speed)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	u8 speed_type;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = pdev_priv->mac->mac_ch;

	switch (speed) {
	case SPEED_10G:
		speed_type = YK3_10G;
		break;
	case SPEED_25G:
		speed_type = YK3_25G;
		break;
	case SPEED_40G:
		speed_type = YK3_40G;
		break;
	case SPEED_100G:
		speed_type = YK3_100G;
		break;
	default:
		yk3_net_err("unsupported speed %u\n", speed);
		return -EOPNOTSUPP;
	}

	yk3_net_debug("set mac_ch %u speed_type %u speed %u\n", mac_ch, speed_type, speed);

	msg.opcode = YK3_MBOX_OPCODE_EMP_SET_SPEED;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data[1] = speed_type;
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
	return 0;
}

u32 yk3_mac_set_link_autoneg(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = pdev_priv->mac->mac_ch;

	yk3_net_debug("set mac_ch %u autoneg\n", mac_ch);

	msg.opcode = YK3_MBOX_OPCODE_EMP_SET_SPEED;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data[1] = YK3_AUTONEG;
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
	return 0;
}

u32 yk3_mac_set_ifg(struct yk3_ndev_priv *ndev_priv, u8 ifg_num)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (yk3_pdev_is_pf(pdev_priv)) {
		if (!pdev_priv->mac) {
			yk3_dev_err("%s pdev_priv mac null\n", __func__);
			return -EIO;
		}

		mac_ch = pdev_priv->mac->mac_ch;

		yk3_net_debug("set mac_ch %u ifg_num %u\n", mac_ch, ifg_num);

		msg.opcode = YK3_MBOX_OPCODE_EMP_SET_IFG;
		msg.dst_id = yk3_mbox_emp_id();
		msg.data[0] = mac_ch;
		msg.data[1] = ifg_num;
		msg.data_length = 2;
		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;
		if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
			yk3_dev_err("send mbox msg err\n");
			return -EIO;
		}
		if (ack_msg.data[0]) {
			yk3_dev_err("mbox resp result 0x%02x reason 0x%02x\n",
				    ack_msg.data[0], ack_msg.data[1]);
			return -EIO;
		}
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		msg.opcode = YK3_MBOX_OPCODE_PF_SET_IFG;
		msg.dst_id = yk3_mbox_pf_id(0);
		msg.data[0] = ifg_num;
		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;
		if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
			yk3_dev_err("send mbox msg err\n");
			return -EIO;
		}
		if (ack_msg.data[0]) {
			yk3_dev_err("pf set ifg err, ret 0x%02x\n", ack_msg.data[0]);
			return -EIO;
		}
	}
	return 0;
}

static void yk3_mac_mbox_pf_set_ifg(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_mac *mac;
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg2 = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_msg ack_msg2 = {0};
	struct yk3_mbox_option opt = {0};
	u8 ifg_num = msg->data[0];
	u8 ack_msg2_ret = 0;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	mac = pdev_priv->mac;
	if (!mac) {
		yk3_dev_err("%s pdev_priv mac null\n", __func__);
		return;
	}

	mac_ch = mac->mac_ch;
	yk3_dev_debug("set mac_ch %u ifg_num %u\n", mac_ch, ifg_num);

	msg2.opcode = YK3_MBOX_OPCODE_EMP_SET_IFG;
	msg2.dst_id = yk3_mbox_emp_id();
	msg2.data[0] = mac_ch;
	msg2.data[1] = ifg_num;
	msg2.data_length = 2;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg2, &opt, &ack_msg)) {
		yk3_dev_err("send mbox msg err\n");
		ack_msg2_ret = 1;
	}
	if (ack_msg.data[0]) {
		yk3_dev_err("mbox resp result 0x%02x reason 0x%02x\n",
			    ack_msg.data[0], ack_msg.data[1]);
		ack_msg2_ret = 2;
	}

	ack_msg2.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg2.seqno = msg->seqno;
	ack_msg2.dst_id = msg->src_id;
	ack_msg2.data[0] = ack_msg2_ret;
	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	yk3_mbox_send_msg(pdev_priv, &ack_msg2, &opt, NULL);
}

int yk3_mac_get_connector(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mac *mac;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	enum yk3_mac_channel mac_ch;
	u32 offset = 0;
	u8 req_len = 8, response_len;
	u16 remain_len;
	u8 connector_type;

	mac = pdev_priv->mac;
	if (!mac) {
		yk3_dev_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}
	mac_ch = mac->mac_ch;
	msg.opcode = YK3_MBOX_OPCODE_EMP_GET_EERPOM;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 4;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	msg.data[1] = req_len;
	put_unaligned_le16(offset, &msg.data[2]);
	memset(&ack_msg, 0, sizeof(ack_msg));
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_dev_err("work get module eeprom, emp response error\n");
		mac->port = PORT_NONE;
		return -EIO;
	}
	response_len = ack_msg.data[1];
	remain_len = le16_to_cpu(get_unaligned((__le16 *)&ack_msg.data[2]));
	if (response_len > req_len) {
		yk3_dev_err("work get module eeprom, exceeds, req_len %u response_len %u\n",
			    req_len, response_len);
		mac->port = PORT_NONE;
		return -EIO;
	}

	yk3_dev_debug("work get module eeprom, req_len %u response_len %u remain_len %u\n",
		      req_len, response_len, remain_len);

	connector_type = ack_msg.data[4 + 2];
	if (connector_type == 0x07)
		mac->port = PORT_FIBRE;
	else if (connector_type >= 0x20 && connector_type <= 0x23)
		mac->port = PORT_DA;
	else
		mac->port = PORT_NONE;
	return 0;
}

static int yk3_mac_get_sts_regval(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mac *mac;
	enum yk3_mac_channel mac_ch;
	u8 i = 3;

	mac = pdev_priv->mac;
	if (!mac) {
		yk3_dev_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = mac->mac_ch;
	switch (mac_ch) {
	case 0 ... 3:
		while (i--)
			mac->sts_regval = yk3_mac_get_status_reg(pdev_priv->bar_addr[YK3_BAR0],
								 pdev_priv->card->hw_bits,
								 0,
								 mac_ch);
		break;
	default:
		yk3_dev_err("%s unknown mac_ch %d\n", __func__, mac_ch);
		return -EIO;
	}

	return 0;
}

static void __maybe_unused yk3_link_timer_callback(struct timer_list *link_timer)
{
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_mac *mac = container_of(link_timer, struct yk3_mac, link_timer);
	struct yk3_mbox_msg msg = {0};
	enum yk3_mac_channel mac_ch = mac->mac_ch;
	int exist_vfs;
	u8 i;

	pdev_priv = pci_get_drvdata(mac->pdev);
	if (!pdev_priv)
		return;

	if (yk3_mac_get_sts_regval(pdev_priv)) {
		yk3_dev_err("%s get sts_regval err, pf %u\n", __func__, pdev_priv->pf_id);
		return;
	}

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv) {
		yk3_dev_err("%s pf %u no ndev\n", __func__, pdev_priv->pf_id);
		return;
	}

	yk3_mac_set_ndev_status(pdev_priv->card->hw_bits, ndev_priv, mac->sts_regval);

	if (yk3_mac_get_speed(pdev_priv, &mac->speed)) {
		yk3_dev_err("%s pf %u get link speed err, mac_ch %u\n",
			    __func__, pdev_priv->pf_id, mac_ch);
		return;
	}

	ndev_priv->link_speed = mac->speed;
	if (ndev_priv->link_speed != SPEED_UNKNOWN && ndev_priv->link_status) {
		yk3_np_tm_speed_limit(pdev_priv, ndev_priv->link_speed);
		yk3_qos_set_link_speed(pdev_priv, ndev_priv->link_speed);
		yk3_edma_l2_vq(pdev_priv, ndev_priv->link_speed);
	}

	exist_vfs = pci_num_vf(pdev_priv->pdev);
	for (i = 0; i < exist_vfs; i++) {
		msg.dst_id = yk3_mbox_vf_id(i);
		msg.opcode = YK3_MBOX_OPCODE_SET_VF_MAC_INFO;
		*(u32 *)&msg.data = mac->sts_regval;
		*(u32 *)&msg.data[4] = mac->speed;
		yk3_mbox_send_msg_atomic(pdev_priv, &msg);
	}
}

void yk3_mac_ndev_start(struct yk3_ndev_priv *ndev_priv)
{
	if (ndev_priv->link_status)
		netif_carrier_on(ndev_priv->ndev);
	else
		netif_carrier_off(ndev_priv->ndev);
}

static void yk3_mac_work(struct work_struct *work)
{
	struct yk3_mac *mac = container_of(work, struct yk3_mac, work);
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_ndev_priv *ndev_priv;
	enum yk3_mac_channel mac_ch = mac->mac_ch;
	struct yk3_mbox_msg msg = {0};
	int exist_vfs;
	u8 i;

	mac->link_int_cnt++;
	if (mac_ch >= YK3_MAC_CH_MAX)
		return;

	pdev_priv = pci_get_drvdata(mac->pdev);
	if (!pdev_priv)
		return;

	if (yk3_mac_get_sts_regval(pdev_priv)) {
		yk3_dev_err("%s get sts_regval err, pf %u\n", __func__, pdev_priv->pf_id);
		return;
	}

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv) {
		yk3_dev_err("%s pf %u no ndev\n", __func__, pdev_priv->pf_id);
		return;
	}

	if (netif_carrier_ok(ndev_priv->ndev)) {
		yk3_mac_set_ndev_status(pdev_priv->card->hw_bits, ndev_priv, mac->sts_regval);

		if (yk3_mac_get_speed(pdev_priv, &mac->speed)) {
			yk3_dev_err("%s pf %u get link speed err, mac_ch %u\n",
				    __func__, pdev_priv->pf_id, mac_ch);
			return;
		}

		ndev_priv->link_speed = mac->speed;
		exist_vfs = pci_num_vf(pdev_priv->pdev);
		for (i = 0; i < exist_vfs; i++) {
			msg.dst_id = yk3_mbox_vf_id(i);
			msg.opcode = YK3_MBOX_OPCODE_SET_VF_MAC_INFO;
			*(u32 *)&msg.data = mac->sts_regval;
			*(u32 *)&msg.data[4] = mac->speed;
			yk3_mbox_send_msg_atomic(pdev_priv, &msg);
		}
	} else {
		mod_timer(&mac->link_timer, jiffies + msecs_to_jiffies(2000));
	}
}

static irqreturn_t yk3_mac_intr(int vector, void *data)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)data;

	schedule_work(&pdev_priv->mac->work);
	return IRQ_HANDLED;
}

static void yk3_mac_mbox_pf_set_vf_mac_info(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_ndev_priv *ndev_priv = (struct yk3_ndev_priv *)param;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);

	yk3_mac_set_ndev_status(pdev_priv->card->hw_bits,
				ndev_priv, *(u32 *)&msg->data);
	ndev_priv->link_speed = *(u32 *)&msg->data[4];
}

static void yk3_mac_mbox_vf_get_pf_mac_info(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_mac *mac;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	mac = pdev_priv->mac;
	if (!mac) {
		yk3_dev_err("%s pdev_priv mac null\n", __func__);
		return;
	}

	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;
	*(u32 *)ack_msg.data = mac->sts_regval;
	*(u32 *)&ack_msg.data[4] = mac->speed;
	ack_msg.data[8] = mac->port;
	ack_msg.data[9] = mac->fec_cfg;
	*(u32 *)&ack_msg.data[10] = mac->speed_cfg;
	ack_msg.data[14] = mac->speed_cfg_autoneg;
	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
}

static void yk3_mac_mbox_pf_set_vf_port(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_ndev_priv *ndev_priv = (struct yk3_ndev_priv *)param;

	if (!yk3_ndev_is_vf(ndev_priv))
		return;

	ndev_priv->port = msg->data[0];

	yk3_net_debug("%s, port 0x%x\n", __func__, ndev_priv->port);
}

static void yk3_mac_mbox_emp_fwd_to_pf_port(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_ndev_priv *ndev_priv, *pos;
	u8 port = msg->data[0];
	struct yk3_mbox_msg msg1 = {0};
	struct yk3_mbox_option opt1 = {0};
	int exist_vfs;
	u8 i;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	yk3_dev_info("%s, pf %u, port 0x%x\n", __func__, pdev_priv->pf_id, port);

	mutex_lock(&pdev_priv->ndev_priv_head_mlock);
	list_for_each_entry_safe(ndev_priv, pos, &pdev_priv->ndev_priv_head, pdev_node)
		ndev_priv->port = port;
	mutex_unlock(&pdev_priv->ndev_priv_head_mlock);

	exist_vfs = pci_num_vf(pdev_priv->pdev);
	for (i = 0; i < exist_vfs; i++) {
		msg1.dst_id = yk3_mbox_vf_id(i);
		msg1.opcode = YK3_MBOX_OPCODE_SET_VF_PORT_INFO;
		msg1.data[0] = port;
		opt1.wait_reply = MB_NO_REPLY;
		opt1.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &msg1, &opt1, NULL);
	}
}

static int yk3_mac_speed_test(struct net_device *ndev, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	*data = 0;

	if (netif_carrier_ok(ndev) &&
	    ndev_priv->link_speed != SPEED_UNKNOWN &&
	    netif_running(ndev)) {
		yk3_net_info("Speed test end: result(0)\n");
		return 0;
	}

	yk3_net_info("Speed test end: result(1)\n");
	*data = 1;

	return 1;
}

static int yk3_mac_link_test(struct net_device *ndev, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	*data = 0;

	if (netif_carrier_ok(ndev) && netif_running(ndev)) {
		yk3_net_info("Link test end: result(0)\n");
		return 0;
	}

	yk3_net_info("Link test end: result(1)\n");
	*data = 1;

	return 1;
}

struct yk3_mac_lbt_priv {
	struct packet_type pt;
	struct completion comp;
	bool loopback_ok;
};

struct yshdr {
	__be32 version;
	__be64 magic;
};

#define YK3_TEST_PKT_SIZE (sizeof(struct ethhdr) + sizeof(struct iphdr) +\
			  sizeof(struct udphdr) + sizeof(struct yshdr))
#define YK3_TEST_MAGIC 0x5AEFD32C798ULL
#define YK3_VERIFY_TIMEOUT (msecs_to_jiffies(200))

static int yk3_test_loopback_validate(struct sk_buff *skb,
				      struct net_device *ndev,
				      struct packet_type *pt,
				      struct net_device *orig_ndev)
{
	struct yk3_mac_lbt_priv *lbtp = pt->af_packet_priv;
	struct yshdr *ysh;
	struct ethhdr *ethh;
	struct udphdr *udph;
	struct iphdr *iph;

	if (YK3_TEST_PKT_SIZE - ETH_HLEN > skb_headlen(skb))
		goto out;

	ethh = (struct ethhdr *)skb_mac_header(skb);
	if (!ether_addr_equal(ethh->h_dest, orig_ndev->dev_addr))
		goto out;

	iph = ip_hdr(skb);
	if (iph->protocol != IPPROTO_UDP)
		goto out;

	udph = (struct udphdr *)((u8 *)iph + 4 * iph->ihl);
	if (udph->dest != htons(9))
		goto out;

	ysh = (struct yshdr *)((char *)udph + sizeof(*udph));
	if (ysh->magic != cpu_to_be64(YK3_TEST_MAGIC))
		goto out;

	lbtp->loopback_ok = true;
	complete(&lbtp->comp);
out:
	kfree_skb(skb);
	return 0;
}

static int yk3_mac_loopback_setup(struct net_device *ndev,
				  struct yk3_mac_lbt_priv *lbtp)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	enum yk3_mac_channel mac_ch;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return -EIO;
	}

	mac_ch = pdev_priv->mac->mac_ch;
	yk3_net_debug("mac_ch %u\n", mac_ch);

	msg.dst_id = yk3_mbox_emp_id();
	msg.opcode = YK3_MBOX_OPCODE_EMP_ENABLE_LOOPBACK;
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

	lbtp->loopback_ok = false;
	init_completion(&lbtp->comp);

	lbtp->pt.type = htons(ETH_P_IP);
	lbtp->pt.func = yk3_test_loopback_validate;
	lbtp->pt.dev = ndev;
	lbtp->pt.af_packet_priv = lbtp;
	dev_add_pack(&lbtp->pt);

	return 0;
}

static struct sk_buff *yk3_test_get_udp_skb(struct net_device *ndev)
{
	struct sk_buff *skb = NULL;
	struct yshdr *ysh;
	struct ethhdr *ethh;
	struct udphdr *udph;
	struct iphdr *iph;
	int    iplen;

	skb = netdev_alloc_skb(ndev, YK3_TEST_PKT_SIZE);
	if (!skb)
		return NULL;

	skb_reserve(skb, NET_IP_ALIGN);

	ethh = (struct ethhdr *)skb_push(skb, ETH_HLEN);
	skb_reset_mac_header(skb);

	skb_set_network_header(skb, skb->len);
	iph = (struct iphdr *)skb_put(skb, sizeof(struct iphdr));

	skb_set_transport_header(skb, skb->len);
	udph = (struct udphdr *)skb_put(skb, sizeof(struct udphdr));

	ether_addr_copy(ethh->h_dest, ndev->dev_addr);
	eth_zero_addr(ethh->h_source);
	ethh->h_proto = htons(ETH_P_IP);

	udph->source = htons(9);
	udph->dest = htons(9); /* Discard Protocol */
	udph->len = htons(sizeof(struct yshdr) + sizeof(struct udphdr));
	udph->check = 0;

	iph->ihl = 5;
	iph->ttl = 32;
	iph->version = 4;
	iph->protocol = IPPROTO_UDP;
	iplen = sizeof(struct iphdr) + sizeof(struct udphdr) +
		sizeof(struct yshdr);
	iph->tot_len = htons(iplen);
	iph->frag_off = 0;
	iph->saddr = 0;
	iph->daddr = 0;
	iph->tos = 0;
	iph->id = 0;
	ip_send_check(iph);

	ysh = (struct yshdr *)skb_put(skb, sizeof(*ysh));
	ysh->version = 0;
	ysh->magic = cpu_to_be64(YK3_TEST_MAGIC);

	skb->csum = 0;
	skb->ip_summed = CHECKSUM_PARTIAL;
	udp4_hwcsum(skb, iph->saddr, iph->daddr);

	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->dev = ndev;

	return skb;
}

static void yk3_test_loopback_cleanup(struct net_device *ndev,
				      struct yk3_mac_lbt_priv *lbtp)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	enum yk3_mac_channel mac_ch;

	if (!pdev_priv->mac) {
		yk3_net_err("%s pdev_priv mac null\n", __func__);
		return;
	}

	mac_ch = pdev_priv->mac->mac_ch;
	yk3_net_debug("mac_ch %u\n", mac_ch);

	msg.dst_id = yk3_mbox_emp_id();
	msg.opcode = YK3_MBOX_OPCODE_EMP_DISABLE_LOOPBACK;
	msg.data[0] = mac_ch;
	msg.data_length = 1;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg))
		yk3_net_err("send mbox msg err\n");
	if (ack_msg.data[0])
		yk3_net_err("mbox resp result 0x%02x reason 0x%02x\n",
			    ack_msg.data[0], ack_msg.data[1]);

	dev_remove_pack(&lbtp->pt);
}

static int yk3_mac_loopback_test(struct net_device *ndev, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_mac_lbt_priv *lbtp;
	struct sk_buff *skb = NULL;
	int ret = 0;

	lbtp = kzalloc(sizeof(*lbtp), GFP_KERNEL);
	if (!lbtp)
		return 1;

	ret = yk3_mac_loopback_setup(ndev, lbtp);
	if (ret) {
		yk3_net_err("failed setup loopback ret %d\n", ret);
		goto cleanup;
	}

	skb = yk3_test_get_udp_skb(ndev);
	if (!skb) {
		yk3_net_err("failed get udp skb\n");
		ret = 1;
		goto cleanup;
	}

	skb_set_queue_mapping(skb, 0);
	ret = dev_queue_xmit(skb);
	if (ret) {
		yk3_net_err("failed to xmit loopback packet, ret %d\n", ret);
		goto cleanup;
	}

	ret = wait_for_completion_timeout(&lbtp->comp, YK3_VERIFY_TIMEOUT);
	if (ret == 0) {
		yk3_net_err("loopback wait recv skb timeout\n");
		ret = -ETIMEDOUT;
	} else {
		ret = 0;
	}

	yk3_net_debug("loopback wait recv skb ok, check lbtp->loopback_ok %u\n", lbtp->loopback_ok);

	ret = !lbtp->loopback_ok;

cleanup:
	yk3_test_loopback_cleanup(ndev, lbtp);
	kfree(lbtp);
	return ret;
}

void yk3_mac_self_offline_test(struct net_device *ndev,
			       struct ethtool_test *eth_test, u64 *data)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	yk3_net_info("%s testing starting\n", eth_test->flags ? "offline" : "online");

	if (netif_carrier_ok(ndev)) {
		if (!yk3_mac_loopback_test(ndev, &data[YK3_MAC_LOOPBACK_TEST])) {
			data[YK3_MAC_LOOPBACK_TEST] = 0;
			data[YK3_MAC_REG_TEST] = 0;
			data[YK3_MAC_INT_TEST] = 0;
			yk3_net_info("Registers test end: result(0)\n");
			yk3_net_info("Interrupt test end: result(0)\n");
			yk3_net_info("Loopback test end: result(0)\n");
		} else {
			eth_test->flags |= ETH_TEST_FL_FAILED;
			data[YK3_MAC_LOOPBACK_TEST] = 1;
			data[YK3_MAC_REG_TEST] = 0;
			data[YK3_MAC_INT_TEST] = 0;
			yk3_net_info("Registers test end: result(0)\n");
			yk3_net_info("Interrupt test end: result(0)\n");
			yk3_net_info("Loopback test end: result(1)\n");
		}
	}

	msleep(3000);

	if (yk3_mac_link_test(ndev, &data[YK3_MAC_LINK_TEST]))
		eth_test->flags |= ETH_TEST_FL_FAILED;
	if (yk3_mac_speed_test(ndev, &data[YK3_MAC_SPEET_TEST]))
		eth_test->flags |= ETH_TEST_FL_FAILED;

	yk3_net_info("%s testing stop\n", eth_test->flags ? "offline" : "online");
}

/**
 * yk3_mac_set_fc - set FC config for mac register to EMP
 * @ndev_priv: pointer to relevant ndev private
 * @mac_ch: set mac channel fc
 * @config: mbox date for pfc config
 */
int yk3_mac_set_fc(struct yk3_ndev_priv *ndev_priv, u8 mac_ch, u8 config)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret = 0;

	if (yk3_pdev_is_mgr(pdev_priv))
		return 0;

	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_FC;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data_length = 2;
	mbox_msg.data[0] = mac_ch;
	mbox_msg.data[1] = config;
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		if (recv_msg.data[0]) {
			yk3_dev_err("Set umac fc data check errno %02x result is %02x\n",
				    recv_msg.data[0], recv_msg.data[1]);
		}
		yk3_dev_err("%s send mbox message errno %d failed!\n", __func__, ret);
		return ret;
	}

	return ret;
}

/**
 * yk3_mac_set_pfc - set PFC config for mac register to EMP
 * @ndev_priv: pointer to relevant ndev private
 * @mac_ch: set mac channel pfc
 * @config: mbox date for pfc config
 */
int yk3_mac_set_pfc(struct yk3_ndev_priv *ndev_priv, u8 mac_ch, u8 config)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret;

	if (yk3_pdev_is_mgr(pdev_priv))
		return 0;

	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_PFC;
	mbox_msg.data_length = 2;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data[0] = mac_ch;
	mbox_msg.data[1] = config;
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		if (recv_msg.data[0]) {
			yk3_dev_err("Set umac pfc data check errno %02x result is %02x\n",
				    recv_msg.data[0], recv_msg.data[1]);
		}
		yk3_dev_err("Send mbox message errno %d failed!\n", ret);
	}

	return ret;
}

/**
 * yk3_set_prio_vlan_mode - set the priority mode for dcbx
 * @ndev_priv: the corresponding netdev privite data
 * @enable: true for vlan mode, false for dscp mode
 */
static int yk3_set_prio_vlan_mode(struct yk3_ndev_priv *ndev_priv, bool enable)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (!pdev_priv)
		return -ENODEV;

#ifdef CONFIG_DCB
	if (!ndev_priv->dcbx)
		return -EINVAL;
#endif

	if (enable) {
		ret = yk3_ppp_primap_mode_set(pdev_priv, PRIMAP_VLAN);
#ifdef CONFIG_DCB
		if (!ret)
			WRITE_ONCE(ndev_priv->dcbx->prio, YK3_PRIO_VLAN);
#endif
	} else {
		ret = yk3_ppp_primap_mode_set(pdev_priv, PRIMAP_IP);
#ifdef CONFIG_DCB
		if (!ret)
			WRITE_ONCE(ndev_priv->dcbx->prio, YK3_PRIO_DSCP);
#endif
	}

	return ret;
}

int yk3_mac_set_priv_flags(struct yk3_ndev_priv *ndev_priv, const u32 flags, bool enable)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	struct yk3_umac_mbox_priv_flags_msg *priv_data;
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

	priv_data = (struct yk3_umac_mbox_priv_flags_msg *)mbox_msg.data;
	priv_data->mac_ch = pdev_priv->mac->mac_ch;
	priv_data->enable = enable ? 1 : 0;

	switch (flags) {
	case YK3_ET_PFLAG_FC_PAUSE_FILTER:
		mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_FC_FILTER;
		break;
	case YK3_ET_PFLAG_PFC_PAUSE_FILTER:
		mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_PFC_FILTER;
		break;
	case YK3_ET_PFLAG_PRIO_VLAN_MODE:
		return yk3_set_prio_vlan_mode(ndev_priv, enable);
	case YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE:
		mbox_msg.opcode = YK3_MBOX_OPCODE_LINK_DOWN_ON_CLOSE;
		break;
	case YK3_ET_PFLAG_LLDP_MODE:
		mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_LLDP_MODE;
		break;
	default:
		yk3_dev_err("ethtool set priv flags is not support.\n");
		return -EOPNOTSUPP;
	}

	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data_length = 2;
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		if (recv_msg.data[0]) {
			yk3_dev_err("Set mac priv flags data check errno %02x result is %02x\n",
				    recv_msg.data[0], recv_msg.data[1]);
		}
		yk3_dev_err("Send mbox message errno %d failed!\n", ret);
		return ret;
	}

	return 0;
}

int yk3_mac_ndev_set_phy_state(struct yk3_ndev_priv *ndev_priv, bool on)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_option opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret;

	if (!pdev_priv || !pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv or mac null\n", __func__);
		return -ENODEV;
	}

	msg.data[0] = pdev_priv->mac->mac_ch;
	msg.data[1] = on ? 1 : 0;
	msg.opcode = YK3_MBOX_OPCODE_LINK_DOWN_ON_CLOSE;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data_length = 2;
	opt.timeout = 1000;
	opt.wait_reply = MB_WAIT_REPLY;

	yk3_dev_info("set phy opcode 0x%x mac_ch %u on %u\n",
		     YK3_MBOX_OPCODE_LINK_DOWN_ON_CLOSE, pdev_priv->mac->mac_ch, on);

	ret = yk3_mbox_send_msg(pdev_priv, &msg, &opt, &recv_msg);
	if (ret != 0) {
		if (recv_msg.data[0]) {
			yk3_dev_err("Set mac phy state failed, errno %02x result is %02x\n",
				    recv_msg.data[0], recv_msg.data[1]);
		}
		yk3_dev_err("Send mbox message errno %d failed!\n", ret);
		return ret;
	}

	return 0;
}

static u32 yk3_mac_get_speed_cfg(struct yk3_pdev_priv *pdev_priv, u8 *speed_type_cfg)
{
	struct yk3_mac *mac = pdev_priv->mac;
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!mac)
		return -EIO;

	mac_ch = mac->mac_ch;
	msg.opcode = YK3_MBOX_OPCODE_EMP_GET_SPEED;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 1;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_dev_err("send mbox msg err, pf %u mac_ch %u opcode 0x%x\n",
			    pdev_priv->pf_id, mac_ch, msg.opcode);
		return -EIO;
	}
	*speed_type_cfg = ack_msg.data[2];

	return 0;
}

static u32 yk3_mac_get_fec_cfg(struct yk3_pdev_priv *pdev_priv, u8 *fec_cfg)
{
	struct yk3_mac *mac = pdev_priv->mac;
	enum yk3_mac_channel mac_ch;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (!mac)
		return -EIO;

	mac_ch = mac->mac_ch;
	msg.opcode = YK3_MBOX_OPCODE_EMP_GET_FEC;
	msg.dst_id = yk3_mbox_emp_id();
	msg.data[0] = mac_ch;
	msg.data_length = 1;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
		yk3_dev_err("send mbox msg err, pf %u mac_ch %u opcode 0x%x\n",
			    pdev_priv->pf_id, mac_ch, msg.opcode);
		return -EIO;
	}
	*fec_cfg = ack_msg.data[2];

	return 0;
}

int yk3_mac_ndev_init(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_card *card = pdev_priv->card;
	struct yk3_mac *mac = pdev_priv->mac;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	u8 speed_type_cfg, speed_cfg_autoneg;
	u32 speed_cfg;

	if (yk3_pdev_is_pf(pdev_priv)) {
		if (!card) {
			yk3_dev_err("card info err\n");
			return -EIO;
		}
		if (!mac) {
			yk3_dev_err("mac info err\n");
			return -EIO;
		}

		if (yk3_mac_get_speed_cfg(pdev_priv, &speed_type_cfg)) {
			yk3_dev_err("get link speed cfg err\n");
			return -EIO;
		}
		switch (speed_type_cfg) {
		case YK3_10G:
			speed_cfg = SPEED_10G;
			speed_cfg_autoneg = AUTONEG_DISABLE;
			break;
		case YK3_25G:
			speed_cfg = SPEED_25G;
			speed_cfg_autoneg = AUTONEG_DISABLE;
			break;
		case YK3_40G:
			speed_cfg = SPEED_40G;
			speed_cfg_autoneg = AUTONEG_DISABLE;
			break;
		case YK3_100G:
			speed_cfg = SPEED_100G;
			speed_cfg_autoneg = AUTONEG_DISABLE;
			break;
		case YK3_AUTONEG:
			speed_cfg = SPEED_UNKNOWN;
			speed_cfg_autoneg = AUTONEG_ENABLE;
			break;
		default:
			speed_cfg = SPEED_UNKNOWN;
			speed_cfg_autoneg = AUTONEG_ENABLE;
			yk3_dev_debug("default link speed not force, speed_type %d\n",
				      (int)speed_type_cfg);
		}
		mac->speed_cfg = speed_cfg;
		mac->speed_cfg_autoneg = speed_cfg_autoneg;
		card->link_speed_cfg = mac->speed_cfg;
		card->link_speed_cfg_autoneg = mac->speed_cfg_autoneg;

		if (yk3_mac_get_fec_cfg(pdev_priv, &mac->fec_cfg)) {
			yk3_dev_err("get link fec cfg err, mac_ch %u\n", mac->mac_ch);
			return -EIO;
		}

		if (yk3_mac_get_speed(pdev_priv, &mac->speed)) {
			yk3_dev_err("get speed err, mac_ch %u\n", mac->mac_ch);
			return -EIO;
		}

		if (yk3_mac_get_connector(pdev_priv)) {
			yk3_dev_err("get connector err, mac_ch %u\n", mac->mac_ch);
			return -EIO;
		}

		if (yk3_mac_get_sts_regval(pdev_priv)) {
			yk3_dev_err("get sts_regval err, mac_ch %u\n", mac->mac_ch);
			return -EIO;
		}

		yk3_dev_info("%s, pf %u port 0x%x speed_cfg %d speed_cfg_autoneg %u speed %d fec_cfg %u\n",
			     __func__,
			     pdev_priv->pf_id,
			     mac->port,
			     (int)card->link_speed_cfg,
			     card->link_speed_cfg_autoneg,
			     (int)mac->speed,
			     mac->fec_cfg);

		yk3_mac_set_ndev_status(pdev_priv->card->hw_bits, ndev_priv, mac->sts_regval);
		ndev_priv->link_speed = mac->speed;
		ndev_priv->link_duplex = DUPLEX_FULL;
		ndev_priv->port = mac->port;
		ndev_priv->link_fec_cfg = mac->fec_cfg;
		ndev_priv->link_speed_cfg = mac->speed_cfg;
		ndev_priv->link_speed_cfg_autoneg = mac->speed_cfg_autoneg;
		if (ndev_priv->link_speed != SPEED_UNKNOWN && ndev_priv->link_status) {
			yk3_np_tm_speed_limit(pdev_priv, ndev_priv->link_speed);
			yk3_qos_set_link_speed(pdev_priv, ndev_priv->link_speed);
			yk3_edma_l2_vq(pdev_priv, ndev_priv->link_speed);
		}
		yk3_set_hqos_bp(pdev_priv);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		/* parameter is ndev, so placed in ndev_init not mac_init */
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_SET_VF_MAC_INFO,
					   yk3_mac_mbox_pf_set_vf_mac_info,
					   (void *)ndev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_SET_VF_PORT_INFO,
					   yk3_mac_mbox_pf_set_vf_port,
					   (void *)ndev_priv);

		msg.dst_id = yk3_mbox_pf_id(0);
		msg.opcode = YK3_MBOX_OPCODE_GET_PF_MAC_INFO;
		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;
		if (yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg)) {
			yk3_net_err("%s send mbox msg err, vf %u\n", __func__, pdev_priv->vf_id);
			return -EIO;
		}
		yk3_mac_set_ndev_status(pdev_priv->card->hw_bits,
					ndev_priv,
					*(u32 *)&ack_msg.data[0]);
		ndev_priv->link_speed = *(u32 *)&ack_msg.data[4];
		ndev_priv->link_duplex = DUPLEX_FULL;
		ndev_priv->port = ack_msg.data[8];
		ndev_priv->link_fec_cfg = ack_msg.data[9];
		ndev_priv->link_speed_cfg = *(u32 *)&ack_msg.data[10];
		ndev_priv->link_speed_cfg_autoneg = ack_msg.data[14];

		yk3_net_debug("%s, vf %u port 0x%x speed_cfg %u speed_cfg_autoneg %u speed %d fec_cfg %u\n",
			      __func__,
			      pdev_priv->vf_id,
			      ndev_priv->port,
			      pdev_priv->card->link_speed_cfg,
			      pdev_priv->card->link_speed_cfg_autoneg,
			      (int)ndev_priv->link_speed,
			      ndev_priv->link_fec_cfg);
	}

	return 0;
}

static void yk3_mbox_emp_report_xcvr_sts(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u8 mac_ch = msg->data[0];
	u32 status = (u32)msg->data[4];
	struct yk3_card *card = pdev_priv->card;
	struct yk3_pdev_priv *pdev_priv_tmp;
	struct yk3_mac *mac;
	struct yk3_mbox_msg msg1 = {0};
	struct yk3_mbox_option opt1 = {0};

	yk3_dev_info("transceiver event, mac_ch %u status 0x%08x\n", mac_ch, status);

	if (!card) {
		yk3_dev_err("emp report cxvr but card err\n");
		return;
	}

	if (mac_ch >= YK3_MAC_CH_MAX) {
		yk3_dev_err("emp report cxvr but mac_ch err, mac_ch %u\n", mac_ch);
		return;
	}

	mutex_lock(&card->pdev_priv_head_mlock);
	list_for_each_entry(pdev_priv_tmp, &card->pdev_priv_head, card_node) {
		mac = pdev_priv_tmp->mac;
		if (!mac)
			continue;
		if (mac->mac_ch != mac_ch)
			continue;
		if (status & XCVR_STATUS_READY) {
			if (yk3_mac_get_connector(pdev_priv_tmp))
				yk3_dev_err("emp report but get connector err, mac_ch %u\n",
					    mac_ch);
		} else {
			mac->port = PORT_NONE;
		}

		yk3_dev_info("%s, pf %u, mac_ch %u, port 0x%x\n",
			     __func__, pdev_priv_tmp->pf_id, mac_ch, mac->port);

		msg1.dst_id = yk3_mbox_pf_id(pdev_priv_tmp->pf_id);
		msg1.opcode = YK3_MBOX_OPCODE_FWD_TO_PF_PORT_INFO;
		msg1.data[0] = mac->port;
		opt1.wait_reply = MB_NO_REPLY;
		opt1.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &msg1, &opt1, NULL);
	}
	mutex_unlock(&card->pdev_priv_head_mlock);
}

int yk3_mac_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_irq_param irq_param;
	struct yk3_mac *mac;
	enum yk3_mac_channel mac_ch;
	void __iomem *hw_addr;
	u32 val = 0;
	int ret = -1;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_REPORT_XCVR_STS,
					   yk3_mbox_emp_report_xcvr_sts,
					   (void *)pdev_priv);
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_GET_PF_MAC_INFO,
					   yk3_mac_mbox_vf_get_pf_mac_info,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_FWD_TO_PF_PORT_INFO,
					   yk3_mac_mbox_emp_fwd_to_pf_port,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_PF_SET_IFG,
					   yk3_mac_mbox_pf_set_ifg,
					   (void *)pdev_priv);

		mac = kzalloc(sizeof(*mac), GFP_KERNEL);
		if (!mac)
			return -ENOMEM;

		pdev_priv->mac = mac;
		mac->pdev = pdev_priv->pdev;
		mac_ch = pdev_priv->card->mac_chs[pdev_priv->pf_id];
		mac->mac_ch = mac_ch;
		timer_setup(&mac->link_timer, yk3_link_timer_callback, 0);
		INIT_WORK(&mac->work, yk3_mac_work);
		snprintf(irq_param.name, sizeof(irq_param.name), "mac0-ch%u", mac_ch);
		irq_param.vector = -1;
		irq_param.handler = yk3_mac_intr;
		irq_param.data = pdev_priv;
		irq_param.flags = YK3_F_IRQ_EXCLUSIVE;
		ret = yk3_irq_request(pdev_priv, &irq_param);
		if (ret < 0) {
			yk3_dev_err("%s yk3_irq_request() failed, %d\n", __func__, ret);
			kfree(mac);
			return ret;
		}
		mac->irq_vector = ret;

		switch (mac_ch) {
		case 0:
			val = yk3_rd32(hw_addr, MACX_INTER_HOST_LAN01(0));
			val |= FIELD_PREP(MAC_INTER_LAN02_MASK, mac->irq_vector);
			yk3_wr32(hw_addr, MACX_INTER_HOST_LAN01(0), val);
			break;
		case 1:
			val = yk3_rd32(hw_addr, MACX_INTER_HOST_LAN01(0));
			val |= FIELD_PREP(MAC_INTER_LAN13_MASK, mac->irq_vector);
			yk3_wr32(hw_addr, MACX_INTER_HOST_LAN01(0), val);
			break;
		case 2:
			val = yk3_rd32(hw_addr, MACX_INTER_HOST_LAN23(0));
			val |= FIELD_PREP(MAC_INTER_LAN02_MASK, mac->irq_vector);
			yk3_wr32(hw_addr, MACX_INTER_HOST_LAN23(0), val);
			break;
		case 3:
			val = yk3_rd32(hw_addr, MACX_INTER_HOST_LAN23(0));
			val |= FIELD_PREP(MAC_INTER_LAN13_MASK, mac->irq_vector);
			yk3_wr32(hw_addr, MACX_INTER_HOST_LAN23(0), val);
			break;
		default:
			yk3_dev_err("%s macch err %u\n", __func__, mac_ch);
			break;
		}

		yk3_dev_info("%s wr vector_reg 0x%08x\n", __func__, val);

		val = FIELD_PREP(MAC_CH_INTER_HOST_PF_MASK, pdev_priv->pf_id);
		yk3_wr32(hw_addr, MACX_INTER_CHY_FUNC(0, mac_ch), val);

		yk3_dev_info("%s wr pf_reg 0x%08x\n", __func__, val);

		yk3_wr32(hw_addr, MACX_INTER_JITTER(0), MAC_INTER_JITTER_DEF);
		yk3_wr32(hw_addr, MACX_INTER_ENABLE(0), MAC_INTER_ENABLE_ALL);

		debugfs_create_u32("mac_phy_stat_err", 0644, pdev_priv->dbgfs_dir,
				   &mac->phy_stat_err);
		debugfs_create_u32("mac_link_int_cnt", 0644, pdev_priv->dbgfs_dir,
				   &mac->link_int_cnt);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		/* Doesn't need do everything , mbox register in ndev init */
	} else {
		yk3_dev_err("%s unknown device %08x\n", __func__, pdev_priv->device);
		return -ENODEV;
	}

	yk3_dev_info("%s ok\n", __func__);
	return 0;
}

void yk3_mac_exit(struct yk3_pdev_priv *pdev_priv)
{
	void __iomem *hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	struct yk3_mac *mac = pdev_priv->mac;
	struct yk3_irq_param irq_param;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_REPORT_XCVR_STS);
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		if (mac->link_timer.function)
			del_timer_sync(&mac->link_timer);
		yk3_wr32(hw_addr, MACX_INTER_ENABLE(0), 0x0);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_GET_PF_MAC_INFO);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_FWD_TO_PF_PORT_INFO);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_PF_SET_IFG);
		if (pdev_priv->mac) {
			irq_param.vector = mac->irq_vector;
			irq_param.handler = yk3_mac_intr;
			irq_param.data = pdev_priv;
			yk3_irq_free(pdev_priv, &irq_param);
			cancel_work_sync(&pdev_priv->mac->work);
			kfree(pdev_priv->mac);
			pdev_priv->mac = NULL;
		}
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_SET_VF_MAC_INFO);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_SET_VF_PORT_INFO);
	}
}
