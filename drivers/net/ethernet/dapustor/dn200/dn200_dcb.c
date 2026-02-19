// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */
#include <linux/bitops.h>
#include <linux/ethtool.h>
#include "dn200_dcb.h"

#define DN200_DMA_STOP_TIMEOUT				1
#define DN200_DCB_MAX_FLOW_CONTROL_QUEUES	8
#define DN200_FIFO_MIN_ALLOC				2048

#define DN200_PRIO_QUEUES(_cnt)		min_t(unsigned int, IEEE_8021QAZ_MAX_TCS, (_cnt))
#define DN200_FIFO_UNIT				256

static u8 dn200_dcb_getdcbx(struct net_device *netdev)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	netif_dbg(priv, drv, netdev, "(%s) get dcbx\n", __func__);
	return DCB_CAP_DCBX_HOST | DCB_CAP_DCBX_VER_IEEE;
}

static u8 dn200_dcb_setdcbx(struct net_device *netdev, u8 dcbx)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);

	netif_dbg(priv, drv, netdev, "set DCBX=%#x\n", dcbx);

	if (dcbx & ~support)
		return 1;

	if ((dcbx & support) != support)
		return 1;

	return 0;
}

static void dn200_config_queue_mapping(struct dn200_priv *priv)
{
	u32 queue;

	/* Map the TxQ to Traffic Class in dn200_dma_tx_mode(), like
	 * TxQ[0,7]->TC[0,7].
	 */
	for (queue = 0; queue < priv->plat->tx_queues_to_use; queue++)
		priv->q2tc_map[queue] = queue;

	/* Map the 8 VLAN priority values to available Rx Queues in
	 * dn200_mac_config_rx_queues_prio()
	 */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++)
		priv->prio2q_map[queue] = queue;

	/**
	 * Set static mapping in dn200_rx_queue_dma_chan_map(), like
	 * RxQ[0,7]->channel[0,7] in dn200_default_data(). However
	 * dynamic mapping will most likely be configured instead of
	 * static mapping, such as RSS, RXP and etc.
	 */
}

static void dn200_config_tc(struct dn200_priv *priv)
{
	unsigned int offset, queue, prio;
	u8 i;

	netdev_reset_tc(priv->dev);
	if (!priv->num_tcs)
		return;

	netdev_set_num_tc(priv->dev, priv->num_tcs);
	netif_dbg(priv, drv, priv->dev, "num_tc %d\n", priv->dev->num_tc);

	for (i = 0, queue = 0, offset = 0; i < priv->num_tcs; i++) {
		while ((queue < priv->plat->tx_queues_to_use) &&
		       (priv->q2tc_map[queue] == i))
			queue++;

		netif_dbg(priv, drv, priv->dev, "TC%u using TXq%u-%u\n",
			  i, offset, queue - 1);
		netdev_set_tc_queue(priv->dev, i, queue - offset, offset);
		offset = queue;
	}

	if (!priv->ets)
		return;

	for (prio = 0; prio < IEEE_8021QAZ_MAX_TCS; prio++) {
		netdev_set_prio_tc_map(priv->dev, prio,
				       priv->ets->prio_tc[prio]);
		netif_dbg(priv, drv, priv->dev, "prio %d assigned to tc %d\n",
			  prio, priv->ets->prio_tc[prio]);
	}
}

