// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Driver for PHYs
 *
 * Copyright (c) 2024 DapuStor Corporation.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Support Phys: YT8531
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/of.h>
#include <linux/clk.h>
#include "dn200.h"

#define REG_DEBUG_ADDR_OFFSET           0x1e
#define REG_DEBUG_DATA                  0x1f
#define REG_SPECIFIC_STATUS_OFFSET      0x11
#define SPECIFIC_STATUS_SPEED_MASK      (0x3 << 14)
#define SPECIFIC_STATUS_SPEED_1000      (0x2 << 14)
#define SPECIFIC_STATUS_SPEED_100       (0x1 << 14)
#define SPECIFIC_STATUS_SPEED_10        (0x0 << 14)
#define SPECIFIC_STATUS_DUPLEX          (0x1 << 13)
#define SPECIFIC_STATUS_RESOLVED        (0x1 << 11)
#define SPECIFIC_STATUS_LINK            (0x1 << 10)
#define SPECIFIC_STATUS_MDI_X           (0x1 << 6)
#define SPECIFIC_STATUS_TX_PAUSE        (0x1 << 3)
#define SPECIFIC_STATUS_RX_PAUSE        (0x1 << 2)

#define REG_SPECIFIC_FUNCTION_CONTROL   0x10
#define SFC_FUNC_CTRL_CROSS_MODE_MASK   (3 << 5)
#define SFC_FUNC_CTRL_MANUAL_MDI        (0 << 5)
#define SFC_FUNC_CTRL_MANUAL_MDIX       (1 << 5)
#define SFC_FUNC_CTRL_AUTO_CROSS        (3 << 5)

static inline void phy_mdio_bus_lock(struct phy_device *phydev)
{
	mutex_lock(&phydev->mdio.bus->mdio_lock);
}

static inline void phy_mdio_bus_unlock(struct phy_device *phydev)
{
	mutex_unlock(&phydev->mdio.bus->mdio_lock);
}

static int phy_addr_get(struct phy_device *phydev)
{
	int addr = 0;

	addr = phydev->mdio.addr;
	return addr;
}

static struct mii_bus *phy_mii_bus_get(struct phy_device *phydev)
{
	struct mii_bus *bus = NULL;

	bus = phydev->mdio.bus;

	return bus;
}

int ytphy_read_ext(struct phy_device *phydev, u32 regnum)
{
	int ret;
	int addr;
	struct mii_bus *bus;

	addr = phy_addr_get(phydev);
	bus = phy_mii_bus_get(phydev);

	phy_mdio_bus_lock(phydev);
	ret = bus->write(bus, addr, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		goto err_handle;

	ret = bus->read(bus, addr, REG_DEBUG_DATA);

err_handle:
	phy_mdio_bus_unlock(phydev);
	return ret;
}

int ytphy_write_ext(struct phy_device *phydev, u32 regnum, u16 val)
{
	int ret;
	int addr;
	struct mii_bus *bus;

	addr = phy_addr_get(phydev);
	bus = phy_mii_bus_get(phydev);

	phy_mdio_bus_lock(phydev);
	ret = bus->write(bus, addr, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, REG_DEBUG_DATA, val);

err_handle:
	phy_mdio_bus_unlock(phydev);
	return ret;
}

static int ytphy_mmd_read(struct phy_device *phydev, u16 mmd, int regnum)
{
	int ret;
	int addr;
	struct mii_bus *bus;

	addr = phy_addr_get(phydev);
	bus = phy_mii_bus_get(phydev);

	phy_mdio_bus_lock(phydev);
	ret = bus->write(bus, addr, MII_MMD_CTRL, mmd);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, MII_MMD_DATA, regnum);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, MII_MMD_CTRL, mmd | MII_MMD_CTRL_NOINCR);
	if (ret < 0)
		goto err_handle;

	ret = bus->read(bus, addr, MII_MMD_DATA);

err_handle:
	phy_mdio_bus_unlock(phydev);
	return ret;
}

