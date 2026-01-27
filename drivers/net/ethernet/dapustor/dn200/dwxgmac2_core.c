// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include <linux/bitrev.h>
#include <linux/crc32.h>
#include <linux/iopoll.h>
#include "dn200.h"
#include "dn200_ptp.h"
#include "dwxgmac_comm.h"

static int dn200_del_vf_uc_rxp_da_route(struct mac_device_info *hw,
					    int offset);
static int dn200_del_pf_uc_rxp_da_route(struct mac_device_info *hw,
					    int offset);
static int dn200_add_pf_uc_rxp_da_route_sriov(struct mac_device_info *hw,
						 u8 *mac_addr, int offset);
static int dn200_add_vf_uc_rxp_da_route_sriov(struct mac_device_info *hw,
						 u8 *mac_addr, int offset, u8 rxq_start);
static void dn200_clear_mc_da_route(struct mac_device_info *hw, u8 rxq_start);
static int dn200_mc_add_rxp(struct mac_device_info *hw, u8 *mac_addr, u8 rxq_start);
static int dn200_pf_lram_uc_add_rxp(struct mac_device_info *hw,
				       u8 *mac_addr);
static void dn200_update_bcmc_channel(struct mac_device_info *hw,
					int rxp_offset, bool add, u8 rxq_start);
static void dn200_update_ampm_rxp(struct mac_device_info *hw, u8 rxq_start);
static int dwxgmac_reset_rxp(struct mac_device_info *hw);
static void dn200_clear_lram_pf_uc_rxp(struct mac_device_info *hw);
static int dwxgmac2_rxp_get_single_entry_sriov(struct mac_device_info *hw,
					       u32 *data, int real_pos);
static int dwxgmac2_rxp_update_single_entry_sriov(struct mac_device_info *hw,
						  u32 data, int real_pos);
static int dwxgmac2_rxp_get_single_entry_sriov(struct mac_device_info *hw,
					       u32 *data, int real_pos);

static int dwxgmac2_rxp_update_single_entry_sriov(struct mac_device_info *hw,
						  u32 data, int real_pos);
static int dwxgmac2_rxp_update_single_da_entry_sriov(struct mac_device_info *hw,
						     struct RXP_FPR_ENTRY
						     *entry, int pos);
static void dwxgmac3_rxp_enable(void __iomem *ioaddr);

static void dwxgmac2_map_mtl_to_dma(struct mac_device_info *hw, u32 queue,
				    u32 chan);
static void dn200_clear_allmu_promisc_da_route(struct mac_device_info *hw,
						 int offset, u8 rxq_start);
static int dwxgmac2_rxp_get_single_da_entry_sriov(struct mac_device_info *hw,
						  struct RXP_FPR_ENTRY *entry,
						  int pos);
static int rxp_offset_get_from_bitmap(int bitmap_off);
static int dn200_get_used_bit_from_last(unsigned long *bitmap, u8 last,
					u8 first);
static void dn200_mc_rxp_channel_route_set(struct mac_device_info *hw,
					      bool enable, u16 bitmap_promisc);

static void dwxgmac2_core_init(struct mac_device_info *hw,
			       struct net_device *dev)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 tx, rx;

	tx = readl(ioaddr + XGMAC_TX_CONFIG);
	rx = readl(ioaddr + XGMAC_RX_CONFIG);

	tx |= XGMAC_CORE_INIT_TX;
	rx |= XGMAC_CORE_INIT_RX;

	if (hw->ps) {
		tx |= XGMAC_CONFIG_TE;
		tx &= ~hw->link.speed_mask;

		switch (hw->ps) {
		case SPEED_10000:
			tx |= hw->link.xgmii.speed10000;
			break;
		case SPEED_2500:
			tx |= hw->link.speed2500;
			break;
		case SPEED_1000:
		default:
			tx |= hw->link.speed1000;
			break;
		}
	}
	if (HW_IS_VF(hw))
		return;
	writel(tx, ioaddr + XGMAC_TX_CONFIG);
	writel(rx | XGMAC_CONFIG_CST | XGMAC_CONFIG_ACS,
	       ioaddr + XGMAC_RX_CONFIG);
	writel(XGMAC_INT_DEFAULT_EN, ioaddr + XGMAC_INT_EN);
}

static void dwxgmac2_set_mac(void __iomem *ioaddr, bool enable,
			     struct mac_device_info *hw)
{
	u32 tx = readl(ioaddr + XGMAC_TX_CONFIG);
	u32 rx = readl(ioaddr + XGMAC_RX_CONFIG);

	if (enable) {
		tx |= XGMAC_CONFIG_TE;
		rx |= XGMAC_CONFIG_RE;
	} else {
		tx &= ~XGMAC_CONFIG_TE;
		rx &= ~XGMAC_CONFIG_RE;
	}
	if (HW_IS_VF(hw))
		return;
	writel(tx, ioaddr + XGMAC_TX_CONFIG);
	writel(rx, ioaddr + XGMAC_RX_CONFIG);
}

static void dwxgmac2_mac_rx_set(void __iomem *ioaddr, bool enable)
{
	u32 rx = readl(ioaddr + XGMAC_RX_CONFIG);

	if (enable)
		rx |= XGMAC_CONFIG_RE;
	else
		rx &= ~XGMAC_CONFIG_RE;

	writel(rx, ioaddr + XGMAC_RX_CONFIG);
}

static int dwxgmac2_mac_rx_get(void __iomem *ioaddr)
{
	u32 rx = readl(ioaddr + XGMAC_RX_CONFIG);

	return !!(rx & XGMAC_CONFIG_RE);
}

static int dwxgmac2_rx_ipc(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	value = readl(ioaddr + XGMAC_RX_CONFIG);
	if (hw->rx_csum)
		value |= XGMAC_CONFIG_IPC;
	else
		value &= ~XGMAC_CONFIG_IPC;
	writel(value, ioaddr + XGMAC_RX_CONFIG);

	return !!(readl(ioaddr + XGMAC_RX_CONFIG) & XGMAC_CONFIG_IPC);
}

static void dwxgmac2_rx_queue_enable(struct mac_device_info *hw, u8 mode,
				     u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	queue += DN200_RXQ_START_GET(hw);
	if (!DN200_MTL_QUEUE_IS_VALID(hw->priv, queue))
		return;

	value = readl(ioaddr + XGMAC_RXQ_CTRL0) & ~XGMAC_RXQEN(queue);
	if (mode == MTL_QUEUE_AVB)
		value |= 0x1 << XGMAC_RXQEN_SHIFT(queue);
	else if (mode == MTL_QUEUE_DCB)
		value |= 0x2 << XGMAC_RXQEN_SHIFT(queue);
	writel(value, ioaddr + XGMAC_RXQ_CTRL0);
}

static void dwxgmac2_rx_queue_disable(struct mac_device_info *hw, u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	queue += DN200_RXQ_START_GET(hw);
	if (!DN200_MTL_QUEUE_IS_VALID(hw->priv, queue))
		return;

	value = readl(ioaddr + XGMAC_RXQ_CTRL0) & ~XGMAC_RXQEN(queue);
	writel(value, ioaddr + XGMAC_RXQ_CTRL0);
}

static void dwxgmac2_rx_dds_config_sriov(struct mac_device_info *hw,
					 bool enable)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	if (!enable) {
		/*disable DDS */
		value = readl(ioaddr + XGMAC_MAC_EXT_CONF);
		value &= ~XGMAC_DDS_ENABLE;
		writel(value, ioaddr + XGMAC_MAC_EXT_CONF);

		//clear mcbc router
		value = readl(ioaddr + XGMAC_RXQ_CTRL1);
		value &= ~XGMAC_MCBCQEN;
		value &= ~XGMAC_MCBCQ;
		writel(value, ioaddr + XGMAC_RXQ_CTRL1);
	} else {
		/*enable DDS */
		value = readl(ioaddr + XGMAC_MAC_EXT_CONF);
		value |= XGMAC_DDS_ENABLE;
		writel(value, ioaddr + XGMAC_MAC_EXT_CONF);
		value = readl(ioaddr + XGMAC_RXQ_CTRL4);
		value |= XGMAC_UDC;
		writel(value, ioaddr + XGMAC_RXQ_CTRL4);
		//route mcbc to rx queue 15
		value = readl(ioaddr + XGMAC_RXQ_CTRL1);
		value |= XGMAC_MCBCQEN;
		value |= ((DN200_LAST_QUEUE(hw->priv)) << XGMAC_MCBCQ_SHIFT);
		value &= ~(XGMAC_RQ);
		value |= (DN200_LAST_QUEUE(hw->priv) << XGMAC_RQ_SHIFT);
		value &= ~(XGMAC_UPQ);
		value |= (DN200_LAST_QUEUE(hw->priv));
		writel(value, ioaddr + XGMAC_RXQ_CTRL1);
	}
}

static void dwxgmac2_rx_queue_prio(struct mac_device_info *hw, u32 prio,
				   u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, reg;

	if (HW_IS_VF(hw))
		return;
	queue += DN200_RXQ_START_GET(hw);
	reg = (queue < 4) ? XGMAC_RXQ_CTRL2 : XGMAC_RXQ_CTRL3;
	if (queue >= 4)
		queue -= 4;

	value = readl(ioaddr + reg);
	value &= ~XGMAC_PSRQ(queue);
	value |= (prio << XGMAC_PSRQ_SHIFT(queue)) & XGMAC_PSRQ(queue);

	writel(value, ioaddr + reg);
}

static void dwxgmac2_tx_queue_prio(struct mac_device_info *hw, u32 prio,
				   u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, reg;

	if (HW_IS_VF(hw))
		return;
	queue += DN200_RXQ_START_GET(hw);
	reg = (queue < 4) ? XGMAC_TC_PRTY_MAP0 : XGMAC_TC_PRTY_MAP1;
	if (queue >= 4)
		queue -= 4;

	value = readl(ioaddr + reg);
	value &= ~XGMAC_PSTC(queue);
	value |= (prio << XGMAC_PSTC_SHIFT(queue)) & XGMAC_PSTC(queue);

	writel(value, ioaddr + reg);
}

static void dwxgmac2_prog_mtl_rx_algorithms(struct mac_device_info *hw,
					    u32 rx_alg)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_MTL_OPMODE);
	value &= ~XGMAC_RAA;

	switch (rx_alg) {
	case MTL_RX_ALGORITHM_SP:
		break;
	case MTL_RX_ALGORITHM_WSP:
		value |= XGMAC_RAA;
		break;
	default:
		break;
	}

	writel(value, ioaddr + XGMAC_MTL_OPMODE);
}

static void dwxgmac2_prog_mtl_tx_algorithms(struct mac_device_info *hw,
					    u32 tx_alg)
{
	void __iomem *ioaddr = hw->pcsr;
	bool ets = true;
	u32 value;
	int i;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_MTL_OPMODE);
	value &= ~XGMAC_ETSALG;

	switch (tx_alg) {
	case MTL_TX_ALGORITHM_WRR:
		value |= XGMAC_WRR;
		break;
	case MTL_TX_ALGORITHM_WFQ:
		value |= XGMAC_WFQ;
		break;
	case MTL_TX_ALGORITHM_DWRR:
		value |= XGMAC_DWRR;
		break;
	default:
		ets = false;
		break;
	}

	writel(value, ioaddr + XGMAC_MTL_OPMODE);

	/* Set ETS if desired */
	for (i = 0; i < MTL_MAX_TX_QUEUES; i++) {
		value = readl(ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
		value &= ~XGMAC_TSA;
		if (ets)
			value |= XGMAC_ETS;
		writel(value, ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
	}
}

static void dwxgmac2_set_mtl_tx_queue_weight(struct mac_device_info *hw,
					     u32 weight, u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;

	queue += DN200_RXQ_START_GET(hw);
	writel(weight, ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(queue));
}

static void dwxgmac2_set_mtl_rx_queue_weight(struct mac_device_info *hw,
					     u32 weight, u32 queue)
{
	u32 value;
	void __iomem *ioaddr = hw->pcsr;

	queue += DN200_RXQ_START_GET(hw);

	value = readl(ioaddr + XGMAC_MTL_RXQ_WEIGHT(queue));
	value &= ~XGMAC_RXQ_WEIGHT;
	value |= weight & XGMAC_RXQ_WEIGHT;
	writel(value, ioaddr + XGMAC_MTL_RXQ_WEIGHT(queue));
}

static void dwxgmac2_mtl_reset(struct mac_device_info *hw, u32 queue, u32 chan,
			       u8 mode)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, reg, que;
	int i;

	queue += DN200_RXQ_START_GET(hw);
	chan += DN200_RXQ_START_GET(hw);
	/* weight reset */
	writel(0, ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(queue));

	/*cbs */
	writel(0, ioaddr + XGMAC_MTL_TCx_SENDSLOPE(queue));
	writel(0, ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(queue));
	writel(0, ioaddr + XGMAC_MTL_TCx_HICREDIT(queue));
	writel(0, ioaddr + XGMAC_MTL_TCx_LOCREDIT(queue));
	writel(0, ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(queue));

	/*map */
	reg = (queue / 4) * 4 + XGMAC_MTL_RXQ_DMA_MAP0;
	que = queue % 4;
	value = readl(ioaddr + reg);
	value &= ~XGMAC_QxMDMACH(que);
	writel(value, ioaddr + reg);

	/*disable queue */
	value = readl(ioaddr + XGMAC_RXQ_CTRL0) & ~XGMAC_RXQEN(queue);
	writel(value, ioaddr + XGMAC_RXQ_CTRL0);
	value = readl(ioaddr + XGMAC_RXQ_CTRL0) & ~XGMAC_RXQEN(queue);
	for (i = 0; i < XGMAC_PER_REGSIZE - 0x24; i = i + 4)
		writel(0, ioaddr + XGMAC_MTL_TXQ_OPMODE(queue) + i);

	writel(readl(ioaddr + XGMAC_MTL_QINTEN(queue)) & (~(BIT(16))),
	       ioaddr + XGMAC_MTL_QINTEN(queue));
	writel(~0, ioaddr + XGMAC_MTL_QINT_STATUS(queue));
	writel(readl(ioaddr + XGMAC_MTL_QINT_STATUS(queue)) & (~(BIT(1))),
	       ioaddr + XGMAC_MTL_QINT_STATUS(queue));
}

static int dwxgmac2_mtl_flush(struct mac_device_info *hw, u32 queue)
{
	u32 val;
	void __iomem *ioaddr = hw->pcsr;

	queue += DN200_RXQ_START_GET(hw);
	val = readl(ioaddr + XGMAC_MTL_TXQ_OPMODE(queue));
	writel((val | 1), ioaddr + XGMAC_MTL_TXQ_OPMODE(queue));
	/* Wait for done */
	return readl_poll_timeout(ioaddr + XGMAC_MTL_TXQ_OPMODE(queue),
			   val, !(val & BIT(0)), 100, 100000);
}

static void dwxgmac2_map_mtl_to_dma(struct mac_device_info *hw, u32 queue,
				    u32 chan)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, reg;

	queue += DN200_RXQ_START_GET(hw);
	chan += DN200_RXQ_START_GET(hw);
	reg = (queue / 4) * 4 + XGMAC_MTL_RXQ_DMA_MAP0;
	queue %= 4;
	value = readl(ioaddr + reg);
	value &= ~XGMAC_QxMDMACH(queue);
	value |= (chan << XGMAC_QxMDMACH_SHIFT(queue)) & XGMAC_QxMDMACH(queue);

	writel(value, ioaddr + reg);
}

static void dwxgmac2_mtl_dynamic_chan_set(struct mac_device_info *hw,
					  u32 queue, bool dynamic)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, reg;

	queue += DN200_RXQ_START_GET(hw);
	if (!DN200_MTL_QUEUE_IS_VALID(hw->priv, queue))
		return;

	reg = (queue / 4) * 4 + XGMAC_MTL_RXQ_DMA_MAP0;
	queue %= 4;
	value = readl(ioaddr + reg);
	if (dynamic)
		value |= XGMAC_QxMDMACH_DYN_SEL(queue);
	else
		value &= ~XGMAC_QxMDMACH_DYN_SEL(queue);

	writel(value, ioaddr + reg);
}

static void dwxgmac2_config_cbs(struct mac_device_info *hw,
				u32 send_slope, u32 idle_slope,
				u32 high_credit, u32 low_credit, u32 queue)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	queue += DN200_RXQ_START_GET(hw);
	writel(send_slope, ioaddr + XGMAC_MTL_TCx_SENDSLOPE(queue));
	writel(idle_slope, ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(queue));
	writel(high_credit, ioaddr + XGMAC_MTL_TCx_HICREDIT(queue));
	writel(low_credit, ioaddr + XGMAC_MTL_TCx_LOCREDIT(queue));

	value = readl(ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(queue));
	value &= ~XGMAC_TSA;
	value |= XGMAC_CC | XGMAC_CBS;
	writel(value, ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(queue));
}

static void dwxgmac2_dump_regs(struct mac_device_info *hw, u32 *reg_space)
{
	void __iomem *ioaddr = hw->pcsr;
	int i;

	for (i = 0; i < XGMAC_MAC_REGSIZE; i++)
		reg_space[i] = readl(ioaddr + i * 4);
}

static int dwxgmac2_host_irq_status(struct mac_device_info *hw,
				    struct dn200_extra_stats *x)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 stat, en;
	int ret = 0;

	en = readl(ioaddr + XGMAC_INT_EN);
	stat = readl(ioaddr + XGMAC_INT_STATUS);

	stat &= en;

	if (stat & XGMAC_PMTIS) {
		x->irq_receive_pmt_irq_n++;
		readl(ioaddr + XGMAC_PMT);
	}

	if (stat & XGMAC_LPIIS) {
		u32 lpi = readl(ioaddr + XGMAC_LPI_CTRL);

		if (lpi & XGMAC_TLPIEN) {
			ret |= CORE_IRQ_TX_PATH_IN_LPI_MODE;
			x->irq_tx_path_in_lpi_mode_n++;
		}
		if (lpi & XGMAC_TLPIEX) {
			ret |= CORE_IRQ_TX_PATH_EXIT_LPI_MODE;
			x->irq_tx_path_exit_lpi_mode_n++;
		}
		if (lpi & XGMAC_RLPIEN)
			x->irq_rx_path_in_lpi_mode_n++;
		if (lpi & XGMAC_RLPIEX)
			x->irq_rx_path_exit_lpi_mode_n++;
	}

	return ret;
}

static int dwxgmac2_host_mtl_irq_status(struct mac_device_info *hw, u32 chan)
{
	void __iomem *ioaddr = hw->pcsr;
	int ret = 0;
	u32 status;

	chan += DN200_RXQ_START_GET(hw);
	status = readl(ioaddr + XGMAC_MTL_INT_STATUS);
	if (status & BIT(chan)) {
		u32 chan_status = readl(ioaddr + XGMAC_MTL_QINT_STATUS(chan));

		if (chan_status & XGMAC_RXOVFIS)
			ret |= CORE_IRQ_MTL_RX_OVERFLOW;

		writel(~0x0, ioaddr + XGMAC_MTL_QINT_STATUS(chan));
	}

	return ret;
}