static void dn200_config_dcb_tc(struct dn200_priv *priv)
{
	struct ieee_ets *ets = priv->ets;
	unsigned int total_weight, min_weight, weight;
	unsigned int mask;
	unsigned int i, prio;
	u32 value;

	if (!ets)
		return;

	/* Set Tx to deficit weighted round robin scheduling algorithm (when
	 * traffic class is using ETS algorithm)
	 */
	value = readl(priv->ioaddr + XGMAC_MTL_OPMODE);
	value &= ~XGMAC_ETSALG;
	value |= XGMAC_DWRR;
	writel(value, priv->ioaddr + XGMAC_MTL_OPMODE);

	/* Set Traffic Class algorithms */
	total_weight = priv->dev->mtu * priv->dma_cap.tc_cnt;
	min_weight = total_weight / 100;
	if (!min_weight)
		min_weight = 1;

	/* Initialize prio2tc bitmap to 0 */
	for (i = 0; i < 8; i++)
		priv->prio2tc_bitmap[i] = 0;

	for (i = 0; i < priv->dma_cap.tc_cnt; i++) {
		/* Map the priorities to the traffic class */
		mask = 0;
		for (prio = 0; prio < IEEE_8021QAZ_MAX_TCS; prio++) {
			if (ets->prio_tc[prio] == i) {
				mask |= (1 << prio);
				priv->prio2tc_bitmap[prio] |= (1 << i);
			}
		}
		mask &= 0xff;
		netif_dbg(priv, drv, priv->dev, "TC%u PRIO mask=%#x\n", i,
			  mask);

		/* Map priorities to the traffic class */
		value =
		    readl(priv->ioaddr + XGMAC_TC_PRTY_MAP0 + 0x4 * (i / 4));
		value &= ~XGMAC_PSTC(i % 4);
		value |= (mask << XGMAC_PSTC_SHIFT(i % 4)) & XGMAC_PSTC(i % 4);
		writel(value,
		       priv->ioaddr + XGMAC_TC_PRTY_MAP0 + 0x4 * (i / 4));
		netif_dbg(priv, drv, priv->dev,
			  "REG[TC_PRTY_MAP%d] value=%08x\n", i / 4, value);

		/* Set the traffic class algorithm */
		switch (ets->tc_tsa[i]) {
		case IEEE_8021QAZ_TSA_STRICT:
			netif_dbg(priv, drv, priv->dev, "TC%u using SP\n", i);
			value =
			    readl(priv->ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
			value &= ~XGMAC_TSA;
			value |= XGMAC_SP;
			writel(value,
			       priv->ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
			break;
		case IEEE_8021QAZ_TSA_ETS:
			weight = total_weight * ets->tc_tx_bw[i] / 100;
			weight = clamp(weight, min_weight, total_weight);
			netif_dbg(priv, drv, priv->dev,
				  "TC%u using DWRR (weight %u)\n", i, weight);
			/* Must set ETS if algorithm is WRR, WFQ or DWRR */
			value =
			    readl(priv->ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
			value &= ~XGMAC_TSA;
			value |= XGMAC_ETS;
			writel(value,
			       priv->ioaddr + XGMAC_MTL_TCx_ETS_CONTROL(i));
			/* Set TC quantum */
			writel(weight,
			       priv->ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(i));
			break;
		default:
			break;
		}
	}

	dn200_config_tc(priv);
}

static int dn200_dcb_ieee_getets(struct net_device *netdev,
				 struct ieee_ets *ets)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	/* Set number of supported traffic classes */
	ets->ets_cap = priv->dma_cap.tc_cnt;

	if (priv->ets) {
		ets->cbs = priv->ets->cbs;
		memcpy(ets->tc_tx_bw, priv->ets->tc_tx_bw,
		       sizeof(ets->tc_tx_bw));
		memcpy(ets->tc_tsa, priv->ets->tc_tsa, sizeof(ets->tc_tsa));
		memcpy(ets->prio_tc, priv->ets->prio_tc, sizeof(ets->prio_tc));
	}

	netif_dbg(priv, drv, netdev, "(%s) get ets\n", __func__);
	return 0;
}

static int dn200_dcb_ieee_setets(struct net_device *netdev,
				 struct ieee_ets *ets)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	unsigned int i, tc_ets, tc_ets_weight;
	u8 max_tc = 0;
	u8 support = dn200_dcb_getdcbx(netdev);

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	if (!priv->ets) {
		priv->ets = devm_kzalloc(priv->device, sizeof(*priv->ets),
					 GFP_KERNEL);
		if (!priv->ets)
			return -ENOMEM;
	}
	/* if don't change any ets settings, don't go on */
	if (memcmp(priv->ets, ets, sizeof(*priv->ets)) == 0)
		return 0;
	memcpy(priv->ets, ets, sizeof(*priv->ets));

	tc_ets = 0;
	tc_ets_weight = 0;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		max_tc = max_t(u8, max_tc, ets->prio_tc[i]);
		if ((ets->tc_tx_bw[i] || ets->tc_tsa[i]))
			max_tc = max_t(u8, max_tc, i);

		switch (ets->tc_tsa[i]) {
		case IEEE_8021QAZ_TSA_STRICT:
			break;
		case IEEE_8021QAZ_TSA_ETS:
			tc_ets = 1;
			tc_ets_weight += ets->tc_tx_bw[i];
			break;
		default:
			/**
			 * Hardware only supports priority strict or
			 * ETS transmission selection algorithms if
			 * we receive some other value from dcbnl
			 * throw an error
			 */
			netif_warn(priv, drv, netdev,
				   "unsupported TSA algorithm (%hhuu)\n",
				   ets->tc_tsa[i]);
			return -EINVAL;
		}
	}

	/* Check maximum traffic class requested */
	if (max_tc >= priv->dma_cap.tc_cnt) {
		netif_warn(priv, drv, netdev,
			   "exceeded number of supported traffic classes\n");
		return -EINVAL;
	}

	/* Weights must add up to 100% */
	if (tc_ets_weight != 100) {
		netif_warn(priv, drv, netdev,
			   "sum of ETS algorithm weights is not 100 (%u)\n",
			   tc_ets_weight);
		return -EINVAL;
	}

	priv->num_tcs = max_tc + 1;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		netif_dbg(priv, drv, netdev,
			  "TC%u: tx_bw=%#x, rx_bw=%#x, tsa=%#x\n", i,
			  ets->tc_tx_bw[i], ets->tc_rx_bw[i], ets->tc_tsa[i]);
		netif_dbg(priv, drv, netdev, "PRIO%u: TC=%#x\n", i,
			  ets->prio_tc[i]);
	}
	netif_dbg(priv, drv, netdev, "(%s) set ets\n", __func__);
	dn200_config_dcb_tc(priv);

	return 0;
}

