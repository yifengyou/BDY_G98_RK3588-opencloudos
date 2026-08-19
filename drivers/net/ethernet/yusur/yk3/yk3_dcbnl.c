// SPDX-License-Identifier: GPL-2.0

#include <linux/device.h>
#include <linux/netdevice.h>
#include <net/dcbnl.h>
#include "yk3.h"

int yk3_get_pfc_mac_l1xon(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13a3108 + 64 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_get_pfc_mac_l1xoff(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13a310c + 64 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_set_pfc_mac_l1xon(struct net_device *ndev, u32 val)
{
	int i;
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 560 || val < 4)
				return -EINVAL;
		} else {
			if (val > 560 || val < 7)
				return -EINVAL;
		}
	}

	for (i = 0; i < 8; i++)
		yk3_wr32(hw_addr, 0x13a3108 + 64 * pdev_priv->mac->mac_ch + 256 * i, val);

	return 0;
}

int yk3_set_pfc_mac_l1xoff(struct net_device *ndev, u32 val)
{
	int i;
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 768 || val < 6)
				return -EINVAL;
		} else {
			if (val > 768 || val < 10)
				return -EINVAL;
		}
	}

	for (i = 0; i < 8; i++)
		yk3_wr32(hw_addr, 0x13a310c + 64 * pdev_priv->mac->mac_ch + 256 * i, val);

	return 0;
}

int yk3_get_pfc_mac_l2xon(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13e3108 + 0x100 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_get_pfc_mac_l2xoff(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13e310c + 0x100 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_set_pfc_mac_l2xon(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 768 || val < 14)
				return -EINVAL;
		} else {
			if (val > 768 || val < 28)
				return -EINVAL;
		}
	}

	yk3_wr32(hw_addr, 0x13e3108 + 0x100 * pdev_priv->mac->mac_ch, val);

	return 0;
}

int yk3_set_pfc_mac_l2xoff(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 1024 || val < 20)
				return -EINVAL;
		} else {
			if (val > 1024 || val < 40)
				return -EINVAL;
		}
	}

	yk3_wr32(hw_addr, 0x13e310c + 0x100 * pdev_priv->mac->mac_ch, val);

	return 0;
}

int yk3_get_pfc_pcie_l1xon(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13c3108 + 64 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_get_pfc_pcie_l1xoff(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13c310c + 64 * pdev_priv->mac->mac_ch);

	return 0;
}

int yk3_set_pfc_pcie_l1xon(struct net_device *ndev, u32 val)
{
	int i;
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 560 || val < 4)
				return -EINVAL;
		} else {
			if (val > 560 || val < 7)
				return -EINVAL;
		}
	}

	for (i = 0; i < 8; i++)
		yk3_wr32(hw_addr, 0x13c3108 + 64 * pdev_priv->mac->mac_ch + 256 * i, val);

	return 0;
}

int yk3_set_pfc_pcie_l1xoff(struct net_device *ndev, u32 val)
{
	int i;
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 768 || val < 6)
				return -EINVAL;
		} else {
			if (val > 768 || val < 10)
				return -EINVAL;
		}
	}

	for (i = 0; i < 8; i++)
		yk3_wr32(hw_addr, 0x13c310c + 64 * pdev_priv->mac->mac_ch + 256 * i, val);

	return 0;
}

int yk3_get_pfc_pcie_l2xon(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13e3008);

	return 0;
}

int yk3_get_pfc_pcie_l2xoff(struct net_device *ndev, u32 *val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	*val = yk3_rd32(hw_addr, 0x13e300c);

	return 0;
}

int yk3_set_pfc_pcie_l2xon(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 768 || val < 14)
				return -EINVAL;
		} else {
			if (val > 768 || val < 28)
				return -EINVAL;
		}
	}

	yk3_wr32(hw_addr, 0x13e3008, val);

	return 0;
}

int yk3_set_pfc_pcie_l2xoff(struct net_device *ndev, u32 val)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return -ENODEV;

	if (yk3_pdev_is_vf(pdev_priv))
		return -EOPNOTSUPP;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (pdev_priv->card->mode == YK3_MODE_TCARD) {
		if (ndev_priv->link_speed <= SPEED_10000) {
			if (val > 1024 || val < 20)
				return -EINVAL;
		} else {
			if (val > 1024 || val < 40)
				return -EINVAL;
		}
	}

	yk3_wr32(hw_addr, 0x13e300c, val);

	return 0;
}

