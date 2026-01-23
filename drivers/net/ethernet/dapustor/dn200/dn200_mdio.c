// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include <linux/gpio/consumer.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mii.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/property.h>
#include <linux/slab.h>

#include "dwxgmac_comm.h"
#include "dn200.h"

#define MII_BUSY 0x00000001
#define MII_WRITE 0x00000002
#define MII_DATA_MASK GENMASK(15, 0)

/* GMAC4 defines */
#define MII_GMAC4_GOC_SHIFT 2
#define MII_GMAC4_REG_ADDR_SHIFT 16
#define MII_GMAC4_WRITE BIT(MII_GMAC4_GOC_SHIFT)
#define MII_GMAC4_READ (3 << MII_GMAC4_GOC_SHIFT)
#define MII_GMAC4_C45E BIT(1)

/* XGMAC defines */
#define MII_XGMAC_SADDR BIT(18)
#define MII_XGMAC_CMD_SHIFT 16
#define MII_XGMAC_WRITE BIT(MII_XGMAC_CMD_SHIFT)
#define MII_XGMAC_READ (3 << MII_XGMAC_CMD_SHIFT)
#define MII_XGMAC_BUSY BIT(22)
/* Extend max C22_address or phy_address from 3 to 15 */
#define MII_XGMAC_MAX_C22ADDR 31
#define MII_XGMAC_C22P_MASK GENMASK(MII_XGMAC_MAX_C22ADDR, 0)
#define MII_XGMAC_PA_SHIFT 16
#define MII_XGMAC_DA_SHIFT 21
#define MII_DEVADDR_C45_SHIFT 16
#define MII_REGADDR_C45_MASK	GENMASK(15, 0)
static DEFINE_MUTEX(dn200_mdio_mutex);

static void dn200_xgmac2_c45_format(struct dn200_priv *priv, int phyaddr,
				    int devad, int phyreg, u32 *hw_addr)
{
	u32 tmp;

	/* Set port as Clause 45 */
	tmp = readl(priv->ioaddr + XGMAC_MDIO_C22P);
	tmp &= ~BIT(phyaddr);
	writel(tmp, priv->ioaddr + XGMAC_MDIO_C22P);

	*hw_addr = (phyaddr << MII_XGMAC_PA_SHIFT) | (phyreg & 0xffff);
	*hw_addr |= (phyreg >> MII_DEVADDR_C45_SHIFT) << MII_XGMAC_DA_SHIFT;
}

static void dn200_xgmac2_c22_format(struct dn200_priv *priv, int phyaddr,
				    int phyreg, u32 *hw_addr)
{
	u32 tmp;
	/* Set port as Clause 22 */
	tmp = readl(priv->ioaddr + XGMAC_MDIO_C22P);
	tmp &= ~MII_XGMAC_C22P_MASK;
	tmp |= BIT(phyaddr);
	writel(tmp, priv->ioaddr + XGMAC_MDIO_C22P);

	*hw_addr = (phyaddr << MII_XGMAC_PA_SHIFT) | (phyreg & 0x1f);
}

static void dn200_mdio_bus_channel_set(struct dn200_priv *priv)
{
	u32 mdc_map, mdio_map;

	/* [8:0]mdc_map: 136 + xge_index * 22 */
	mdc_map = (priv->plat_ex->funcid * 22 + 136) & GENMASK(8, 0);
	/* [24:16]mdio_map: 137 + xge_index * 22 */
	mdio_map = ((priv->plat_ex->funcid * 22 + 137) << 16) & GENMASK(24, 16);
	writel(mdc_map | mdio_map, priv->ioaddr + XGMAC_MDIO_CHANNEL);
}

static int dn200_xgmac2_mdio_read(struct dn200_priv *priv,
	int phyaddr, int phyreg, u32 value)
{
	unsigned int mii_address = priv->hw->mii.addr;
	unsigned int mii_data = priv->hw->mii.data;
	u32 tmp, addr;
	int ret;

	mutex_lock(&dn200_mdio_mutex);
	dn200_mdio_bus_channel_set(priv);

	/* Wait until any existing MII operation is complete */
	if (readl_poll_timeout(priv->ioaddr + mii_data, tmp,
			       !(tmp & MII_XGMAC_BUSY), 100, 10000)) {
		ret = -EBUSY;
		goto err_disable_clks;
	}

	if (!(value & MII_XGMAC_SADDR))
		dn200_xgmac2_c45_format(priv, phyaddr, 0, phyreg, &addr);
	else
		dn200_xgmac2_c22_format(priv, phyaddr, phyreg, &addr);

	value |= (priv->clk_csr << priv->hw->mii.clk_csr_shift)
	    & priv->hw->mii.clk_csr_mask;
	value |= MII_XGMAC_READ;

	/* Wait until any existing MII operation is complete */
	if (readl_poll_timeout(priv->ioaddr + mii_data, tmp,
			       !(tmp & MII_XGMAC_BUSY), 100, 10000)) {
		ret = -EBUSY;
		goto err_disable_clks;
	}

	/* Set the MII address register to read */
	writel(addr, priv->ioaddr + mii_address);
	writel(value, priv->ioaddr + mii_data);

	/* Wait until any existing MII operation is complete */
	if (readl_poll_timeout(priv->ioaddr + mii_data, tmp,
			       !(tmp & MII_XGMAC_BUSY), 100, 10000)) {
		ret = -EBUSY;
		goto err_disable_clks;
	}

	/* Read the data from the MII data register */
	ret = (int)readl(priv->ioaddr + mii_data) & GENMASK(15, 0);

err_disable_clks:
	mutex_unlock(&dn200_mdio_mutex);
	return ret;
}