static bool dn200_is_pfc_queue(struct dn200_priv *priv, unsigned int queue)
{
	unsigned int prio, tc;

	for (prio = 0; prio < IEEE_8021QAZ_MAX_TCS; prio++) {
		/* Does this queue handle the priority? */
		if (priv->prio2q_map[prio] != queue)
			continue;

		/* Get the Traffic Class for this priority */
		tc = priv->ets->prio_tc[prio];

		/* Check if PFC is enabled for this traffic class */
		if (priv->pfc->pfc_en & (1 << tc))
			return true;
	}

	return false;
}

static unsigned int dn200_get_pfc_queues(struct dn200_priv *priv)
{
	unsigned int count, prio_queues;
	unsigned int i;

	if (!priv->pfc->pfc_en)
		return 0;

	count = 0;
	prio_queues = DN200_PRIO_QUEUES(priv->plat->rx_queues_to_use);
	for (i = 0; i < prio_queues; i++) {
		if (!dn200_is_pfc_queue(priv, i))
			continue;

		priv->pfcq[i] = 1;
		count++;
	}

	return count;
}

static void dn200_config_rx_fifo_size(struct dn200_priv *priv)
{
	unsigned int fifo[MTL_MAX_RX_QUEUES];
	unsigned int pfc_queues;
	unsigned int queue;
	u32 value;

	/* Clear any DCB related fifo/queue information */
	memset(priv->pfcq, 0, sizeof(priv->pfcq));

	pfc_queues = dn200_get_pfc_queues(priv);

	/**
	 * Assign equal Rx FIFO size and set flow control
	 * RFA/RFD in dn200_dma_operation_mode(). If FIFO
	 * size is equal or more than 4K, EHFC(Enable HW
	 * Flow Control) will be set.
	 */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++) {
		value = readl(priv->ioaddr + XGMAC_MTL_RXQ_OPMODE(queue));
		fifo[queue] = (value & XGMAC_RQS) >> XGMAC_RQS_SHIFT;
		netif_dbg(priv, drv, priv->dev, "RxQ%u, %u byte fifo queue\n",
			  queue, ((fifo[queue] + 1) * DN200_FIFO_UNIT));
	}

	/**
	 * There are only total 64K FIFO due to Hardware limit.
	 * In the case of 8 queues enabled, each queue can get
	 * 8KB fifo on average, which means it maybe unnecessary
	 * to set RFA and RFD in the unit of 0.5KB.
	 */
}