static void dwxgmac2_flow_ctrl(struct mac_device_info *hw, unsigned int duplex,
			       unsigned int fc, unsigned int pause_time,
			       u32 tx_cnt)
{
	void __iomem *ioaddr = hw->pcsr;
	struct dn200_priv *priv = hw->priv;
	u32 i;

	if (fc & FLOW_RX) {
		writel(XGMAC_RFE, ioaddr + XGMAC_RX_FLOW_CTRL);
	} else {
		if (priv->pfc && priv->pfc->pfc_en) {
			/* pfc mode enabled, do not disable flow control */
		} else {
			u32 value = readl(ioaddr + XGMAC_RX_FLOW_CTRL);

			writel(value & ~XGMAC_RFE, ioaddr + XGMAC_RX_FLOW_CTRL);
		}
	}
	if (fc & FLOW_TX) {
		for (i = 0; i < tx_cnt; i++) {
			u32 value = XGMAC_TFE;

			if (duplex)
				value |= pause_time << XGMAC_PT_SHIFT;

			writel(value,
			       ioaddr +
			       XGMAC_Qx_TX_FLOW_CTRL((i +
						      DN200_RXQ_START_GET
						      (hw))));
		}
	} else {
		if (priv->pfc && priv->pfc->pfc_en) {
			/* pfc mode enabled, do not disable flow control */
		} else {
			for (i = 0; i < tx_cnt; i++) {
				u32 value =
				    readl(ioaddr +
					  XGMAC_Qx_TX_FLOW_CTRL((i +
								 DN200_RXQ_START_GET
								 (hw))));

				value &= ~XGMAC_TFE;
				value &= ~(0xffff << XGMAC_PT_SHIFT);

				writel(value,
				       ioaddr +
				       XGMAC_Qx_TX_FLOW_CTRL((i +
							      DN200_RXQ_START_GET
							      (hw))));
			}
		}
	}
}

static int dwxgmac2_indiraccess_write(struct mac_device_info *hw, u32 addr_off,
				      u8 mode_sel, u32 data);
static int dwxgmac2_wq_set_umac_addr(struct mac_device_info *hw,
				   unsigned char *addr, unsigned int reg_n,
				   struct dn200_vf_rxp_async_info *async_info)
{
	u32 value;
	int ret = 0;

	if (async_info->is_vf) {
		reg_n += 1 + async_info->vf_offset;
		ret = dn200_del_vf_uc_rxp_da_route(hw,
						DN200_VF_UC_OFF + async_info->vf_offset);
		if (ret < 0) {
			netdev_err(hw->priv->dev, "%s: del vf uc fail, ret:%d.\n", __func__, ret);
			goto XDCS;
		}

		ret = dn200_add_vf_uc_rxp_da_route_sriov(hw, &async_info->uc_mac_addr[0],
								DN200_VF_UC_OFF + async_info->vf_offset, async_info->rxq_start);
		if (ret < 0) {
			netdev_err(hw->priv->dev, "%s: add vf uc fail, ret:%d.\n", __func__, ret);
			goto XDCS;
		}
	} else {
		ret = dn200_del_pf_uc_rxp_da_route(hw, 1);
		if (ret < 0) {
			netdev_err(hw->priv->dev, "%s: del pf uc fail, ret:%d.\n", __func__, ret);
			goto XDCS;
		}
		ret = dn200_add_pf_uc_rxp_da_route_sriov(hw, addr, 1);
		if (ret < 0) {
			netdev_err(hw->priv->dev, "%s: add pf uc fail, ret:%d.\n", __func__, ret);
			goto XDCS;
		}
	}
XDCS:
	//enable XDCS
	value = 1 << async_info->rxq_start;
	ret = dwxgmac2_indiraccess_write(hw, (reg_n & 0x1f), XGMAC_INDIR_DCHSEL,
					value);
	return ret;
}

static void dwxgmac2_vf_set_async_info(struct mac_device_info *hw,
				      struct net_device *dev, u8 *wakeup_wq,
					  u8 type);

static int dwxgmac2_set_umac_addr(struct mac_device_info *hw,
				   unsigned char *addr, unsigned int reg_n, u8 *wakeup_wq)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret = 0;

	if (HW_IS_VF(hw)) {
		dwxgmac2_vf_set_async_info(hw, hw->priv->dev, wakeup_wq, DN200_VF_SET_UMAC);
		return 0;
	} else if (!HW_IS_PUREPF(hw)) {
		*wakeup_wq = true;
		return 0;
	}

	value = (DN200_RXQ_START_GET(hw) << 16) | (addr[5] << 8) | addr[4];
	writel(value | XGMAC_AE, ioaddr + XGMAC_ADDRX_HIGH(reg_n));

	value = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	writel(value, ioaddr + XGMAC_ADDRX_LOW(reg_n));
	return ret;
}

static void dwxgmac2_get_umac_addr(struct mac_device_info *hw,
				   unsigned char *addr, unsigned int reg_n)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 hi_addr, lo_addr;

	reg_n += DN200_RXQ_START_GET(hw);
	/* Read the MAC address from the hardware */
	hi_addr = readl(ioaddr + XGMAC_ADDRX_HIGH(reg_n));
	lo_addr = readl(ioaddr + XGMAC_ADDRX_LOW(reg_n));

	/* Extract the MAC address from the high and low words */
	addr[0] = lo_addr & 0xff;
	addr[1] = (lo_addr >> 8) & 0xff;
	addr[2] = (lo_addr >> 16) & 0xff;
	addr[3] = (lo_addr >> 24) & 0xff;
	addr[4] = hi_addr & 0xff;
	addr[5] = (hi_addr >> 8) & 0xff;
}

static void dwxgmac2_set_eee_mode(struct mac_device_info *hw,
				  bool en_tx_lpi_clockgating, bool en_tx_lpi_auto_timer)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_LPI_CTRL);

	value |= XGMAC_LPITXEN | XGMAC_LPITXA;
	if (en_tx_lpi_clockgating)
		value |= XGMAC_TXCGE;

	if (en_tx_lpi_auto_timer)
		value |= XGMAC_LPIATE;
	else
		value &= ~XGMAC_LPIATE;

	writel(value, ioaddr + XGMAC_LPI_CTRL);
}

static void dwxgmac2_reset_eee_mode(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_LPI_CTRL);
	value &= ~(XGMAC_LPITXEN | XGMAC_LPITXA | XGMAC_TXCGE | XGMAC_LPIATE);
	writel(value, ioaddr + XGMAC_LPI_CTRL);
}

static void dwxgmac2_set_eee_pls(struct mac_device_info *hw, int link)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	value = readl(ioaddr + XGMAC_LPI_CTRL);
	if (link)
		value |= XGMAC_PLS;
	else
		value &= ~XGMAC_PLS;
	writel(value, ioaddr + XGMAC_LPI_CTRL);
}

static void dwxgmac2_set_eee_timer(struct mac_device_info *hw, int ls, int tw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = (tw & 0xffff) | ((ls & 0x3ff) << 16);
	writel(value, ioaddr + XGMAC_LPI_TIMER_CTRL);
}

static void dwxgmac2_set_mchash(void __iomem *ioaddr, u32 *mcfilterbits,
				int mcbitslog2)
{
	int numhashregs, regs;

	switch (mcbitslog2) {
	case 6:
		numhashregs = 2;
		break;
	case 7:
		numhashregs = 4;
		break;
	case 8:
		numhashregs = 8;
		break;
	default:
		return;
	}

	for (regs = 0; regs < numhashregs; regs++)
		writel(mcfilterbits[regs], ioaddr + XGMAC_HASH_TABLE(regs));
}

static void dwxgamc_get_change(struct mac_device_info *hw,
			       struct net_device *dev)
{
	unsigned long bitmap_am;
	unsigned long bitmap_pm;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, &bitmap_am);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, &bitmap_pm);
	if (bitmap_am & (1 << DN200_RXQ_START_GET(hw)))
		hw->set_state.is_allmuslt = true;
	else
		hw->set_state.is_allmuslt = false;

	if (bitmap_pm & (1 << DN200_RXQ_START_GET(hw)))
		hw->set_state.is_promisc = true;
	else
		hw->set_state.is_promisc = false;

	hw->set_state.uc_num =
	    netdev_uc_count(dev) >
	    DN200_MAX_UC_MAC_ADDR_NUM ? DN200_MAX_UC_MAC_ADDR_NUM :
	    netdev_uc_count(dev);
	hw->set_state.mc_num =
	    netdev_mc_count(dev) >
	    DN200_MAX_MC_ADDR_NUM ? DN200_MAX_MC_ADDR_NUM :
	    netdev_mc_count(dev);
}

static void dwxgamc_set_vf_change(struct mac_device_info *hw,
			       struct net_device *dev, u8 rxq_start, u32 seq)
{
	unsigned long bitmap_am;
	unsigned long bitmap_pm;
	struct dn200_vf_rxp_async_wb wb;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, &bitmap_am);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, &bitmap_pm);
	if (bitmap_am & (1 << rxq_start))
		wb.is_promisc = true;
	else
		wb.is_promisc = false;

	if (bitmap_pm & (1 << rxq_start))
		wb.is_allmuslt = true;
	else
		wb.is_allmuslt = false;

	wb.uc_num = 0;
	wb.mc_num =
	    netdev_mc_count(dev) >
	    DN200_MAX_MC_ADDR_NUM ? DN200_MAX_MC_ADDR_NUM :
	    netdev_mc_count(dev);
	wb.seq = seq;
	wb.crc32 = crc32_le(~0, (u8 *)&wb + sizeof(u32), sizeof(wb) - sizeof(u32));
	dn200_set_lram_rxp_wb_info(hw, (u8 *)&wb);
}

static int dwxgamc_comp_change(struct mac_device_info *hw,
			       struct net_device *dev)
{
	int ret = 1;
	int uc_num = 0;
	int mc_num = 0;

	if (netdev_uc_count(dev) > DN200_MAX_UC_MAC_ADDR_NUM)
		uc_num = DN200_MAX_UC_MAC_ADDR_NUM;
	else
		uc_num = netdev_uc_count(dev);
	if (netdev_mc_count(dev) > DN200_MAX_MC_ADDR_NUM)
		mc_num = DN200_MAX_MC_ADDR_NUM;
	else
		mc_num = netdev_mc_count(dev);

	if ((dev->flags & IFF_PROMISC)) {
		if (!hw->set_state.is_promisc)
			return ret;
	} else {
		if (hw->set_state.is_promisc)
			return ret;
	}
	if (dev->flags & IFF_ALLMULTI) {
		if (!hw->set_state.is_allmuslt)
			return ret;
	} else {
		if (hw->set_state.is_allmuslt)
			return ret;
	}
	if (hw->set_state.uc_num != uc_num)
		return ret;

	if (hw->set_state.mc_num != mc_num)
		return ret;
	return 0;
}

static void dwxgmac2_vf_set_async_info(struct mac_device_info *hw,
				      struct net_device *dev, u8 *wakeup_wq,
					  u8 type)
{
	size_t info_size = sizeof(struct dn200_vf_rxp_async_info);
	struct dn200_vf_rxp_async_info *async_info = NULL;
	struct netdev_hw_addr *ha;
	u8 mc_of = 0;
	u32 crc32, tmp_crc32;

	dn200_get_lram_rxp_async_crc32(hw, DN200_VF_OFFSET_GET(hw), (u8 *)&tmp_crc32);
	async_info = devm_kzalloc(hw->priv->device, info_size, GFP_ATOMIC);
	if (!async_info)
		return;
	//to do, delete this
	dn200_get_lram_rxp_async_info(hw, (u8 *)async_info, DN200_VF_OFFSET_GET(hw));
	netdev_dbg(hw->priv->dev, "%s %d cur seq %d tmp seq %d\n", __func__, __LINE__, async_info->seq, hw->cfg_rxp_seq);
	memset(async_info, 0, info_size);

	async_info->flags = dev->flags;
	async_info->vf_offset = DN200_VF_OFFSET_GET(hw);
	async_info->seq = hw->cfg_rxp_seq + 1;
	async_info->mc_cnt = netdev_mc_count(dev);
	async_info->uc_cnt = netdev_uc_count(dev);
	ether_addr_copy((u8 *)&async_info->uc_mac_addr[0], dev->dev_addr);

	netdev_for_each_mc_addr(ha, dev) {
		ether_addr_copy((u8 *)&async_info->mc_mac_addr[mc_of], ha->addr);
		mc_of++;
		if (mc_of >= DN200_MAX_MC_ADDR_NUM)
			break;
	}
	async_info->rxq_start = DN200_RXQ_START_GET(hw);
	async_info->type = type;
	async_info->is_vf = true;
	crc32 = crc32_le(~0, (u8 *)async_info + sizeof(u32), info_size - sizeof(u32));
	async_info->crc32 = crc32;
	netdev_dbg(hw->priv->dev, "%s %d new crc32 %#x old crc32 %#x seq %d\n", __func__, __LINE__, crc32, tmp_crc32, async_info->seq);
	if (crc32 != tmp_crc32) {
		dn200_reset_lram_rxp_async_info(hw);
		dn200_set_lram_rxp_async_info(hw, (u8 *)async_info);
		hw->cfg_rxp_seq++;
		*wakeup_wq = true;
	}
	devm_kfree(hw->priv->device, async_info);
}

static void dwxgmac2_set_filter_sriov(struct mac_device_info *hw,
				      struct net_device *dev, u8 *wakeup_wq)
{
	int ret = 0;

	*wakeup_wq = false;
	if (HW_IS_VF(hw)) {
		dwxgmac2_vf_set_async_info(hw, dev, wakeup_wq, DN200_VF_SET_FLT);
	} else {
		ret = dwxgamc_comp_change(hw, dev);
		if (!ret)
			return;
		*wakeup_wq = true;
	}
}

static void dwxgmac2_wq_set_filter(struct mac_device_info *hw,
				      struct net_device *dev, bool is_vf,
					  struct dn200_vf_rxp_async_info *async_info)
{
	unsigned long bitmap_am;
	unsigned long bitmap_pm;
	u8 rxq_start = is_vf ? async_info->rxq_start : 0;
	u8 i = 0;
	u64 flags = is_vf ? async_info->flags : dev->flags;

	if (!is_vf)
		dn200_clear_lram_pf_uc_rxp(hw);

	/*clear mp and ap */
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, &bitmap_am);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, &bitmap_pm);

	dn200_mc_rxp_channel_route_set(hw, false,
					 (1 << rxq_start));
	dn200_clear_mc_da_route(hw, rxq_start);	/*pf and vf */

	if (flags & IFF_ALLMULTI) {
		bitmap_am |= (1 << rxq_start);
		dev_dbg(hw->priv->device, "allmulitcast mode\n");
	} else {
		bitmap_am &= (~(1 << rxq_start));
		dev_dbg(hw->priv->device, "no allmulitcast mode\n");
	}
	if (flags & IFF_PROMISC) {
		bitmap_pm |= (1 << rxq_start);
		bitmap_am |= (1 << rxq_start);
		dev_dbg(hw->priv->device, "promisc mode\n");
	} else {
		bitmap_pm &= (~(1 << rxq_start));
		dev_dbg(hw->priv->device, "no promisc mode\n");
	}
	if (is_vf) {
		if (async_info->mc_cnt && (async_info->flags & IFF_MULTICAST)) {
			for (i = 0; i < async_info->mc_cnt; i++)
				dn200_mc_add_rxp(hw, (u8 *)&async_info->mc_mac_addr[i], rxq_start);
		}
	} else {
		if (!netdev_mc_empty(dev) && (dev->flags & IFF_MULTICAST)) {
			struct netdev_hw_addr *ha;

			netdev_for_each_mc_addr(ha, dev) {
				dn200_mc_add_rxp(hw, ha->addr, rxq_start);
			}
		}

	}

	if (is_vf) {
		if (async_info->uc_cnt)
			bitmap_pm |= (1 << rxq_start);
	} else {
		/* Handle multiple unicast addresses */
		if (netdev_uc_count(dev) > DN200_MAX_UC_MAC_ADDR_NUM) {
			bitmap_pm |= (1 << DN200_RXQ_START_GET(hw));
		} else {
			struct netdev_hw_addr *ha;

			netdev_for_each_uc_addr(ha, dev)
				dn200_pf_lram_uc_add_rxp(hw, ha->addr);
		}
	}
	dn200_update_bcmc_channel(hw, 0, true, rxq_start);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, bitmap_pm);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, bitmap_am);
	dn200_mc_rxp_channel_route_set(hw, true, (u16)bitmap_pm);
	dn200_update_ampm_rxp(hw, rxq_start);
	if (!is_vf)
		dwxgamc_get_change(hw, dev);
	else
		dwxgamc_set_vf_change(hw, dev, async_info->rxq_start, async_info->seq);
}

static void dwxgmac2_set_filter_purepf(struct mac_device_info *hw,
				       struct net_device *dev, u8 *wakeup_wq)
{
	void __iomem *ioaddr = (void __iomem *)dev->base_addr;
	u32 value = readl(ioaddr + XGMAC_PACKET_FILTER);
	int mcbitslog2 = hw->mcast_bits_log2;
	u32 mc_filter[8];
	int i;

	/*Close vlan filter when promisc on */
	value &= ~(XGMAC_FILTER_PR | XGMAC_FILTER_HMC |
		   XGMAC_FILTER_PM | XGMAC_FILTER_PCF | XGMAC_FILTER_VTFE);
	value |= XGMAC_FILTER_HPF;

	if ((dev->features & NETIF_F_HW_VLAN_CTAG_FILTER) &&
	    !(dev->flags & IFF_PROMISC)) {
		value |= XGMAC_FILTER_VTFE;
	}

	memset(mc_filter, 0, sizeof(mc_filter));
	value |= (0x3 << XGMAC_FILTER_PCF_SHIFT);
	if (dev->flags & IFF_PROMISC) {
		value |= XGMAC_FILTER_PR;
		value &= ~XGMAC_FILTER_PCF;
		value |= (0x2 << XGMAC_FILTER_PCF_SHIFT);
	} else if ((dev->flags & IFF_ALLMULTI) ||
		   (netdev_mc_count(dev) > hw->multicast_filter_bins)) {
		value |= XGMAC_FILTER_PM;

		for (i = 0; i < XGMAC_MAX_HASH_TABLE; i++)
			writel(~0x0, ioaddr + XGMAC_HASH_TABLE(i));
	} else if (!netdev_mc_empty(dev) && (dev->flags & IFF_MULTICAST)) {
		struct netdev_hw_addr *ha;

		value |= XGMAC_FILTER_HMC;

		netdev_for_each_mc_addr(ha, dev) {
			u32 nr = (bitrev32(~crc32_le(~0, ha->addr, 6)) >>
				  (32 - mcbitslog2));
			mc_filter[nr >> 5] |= (1 << (nr & 0x1F));
		}
	}