int yk3_set_pfc_pcie_l1vq(struct net_device *ndev, u32 xoff, u32 xon)
{
	int ret;

	ret = yk3_set_pfc_pcie_l1xoff(ndev, xoff);
	ret |= yk3_set_pfc_pcie_l1xon(ndev, xon);

	return ret;
}

static int yk3_np_set_pfc_threshold(struct net_device *ndev)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret;
	u32 port_speed = ndev_priv->link_speed;
	u8 port_num = pdev_priv->card->pf_num;
	u32 base;
	u32 mac_l1_xoff;
	u32 mac_l1_xon;
	u32 mac_l2_xoff;
	u32 mac_l2_xon;
	u32 size;
	int i;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (port_speed == 25000 && port_num == 2)
		size = 96;
	else if (port_speed == 25000 && port_num == 4)
		size = 192;
	else
		size = 128;

	base = port_speed / (size * 8);

	mac_l1_xoff = 36 * base / 10;
	mac_l2_xoff = 1536 / port_num - 320 - 3 * base;

	mac_l1_xon = mac_l1_xoff * 7 / 10;
	mac_l2_xon = mac_l2_xoff * 7 / 10;

	ret = yk3_set_pfc_mac_l1xon(ndev, mac_l1_xon);
	ret |= yk3_set_pfc_mac_l1xoff(ndev, mac_l1_xoff);
	ret |= yk3_set_pfc_mac_l2xon(ndev, mac_l2_xon);
	ret |= yk3_set_pfc_mac_l2xoff(ndev, mac_l2_xoff);

	for (i = 0; i < 32; i++)
		yk3_wr32(hw_addr, 0x14b0200 + 4 * i, mac_l1_xoff + 0x90);  //mac l1 ovf

	for (i = 0; i < 4; i++)
		yk3_wr32(hw_addr, 0x14b0600 + 4 * i, mac_l2_xoff + 0x90);  //mac l2 ovf

	return ret;
}

/**
 * yk3_np_set_pfc - set PFC config for np register
 * @ndev: pointer to relevant netdev
 * @pfc_en: the pfc enabled priority
 * @mac_ch: the mac channel is set
 */
static int yk3_np_set_pfc(struct net_device *ndev, u8 pfc_en, u8 mac_ch)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	bool enable;
	int i;
	int ret;

	if (!pdev_priv)
		return -EINVAL;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (pfc_en) {
		ret = yk3_np_set_pfc_threshold(ndev);
		if (ret) {
			yk3_net_err("Set np pfc threshold failed.\n");
			return ret;
		}
	}

	for (i = 0; i < 8; i++) {
		enable = (pfc_en & BIT(i)) ? true : false;
		yk3_wr32(hw_addr, 0x13a3100 + 64 * mac_ch + 256 * i, enable);  // MAC侧PFC开关
	}

	enable = pfc_en ? true : false;

	/* MAC 配置 */
	yk3_wr32(hw_addr, 0x13e3100 + 0x100 * mac_ch, enable); //l2 enable

	/* PCIE 配置 */
	yk3_wr32(hw_addr, 0x13e3000, enable); //l2 enable

	/* edma pfc buffer and pkt threshold */
	if (enable) {
		for (i = 0; i < 8; i++) {
			yk3_wr32(hw_addr, 0x14b1000 + 4 * (mac_ch + i), 0x28);    // MAC L1 reserve
			yk3_wr32(hw_addr, 0x14b1100 + 4 * (mac_ch + i), 0x28);    // PCIE L1 reserve
		}
		yk3_wr32(hw_addr, 0x14b040c + 0x10 * mac_ch, (u32)(0xff << (8 * mac_ch))); //l2 map
		/* EDMA */
		for (i = 0; i < 32; i++) {
			yk3_wr32(hw_addr, 0x03a4 + 4 * i, 0x200);
			yk3_wr32(hw_addr, 0x0424 + 4 * i, 0x100);
		}
	} else {
		for (i = 0; i < 8; i++) {
			yk3_wr32(hw_addr, 0x14b1000 + 4 * (mac_ch + i), 0);     // MAC L1 reserve
			yk3_wr32(hw_addr, 0x14b1100 + 4 * (mac_ch + i), 0);     // PCIE L1 reserve
		}
		yk3_wr32(hw_addr, 0x14b040c + 0x10 * mac_ch, 0x0); //l2 map

		for (i = 0; i < 32; i++)
			yk3_wr32(hw_addr, 0x14b0200 + 4 * i, 0);   //mac l1 ovf

		for (i = 0; i < 4; i++)
			yk3_wr32(hw_addr, 0x14b0600 + 4 * i, 0);  //mac l2 ovf

		/* EDMA */
		for (i = 0; i < 32; i++) {
			yk3_wr32(hw_addr, 0x03a4 + 4 * i, 0x1800);
			yk3_wr32(hw_addr, 0x0424 + 4 * i, 0xd00);
		}
	}

	return 0;
}