static int ytphy_mmd_write(struct phy_device *phydev, u16 mmd, int regnum, u16 val)
{
	int ret;
	int addr;
	struct mii_bus *bus;

	addr = phy_addr_get(phydev);
	bus = phy_mii_bus_get(phydev);

	phy_mdio_bus_lock(phydev);
	ret = bus->write(bus, addr, MII_MMD_CTRL, mmd);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, MII_MMD_DATA, regnum);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, MII_MMD_CTRL, mmd | MII_MMD_CTRL_NOINCR);
	if (ret < 0)
		goto err_handle;

	ret = bus->write(bus, addr, MII_MMD_DATA, val);

err_handle:
	phy_mdio_bus_unlock(phydev);
	return ret;
}

void extern_phy_force_led(struct phy_device *phydev, struct dn200_priv *priv, u32 index, u32 mode)
{
	u32 val;
	int ret;

	if (!phydev || index >= 3 || mode >= 3)
		return;

	ret = ytphy_read_ext(phydev, 0xa00b);
	if (ret < 0)
		return;

	val = ret;
	/* index: 0 -> active; 1 -> 1000M; 2 -> 100M */
	if (index) {
		/* index 1 off */
		val &= ~(0x7 << 3);
		val |= (priv->plat_ex->hw_rj45_type ? 0x4 : 0x5) << 3;
		/* index 2 off */
		val &= ~(0x7 << 6);
		val |= 0x4 << 6;
	}
	val &= ~(0x7 << (index * 3));

	switch (mode) {
	case 0:
		/* force on */
		val |= (index == 2 || priv->plat_ex->hw_rj45_type ? 0x5 : 0x4) << (index * 3);
		break;
	case 1:
		/* force off */
		val |= (index == 2 || priv->plat_ex->hw_rj45_type ? 0x4 : 0x5) << (index * 3);
		break;
	case 2:
		/* force blink */
		val |= 0x6 << (index * 3);
		break;
	default:
		val = ret;
		break;
	}

	ytphy_write_ext(phydev, 0xa00b, val);
}

void extern_phy_init(struct phy_device *phydev, u8 hw_type)
{
	int val = 0;

	ytphy_write_ext(phydev, 0xa012, 0x00c8);
	ytphy_write_ext(phydev, 0xa001, 0x8160);

	/* init para */
	ytphy_write_ext(phydev, 0x52, 0x231d);
	ytphy_write_ext(phydev, 0x51, 0x04a9);
	ytphy_write_ext(phydev, 0x57, 0x274c);

	phy_write(phydev, 0, 0x9140);

	/* for rgmii 1.8V */
	ytphy_write_ext(phydev, 0xa010, 0xabff);

	/* close all led */
	val = ytphy_read_ext(phydev, 0xa00b);
	if (val >= 0) {
		val &= ~0x1ff;
		val |= hw_type ? 0x124 : 0x12d;
		ytphy_write_ext(phydev, 0xa00b, val);
	}
	/* set rx tx delay time, default 0xf0 */
	ytphy_write_ext(phydev, 0xa003, 0x04f6);

	/* disable eee */
	val = ytphy_mmd_read(phydev, MDIO_MMD_AN, MDIO_AN_EEE_ADV);
	if (val >= 0) {
		val &= ~(MDIO_AN_EEE_ADV_100TX | MDIO_AN_EEE_ADV_1000T);
		ytphy_mmd_write(phydev, MDIO_MMD_AN, MDIO_AN_EEE_ADV, val);
	}

	/* enable stats */
	ytphy_write_ext(phydev, 0xa0, 0xa8d0);
}