	dwxgmac2_set_mchash(ioaddr, mc_filter, mcbitslog2);

	/* Handle multiple unicast addresses */
	if (netdev_uc_count(dev) > hw->unicast_filter_entries) {
		value |= XGMAC_FILTER_PR;
	} else {
		struct netdev_hw_addr *ha;
		int reg = 1;

		netdev_for_each_uc_addr(ha, dev) {
			dwxgmac2_set_umac_addr(hw, ha->addr, reg, NULL);
			reg++;
		}

		for (; reg < XGMAC_ADDR_MAX; reg++) {
			writel(0, ioaddr + XGMAC_ADDRX_HIGH(reg));
			writel(0, ioaddr + XGMAC_ADDRX_LOW(reg));
		}
	}
	writel(value, ioaddr + XGMAC_PACKET_FILTER);
}

static void dwxgmac2_hw_vlan_init(struct mac_device_info *hw);
static void dwxgmac2_sriov_init_rxp_vlan_route(struct mac_device_info *hw)
{
	u32 channel = 0;
	u32 vlan_tag = 0;
	u32 proto = 0;
	struct RXP_FPR_ENTRY frp_entry_data[2];

	if (HW_IS_VF(hw))
		return;

	if (!HW_IS_VF(hw))
		dwxgmac2_hw_vlan_init(hw);

	channel = (1 << DN200_RXQ_START_GET(hw));
	proto = (htons(ETH_P_8021Q) & 0xffff);
	/*Init vid proto entry */
	memset(&frp_entry_data[0], 0, sizeof(struct RXP_FPR_ENTRY));
	vlan_tag =
	    (htons((u32)0 & 0xfff) << 16) | (htons(ETH_P_8021Q) & 0xffff);
	frp_entry_data[0].match_data = cpu_to_le32(proto);
	frp_entry_data[0].match_en = 0xffff;
	frp_entry_data[0].af = 1;
	frp_entry_data[0].rf = 1;
	frp_entry_data[0].nc = 0;
	frp_entry_data[0].im = 1;
	frp_entry_data[0].dma_ch_no = channel;
	frp_entry_data[0].ok_index = DN200_VLAN_ADDR_START + 1;	//to all drop
	frp_entry_data[0].frame_offset = 3;

	/*Init first vid entry, vid 0 */
	memset(&frp_entry_data[1], 0, sizeof(struct RXP_FPR_ENTRY));
	frp_entry_data[1].match_data = vlan_tag;
	frp_entry_data[1].match_en = 0xffffffff;
	frp_entry_data[1].af = 1;
	frp_entry_data[1].rf = 1;
	frp_entry_data[1].nc = 1;
	frp_entry_data[1].dma_ch_no = channel;
	frp_entry_data[1].ok_index = DN200_ALL_DROP_OFF;	//to all drop
	frp_entry_data[1].frame_offset = 3;

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  DN200_VLAN_ADDR_START);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  DN200_VLAN_ADDR_START + 1);
	dwxgmac3_rxp_enable(hw->pcsr);
}

static int dwxgmac2_pf_add_rxp_vlan_route(struct net_device *dev,
					  struct mac_device_info *hw,
					  __be16 proto, u16 vid, uint8_t off,
					  bool is_last)
{
	u32 channel = 0;
	u32 vlan_tag = 0;
	u8 rxp_off = 0;
	u16 oki = 0;
	struct RXP_FPR_ENTRY frp_entry_data[1];

	if (HW_IS_VF(hw))
		return 0;
	rxp_off = DN200_VLAN_ADDR_START + off + 1;
	oki = rxp_off + 1;

	if (is_last)
		oki = DN200_ALL_DROP_OFF;	//to all drop
	channel = (1 << DN200_RXQ_START_GET(hw));
	vlan_tag = (htons((u32)vid & 0xfff) << 16);
	memset(&frp_entry_data[0], 0, sizeof(struct RXP_FPR_ENTRY));
	frp_entry_data[0].match_data = cpu_to_le32(vlan_tag);
	frp_entry_data[0].match_en = 0xff0f0000;
	frp_entry_data[0].af = 1;
	frp_entry_data[0].rf = 1;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].dma_ch_no = channel;

	frp_entry_data[0].ok_index = oki;
	frp_entry_data[0].frame_offset = 3;

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  rxp_off);
	return 0;
}

static int dwxgmac2_pf_del_rxp_vlan_route(struct net_device *dev,
					  struct mac_device_info *hw,
					  __be16 proto, u16 vid, uint8_t off,
					  bool is_last)
{
	u32 channel = 0;
	u8 rxp_off = 0;
	u16 oki = 0;
	struct RXP_FPR_ENTRY frp_entry_data[1];

	if (HW_IS_VF(hw))
		return 0;
	rxp_off = DN200_VLAN_ADDR_START + off + 1;
	oki = rxp_off + 1;
	if (is_last)
		oki = DN200_ALL_DROP_OFF;	//to all drop

	channel = (1 << DN200_RXQ_START_GET(hw));
	memset(&frp_entry_data[0], 0, sizeof(struct RXP_FPR_ENTRY));
	frp_entry_data[0].match_data = 0;
	frp_entry_data[0].match_en = 0xffff0000;
	frp_entry_data[0].af = 1;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].dma_ch_no = channel;

	frp_entry_data[0].ok_index = oki;
	frp_entry_data[0].frame_offset = 3;

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  rxp_off);
	return 0;
}

static void dwxgmac2_rxp_vlan_filter_config(struct mac_device_info *hw,
					    bool enable)
{
	u32 data;
	unsigned long bitmap;
	u32 prev_off_val = 0;
	int prev_off;
	u8 off = 1 + 1 + 15;
	u8 i = 1 + 1 + 15;

	if (HW_IS_VF(hw))
		return;

	if (hw->priv->plat_ex->vlan_num > (hw->max_vlan_num + 1))
		enable = false;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	while (i > 0) {
		prev_off = dn200_get_prev_used_bit(&bitmap, off);
		prev_off_val = rxp_offset_get_from_bitmap(prev_off);
		if (!prev_off)
			break;

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    AFRFNC_ENTRY_OFFSET
						    (prev_off_val + 1));
		if (enable) {
			data &= (~RF_ENABLE);
			data &= (~AF_ENABLE);
		} else {
			data |= RF_ENABLE;
			data |= AF_ENABLE;
		}
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       AFRFNC_ENTRY_OFFSET
						       (prev_off_val + 1));

		off = prev_off;
		i--;
	}
}

static int dwxgmac2_set_mac_loopback(void __iomem *ioaddr, bool enable)
{
	u32 value = readl(ioaddr + XGMAC_RX_CONFIG);

	if (enable)
		value |= XGMAC_CONFIG_LM;
	else
		value &= ~XGMAC_CONFIG_LM;

	writel(value, ioaddr + XGMAC_RX_CONFIG);
	return 0;
}

static int dwxgmac2_rss_write_reg(void __iomem *ioaddr, bool is_key, int idx,
				  u32 val)
{
	u32 ctrl = 0;
	int ret = 0;
	int retry = 0;

retry:
	writel(val, ioaddr + XGMAC_RSS_DATA);
	ctrl |= (idx & 0xFFF) << XGMAC_RSSIA_SHIFT;
	ctrl |= is_key ? XGMAC_ADDRT : 0x0;
	ctrl |= XGMAC_OB;
	writel(ctrl, ioaddr + XGMAC_RSS_ADDR);

	ret = readl_poll_timeout_atomic(ioaddr + XGMAC_RSS_ADDR, ctrl,
					!(ctrl & XGMAC_OB), 10, 10000);
	if (ret && !retry) {
		retry++;
		writel(0, ioaddr + XGMAC_RSS_ADDR);
		goto retry;
	}
	return ret;
}

static int dwxgmac2_rss_configure(struct mac_device_info *hw,
				  struct dn200_rss *cfg, u32 num_rxq)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, *key;
	int i, ret;
	u32 *table = NULL;
	u32 *tmp_table = NULL;

	if (HW_IS_VF(hw))
		return 0;
	value = readl(ioaddr + XGMAC_RSS_CTRL);
	if (!PRIV_SRIOV_SUPPORT(hw->priv)) {
		if (!cfg || !cfg->enable) {
			value &= ~XGMAC_RSSE;
			writel(value, ioaddr + XGMAC_RSS_CTRL);
			return 0;
		}
		tmp_table = cfg->table;
	} else {
		if (!cfg) {
			return 0;
		} else if (cfg && !cfg->enable) {
			table = devm_kzalloc(hw->priv->device,
				 sizeof(u32) * DN200_RSS_MAX_TABLE_SIZE, GFP_ATOMIC);
			if (!table)
				return -ENOMEM;
			tmp_table = table;
		} else {
			tmp_table = cfg->table;
		}
	}
	key = (u32 *)cfg->key;
	for (i = 0; i < (ARRAY_SIZE(cfg->key) / sizeof(u32)); i++) {
		ret = dwxgmac2_rss_write_reg(ioaddr, true, i, key[i]);
		if (ret) {
			value = readl(ioaddr + XGMAC_RSS_ADDR);
			netdev_err(hw->priv->dev,
				   "%s: %d poll_timeout.RSS_ADDR's val = 0x%x\n",
				   __func__, __LINE__, value);
			goto free_table;
		}
	}

	for (i = 0; i < DN200_RSS_MAX_TABLE_SIZE; i++) {
		ret = dwxgmac2_rss_write_reg(ioaddr, false, i, tmp_table[i]);
		if (ret) {
			value = readl(ioaddr + XGMAC_RSS_ADDR);
			netdev_err(hw->priv->dev,
				   "%s: %d poll_timeout.RSS_ADDR's val = 0x%x\n",
				   __func__, __LINE__, value);
			goto free_table;
		}
	}

	if (cfg->rss_flags & DN200_RSS_IP2TE)
		value |= XGMAC_IP2TE;

	if (cfg->rss_flags & DN200_RSS_UDP4TE)
		value |= XGMAC_UDP4TE;

	if (cfg->rss_flags & DN200_RSS_TCP4TE)
		value |= XGMAC_TCP4TE;

	value |= XGMAC_RSSE;
	writel(value, ioaddr + XGMAC_RSS_CTRL);

free_table:
	if (table)
		devm_kfree(hw->priv->device, table);
	return ret;
}

static void dwxgmac2_update_vlan_hash(struct mac_device_info *hw, u32 hash,
				      __le16 perfect_match, bool is_double)
{
	void __iomem *ioaddr = hw->pcsr;

	writel(hash, ioaddr + XGMAC_VLAN_HASH_TABLE);
	if (HW_IS_VF(hw))
		return;
	if (hash) {
		u32 value = readl(ioaddr + XGMAC_PACKET_FILTER);

		value |= XGMAC_FILTER_VTFE;

		writel(value, ioaddr + XGMAC_PACKET_FILTER);

		value = readl(ioaddr + XGMAC_VLAN_TAG);

		value |= XGMAC_VLAN_VTHM | XGMAC_VLAN_ETV;
		if (is_double) {
			value |= XGMAC_VLAN_EDVLP;
			value |= XGMAC_VLAN_ESVL;
			value |= XGMAC_VLAN_DOVLTC;
		} else {
			value &= ~XGMAC_VLAN_EDVLP;
			value &= ~XGMAC_VLAN_ESVL;
			value &= ~XGMAC_VLAN_DOVLTC;
		}

		value &= ~XGMAC_VLAN_VID;
		writel(value, ioaddr + XGMAC_VLAN_TAG);
	} else if (perfect_match) {
		u32 value = readl(ioaddr + XGMAC_PACKET_FILTER);

		value |= XGMAC_FILTER_VTFE;

		writel(value, ioaddr + XGMAC_PACKET_FILTER);

		value = readl(ioaddr + XGMAC_VLAN_TAG);

		value &= ~XGMAC_VLAN_VTHM;
		value |= XGMAC_VLAN_ETV;
		if (is_double) {
			value |= XGMAC_VLAN_EDVLP;
			value |= XGMAC_VLAN_ESVL;
			value |= XGMAC_VLAN_DOVLTC;
		} else {
			value &= ~XGMAC_VLAN_EDVLP;
			value &= ~XGMAC_VLAN_ESVL;
			value &= ~XGMAC_VLAN_DOVLTC;
		}

		value &= ~XGMAC_VLAN_VID;
		writel(value | perfect_match, ioaddr + XGMAC_VLAN_TAG);
	} else {
		u32 value = readl(ioaddr + XGMAC_PACKET_FILTER);

		value &= ~XGMAC_FILTER_VTFE;

		writel(value, ioaddr + XGMAC_PACKET_FILTER);

		value = readl(ioaddr + XGMAC_VLAN_TAG);

		value &= ~(XGMAC_VLAN_VTHM | XGMAC_VLAN_ETV);
		value &= ~(XGMAC_VLAN_EDVLP | XGMAC_VLAN_ESVL);
		value &= ~XGMAC_VLAN_DOVLTC;
		value &= ~XGMAC_VLAN_VID;

		writel(value, ioaddr + XGMAC_VLAN_TAG);
	}
}

struct dwxgmac3_error_desc {
	bool valid;
	const char *desc;
	const char *detailed_desc;
};

#define STAT_OFF(field)		offsetof(struct dn200_safety_stats, field)

static void dwxgmac3_log_error(struct net_device *ndev, u32 value, bool corr,
			       const char *module_name,
			       const struct dwxgmac3_error_desc *desc,
			       unsigned long field_offset,
			       struct dn200_safety_stats *stats)
{
	unsigned long loc, mask;
	u8 *bptr = (u8 *)stats;
	unsigned long *ptr;

	ptr = (unsigned long *)(bptr + field_offset);

	mask = value;
	for_each_set_bit(loc, &mask, 32) {
		netdev_err(ndev, "Found %s error in %s: '%s: %s'\n", corr ?
			   "correctable" : "uncorrectable", module_name,
			   desc[loc].desc, desc[loc].detailed_desc);

		/* Update counters */
		ptr[loc]++;
	}
}