/**
 * yk3_set_dcbx_mode - set DCBX mode to HOST or FW
 * @ndev: pointer to the netdev struct
 * @mode: dcbx mode
 */
static int yk3_set_dcbx_mode(struct net_device *ndev,
			     enum yk3_dcbx_mode mode)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret = 0;

	if (yk3_pdev_is_mgr(pdev_priv))
		return 0;

	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_LLDP_MODE;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data[0] = mode;
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		if (recv_msg.data[0]) {
			yk3_net_err("send mbox data check errno %02x result is %02x\n",
				    recv_msg.data[0], recv_msg.data[1]);
		}
		yk3_net_err("send mbox message errno %d failed!\n", ret);
		return ret;
	}

	return ret;
}

static int yk3_ndev_setup_tc_map(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u16 offset = 0, tx_count = 0;
	u16 num_txq_per_tc;
	u16 qcount_tx = ndev_priv->txq_real_num;
	u8 numtc = 8;
	int i;

	num_txq_per_tc = qcount_tx / numtc;
	if (!num_txq_per_tc)
		num_txq_per_tc = 1;

	for (i = 0; i < YK3_MAX_TRAFFIC_CLASS; i++) {
		/* TC is enabled */
		ndev_priv->tc_cfg.tc_info[i].qoffset = offset;
		ndev_priv->tc_cfg.tc_info[i].qcount_tx = num_txq_per_tc;

		offset += num_txq_per_tc;
		tx_count += num_txq_per_tc;
	}

	if (tx_count > ndev_priv->txq_real_num) {
		yk3_net_err("Trying to use more Tx queues (%u), than were allocated (%u)!\n",
			    tx_count, ndev_priv->txq_real_num);
		return -EINVAL;
	}

	return 0;
}

/**
 * yk3_dcbnl_getpfc - retrieve local IEEE PFC config
 * @ndev: pointer to netdev struct
 * @pfc: struct to hold PFC info
 */
static int yk3_dcbnl_getpfc(struct net_device *ndev, struct ieee_pfc *pfc)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	void __iomem *hw_addr;
	u8 mac_ch;
	int i;

	if (!pdev_priv)
		return -EINVAL;

	pfc->pfc_en = ndev_priv->dcbx->pfc.pfcena;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	mac_ch = pdev_priv->mac->mac_ch;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		pfc->requests[i]    = yk3_rd64(hw_addr, 0xa0C0B8 + 0x400 * mac_ch + 8 * i);  //tx
		pfc->indications[i] = yk3_rd64(hw_addr, 0xa0C258 + 0x400 * mac_ch + 8 * i);  //rx
	}

	yk3_net_debug("DCBX get pfc is %d.\n", pfc->pfc_en);

	return 0;
}

/**
 * yk3_dcbnl_setpfc - set local IEEE PFC config
 * @ndev: pointer to relevant netdev
 * @pfc: pointer to struct holding PFC config
 */
static int yk3_dcbnl_setpfc(struct net_device *ndev, struct ieee_pfc *pfc)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u32 value = 0, value_bit8 = 0, value_bit10 = 0;
	void __iomem *hw_addr;
	int ret = 0;

	if (!pdev_priv || !ndev_priv->dcbx)
		return -EINVAL;

	if (!pdev_priv->mac) {
		yk3_net_warn("%s pdev_priv mac null\n", __func__);
		return -ENODEV;
	}

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	if ((ndev_priv->dcbx->dcbx_cap & DCB_CAP_DCBX_LLD_MANAGED) ||
	    !(ndev_priv->dcbx->dcbx_cap & DCB_CAP_DCBX_VER_IEEE))
		return -EINVAL;

	value = yk3_rd32(hw_addr, 0xa00008 + 0x400 * pdev_priv->mac->mac_ch);
	value_bit8 = (value >> 8) & 0x1;
	value_bit10 = (value >> 10) & 0x1;
	if (value_bit8 || value_bit10) {
		yk3_net_warn("fc is enable, do not set dcbx pfc\n");
		return -EOPNOTSUPP;
	}

	if (pfc->pfc_en != ndev_priv->dcbx->pfc.pfcena) {
		ret = yk3_mac_set_pfc(ndev_priv, pdev_priv->mac->mac_ch, pfc->pfc_en);
		if (ret) {
			yk3_net_err("Set umac pfc failed, error is %d.\n", ret);
			return ret;
		}
		ret = yk3_np_set_pfc(ndev, pfc->pfc_en, pdev_priv->mac->mac_ch);
		if (ret) {
			yk3_net_err("Set np pfc failed, error is %d.\n", ret);
			ret = yk3_mac_set_pfc(ndev_priv, pdev_priv->mac->mac_ch, false);
			return ret;
		}

		ndev_priv->dcbx->pfc.pfcena = pfc->pfc_en;
		if (!ndev_priv->tc_cfg.ena_tc && pfc->pfc_en)
			yk3_ndev_setup_tc_map(ndev);
	}

	return 0;
}