static void dn200_config_flow_control(struct dn200_priv *priv)
{
	struct ieee_pfc *pfc = priv->pfc;
	u32 value;

	if (pfc && pfc->pfc_en) {
		/* Set RFE for Rx flow control and TFE/PT for Tx */
		dn200_flow_ctrl(priv, priv->hw, DUPLEX_FULL, FLOW_AUTO,
				PAUSE_TIME, priv->plat->tx_queues_to_use);

		/* Enable PFC instead of Pause Frame */
		value = readl(priv->ioaddr + XGMAC_RX_FLOW_CTRL);
		value |= XGMAC_PFCE;
		writel(value, priv->ioaddr + XGMAC_RX_FLOW_CTRL);
	} else {
		/* Restore to the original pause configuration */
		dn200_flow_ctrl(priv, priv->hw, priv->duplex, priv->flow_ctrl,
				priv->pause, priv->plat->tx_queues_to_use);

		/* Disable PFC mode */
		value = readl(priv->ioaddr + XGMAC_RX_FLOW_CTRL);
		value &= ~XGMAC_PFCE;
		writel(value, priv->ioaddr + XGMAC_RX_FLOW_CTRL);
	}
}

static void dn200_prepare_rx_stop(struct dn200_priv *priv, u32 queue)
{
	u32 rx_status;
	unsigned long rx_timeout;

	/* The Rx engine cannot be stopped if it is actively processing
	 * packets. Wait for the Rx queue to empty the Rx fifo.  Don't
	 * wait forever though...
	 */
	rx_timeout = jiffies + (DN200_DMA_STOP_TIMEOUT * HZ);
	while (time_before(jiffies, rx_timeout)) {
		rx_status = readl(priv->ioaddr + XGMAC_MTL_RXQ_DEBUG(queue));
		if (((rx_status & XGMAC_PRXQ) == 0)
		    && ((rx_status & XGMAC_RXQSTS) == 0))
			break;
		usleep_range(500, 1000);
	}

	if (!time_before(jiffies, rx_timeout))
		netdev_info(priv->dev,
			    "timed out waiting for Rx queue %u to empty\n",
			    queue);
}

static void dn200_enable_rx(struct dn200_priv *priv)
{
	u32 value, queue;
	u8 mode;

	/* Enable each Rx DMA channel */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++)
		dn200_start_rx(priv, priv->ioaddr, queue, priv->hw);

	/* Enable each Rx queue */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++) {
		mode = priv->plat->rx_queues_cfg[queue].mode_to_use;
		dn200_rx_queue_enable(priv, priv->hw, mode, queue);
	}

	if (!PRIV_IS_VF(priv)) {
		/* broadcast & mutlicast put to last mtl queue and copy to all dma channels */
		queue = DN200_LAST_QUEUE(priv);
		dn200_rx_queue_enable(priv, priv->hw, MTL_QUEUE_DCB, queue);
	}

	/* Enable MAC Rx */
	value = readl(priv->ioaddr + XGMAC_RX_CONFIG);
	value |= XGMAC_CONFIG_RE;
	writel(value, priv->ioaddr + XGMAC_RX_CONFIG);
}