static const struct dwxgmac3_error_desc dwxgmac3_mac_errors[32] = {
	{ true, "ATPES", "Application Transmit Interface Parity Check Error" },
	{ true, "DPES", "Descriptor Cache Data Path Parity Check Error" },
	{ true, "TPES", "TSO Data Path Parity Check Error" },
	{ true, "TSOPES", "TSO Header Data Path Parity Check Error" },
	{ true, "MTPES", "MTL Data Path Parity Check Error" },
	{ true, "MTSPES", "MTL TX Status Data Path Parity Check Error" },
	{ true, "MTBUPES", "MAC TBU Data Path Parity Check Error" },
	{ true, "MTFCPES", "MAC TFC Data Path Parity Check Error" },
	{ true, "ARPES",
	 "Application Receive Interface Data Path Parity Check Error" },
	{ true, "MRWCPES", "MTL RWC Data Path Parity Check Error" },
	{ true, "MRRCPES", "MTL RCC Data Path Parity Check Error" },
	{ true, "CWPES", "CSR Write Data Path Parity Check Error" },
	{ true, "ASRPES", "AXI Slave Read Data Path Parity Check Error" },
	{ true, "TTES", "TX FSM Timeout Error" },
	{ true, "RTES", "RX FSM Timeout Error" },
	{ true, "CTES", "CSR FSM Timeout Error" },
	{ true, "ATES", "APP FSM Timeout Error" },
	{ true, "PTES", "PTP FSM Timeout Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 18 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 19 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 20 */
	{ true, "MSTTES", "Master Read/Write Timeout Error" },
	{ true, "SLVTES", "Slave Read/Write Timeout Error" },
	{ true, "ATITES", "Application Timeout on ATI Interface Error" },
	{ true, "ARITES", "Application Timeout on ARI Interface Error" },
	{ true, "FSMPES", "FSM State Parity Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 26 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 27 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 28 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 29 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 30 */
	{ true, "CPI", "Control Register Parity Check Error" },
};

static void dwxgmac3_handle_mac_err(struct net_device *ndev,
				    void __iomem *ioaddr, bool correctable,
				    struct dn200_safety_stats *stats)
{
	u32 value;

	value = readl(ioaddr + XGMAC_MAC_DPP_FSM_INT_STATUS);
	writel(value, ioaddr + XGMAC_MAC_DPP_FSM_INT_STATUS);
	netdev_err(ndev, "dwxgmac come across mac err REG=%#x value=%#x\n",
		   XGMAC_MAC_DPP_FSM_INT_STATUS, value);
	dwxgmac3_log_error(ndev, value, correctable, "MAC", dwxgmac3_mac_errors,
			   STAT_OFF(mac_errors), stats);
}

static const struct dwxgmac3_error_desc dwxgmac3_mtl_errors[32] = {
	{ true, "TXCES", "MTL TX Memory Error" },
	{ true, "TXAMS", "MTL TX Memory Address Mismatch Error" },
	{ true, "TXUES", "MTL TX Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 3 */
	{ true, "RXCES", "MTL RX Memory Error" },
	{ true, "RXAMS", "MTL RX Memory Address Mismatch Error" },
	{ true, "RXUES", "MTL RX Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 7 */
	{ true, "ECES", "MTL EST Memory Error" },
	{ true, "EAMS", "MTL EST Memory Address Mismatch Error" },
	{ true, "EUES", "MTL EST Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 11 */
	{ true, "RPCES", "MTL RX Parser Memory Error" },
	{ true, "RPAMS", "MTL RX Parser Memory Address Mismatch Error" },
	{ true, "RPUES", "MTL RX Parser Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 15 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 16 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 17 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 18 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 19 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 20 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 21 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 22 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 23 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 24 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 25 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 26 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 27 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 28 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 29 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 30 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 31 */
};

static void dwxgmac3_handle_mtl_err(struct net_device *ndev,
				    void __iomem *ioaddr, bool correctable,
				    struct dn200_safety_stats *stats)
{
	u32 value;

	value = readl(ioaddr + XGMAC_MTL_ECC_INT_STATUS);
	writel(value, ioaddr + XGMAC_MTL_ECC_INT_STATUS);
	netdev_err(ndev, "dwxgmac come across mtl err REG=%#x value=%#x\n",
		   XGMAC_MTL_ECC_INT_STATUS, value);
	dwxgmac3_log_error(ndev, value, correctable, "MTL", dwxgmac3_mtl_errors,
			   STAT_OFF(mtl_errors), stats);
}

static const struct dwxgmac3_error_desc dwxgmac3_dma_errors[32] = {
	{ true, "TCES", "DMA TSO Memory Error" },
	{ true, "TAMS", "DMA TSO Memory Address Mismatch Error" },
	{ true, "TUES", "DMA TSO Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 3 */
	{ true, "DCES", "DMA DCACHE Memory Error" },
	{ true, "DAMS", "DMA DCACHE Address Mismatch Error" },
	{ true, "DUES", "DMA DCACHE Memory Error" },
	{ false, "UNKNOWN", "Unknown Error" },	/* 7 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 8 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 9 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 10 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 11 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 12 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 13 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 14 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 15 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 16 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 17 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 18 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 19 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 20 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 21 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 22 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 23 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 24 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 25 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 26 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 27 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 28 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 29 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 30 */
	{ false, "UNKNOWN", "Unknown Error" },	/* 31 */
};

static void dwxgmac3_handle_dma_err(struct net_device *ndev,
				    void __iomem *ioaddr, bool correctable,
				    struct dn200_safety_stats *stats)
{
	u32 value;

	value = readl(ioaddr + XGMAC_DMA_ECC_INT_STATUS);
	writel(value, ioaddr + XGMAC_DMA_ECC_INT_STATUS);
	netdev_err(ndev, "dwxgmac come across dma err REG=%#x value=%#x\n",
		   XGMAC_DMA_ECC_INT_STATUS, value);
	dwxgmac3_log_error(ndev, value, correctable, "DMA", dwxgmac3_dma_errors,
			   STAT_OFF(dma_errors), stats);
}

static void dn200_feat_config_even_parity_check(void __iomem *ioaddr)
{
	u32 value;
	/* 2. Change to even parity check */
	value = readl(ioaddr + XGMAC_MTL_DPP_CONTROL);
	value &= 0xFFFFFFFD;
	writel(value, ioaddr + XGMAC_MTL_DPP_CONTROL);
}

static int
dwxgmac3_safety_feat_config(void __iomem *ioaddr, unsigned int asp,
			    struct dn200_safety_feature_cfg *safety_cfg,
			    struct mac_device_info *hw)
{
	u32 value;

	if (!asp)
		return -EINVAL;
	if (HW_IS_VF(hw))
		return 0;
	/* 1. Enable Safety Features */
	writel(0x0, ioaddr + XGMAC_MTL_ECC_CONTROL);

	dn200_feat_config_even_parity_check(ioaddr);

	/* 2. Enable MTL Safety Interrupts */
	value = readl(ioaddr + XGMAC_MTL_ECC_INT_ENABLE);
	value |= XGMAC_RPCEIE;	/* RX Parser Memory Correctable Error */
	value |= XGMAC_ECEIE;	/* EST Memory Correctable Error */
	value |= XGMAC_RXCEIE;	/* RX Memory Correctable Error */
	value |= XGMAC_TXCEIE;	/* TX Memory Correctable Error */
	writel(value, ioaddr + XGMAC_MTL_ECC_INT_ENABLE);

	/* 3. Enable DMA Safety Interrupts */
	value = readl(ioaddr + XGMAC_DMA_ECC_INT_ENABLE);
	value |= XGMAC_DCEIE;	/* Descriptor Cache Memory Correctable Error */
	value |= XGMAC_TCEIE;	/* TSO Memory Correctable Error */
	writel(value, ioaddr + XGMAC_DMA_ECC_INT_ENABLE);

	/* Only ECC Protection for External Memory feature is selected */
	if (asp <= 0x1)
		return 0;

	/* 4. Enable Parity and Timeout for FSM */
	value = readl(ioaddr + XGMAC_MAC_FSM_CONTROL);
	value |= XGMAC_PRTYEN;	/* FSM Parity Feature */
	value |= XGMAC_TMOUTEN;	/* FSM Timeout Feature */
	writel(value, ioaddr + XGMAC_MAC_FSM_CONTROL);

	return 0;
}

static int dwxgmac3_safety_feat_irq_status(struct net_device *ndev,
					   void __iomem *ioaddr,
					   unsigned int asp,
					   struct dn200_safety_stats *stats)
{
	bool err, corr;
	u32 mtl, dma;
	int ret = 0;

	if (!asp)
		return -EINVAL;

	mtl = readl(ioaddr + XGMAC_MTL_SAFETY_INT_STATUS);
	dma = readl(ioaddr + XGMAC_DMA_SAFETY_INT_STATUS);

	err = (mtl & XGMAC_MCSIS) || (dma & XGMAC_MCSIS);
	corr = false;
	if (err) {
		dwxgmac3_handle_mac_err(ndev, ioaddr, corr, stats);
		ret |= !corr;
	}

	err = (mtl & (XGMAC_MEUIS | XGMAC_MECIS)) ||
	    (dma & (XGMAC_MSUIS | XGMAC_MSCIS));
	corr = (mtl & XGMAC_MECIS) || (dma & XGMAC_MSCIS);
	if (err) {
		dwxgmac3_handle_mtl_err(ndev, ioaddr, corr, stats);
		ret |= !corr;
	}

	err = dma & (XGMAC_DEUIS | XGMAC_DECIS);
	corr = dma & XGMAC_DECIS;
	if (err) {
		dwxgmac3_handle_dma_err(ndev, ioaddr, corr, stats);
		ret |= !corr;
	}

	return ret;
}

static const struct dwxgmac3_error {
	const struct dwxgmac3_error_desc *desc;
} dwxgmac3_all_errors[] = {
	{ dwxgmac3_mac_errors },
	{ dwxgmac3_mtl_errors },
	{ dwxgmac3_dma_errors },
};

static int dwxgmac3_safety_feat_dump(struct dn200_safety_stats *stats,
				     int index, unsigned long *count,
				     const char **desc)
{
	int module = index / 32, offset = index % 32;
	unsigned long *ptr = (unsigned long *)stats;

	if (module >= ARRAY_SIZE(dwxgmac3_all_errors))
		return -EINVAL;
	if (!dwxgmac3_all_errors[module].desc[offset].valid)
		return -EINVAL;
	if (count)
		*count = *(ptr + index);
	if (desc)
		*desc = dwxgmac3_all_errors[module].desc[offset].desc;
	return 0;
}

static int dwxgmac2_get_mac_tx_timestamp(struct mac_device_info *hw, u64 *ts)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	value = readl(ioaddr + XGMAC_TIMESTAMP_STATUS);
	if (!(value & XGMAC_TXTSC))
		return -EBUSY;
	*ts = readl(ioaddr + XGMAC_TXTIMESTAMP_NSEC) & XGMAC_TXTSSTSLO;
	*ts += readl(ioaddr + XGMAC_TXTIMESTAMP_SEC) * 1000000000ULL;
	return 0;
}

static int dwxgmac2_flex_pps_config(void __iomem *ioaddr, int index,
				    struct dn200_pps_cfg *cfg, bool enable,
				    u32 sub_second_inc, u32 systime_flags)
{
	u32 tnsec = readl(ioaddr + XGMAC_PPSx_TARGET_TIME_NSEC(index));
	u32 val = readl(ioaddr + XGMAC_PPS_CONTROL);
	u64 period;

	if (!cfg->available)
		return -EINVAL;
	if (tnsec & XGMAC_TRGTBUSY0)
		return -EBUSY;
	if (!sub_second_inc || !systime_flags)
		return -EINVAL;

	val &= ~XGMAC_PPSx_MASK(index);

	if (!enable) {
		val |= XGMAC_PPSCMDX(index, XGMAC_PPSCMD_STOP);
		writel(val, ioaddr + XGMAC_PPS_CONTROL);
		return 0;
	}

	val |= XGMAC_PPSCMDX(index, XGMAC_PPSCMD_START);
	val |= XGMAC_TRGTMODSELX(index, XGMAC_PPSCMD_START);
	val |= XGMAC_PPSEN0;

	writel(cfg->start.tv_sec, ioaddr + XGMAC_PPSx_TARGET_TIME_SEC(index));

	if (!(systime_flags & PTP_TCR_TSCTRLSSR))
		cfg->start.tv_nsec = (cfg->start.tv_nsec * 1000) / 465;
	writel(cfg->start.tv_nsec, ioaddr + XGMAC_PPSx_TARGET_TIME_NSEC(index));

	period = cfg->period.tv_sec * 1000000000;
	period += cfg->period.tv_nsec;

	do_div(period, sub_second_inc);

	if (period <= 1)
		return -EINVAL;

	writel(period - 1, ioaddr + XGMAC_PPSx_INTERVAL(index));

	period >>= 1;
	if (period <= 1)
		return -EINVAL;

	writel(period - 1, ioaddr + XGMAC_PPSx_WIDTH(index));

	/* Finally, activate it */
	writel(val, ioaddr + XGMAC_PPS_CONTROL);
	return 0;
}

static void dwxgmac2_sarc_configure(void __iomem *ioaddr, int val)
{
	u32 value = readl(ioaddr + XGMAC_TX_CONFIG);

	value &= ~XGMAC_CONFIG_SARC;
	value |= val << XGMAC_CONFIG_SARC_SHIFT;

	writel(value, ioaddr + XGMAC_TX_CONFIG);
}

static void dwxgmac2_enable_vlan(struct mac_device_info *hw, u32 type)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	value = readl(ioaddr + XGMAC_VLAN_INCL);
	value |= XGMAC_VLAN_VLTI;
	value &= ~XGMAC_VLAN_CSVL;	/* Only use SVLAN */
	value &= ~XGMAC_VLAN_VLC;
	value |= (type << XGMAC_VLAN_VLC_SHIFT) & XGMAC_VLAN_VLC;
	writel(value, ioaddr + XGMAC_VLAN_INCL);
}

static int dwxgmac2_filter_wait(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	return readl_poll_timeout_atomic(ioaddr + XGMAC_L3L4_ADDR_CTRL, value,
					 !(value & XGMAC_XB), 10, 10000);
}

static int dwxgmac2_filter_read(struct mac_device_info *hw, u32 filter_no,
				u8 reg, u32 *data)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret;

	ret = dwxgmac2_filter_wait(hw);
	if (ret)
		return ret;

	value = (((filter_no & 0x1f) << XGMAC_IDDR_FNUM) | reg) << XGMAC_IDDR_SHIFT;
	value |= XGMAC_TT | XGMAC_XB;
	writel(value, ioaddr + XGMAC_L3L4_ADDR_CTRL);

	ret = dwxgmac2_filter_wait(hw);
	if (ret)
		return ret;

	*data = readl(ioaddr + XGMAC_L3L4_DATA);
	return 0;
}

static int dwxgmac2_filter_write(struct mac_device_info *hw, u32 filter_no,
				 u8 reg, u32 data)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int retry = 0;
	int ret;

retry:
	ret = dwxgmac2_filter_wait(hw);
	if (ret)
		return ret;

	writel(data, ioaddr + XGMAC_L3L4_DATA);

	value = (((filter_no & 0x1f) << XGMAC_IDDR_FNUM) | reg) << XGMAC_IDDR_SHIFT;
	value |= XGMAC_XB;
	writel(value, ioaddr + XGMAC_L3L4_ADDR_CTRL);
	ret = dwxgmac2_filter_wait(hw);
	if (ret && !retry) {
		writel(0, ioaddr + XGMAC_L3L4_ADDR_CTRL);
		retry++;
		goto retry;
	} else if (ret) {
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
	}
	return ret;
}

static int dwxgmac2_config_l3_filter(struct mac_device_info *hw, u32 filter_no,
				     bool en, bool ipv6, bool sa, bool inv,
				     u32 match)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret;

	if (HW_IS_VF(hw))
		return 0;
	if (!HW_IS_PUREPF(hw)) {
		unsigned long bitmap_l3l4 = 0;
		int offset = 0;

		offset = dn200_get_l3l4_filter_offset(hw, filter_no);
		if (offset == -1) {
			if (en) {
				DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      &bitmap_l3l4);
				offset =
				    dn200_get_unused_bit(&bitmap_l3l4, 31, 0);
				bitmap_set((unsigned long *)&bitmap_l3l4,
					   offset, 1);
				DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      bitmap_l3l4);
				dn200_set_l3l4_filter_info(hw, filter_no,
							   offset, false);
				filter_no = offset;
			} else {
				return -1;
			}
		} else {
			filter_no = offset;
			if (!en) {
				DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      &bitmap_l3l4);
				bitmap_clear((unsigned long *)&bitmap_l3l4,
					     offset, 1);
				DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      bitmap_l3l4);
				dn200_set_l3l4_filter_info(hw, filter_no,
							   offset, true);
			}
		}
	}

	value = readl(ioaddr + XGMAC_PACKET_FILTER);
	value |= XGMAC_FILTER_IPFE;
	writel(value, ioaddr + XGMAC_PACKET_FILTER);

	ret = dwxgmac2_filter_read(hw, filter_no, XGMAC_L3L4_CTRL, &value);
	if (ret)
		return ret;

	/* For IPv6 not both SA/DA filters can be active */
	if (ipv6) {
		value |= XGMAC_L3PEN0;
		value &= ~(XGMAC_L3SAM0 | XGMAC_L3SAIM0);
		value &= ~(XGMAC_L3DAM0 | XGMAC_L3DAIM0);
		if (sa) {
			value |= XGMAC_L3SAM0;
			if (inv)
				value |= XGMAC_L3SAIM0;
		} else {
			value |= XGMAC_L3DAM0;
			if (inv)
				value |= XGMAC_L3DAIM0;
		}
	} else {
		value &= ~XGMAC_L3PEN0;
		if (sa) {
			value |= XGMAC_L3SAM0;
			if (inv)
				value |= XGMAC_L3SAIM0;
		} else {
			value |= XGMAC_L3DAM0;
			if (inv)
				value |= XGMAC_L3DAIM0;
		}
	}

	ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, value);
	if (ret)
		return ret;

	if (sa) {
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR0, match);
		if (ret)
			return ret;
	} else {
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR1, match);
		if (ret)
			return ret;
	}

	if (!en)
		return dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, 0);
	return 0;
}

static int dwxgmac2_config_l4_filter(struct mac_device_info *hw, u32 filter_no,
				     bool en, bool udp, bool sa, bool inv,
				     u32 match)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret;

	if (HW_IS_VF(hw))
		return 0;
	if (!HW_IS_PUREPF(hw)) {
		unsigned long bitmap_l3l4 = 0;
		int offset = 0;

		offset = dn200_get_l3l4_filter_offset(hw, filter_no);
		if (offset == -1) {
			if (en) {
				DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      &bitmap_l3l4);
				offset =
				    dn200_get_unused_bit(&bitmap_l3l4, 31, 0);
				bitmap_set((unsigned long *)&bitmap_l3l4,
					   offset, 1);
				DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      bitmap_l3l4);
				dn200_set_l3l4_filter_info(hw, filter_no,
							   offset, false);
				filter_no = offset;
			} else {
				return -1;
			}
		} else {
			filter_no = offset;
			if (!en) {
				DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      &bitmap_l3l4);
				bitmap_clear((unsigned long *)&bitmap_l3l4,
					     offset, 1);
				DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_l3l4,
							      bitmap_l3l4);
				dn200_set_l3l4_filter_info(hw, filter_no,
							   offset, true);
			}
		}
	}

	value = readl(ioaddr + XGMAC_PACKET_FILTER);
	value |= XGMAC_FILTER_IPFE;
	writel(value, ioaddr + XGMAC_PACKET_FILTER);

	ret = dwxgmac2_filter_read(hw, filter_no, XGMAC_L3L4_CTRL, &value);
	if (ret)
		return ret;

	if (udp)
		value |= XGMAC_L4PEN0;
	else
		value &= ~XGMAC_L4PEN0;

	value &= ~(XGMAC_L4SPM0 | XGMAC_L4SPIM0);
	value &= ~(XGMAC_L4DPM0 | XGMAC_L4DPIM0);
	if (sa) {
		value |= XGMAC_L4SPM0;
		if (inv)
			value |= XGMAC_L4SPIM0;
	} else {
		value |= XGMAC_L4DPM0;
		if (inv)
			value |= XGMAC_L4DPIM0;
	}

	ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, value);
	if (ret)
		return ret;

	if (sa) {
		value = match & XGMAC_L4SP0;
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L4_ADDR, value);
		if (ret)
			return ret;
	} else {
		value = (match << XGMAC_L4DP0_SHIFT) & XGMAC_L4DP0;
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L4_ADDR, value);
		if (ret)
			return ret;
	}

	if (!en)
		return dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, 0);

	return 0;
}

static int dwxgmac2_config_ntuple_filter(struct mac_device_info *hw,
					 u32 filter_no,
					 struct dn200_fdir_filter *input,
					 bool en)
{
	u32 value = 0;
	int ret;
	int mask;
	u32 data = 0;

	if (HW_IS_VF(hw))
		return 0;
	if (!en)
		return dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, 0);

	if (!(input->flow_type & (DN200_FLOW_TYPE_SA | DN200_FLOW_TYPE_DA |
			  DN200_FLOW_TYPE_SPORT | DN200_FLOW_TYPE_DPORT)))
		return -1;
	if (input->action & DN200_FLOW_ACTION_ROUTE) {
		value |= XGMAC_DMCHEN;
		value |= ((input->queue << XGMAC_DMCHN_SHIFT) & XGMAC_DMCHN);
	}
	/* For IPv6 not both SA/DA filters can be active */
	if (input->flow_type & DN200_FLOW_TYPE_V6) {
		value |= XGMAC_L3PEN0;
		value &= ~(XGMAC_L3SAM0 | XGMAC_L3SAIM0);
		value &= ~(XGMAC_L3DAM0 | XGMAC_L3DAIM0);
		if (input->flow_type & DN200_FLOW_TYPE_SA)
			value |= XGMAC_L3SAM0;
		else if (input->flow_type & DN200_FLOW_TYPE_DA)
			value |= XGMAC_L3DAM0;
		else
			goto set_port;
		mask = input->xgmac_mask_src;
		value |= ((mask << XGMAC_L3HSBM0_SHIFT) & XGMAC_L3HSBM0_V6);
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR0,
					  ntohl(input->ip6[3]));
		if (ret)
			return ret;
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR1,
					  ntohl(input->ip6[2]));
		if (ret)
			return ret;
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR2,
					  ntohl(input->ip6[1]));
		if (ret)
			return ret;
		ret =
		    dwxgmac2_filter_write(hw, filter_no, XMGAC_L3_ADDR3,
					  ntohl(input->ip6[0]));
		if (ret)
			return ret;
	}
	if (input->flow_type & DN200_FLOW_TYPE_V4) {
		value &= ~XGMAC_L3PEN0;
		if (input->flow_type & DN200_FLOW_TYPE_SA) {
			value |= XGMAC_L3SAM0;
			/*get src ip mask */
			mask = (input->xgmac_mask_src);
			value |=
			    ((mask << XGMAC_L3HSBM0_SHIFT) & XGMAC_L3HSBM0);
			ret =
			    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR0,
						  ntohl(input->src_ip));
			if (ret)
				return ret;
		}
		if (input->flow_type & DN200_FLOW_TYPE_DA) {
			value |= XGMAC_L3DAM0;
			/*get dst ip mask */
			mask = (input->xgmac_mask_dst);
			value |=
			    ((mask << XGMAC_L3HDBM0_SHIFT) & XGMAC_L3HDBM0);
			ret =
			    dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR1,
						  ntohl(input->dst_ip));
			if (ret)
				return ret;
		}
	}