int yk3_rdma_setpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc)
{
	void __iomem *hw_addr;
	struct yk3_ndev_priv *ndev_priv;
	int ret;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	if (!hw_addr)
		return -ENODEV;

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv) {
		yk3_dev_err("%s pf %u no ndev\n", __func__, pdev_priv->pf_id);
		return -ENODEV;
	}

	ret = yk3_dcbnl_setpfc(ndev_priv->ndev, pfc);
	if (ret) {
		yk3_dev_err("%s setpfc failed: %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int yk3_rdma_getpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc)
{
	struct yk3_ndev_priv *ndev_priv;

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv) {
		yk3_dev_err("%s pf %u no ndev\n", __func__, pdev_priv->pf_id);
		return -1;
	}

	return yk3_dcbnl_getpfc(ndev_priv->ndev, pfc);
}

#ifdef YK3_HAVE_APP_SEL_DSCP
static int yk3_dcbnl_setapp(struct net_device *ndev, struct dcb_app *app)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct dcb_app temp;
	u8 prio_mode;
	u8 tc;
	int ret;

	if (app->selector != IEEE_8021QAZ_APP_SEL_DSCP ||
	    app->protocol >= YK3_MAX_DSCP)
		return -EINVAL;

	if (!ndev_priv->dcbx)
		return -EINVAL;

	/* Save the old entry info */
	temp.selector = IEEE_8021QAZ_APP_SEL_DSCP;
	temp.protocol = app->protocol;
	temp.priority = ndev_priv->dcbx->dscp2prio[app->protocol];

	/* Check if need to switch to dscp trust state */
	ret = yk3_ppp_primap_mode_get(pdev_priv);
	if (ret < 0) {
		yk3_dev_err("Failed to get prio mode, using default dscp mode.\n");
		prio_mode = YK3_PRIO_DSCP;
	} else {
		yk3_dev_debug("get prio mode is %d.\n", ret);
		prio_mode = ret;
	}

	if (prio_mode != YK3_PRIO_DSCP) {
		yk3_dev_warn("Priority is VLAN, failed to set DSCP priority map.\n");
		return -EOPNOTSUPP;
	}

	ret = yk3_ppp_primap_dscp_map_get(pdev_priv, ndev_priv->dcbx->dscp2tc);
	if (ret)
		return ret;

	tc = netdev_get_prio_tc_map(ndev, app->priority);

	/* Skip the fw command if new and old mapping are the same */
	if (tc != ndev_priv->dcbx->dscp2tc[app->protocol]) {
		ndev_priv->dcbx->dscp2tc[app->protocol] = tc;
		ret = yk3_ppp_primap_dscp_map_set(pdev_priv, ndev_priv->dcbx->dscp2tc);
		if (ret)
			return ret;
	}

	ndev_priv->dcbx->dscp2prio[app->protocol] = app->priority;

	/* Delete the old entry if exists */
	dcb_ieee_delapp(ndev, &temp);

	/* Add new entry and update counter */
	ret = dcb_ieee_setapp(ndev, app);
	if (ret)
		return ret;

	return 0;
}