int extern_phy_read_status(struct phy_device *phydev)
{
	int ret;
	u16 val;

	ret = phy_read(phydev, REG_SPECIFIC_STATUS_OFFSET);
	if (ret < 0)
		return ret;

	val = ret;

	phydev->link = val & SPECIFIC_STATUS_LINK ? DN200_LINK_UP : DN200_LINK_DOWN;
	if (phydev->link == DN200_LINK_DOWN) {
		phydev->speed = SPEED_UNKNOWN;
		phydev->duplex = DUPLEX_UNKNOWN;
		return 0;
	}

	switch (val & SPECIFIC_STATUS_SPEED_MASK) {
	case SPECIFIC_STATUS_SPEED_1000:
		phydev->speed = SPEED_1000;
		break;
	case SPECIFIC_STATUS_SPEED_100:
		phydev->speed = SPEED_100;
		break;
	case SPECIFIC_STATUS_SPEED_10:
		phydev->speed = SPEED_10;
		break;
	default:
		phydev->speed = SPEED_UNKNOWN;
		break;
	}

	phydev->duplex = val & SPECIFIC_STATUS_DUPLEX ? DUPLEX_FULL : DUPLEX_HALF;

	return 0;
}

int extern_phy_pause_autoneg_result(struct phy_device *phydev, bool *tx_pause, bool *rx_pause)
{
	int ret;
	u16 val;

	if (!phydev->link)
		return -EOPNOTSUPP;

	ret = phy_read(phydev, REG_SPECIFIC_STATUS_OFFSET);
	if (ret < 0)
		return ret;

	val = ret;

	*rx_pause = !!(val & SPECIFIC_STATUS_RX_PAUSE);
	*tx_pause = !!(val & SPECIFIC_STATUS_TX_PAUSE);

	return 0;
}

int extern_phy_mdix_status_get(struct phy_device *phydev, u8 *mdix, u8 *mdix_ctrl)
{
	int ret;
	u16 val;

	ret = phy_read(phydev, REG_SPECIFIC_STATUS_OFFSET);
	if (ret < 0)
		return ret;

	val = ret;

	*mdix = ETH_TP_MDI_INVALID;
	*mdix_ctrl = ETH_TP_MDI_INVALID;

	if (val & SPECIFIC_STATUS_RESOLVED) {
		if (val & SPECIFIC_STATUS_MDI_X)
			*mdix = ETH_TP_MDI_X;
		else
			*mdix = ETH_TP_MDI;

		ret = phy_read(phydev, REG_SPECIFIC_FUNCTION_CONTROL);
		if (ret < 0)
			return ret;

		val = ret;
		switch (val & SFC_FUNC_CTRL_CROSS_MODE_MASK) {
		case SFC_FUNC_CTRL_MANUAL_MDI:
			*mdix_ctrl = ETH_TP_MDI;
			break;
		case SFC_FUNC_CTRL_MANUAL_MDIX:
			*mdix_ctrl = ETH_TP_MDI_X;
			break;
		case SFC_FUNC_CTRL_AUTO_CROSS:
			*mdix_ctrl = ETH_TP_MDI_AUTO;
			break;
		default:
			*mdix_ctrl = ETH_TP_MDI_INVALID;
			break;
		}
	}

	return 0;
}

int extern_phy_mdix_status_set(struct phy_device *phydev, u8 ctrl)
{
	int ret;
	u16 val;

	ret = phy_read(phydev, REG_SPECIFIC_FUNCTION_CONTROL);
	if (ret < 0)
		return ret;

	val = ret;
	val &= ~SFC_FUNC_CTRL_CROSS_MODE_MASK;

	switch (ctrl) {
	case ETH_TP_MDI:
		val |= SFC_FUNC_CTRL_MANUAL_MDI;
		break;
	case ETH_TP_MDI_X:
		val |= SFC_FUNC_CTRL_MANUAL_MDIX;
		break;
	case ETH_TP_MDI_AUTO:
		val |= SFC_FUNC_CTRL_AUTO_CROSS;
		break;
	default:
		return 0;
	}

	ret = phy_write(phydev, REG_SPECIFIC_FUNCTION_CONTROL, val);
	if (ret < 0)
		return ret;

	ret = phy_read(phydev, MII_BMCR);
	if (ret < 0)
		return ret;

	val = ret;
	return phy_write(phydev, MII_BMCR, val | BMCR_RESET);
}