set_port:
	if (input->flow_type & DN200_FLOW_TYPE_UDP)
		value |= XGMAC_L4PEN0;
	else
		value &= ~XGMAC_L4PEN0;

	value &= ~(XGMAC_L4SPM0 | XGMAC_L4SPIM0);
	value &= ~(XGMAC_L4DPM0 | XGMAC_L4DPIM0);
	if (input->flow_type & DN200_FLOW_TYPE_SPORT) {
		value |= XGMAC_L4SPM0;
		data |= (ntohs(input->src_port) & XGMAC_L4SP0);
	}

	if (input->flow_type & DN200_FLOW_TYPE_DPORT) {
		value |= XGMAC_L4DPM0;
		data |=
		    (ntohs(input->dst_port) << XGMAC_L4DP0_SHIFT) & XGMAC_L4DP0;
	}
	ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L4_ADDR, data);
	if (ret)
		return ret;

	ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, value);
	if (ret)
		return ret;

	return 0;
}

static void dwxgmac2_config_l3l4_filter(struct mac_device_info *hw, bool en)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_PACKET_FILTER);

	if (en) {
		value |= (2 << XGMAC_DHLFRS_SHIFT);
		value |= XGMAC_FILTER_IPFE;
	} else {
		value &= ~(XGMAC_DHLFRS_MASK);
		value &= ~XGMAC_FILTER_IPFE;
	}
	writel(value, ioaddr + XGMAC_PACKET_FILTER);
	if (en) {
		/*enable ipv4 l3 filter all pass */
		dwxgmac2_filter_write(hw, 30, XGMAC_L3L4_CTRL,
				      XGMAC_L3SAM0 | XGMAC_L3HSBM0);
		dwxgmac2_filter_write(hw, 30, XGMAC_L3_ADDR0, (0));
		dwxgmac2_filter_write(hw, 31, XGMAC_L3L4_CTRL,
				      XGMAC_L3SAM0 | XGMAC_L3HSBM0);
		dwxgmac2_filter_write(hw, 31, XGMAC_L3_ADDR0, (0x80000000));
		/*enable ipv6 l3 filter all pass */
		dwxgmac2_filter_write(hw, 30, XGMAC_L3L4_CTRL,
				      XGMAC_L3SAM0 | XGMAC_L3HSBM0_V6 |
				      XGMAC_L3PEN0);
		dwxgmac2_filter_write(hw, 30, XMGAC_L3_ADDR3, (0));
		dwxgmac2_filter_write(hw, 31, XGMAC_L3L4_CTRL,
				      XGMAC_L3SAM0 | XGMAC_L3HSBM0_V6 |
				      XGMAC_L3PEN0);
		dwxgmac2_filter_write(hw, 31, XMGAC_L3_ADDR3, (0x80000000));
	} else {
		/*disable ipv4 l3 filter all pass entry */
		dwxgmac2_filter_write(hw, 30, 0, 0);
		dwxgmac2_filter_write(hw, 31, 0, 0);
		/*disable ipv6 l3 filter all pass entry */
		dwxgmac2_filter_write(hw, 29, 0, 0);
		dwxgmac2_filter_write(hw, 28, 0, 0);
	}
}

static void dwxgmac2_set_arp_offload(struct mac_device_info *hw, bool en,
				     u32 addr)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	writel(addr, ioaddr + XGMAC_ARP_ADDR);

	value = readl(ioaddr + XGMAC_RX_CONFIG);
	if (en)
		value |= XGMAC_CONFIG_ARPEN;
	else
		value &= ~XGMAC_CONFIG_ARPEN;
	if (HW_IS_VF(hw))
		return;
	writel(value, ioaddr + XGMAC_RX_CONFIG);
}

static int dwxgmac3_est_write(void __iomem *ioaddr, u32 reg, u32 val, bool gcl)
{
	u32 ctrl;

	writel(val, ioaddr + XGMAC_MTL_EST_GCL_DATA);

	ctrl = (reg << XGMAC_ADDR_SHIFT);
	ctrl |= gcl ? 0 : XGMAC_GCRR;

	writel(ctrl, ioaddr + XGMAC_MTL_EST_GCL_CONTROL);

	ctrl |= XGMAC_SRWO;
	writel(ctrl, ioaddr + XGMAC_MTL_EST_GCL_CONTROL);

	return readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_EST_GCL_CONTROL,
					 ctrl, !(ctrl & XGMAC_SRWO), 100,
					 50000);
}

static int dwxgmac3_est_configure(void __iomem *ioaddr, struct dn200_est *cfg,
				  unsigned int ptp_rate)
{
	int i, ret = 0x0;
	u32 ctrl;

	ret |= dwxgmac3_est_write(ioaddr, XGMAC_BTR_LOW, cfg->btr[0], false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_BTR_HIGH, cfg->btr[1], false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_TER, cfg->ter, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_LLR, cfg->gcl_size, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_CTR_LOW, cfg->ctr[0], false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_CTR_HIGH, cfg->ctr[1], false);
	if (ret)
		return ret;

	for (i = 0; i < cfg->gcl_size; i++) {
		ret = dwxgmac3_est_write(ioaddr, i, cfg->gcl[i], true);
		if (ret)
			return ret;
	}

	ctrl = readl(ioaddr + XGMAC_MTL_EST_CONTROL);
	ctrl &= ~XGMAC_PTOV;
	ctrl |= ((1000000000 / ptp_rate) * 9) << XGMAC_PTOV_SHIFT;
	if (cfg->enable)
		ctrl |= XGMAC_EEST | XGMAC_SSWL;
	else
		ctrl &= ~XGMAC_EEST;

	writel(ctrl, ioaddr + XGMAC_MTL_EST_CONTROL);
	return 0;
}

static void dwxgmac3_fpe_configure(void __iomem *ioaddr, u32 num_txq,
				   u32 num_rxq, bool enable)
{
	u32 value;

	if (!enable) {
		value = readl(ioaddr + XGMAC_FPE_CTRL_STS);

		value &= ~XGMAC_EFPE;

		writel(value, ioaddr + XGMAC_FPE_CTRL_STS);
		return;
	}

	value = readl(ioaddr + XGMAC_RXQ_CTRL1);
	value &= ~XGMAC_RQ;
	value |= (num_rxq - 1) << XGMAC_RQ_SHIFT;
	writel(value, ioaddr + XGMAC_RXQ_CTRL1);

	value = readl(ioaddr + XGMAC_FPE_CTRL_STS);
	value |= XGMAC_EFPE;
	writel(value, ioaddr + XGMAC_FPE_CTRL_STS);
}

static int dwxgmac2_indiraccess_wait(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	return readl_poll_timeout_atomic(ioaddr + XGMAC_INDIR_ACCESS_CTRL,
					 value, !(value & XGMAC_INDIR_OB), 10,
					 10000);
}

static int __maybe_unused dwxgmac2_indiraccess_read(struct mac_device_info *hw,
						    u32 addr_off, u8 mode_sel,
						    u32 *data)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret;

	ret = dwxgmac2_indiraccess_wait(hw);
	if (ret)
		return ret;

	value =
	    ((addr_off & 0xfff) << XGMAC_INDIR_AOFF_SHIFT) | (mode_sel <<
						    XGMAC_INDIR_MSEL_SHIFT);
	value |= XGMAC_INDIR_COM | XGMAC_INDIR_OB;
	writel(value, ioaddr + XGMAC_INDIR_ACCESS_CTRL);

	ret = dwxgmac2_indiraccess_wait(hw);
	if (ret)
		return ret;

	*data = readl(ioaddr + XGMAC_INDRI_ACCESS_DATA);
	return 0;
}

static int dwxgmac2_indiraccess_write(struct mac_device_info *hw, u32 addr_off,
				      u8 mode_sel, u32 data)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;
	int ret;
	int retry = 0;

	ret = dwxgmac2_indiraccess_wait(hw);
	if (ret)
		return ret;
retry:
	writel(data, ioaddr + XGMAC_INDRI_ACCESS_DATA);

	value =
	    ((addr_off & 0xfff) << XGMAC_INDIR_AOFF_SHIFT) | (mode_sel <<
						    XGMAC_INDIR_MSEL_SHIFT);
	value |= XGMAC_INDIR_OB;
	writel(value, ioaddr + XGMAC_INDIR_ACCESS_CTRL);

	ret = dwxgmac2_indiraccess_wait(hw);
	if (ret && !retry) {
		writel(0, ioaddr + XGMAC_INDIR_ACCESS_CTRL);
		retry++;
		goto retry;
	} else if (ret) {
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
		return -EBUSY;
	}
	return ret;
}

static void dwxgmac2_hw_vlan_init(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value, i;
	int ret;
	int rovlt_init = 0;

	if (!HW_IS_PUREPF(hw))
		rovlt_init = 1;
	writel(GENMASK(11, 0), ioaddr + XGMAC_RVLAN_LKP_SIZE);
	for (i = 0; i < VLAN_N_VID - 1; i++) {
		ret =
		    dwxgmac2_indiraccess_write(hw, i, XGMAC_INDIR_EXT_ROVTL,
					       rovlt_init);
		if (ret)
			return;
	}
	writel(GENMASK(11, 0), ioaddr + XGMAC_RVLAN_LKP_SIZE);

	value = readl(ioaddr + XGMAC_VLAN_TAG);
	value &= ~XGMAC_VLAN_VTHM;
	value &= ~XGMAC_VLAN_ETV;
	/* Enable external Receive Outer VLAN Tag Lookup based perfect filtering and routing */
	value &= ~XGMAC_VLAN_EROVTL_MASK;
	value |= (0x3 << XGMAC_VLAN_EROVTL_SHIFT);
	value |= XGMAC_VLAN_EDVLP;
	value |= XGMAC_VLAN_DOVLTC;

	writel(value, ioaddr + XGMAC_VLAN_TAG);
}

static int dwxgmac2_add_hw_vlan_rx_fltr(struct net_device *dev,
					struct mac_device_info *hw,
					__be16 proto, u16 vid, uint8_t off,
					bool is_last)
{
	/* enable external lookup perfect fileter */
	return dwxgmac2_indiraccess_write(hw, vid, XGMAC_INDIR_EXT_ROVTL, 1);
}

static int dwxgmac2_del_hw_vlan_rx_fltr(struct net_device *dev,
					struct mac_device_info *hw,
					__be16 proto, u16 vid, uint8_t off,
					bool is_last)
{
	int ret;

	ret = dwxgmac2_indiraccess_write(hw, vid & 0xFFF, XGMAC_INDIR_EXT_ROVTL,
				       0);
	return ret;
}

static void dwxgmac2_config_vlan_rx_fltr(struct mac_device_info *hw,
					 bool enable)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value = readl(ioaddr + XGMAC_PACKET_FILTER);

	if (enable)
		value |= XGMAC_FILTER_VTFE;
	else
		value &= ~XGMAC_FILTER_VTFE;
	writel(value, ioaddr + XGMAC_PACKET_FILTER);
}

static void dwxgmac2_rx_vlan_stripping_config(struct mac_device_info *hw,
					      bool enable)
{
	void __iomem *ioaddr = hw->pcsr;
	u32 value;

	if (HW_IS_VF(hw))
		return;
	value = readl(ioaddr + XGMAC_VLAN_TAG);

	if (enable) {
		value |= (XGMAC_VLAN_TAG_CTRL_EVLRXS);
		value &= ~(XGMAC_VLAN_TAG_CTRL_EVLS_MASK);
		value |= XGMAC_VLAN_TAG_STRIP_PASS;
	} else {
		value &= ~XGMAC_VLAN_TAG_CTRL_EVLS_MASK;
		value |= XGMAC_VLAN_TAG_STRIP_NONE;
	}
	writel(value, ioaddr + XGMAC_VLAN_TAG);
}

#define RSS_KEY_SIZE 10
#define RSS_TABLE_SIZE 256
#define EXT_DAHASH_SIZE 256
#define GCL_MEM_SIZE 256
static int dwxgmac2_rxf_and_acl_mem_reset(struct mac_device_info *hw)
{
	void __iomem *ioaddr = hw->pcsr;
	int filter_no = 0, i = 0;
	int ret = 0;

	/*rss reset */
	writel(0, ioaddr + XGMAC_RSS_DATA);
	writel(0, ioaddr + XGMAC_RSS_ADDR);
	for (i = 0; i < RSS_KEY_SIZE; i++) {
		ret = dwxgmac2_rss_write_reg(ioaddr, true, i, 0);
		if (ret) {
			netdev_err(hw->priv->dev, "%s: %d i %d poll_timeout.\n",
				   __func__, __LINE__, i);
			return ret;
		}
	}

	for (i = 0; i < RSS_TABLE_SIZE; i++) {
		ret = dwxgmac2_rss_write_reg(ioaddr, false, i, 0);
		if (ret) {
			netdev_err(hw->priv->dev, "%s: %d i %d poll_timeout.\n",
				   __func__, __LINE__, i);
			return ret;
		}
	}

	/*filter */
	/*clear operation busy */
	writel(0, ioaddr + XGMAC_L3L4_ADDR_CTRL);
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3L4_CTRL, 0);
		if (ret)
			return ret;
	}
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L4_ADDR, 0);
		if (ret)
			return ret;
	}
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR0, 0);
		if (ret)
			return ret;
	}
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR1, 0);
		if (ret)
			return ret;
	}
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XGMAC_L3_ADDR2, 0);
		if (ret)
			return ret;
	}
	for (filter_no = 0; filter_no < hw->priv->dma_cap.l3l4fnum; filter_no++) {
		ret = dwxgmac2_filter_write(hw, filter_no, XMGAC_L3_ADDR3, 0);
		if (ret)
			return ret;
	}

	 /**/ writel(0, ioaddr + XGMAC_RVLAN_LKP_SIZE);
	for (i = 0; i < VLAN_N_VID; i++) {
		ret =
		    dwxgmac2_indiraccess_write(hw, i, XGMAC_INDIR_EXT_ROVTL, 0);
		if (ret)
			return ret;
	}

	for (i = 0; i < VLAN_N_VID; i++) {
		ret =
		    dwxgmac2_indiraccess_write(hw, i, XGMAC_INDIR_EXT_RIVTL, 0);
		if (ret)
			return ret;
	}

	for (i = 0; i < 0x1f; i++) {
		ret =
		    dwxgmac2_indiraccess_write(hw, i, XGMAC_INDIR_DCHSEL, 0);
		if (ret)
			return ret;
	}
	/*gcl */
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_BTR_LOW, 0, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_BTR_HIGH, 0, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_TER, 0, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_LLR, GCL_MEM_SIZE, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_CTR_LOW, 0, false);
	ret |= dwxgmac3_est_write(ioaddr, XGMAC_CTR_HIGH, 0, false);
	if (ret)
		return ret;

	for (i = 0; i < GCL_MEM_SIZE; i++) {
		ret = dwxgmac3_est_write(ioaddr, i, 0, true);
		if (ret)
			return ret;
	}
	return 0;
}

static int dwxgmac3_rxp_disable(void __iomem *ioaddr)
{
	u32 val = readl(ioaddr + XGMAC_MTL_OPMODE);

	val &= ~XGMAC_FRPE;
	writel(val, ioaddr + XGMAC_MTL_OPMODE);

	return 0;
}

static void dwxgmac3_rxp_enable(void __iomem *ioaddr)
{
	u32 val;

	val = readl(ioaddr + XGMAC_MTL_OPMODE);
	val |= XGMAC_FRPE;
	writel(val, ioaddr + XGMAC_MTL_OPMODE);
}

static int dwxgmac3_rxp_update_single_entry(struct mac_device_info *hw,
					    struct dn200_tc_entry *entry,
					    int pos)
{
	int ret, i;
	void __iomem *ioaddr = hw->pcsr;
	int retry = 0;
	u32 val;
	int real_pos;

	for (i = 0; i < (sizeof(entry->val) / sizeof(u32)); i++) {
		real_pos = pos * (sizeof(entry->val) / sizeof(u32)) + i;
		retry = 0;
		/* Wait for ready */
		ret =
		    readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST, val,
					      !(val & XGMAC_STARTBUSY), 1,
					      10000);
		if (ret) {
			netdev_err(hw->priv->dev, "%s: %d poll_timeout\n",
				   __func__, __LINE__);
			return ret;
		}
retry:
		/* Write data */
		val = *((u32 *)&entry->val + i);
		writel(val, ioaddr + XGMAC_MTL_RXP_IACC_DATA);

		/* Write pos */
		val = real_pos & XGMAC_ADDR;
		writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

		/* Write OP */
		val |= XGMAC_WRRDN;
		writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

		/* Start Write */
		val |= XGMAC_STARTBUSY;
		writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

		/* Wait for done */
		ret =
		    readl_poll_timeout_atomic(ioaddr +
					      XGMAC_MTL_RXP_IACC_CTRL_ST, val,
					      !(val & XGMAC_STARTBUSY), 1,
					      10000);
		if (ret && !retry) {
			retry = 1;
			writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
			writel(0, ioaddr + XGMAC_MTL_RXP_IACC_DATA);
			goto retry;
		} else if (ret) {
			writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
			netdev_err(hw->priv->dev, "%s: %d poll_timeout\n",
				   __func__, __LINE__);
			return ret;
		}
	}
	return 0;
}

static int dwxgmac2_rxp_get_single_entry_sriov(struct mac_device_info *hw,
					       u32 *data, int real_pos)
{
	int ret;
	u32 val;
	int retry = 0;
	void __iomem *ioaddr = hw->pcsr;

	/* Wait for ready */
	ret = readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST,
					val, !(val & XGMAC_STARTBUSY), 10,
					10000);
	if (ret) {
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
		return ret;
	}
retry:
	/* Write pos */
	val = real_pos & XGMAC_ADDR;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Write OP */
	val &= ~XGMAC_WRRDN;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Start Write */
	val |= XGMAC_STARTBUSY;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Wait for done */
	ret = readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST,
					val, !(val & XGMAC_STARTBUSY), 10,
					100000);
	if (ret && !ret) {
		retry = 1;
		writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
		goto retry;
	} else if (ret) {
		WARN_ON(1);
		writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
		return ret;
	}

	/* Read data */
	*data = readl(ioaddr + XGMAC_MTL_RXP_IACC_DATA);

	return 0;
}

static int dwxgmac2_rxp_update_single_entry_sriov(struct mac_device_info *hw,
						  u32 data, int real_pos)
{
	int ret;
	u32 val;
	void __iomem *ioaddr = hw->pcsr;
	int retry = 0;

	/* Wait for ready */
	ret = readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST,
					val, !(val & XGMAC_STARTBUSY), 10,
					10000);
	if (ret) {
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
		return ret;
	}