static int dn200_xgmac2_mdio_read_c22(struct mii_bus *bus, int phyaddr,
				      int phyreg)
{
	struct net_device *ndev = bus->priv;
	struct dn200_priv *priv;

	priv = netdev_priv(ndev);

	/* HW does not support C22 addr >= 4 */
	if (phyaddr > MII_XGMAC_MAX_C22ADDR)
		return -ENODEV;

	return dn200_xgmac2_mdio_read(priv, phyaddr, phyreg,
		MII_XGMAC_BUSY | MII_XGMAC_SADDR);
}

static int dn200_xgmac2_mdio_read_c45(struct mii_bus *bus, int phyaddr,
				      int devad, int phyreg)
{
	struct net_device *ndev = bus->priv;
	struct dn200_priv *priv;

	priv = netdev_priv(ndev);
	return dn200_xgmac2_mdio_read(priv, phyaddr, phyreg, MII_XGMAC_BUSY);
}

static int dn200_xgmac2_mdio_write(struct dn200_priv *priv,
	int phyaddr, int phyreg, u32 value, u16 phydata)
{
	unsigned int mii_address = priv->hw->mii.addr;
	unsigned int mii_data = priv->hw->mii.data;
	u32 tmp, addr;
	int ret;

	mutex_lock(&dn200_mdio_mutex);
	dn200_mdio_bus_channel_set(priv);

	/* Wait until any existing MII operation is complete */
	if (readl_poll_timeout(priv->ioaddr + mii_data, tmp,
			       !(tmp & MII_XGMAC_BUSY), 100, 10000)) {
		ret = -EBUSY;
		goto err_disable_clks;
	}

	if (!(value & MII_XGMAC_SADDR))
		dn200_xgmac2_c45_format(priv, phyaddr, 0, phyreg, &addr);
	else
		dn200_xgmac2_c22_format(priv, phyaddr, phyreg, &addr);

	value |= (priv->clk_csr << priv->hw->mii.clk_csr_shift)
	    & priv->hw->mii.clk_csr_mask;
	value |= phydata;
	value |= MII_XGMAC_WRITE;

	/* Wait until any existing MII operation is complete */
	if (readl_poll_timeout(priv->ioaddr + mii_data, tmp,
			       !(tmp & MII_XGMAC_BUSY), 100, 10000)) {
		ret = -EBUSY;
		goto err_disable_clks;
	}

	/* Set the MII address register to write */
	writel(addr, priv->ioaddr + mii_address);
	writel(value, priv->ioaddr + mii_data);

	/* Wait until any existing MII operation is complete */
	ret = readl_poll_timeout(priv->ioaddr + mii_data, tmp,
				 !(tmp & MII_XGMAC_BUSY), 100, 10000);

err_disable_clks:
	mutex_unlock(&dn200_mdio_mutex);
	return ret;
}

static int dn200_xgmac2_mdio_write_c22(struct mii_bus *bus, int phyaddr,
				       int phyreg, u16 phydata)
{
	struct net_device *ndev = bus->priv;
	struct dn200_priv *priv;

	priv = netdev_priv(ndev);

	/* HW does not support C22 addr >= 4 */
	if (phyaddr > MII_XGMAC_MAX_C22ADDR)
		return -ENODEV;

	return dn200_xgmac2_mdio_write(priv, phyaddr, phyreg,
		MII_XGMAC_BUSY | MII_XGMAC_SADDR, phydata);
}

static int dn200_xgmac2_mdio_write_c45(struct mii_bus *bus, int phyaddr,
				       int devad, int phyreg, u16 phydata)
{
	struct net_device *ndev = bus->priv;
	struct dn200_priv *priv;

	priv = netdev_priv(ndev);
	return dn200_xgmac2_mdio_write(priv, phyaddr, devad, MII_XGMAC_BUSY, phydata);
}