static int yk3_dcbnl_delapp(struct net_device *ndev, struct dcb_app *app)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u8 tc;
	int ret;

	if (app->selector != IEEE_8021QAZ_APP_SEL_DSCP ||
	    app->protocol >= YK3_MAX_DSCP)
		return -EINVAL;

	if (!ndev_priv->dcbx)
		return -EINVAL;

	ret = yk3_ppp_primap_dscp_map_get(pdev_priv, ndev_priv->dcbx->dscp2tc);
	if (ret)
		return ret;

	tc = netdev_get_prio_tc_map(ndev, app->priority);

	/* Check if the entry matches fw setting */
	if (tc != ndev_priv->dcbx->dscp2tc[app->protocol])
		return -ENOENT;

	/* Delete the app entry */
	ret = dcb_ieee_delapp(ndev, app);
	if (ret)
		return ret;

	/* Reset the priority mapping to default based on DSCP value */
	ndev_priv->dcbx->dscp2tc[app->protocol] = app->protocol >> 3;
	ret = yk3_ppp_primap_dscp_map_set(pdev_priv, ndev_priv->dcbx->dscp2tc);
	if (ret)
		return ret;

	ndev_priv->dcbx->dscp2prio[app->protocol] = app->protocol >> 3;

	return 0;
}
#endif

/**
 * yk3_set_prio_map_tc - remap priority to traffic class for ppp
 * @ndev_priv: pointer to the net device private data
 * @priority: the priority value to be remapped
 * @tc: the traffic class to map to
 */
int yk3_set_prio_map_tc(struct yk3_ndev_priv *ndev_priv, u8 priority, u8 tc)
{
	u8 i;
	int ret;

	if (!ndev_priv->dcbx)
		return -EINVAL;

	ret = yk3_ppp_primap_dscp_map_get(ndev_priv->pdev_priv, ndev_priv->dcbx->dscp2tc);
	if (ret)
		return ret;

	for (i = 0; i < YK3_MAX_DSCP; i++) {
		if (ndev_priv->dcbx->dscp2prio[i] == priority)
			ndev_priv->dcbx->dscp2tc[i] = tc;
	}

	ret = yk3_ppp_primap_dscp_map_set(ndev_priv->pdev_priv, ndev_priv->dcbx->dscp2tc);
	if (ret)
		return ret;

	return 0;
}

/**
 * yk3_dcbnl_getdcbx - retrieve current DCBX capability
 * @ndev: pointer to the netdev struct
 */
static u8 yk3_dcbnl_getdcbx(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (!ndev_priv || !ndev_priv->dcbx)
		return 1;

	yk3_net_debug("DCBx cap = 0x%x\n", ndev_priv->dcbx->dcbx_cap);
	return ndev_priv->dcbx->dcbx_cap;
}

/**
 * yk3_dcbnl_setdcbx - set required DCBX capability
 * @ndev: the corresponding netdev
 * @mode: required mode
 */
static u8 yk3_dcbnl_setdcbx(struct net_device *ndev, u8 mode)
{
	int ret;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	if (!ndev_priv || !ndev_priv->dcbx)
		return 1;

	/* No support for LLD_MANAGED modes or CEE */
	if ((mode & DCB_CAP_DCBX_LLD_MANAGED) ||
	    (mode & DCB_CAP_DCBX_VER_CEE))
		return 1;

	/* Already set to the given mode no change */
	if (mode == ndev_priv->dcbx->dcbx_cap)
		return 0;

	if (!mode) {
		ret = yk3_set_dcbx_mode(ndev, YK3_DCBX_FW);
		if (ret) {
			yk3_net_err("Set dcbx mode FW is failed, error is %d.\n", ret);
			return 1;
		}
		ndev_priv->dcbx->mode = YK3_DCBX_FW;
		ndev_priv->dcbx->dcbx_cap &= ~DCB_CAP_DCBX_HOST;
		return 0;
	}

	if (!(mode & DCB_CAP_DCBX_HOST))
		return 1;

	ret = yk3_set_dcbx_mode(ndev, YK3_DCBX_HOST);
	if (ret) {
		yk3_net_err("Set dcbx mode HOST is failed, error is %d.\n", ret);
		return 1;
	}
	ndev_priv->dcbx->mode = YK3_DCBX_HOST;
	ndev_priv->dcbx->dcbx_cap = mode;

	yk3_net_debug("DCBx mode = 0x%x\n", mode);
	return 0;
}

static const struct dcbnl_rtnl_ops ys_dcbnl_ops = {
	/* IEEE 802.1Qaz std */
	.ieee_getets = yk3_dcbnl_getets,
	.ieee_setets = yk3_dcbnl_setets,
	.ieee_getmaxrate = yk3_dcbnl_getmaxrate,
	.ieee_setmaxrate = yk3_dcbnl_setmaxrate,
	.ieee_getpfc = yk3_dcbnl_getpfc,
	.ieee_setpfc = yk3_dcbnl_setpfc,
#ifdef YK3_HAVE_APP_SEL_DSCP
	.ieee_setapp = yk3_dcbnl_setapp,
	.ieee_delapp = yk3_dcbnl_delapp,
#endif
	.getdcbx     = yk3_dcbnl_getdcbx,
	.setdcbx     = yk3_dcbnl_setdcbx,
};