retry:
	/* Write data */
	writel(data, ioaddr + XGMAC_MTL_RXP_IACC_DATA);

	/* Write pos */
	val = real_pos & XGMAC_ADDR;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Write OP */
	val |= XGMAC_WRRDN;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Start Write */
	val |= XGMAC_STARTBUSY;
	writel(val, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);

	/* Wait for done */
	ret = readl_poll_timeout_atomic(ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST,
					val, !(val & XGMAC_STARTBUSY), 10,
					100000);
	if (ret && !retry) {
		writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
		retry++;
		goto retry;
	} else if (ret) {
		writel(0, ioaddr + XGMAC_MTL_RXP_IACC_CTRL_ST);
		netdev_err(hw->priv->dev, "%s: %d poll_timeout\n", __func__,
			   __LINE__);
	}

	return ret;
}

static void dwxgmac2_rxp_clear_entry_sriov(struct mac_device_info *hw)
{
	int i = 0;

	for (; i < DN200_MAX_USED_RXP_NUM; i++) {
		dwxgmac2_rxp_update_single_entry_sriov(hw, 0, i * 4 + 0);
		dwxgmac2_rxp_update_single_entry_sriov(hw, 0, i * 4 + 1);
		dwxgmac2_rxp_update_single_entry_sriov(hw, 0, i * 4 + 2);
		dwxgmac2_rxp_update_single_entry_sriov(hw, 0, i * 4 + 3);
	}
}

static int dwxgmac2_rxp_update_single_da_entry_sriov(struct mac_device_info *hw,
						     struct RXP_FPR_ENTRY
						     *entry, int pos)
{
	int ret, i;
	u32 val;
	int real_pos;

	/*delete rxp channel, clear dma channel firstly */
	if (entry->dma_ch_no == 0) {
		for (i = (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) - 1;
		     i >= 0; i--) {
			real_pos =
			    pos * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) + i;

			val = *((u32 *)entry + i);
			ret =
			    dwxgmac2_rxp_update_single_entry_sriov(hw, val,
								   real_pos);
			if (ret)
				return ret;
		}
	} else {
		for (i = 0; i < (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32));
		     i++) {
			real_pos =
			    pos * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) +
			    i;
			val = *((u32 *)entry + i);
			ret =
			    dwxgmac2_rxp_update_single_entry_sriov(hw, val,
								   real_pos);
			if (ret)
				return ret;
		}
	}
	return 0;
}

static int dwxgmac2_rxp_get_single_da_entry_sriov(struct mac_device_info *hw,
						  struct RXP_FPR_ENTRY *entry,
						  int pos)
{
	int ret, i;
	u32 val;
	int real_pos;

	for (i = 0; i < (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)); i++) {
		real_pos =
		    pos * (sizeof(struct RXP_FPR_ENTRY) / sizeof(u32)) + i;
		ret = dwxgmac2_rxp_get_single_entry_sriov(hw, &val, real_pos);
		if (ret)
			return ret;

		*((u32 *)entry + i) = val;
	}

	return 0;
}

static int rxp_offset_get_from_bitmap(int bitmap_off)
{
	int rxp_off = 0;

	if (bitmap_off < 1)
		rxp_off = 0;
	else if (bitmap_off < DN200_VF_UC_OFF)
		rxp_off = (bitmap_off) * 2;
	else
		rxp_off =
		    (bitmap_off - DN200_VF_UC_OFF) * 2 + 2 +
		    DN200_PF_VLAN_ENTRY_NUM + 1 + (DN200_PF_SELF_UC_NUM +
						   DN200_PF_OTHER_UC_NUM) * 2;
	return rxp_off;
}

static int dn200_add_vf_uc_rxp_da_route_sriov(struct mac_device_info *hw,
						 u8 *mac_addr, int offset, u8 rxq_start)
{
	unsigned long bitmap;
	int prev_off;
	int next_off;
	u32 prev_off_val;
	u32 data;
	u32 channel = 0;
	u8 entry_offset = 0;
	struct RXP_FPR_ENTRY *frp_entry_data;

	frp_entry_data = kcalloc(3, sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return -ENOMEM;

	channel = (1 << rxq_start);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	entry_offset = rxp_offset_get_from_bitmap(offset);

	/*get prev used entry */
	prev_off = dn200_get_prev_used_bit(&bitmap, offset);
	if (prev_off < 0)
		prev_off = 0;

	prev_off_val = rxp_offset_get_from_bitmap(prev_off);

	if (prev_off && prev_off < DN200_VF_UC_OFF) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val + 2));

		next_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));

		next_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;
	}
	frp_entry_data[0].match_data = cpu_to_le32(*(u32 *)(mac_addr));
	frp_entry_data[0].match_en = 0xffffffff;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].ok_index = next_off;
	frp_entry_data[0].frame_offset = 0;

	frp_entry_data[1].match_data = cpu_to_le32(*(u16 *)(mac_addr + 4));
	frp_entry_data[1].match_en = 0xffff;
	frp_entry_data[1].af = 1;
	frp_entry_data[1].nc = 1;
	frp_entry_data[1].frame_offset = 1;
	frp_entry_data[1].ok_index = next_off;
	frp_entry_data[1].dma_ch_no = channel;

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  entry_offset);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  entry_offset + 1);

	if (prev_off && prev_off < DN200_VF_UC_OFF) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val + 2));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val + 2));
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET
						       (prev_off_val));

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val));

		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val));
	}
	bitmap_set((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
	kfree(frp_entry_data);
	return offset;
}

/* uc mac addr add*/
static int dn200_add_pf_uc_rxp_da_route_sriov(struct mac_device_info *hw,
						 u8 *mac_addr, int offset)
{
	unsigned long bitmap;
	int prev_off;
	int next_off;
	u32 prev_off_val;
	u32 data;
	u32 channel = 0;
	u8 entry_offset = 0;
	int next_uc_offset = 0;
	struct RXP_FPR_ENTRY *frp_entry_data;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return -EINVAL;
	}

	frp_entry_data = kcalloc(3, sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return -ENOMEM;

	channel = (1 << DN200_RXQ_START_GET(hw));
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	if (!offset) {
		offset =
		    dn200_get_unused_bit(&bitmap, DN200_PF_UC_ADDR_END,
					 DN200_PF_UC_ADDR_START);
		if (offset < 0) {
			netdev_dbg(hw->priv->dev, "%s:uc unused bit smaller zero\n", __func__);
			return -EBUSY;
		}

		/*Skip this option if it has already been configured */
		if (bitmap & (1 << offset)) {
			netdev_dbg(hw->priv->dev, "%s: uc has already been configured\n", __func__);
			kfree(frp_entry_data);
			return offset;
		}
	}

	/*get prev used entry */
	prev_off = dn200_get_prev_used_bit(&bitmap, offset);
	if (prev_off < 0)
		prev_off = 0;

	entry_offset = rxp_offset_get_from_bitmap(offset);

	prev_off_val = rxp_offset_get_from_bitmap(prev_off);

	/* from the pre entry,we can get the entry that pre entry shot */
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_FIRST_ENTRY_OFFSET
					    (prev_off_val));
	next_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;
	frp_entry_data[0].match_data = cpu_to_le32(*(u32 *)(mac_addr));
	frp_entry_data[0].match_en = 0xffffffff;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].ok_index = next_off;
	frp_entry_data[0].frame_offset = 0;

	frp_entry_data[1].match_data = cpu_to_le32(*(u16 *)(mac_addr + 4));
	frp_entry_data[1].match_en = 0xffff;
	frp_entry_data[1].af = 1;
	frp_entry_data[1].rf = 1;
	frp_entry_data[1].nc = 0;
	frp_entry_data[1].frame_offset = 1;
	frp_entry_data[1].ok_index = (DN200_VLAN_ADDR_START);
	frp_entry_data[1].dma_ch_no = channel;

	/*when match ,need route the vlan entry */
	next_uc_offset = dn200_get_next_used_bit(&bitmap, offset, DN200_VF_UC_OFF);

	/*last uc*/
	if (next_uc_offset < 0) {
		if (prev_off < 1) {
			dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
							DA_OK_INDEX_FIRST_ENTRY_OFFSET(prev_off));
			frp_entry_data[2].match_data = 0;
			frp_entry_data[2].match_en = 0;
			frp_entry_data[2].af = 0;
			frp_entry_data[2].nc = 0;
			frp_entry_data[2].ok_index = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;
			frp_entry_data[2].dma_ch_no = channel;
			dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data
								  [2],
								  entry_offset +
								  2);
		} else {
			dwxgmac2_rxp_get_single_da_entry_sriov(hw,
							       &frp_entry_data[2],
							       prev_off_val + 2);
			dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data[2],
								  entry_offset + 2);
		}
	}

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  entry_offset);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  entry_offset + 1);

	if (next_uc_offset < 0) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (entry_offset));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset + 2) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (entry_offset));
	}

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_FIRST_ENTRY_OFFSET
					    (prev_off_val));
	data &= ~(OK_INDEX_MASK);
	data |= ((entry_offset) << OK_INDEX_OFFSET);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_OK_INDEX_FIRST_ENTRY_OFFSET
					       (prev_off_val));
	if (!prev_off_val) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					DA_OK_INDEX_SEC_ENTRY_OFFSET
					(prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					DA_OK_INDEX_SEC_ENTRY_OFFSET
					(prev_off_val));
	}

	bitmap_set((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
	kfree(frp_entry_data);
	return offset;
}

/*from lram ,we can get pf's ohter uc mac addr(not contain self) */
static int dn200_pf_lram_uc_add_rxp(struct mac_device_info *hw,
				       u8 *mac_addr)
{
	unsigned long bitmap_uc;
	unsigned long bitmap_pm;
	int j = 0, found = 0, unused_bit = 0;
	struct mac_addr_route *addr_route =
	    kmalloc(sizeof(struct mac_addr_route), GFP_ATOMIC);

	if (!addr_route)
		return -ENOMEM;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_uc, &bitmap_uc);
	for (j = 0; j < DN200_MAX_UC_MAC_ADDR_NUM; j++) {
		if (!(bitmap_uc & (1 << j)))
			continue;

		dn200_get_func_uc_mac_addr(hw, j, addr_route);
		if (memcmp(addr_route->mac_addr, mac_addr, ETH_ALEN) == 0) {
			found = 1;
			break;
		}
	}
	/*if found, we do nothing */
	if (!found) {
		addr_route->rxp_offset =
		    dn200_add_pf_uc_rxp_da_route_sriov(hw, mac_addr, 0);
		if (addr_route->rxp_offset <= 0) {
			kfree(addr_route);
			return 0;
		}
		addr_route->channel = (1 << DN200_RXQ_START_GET(hw));
		memcpy(addr_route->mac_addr, mac_addr, ETH_ALEN);
		/*supoort 16 pf uc mac addr ,but first can not change here */
		unused_bit =
		    dn200_get_unused_bit(&bitmap_uc, DN200_MAX_UC_MAC_ADDR_NUM,
					 0);
		if (unused_bit < 0) {	/*if full, we should set promisc */
			DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc,
						      &bitmap_pm);
			bitmap_pm |= DN200_RXQ_START_GET(hw);
			DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc,
						      bitmap_pm);
			kfree(addr_route);
			return 0;
		}
		dn200_set_func_uc_mac_addr(hw, unused_bit, addr_route);
		bitmap_set((unsigned long *)&bitmap_uc, unused_bit, 1);
		DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_uc, bitmap_uc);
	}

	kfree(addr_route);
	return 0;
}

static int dn200_del_vf_uc_rxp_da_route(struct mac_device_info *hw,
					    int offset)
{
	int prev_off;
	int next_off;
	u32 data;
	u32 prev_off_val;
	u8 entry_offset;
	unsigned long bitmap;
	struct RXP_FPR_ENTRY *frp_entry_data;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return  -EINVAL;
	}

	frp_entry_data =
	    kcalloc(3, sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return -ENOMEM;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	entry_offset = rxp_offset_get_from_bitmap(offset);

	prev_off = dn200_get_prev_used_bit(&bitmap, offset);
	prev_off_val = rxp_offset_get_from_bitmap(prev_off);
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_SEC_ENTRY_OFFSET(entry_offset));
	if (!data)
		goto free_mem;

	next_off = ((data & OK_INDEX_MASK) >> OK_INDEX_OFFSET);

	if (!next_off)
		goto free_mem;

	if (prev_off && prev_off < DN200_VF_UC_OFF) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val + 2));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val + 2));
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val));

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET
						       (prev_off_val));
	}

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  entry_offset);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  entry_offset + 1);

	bitmap_clear((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
free_mem:
	kfree(frp_entry_data);
	return 0;
}

/*rxp uc delete and laram delete*/
static int dn200_del_pf_uc_rxp_da_route(struct mac_device_info *hw,
					    int offset)
{
	int prev_off;
	int next_off;
	u32 data;
	u32 prev_off_val;
	int entry_offset;
	unsigned long bitmap;
	u8 tmp_off;
	struct RXP_FPR_ENTRY *frp_entry_data;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return -EINVAL;
	}

	frp_entry_data = kcalloc(3, sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return -ENOMEM;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	entry_offset = rxp_offset_get_from_bitmap(offset);

	prev_off = dn200_get_prev_used_bit(&bitmap, offset);
	prev_off_val = rxp_offset_get_from_bitmap(prev_off);

	next_off = dn200_get_next_used_bit(&bitmap, offset, DN200_VF_UC_OFF);
	if (next_off < 0) {	/*uc last */
		dwxgmac2_rxp_get_single_da_entry_sriov(hw, &frp_entry_data[0],
						       entry_offset + 2);

		/*delete 3's entry rxp*/
		dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data[1],
								  entry_offset);
		dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data[1],
								  entry_offset + 1);
		dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data[1],
								  entry_offset + 2);
		/*first uc*/
		if (!prev_off) {
			/*prev first ok_index*/
			dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET(prev_off_val));
			data &= ~(OK_INDEX_MASK);
			data |= (frp_entry_data[0].ok_index << OK_INDEX_OFFSET);
			dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					 DA_OK_INDEX_FIRST_ENTRY_OFFSET(prev_off_val));
			/*prev second ok_index*/
			dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET(prev_off_val));
			data &= ~(OK_INDEX_MASK);
			data |= (frp_entry_data[0].ok_index << OK_INDEX_OFFSET);
			dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET(prev_off_val));
		} else {
			dwxgmac2_rxp_update_single_da_entry_sriov(hw,
								  &frp_entry_data[0],
								  entry_offset);
		}
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (entry_offset));
		tmp_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= (tmp_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val));
		dwxgmac2_rxp_update_single_da_entry_sriov(hw,
							  &frp_entry_data[1],
							  entry_offset);
		dwxgmac2_rxp_update_single_da_entry_sriov(hw,
							  &frp_entry_data[1],
							  entry_offset + 1);
		if (!prev_off) {
			dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));
			data &= ~(OK_INDEX_MASK);
			data |= (tmp_off << OK_INDEX_OFFSET);
			dwxgmac2_rxp_update_single_entry_sriov(hw, data,
							DA_OK_INDEX_SEC_ENTRY_OFFSET
							(prev_off_val));
		}
	}

	bitmap_clear((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
	kfree(frp_entry_data);
	return 0;
}

static void dn200_clear_lram_pf_uc_rxp(struct mac_device_info *hw)
{
	unsigned long bitmap_uc;
	u64 j;
	struct mac_addr_route *addr_route =
	    kmalloc(sizeof(struct mac_addr_route), GFP_ATOMIC);

	if (!addr_route) {
		netdev_err(hw->priv->dev, "%s Alloc addr_route memory failed\n",
			   __func__);
		return;
	}

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_uc, &bitmap_uc);
	for (j = 0; j < DN200_MAX_UC_MAC_ADDR_NUM; j++) {
		if (!(bitmap_uc & (1ULL << j)))
			continue;

		dn200_get_func_uc_mac_addr(hw, j, addr_route);
		dn200_del_pf_uc_rxp_da_route(hw, addr_route->rxp_offset);
		memset(addr_route, 0, sizeof(struct mac_addr_route));
		dn200_set_func_uc_mac_addr(hw, j, addr_route);
		bitmap_clear((unsigned long *)&bitmap_uc, j, 1);
	}
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_uc, bitmap_uc);
	kfree(addr_route);
}

/*multi addr*/
/*clear/add mc rxp channel*/
static void dn200_mc_rxp_channel_route_set(struct mac_device_info *hw,
					      bool enable, u16 bitmap_promisc)
{
	int entry_offset;
	u32 data;
	u8 next_off;

	if (!bitmap_promisc)
		return;

	entry_offset = rxp_offset_get_from_bitmap(DN200_MC_ADDR_START);
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_SEC_ENTRY_OFFSET
					    (entry_offset));
	next_off = ((data & OK_INDEX_MASK) >> OK_INDEX_OFFSET);
	if (!next_off)
		return;

	/*get channel */
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_DMA_CHA_NO_OFFSET(entry_offset));
	if (enable)
		data |= bitmap_promisc;
	else
		data &= ~bitmap_promisc;
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET
					       (entry_offset));
	while (next_off < DN200_ALL_MULTCAST_OFF) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_DMA_CHA_NO_OFFSET
						    (next_off));
		if (enable)
			data |= bitmap_promisc;
		else
			data &= ~bitmap_promisc;
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_DMA_CHA_NO_OFFSET
						       (next_off));
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_DMA_CHA_NO_OFFSET
						    (next_off));
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (next_off));
		next_off = ((data & OK_INDEX_MASK) >> OK_INDEX_OFFSET);
		if (!next_off)
			return;
	}
}

static void dn200_del_mc_rxp_da_route(struct mac_device_info *hw, int offset)
{
	unsigned long bitmap;
	u8 prev_off;
	u32 prev_off_val;
	u8 next_off;
	u32 data;
	u8 entry_offset;
	struct RXP_FPR_ENTRY *frp_entry_data;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return;
	}

	frp_entry_data = kcalloc(2, sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);

	entry_offset = rxp_offset_get_from_bitmap(offset);

	prev_off = dn200_get_prev_used_bit(&bitmap, offset);

	prev_off_val = rxp_offset_get_from_bitmap(prev_off);

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_FIRST_ENTRY_OFFSET
					    (entry_offset));
	next_off = ((data & OK_INDEX_MASK) >> OK_INDEX_OFFSET);
	if (!next_off)
		goto free_mem;

	if (prev_off && (prev_off < (DN200_VF_UC_OFF))) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val + 2));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val + 2));
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val));

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= (next_off << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET
						       (prev_off_val));
	}

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  entry_offset);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  entry_offset + 1);

	bitmap_clear((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
free_mem:
	kfree(frp_entry_data);
}

static void dn200_update_bcmc_channel(struct mac_device_info *hw,
					int rxp_offset, bool add, u8 rxq_start)
{
	u32 data;

	if (rxp_offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return;
	}
	/* Disable RX Parser */
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data, (u8)rxp_offset);
	if (add)
		data |= (1 << rxq_start);
	else
		data &= ~(1 << rxq_start);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data, (u8)rxp_offset);
}