static void dn200_disable_rx(struct dn200_priv *priv)
{
	u32 value, queue;

	/* Disable MAC Rx */
	value = readl(priv->ioaddr + XGMAC_RX_CONFIG);
	value &= ~XGMAC_CONFIG_RE;
	writel(value, priv->ioaddr + XGMAC_RX_CONFIG);

	/* Prepare for Rx DMA channel stop */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++)
		dn200_prepare_rx_stop(priv, queue);

	/* Disable each Rx queue */
	writel(0, priv->ioaddr + XGMAC_RXQ_CTRL0);

	/* Disable each Rx DMA channel */
	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++)
		dn200_stop_rx(priv, priv->ioaddr, queue, priv->hw);
}

static void dn200_config_dcb_pfc(struct dn200_priv *priv)
{
	if (!test_bit(DN200_DCB_DOWN, &priv->state)) {
		/* Just stop the Tx queues while Rx fifo is changed */
		netif_tx_stop_all_queues(priv->dev);

		/* Suspend Rx so that fifo's can be adjusted */
		dn200_disable_rx(priv);
	}

	dn200_config_rx_fifo_size(priv);
	dn200_config_flow_control(priv);

	if (!test_bit(DN200_DCB_DOWN, &priv->state)) {
		/* Resume Rx */
		dn200_enable_rx(priv);

		/* Resume Tx queues */
		netif_tx_start_all_queues(priv->dev);
	}
}

static int dn200_dcb_ieee_getpfc(struct net_device *netdev,
				 struct ieee_pfc *pfc)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	/* Set number of supported PFC traffic classes */
	pfc->pfc_cap = priv->dma_cap.tc_cnt;

	if (priv->pfc) {
		pfc->pfc_en = priv->pfc->pfc_en;
		pfc->mbc = priv->pfc->mbc;
		pfc->delay = priv->pfc->delay;
	}

	netif_dbg(priv, drv, netdev, "(%s) get pfc\n", __func__);
	return 0;
}

static int dn200_dcb_ieee_setpfc(struct net_device *netdev,
				 struct ieee_pfc *pfc)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	if (!priv->pfc) {
		priv->pfc = devm_kzalloc(priv->device, sizeof(*priv->pfc),
					 GFP_KERNEL);
		if (!priv->pfc)
			return -ENOMEM;
	}
	/* if don't change any pfc settings, don't go on */
	if (memcmp(priv->pfc, pfc, sizeof(*priv->pfc)) == 0)
		return 0;
	memcpy(priv->pfc, pfc, sizeof(*priv->pfc));

	/* Check PFC for supported number of traffic classes */
	if (pfc->pfc_en & ~((1 << priv->dma_cap.tc_cnt) - 1)) {
		netif_warn(priv, drv, netdev,
			   "PFC requested for unsupported traffic class\n");
		return -EINVAL;
	}

	netif_dbg(priv, drv, netdev,
		  "cap=%hhuu, en=%#hhxx, mbc=%hhuu, delay=%hhuu\n", pfc->pfc_cap,
		  pfc->pfc_en, pfc->mbc, pfc->delay);
	dn200_config_dcb_pfc(priv);

	netif_dbg(priv, drv, netdev, "(%s) set pfc\n", __func__);
	return 0;
}