/**
 * yk3_set_pause_srcaddr - query the dcbx mode
 * @pdev_priv: the corresponding pdev privite data
 * @addr: pointer to mac address
 */
int yk3_set_pause_srcaddr(struct yk3_pdev_priv *pdev_priv, const u8 *addr)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret;

	if (!addr || !pdev_priv || !pdev_priv->card)
		return -EINVAL;

	if (yk3_pdev_is_vf(pdev_priv) || yk3_pdev_is_mgr(pdev_priv))
		return 0;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return 0;

	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_SET_PAUSE_SRCMAC;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_msg.data_length = 8;
	mbox_msg.data[0] = pdev_priv->card->mac_chs[pdev_priv->pf_id];
	memcpy(mbox_msg.data + 2, addr, ETH_ALEN);
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	yk3_dev_debug("MAC address %02x:%02x:%02x:%02x:%02x:%02x",
		      mbox_msg.data[2], mbox_msg.data[3], mbox_msg.data[4], mbox_msg.data[5],
		      mbox_msg.data[6], mbox_msg.data[7]);

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		yk3_dev_err("Send mbox message errno is %d failed.\n", ret);
		return ret;
	}

	return 0;
}

static const char * const yk3_fc_dbg_name[] = {
	"fc_xon",
	"fc_xoff",
	"pfc_mac_l1xon",
	"pfc_mac_l1xoff",
	"pfc_mac_l2xon",
	"pfc_mac_l2xoff",
	"pfc_pcie_l1xon",
	"pfc_pcie_l1xoff",
	"pfc_pcie_l2xon",
	"pfc_pcie_l2xoff",
};