static void dn200_clear_mc_da_route(struct mac_device_info *hw, u8 rxq_start)
{
	unsigned long bitmap_mac;
	u64 j;
	struct mac_addr_route *addr_route =
	    kmalloc(sizeof(struct mac_addr_route), GFP_ATOMIC);

	if (!addr_route) {
		netdev_err(hw->priv->dev, "%s Alloc addr_route memory failed\n",
			   __func__);
		return;
	}

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_mac, &bitmap_mac);
	for (j = 0; j < DN200_MAX_MC_ADDR_NUM; j++) {
		if (!(bitmap_mac & (1ULL << j)))
			continue;

		dn200_get_func_mac_addr(hw, j, addr_route);
		if (addr_route->rxp_offset <= 0) {
			memset(addr_route, 0,
				       sizeof(struct mac_addr_route));
			bitmap_clear((unsigned long *)&bitmap_mac, j,
					     1);
			dn200_set_func_mac_addr(hw, j, addr_route);
			goto free_route;
		}
		if (addr_route->channel & (1 << rxq_start)) {	/*some func owns this mc */
			addr_route->channel &= ~(1 << rxq_start);
			if (addr_route->channel == 0) {	/*only one func owned this mac */
				dn200_del_mc_rxp_da_route(hw,
							     addr_route->rxp_offset);
				memset(addr_route, 0,
				       sizeof(struct mac_addr_route));
				bitmap_clear((unsigned long *)&bitmap_mac, j,
					     1);
			} else {	/*update channel */
				dn200_update_bcmc_channel(hw,
						 addr_route->rxp_offset, false, rxq_start);
			}
			dn200_set_func_mac_addr(hw, j, addr_route);
		}
	}

free_route:
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_mac, bitmap_mac);
	kfree(addr_route);
}

static void dwxgmac2_append_rxp_da_route_sriov(struct mac_device_info *hw,
					       int offset, u16 channel)
{
	u32 data;
	u32 entry_offset = 0;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return;
	}
	entry_offset = rxp_offset_get_from_bitmap(offset);
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_DMA_CHA_NO_OFFSET(entry_offset));

	data |= (1 << channel);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET
					       (entry_offset));
}

static int dwxgmac2_add_mc_rxp_da_route_sriov(struct mac_device_info *hw,
					      u8 *mac_addr, int offset, u8 rxq_start)
{
	unsigned long bitmap;
	int prev_off;
	int prev_off_val;
	int next_off;
	u32 data;
	u32 channel = 0;
	u8 entry_offset = 0;
	struct RXP_FPR_ENTRY *frp_entry_data;

	if (offset < 0) {
		netdev_err(hw->priv->dev, "%s: parameter offset less than zero\n", __func__);
		return -EINVAL;
	}

	if (!netif_running(hw->priv->dev))
		return -EBUSY;

	frp_entry_data = kmalloc(2 * sizeof(struct RXP_FPR_ENTRY), GFP_ATOMIC);
	if (!frp_entry_data)
		return -ENOMEM;

	channel = (1 << rxq_start);
	memset(&frp_entry_data[0], 0, sizeof(struct RXP_FPR_ENTRY));
	memset(&frp_entry_data[1], 0, sizeof(struct RXP_FPR_ENTRY));

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	if (!offset) {
		offset =
		    dn200_get_unused_bit(&bitmap, DN200_MC_ADDR_END,
					 DN200_MC_ADDR_START);
		if (offset < 0) {
			netdev_err(hw->priv->dev, "%s: mc unused bit smaller zero\n", __func__);
			kfree(frp_entry_data);
			return -EBUSY;
		}
	}
	/*Skip this option if it has already been configured */
	if (bitmap & (1ULL << offset)) {
		netdev_dbg(hw->priv->dev, "%s: mc has already been configured\n", __func__);
		kfree(frp_entry_data);
		return offset;
	}
	entry_offset = rxp_offset_get_from_bitmap(offset);

	prev_off = dn200_get_prev_used_bit(&bitmap, offset);

	prev_off_val = rxp_offset_get_from_bitmap(prev_off);

	if (prev_off && prev_off < DN200_VF_UC_OFF) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val + 2));

		next_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;

		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val + 2));
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (prev_off_val));

		next_off = (data & OK_INDEX_MASK) >> OK_INDEX_OFFSET;

		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET
						       (prev_off_val));

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (prev_off_val));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (prev_off_val));
	}

	frp_entry_data[0].match_data = cpu_to_le32(*(u32 *)(mac_addr));
	frp_entry_data[0].match_en = 0xffffffff;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].ok_index = next_off;
	frp_entry_data[0].frame_offset = 0;

	frp_entry_data[1].match_data = cpu_to_le32(*(u16 *)(mac_addr + 4));
	frp_entry_data[1].match_en = 0xffff;
	frp_entry_data[1].nc = 1;
	frp_entry_data[1].frame_offset = 1;
	frp_entry_data[1].dma_ch_no = channel;
	frp_entry_data[1].ok_index = next_off;
	frp_entry_data[1].af = 1;

	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0],
						  entry_offset);
	dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1],
						  entry_offset + 1);

	bitmap_set((unsigned long *)&bitmap, offset, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
	kfree(frp_entry_data);
	return offset;
}

static int dn200_mc_add_rxp(struct mac_device_info *hw, u8 *mac_addr, u8 rxq_start)
{
	unsigned long bitmap_mac;
	unsigned long bitmap_allmucast;
	int unused_bit = -1;
	int j;
	int found = 0;
	struct mac_addr_route *addr_route =
	    kmalloc(sizeof(struct mac_addr_route), GFP_ATOMIC);

	if (!addr_route) {
		netdev_err(hw->priv->dev, "%s Alloc addr_route memory failed\n",
			   __func__);
		return -ENOMEM;
	}

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_mac, &bitmap_mac);
	for (j = 0; j < DN200_MAX_MC_ADDR_NUM; j++) {
		if (!(bitmap_mac & (1ULL << j)))
			continue;
		dn200_get_func_mac_addr(hw, j, addr_route);
		if (memcmp(addr_route->mac_addr, mac_addr, ETH_ALEN) == 0) {
			found = 1;
			break;
		}
	}

	if (found) {
		if (addr_route->rxp_offset <= 0)
			goto free_route;

		if (addr_route->rxp_offset != 0) {
			if ((1 << rxq_start) ^ addr_route->channel) {
				addr_route->channel |=
				    (1 << rxq_start);
				dwxgmac2_append_rxp_da_route_sriov(hw,
								   addr_route->rxp_offset,
								   rxq_start);
			}
		}
		dn200_set_func_mac_addr(hw, j, addr_route);
	} else {
		unused_bit =
		    dn200_get_unused_bit(&bitmap_mac,
					 (DN200_MAX_MC_ADDR_NUM - 1), 0);
		if (unused_bit < 0) {
			/*set allmultist */
			DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast,
						      &bitmap_allmucast);
			bitmap_allmucast |= rxq_start;
			DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast,
						      bitmap_allmucast);

			kfree(addr_route);
			return -ENOMEM;
		}
		addr_route->rxp_offset =
		    dwxgmac2_add_mc_rxp_da_route_sriov(hw, mac_addr, 0, rxq_start);
		if (addr_route->rxp_offset < 0)
			goto free_route;
		addr_route->channel = (1 << rxq_start);
		memcpy(addr_route->mac_addr, mac_addr, ETH_ALEN);
		dn200_set_func_mac_addr(hw, unused_bit, addr_route);
		bitmap_set((unsigned long *)&bitmap_mac, unused_bit, 1);
		DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_mac, bitmap_mac);
	}
free_route:
	kfree(addr_route);
	return 0;
}

static int dn200_get_used_bit_from_last(unsigned long *bitmap, u8 last,
					u8 first)
{
	int i = last - 1;

	for (; i >= first; i--) {
		if ((bitmap[0] & (1ULL << i)))
			return i;
	}
	return -1;
}

/* update the last
 * update promsic
 * update allmultist
 */
static void dn200_update_ampm_rxp(struct mac_device_info *hw, u8 rxq_start)
{
	unsigned long bitmap;
	unsigned long bitmap_allmucast;
	unsigned long bitmap_promisc;
	u32 offset = 0;
	u32 offset_val = 0;
	u32 data;
	u32 entry_offset = 0;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	offset =
	    dn200_get_used_bit_from_last(&bitmap, (DN200_MC_ADDR_END + 1), 0);
	if (offset < 0)
		return;

	offset_val = rxp_offset_get_from_bitmap(offset);

	/*according to the bitmap,we can judge the direction */
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, &bitmap_allmucast);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, &bitmap_promisc);
	if (bitmap_allmucast)
		entry_offset = DN200_ALL_MULTCAST_OFF;
	else if (bitmap_promisc)
		entry_offset = DN200_ALL_PROMISC_OFF;
	else
		entry_offset = DN200_ALL_DROP_OFF;
	if (offset && (offset < (1 + 1 + 15))) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (offset_val + 2));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (offset_val + 2));
	} else {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_FIRST_ENTRY_OFFSET
						    (offset_val));
		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_FIRST_ENTRY_OFFSET
						       (offset_val));

		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DA_OK_INDEX_SEC_ENTRY_OFFSET
						    (offset_val));

		data &= ~(OK_INDEX_MASK);
		data |= ((entry_offset) << OK_INDEX_OFFSET);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DA_OK_INDEX_SEC_ENTRY_OFFSET
						       (offset_val));
	}

	/*judge only PF open and update ampm */
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_OK_INDEX_FIRST_ENTRY_OFFSET
					    (DN200_ALL_MULTCAST_OFF));
	if (bitmap_allmucast) {
		if (bitmap_promisc) {
			data &= ~(OK_INDEX_MASK);
			data |= ((DN200_ALL_PROMISC_OFF) << OK_INDEX_OFFSET);
		} else {
			data &= ~(OK_INDEX_MASK);
			data |= ((DN200_ALL_DROP_OFF) << OK_INDEX_OFFSET);
		}
	}
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_OK_INDEX_FIRST_ENTRY_OFFSET
					       (DN200_ALL_MULTCAST_OFF));

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_DMA_CHA_NO_OFFSET_AMPM
					    (DN200_ALL_MULTCAST_OFF));
	if (bitmap_allmucast & (1 << rxq_start))
		data |= (1 << rxq_start);
	else
		data &= ~(1 << rxq_start);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET_AMPM
					       (DN200_ALL_MULTCAST_OFF));

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    DA_DMA_CHA_NO_OFFSET_AMPM
					    (DN200_ALL_PROMISC_OFF));
	if (bitmap_promisc & (1 << rxq_start))
		data |= (1 << rxq_start);
	else
		data &= ~(1 << rxq_start);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET_AMPM
					       (DN200_ALL_PROMISC_OFF));

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    AFRFNC_ENTRY_OFFSET
					    (DN200_ALL_MULTCAST_OFF));
	data &= (~RF_ENABLE);
	data |= AF_ENABLE;
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       AFRFNC_ENTRY_OFFSET
					       (DN200_ALL_MULTCAST_OFF));

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
					    AFRFNC_ENTRY_OFFSET
					    (DN200_ALL_PROMISC_OFF));
	if (bitmap_promisc & BIT(0)) {	/*PF open */
		if (bitmap_promisc & GENMASK(15, 8)) {
			data &= (~RF_ENABLE);
			data |= AF_ENABLE;
		} else {	/* only pf open */
			data &= (~RF_ENABLE);
			data &= (~AF_ENABLE);
		}
	} else if (bitmap_promisc) {
		data &= (~RF_ENABLE);
		data |= AF_ENABLE;
	}
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       AFRFNC_ENTRY_OFFSET
					       (DN200_ALL_PROMISC_OFF));
}

static void dn200_clear_allmu_promisc_da_route(struct mac_device_info *hw,
						 int offset, u8 rxq_start)
{
	unsigned long bitmap_am;
	unsigned long bitmap_pm;
	u32 data;

	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, &bitmap_am);
	DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, &bitmap_pm);
	/*clear allmuticast */
	if (bitmap_am && (bitmap_am & (1 << rxq_start))) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DN200_ALL_MULTCAST_OFF);
		data &= ~(1 << offset);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DN200_ALL_MULTCAST_OFF);
		bitmap_clear((unsigned long *)&bitmap_am,
			     rxq_start, 1);
	}

	if (bitmap_pm && (bitmap_pm & (1 << rxq_start))) {
		dwxgmac2_rxp_get_single_entry_sriov(hw, &data,
						    DN200_ALL_PROMISC_OFF);
		data &= ~(1 << offset);
		dwxgmac2_rxp_update_single_entry_sriov(hw, data,
						       DN200_ALL_PROMISC_OFF);
		bitmap_clear((unsigned long *)&bitmap_pm,
			     rxq_start, 1);
	}
}

static int dwxgmac_config_rxp_broadcast_sriov(struct mac_device_info *hw)
{
	struct RXP_FPR_ENTRY frp_entry_data[5];
	u32 channel = 0;
	u32 val;
	unsigned long bitmap = 0;
	unsigned long bitmap_ampm = 0;
	int ret = 0;

	channel = (1 << 0);
	memset(&frp_entry_data[0], 0, sizeof(struct RXP_FPR_ENTRY));
	memset(&frp_entry_data[1], 0, sizeof(struct RXP_FPR_ENTRY));
	memset(&frp_entry_data[2], 0, sizeof(struct RXP_FPR_ENTRY));
	memset(&frp_entry_data[3], 0, sizeof(struct RXP_FPR_ENTRY));
	memset(&frp_entry_data[4], 0, sizeof(struct RXP_FPR_ENTRY));

	frp_entry_data[0].match_data = 0xffffffff;
	frp_entry_data[0].match_en = 0xffffffff;
	frp_entry_data[0].nc = 1;
	frp_entry_data[0].ok_index = DN200_ALL_DROP_OFF;
	frp_entry_data[0].frame_offset = 0;
	frp_entry_data[0].dma_ch_no = channel;

	frp_entry_data[1].match_data = 0xffff;
	frp_entry_data[1].match_en = 0xffff;
	frp_entry_data[1].nc = 0;
	frp_entry_data[1].frame_offset = 1;
	frp_entry_data[1].dma_ch_no = channel;
	frp_entry_data[1].ok_index = DN200_ALL_DROP_OFF;
	frp_entry_data[1].af = 1;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[0], 0);
	if (ret)
		goto rxp_updt_err;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[1], 1);
	if (ret)
		goto rxp_updt_err;

	frp_entry_data[2].af = 1;
	frp_entry_data[2].rf = 1;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[2],
						  DN200_BC_RXP_OFF);
	if (ret)
		goto rxp_updt_err;

	frp_entry_data[2].af = 0;
	frp_entry_data[2].rf = 1;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[2],
						  DN200_ALL_DROP_OFF);
	if (ret)
		goto rxp_updt_err;

	/*allmulcast */
	frp_entry_data[3].match_data = 0x1;
	frp_entry_data[3].match_en = 0x1;
	frp_entry_data[3].nc = 1;
	frp_entry_data[3].ok_index = DN200_ALL_DROP_OFF;
	frp_entry_data[3].frame_offset = 0;
	frp_entry_data[3].dma_ch_no = 0;
	frp_entry_data[3].af = 1;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[3],
						  DN200_ALL_MULTCAST_OFF);
	if (ret)
		goto rxp_updt_err;

	/*promsic */
	frp_entry_data[4].nc = 0;
	frp_entry_data[4].ok_index = DN200_BC_RXP_OFF;
	frp_entry_data[4].frame_offset = 0;
	frp_entry_data[4].dma_ch_no = 0;
	frp_entry_data[4].af = 1;
	frp_entry_data[4].rf = 0;
	ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data[4],
						  DN200_ALL_PROMISC_OFF);
	if (ret)
		goto rxp_updt_err;

	val = (DN200_MAX_USED_RXP_NUM << 16) & XGMAC_NPE;
	val |= DN200_MAX_USED_RXP_NUM & XGMAC_NVE;
	writel(val, hw->pcsr + XGMAC_MTL_RXP_CONTROL_STATUS);
	// DN200_GET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, &bitmap);
	bitmap_set((unsigned long *)&bitmap, 0, 1);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_rxp, bitmap);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_uc, 0);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_allmucast, bitmap_ampm);
	DN200_SET_LRAM_MAILBOX_MEMBER(hw, bitmap_promisc, bitmap_ampm);
	dwxgmac3_rxp_enable(hw->pcsr);
rxp_updt_err:
	return ret;
}

/*append broadcast dma_channel*/
static void dwxgmac2_vf_append_rxp_bc_sriov(struct mac_device_info *hw,
					    u16 channel)
{
	u32 data;

	dwxgmac2_rxp_get_single_entry_sriov(hw, &data, DA_DMA_CHA_NO_OFFSET(0));

	data |= (1 << channel);
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET(0));
}

static int dwxgmac_reset_rxp(struct mac_device_info *hw)
{
	int i = 0;
	int ret = 0;
	struct RXP_FPR_ENTRY frp_entry_data = { 0 };

	for (; i < 128; i++) {
		ret = dwxgmac2_rxp_update_single_da_entry_sriov(hw, &frp_entry_data, i);
		if (ret)
			break;
	}
	return ret;
}

static void dwxgmac_get_rxp_filter_sriov(struct mac_device_info *hw,
					 struct seq_file *seq)
{
	int i = 0;
	struct RXP_FPR_ENTRY frp_entry_data = { 0 };
	u32 *data;

	for (; i < DN200_MAX_USED_RXP_NUM; i++) {
		data = (u32 *)&frp_entry_data;
		dwxgmac2_rxp_get_single_entry_sriov(hw, data, i * 4);
		data = ((u32 *)&frp_entry_data) + 1;
		dwxgmac2_rxp_get_single_entry_sriov(hw, data, i * 4 + 1);
		data = ((u32 *)&frp_entry_data) + 2;
		dwxgmac2_rxp_get_single_entry_sriov(hw, data, i * 4 + 2);
		data = ((u32 *)&frp_entry_data) + 3;
		dwxgmac2_rxp_get_single_entry_sriov(hw, data, i * 4 + 3);
		seq_puts(seq, "==============\n");
		seq_printf(seq, "RXP ENTRY %d\n", i);
		seq_printf(seq, "%16s: %08x\n", "match_data",
			   frp_entry_data.match_data);
		seq_printf(seq, "%16s: %08x\n", "match_en",
			   frp_entry_data.match_en);
		seq_printf(seq, "%16s: %8x\n", "af", frp_entry_data.af);
		seq_printf(seq, "%16s: %8x\n", "rf", frp_entry_data.rf);
		seq_printf(seq, "%16s: %8x\n", "im", frp_entry_data.im);
		seq_printf(seq, "%16s: %8x\n", "frame_offset",
			   frp_entry_data.frame_offset);
		seq_printf(seq, "%16s: %8x\n", "nc", frp_entry_data.nc);
		seq_printf(seq, "%16s: %8d\n", "ok_index",
			   frp_entry_data.ok_index);
		seq_printf(seq, "%16s: %8x\n", "dma_ch_no",
			   frp_entry_data.dma_ch_no);
	}
}