/**
 * dn200_mdio_reset
 * @bus: points to the mii_bus structure
 * Description: reset the MII bus
 */
int dn200_mdio_reset(struct mii_bus *bus)
{
	return 0;
}

/**
 * dn200_mdio_register
 * @ndev: net device structure
 * Description: it registers the MII bus
 */
int dn200_mdio_register(struct net_device *ndev)
{
	int err = 0;
	struct mii_bus *new_bus;
	struct dn200_priv *priv = netdev_priv(ndev);
	struct dn200_mdio_bus_data *mdio_bus_data = priv->plat->mdio_bus_data;
	struct device_node *mdio_node = priv->plat->mdio_node;
	struct device *dev = ndev->dev.parent;
	int addr, found, max_addr;
	int i = 0;

	if (!mdio_bus_data)
		return 0;

	new_bus = mdiobus_alloc();
	if (!new_bus)
		return -ENOMEM;

	if (mdio_bus_data->irqs) {
		while (mdio_bus_data->irqs[i] && (i < sizeof(new_bus->irq))) {
			new_bus->irq[i] = mdio_bus_data->irqs[i];
			i++;
		}
	}

	new_bus->name = "dn200";

	if (priv->plat->has_xgmac) {
		new_bus->read = &dn200_xgmac2_mdio_read_c22;
		new_bus->write = &dn200_xgmac2_mdio_write_c22;
		new_bus->read_c45 = &dn200_xgmac2_mdio_read_c45;
		new_bus->write_c45 = &dn200_xgmac2_mdio_write_c45;
		/* Right now only C22 phys are supported */
		max_addr = MII_XGMAC_MAX_C22ADDR + 1;

		/* Check if DT specified an unsupported phy addr */
		if (priv->plat->phy_addr > MII_XGMAC_MAX_C22ADDR)
			dev_err(dev, "Unsupported phy_addr (max=%d)\n",
				MII_XGMAC_MAX_C22ADDR);
	} else {
		goto bus_register_fail;
	}

	if (mdio_bus_data->needs_reset)
		new_bus->reset = &dn200_mdio_reset;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s-%x-%x-%x",
		 new_bus->name, pci_domain_nr(priv->plat_ex->pdev->bus),
		 priv->plat_ex->pdev->bus->number, priv->plat->bus_id);
	new_bus->priv = ndev;
	/* ignore phy addr 0 */
	if (mdio_bus_data->phy_mask)
		new_bus->phy_mask = mdio_bus_data->phy_mask;
	else
		new_bus->phy_mask = 0x1;
	new_bus->parent = priv->device;

	err = mdiobus_register(new_bus);
	if (err != 0) {
		dev_err(dev, "Cannot register the MDIO bus\n");
		goto bus_register_fail;
	}

	/* Looks like we need a dummy read for XGMAC only and C45 PHYs */
	if (priv->plat->has_xgmac)
		dn200_xgmac2_mdio_read_c45(new_bus, 0, 0, 0);

	if (priv->plat->phy_node || mdio_node)
		goto bus_register_done;

	found = 0;
	for (addr = 0; addr < max_addr; addr++) {
		struct phy_device *phydev = mdiobus_get_phy(new_bus, addr);

		if (!phydev)
			continue;

		/* If an IRQ was provided to be assigned after
		 * the bus probe, do it here.
		 */
		if (!mdio_bus_data->irqs &&
		    mdio_bus_data->probed_phy_irq > 0) {
			new_bus->irq[addr] = mdio_bus_data->probed_phy_irq;
			phydev->irq = mdio_bus_data->probed_phy_irq;
		}

		/* If we're going to bind the MAC to this PHY bus,
		 * and no PHY number was provided to the MAC,
		 * use the one probed here.
		 */
		if (priv->plat->phy_addr == -1)
			priv->plat->phy_addr = addr;

		// phy_attached_info(phydev);
		found = 1;
	}

	if (!found && !mdio_node) {
		dev_warn(dev, "No PHY found\n");
		err = -ENODEV;
		goto no_phy_found;
	}

bus_register_done:
	priv->mii = new_bus;

	return 0;

no_phy_found:
	mdiobus_unregister(new_bus);
bus_register_fail:
	mdiobus_free(new_bus);
	return err;
}

/**
 * dn200_mdio_unregister
 * @ndev: net device structure
 * Description: it unregisters the MII bus
 */
int dn200_mdio_unregister(struct net_device *ndev)
{
	struct dn200_priv *priv = netdev_priv(ndev);

	if (!priv->mii)
		return 0;

	mdiobus_unregister(priv->mii);
	priv->mii->priv = NULL;
	mdiobus_free(priv->mii);
	priv->mii = NULL;

	return 0;
}