static int yk3_get_fc_params(struct net_device *ndev, int offset, u32 *val)
{
	int ret = 0;

	switch (offset) {
	case YK3_DBG_FC_XON:
		ret = yk3_get_fc_xon(ndev, val);
		break;
	case YK3_DBG_FC_XOFF:
		ret = yk3_get_fc_xoff(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L1XON:
		ret = yk3_get_pfc_mac_l1xon(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L1XOFF:
		ret = yk3_get_pfc_mac_l1xoff(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L2XON:
		ret = yk3_get_pfc_mac_l2xon(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L2XOFF:
		ret = yk3_get_pfc_mac_l2xoff(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L1XON:
		ret = yk3_get_pfc_pcie_l1xon(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L1XOFF:
		ret = yk3_get_pfc_pcie_l1xoff(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L2XON:
		ret = yk3_get_pfc_pcie_l2xon(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L2XOFF:
		ret = yk3_get_pfc_pcie_l2xoff(ndev, val);
		break;
	default:
		break;
	}
	return ret;
}

static int yk3_set_fc_params(struct net_device *ndev, int offset, u32 val)
{
	int ret = 0;

	switch (offset) {
	case YK3_DBG_FC_XON:
		ret = yk3_set_fc_xon(ndev, val);
		break;
	case YK3_DBG_FC_XOFF:
		ret = yk3_set_fc_xoff(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L1XON:
		ret = yk3_set_pfc_mac_l1xon(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L1XOFF:
		ret = yk3_set_pfc_mac_l1xoff(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L2XON:
		ret = yk3_set_pfc_mac_l2xon(ndev, val);
		break;
	case YK3_DBG_PFC_MAC_L2XOFF:
		ret = yk3_set_pfc_mac_l2xoff(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L1XON:
		ret = yk3_set_pfc_pcie_l1xon(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L1XOFF:
		ret = yk3_set_pfc_pcie_l1xoff(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L2XON:
		ret = yk3_set_pfc_pcie_l2xon(ndev, val);
		break;
	case YK3_DBG_PFC_PCIE_L2XOFF:
		ret = yk3_set_pfc_pcie_l2xoff(ndev, val);
		break;
	default:
		break;
	}
	return ret;
}

static ssize_t fc_read(struct file *filp,
		       char __user *user_buf,
		       size_t count,
		       loff_t *ppos)
{
	struct yk3_fc_dbg *param = filp->private_data;
	int offset = param->offset;
	char buf[32];
	u32 val = 0;
	int ret = 0;

	ret = yk3_get_fc_params(param->ndev, offset, &val);
	if (ret)
		return ret;

	ret = snprintf(buf, sizeof(buf), "%u\n", val);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t fc_write(struct file *filp,
			const char __user *user_buf,
			size_t count,
			loff_t *ppos)
{
	struct yk3_fc_dbg *param = filp->private_data;
	int offset = param->offset;
	char buf[32] = {};
	u32 val = 0;
	int ret = 0;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = '\0';

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;

	ret = yk3_set_fc_params(param->ndev, offset, val);

	return ret ? ret : count;
}

static const struct file_operations fc_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = fc_read,
	.write = fc_write,
};

int yk3_debug_fc_init(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_fcdbg_params *fcdbg;
	int i;

	if (!pdev_priv)
		return -EINVAL;

	if (yk3_pdev_is_vf(pdev_priv) || yk3_pdev_is_mgr(pdev_priv))
		return 0;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return 0;

	if (!ndev_priv->dbgfs_dir)
		return -ENOENT;

	fcdbg = kzalloc(sizeof(*fcdbg), GFP_KERNEL);
	if (!fcdbg)
		return -ENOENT;

	fcdbg->root = debugfs_create_dir("fc", ndev_priv->dbgfs_dir);
	if (!fcdbg->root) {
		kfree(fcdbg);
		return -ENOMEM;
	}

	for (i = 0; i < YK3_DBG_FC_MAX; i++) {
		fcdbg->params[i].offset = i;
		fcdbg->params[i].ndev = ndev_priv->ndev;
		fcdbg->params[i].dentry = debugfs_create_file(yk3_fc_dbg_name[i],
							      0644, fcdbg->root,
							      &fcdbg->params[i],
							      &fc_fops);
	}

	ndev_priv->fcdbg = fcdbg;

	return 0;
}

void yk3_debug_fc_exit(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return;

	if (yk3_pdev_is_vf(pdev_priv) || yk3_pdev_is_mgr(pdev_priv))
		return;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return;

	if (!ndev_priv->dbgfs_dir ||
	    !ndev_priv->fcdbg ||
	    !ndev_priv->fcdbg->root)
		return;

	debugfs_remove_recursive(ndev_priv->fcdbg->root);
	kfree(ndev_priv->fcdbg);
	ndev_priv->fcdbg = NULL;
}

/**
 * yk3_prio_mode_init - init the priority state for dcbx
 * @ndev_priv: the corresponding netdev privite data
 */
static void yk3_prio_mode_init(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u8 prio_state = 0;
	int ret;
	int i;

	ret = yk3_ppp_primap_mode_get(pdev_priv);
	if (ret < 0) {
		yk3_dev_err("Failed to get prio mode, using default dscp mode.\n");
		prio_state = YK3_PRIO_DSCP;
	} else {
		yk3_dev_debug("get prio mode is %d.\n", ret);
		prio_state = ret;
	}

	if (prio_state == YK3_PRIO_DSCP) {
		ret = yk3_ppp_primap_dscp_map_get(pdev_priv, ndev_priv->dcbx->dscp2tc);
		if (ret) {
			yk3_dev_err("Failed to get dscp map, using default mapping.\n");
			for (i = 0; i < YK3_MAX_DSCP; i++) {
				ndev_priv->dcbx->dscp2tc[i] = i >> 3;
				ndev_priv->dcbx->dscp2prio[i] = i >> 3;
			}
		}
		memcpy(ndev_priv->dcbx->dscp2prio,
		       ndev_priv->dcbx->dscp2tc,
		       sizeof(ndev_priv->dcbx->dscp2prio));
	}

	WRITE_ONCE(ndev_priv->dcbx->prio, prio_state);
}

/**
 * yk3_dcbnl_query_dcbx_mode - query the dcbx mode
 * @pdev_priv: the corresponding pdev privite data
 * @mode: pointer to dcbx mode
 */
static int yk3_dcbnl_query_dcbx_mode(struct yk3_pdev_priv *pdev_priv, u32 *mode)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_msg recv_msg = {0};
	int ret;

	if (!mode)
		return -EINVAL;

	mbox_msg.opcode = YK3_MBOX_OPCODE_EMP_GET_LLDP_MODE;
	mbox_msg.dst_id = yk3_mbox_emp_id();
	mbox_opt.timeout = 1000;
	mbox_opt.wait_reply = MB_WAIT_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &recv_msg);
	if (ret != 0) {
		yk3_dev_err("Send mbox message errno %d failed.\n", ret);
		return ret;
	}

	*mode = *(u32 *)recv_msg.data;

	return 0;
}

/**
 * yk3_clear_pfc_register - clear pfc register when init and exit
 * @ndev_priv: the corresponding netdev privite data
 */
static int yk3_clear_pfc_register(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (!pdev_priv || !ndev_priv->dcbx || !pdev_priv->card)
		return -ENODEV;

	ret = yk3_mac_set_pfc(ndev_priv, pdev_priv->card->mac_chs[pdev_priv->pf_id], false);
	if (ret)
		yk3_net_warn("Set mac pfc failed, ret=%d\n", ret);

	ret = yk3_np_set_pfc(ndev_priv->ndev, 0, pdev_priv->card->mac_chs[pdev_priv->pf_id]);
	if (ret)
		yk3_net_warn("Set np pfc failed, ret=%d\n", ret);

	ndev_priv->dcbx->pfc.pfcena = 0;

	return 0;
}

/**
 * yk3_dcbnl_init - init DCBX capability
 * @ndev_priv: the corresponding netdev privite data
 */
void yk3_dcbnl_init(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_dcbx *dcbx;

	if (!pdev_priv)
		return;

	if (yk3_pdev_is_vf(pdev_priv) || yk3_pdev_is_mgr(pdev_priv))
		return;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return;

	dcbx = kzalloc(sizeof(*dcbx), GFP_KERNEL);
	if (!dcbx)
		return;

	ndev_priv->dcbx = dcbx;
	ndev_priv->ndev->dcbnl_ops = &ys_dcbnl_ops;

	if (yk3_clear_pfc_register(ndev_priv))
		yk3_dev_warn("Failed to clear pfc register.\n");

	if (yk3_clear_fc_register(ndev_priv))
		yk3_dev_warn("Failed to clear fc register.\n");

	if (yk3_dcbnl_query_dcbx_mode(pdev_priv, &dcbx->mode)) {
		yk3_dev_warn("Failed to query DCBX mode, using default HOST mode.\n");
		dcbx->mode = YK3_DCBX_HOST;
	}

	if (yk3_mac_set_priv_flags(ndev_priv, YK3_ET_PFLAG_PFC_PAUSE_FILTER, true)) {
		ndev_priv->ethtool_priv_flags &= ~BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER);
		yk3_net_err("umac set ethtool priv pfc pause filter flags failed\n");
	} else {
		ndev_priv->ethtool_priv_flags |= BIT(YK3_ET_PFLAG_PFC_PAUSE_FILTER);
	}

	if (yk3_mac_set_priv_flags(ndev_priv, YK3_ET_PFLAG_FC_PAUSE_FILTER, true)) {
		ndev_priv->ethtool_priv_flags &= ~BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER);
		yk3_net_err("umac set ethtool priv fc pause filter flags failed\n");
	} else {
		ndev_priv->ethtool_priv_flags |= BIT(YK3_ET_PFLAG_FC_PAUSE_FILTER);
	}

	yk3_prio_mode_init(ndev_priv);

	if (yk3_set_pause_srcaddr(pdev_priv, ndev_priv->ndev->dev_addr))
		yk3_dev_warn("Failed to set pause srcaddr.\n");

	if (yk3_set_pfc_pcie_l1vq(ndev_priv->ndev, 0x200, 0x160))
		yk3_dev_warn("Failed to set default pcie l1vq.\n");

	ndev_priv->dcbx->dcbx_cap = DCB_CAP_DCBX_VER_IEEE;
	if (ndev_priv->dcbx->mode == YK3_DCBX_HOST)
		ndev_priv->dcbx->dcbx_cap |= DCB_CAP_DCBX_HOST;
	else if (ndev_priv->dcbx->mode == YK3_DCBX_FW)
		ndev_priv->dcbx->dcbx_cap &= ~DCB_CAP_DCBX_HOST;

	//yk3_ets_init(priv);
}

/**
 * yk3_dcbnl_exit - uninit DCBX capability
 * @ndev_priv: the corresponding netdev privite data
 */
void yk3_dcbnl_exit(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	if (!pdev_priv)
		return;

	if (yk3_pdev_is_vf(pdev_priv)  || yk3_pdev_is_mgr(pdev_priv))
		return;

	if (pdev_priv->card->hw_bits & YK3_HW_BIT_XMAC)
		return;

	if (yk3_clear_pfc_register(ndev_priv))
		yk3_dev_warn("Failed to clear pfc register.\n");

	if (yk3_clear_fc_register(ndev_priv))
		yk3_dev_warn("Failed to clear fc register.\n");

	kfree(ndev_priv->dcbx);
	ndev_priv->dcbx = NULL;
}