static struct dn200_tc_entry *dwxgmac3_rxp_get_next_entry(struct dn200_tc_entry
							  *entries,
							  unsigned int count,
							  u32 curr_prio)
{
	struct dn200_tc_entry *entry;
	u32 min_prio = ~0x0;
	int i, min_prio_idx;
	bool found = false;

	for (i = count - 1; i >= 0; i--) {
		entry = &entries[i];

		/* Do not update unused entries */
		if (!entry->in_use)
			continue;
		/* Do not update already updated entries (i.e. fragments) */
		if (entry->in_hw)
			continue;
		/* Let last entry be updated last */
		if (entry->is_last)
			continue;
		/* Do not return fragments */
		if (entry->is_frag)
			continue;
		/* Check if we already checked this prio */
		if (entry->prio < curr_prio)
			continue;
		/* Check if this is the minimum prio */
		if (entry->prio < min_prio) {
			min_prio = entry->prio;
			min_prio_idx = i;
			found = true;
		}
	}

	if (found)
		return &entries[min_prio_idx];
	return NULL;
}

static int dwxgmac3_rxp_config_sriov(struct mac_device_info *hw,
				     struct dn200_tc_entry *entries,
				     unsigned int count)
{
	int ret, nve = DN200_BC_RXP_OFF;
	u32 old_val, val;
	void __iomem *ioaddr = hw->pcsr;
	/* Force disable RX */
	old_val = readl(ioaddr + XGMAC_RX_CONFIG);
	val = old_val & ~XGMAC_CONFIG_RE;
	writel(val, ioaddr + XGMAC_RX_CONFIG);

	/* Disable RX Parser */
	ret = dwxgmac3_rxp_disable(ioaddr);
	if (ret)
		goto re_enable;

	if (!nve)
		goto re_enable;

	/* Assume n. of parsable entries == n. of valid entries */
	val = (nve << 16) & XGMAC_NPE;
	val |= nve & XGMAC_NVE;
	writel(val, ioaddr + XGMAC_MTL_RXP_CONTROL_STATUS);

	/* Enable RX Parser */
	dwxgmac3_rxp_enable(ioaddr);

re_enable:
	/* Re-enable RX */
	writel(old_val, ioaddr + XGMAC_RX_CONFIG);
	return ret;
}

static int dwxgmac3_rxp_config_purepf(struct mac_device_info *hw,
				      struct dn200_tc_entry *entries,
				      unsigned int count)
{
	struct dn200_tc_entry *entry, *frag;
	int i, ret, nve = 0;
	u32 curr_prio = 0;
	u32 old_val, val;
	void __iomem *ioaddr = hw->pcsr;
	/* Force disable RX */
	old_val = readl(ioaddr + XGMAC_RX_CONFIG);
	val = old_val & ~XGMAC_CONFIG_RE;
	writel(val, ioaddr + XGMAC_RX_CONFIG);

	/* Disable RX Parser */
	ret = dwxgmac3_rxp_disable(ioaddr);
	if (ret)
		goto re_enable;

	/* Set all entries as NOT in HW */
	for (i = 0; i < count; i++) {
		entry = &entries[i];
		entry->in_hw = false;
	}

	/* Update entries by reverse order */
	while (1) {
		entry = dwxgmac3_rxp_get_next_entry(entries, count, curr_prio);
		if (!entry)
			break;

		curr_prio = entry->prio;
		frag = entry->frag_ptr;

		/* Set special fragment requirements */
		if (frag) {
			entry->val.af = 0;
			entry->val.rf = 0;
			entry->val.nc = 1;
			entry->val.ok_index = nve + 2;
		}

		ret = dwxgmac3_rxp_update_single_entry(hw, entry, nve);
		if (ret)
			goto re_enable;

		entry->table_pos = nve++;
		entry->in_hw = true;

		if (frag && !frag->in_hw) {
			ret = dwxgmac3_rxp_update_single_entry(hw, frag, nve);
			if (ret)
				goto re_enable;
			frag->table_pos = nve++;
			frag->in_hw = true;
		}
	}

	if (!nve)
		goto re_enable;

	/* Update all pass entry */
	for (i = 0; i < count; i++) {
		entry = &entries[i];
		if (!entry->is_last)
			continue;

		ret = dwxgmac3_rxp_update_single_entry(hw, entry, nve);
		if (ret)
			goto re_enable;

		entry->table_pos = nve++;
	}

	/* Assume n. of parsable entries == n. of valid entries */
	val = (nve << 16) & XGMAC_NPE;
	val |= nve & XGMAC_NVE;
	writel(val, ioaddr + XGMAC_MTL_RXP_CONTROL_STATUS);

	/* Enable RX Parser */
	dwxgmac3_rxp_enable(ioaddr);

re_enable:
	/* Re-enable RX */
	writel(old_val, ioaddr + XGMAC_RX_CONFIG);
	return ret;
}

static void dwxgmac2_clear_vf_rxp_route(struct mac_device_info *hw, u8 vf_off)
{
	u32 data;
	u8 vf_rx_start, off;

	if (HW_IS_VF(hw) || !PRIV_SRIOV_SUPPORT(hw->priv))
		return;

	vf_rx_start = hw->priv->plat_ex->pf.vfs[vf_off].rx_queue_start;
	/*unicast */
	dn200_mc_rxp_channel_route_set(hw, false,
					  (1 << vf_rx_start));

	dn200_clear_mc_da_route(hw, vf_rx_start);	/*pf and vf */

	/*broadcast */
	dwxgmac2_rxp_get_single_entry_sriov(hw, &data, DA_DMA_CHA_NO_OFFSET(0));
	data &= ~(1 << (vf_rx_start));
	dwxgmac2_rxp_update_single_entry_sriov(hw, data,
					       DA_DMA_CHA_NO_OFFSET(0));

	dn200_clear_allmu_promisc_da_route(hw, vf_rx_start, vf_rx_start);	/*pf and vf */

	/*delete rxp unicast mac addr */
	if (HW_IS_VF(hw))
		off = (vf_rx_start - 8) + DN200_VF_UC_OFF;
	else
		off = 1;
	dn200_del_vf_uc_rxp_da_route(hw, off);
}

static void dwxgmac2_wq_vf_del_rxp_route(struct mac_device_info *hw, int offset, u8 rxq_start)
{
	u8 off;

	/*broadcast */
	dn200_update_bcmc_channel(hw, DA_DMA_CHA_NO_OFFSET(0), false, rxq_start);

	/*delete rxp unicast mac addr */
	off = (rxq_start - 8) + DN200_VF_UC_OFF;
	dn200_del_vf_uc_rxp_da_route(hw, off);

	dn200_mc_rxp_channel_route_set(hw, false,
					  (1 << rxq_start));

	/*delete rxp multi mac addr */
	dn200_clear_mc_da_route(hw, rxq_start);	/*pf and vf */
	/*pm am */
	dn200_clear_allmu_promisc_da_route(hw, rxq_start, rxq_start);
}

static void dwxgmac2_vf_del_rxp_route(struct mac_device_info *hw)
{
	u8 wakeup_wq;

	dwxgmac2_vf_set_async_info(hw, hw->priv->dev, &wakeup_wq, DN200_VF_CLEAR_RXP);
}

const struct dn200_ops dwxgmac_purepf_ops = {
	.core_init = dwxgmac2_core_init,
	.set_mac = dwxgmac2_set_mac,
	.set_mac_rx = dwxgmac2_mac_rx_set,
	.get_mac_rx = dwxgmac2_mac_rx_get,
	.rx_ipc = dwxgmac2_rx_ipc,
	.rx_queue_enable = dwxgmac2_rx_queue_enable,
	.rx_queue_disable = dwxgmac2_rx_queue_disable,
	.rx_dds_config = NULL,
	.rx_queue_prio = dwxgmac2_rx_queue_prio,
	.tx_queue_prio = dwxgmac2_tx_queue_prio,
	.rx_queue_routing = NULL,
	.prog_mtl_rx_algorithms = dwxgmac2_prog_mtl_rx_algorithms,
	.prog_mtl_tx_algorithms = dwxgmac2_prog_mtl_tx_algorithms,
	.set_mtl_tx_queue_weight = dwxgmac2_set_mtl_tx_queue_weight,
	.set_mtl_rx_queue_weight = dwxgmac2_set_mtl_rx_queue_weight,
	.map_mtl_to_dma = dwxgmac2_map_mtl_to_dma,
	.mtl_dynamic_chan_set = dwxgmac2_mtl_dynamic_chan_set,
	.config_cbs = dwxgmac2_config_cbs,
	.dump_regs = dwxgmac2_dump_regs,
	.host_irq_status = dwxgmac2_host_irq_status,
	.host_mtl_irq_status = dwxgmac2_host_mtl_irq_status,
	.flow_ctrl = dwxgmac2_flow_ctrl,
	.set_umac_addr = dwxgmac2_set_umac_addr,
	.get_umac_addr = dwxgmac2_get_umac_addr,
	.set_eee_mode = dwxgmac2_set_eee_mode,
	.reset_eee_mode = dwxgmac2_reset_eee_mode,
	.set_eee_timer = dwxgmac2_set_eee_timer,
	.set_eee_pls = dwxgmac2_set_eee_pls,
	.pcs_ctrl_ane = NULL,
	.pcs_rane = NULL,
	.pcs_get_adv_lp = NULL,
	.debug = NULL,
	.set_filter = dwxgmac2_set_filter_purepf,
	.safety_feat_config = dwxgmac3_safety_feat_config,
	.safety_feat_irq_status = dwxgmac3_safety_feat_irq_status,
	.safety_feat_dump = dwxgmac3_safety_feat_dump,
	.set_mac_loopback = dwxgmac2_set_mac_loopback,
	.rss_configure = dwxgmac2_rss_configure,
	.update_vlan_hash = dwxgmac2_update_vlan_hash,
	.rxp_config = dwxgmac3_rxp_config_purepf,
	.get_mac_tx_timestamp = dwxgmac2_get_mac_tx_timestamp,
	.flex_pps_config = dwxgmac2_flex_pps_config,
	.sarc_configure = dwxgmac2_sarc_configure,
	.enable_vlan = dwxgmac2_enable_vlan,
	.config_l3_filter = dwxgmac2_config_l3_filter,
	.config_l4_filter = dwxgmac2_config_l4_filter,
	.config_ntuple_filter = dwxgmac2_config_ntuple_filter,
	.l3_l4_filter_config = dwxgmac2_config_l3l4_filter,
	.set_arp_offload = dwxgmac2_set_arp_offload,
	.est_configure = dwxgmac3_est_configure,
	.fpe_configure = dwxgmac3_fpe_configure,
	.rxp_broadcast = NULL,
	.rxp_filter_get = NULL,
	.add_hw_vlan_rx_fltr = dwxgmac2_add_hw_vlan_rx_fltr,
	.del_hw_vlan_rx_fltr = dwxgmac2_del_hw_vlan_rx_fltr,
	.init_hw_vlan_rx_fltr = dwxgmac2_hw_vlan_init,
	.config_vlan_rx_fltr = dwxgmac2_config_vlan_rx_fltr,
	.rx_vlan_stripping_config = dwxgmac2_rx_vlan_stripping_config,
	.tx_queue_flush = dwxgmac2_mtl_flush,
	.reset_rxp = dwxgmac_reset_rxp,
	.rxf_and_acl_mem_reset = dwxgmac2_rxf_and_acl_mem_reset,
};

const struct dn200_ops dwxgmac_sriov_ops = {
	.core_init = dwxgmac2_core_init,
	.set_mac = dwxgmac2_set_mac,
	.set_mac_rx = dwxgmac2_mac_rx_set,
	.get_mac_rx = dwxgmac2_mac_rx_get,
	.rx_ipc = dwxgmac2_rx_ipc,
	.rx_queue_enable = dwxgmac2_rx_queue_enable,
	.rx_queue_disable = dwxgmac2_rx_queue_disable,
	.rx_dds_config = dwxgmac2_rx_dds_config_sriov,
	.rx_queue_prio = dwxgmac2_rx_queue_prio,
	.tx_queue_prio = dwxgmac2_tx_queue_prio,
	.rx_queue_routing = NULL,
	.prog_mtl_rx_algorithms = dwxgmac2_prog_mtl_rx_algorithms,
	.prog_mtl_tx_algorithms = dwxgmac2_prog_mtl_tx_algorithms,
	.set_mtl_tx_queue_weight = dwxgmac2_set_mtl_tx_queue_weight,
	.set_mtl_rx_queue_weight = dwxgmac2_set_mtl_rx_queue_weight,
	.map_mtl_to_dma = dwxgmac2_map_mtl_to_dma,
	.mtl_dynamic_chan_set = dwxgmac2_mtl_dynamic_chan_set,
	.config_cbs = dwxgmac2_config_cbs,
	.dump_regs = dwxgmac2_dump_regs,
	.host_irq_status = dwxgmac2_host_irq_status,
	.host_mtl_irq_status = dwxgmac2_host_mtl_irq_status,
	.flow_ctrl = dwxgmac2_flow_ctrl,
	.set_umac_addr = dwxgmac2_set_umac_addr,
	.wq_set_umac_addr = dwxgmac2_wq_set_umac_addr,
	.get_umac_addr = NULL,
	.set_eee_mode = dwxgmac2_set_eee_mode,
	.reset_eee_mode = dwxgmac2_reset_eee_mode,
	.set_eee_timer = dwxgmac2_set_eee_timer,
	.set_eee_pls = dwxgmac2_set_eee_pls,
	.pcs_ctrl_ane = NULL,
	.pcs_rane = NULL,
	.pcs_get_adv_lp = NULL,
	.debug = NULL,
	.set_filter = dwxgmac2_set_filter_sriov,
	.wq_set_filter = dwxgmac2_wq_set_filter,
	.safety_feat_config = dwxgmac3_safety_feat_config,
	.safety_feat_irq_status = dwxgmac3_safety_feat_irq_status,
	.safety_feat_dump = dwxgmac3_safety_feat_dump,
	.set_mac_loopback = dwxgmac2_set_mac_loopback,
	.rss_configure = dwxgmac2_rss_configure,
	.update_vlan_hash = dwxgmac2_update_vlan_hash,
	.rxp_config = dwxgmac3_rxp_config_sriov,
	.get_mac_tx_timestamp = dwxgmac2_get_mac_tx_timestamp,
	.flex_pps_config = dwxgmac2_flex_pps_config,
	.sarc_configure = dwxgmac2_sarc_configure,
	.enable_vlan = dwxgmac2_enable_vlan,
	.config_l3_filter = dwxgmac2_config_l3_filter,
	.config_l4_filter = dwxgmac2_config_l4_filter,
	.config_ntuple_filter = dwxgmac2_config_ntuple_filter,
	.l3_l4_filter_config = dwxgmac2_config_l3l4_filter,
	.set_arp_offload = dwxgmac2_set_arp_offload,
	.est_configure = dwxgmac3_est_configure,
	.fpe_configure = dwxgmac3_fpe_configure,
	.rxp_broadcast = dwxgmac_config_rxp_broadcast_sriov,
	.rxp_filter_get = dwxgmac_get_rxp_filter_sriov,
	.rxp_clear = dwxgmac2_rxp_clear_entry_sriov,
	.vf_del_rxp = dwxgmac2_vf_del_rxp_route,
	.wq_vf_del_rxp = dwxgmac2_wq_vf_del_rxp_route,
	.clear_vf_rxp = dwxgmac2_clear_vf_rxp_route,
	.vf_append_rxp_bc = dwxgmac2_vf_append_rxp_bc_sriov,
	.mtl_reset = dwxgmac2_mtl_reset,
	.tx_queue_flush = dwxgmac2_mtl_flush,
	.config_vlan_rx_fltr = dwxgmac2_rxp_vlan_filter_config,
	.add_hw_vlan_rx_fltr = dwxgmac2_pf_add_rxp_vlan_route,
	.init_hw_vlan_rx_fltr = dwxgmac2_sriov_init_rxp_vlan_route,
	.del_hw_vlan_rx_fltr = dwxgmac2_pf_del_rxp_vlan_route,
	.rx_vlan_stripping_config = dwxgmac2_rx_vlan_stripping_config,
	.reset_rxp = dwxgmac_reset_rxp,
	.rxf_and_acl_mem_reset = dwxgmac2_rxf_and_acl_mem_reset,
};

static u32 dwxgmac2_get_num_vlan(void __iomem *ioaddr)
{
	u32 val, num_vlan;

	val = readl(ioaddr + XGMAC_HW_FEATURE3);
	switch (val & XGMAC_HWFEAT_NRVF) {
	case 0:
		num_vlan = 1;
		break;
	case 1:
		num_vlan = 4;
		break;
	case 2:
		num_vlan = 8;
		break;
	case 3:
		num_vlan = 16;
		break;
	case 4:
		num_vlan = 24;
		break;
	case 5:
		num_vlan = 32;
		break;
	case 7:
		num_vlan = 4096;
		break;
	default:
		num_vlan = 1;
	}
	return num_vlan;
}

int dwxgmac2_setup(struct dn200_priv *priv)
{
	struct mac_device_info *mac = priv->hw;

	mac->pcsr = priv->ioaddr;
	mac->pmail = priv->plat_ex->pf.ioaddr;
	mac->multicast_filter_bins = priv->plat->multicast_filter_bins;
	mac->unicast_filter_entries = priv->plat->unicast_filter_entries;
	mac->mcast_bits_log2 = 0;

	if (mac->multicast_filter_bins)
		mac->mcast_bits_log2 = ilog2(mac->multicast_filter_bins);

	mac->link.duplex = 0;
	mac->link.speed10 = XGMAC_CONFIG_SS_10_MII;
	mac->link.speed100 = XGMAC_CONFIG_SS_100_MII;
	mac->link.speed1000 = XGMAC_CONFIG_SS_1000_GMII;
	mac->link.speed2500 = XGMAC_CONFIG_SS_2500_GMII;
	mac->link.xgmii.speed2500 = XGMAC_CONFIG_SS_2500;
	mac->link.xgmii.speed5000 = XGMAC_CONFIG_SS_5000;
	mac->link.xgmii.speed10000 = XGMAC_CONFIG_SS_10000;
	mac->link.speed_mask = XGMAC_CONFIG_SS_MASK;

	mac->mii.addr = XGMAC_MDIO_ADDR;
	mac->mii.data = XGMAC_MDIO_DATA;
	mac->mii.addr_shift = 16;
	mac->mii.addr_mask = GENMASK(20, 16);
	mac->mii.reg_shift = 0;
	mac->mii.reg_mask = GENMASK(15, 0);
	mac->mii.clk_csr_shift = 19;
	mac->mii.clk_csr_mask = GENMASK(21, 19);
	if (HW_IS_PUREPF(priv->hw))
		mac->max_vlan_num = dwxgmac2_get_num_vlan(priv->ioaddr);

	return 0;
}