static int dn200_dcb_ieee_setapp(struct net_device *netdev, struct dcb_app *app)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);
	struct dcb_ieee_app_dscp_map priority_map;
	struct dcb_app temp;
	bool is_new, need_add;
	int err;
	u8 up, dscp;

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	if (app->selector != IEEE_8021QAZ_APP_SEL_DSCP) {
		netif_dbg(priv, drv, netdev, "unsupported selector %d\n",
			  app->selector);
		return -EOPNOTSUPP;
	}

	if (app->protocol >= DN200_TRUST_DSCP ||
	    app->priority >= DN200_TRUST_UP) {
		netif_warn(priv, drv, netdev,
			   "invalid parameters protocol %d priority%d\n",
			   app->protocol, app->priority);
		return -EINVAL;
	}

	dcb_ieee_getapp_dscp_prio_mask_map(netdev, &priority_map);
	if (priority_map.map[app->protocol]) {
		is_new = false;
		/**
		 * Skip the APP command if new and old mapping are the same,
		 * and replace the old APP entry if new mapping is different.
		 */
		if (priority_map.map[app->protocol] & (1 << app->priority)) {
			/* New and old mapping are the same, so do nothing */
			need_add = false;
			netif_dbg(priv, drv, netdev,
				  "skip the dscp app command\n");
		} else {
			/* Delete the old APP entry if exists */
			temp.selector = IEEE_8021QAZ_APP_SEL_DSCP;
			temp.priority =
			    ffs(priority_map.map[app->protocol]) - 1;
			temp.protocol = app->protocol;
			err = dcb_ieee_delapp(netdev, &temp);
			if (err)
				return err;

			need_add = true;
			netif_dbg(priv, drv, netdev,
				  "replace the old dscp app entry\n");
		}
	} else {
		/* No dscp mapping entry exists */
		is_new = true;
		need_add = true;
		netif_dbg(priv, drv, netdev, "add the new dscp app entry\n");
	}

	if (need_add) {
		err = dcb_ieee_setapp(netdev, app);
		if (err) {
			netif_warn(priv, drv, netdev,
				   "fail to add the dscp app entry\n");
			return err;
		}
	}

	/* Update mapping in private data, default up 0 without dscp entry */
	dcb_ieee_getapp_dscp_prio_mask_map(netdev, &priority_map);
	for (dscp = 0; dscp < DN200_TRUST_DSCP; dscp++) {
		up = ffs(priority_map.map[dscp]);
		if (up)
			priv->dscp2up[dscp] = up - 1;
		else
			priv->dscp2up[dscp] = 0;
	}

	if (is_new) {
		priv->dscp_app_cnt++;
		netif_dbg(priv, drv, netdev, "dev %d dscp app entry count %d\n",
			  priv->dev->ifindex, priv->dscp_app_cnt);
	}

	return 0;
}

static int dn200_dcb_ieee_delapp(struct net_device *netdev, struct dcb_app *app)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 support = dn200_dcb_getdcbx(netdev);
	int err;

	if (PRIV_IS_VF(priv) || !(support & DCB_CAP_DCBX_VER_IEEE))
		return -EOPNOTSUPP;

	if (app->selector != IEEE_8021QAZ_APP_SEL_DSCP) {
		netif_warn(priv, drv, netdev, "unsupported selector %d\n",
			   app->selector);
		return -EOPNOTSUPP;
	}

	if (app->protocol >= DN200_TRUST_DSCP ||
	    app->priority >= DN200_TRUST_UP) {
		netif_warn(priv, drv, netdev,
			   "invalid parameters protocol %d priority%d\n",
			   app->protocol, app->priority);
		return -EINVAL;
	}

	/* Skip if no dscp app entry */
	if (!priv->dscp_app_cnt)
		return -ENOENT;

	/* Delete the dscp app entry */
	err = dcb_ieee_delapp(netdev, app);
	if (err) {
		netif_warn(priv, drv, netdev,
			   "fail to delete the dscp app entry\n");
		return err;
	}

	if (priv->dscp_app_cnt)
		priv->dscp_app_cnt--;
	netif_dbg(priv, drv, netdev, "dev %d dscp app entry count %d\n",
		  priv->dev->ifindex, priv->dscp_app_cnt);

	return 0;
}
static u8 dn200_dcb_getstate(struct net_device *netdev)
{
	/* DCB mode is on */
	return 1;
}

static void dn200_dcb_getpermhwaddr(struct net_device *netdev, u8 *perm_addr)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	unsigned int hi_addr, lo_addr;

	if (!perm_addr)
		return;

	memset(perm_addr, 0xff, MAX_ADDR_LEN);

	hi_addr = readl(priv->ioaddr + XGMAC_ADDRX_HIGH(0));
	lo_addr = readl(priv->ioaddr + XGMAC_ADDRX_LOW(0));

	/* Extract the MAC address from the high and low words */
	perm_addr[0] = lo_addr & 0xff;
	perm_addr[1] = (lo_addr >> 8) & 0xff;
	perm_addr[2] = (lo_addr >> 16) & 0xff;
	perm_addr[3] = (lo_addr >> 24) & 0xff;
	perm_addr[4] = hi_addr & 0xff;
	perm_addr[5] = (hi_addr >> 8) & 0xff;
}

static void dn200_dcb_getpgtccfgtx(struct net_device *netdev,
				   int priority, u8 *prio_type,
				   u8 *pgid, u8 *bw_pct, u8 *up_map)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	if (!priv->ets) {
		netdev_err(netdev, "%s, ets is not supported\n", __func__);
		return;
	}

	if (priority >= 8) {
		netdev_err(netdev, "%s, priority is out of range\n", __func__);
		return;
	}

	*prio_type = 0;
	*bw_pct = 0;
	*up_map = 0;
	*pgid = priv->prio2tc_bitmap[priority];
}

static void dn200_dcb_getpgbwgcfgtx(struct net_device *netdev,
				    int pgid, u8 *bw_pct)
{
	struct ieee_ets ets;

	if (pgid >= 8) {
		netdev_err(netdev, "%s, priority group(TC) is out of range\n",
			   __func__);
		return;
	}

	dn200_dcb_ieee_getets(netdev, &ets);
	*bw_pct = ets.tc_tx_bw[pgid];
}

static int dn200_dcb_get_priority_pfc(struct net_device *netdev,
				      int priority, u8 *setting)
{
	struct ieee_pfc pfc;
	int err;

	err = dn200_dcb_ieee_getpfc(netdev, &pfc);

	if (err)
		*setting = 0;
	else
		*setting = (pfc.pfc_en >> priority) & 0x01;

	return err;
}

static void dn200_dcb_getpfccfg(struct net_device *netdev,
				int priority, u8 *setting)
{
	if (priority >= 8) {
		netdev_err(netdev, "%s, priority is out of range\n", __func__);
		return;
	}

	if (!setting)
		return;

	dn200_dcb_get_priority_pfc(netdev, priority, setting);
}

static u8 dn200_dcb_getcap(struct net_device *netdev, int capid, u8 *cap)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u8 rval = 0;

	switch (capid) {
	case DCB_CAP_ATTR_PG:
		*cap = true;
		break;
	case DCB_CAP_ATTR_PFC:
		*cap = true;
		break;
	case DCB_CAP_ATTR_UP2TC:
		*cap = false;
		break;
	case DCB_CAP_ATTR_PG_TCS:
		*cap = 1 << (priv->dma_cap.tc_cnt - 1);
		break;
	case DCB_CAP_ATTR_PFC_TCS:
		*cap = 1 << (priv->dma_cap.tc_cnt - 1);
		break;
	case DCB_CAP_ATTR_GSP:
		*cap = false;
		break;
	case DCB_CAP_ATTR_BCN:
		*cap = false;
		break;
	case DCB_CAP_ATTR_DCBX:
		*cap = DCB_CAP_DCBX_HOST | DCB_CAP_DCBX_VER_IEEE;
		break;
	default:
		*cap = 0;
		rval = 1;
		break;
	}

	return rval;
}

static int dn200_dcb_getnumtcs(struct net_device *netdev, int tcs_id, u8 *num)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	switch (tcs_id) {
	case DCB_NUMTCS_ATTR_PG:
	case DCB_NUMTCS_ATTR_PFC:
		*num = priv->dma_cap.tc_cnt;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static u8 dn200_dcb_getpfcstate(struct net_device *netdev)
{
	struct ieee_pfc pfc;

	if (dn200_dcb_ieee_getpfc(netdev, &pfc))
		return 0;

	return pfc.pfc_en ? 1 : 0;
}

static const struct dcbnl_rtnl_ops dn200_dcbnl_ops = {
	/* IEEE 802.1Qaz std */
	.ieee_getets = dn200_dcb_ieee_getets,
	.ieee_setets = dn200_dcb_ieee_setets,
	.ieee_getpfc = dn200_dcb_ieee_getpfc,
	.ieee_setpfc = dn200_dcb_ieee_setpfc,
	.ieee_setapp = dn200_dcb_ieee_setapp,
	.ieee_delapp = dn200_dcb_ieee_delapp,

	/* DCBX configuration */
	.getdcbx = dn200_dcb_getdcbx,
	.setdcbx = dn200_dcb_setdcbx,

	/* CEE Interfaces only for get_message */
	.getstate = dn200_dcb_getstate,
	.getpermhwaddr = dn200_dcb_getpermhwaddr,
	.getpgtccfgtx = dn200_dcb_getpgtccfgtx,
	.getpgbwgcfgtx = dn200_dcb_getpgbwgcfgtx,
	.getpfccfg = dn200_dcb_getpfccfg,
	.getcap = dn200_dcb_getcap,
	.getnumtcs = dn200_dcb_getnumtcs,
	.getpfcstate = dn200_dcb_getpfcstate,
};

const struct dcbnl_rtnl_ops *dn200_get_dcbnl_ops(void)
{
	return &dn200_dcbnl_ops;
}

void dn200_dcbnl_init(struct dn200_priv *priv, bool init)
{
	struct dcb_app temp;
	struct dcb_ieee_app_dscp_map priority_map;
	int i, j, err, up = 0;

	priv->dscp_app_cnt = 0;

	/* Init when dev open */
	if (init) {
		netdev_dbg(priv->dev,
				    "Search all existing app dscp entries of dev %d\n",
				    priv->dev->ifindex);
		/* Find the priority mapping to the DSCP */
		dcb_ieee_getapp_dscp_prio_mask_map(priv->dev, &priority_map);
		for (i = 0; i < DN200_TRUST_DSCP; i++) {
			/* Find the UP (ffs - 1) mapping to the DSCP */
			up = ffs(priority_map.map[i]);
			if (up) {
				netdev_dbg(priv->dev,
						    "Find the app dscp entry selector 5 protocol %d priority %d\n",
						    i, up - 1);
				priv->dscp_app_cnt++;
			}
		}
		netdev_dbg(priv->dev, "(%s) dev %d dscp app entry count %d\n",
			   __func__, priv->dev->ifindex, priv->dscp_app_cnt);

		/* Map TC to queue statically */
		dn200_config_queue_mapping(priv);
	} else {
		/* Delete when dn200_dvr_remove() */
		netdev_dbg(priv->dev,
				"Clear all existing app dscp entries of dev %d\n",
				priv->dev->ifindex);
		/* Clear all existing app dscp entry */
		temp.selector = IEEE_8021QAZ_APP_SEL_DSCP;
		for (i = 0; i < DN200_TRUST_DSCP; i++) {
			priv->dscp2up[i] = 0;
			temp.protocol = i;
			for (j = 0; j < IEEE_8021QAZ_MAX_TCS; j++) {
				temp.priority = j;
				err = dcb_ieee_delapp(priv->dev, &temp);
				if (!err) {
					netdev_dbg(priv->dev,
							"Delete the app dscp entry selector %d protocol %d priority %d\n",
							temp.selector,
							temp.protocol,
							temp.priority);
				}
			}
		}
	}
}
