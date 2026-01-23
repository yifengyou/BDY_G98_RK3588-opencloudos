// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config extern phy or xpcs phy
 */
#include "dn200_phy.h"
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/phylink.h>
#include <linux/iopoll.h>
#include "dn200_self.h"
#include "dn200_sriov.h"
#include "dn200_self.h"
#include "mmc.h"

#pragma GCC diagnostic ignored "-Warray-bounds"
static DEFINE_MUTEX(dn200_gpio_mutex);
static DEFINE_SPINLOCK(dn200_xpcs_lock);
struct xpcs_link_down_dump xpcs_link_down_dump_regs[] = {
	/*DUMP PMA_PMD STATUS REG INFO */
	{ "SR_PMA_STATUS1", MDIO_MMD_PMAPMD, MDIO_STAT1 },	//0x40004
	{ "SR_PMA_STATUS2", MDIO_MMD_PMAPMD, MDIO_STAT2 },	//0x40020
	{ "SR_PMA_RX_SIG_DET", MDIO_MMD_PMAPMD, MDIO_PMA_RXDET },	//0x40040

	/*DUMP PCS STATUS REG INFO */
	{ "SR_XS_PCS_STS1", MDIO_MMD_PCS, MDIO_STAT1 },	//0xc0004
	{ "SR_XS_PCS_STS2", MDIO_MMD_PCS, MDIO_STAT2 },	//0xc0020
	{ "SR_XS_PCS_LSTS", MDIO_MMD_PCS, MDIO_PHYXS_LNSTAT },	//0xc0060
	{ "SR_XS_PCS_KR_STS1", MDIO_MMD_PCS, XS_PCS_KR_STS1 },	//0xc0080
	{ "SR_XS_PCS_KR_STS2", MDIO_MMD_PCS, XS_PCS_KR_STS2 },	//0xc0084

	/*DUMP SERDES STATUS REG INFO */
	{ "VR_XS_PMA_RX_LSTS", MDIO_MMD_PMAPMD, VR_XS_PMA_RX_LSTS },	//0x60080

	/*DUMP AN STATUS REG INFO */
	{ "AN CTRL1", MDIO_MMD_AN, MDIO_CTRL1 },
	{ "AN STAT2", MDIO_MMD_AN, MDIO_STAT2 },
	{ "AN COMP_STS", MDIO_MMD_AN, SR_AN_COMP_STS },
};

static int dn200_phy_info_state_change(struct dn200_phy_info *phy_info);
static void dn200_phy_print_status(struct dn200_phy_info *phy_info);
static int dn200_xpcs_read(struct dn200_phy_info *phy_info, u32 addr, u32 devad,
			   u32 reg)
{
	u32 phy_reg = 0;
	u32 val = 0;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	int ret = 0;

	if (devad == MDIO_DEVAD_NONE) {
		// do nothing
		return 0xffff;
	}
	phy_reg |= (devad & 0x1F) << 18;
	phy_reg |= (reg & 0xFFFF) << 2;
	if (phy_reg >= 0xc0000 && phy_reg <= 0xc00ff) {
		phy_reg -= 0xc0000;
		val = XPCS32_IOREAD(phy_info->xpcs, phy_reg);
	} else {
		ret = fw_reg_read(&priv->plat_ex->ctrl, 0x2c000000 + 0x800000 * phy_info->xpcs_idx + phy_reg, &val);
		if (ret < 0) {
			dev_err(priv->device, "(%s) read xpcs apb address=%08x val=%08x faile! err %d\n",
				__func__, phy_reg, val, ret);
		}
	}
	dev_dbg(priv->device, "(%s) xpcs apb address=%08x val=%08x\n",
		__func__, phy_reg, val);
	return val;
}

static int dn200_xpcs_write(struct dn200_phy_info *phy_info, u32 addr,
			    u32 devad, u32 reg, u32 val)
{
	u32 phy_reg = 0;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	int ret = 0;

	if (devad == MDIO_DEVAD_NONE) {
		// do nothing
	} else {
		phy_reg |= (devad & 0x1F) << 18;
		phy_reg |= (reg & 0xFFFF) << 2;
		dev_dbg(priv->device,
			"(%s) write %08x to xpcs apb address %08x\n", __func__,
			val, phy_reg);
		if (phy_reg >= 0xc0000 && phy_reg <= 0xc00ff) {
			phy_reg -= 0xc0000;
			XPCS32_IOWRITE(phy_info->xpcs, phy_reg, val);
		} else  {
			ret = fw_reg_write(&priv->plat_ex->ctrl, 0x2c000000 + 0x800000 * phy_info->xpcs_idx + phy_reg, val);
			if (ret < 0)
				dev_err(priv->device, "(%s) write %08x to xpcs apb address %08x failed! ret %d\n", __func__,
					val, phy_reg, ret);
		}
	}
	return 0;
}

static int dn200_xpcs_set_bit(struct dn200_phy_info *phy_info, u32 addr,
			      u32 devad, u32 reg, u32 mask, u32 shift, u32 val)
{
	u16 reg_val = 0;

	if (devad == MDIO_DEVAD_NONE) {
		// do nothing
	} else {
		reg_val = dn200_xpcs_read(phy_info, addr, devad, reg);
		reg_val &= ~(mask);
		reg_val |= (val << shift);
		dn200_xpcs_write(phy_info, addr, devad, reg, reg_val);
	}
	return 0;
}

static int dn200_phy_read(struct dn200_phy_info *phy_info, int devad, int reg)
{
	if (phy_info->phydev)
		return __phy_read(phy_info->phydev, reg);
	else if (phy_info->xpcs)
		return phy_info->xpcs->xpcs_read(phy_info, phy_info->phy_addr,
						 devad, reg);
	return 0;
}

static int __maybe_unused dn200_phy_write(struct dn200_phy_info *phy_info,
					  int devad, int reg, u32 val)
{
	if (phy_info->phydev)
		return __phy_write(phy_info->phydev, reg, val);
	else if (phy_info->xpcs)
		return phy_info->xpcs->xpcs_write(phy_info, phy_info->phy_addr,
						  devad, reg, val);
	return 0;
}

static u16 dn200_xpcs_phy_reg_read(struct dn200_phy_info *phy_info, u8 dev,
				   u32 reg)
{
	u32 reg_result = 0;

	reg_result |= (((dev & 0xff) << 24) | ((reg & 0xff) << 16));
	reg_result |= (dn200_phy_read(phy_info, dev, reg) & 0xffff);
	return reg_result;
}

static int dn200_extern_phy_identity(struct dn200_phy_info *phy_info)
{
	u32 id = 0;
	int ret = 0;

	/* First, search C73 PCS using PCS MMD */
	ret = dn200_phy_read(phy_info, MDIO_MMD_PCS, MII_PHYSID1);
	if (ret < 0)
		return 0xffffffff;

	id = (u32) ret << 16;

	ret = dn200_phy_read(phy_info, MDIO_MMD_PCS, MII_PHYSID2);
	if (ret < 0)
		return 0xffffffff;
	id |= (u32) ret;

	return id;
}

static void dn200_phy_sfp_present(struct dn200_phy_info *phy_info);
static int dn200_i2c_xfer(struct dn200_phy_info *phy_info,
				       u8 byte_offset, u8 dev_addr, u8 command, u8 *data)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	int retry = 0;
	int ret = 0;

	while (retry < 5) {
		ret = dn200_fw_i2c_rw_commit(&priv->plat_ex->ctrl, dev_addr, data, byte_offset, command);
		if (ret <= 0)
			return ret;
		if (ret == 1) { /*read or write timeout*/
			dn200_phy_sfp_present(phy_info);
			if (phy_info->sfp_mod_absent) {
				netdev_warn(phy_info->dev, "sfp not present\n");
				return ret;
			}
		}
		retry++;
	}
	netdev_err(phy_info->dev, "i2c command fail ret = %d\n", ret);
	return ret;
}
static int dn200_generic_i2c_byte_read(struct dn200_phy_info *phy_info,
				       u8 byte_offset, u8 dev_addr, u8 *data)
{
	int ret = 0;

	ret = dn200_i2c_xfer(phy_info, byte_offset, dev_addr, IC_READ, data);
	return ret;
}

static int dn200_generic_i2c_byte_write(struct dn200_phy_info *phy_info,
					u8 byte_offset, u8 dev_addr, u8 data)
{
	int ret = 0;

	ret = dn200_i2c_xfer(phy_info, byte_offset, dev_addr, IC_WRITE, &data);
	return ret;
}

static int dn200_i2c_read_eeprom(struct dn200_phy_info *phy_info,
				 u8 byte_offset, u8 *eeprom_data)
{
	return phy_info->phy_ops->read_i2c_byte(phy_info, byte_offset,
						DN200_I2C_EEPROM_DEV_ADDR,
						eeprom_data);
}

static int dn200_i2c_read_sff8472(struct dn200_phy_info *phy_info,
				  u8 byte_offset, u8 *eeprom_data)
{
	return phy_info->phy_ops->read_i2c_byte(phy_info, byte_offset,
						DN200_I2C_SFF8472_DEV_ADDR,
						eeprom_data);
}

static int dn200_i2c_write_eeprom(struct dn200_phy_info *phy_info,
				  u8 byte_offset, u8 eeprom_data)
{
	return phy_info->phy_ops->write_i2c_byte(phy_info, byte_offset,
						 DN200_I2C_EEPROM_DEV_ADDR,
						 eeprom_data);
}

static void dn200_gpio_iowrite(struct dn200_phy_info *phy_info, int offset_val,
			       u8 val)
{
	u32 value = 0;
	u8 offset_pin = 0;
	u16 offset = offset_val & 0xffff;

	mutex_lock(&dn200_gpio_mutex);
	if ((offset_val >> 16) & 0x1)
		val = (~val) & 0x1;
	/*may be offset is greater than 31 */
	offset_pin = (offset >> 5) << 2;
	offset = offset & 31;
	value =
	    ioread32(phy_info->gpio_base + offset_pin +
		     phy_info->gpio_data->reg_off_set_write);
	value &= ~BIT(offset);
	value |= val << offset;
	iowrite32(value,
		  phy_info->gpio_base + offset_pin +
		  phy_info->gpio_data->reg_off_set_write);

	mutex_unlock(&dn200_gpio_mutex);
}

static u8 dn200_gpio_ioread(struct dn200_phy_info *phy_info, int offset_val)
{
	u32 value = 0;
	u8 offset_pin = 0;
	u8 val = 0;
	u16 offset = offset_val & 0xffff;

	mutex_lock(&dn200_gpio_mutex);

	/*may be offset is greater than 31 */
	offset_pin = (offset >> 5) << 2;
	offset = offset & 31;
	value =
	    ioread32(phy_info->gpio_base + offset_pin +
		     phy_info->gpio_data->reg_off_set_read);
	val = (value >> (offset)) & 0x1;
	if ((offset_val >> 16) & 0x1)
		val = (~val) & 0x1;
	mutex_unlock(&dn200_gpio_mutex);

	return val;
}

static void dn200_phy_sfp_present(struct dn200_phy_info *phy_info)
{
	if (phy_info->sfp_has_gpio) {
		phy_info->sfp_mod_absent =
		    dn200_gpio_ioread(phy_info,
				      phy_info->gpio_data->sfp_detect_pin);
	}
}

static void dn200_phy_sfp_rx_los(struct dn200_phy_info *phy_info)
{
	if (phy_info->sfp_has_gpio) {
		phy_info->sfp_rx_los =
		    dn200_gpio_ioread(phy_info,
				      phy_info->gpio_data->sfp_rx_los_pin);
	}
}

static void dn200_phy_sfp_tx_falut(struct dn200_phy_info *phy_info)
{
	if (phy_info->sfp_has_gpio) {
		phy_info->sfp_tx_falut =
		    dn200_gpio_ioread(phy_info,
				      phy_info->gpio_data->sfp_tx_fault_pin);
	}
}

static void dn200_phy_set_sfp_tx_disable(struct dn200_phy_info *phy_info)
{
	if (phy_info->sfp_has_gpio) {
		dn200_gpio_iowrite(phy_info,
				   phy_info->gpio_data->sfp_tx_disable_pin,
				   phy_info->sfp_tx_disable);
	}
}

static void dn200_phy_set_rs_mode(struct dn200_phy_info *phy_info, bool high)
{
	if (high) {
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_rs0_pin,
				   1);
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_rs1_pin,
				   1);
	} else {
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_rs0_pin,
				   0);
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_rs1_pin,
				   0);
	}
}

static void dn200_phy_set_led(struct dn200_phy_info *phy_info, bool on)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (phy_info->phydev) {
		if (!on) {
			extern_phy_force_led(phy_info->phydev, priv, 0, 1);
			extern_phy_force_led(phy_info->phydev, priv, 1, 1);
			return;
		}
		if (phy_info->speed == SPEED_1000)
			extern_phy_force_led(phy_info->phydev, priv, 1, 0);
		else if (phy_info->speed == SPEED_100)
			extern_phy_force_led(phy_info->phydev, priv, 2, 0);
		else
			extern_phy_force_led(phy_info->phydev, priv, 1, 1);

		extern_phy_force_led(phy_info->phydev, priv, 0, 0);
		return;
	}
	if (on) {
		if (phy_info->speed == SPEED_10000) {
			dn200_gpio_iowrite(phy_info,
					   phy_info->gpio_data->sfp_led1_pin,
					   1);
			dn200_gpio_iowrite(phy_info,
					   phy_info->gpio_data->sfp_led2_pin,
					   0);
		} else if (phy_info->speed == SPEED_1000) {
			dn200_gpio_iowrite(phy_info,
					   phy_info->gpio_data->sfp_led2_pin,
					   1);
			dn200_gpio_iowrite(phy_info,
					   phy_info->gpio_data->sfp_led1_pin,
					   0);
		}
	} else {
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_led2_pin,
				   1);
		dn200_gpio_iowrite(phy_info, phy_info->gpio_data->sfp_led1_pin,
				   1);
	}
}

static void dn200_blink_control(struct dn200_phy_info *phy_info,
				bool link_status)
{
	u64 tmp_rx = 0;
	u64 tmp_tx = 0;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	dwxgmac_read_mmc_reg(priv->mmcaddr, MMC_XGMAC_RX_PKT_GB, &tmp_rx);
	dwxgmac_read_mmc_reg(priv->mmcaddr, MMC_XGMAC_TX_PKT_GB, &tmp_tx);
	priv->mmc.mmc_rx_framecount_gb += tmp_rx;
	priv->mmc.mmc_tx_framecount_gb += tmp_tx;
	if ((tmp_rx || tmp_tx) && link_status) {
		if (!priv->blink_state_last) {
			if (phy_info->phydev)
				extern_phy_force_led(phy_info->phydev, priv, 0, 2);
			else
				dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_ENABLE);
			priv->blink_state_last = BLINK_ENABLE;
		}
	} else {
		if (priv->blink_state_last) {
			if (phy_info->phydev)
				extern_phy_force_led(phy_info->phydev, priv, 0, !phy_info->phydev->link);
			else
				dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_DISABLE);
			priv->blink_state_last = BLINK_DISABLE;
		}
	}
}

static int dn200_sfp_module_identify(struct dn200_phy_info *phy_info)
{
	s32 err = DN200_ERR_PHY_ADDR_INVALID;
	u32 vendor_oui = 0;
	u8 identifier = 0;
	u8 oui_bytes[3] = { 0, 0, 0 };

	dn200_phy_sfp_present(phy_info);
	if (phy_info->sfp_mod_absent)
		return DN200_ERR_SFP_NOT_PRESENT;

	if (phy_info->media_type != DN200_MEDIA_TYPE_XPCS_1000BASEX &&
	    phy_info->media_type != DN200_MEDIA_TYPE_XPCS_10GBASEKR) {
		phy_info->sfp_type = (u8) DN200_SFP_TYPE_NOT_PRESENT;
		return DN200_ERR_SFP_NOT_PRESENT;
	}
	err = phy_info->phy_ops->read_i2c_eeprom(phy_info, DN200_SFF_IDENTIFIER,
						 &identifier);
	if (err != 0) {
ERR_I2C:
		dn200_phy_sfp_present(phy_info);
		if (!phy_info->sfp_mod_absent)
			netdev_warn(phy_info->dev, "sfp not present\n");
		phy_info->sfp_type = (u8) DN200_SFP_TYPE_NOT_PRESENT;
		return DN200_ERR_SFP_NOT_PRESENT;
	}

	if (identifier != DN200_SFF_IDENTIFIER_SFP) {
		netdev_warn(phy_info->dev, "sfp not support, identifier %d\n", identifier);
		phy_info->sfp_type = (u8) DN200_SFP_TYPE_UNKNOWN;
		return DN200_ERR_SFP_NOT_SUPPORTED;
	}

	/* Determine SFF module vendor */
	err = phy_info->phy_ops->read_i2c_eeprom(phy_info,
						 DN200_SFF_VENDOR_OUI_BYTE0,
						 &oui_bytes[0]);
	if (err != 0)
		goto ERR_I2C;

	err = phy_info->phy_ops->read_i2c_eeprom(phy_info,
						 DN200_SFF_VENDOR_OUI_BYTE1,
						 &oui_bytes[1]);
	if (err != 0)
		goto ERR_I2C;

	err = phy_info->phy_ops->read_i2c_eeprom(phy_info,
						 DN200_SFF_VENDOR_OUI_BYTE2,
						 &oui_bytes[2]);
	if (err != 0)
		goto ERR_I2C;

	vendor_oui = ((u32)oui_bytes[0] << 24) |
	    ((u32)oui_bytes[1] << 16) | ((u32)oui_bytes[2] << 8);
	switch (vendor_oui) {
	case DN200_SFF_VENDOR_OUI_AVAGO:
		phy_info->sfp_module_type = DN200_PHY_SFP_MODULE_AVAGO;
		break;
	default:
		phy_info->sfp_module_type = DN200_PHY_SFP_MODULE_UNKNOWN;
		break;
	}
	phy_info->sfp_id = DN200_SFF_IDENTIFIER_SFP;
	return err;
}

static void dn200_phy_sfp_reset(struct dn200_phy_info *phy_info)
{
	phy_info->sfp_rx_los = 0;
	phy_info->sfp_tx_disable = 0;
	dn200_phy_sfp_present(phy_info);
	phy_info->sfp_base = DN200_SFP_BASE_UNKNOWN;
	phy_info->sfp_cable = DN200_SFP_CABLE_UNKNOWN;
	phy_info->sfp_speed = DN200_SFP_SPEED_UNKNOWN;
	phy_info->multispeed_sfp = false;
}

static bool dn200_phy_sfp_bit_rate(struct dn200_phy_info *phy_info,
				   u32 sfp_speed)
{
	u8 min;
	u8 sfp_base_br;

	phy_info->phy_ops->read_i2c_eeprom(phy_info, DN200_SFP_BASE_BR,
					   &sfp_base_br);

	switch (sfp_speed) {
	case SPEED_1000:
		min = DN200_SFP_BASE_BR_1GBE_MIN;
		break;
	case SPEED_10000:
		min = DN200_SFP_BASE_BR_10GBE_MIN;
		break;
	default:
		return false;
	}

	return sfp_base_br >= min;
}

static void dn200_clear_lks(struct dn200_phy_info *phy_info, int type, bool is_sup);
static void dn200_link_status_reset(struct dn200_phy_info *phy_info, bool enable);
static void dn200_xpcs_switch_to_10G(struct dn200_phy_info *phy_info);
static int dn200_phy_sfp_detect(struct dn200_phy_info *phy_info)
{
	u8 sff_cable, sff_10g_comp, sff_1g_comp;
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	u32 sfp_speed = phy_info->sfp_speed;

	/* Reset the SFP signals and info */
	dn200_phy_sfp_reset(phy_info);
	/* Read the SFP signals and check for module presence */
	dn200_phy_sfp_present(phy_info);
	if (phy_info->sfp_mod_absent)
		goto put;

	dn200_clear_lks(phy_info, DN200_SFP_TYPE_1000 | DN200_SFP_TYPE_10000, true);
	phy_info->link_modes = 0;
	phy_info->phy_ops->read_i2c_eeprom(phy_info, DN200_SFF_CABLE_TECHNOLOGY,
					   &sff_cable);
	/* Assume FIBRE cable unless told otherwise */
	if (sff_cable & DN200_SFP_BASE_CABLE_PASSIVE)
		phy_info->sfp_cable = DN200_SFP_CABLE_PASSIVE;
	else if (sff_cable & DN200_SFP_BASE_CABLE_ACTIVE)
		phy_info->sfp_cable = DN200_SFP_CABLE_ACTIVE;
	else
		phy_info->sfp_cable = DN200_SFP_CABLE_FIBRE;

	/* Determine the type of SFP */
	phy_info->phy_ops->read_i2c_eeprom(phy_info, DN200_SFF_10GBE_COMP_CODES,
					   &sff_10g_comp);
	phy_info->phy_ops->read_i2c_eeprom(phy_info, DN200_SFF_1GBE_COMP_CODES,
					   &sff_1g_comp);

	/*10G mode */
	if (phy_info->sfp_cable != DN200_SFP_CABLE_FIBRE &&
	    dn200_phy_sfp_bit_rate(phy_info, SPEED_10000))
		phy_info->sfp_base |= DN200_SFP_BASE_10000_CR;
	else if (sff_10g_comp & DN200_SFP_BASE_10GBE_CC_SR)
		phy_info->sfp_base |= DN200_SFP_BASE_10000_SR;
	else if (sff_10g_comp & DN200_SFP_BASE_10GBE_CC_LR)
		phy_info->sfp_base |= DN200_SFP_BASE_10000_LR;
	else if (sff_10g_comp & DN200_SFP_BASE_10GBE_CC_LRM)
		phy_info->sfp_base |= DN200_SFP_BASE_10000_LRM;
	else if (sff_10g_comp & DN200_SFP_BASE_10GBE_CC_ER)
		phy_info->sfp_base |= DN200_SFP_BASE_10000_ER;

	/*check sfp module 1G mode */
	if (sff_1g_comp & DN200_SFP_BASE_1GBE_CC_SX)
		phy_info->sfp_base |= DN200_SFP_BASE_1000_SX;
	else if (sff_1g_comp & DN200_SFP_BASE_1GBE_CC_LX)
		phy_info->sfp_base |= DN200_SFP_BASE_1000_LX;
	else if (sff_1g_comp & DN200_SFP_BASE_1GBE_CC_CX)
		phy_info->sfp_base |= DN200_SFP_BASE_1000_CX;
	else if (sff_1g_comp & DN200_SFP_BASE_1GBE_CC_T)
		phy_info->sfp_base |= DN200_SFP_BASE_1000_T;

	switch (phy_info->sfp_base) {
	case DN200_SFP_BASE_1000_T:
		DN200_SET_SUP(lks, 1000baseT_Full);
		DN200_SET_SUP(lks, 100baseT_Full);
		phy_info->sfp_speed =
		    (DN200_SFP_SPEED_1000 | DN200_SFP_SPEED_100);
		phy_info->link_modes |= DN300_100BASET_Full | DN300_1000BASET_Full;
		break;
	case DN200_SFP_BASE_1000_SX:
		DN200_SET_SUP(lks, 1000baseX_Full);
		phy_info->link_modes |= DN300_1000BASET_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_1000;
		break;
	case DN200_SFP_BASE_1000_LX:
	case DN200_SFP_BASE_1000_CX:
		DN200_SET_SUP(lks, 1000baseX_Full);
		phy_info->link_modes |= DN300_1000BASET_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_1000;
		break;
	case DN200_SFP_BASE_10000_SR:
		DN200_SET_SUP(lks, 10000baseSR_Full);
		phy_info->link_modes |= DN300_10000baseSR_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_10000;
		break;
	case DN200_SFP_BASE_10000_LR:
		DN200_SET_SUP(lks, 10000baseLR_Full);
		phy_info->link_modes |= DN300_10000baseLR_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_10000;
		break;
	case DN200_SFP_BASE_10000_LRM:
		DN200_SET_SUP(lks, 10000baseLRM_Full);
		phy_info->link_modes |= DN300_10000baseLRM_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_10000;
		break;
	case DN200_SFP_BASE_10000_ER:
		DN200_SET_SUP(lks, 10000baseER_Full);
		phy_info->link_modes |= DN300_10000baseER_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_10000;
		break;
	case DN200_SFP_BASE_10000_CR:
		DN200_SET_SUP(lks, 10000baseCR_Full);
		phy_info->link_modes |= DN300_10000baseCR_Full;
		phy_info->sfp_speed |= DN200_SFP_SPEED_10000;
		break;
	case DN200_SFP_BASE_10000_SR | DN200_SFP_BASE_1000_SX:
		phy_info->link_modes |= DN300_10000baseSR_Full | DN300_1000BASEX_Full;
		DN200_SET_SUP(lks, 10000baseSR_Full);
		DN200_SET_SUP(lks, 1000baseX_Full);
		phy_info->sfp_speed =
		    DN200_SFP_SPEED_10000 | DN200_SFP_SPEED_1000;
		break;
	case DN200_SFP_BASE_10000_LR | DN200_SFP_BASE_1000_LX:
		phy_info->link_modes |= DN300_10000baseLR_Full | DN300_1000BASEX_Full;
		DN200_SET_SUP(lks, 10000baseLR_Full);
		DN200_SET_SUP(lks, 1000baseX_Full);
		phy_info->sfp_speed =
		    DN200_SFP_SPEED_10000 | DN200_SFP_SPEED_1000;
		break;
	case DN200_SFP_BASE_10000_CR | DN200_SFP_BASE_1000_CX:
		phy_info->link_modes |= DN300_10000baseCR_Full | DN300_1000BASEX_Full;
		DN200_SET_SUP(lks, 10000baseCR_Full);
		DN200_SET_SUP(lks, 1000baseX_Full);
		phy_info->sfp_speed =
		    DN200_SFP_SPEED_10000 | DN200_SFP_SPEED_1000;
		break;
	default:
		break;
	}

	if ((phy_info->sfp_speed & DN200_SFP_SPEED_10000)
	    && (phy_info->sfp_speed & DN200_SFP_SPEED_1000)) {
		phy_info->multispeed_sfp = true;
	}

	if (sfp_speed != phy_info->sfp_speed && sfp_speed != DN200_SFP_SPEED_UNKNOWN)
		phy_info->sfp_changed = true;
	bitmap_copy(lks->link_modes.advertising,
		    lks->link_modes.supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
	if (phy_info->speed == SPEED_1000) {
		dn200_clear_lks(phy_info, DN200_SFP_TYPE_10000, false);
		phy_info->link_modes = phy_info->link_modes & 0x7;
	}
	set_bit(DN200_PHY_SFP_INITED, &phy_info->phy_state);
	clear_bit(DN200_PHY_SFP_NEED_RESET, &phy_info->phy_state);
put:
	return 0;
}

static int dn200_xpcs_link_down_reg_dump(struct dn200_phy_info *phy_info)
{
	u8 max_len = DN200_MAX_PHY_DUMP_NUM;
	int i = 0;
	u16 reg_info;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (!phy_info->xpcs)
		return 0;

	for (; i < max_len; i++) {
		reg_info =
		    dn200_xpcs_phy_reg_read(phy_info,
					    xpcs_link_down_dump_regs[i].dev,
					    xpcs_link_down_dump_regs[i].reg);
		dev_info(priv->device,
			 "%s dev (%#06x) reg (%#06x) offset (%#06x) value (%#06x)\n",
			 xpcs_link_down_dump_regs[i].reg_str,
			 xpcs_link_down_dump_regs[i].dev,
			 xpcs_link_down_dump_regs[i].reg,
			 (xpcs_link_down_dump_regs[i].dev << 18) | (xpcs_link_down_dump_regs[i].reg << 2),
			 reg_info);
	}

	return 0;
}

int dn200_xpcs_config_eee(struct dn200_phy_info *phy_info, int mult_fact_100ns,
			  int enable)
{
	int ret;

	ret = dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS, VR_MII_EEE_MCTRL0);
	if (ret < 0)
		return ret;

	if (enable) {
		/* Enable EEE */
		ret = VR_MII_EEE_LTX_EN | VR_MII_EEE_LRX_EN |
		    VR_MII_EEE_TX_QUIET_EN | VR_MII_EEE_RX_QUIET_EN |
		    VR_MII_EEE_TX_EN_CTRL | VR_MII_EEE_RX_EN_CTRL |
		    mult_fact_100ns << VR_MII_EEE_MULT_FACT_100NS_SHIFT;
	} else {
		ret &= ~(VR_MII_EEE_LTX_EN | VR_MII_EEE_LRX_EN |
			 VR_MII_EEE_TX_QUIET_EN | VR_MII_EEE_RX_QUIET_EN |
			 VR_MII_EEE_TX_EN_CTRL | VR_MII_EEE_RX_EN_CTRL |
			 VR_MII_EEE_MULT_FACT_100NS);
	}

	ret =
	    dn200_xpcs_write(phy_info, 0, MDIO_MMD_PCS, VR_MII_EEE_MCTRL0, ret);
	if (ret < 0)
		return ret;

	ret = dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS, VR_MII_EEE_MCTRL1);
	if (ret < 0)
		return ret;

	if (enable)
		ret |= VR_MII_EEE_TRN_LPI;
	else
		ret &= ~VR_MII_EEE_TRN_LPI;

	return dn200_xpcs_write(phy_info, 0, MDIO_MMD_PCS, VR_MII_EEE_MCTRL1,
				ret);
}

static int dn200_ethtool_get_eee(struct dn200_phy_info *phy_info,
				 struct ethtool_eee *data)
{
	int val;

	if (phy_info->phydev) {
		return phy_ethtool_get_eee(phy_info->phydev, data);
	} else if (phy_info->xpcs) {
		/* Get Supported EEE */
		val =
		    dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS,
				    MDIO_PCS_EEE_ABLE);
		if (val < 0)
			return val;
		data->supported = mmd_eee_cap_to_ethtool_sup_t(val);

		/* Get advertisement EEE */
		val =
		    dn200_xpcs_read(phy_info, 0, MDIO_MMD_AN, MDIO_AN_EEE_ADV);
		if (val < 0)
			return val;
		data->advertised = mmd_eee_adv_to_ethtool_adv_t(val);
		data->eee_enabled = !!data->advertised;

		/* Get LP advertisement EEE */
		val =
		    dn200_xpcs_read(phy_info, 0, MDIO_MMD_AN,
				    MDIO_AN_EEE_LPABLE);
		if (val < 0)
			return val;
		data->lp_advertised = mmd_eee_adv_to_ethtool_adv_t(val);

		data->eee_active = !!(data->advertised & data->lp_advertised);

		return 0;
	}
	return 0;
}
static int dn200_xpcs_an_config(struct dn200_phy_info *phy_info);
static int dn200_ethtool_set_eee(struct dn200_phy_info *phy_info,
				 struct ethtool_eee *data)
{
	int cap, old_adv, adv = 0, ret;

	if (phy_info->phydev) {
		return phy_ethtool_set_eee(phy_info->phydev, data);
	} else if (phy_info->xpcs) {
		/* Get Supported EEE */
		cap =
		    dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS,
				    MDIO_PCS_EEE_ABLE);
		if (cap < 0)
			return cap;

		old_adv =
		    dn200_xpcs_read(phy_info, 0, MDIO_MMD_AN, MDIO_AN_EEE_ADV);
		if (old_adv < 0)
			return old_adv;

		if (data->eee_enabled) {
			adv = !data->advertised ? cap :
			    ethtool_adv_to_mmd_eee_adv_t(data->advertised) & cap;
			/* Mask prohibited EEE modes */
			adv &= ~phy_info->eee_broken_modes;
		}
		if (old_adv != adv) {
			ret =
			    dn200_xpcs_write(phy_info, 0, MDIO_MMD_AN,
					     MDIO_AN_EEE_ADV, adv);
			if (ret < 0)
				return ret;

			/* Restart autonegotiation so the new modes get sent to the
			 * link partner.
			 */
			if (phy_info->an == DN200_AN_ENABLE) {
				ret = dn200_xpcs_an_config(phy_info);
				if (ret < 0)
					return ret;
			}
		}

		return 0;
	}
	return 0;
}
static void mmd_eee_adv_to_linkmode(unsigned long *advertising, u16 eee_adv)
{
	linkmode_zero(advertising);

	if (eee_adv & MDIO_EEE_100TX)
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,
				 advertising);
	if (eee_adv & MDIO_EEE_1000T)
		linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
				 advertising);
	if (eee_adv & MDIO_EEE_10GT)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10000baseT_Full_BIT,
				 advertising);
	if (eee_adv & MDIO_EEE_1000KX)
		linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseKX_Full_BIT,
				 advertising);
	if (eee_adv & MDIO_EEE_10GKX4)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10000baseKX4_Full_BIT,
				 advertising);
	if (eee_adv & MDIO_EEE_10GKR)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
				 advertising);
}

bool phy_check_valid(int speed, int duplex, unsigned long *features)
{
	return !!phy_lookup_setting(speed, duplex, features, true);
}

static int dn200_phy_init_eee(struct dn200_phy_info *phy_info,
			      bool clk_stop_enable)
{
	if (phy_info->phydev) {
		return phy_init_eee(phy_info->phydev, clk_stop_enable);
	} else if (phy_info->xpcs) {
		/* According to 802.3az,the EEE is supported only in full duplex-mode.
		 */
		if (phy_info->dup == DN200_DUP_FULL) {
			__ETHTOOL_DECLARE_LINK_MODE_MASK(common);
			__ETHTOOL_DECLARE_LINK_MODE_MASK(lp);
			__ETHTOOL_DECLARE_LINK_MODE_MASK(adv);
			int eee_lp, eee_cap, eee_adv;
			int status;
			u32 cap;

			/* Read phy status to properly get the right settings */
			//status = phy_read_status(phydev);
			//if (status)
			//      return status;

			/* First check if the EEE ability is supported */
			eee_cap =
			    dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS,
					    MDIO_PCS_EEE_ABLE);
			if (eee_cap <= 0)
				goto eee_exit_err;

			cap = mmd_eee_cap_to_ethtool_sup_t(eee_cap);
			if (!cap)
				goto eee_exit_err;

			/* Check which link settings negotiated and verify it in
			 * the EEE advertising registers.
			 */
			eee_lp =
			    dn200_xpcs_read(phy_info, 0, MDIO_MMD_AN,
					    MDIO_AN_EEE_LPABLE);
			if (eee_lp <= 0)
				goto eee_exit_err;

			eee_adv =
			    dn200_xpcs_read(phy_info, 0, MDIO_MMD_AN,
					    MDIO_AN_EEE_ADV);
			if (eee_adv <= 0)
				goto eee_exit_err;

			mmd_eee_adv_to_linkmode(adv, eee_adv);
			mmd_eee_adv_to_linkmode(lp, eee_lp);
			dn200_linkmode_and(common, adv, lp);
			if (!phy_check_valid
			    (phy_info->speed, phy_info->dup, common))
				goto eee_exit_err;
			if (clk_stop_enable) {
				/* Configure the PHY to stop receiving xMII
				 * clock while it is signaling LPI.
				 */
				status =
				    dn200_xpcs_read(phy_info, 0, MDIO_MMD_PCS,
						    MDIO_CTRL1);
				dn200_xpcs_write(phy_info, 0, MDIO_MMD_PCS,
						 MDIO_CTRL1,
						 status |
						 MDIO_PCS_CTRL1_CLKSTOP_EN);
			}
			return 0;	/* EEE supported */
		}
	}
eee_exit_err:
	return -EPROTONOSUPPORT;
}

static int dn200_get_link_ksettings(struct net_device *netdev,
				    struct ethtool_link_ksettings *cmd)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	struct ethtool_link_ksettings *lks = &PRIV_PHY_INFO(priv)->lks;

	DN200_LM_COPY(cmd, supported, lks, supported);
	DN200_LM_COPY(cmd, advertising, lks, advertising);
	if (phy_info->an && phy_info->an_sucess)
		DN200_LM_COPY(cmd, lp_advertising, lks, lp_advertising);
	if (phy_info->link_status) {
		cmd->base.speed = phy_info->speed;
		cmd->base.duplex = phy_info->dup;
	} else {
		/* With no link speed and duplex are unknown */
		cmd->base.speed = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}
	if (phy_info->phydev) {
		extern_phy_mdix_status_get(phy_info->phydev,
			&cmd->base.eth_tp_mdix, &cmd->base.eth_tp_mdix_ctrl);
	}
	cmd->base.phy_address = phy_info->phy_addr;
	cmd->base.autoneg = phy_info->an;
	cmd->base.port = phy_info->port_type;
	return 0;
}

static void dn200_set_pcie_conf(struct dn200_phy_info *phy_info, u32 offset,
				u32 val)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	dev_dbg(priv->device, "offset %#x val %#x\n", offset, val);
	if (priv->speed_cmd)
		fw_reg_write(&priv->plat_ex->ctrl, 0x24300000 + offset, val);
	else
		writel(val, phy_info->xpcs->xpcs_regs_base +
		       DN200_PCIE_BAROFF + offset);
}

static void dn200_get_pcie_conf(struct dn200_phy_info *phy_info, u32 offset,
				u32 *val)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (priv->speed_cmd)
		fw_reg_read(&priv->plat_ex->ctrl, 0x24300000 + offset, val);
	else
		*val = readl(phy_info->xpcs->xpcs_regs_base +
			  DN200_PCIE_BAROFF + offset);
}

static void dn200_conf_pcie_common_para(struct dn200_phy_info *phy_info)
{
	u32 reg_val;

	dn200_set_pcie_conf(phy_info, 0x1c90, 0x1);
	dn200_set_pcie_conf(phy_info, 0x1c60, 0x96);
	dn200_set_pcie_conf(phy_info, 0x1c80, 0x1);
	dn200_set_pcie_conf(phy_info, 0x1c88, 0x2);
	dn200_get_pcie_conf(phy_info, 0x24e8, &reg_val);
	reg_val |= BIT(8);
	reg_val &= ~(GENMASK(14, 10));
	reg_val |= (0x7 << 10);
	dn200_set_pcie_conf(phy_info, 0x24e8, reg_val);
	dn200_set_pcie_conf(phy_info, 0x1c20, 0x4b);
	dn200_set_pcie_conf(phy_info, 0x1c18, 0x63f);
	dn200_set_pcie_conf(phy_info, 0x1c10, 0x63f);
	dn200_set_pcie_conf(phy_info, 0x181c, 0x3);
	dn200_set_pcie_conf(phy_info, 0x2180, 0x5);
	dn200_get_pcie_conf(phy_info, 0x1b90, &reg_val);
	reg_val |= BIT(6) | BIT(7);
	dn200_set_pcie_conf(phy_info, 0x1b90, reg_val);
}

static void dn200_conf_pcie_10G_tx_para(struct dn200_phy_info *phy_info)
{
	dn200_set_pcie_conf(phy_info, 0x21c0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x0);
	dn200_set_pcie_conf(phy_info, 0x2190 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x8);
	dn200_set_pcie_conf(phy_info, 0x2198 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x8);
	dn200_set_pcie_conf(phy_info, 0x21e8 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4f);
	dn200_set_pcie_conf(phy_info, 0x21f0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4f);
	dn200_set_pcie_conf(phy_info, 0x2220 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4);
	dn200_set_pcie_conf(phy_info, 0x2238 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x3);
	dn200_set_pcie_conf(phy_info, 0x2240 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x2);
	dn200_set_pcie_conf(phy_info, 0x2218 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x21d0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x21f8 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x2108 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x6);
	dn200_set_pcie_conf(phy_info, 0x2228 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x5);
	dn200_set_pcie_conf(phy_info, 0x2208 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0xb);
	dn200_set_pcie_conf(phy_info, 0x2210 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4);
	dn200_set_pcie_conf(phy_info, 0x2248 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x2);
}

static void dn200_conf_pcie_10G_rx_para(struct dn200_phy_info *phy_info)
{
	dn200_set_pcie_conf(phy_info, 0x1d58 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x13);


	/*eq_ctle_boost */
	dn200_set_pcie_conf(phy_info, 0x1dc0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x10);
	/*eq_ctle_pole */
	dn200_set_pcie_conf(phy_info, 0x1dc8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	/*eq_afe_rate */
	dn200_set_pcie_conf(phy_info, 0x1db0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x6);
	/*eq_vga_gain */
	dn200_set_pcie_conf(phy_info, 0x1de8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x10);
	/*eq_afe_config */
	dn200_set_pcie_conf(phy_info, 0x1da8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x514);
	dn200_set_pcie_conf(phy_info, 0x1dd8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xc);
	dn200_set_pcie_conf(phy_info, 0x1de0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x80);
	dn200_set_pcie_conf(phy_info, 0x1d78 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x6);
	dn200_set_pcie_conf(phy_info, 0x1d60 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x403);
	dn200_set_pcie_conf(phy_info, 0x1d70 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xb);
	dn200_set_pcie_conf(phy_info, 0x1d68 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xb);
	dn200_set_pcie_conf(phy_info, 0x1e20 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x4);
	dn200_set_pcie_conf(phy_info, 0x1e18 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x2);
	dn200_set_pcie_conf(phy_info, 0x1df0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);
	dn200_set_pcie_conf(phy_info, 0x1e30 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x1e38 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x2);
	dn200_set_pcie_conf(phy_info, 0x1e00 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x1df8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x1d40 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x1d90 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);
	dn200_set_pcie_conf(phy_info, 0x1d48 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x4);
	/*rx_dfe_bypass */
	dn200_set_pcie_conf(phy_info, 0x1d80 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);
	/* rx_vco_ld_val=1650&&rx_ref_ld_val=20=>rx_clk */
	dn200_set_pcie_conf(phy_info, 0x1e08 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x14);
	dn200_set_pcie_conf(phy_info, 0x1e40 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x672);
}

static void dn200_conf_pcie_1G_tx_para(struct dn200_phy_info *phy_info)
{
	writel(0x81,
	       phy_info->xpcs->xpcs_regs_base +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(phy_info->xpcs_idx));
	/*tx_misc */
	dn200_set_pcie_conf(phy_info, 0x21c0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x80);/*tx*/

	dn200_set_pcie_conf(phy_info, 0x2190 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x8);/*tx*/
	dn200_set_pcie_conf(phy_info, 0x2198 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x8);/*tx*/
	/*cp_ctl_intg */
	dn200_set_pcie_conf(phy_info, 0x21e8 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x48);/*tx*/
	/*cp_ctl_prog */
	dn200_set_pcie_conf(phy_info, 0x21f0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x61);/*tx*/

	dn200_set_pcie_conf(phy_info, 0x2220 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4);/*tx*/

	dn200_set_pcie_conf(phy_info, 0x2238 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x3);/*tx*/
	/*vco_low_freq */
	dn200_set_pcie_conf(phy_info, 0x2240 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x3);/*tx*/
	/*postdiv */
	dn200_set_pcie_conf(phy_info, 0x2218 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x1);/*tx*/
	/*tx_rate */
	dn200_set_pcie_conf(phy_info, 0x21d0 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x4);/*tx*/
	/*div16P5_clk */
	dn200_set_pcie_conf(phy_info, 0x21f8 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x0);/*tx*/
	/*word_clk_freq */
	dn200_set_pcie_conf(phy_info, 0x2108 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x0);/*tx*/
	/*ropll_refdiv */
	dn200_set_pcie_conf(phy_info, 0x2228 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0xf);/*tx*/
	/*ropll_fbdiv */
	dn200_set_pcie_conf(phy_info, 0x2208 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x20);/*tx*/
	/*ropll_out_div */
	dn200_set_pcie_conf(phy_info, 0x2210 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x5);/*tx*/
	/*word_clk_div_sel */
	dn200_set_pcie_conf(phy_info, 0x2248 + (3 - phy_info->xpcs_idx) * 0xd0,
			    0x3);/*tx*/
	writel(0x87,
	       phy_info->xpcs->xpcs_regs_base +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(phy_info->xpcs_idx));
	udelay(100);
}

static void dn200_conf_pcie_1G_rx_para(struct dn200_phy_info *phy_info)
{
	writel(0x81,
	       phy_info->xpcs->xpcs_regs_base +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(phy_info->xpcs_idx));
	dn200_set_pcie_conf(phy_info, 0x1e08 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x11);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1e40 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x550);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1d58 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x13);/*rx*/

	/*eq_ctle_boost */
	dn200_set_pcie_conf(phy_info, 0x1dc0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xC);/*rx*/
	/*eq_ctle_pole */
	dn200_set_pcie_conf(phy_info, 0x1dc8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);/*rx*/
	/*eq_afe_rate */
	dn200_set_pcie_conf(phy_info, 0x1db0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x7);/*rx*/
	/*eq_vga_gain */
	dn200_set_pcie_conf(phy_info, 0x1de8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x14);/*rx*/
	/*eq_afe_config */
	dn200_set_pcie_conf(phy_info, 0x1da8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x514);/*rx*/
	/*afe_tap1 */
	dn200_set_pcie_conf(phy_info, 0x1dd8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1de0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x80);/*rx*/
	/*delta_iq */
	dn200_set_pcie_conf(phy_info, 0x1d78 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1d60 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x403);/*rx*/

	dn200_set_pcie_conf(phy_info, 0x1d70 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xb);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1d68 + (3 - phy_info->xpcs_idx) * 0x110,
			    0xb);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1e20 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x4);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1e18 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x2);/*rx*/
	/*rx_misc */
	dn200_set_pcie_conf(phy_info, 0x1df0 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x80);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1e38 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x2);/*rx*/
	/*rx_rate */
	dn200_set_pcie_conf(phy_info, 0x1e00 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x4);/*rx*/
	/*rx_dfe_bypass */
	dn200_set_pcie_conf(phy_info, 0x1d80 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1df8 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);/*rx*/
	dn200_set_pcie_conf(phy_info, 0x1d40 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x1);/*rx*/
	/*16P5_clk */
	dn200_set_pcie_conf(phy_info, 0x1d90 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);/*rx*/
	/*adapt_mode */
	dn200_set_pcie_conf(phy_info, 0x1d48 + (3 - phy_info->xpcs_idx) * 0x110,
			    0x0);/*rx*/
	writel(0x87,
	       phy_info->xpcs->xpcs_regs_base +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(phy_info->xpcs_idx));
	udelay(100);
}


static int  _phy_reg_read(struct dn200_phy_info *phy_info,
					u16 phy_reg_addr, u16 *reg_val);
int dn200_phy_clock_stable_judge(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;
	int clock_state = 0;
	int ret = 0;
	unsigned long out_time_start = 0;
	unsigned long in_time_start = 0;

	out_time_start = jiffies;
	while (true) {
		_phy_reg_read(phy_info, 0x1069 + (3 - phy_info->xpcs_idx) * 0x200, &reg_val);
		in_time_start = jiffies;
		if (reg_val & BIT(15)) {
			while (true) {
				if (time_after(jiffies, in_time_start + usecs_to_jiffies(2000))) {
					clock_state = 1;
					break;
				}
				_phy_reg_read(phy_info, 0x1069 + (3 - phy_info->xpcs_idx) * 0x200, &reg_val);
				if (!(reg_val & BIT(15))) {
					clock_state = 0;
					break;
				}
				usleep_range(10, 20);
			}
		}

		if (clock_state)
			break;
		if (time_after(jiffies, out_time_start + msecs_to_jiffies(2000))) {
			clock_state = 0;
			break;
		}
		usleep_range(10, 20);
	}

	if (!clock_state)
		ret = -1;
	return ret;
}

static int dn200_xpcs_switch_to_1G_tx_regset(struct dn200_phy_info *phy_info);
static int dn200_xpcs_switch_to_1G_rx_regset(struct dn200_phy_info *phy_info);
static int dn200_xpcs_switch_to_10G_rx_regset(struct dn200_phy_info *phy_info);
static int dn200_xpcs_switch_to_10G_tx_regset(struct dn200_phy_info *phy_info);
static int _phy_reg_write(struct dn200_phy_info *phy_info, u16 phy_reg_addr,
			  u16 reg_val);
static void dn200_xpcs_prepare_switch_speed(struct dn200_phy_info *phy_info)
{
	u32 reg_val;

	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL);
	reg_val |= TX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL, reg_val);
	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1);
	reg_val |= RX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1, reg_val);
	usleep_range(20, 30);
	_phy_reg_write(phy_info, 0x2036 + 0x200 * (3 - phy_info->xpcs_idx), 0x3);
	usleep_range(20, 30);
	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL);
	reg_val &= ~TX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL, reg_val);
	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1);
	reg_val &= ~RX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1, reg_val);
}

static void dn200_xpcs_switch_to_10G(struct dn200_phy_info *phy_info)
{
	u16 phy_val, phy_val1;
	int try = 0;

	dn200_phy_set_rs_mode(phy_info, true);
	dn200_xpcs_prepare_switch_speed(phy_info);
	/*wait tx rest done*/
	while (try++ < 100000) {
		_phy_reg_read(phy_info, 0x2034 + 0x200 * (3 - phy_info->xpcs_idx), &phy_val);
		if ((phy_val & BIT(0))) {
			netdev_dbg(phy_info->dev, "%s %d wait tx rest_done\n", __func__, __LINE__);
			break;
		}
		usleep_range(10, 20);
	};
	dn200_conf_pcie_10G_tx_para(phy_info);
	dn200_xpcs_switch_to_10G_tx_regset(phy_info);
	usleep_range(10, 20);
	_phy_reg_write(phy_info, 0x2036 + 0x200 * (3 - phy_info->xpcs_idx), 0x2);
	/*wait rx rest done*/
	while (try++ < 100000) {
		_phy_reg_read(phy_info, 0x2035 + 0x200 * (3 - phy_info->xpcs_idx), &phy_val1);
		if ((phy_val1 & BIT(0))) {
			netdev_dbg(phy_info->dev, "%s %d wait rx rest_done\n", __func__, __LINE__);
			break;
		}
		usleep_range(10, 20);
	};
	dn200_conf_pcie_10G_rx_para(phy_info);
	dn200_xpcs_switch_to_10G_rx_regset(phy_info);
	_phy_reg_write(phy_info, 0x2036 + 0x200 * (3 - phy_info->xpcs_idx), 0x0);
	usleep_range(10, 20);
	dn200_link_status_reset(phy_info, true);
}

static void dn200_xpcs_switch_to_1G(struct dn200_phy_info *phy_info)
{
	u16 phy_val, phy_val1;
	int try = 0;

	dn200_phy_set_rs_mode(phy_info, false);
	dn200_xpcs_prepare_switch_speed(phy_info);
	/*wait tx rest done*/
	while (try++ < 100000) {
		_phy_reg_read(phy_info, 0x2034 + 0x200 * (3 - phy_info->xpcs_idx), &phy_val);
		if ((phy_val & BIT(0))) {
			netdev_dbg(phy_info->dev, "%s %d wait tx rest_done\n", __func__, __LINE__);
			break;
		}
		usleep_range(10, 20);
	};
	dn200_conf_pcie_1G_tx_para(phy_info);
	dn200_xpcs_switch_to_1G_tx_regset(phy_info);
	usleep_range(10, 20);
	_phy_reg_write(phy_info, 0x2036 + 0x200 * (3 - phy_info->xpcs_idx), 0x2);
	/*wait rx rest done*/
	while (try++ < 100000) {
		_phy_reg_read(phy_info, 0x2035 + 0x200 * (3 - phy_info->xpcs_idx), &phy_val1);
		if ((phy_val1 & BIT(0))) {
			netdev_dbg(phy_info->dev, "%s %d wait rx rest_done\n", __func__, __LINE__);
			break;
		}
		usleep_range(10, 20);
	};
	dn200_conf_pcie_1G_rx_para(phy_info);
	dn200_xpcs_switch_to_1G_rx_regset(phy_info);
	_phy_reg_write(phy_info, 0x2036 + 0x200 * (3 - phy_info->xpcs_idx), 0x0);
	usleep_range(10, 20);
	dn200_link_status_reset(phy_info, true);
}

static int dn200_set_link_ksettings(struct net_device *netdev,
				    const struct ethtool_link_ksettings *cmd)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	struct ethtool_link_ksettings *lks = &PRIV_PHY_INFO(priv)->lks;
	enum an_state curr_an = phy_info->an;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(advertising);
	int ret;

	if (priv->plat_ex->has_xpcs) {
		if (cmd->base.speed != phy_info->speed &&
		    cmd->base.speed <= phy_info->max_speed &&
		    cmd->base.speed != phy_info->setting_speed) {
			if (cmd->base.speed == SPEED_10000) {
				phy_info->speed = cmd->base.speed;
				netdev_info(netdev, "succeeded to switch to %d\n",
					    cmd->base.speed);
			} else if (cmd->base.speed == SPEED_1000) {
				/*switch mac's speed and then switch xpcs's speed */
				netdev_info(netdev, "succeeded to switch to %d\n",
					    cmd->base.speed);
			} else {
				netdev_err(netdev, "unsupported speed %d\n",
					   cmd->base.speed);
				return -EINVAL;
			}
		}
		if (cmd->base.duplex != phy_info->dup) {
			netdev_info(netdev, "unsupported duplex %#x\n",
				   cmd->base.duplex);
		}
	}

	if (cmd->base.phy_address != phy_info->phy_addr) {
		netdev_err(netdev, "invalid phy address %#x\n",
			   cmd->base.phy_address);
		return -EINVAL;
	}

	if ((cmd->base.autoneg != AUTONEG_ENABLE) &&
	    (cmd->base.autoneg != AUTONEG_DISABLE)) {
		netdev_err(netdev, "unsupported autoneg %#x\n",
			   cmd->base.autoneg);
		return -EINVAL;
	}

	if (cmd->base.autoneg == AUTONEG_DISABLE) {
		if (!priv->mii && cmd->base.duplex != DUPLEX_FULL) {
			netdev_err(netdev, "unsupported duplex %#x\n",
				   cmd->base.duplex);
			return -EINVAL;
		}
	}

	netif_dbg(priv, link, netdev,
		  "requested advertisement 0x%*pb, phy supported 0x%*pb\n",
		  __ETHTOOL_LINK_MODE_MASK_NBITS, cmd->link_modes.advertising,
		  __ETHTOOL_LINK_MODE_MASK_NBITS, lks->link_modes.supported);

	bitmap_and(advertising,
		   cmd->link_modes.advertising, lks->link_modes.supported,
		   __ETHTOOL_LINK_MODE_MASK_NBITS);

	if ((cmd->base.autoneg == AUTONEG_ENABLE) &&
	    bitmap_empty(advertising, __ETHTOOL_LINK_MODE_MASK_NBITS)) {
		netdev_err(netdev, "unsupported requested advertisement\n");
		return -EINVAL;
	}

	if (cmd->base.port != phy_info->port_type) {
		netdev_err(netdev,
			   "unsupported port type %#x\n", cmd->base.port);
		return -EINVAL;
	}

	ret = 0;
	set_bit(DN200_PHY_IN_RESET, &phy_info->phy_state);
	phy_info->speed = cmd->base.speed;
	if (!priv->plat_ex->has_xpcs)
		phy_info->dup = cmd->base.duplex;
	bitmap_copy(lks->link_modes.advertising, advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);

	phy_info->setting_speed = cmd->base.speed;
	if (cmd->base.speed != phy_info->last_link_speed) {
		phy_info->cur_an = cmd->base.autoneg;
		if (phy_info->cur_an == AUTONEG_ENABLE)
			DN200_SET_ADV(lks, Autoneg);
		else
			DN200_CLR_ADV(lks, Autoneg);
		if (!priv->mii) {
			if (phy_info->phy_multispeed_work.work.func)
				cancel_delayed_work_sync(&phy_info->phy_multispeed_work);
			dn200_normal_reset(priv);
		} else
			clear_bit(DN200_PHY_IN_RESET, &phy_info->phy_state);
	} else {
		clear_bit(DN200_PHY_IN_RESET, &phy_info->phy_state);
		DN200_SET_SUP(lks, Autoneg);
		phy_info->cur_an = cmd->base.autoneg;
		if (phy_info->cur_an == AUTONEG_ENABLE)
			DN200_SET_ADV(lks, Autoneg);
		else
			DN200_CLR_ADV(lks, Autoneg);
	}
	if (curr_an != phy_info->cur_an && !phy_info->phydev) {
		if (phy_info->link_status) {
			phy_info->link_status = DN200_LINK_DOWN;
			dn200_phy_info_state_change(phy_info);
			dn200_phy_print_status(phy_info);
			netif_carrier_off(phy_info->dev);
			phy_info->sfp_rx_los = true;
		}
	}
	if (phy_info->phydev && cmd->base.eth_tp_mdix_ctrl) {
		extern_phy_mdix_status_set(phy_info->phydev,
			cmd->base.eth_tp_mdix_ctrl);
	}
	phy_info->an = phy_info->cur_an;
	if (netif_running(netdev) && phy_info->phydev)
		ret = phy_info->phy_ops->an_config(phy_info);

	return ret;
}

static int dn200_get_phy_pauseparam(struct dn200_phy_info *phy_info,
				    struct ethtool_pauseparam *pause)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (phy_info->phydev)
		pause->autoneg = priv->flow_ctrl_an;
	else
		pause->autoneg = phy_info->an;
	pause->rx_pause = !!(phy_info->pause & MLO_PAUSE_RX);
	pause->tx_pause = !!(phy_info->pause & MLO_PAUSE_TX);

	return 0;
}

static int dn200_set_phy_pauseparam(struct dn200_phy_info *phy_info,
				    struct ethtool_pauseparam *pause)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (phy_info->phydev) {
		if (!pause->autoneg) {
			phy_info->pause = 0;
			if (pause->tx_pause)
				phy_info->pause |= MLO_PAUSE_TX;
			if (pause->rx_pause)
				phy_info->pause |= MLO_PAUSE_RX;

			priv->flow_ctrl = phy_info->pause;
		} else
			priv->flow_ctrl = MLO_PAUSE_NONE;

		priv->flow_ctrl_an = pause->autoneg;
	} else {
		phy_info->pause = 0;
		if (pause->tx_pause)
			phy_info->pause |= MLO_PAUSE_TX;
		if (pause->rx_pause)
			phy_info->pause |= MLO_PAUSE_RX;

		if (pause->autoneg != phy_info->an) {
			netdev_info(phy_info->dev, "To change autoneg please use: ethtool -s <dev> autoneg <on|off>\n");
			return -EOPNOTSUPP;
		}
		priv->flow_ctrl = phy_info->pause;

		DN200_CLR_ADV(lks, Pause);
		DN200_CLR_ADV(lks, Asym_Pause);

		if (pause->rx_pause) {
			DN200_SET_ADV(lks, Pause);
			DN200_SET_ADV(lks, Asym_Pause);
		}

		if (pause->tx_pause) {
			/* Equivalent to XOR of Asym_Pause */
			if (DN200_ADV(lks, Asym_Pause))
				DN200_CLR_ADV(lks, Asym_Pause);
			else
				DN200_SET_ADV(lks, Asym_Pause);
		}
	}

	dn200_normal_reset(priv);

	return 0;
}

static int dn200_phy_loopback(struct dn200_phy_info *phy_info, bool enable)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	int ret = 0;
	u32 value = 0;
	u32 link_status = DN200_LINK_DOWN;

	if (PRIV_IS_VF(priv))
		return -EOPNOTSUPP;

	if (phy_info->phydev) {
		ret = phy_loopback(phy_info->phydev, enable);
	} else if (phy_info->xpcs) {
		value =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, MII_BMCR);
		if (enable)
			value |= PMA_CTRL1_LB;
		else
			value &= ~PMA_CTRL1_LB;
		dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
				 MII_BMCR, value);
		ret =
		    readl_poll_timeout(phy_info->phy_ops->link_status,
				       link_status, !(link_status & BIT(0)),
				       500, 1000000);
	}
	return ret;
}

static int dn200_phy_link_train_config(struct dn200_phy_info *phy_info);
static int dn200_nway_reset(struct dn200_phy_info *phy_info)
{
	int ret = -EOPNOTSUPP;

	if (phy_info->phydev)
		ret = phy_restart_aneg(phy_info->phydev);
	else {
		phy_info->link_status = DN200_LINK_DOWN;
		phy_info->an_sucess = false;
		ret = dn200_phy_link_train_config(phy_info);
	}
	return ret;
}

static void dn200_link_status_reset(struct dn200_phy_info *phy_info, bool enable)
{
	u32 reg_val = 0;
	int ret = 0;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S,
				   0);
	reg_val = readl(priv->ioaddr + XGE_TOP_CONFIG_OFFSET + 0x1c + phy_info->xpcs_idx * 0x50);
	reg_val &= ~(BIT(2) | BIT(1));
	writel(reg_val, priv->ioaddr + XGE_TOP_CONFIG_OFFSET + 0x1c + phy_info->xpcs_idx * 0x50);
	usleep_range(10, 15);
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			    VR_RX_GENCTRL0);
	reg_val &= ~RX_DT_EN_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_RX_GENCTRL0, reg_val);
	usleep_range(10, 15);
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			    VR_RX_GENCTRL0);
	reg_val |= RX_DT_EN_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL0, reg_val);
	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL);
	usleep_range(10, 15);
	reg_val |= TX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL, reg_val);
	usleep_range(10, 15);
	reg_val &= ~TX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_TX_GENCTRL, reg_val);
	usleep_range(10, 15);
	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1);
	reg_val |= RX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1, reg_val);
	usleep_range(10, 15);
	reg_val &= ~RX_RST_0;
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
						VR_RX_GENCTRL1, reg_val);
	usleep_range(10, 15);
	reg_val = readl(priv->ioaddr + XGE_TOP_CONFIG_OFFSET + 0x1c + phy_info->xpcs_idx * 0x50);
	reg_val |= (BIT(2) | BIT(1));
	writel(reg_val, priv->ioaddr + XGE_TOP_CONFIG_OFFSET + 0x1c + phy_info->xpcs_idx * 0x50);
	usleep_range(1000, 2000);
	dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT1);
	dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
	if (enable) {
		ret = dn200_phy_clock_stable_judge(phy_info);
		if (ret)
			usleep_range(10000, 20000);
	}
}

static void dn200_speed_set(struct dn200_phy_info *phy_info, u32 speed);

static void dn200_tx_xnp(struct dn200_phy_info *phy_info)
{
	u16 reg_val;

	/*send null page */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			 SR_AN_XNP_TX3, 0);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			 SR_AN_XNP_TX2, 0);
	reg_val = (BIT(13) | BIT(0));
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			 SR_AN_XNP_TX1, reg_val);
	usleep_range(100, 200);
}

static u16 dn200_an_rx_intr_get(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;
	unsigned long an_timer_start = 0;

	an_timer_start = jiffies;
	while (!time_after(jiffies, an_timer_start + msecs_to_jiffies(1000))) {
		reg_val =
			dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
							VR_AN_INTR);
		if (reg_val) {
			netdev_dbg(phy_info->dev, "%s recv an intr %#x\n",
						__func__, reg_val);
			/* Clear intr status and enable xpcs intr */
			dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
								MDIO_MMD_AN, VR_AN_INTR, GENMASK(2, 0),
								0, 0);
			return reg_val;
		}
		usleep_range(100, 200);
	}
	netdev_dbg(phy_info->dev,
				"%s lwait for AN_PG_RCV intr timeout\n", __func__);
	return 0;
}

#define DN200_MAX_AN_LINK_UP_TIME	(1000)
#define DN200_AN_LINK_UP_WAIT_INTR	(1)
#define DN200_AN_SUCCESS_LINK_UP_SUCESS	(DN200_MAX_AN_LINK_UP_TIME + 100)
static int dn200_rx_train_sw_process(struct dn200_phy_info *phy_info);
static void dn200_kr_train_disable(struct dn200_phy_info *phy_info);
static int dn200_10G_rx_train_set(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0, ad_reg = 0, ctrl_val = 0;
	unsigned long an_timer_start = 0;
	int ret = 0;

	if (phy_info->speed == SPEED_10000) {
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV2, GENMASK(15, 0), 0, 0x80);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_VEND2,
			   SR_MII_CTRL, AN_ENABLE, AN_ENABLE_SHIFT, 0);
	} else if (phy_info->speed == SPEED_1000) {
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV2, GENMASK(15, 0), 0, 0x20);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_VEND2,
			   SR_MII_CTRL, AN_ENABLE, AN_ENABLE_SHIFT, 1);
	}
	usleep_range(50000, 70000);
	an_timer_start = jiffies;
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			   VR_AN_INTR, GENMASK(2, 0), 0, 0);
	/* Enable C73 auto-negotiation */
	/*2: enable an */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S, 1);
	usleep_range(100, 200);
recv_intr:
	if (time_after(jiffies, an_timer_start + msecs_to_jiffies(2000))) {
		netdev_dbg(phy_info->dev, "%s an timeout received\n", __func__);
		goto disable_an;
	}
	reg_val = dn200_an_rx_intr_get(phy_info);
	if (reg_val & AN_PG_RCV) {
		netdev_dbg(phy_info->dev, "%s AN_PG_RCV received\n", __func__);
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL2);
		if (!(reg_val & BIT(7))) {
			netdev_dbg(phy_info->dev,
				   "%s link partner does not support 10G KR %#x\n",
				   __func__, reg_val);
			goto disable_an;
		}
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL3);
		if ((reg_val & GENMASK(15, 14)) != GENMASK(15, 14)) {
			netdev_dbg(phy_info->dev,
				   "%s link partner does not support 10G fec %#x\n",
				   __func__, reg_val);
			goto disable_an;
		}
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL1);
		ad_reg =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_ADV1);
		if ((reg_val & ADVERTISE_NPAGE) || (ad_reg & ADVERTISE_NPAGE)) {
			netdev_dbg(phy_info->dev,
				   "%s link partner need np %#x\n", __func__,
				   reg_val);
			dn200_tx_xnp(phy_info);
		} else {
			goto link_train;
		}
XNP_RECV:
		reg_val = dn200_an_rx_intr_get(phy_info);
		if (reg_val & AN_PG_RCV) {
			reg_val =
			    dn200_xpcs_read(phy_info, phy_info->phy_addr,
					    MDIO_MMD_AN, AN_LP_XNP_ABL1);
			ad_reg =
			    dn200_xpcs_read(phy_info, phy_info->phy_addr,
					    MDIO_MMD_AN, SR_AN_XNP_TX1);
			if ((reg_val & AN_ADV_NP) || (ad_reg & AN_ADV_NP)) {
				netdev_dbg(phy_info->dev,
					   "%s link partner need np %#x\n",
					   __func__, reg_val);
				dn200_tx_xnp(phy_info);
				goto XNP_RECV;
			} else {
				reg_val = 0;
				goto link_train;
			}
		} else {
			if (!time_after(jiffies, an_timer_start + msecs_to_jiffies(2000)))
				goto XNP_RECV;
			goto disable_an;
		}

link_train:
		ctrl_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_FEC_CTRL);
		ctrl_val &=
		    ~(MDIO_PMA_10GBR_FECABLE_ABLE |
		      MDIO_PMA_10GBR_FECABLE_ERRABLE);
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL3);
		ad_reg =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_ADV3);
		if ((reg_val & (KR10G_FEC_ABL | KR10G_FEC_REQ))
		    && (ad_reg & (KR10G_FEC_ABL | KR10G_FEC_REQ))) {
			netdev_dbg(phy_info->dev,
				   "%s link partner support fec %#x local %#x\n",
				   __func__, reg_val, ad_reg);
			ctrl_val |= (MDIO_PMA_10GBR_FECABLE_ABLE);
		}
		if (phy_info->speed == SPEED_10000) {
			dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
					SR_PMA_KR_FEC_CTRL, ctrl_val);
			ret = dn200_rx_train_sw_process(phy_info);
			usleep_range(20000, 30000);
		}
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_RX_GENCTRL0,
				   RX_DT_EN_0_MASK, RX_DT_EN_0_S, 1);
		usleep_range(2000, 3000);
		if (ret < 0)
			goto disable_an;

		/* Wait for link up */
		an_timer_start = jiffies;
		while (true) {
			if (phy_info->speed == SPEED_10000) {
				reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS,
						MDIO_PCS_10GBRT_STAT1);
				if (reg_val & XPCS_10G_PLU) {
					netdev_dbg(phy_info->dev,
						"%s link up as 10G speed mode\n",
						__func__);
					break;
				}
			} else if (phy_info->speed == SPEED_1000) {
				reg_val =
				    dn200_phy_read(phy_info, MDIO_MMD_PCS,
						   XS_PCS_LSTS);
				if (reg_val & MDIO_PHYXS_LNSTAT_ALIGN) {
					netdev_dbg(phy_info->dev,
						"%s link up as 1G speed mode\n",
						__func__);
					break;
				}
			}
			if (time_after(jiffies, an_timer_start + msecs_to_jiffies(DN200_MAX_AN_LINK_UP_TIME))) {
				netdev_dbg(phy_info->dev,
					"%s wait for link up timeout\n", __func__);
				goto disable_an;
			}
			usleep_range(DN200_AN_LINK_UP_WAIT_INTR * 1000, (DN200_AN_LINK_UP_WAIT_INTR + 1) * 1000);
		}
		/* Waiting for AN_INT_CMPLT */
		while (true) {
			reg_val = dn200_phy_read(phy_info, MDIO_MMD_AN, VR_AN_INTR);
			if (reg_val & BIT(0)) {
				netdev_dbg(phy_info->dev, "AN_INT_CMPLT received\n");
				break;
			}
			if (time_after(jiffies, an_timer_start + msecs_to_jiffies(DN200_MAX_AN_LINK_UP_TIME))) {
				netdev_dbg(phy_info->dev,
				    "%s wait for AN_INT_CMPLT received timeout\n", __func__);
				goto disable_an;
			}
			usleep_range(DN200_AN_LINK_UP_WAIT_INTR * 1000, (DN200_AN_LINK_UP_WAIT_INTR + 1) * 1000);
		}
		/*read clear link remote fault */
		dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
		/*wait 500ms for phy clock reset complete */
		usleep_range(DN200_AN_SUCCESS_LINK_UP_SUCESS * 1000, (DN200_AN_SUCCESS_LINK_UP_SUCESS + 1) * 1000);
		if (phy_info->speed == SPEED_10000) {
			reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS,
					MDIO_PCS_10GBRT_STAT1);
			if (!(reg_val & XPCS_10G_PLU)) {
				netdev_dbg(phy_info->dev,
						"%s link up as %d speed mode failed!\n",
						__func__, phy_info->speed);
				goto disable_an;
			}
		} else if (phy_info->speed == SPEED_1000) {
			reg_val =
				dn200_phy_read(phy_info, MDIO_MMD_PCS,
						XS_PCS_LSTS);
			if (!(reg_val & MDIO_PHYXS_LNSTAT_ALIGN)) {
				netdev_dbg(phy_info->dev,
						"%s link up as %d speed mode failed!\n",
						__func__, phy_info->speed);
				goto disable_an;
			}
		}
		phy_info->an_sucess = true;
		// phy_info->speed = SPEED_10000;
	} else if (reg_val & AN_INC_LINK) {
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL2);
		if ((reg_val & BIT(7))) {
			netdev_dbg(phy_info->dev,
				   "%s link partner support 10G KR %#x\n",
				   __func__, reg_val);
			/* Clear intr status and enable xpcs intr */
			dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
					   MDIO_MMD_AN, VR_AN_INTR, GENMASK(2, 0),
					   0, 0);
			goto recv_intr;
		}
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    SR_AN_LP_ABL3);
		if ((reg_val & GENMASK(15, 14)) == GENMASK(15, 14)) {
			netdev_dbg(phy_info->dev,
				   "%s link partner support 10G FEC %#x\n",
				   __func__, reg_val);
			/* Clear intr status and enable xpcs intr */
			dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
					   MDIO_MMD_AN, VR_AN_INTR, GENMASK(2, 0),
					   0, 0);
			goto recv_intr;
		}
		netdev_dbg(phy_info->dev,
			   "%s  AN_INC_LINK received and  link partner does not support 10GKR speed mode\n",
			   __func__);
		goto disable_an;
	} else if (reg_val & AN_INT_CMPLT) {
		netdev_dbg(phy_info->dev,
			   "%s  AN_INT_CMPLT received and continues to work in 10GKR speed mode\n",
			   __func__);
		goto disable_an;
	} else {
		goto disable_an;
	}
	return 0;
disable_an:
	phy_info->an_sucess = false;
	// phy_info->speed = phy_info->setting_speed;
	dn200_kr_train_disable(phy_info);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_RX_GENCTRL0, RX_DT_EN_0_MASK, RX_DT_EN_0_S, 1);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S, 0);
	/* reset link status for AN failure caused by "block lock fail" */
	dn200_link_status_reset(phy_info, false);
	phy_info->speed_reset_time = jiffies + DN200_SFP_RESET_TIME;
	return 0;
}

static int dn200_rx_eq_process(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0, ctrl_val = 0;
	u16 cff_upd0 = 0, cff_upd1 = 0, cff_updtm1 = 0;
	int retry = 1000;

	/*rx_ad_req start */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_XS_PMA_MP_32G_RX_EQ_CTRL4, RX_AD_REQ, RX_AD_REQ_S,
			   1);
	usleep_range(10, 20);

	while (retry-- > 0) {
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD,
				    VR_XS_PMA_MP_12G_16G_25G_MISC_STS);
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, VR_PMA_PHY_RX_EQ_CEU);
		ctrl_val = 0;
		if ((reg_val & CFF_UPDT_VLD_ALL) != 0) {
			netdev_dbg(phy_info->dev,
				   "%s %d VR_PMA_PHY_RX_EQ_CEU %#x\n", __func__,
				   __LINE__, reg_val);
			if (reg_val & CFF_UPDT1_VLD) {
				cff_upd1 = ((reg_val & CFF_UPDT1) >> 4);
				if (cff_upd1 == 0x2)
					cff_upd1 = 0;
				ctrl_val |= CFF_UPDT1_VLD;
				ctrl_val |= (cff_upd1 << 4);
			}
			if (reg_val & CFF_UPDT0_VLD) {
				cff_upd0 = ((reg_val & CFF_UPDT0) >> 2);
				if (cff_upd0 == 0x2)
					cff_upd0 = 0;
				ctrl_val |= CFF_UPDT0_VLD;
				ctrl_val |= (cff_upd0 << 2);
			}
			if (reg_val & CFF_UPDTM1_VLD) {
				cff_updtm1 = (reg_val & CFF_UPDTM1);
				if (cff_updtm1 == 0x2)
					cff_updtm1 = 0;
				ctrl_val |= CFF_UPDTM1_VLD;
				ctrl_val |= (cff_updtm1);
			}

			reg_val =
			    dn200_xpcs_read(phy_info, phy_info->phy_addr,
					    MDIO_MMD_PMAPMD, SR_PMA_KR_PMD_STS);
			if (!(reg_val & BIT(0))) {
				reg_val =
				    dn200_xpcs_read(phy_info,
						    phy_info->phy_addr,
						    MDIO_MMD_PMAPMD,
						    VR_PMA_KRTR_RX_EQ_CTRL);
				reg_val &= ~(GENMASK(5, 0));
				reg_val |= ctrl_val;
				netdev_dbg(phy_info->dev,
					   "RX: %d c+1(%d) c0(%d) c-1(%d)\n",
					   __LINE__,
					   (u16) (reg_val & GENMASK(5, 4)) >> 4,
					   (u16) (reg_val & GENMASK(3, 2)) >> 2,
					   (u16) (reg_val & GENMASK(1, 0)));
				dn200_xpcs_write(phy_info, phy_info->phy_addr,
						 MDIO_MMD_PMAPMD,
						 VR_PMA_KRTR_RX_EQ_CTRL,
						 reg_val);
				phy_info->rx_eq_states = RX_EQ_WAIT_UPDATE;
			} else {
				netdev_dbg(phy_info->dev,
					   "%s %d SR_PMA_KR_PMD_STS %#x LD already ready\n",
					   __func__, __LINE__, reg_val);
				phy_info->rx_eq_states = RX_EQ_LD_NOCMD;
			}
			dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
					   MDIO_MMD_PMAPMD,
					   VR_XS_PMA_MP_32G_RX_EQ_CTRL4,
					   RX_AD_REQ, RX_AD_REQ_S, 0);
			return 0;
		}
		usleep_range(10, 20);
	}
	if (retry <= 0)
		phy_info->rx_eq_states = RX_EQ_LD_NOCMD;
	return 0;
}

static const char *dn200_rxeq_state_to_string(struct dn200_phy_info *phy_info)
{
	switch (phy_info->rx_eq_states) {
	case RX_EQ_NONE:
		return "RX_EQ_NONE";
	case RX_EQ_WAIT_UPDATE:
		return "RX_EQ_WAIT_UPDATE";
	case RX_EQ_SEND_HOLD:
		return "RX_EQ_SEND_HOLD";
	case RX_EQ_WAIT_NOTUPDATE:
		return "RX_EQ_WAIT_NOTUPDATE";
	case RX_EQ_POLL_COF:
		return "RX_EQ_POLL_COF";
	case RX_EQ_READY:
		return "RX_EQ_READY";
	case RX_EQ_LD_NOCMD:
		return "RX_EQ_LD_NOCMD";
	default:
		return "error state";
	}
	return "error state";
}

static const char *dn200_txeq_state_to_string(struct dn200_phy_info *phy_info)
{
	switch (phy_info->tx_eq_states) {
	case TX_EQ_NONE:
		return "TX_EQ_NONE";
	case TX_EQ_POLL_LP_CMD:
		return "TX_EQ_POLL_LP_CMD";
	case TX_EQ_WAIT_HOLD_CMD:
		return "TX_EQ_WAIT_HOLD_CMD";
	case TX_EQ_WAIT_LD_VLD:
		return "TX_EQ_WAIT_LD_VLD";
	case TX_EQ_WAIT_LD_INVLD:
		return "TX_EQ_WAIT_LD_INVLD";
	case TX_EQ_LP_RDY:
		return "TX_EQ_LP_RDY";
	default:
		return "error state";
	}
	return "error state";
}

static int dn200_tx_eq_state_mach(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0, ctrl_val = 0;
	u16 cff_upd0 = 0, cff_upd1 = 0, cff_updtm1 = 0;

	netdev_dbg(phy_info->dev, "NOW tx_eq_states(%d): %s\n",
		   phy_info->tx_eq_states,
		   dn200_txeq_state_to_string(phy_info));
	switch (phy_info->tx_eq_states) {
	case TX_EQ_NONE:
		break;
	case TX_EQ_POLL_LP_CMD:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_LP_CESTS);
		if (reg_val & LP_RR) {
			netdev_dbg(phy_info->dev, "TX: recv lp rx_rdy %x\n",
				   reg_val);
			phy_info->tx_eq_states = TX_EQ_LP_RDY;
			break;
		}
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_LP_CEU);
		if ((reg_val & LP_PRST) || (reg_val & LP_INIT)
		    || (reg_val & GENMASK(5, 0))) {
			ctrl_val =
			    ((reg_val & LP_PRST) | (reg_val & LP_INIT) |
			     (reg_val & GENMASK(5, 0)));
			netdev_dbg(phy_info->dev,
				   "TX: %d preset(%d) INIT(%d) c+1(%d) c0(%d) c-1(%d)\n",
				   __LINE__, !!(reg_val & LP_PRST),
				   !!(reg_val & LP_INIT),
				   (u16) (reg_val & GENMASK(5, 4)) >> 4,
				   (u16) (reg_val & GENMASK(3, 2)) >> 2,
				   (u16) (reg_val & GENMASK(1, 0)));
			dn200_xpcs_write(phy_info, phy_info->phy_addr,
					 MDIO_MMD_PMAPMD,
					 VR_PMA_KRTR_TX_EQ_CFF_CTRL, ctrl_val);
			phy_info->tx_eq_states = TX_EQ_WAIT_LD_VLD;
		}
		break;
	case TX_EQ_WAIT_LD_VLD:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, VR_PMA_PHY_TX_EQ_STS);
		if (reg_val & CFF_UPDT_VLD_ALL) {
			ctrl_val = 0;
			if (reg_val & CFF_UPDT1_VLD) {
				cff_upd1 = ((reg_val & CFF_UPDT1) >> 4);
				if (cff_upd1 == 0x2)
					cff_upd1 = 0;
				ctrl_val |= (cff_upd1 << 4);
			}
			if (reg_val & CFF_UPDT0_VLD) {
				cff_upd0 = ((reg_val & CFF_UPDT0) >> 2);
				if (cff_upd0 == 0x2)
					cff_upd0 = 0;
				ctrl_val |= (cff_upd0 << 2);
			}
			if (reg_val & CFF_UPDTM1_VLD) {
				cff_updtm1 = (reg_val & CFF_UPDTM1);
				if (cff_updtm1 == 0x2)
					cff_updtm1 = 0;
				ctrl_val |= (cff_updtm1);
			}
			reg_val =
			    dn200_xpcs_read(phy_info, phy_info->phy_addr,
					    MDIO_MMD_PMAPMD,
					    VR_PMA_KRTR_TX_EQ_STS_CTRL);
			reg_val &= ~(GENMASK(5, 0));
			reg_val |= ctrl_val;
			netdev_dbg(phy_info->dev,
				   "TX: %d c+1(%d) c0(%d) c-1(%d) send update\n",
				   __LINE__,
				   (u16) (reg_val & GENMASK(5, 4)) >> 4,
				   (u16) (reg_val & GENMASK(3, 2)) >> 2,
				   (u16) (reg_val & GENMASK(1, 0)));
			dn200_xpcs_write(phy_info, phy_info->phy_addr,
					 MDIO_MMD_PMAPMD,
					 VR_PMA_KRTR_TX_EQ_STS_CTRL, reg_val);
			phy_info->tx_eq_states = TX_EQ_WAIT_HOLD_CMD;
		}
		break;
	case TX_EQ_WAIT_HOLD_CMD:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_LP_CEU);
		if (((reg_val & LP_PRST) | (reg_val & LP_INIT) |
		     (reg_val & GENMASK(5, 0))) == 0) {
			netdev_dbg(phy_info->dev,
				   "TX: %d SR_PMA_KR_LP_CEU %x recv hold done\n",
				   __LINE__, reg_val);
			dn200_xpcs_write(phy_info, phy_info->phy_addr,
					 MDIO_MMD_PMAPMD,
					 VR_PMA_KRTR_TX_EQ_CFF_CTRL, 0);
			phy_info->tx_eq_states = TX_EQ_WAIT_LD_INVLD;
		}
		break;
	case TX_EQ_WAIT_LD_INVLD:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, VR_PMA_PHY_TX_EQ_STS);
		if (!(reg_val & CFF_UPDT_VLD_ALL)) {
			netdev_dbg(phy_info->dev,
				   "TX: %d VR_PMA_PHY_TX_EQ_STS %x send notupdate\n",
				   __LINE__, reg_val);
			reg_val =
			    dn200_xpcs_read(phy_info, phy_info->phy_addr,
					    MDIO_MMD_PMAPMD,
					    VR_PMA_KRTR_TX_EQ_STS_CTRL);
			reg_val &= ~(GENMASK(5, 0));
			dn200_xpcs_write(phy_info, phy_info->phy_addr,
					 MDIO_MMD_PMAPMD,
					 VR_PMA_KRTR_TX_EQ_STS_CTRL, reg_val);
			phy_info->tx_eq_states = TX_EQ_POLL_LP_CMD;
		}
		break;
	case TX_EQ_LP_RDY:
		break;
	default:
		break;
	}
	return 0;
}

static int dn200_rx_eq_state_mach(struct dn200_phy_info *phy_info)
{
	u16 reg_val;

	netdev_dbg(phy_info->dev, "NOW rx_eq_states(%d): %s\n",
		   phy_info->rx_eq_states,
		   dn200_rxeq_state_to_string(phy_info));
	switch (phy_info->rx_eq_states) {
	case RX_EQ_NONE:
		/*program INT to LP */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_PMA_KRTR_RX_EQ_CTRL,
				   GENMASK(5, 0), 0, 0);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_PMA_KRTR_RX_EQ_CTRL,
				   GENMASK(6, 6), 6, 1);
		phy_info->rx_eq_states = RX_EQ_WAIT_UPDATE;
		netdev_dbg(phy_info->dev, "RX: send init\n");
		usleep_range(100, 200);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_PMA_KRTR_RX_EQ_CTRL,
				   GENMASK(6, 6), 6, 0);
		break;
	case RX_EQ_WAIT_UPDATE:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_LP_CESTS);
		/*wait LP update */
		if ((reg_val & GENMASK(5, 0)) == 0b010101) {
			netdev_dbg(phy_info->dev,
				   "RX: SR_PMA_KR_LP_CESTS %x update done\n",
				   reg_val);
			phy_info->rx_eq_states = RX_EQ_SEND_HOLD;
		}
		break;
	case RX_EQ_SEND_HOLD:
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_PMA_KRTR_RX_EQ_CTRL,
				   GENMASK(5, 0), 0, 0);
		netdev_dbg(phy_info->dev, "RX: send update\n");
		phy_info->rx_eq_states = RX_EQ_WAIT_NOTUPDATE;
		break;
	case RX_EQ_WAIT_NOTUPDATE:
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_LP_CESTS);
		/*wait LP not update */
		if ((reg_val & GENMASK(5, 0)) == 0) {
			netdev_dbg(phy_info->dev,
				   "RX: SR_PMA_KR_LP_CESTS %x NOTUPDATE done\n",
				   reg_val);
			phy_info->rx_eq_states = RX_EQ_POLL_COF;
		}
		break;
	case RX_EQ_POLL_COF:
		dn200_rx_eq_process(phy_info);
		break;
	case RX_EQ_LD_NOCMD:
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_PMA_KRTR_RX_EQ_CTRL,
				   GENMASK(8, 8), 8, 1);
		netdev_dbg(phy_info->dev, "RX:local set rr rdy\n");
		break;
	case RX_EQ_READY:
		break;
	default:
		netdev_info(phy_info->dev, "err state %d\n",
			    phy_info->rx_eq_states);
		return -EINVAL;
	}
	return 0;
}

static void dn200_kr_train_disable(struct dn200_phy_info *phy_info)
{
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_XS_PMA_MP_32G_RX_EQ_CTRL4, GENMASK(12, 12), 12,
			   0);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_RX_EQ_CTRL, GENMASK(8, 8), 8, 1);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_RX_EQ_CTRL, GENMASK(15, 15), 15, 0);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_TX_EQ_STS_CTRL, GENMASK(15, 15), 15, 0);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_XS_PMA_MP_32G_RX_EQ_CTRL4, GENMASK(10, 8), 8, 0);
	/*Enable train */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   SR_PMA_KR_PMD_CTRL, GENMASK(1, 1), 1, 0);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   SR_PMA_KR_PMD_CTRL, GENMASK(0, 0), 0, 1);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_VEND2,
			   SR_MII_CTRL, AN_ENABLE, AN_ENABLE_SHIFT, 0);
}

static int dn200_rx_train_sw_process(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;

	phy_info->rx_eq_states = RX_EQ_NONE;
	phy_info->tx_eq_states = TX_EQ_NONE;
	usleep_range(1000, 2000);
	netdev_dbg(phy_info->dev, "start sw kt process  %x\n", reg_val);
	/*RR RDY */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_RX_EQ_CTRL, RR_RDY, 8, 0);
	/*Enable MM */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_RX_EQ_CTRL, RX_EQ_MM, 15, 1);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_PMA_KRTR_TX_EQ_STS_CTRL, TX_EQ_MM, 15, 1);
	/*enable ping-pong mode */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_XS_PMA_MP_32G_RX_EQ_CTRL4, GENMASK(10, 8), 8,
			   0x7);
	/*Enable train */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   SR_PMA_KR_PMD_CTRL, TR_EN, TR_EN_S, 1);
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   SR_PMA_KR_PMD_CTRL, RS_TR, RS_TR_S, 1);
	phy_info->tr_timeout = jiffies + DN200_KT_TRAIN_TIME;
	usleep_range(10, 20);
	/*rx_ad_req start */
	dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			   VR_XS_PMA_MP_32G_RX_EQ_CTRL4, RX_AD_REQ, RX_AD_REQ_S,
			   1);
	usleep_range(10, 20);

	while (true) {
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr,
				    MDIO_MMD_PMAPMD, SR_PMA_KR_PMD_STS);
		if (reg_val & (BIT(3))) {
			netdev_dbg(phy_info->dev, "%s link train timeout\n",
				    __func__);
			dn200_kr_train_disable(phy_info);
			return -ETIMEDOUT;
		} else if (reg_val & BIT(0)) {
			netdev_dbg(phy_info->dev, "local recv ready %x\n",
				   reg_val);
			phy_info->rx_eq_states = RX_EQ_READY;
		}
		if (reg_val & BIT(1)) {
			if (phy_info->tx_eq_states == TX_EQ_NONE)
				phy_info->tx_eq_states = TX_EQ_POLL_LP_CMD;
		}
		if (time_after(phy_info->tr_timeout, jiffies)) {
			if (phy_info->tx_eq_states == TX_EQ_LP_RDY &&
			    phy_info->rx_eq_states == RX_EQ_READY) {
				netdev_dbg(phy_info->dev,
					    "link train success\n");
				return 0;
			}
			dn200_rx_eq_state_mach(phy_info);
			dn200_tx_eq_state_mach(phy_info);
			usleep_range(10, 20);
		} else {
			netdev_dbg(phy_info->dev, "%s link train timeout\n",
				    __func__);
			dn200_kr_train_disable(phy_info);
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int dn200_phy_link_train_config(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;

	if (phy_info->an) {
		dn200_xpcs_an_config(phy_info);
		reg_val =
			dn200_xpcs_read(phy_info, phy_info->phy_addr,
					MDIO_MMD_PMAPMD,
					SR_PMA_KR_FEC_CTRL);
		reg_val &=
			~(MDIO_PMA_10GBR_FECABLE_ABLE |
				MDIO_PMA_10GBR_FECABLE_ERRABLE);
		dn200_xpcs_write(phy_info, phy_info->phy_addr,
					MDIO_MMD_PMAPMD, SR_PMA_KR_FEC_CTRL,
					reg_val);
		queue_work(phy_info->dev_workqueue,
				&phy_info->kr_train_work);
	}
	return 0;
}

static void dn200_kr_train_work(struct work_struct *work)
{
	struct dn200_phy_info *phy_info = container_of(work,
						       struct dn200_phy_info,
						       kr_train_work);

	/*someone else is in init, wait until next timer event */
	if (test_and_set_bit(DN200_PHY_IN_TRAIN, &phy_info->phy_state))
		return;

	dn200_10G_rx_train_set(phy_info);

	clear_bit(DN200_PHY_IN_TRAIN, &phy_info->phy_state);
}
static int dn200_link_status_get(struct dn200_phy_info *phy_info)
{
	int ret = 0;
	u16 reg_val = 0;
	u16 block_err;

	if (phy_info->phydev) {
		/* Check external PHY */
		ret = phy_read_status(phy_info->phydev);
		if (ret < 0)
			return 0;

		if (phy_info->an == AUTONEG_ENABLE &&
		    !phy_aneg_done(phy_info->phydev))
			return 0;

		return phy_info->phydev->link;
	}
	if (phy_info->speed == SPEED_10000) {
		block_err =
		    dn200_phy_read(phy_info, MDIO_MMD_PCS,
				   MDIO_PCS_10GBRT_STAT2) & 0xFF;
		if (!phy_info->an_sucess)
			reg_val =
			    dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
		if ((block_err)
		    || (reg_val & (XS_PCS_STS2_RF | XS_PCS_STS2_TF))) {
			if (phy_info->blk_err_ck < DN200_MAX_BLK_ERR_CNT) {
				phy_info->blk_err_ck++;
			} else {
				if (block_err != 0) {
					netdev_dbg(phy_info->dev,
						   "Check high block err %x !!, Please check the optical\n",
						   block_err);
				}
				if (reg_val & (XS_PCS_STS2_RF | XS_PCS_STS2_TF)) {
					netdev_dbg(phy_info->dev,
						   "fault detect %x !!\n",
						   reg_val);
					if (phy_info->speed == SPEED_1000)
						dn200_xpcs_switch_to_1G(phy_info);
					else
						dn200_xpcs_switch_to_10G(phy_info);
					phy_info->blk_err_ck = false;
					goto LINK_DOWN;
				}
			}
		}
	}
	/* Link status is latched low, so read once to clear
	 * and then read again to get current state
	 */
	reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT1);
	if (!(reg_val & MDIO_STAT1_LSTATUS))
		goto LINK_DOWN;
	if (reg_val & MDIO_STAT1_LSTATUS) {
		if (phy_info->phy_interface == PHY_INTERFACE_MODE_XGMII) {
			if (phy_info->speed == SPEED_10000) {
				reg_val =
				    dn200_phy_read(phy_info, MDIO_MMD_PCS,
						   MDIO_PCS_10GBRT_STAT1);
				if (reg_val & XPCS_10G_PLU) {
					reg_val = 0;
					/*read Transmitter Fault and Receiver Fault,
					 * LH type read twice
					 */
					reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
					if (!(reg_val & (XS_PCS_STS2_RF | XS_PCS_STS2_TF)))
						return DN200_LINK_UP;
					netdev_dbg(phy_info->dev,
							"fault detect %x !!\n",
							reg_val);
				}
			}
			if (phy_info->speed == SPEED_1000) {
				reg_val =
				    dn200_phy_read(phy_info, MDIO_MMD_PCS,
						   XS_PCS_LSTS);
				if (reg_val & MDIO_PHYXS_LNSTAT_ALIGN) {
					/*read Receiver Fault, LH type read twice */
					reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
					if (!(reg_val & (XS_PCS_STS2_RF)))
						return DN200_LINK_UP;
				}
			}
		} else if (phy_info->phy_interface == PHY_INTERFACE_MODE_GMII) {
			reg_val =
			    dn200_phy_read(phy_info, MDIO_MMD_PCS, XS_PCS_LSTS);
			reg_val =
			    dn200_phy_read(phy_info, MDIO_MMD_PCS, XS_PCS_LSTS);
			if (reg_val & MDIO_PHYXS_LNSTAT_ALIGN) {
				/*read Receiver Fault, LH type read twice */
				reg_val = dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT2);
				if (!(reg_val & (XS_PCS_STS2_RF)))
					return DN200_LINK_UP;
			}
		}
	}
LINK_DOWN:
	/* if an is success, but link is down, close AN and reset phy */
	if (phy_info->an_sucess) {
		phy_info->an_sucess = false;
		phy_info->speed = phy_info->setting_speed;
		dn200_kr_train_disable(phy_info);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
				VR_RX_GENCTRL0, RX_DT_EN_0_MASK, RX_DT_EN_0_S, 1);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S, 0);
		/* reset link status for AN failure caused by "block lock fail" */
		if (phy_info->speed == SPEED_1000)
			dn200_xpcs_switch_to_1G(phy_info);
		else
			dn200_xpcs_switch_to_10G(phy_info);
	}
	return DN200_LINK_DOWN;
}

static int dn200_phy_xpcs_an73_result(struct dn200_phy_info *phy_info)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	//enum dn200_mode mode;
	unsigned int ad_reg, lp_reg, speed_modes;

	DN200_SET_LP_ADV(lks, Autoneg);
	DN200_SET_LP_ADV(lks, Backplane);

	/* Compare Advertisement and Link Partner register 1 */
	ad_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    MDIO_AN_ADVERTISE);
	lp_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    MDIO_AN_LPA);
	if (lp_reg & ADVERTISE_PAUSE_CAP)
		DN200_SET_LP_ADV(lks, Pause);
	if (lp_reg & ADVERTISE_PAUSE_ASYM)
		DN200_SET_LP_ADV(lks, Asym_Pause);

	if (phy_info->an) {
		/* Set flow control based on auto-negotiation result */
		phy_info->pause = MLO_PAUSE_NONE;
		if (ad_reg & lp_reg & ADVERTISE_PAUSE_CAP) {
			phy_info->pause |= MLO_PAUSE_TX;
			phy_info->pause |= MLO_PAUSE_RX;
		} else if (ad_reg & lp_reg & ADVERTISE_PAUSE_ASYM) {
			if (ad_reg & ADVERTISE_PAUSE_CAP)
				phy_info->pause |= MLO_PAUSE_RX;
			else if (lp_reg & ADVERTISE_PAUSE_CAP)
				phy_info->pause |= MLO_PAUSE_TX;
		}
		priv->flow_ctrl = phy_info->pause;
	}

	/* Compare Advertisement and Link Partner register 2 */
	ad_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    SR_AN_ADV2);
	lp_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    SR_AN_LP_ABL2);
	if (lp_reg & C73_10000KR)
		DN200_SET_LP_ADV(lks, 10000baseKR_Full);
	if (lp_reg & C73_1000KX)
		DN200_SET_LP_ADV(lks, 1000baseKX_Full);

	ad_reg &= lp_reg;
	if (ad_reg & C73_10000KR)
		speed_modes = SPEED_10000;
	else if (ad_reg & C73_1000KX)
		speed_modes = SPEED_1000;
	else
		speed_modes = SPEED_UNKNOWN;

	/* Compare Advertisement and Link Partner register 3 */
	ad_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    SR_AN_ADV3);
	lp_reg =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			    SR_AN_LP_ABL3);
	if (lp_reg & (C73_LP_FEC_EN | C73_NEED_FEC_EN))
		DN200_SET_LP_ADV(lks, 10000baseR_FEC);

	return speed_modes;
}

static const char *dn200_phy_fc_string(struct dn200_phy_info *phy_info)
{
	if ((phy_info->pause & MLO_PAUSE_TX) && (phy_info->pause & MLO_PAUSE_RX))
		return "rx/tx";
	else if (phy_info->pause & MLO_PAUSE_RX)
		return "rx";
	else if (phy_info->pause & MLO_PAUSE_TX)
		return "tx";
	else
		return "off";
}

static const char *dn200_phy_speed_string(int speed)
{
	switch (speed) {
	case SPEED_10:
		return "10Mbps";
	case SPEED_100:
		return "100Mbps";
	case SPEED_1000:
		return "1Gbps";
	case SPEED_2500:
		return "2.5Gbps";
	case SPEED_10000:
		return "10Gbps";
	case SPEED_UNKNOWN:
		return "Unknown";
	default:
		return "Unsupported";
	}
}

static void dn200_phy_print_status(struct dn200_phy_info *phy_info)
{
	if (phy_info->link_status)
		netdev_info(phy_info->dev,
			    "Link is Up - %s/%s - flow control %s\n",
			    dn200_phy_speed_string(phy_info->speed),
			    phy_info->dup == DN200_DUP_FULL ? "Full" : "Half",
			    dn200_phy_fc_string(phy_info));
	else
		netdev_info(phy_info->dev, "Link is Down\n");
}

static int dn200_phy_info_state_change(struct dn200_phy_info *phy_info)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	bool tx_pause, rx_pause;

	/*Reset link parnter advertising */
	DN200_ZERO_LP_ADV(lks);
	if (phy_info->phydev) {
		if (priv->flow_ctrl_an &&
			!extern_phy_pause_autoneg_result(phy_info->phydev, &tx_pause, &rx_pause)) {
			phy_info->pause &= ~(MLO_PAUSE_RX | MLO_PAUSE_TX);
			if (tx_pause)
				phy_info->pause |= MLO_PAUSE_TX;
			if (rx_pause)
				phy_info->pause |= MLO_PAUSE_RX;
		}
		bitmap_copy(lks->link_modes.lp_advertising,
			    phy_info->phydev->lp_advertising,
			    __ETHTOOL_LINK_MODE_MASK_NBITS);
		bitmap_copy(lks->link_modes.advertising,
			    phy_info->phydev->advertising,
			    __ETHTOOL_LINK_MODE_MASK_NBITS);
		priv->flow_ctrl = phy_info->pause;
		phy_info->speed = phy_info->phydev->speed;
		phy_info->dup = phy_info->phydev->duplex;
	} else {
		if (phy_info->an && phy_info->an_sucess)
			phy_info->speed = dn200_phy_xpcs_an73_result(phy_info);
		//phy_info->speed = SPEED_1000;
		phy_info->dup = DN200_DUP_FULL;
	}
	if (PRIV_SRIOV_SUPPORT(priv)) {
		struct dn200_sriov_phy_info sriov_info;

		sriov_info.speed = phy_info->speed;
		sriov_info.dup = phy_info->dup;
		sriov_info.media_type = phy_info->media_type;
		sriov_info.an = phy_info->an;
		sriov_info.pause = phy_info->pause;
		sriov_info.phy_interface = phy_info->phy_interface;
		sriov_info.link_modes = phy_info->link_modes;
		dn200_set_phy_info(((struct dn200_priv *)
				    netdev_priv(phy_info->dev))->hw,
				   &sriov_info);
	}

	return 0;
}

static void dn200_phy_speed_self_adapt(struct dn200_phy_info *phy_info)
{
	if (!phy_info->xpcs)
		return;
	/*link up, do not need to change speed */
	if (phy_info->link_status || phy_info->an_sucess)
		return;
	/*wait 2s to link at highest speed */
	if (time_after(phy_info->speed_reset_time, jiffies))
		return;
	phy_info->speed_reset_time = jiffies + DN200_SFP_RESET_TIME;
	/* firstly: old phy speed is 10G and sfp module support 10G, wait 1s for link at 10G
	 *secondly: old speed is 1G and sfp module support 10G, switch to 10G immediately
	 *than: old speed is 10G and sfp module support 1G, switch to 1G immediately
	 *lastly: old speed is 1G and sfp module support 1G, wait 1s for link at 1G
	 */
	if (phy_info->speed == SPEED_10000 &&
	    (phy_info->sfp_speed & DN200_SFP_SPEED_10000)) {
		/*delay 10s to start multispeed work */
		queue_delayed_work(phy_info->dev_workqueue,
				   &phy_info->phy_multispeed_work,
				   msecs_to_jiffies(2000));
	} else if ((phy_info->speed == SPEED_1000) &&
		   (phy_info->sfp_speed & DN200_SFP_SPEED_10000)) {
		/*delay 1ms to start multispeed work */
		queue_delayed_work(phy_info->dev_workqueue,
				   &phy_info->phy_multispeed_work,
				   msecs_to_jiffies(1));
	} else if ((phy_info->speed == SPEED_10000) &&
		   (phy_info->sfp_speed & DN200_SFP_SPEED_1000)) {
		/*delay 1ms to start multispeed work */
		queue_delayed_work(phy_info->dev_workqueue,
				   &phy_info->phy_multispeed_work,
				   msecs_to_jiffies(1));
	} else if ((phy_info->speed == SPEED_1000) &&
		   (phy_info->sfp_speed & DN200_SFP_SPEED_1000)) {
		/*delay 1ms to start multispeed work */
		queue_delayed_work(phy_info->dev_workqueue,
				   &phy_info->phy_multispeed_work,
				   msecs_to_jiffies(1000));
	}
}

static bool dn200_mac_dma_link_check(struct dn200_priv *priv, struct dn200_phy_info *phy_info)
{
	u32 reg_val = 0;
	u32 mtl_debug = 0, dma_debug = 0;
	int queue_count = 0;
	int i = 0;

	reg_val = readl(priv->ioaddr + XGMAC_MAC_DEBUG);

	if (reg_val) {
		netdev_dbg(phy_info->dev, "%s %d mac debug %#x\n", __func__, __LINE__, reg_val);
		if (reg_val & XGMAC_MAC_RX_FIFO_ACT) {
			/*check rx fifo and dma valid*/
			queue_count = priv->plat_ex->rx_queues_total;
			for (i = 0; i < queue_count; i++) {
				mtl_debug = readl(priv->ioaddr + XGMAC_MTL_RXQ_DEBUG(i));
				dma_debug = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(i));
				if (mtl_debug ||
					((dma_debug & XGMAC_RXDMA_FSM_STATE_MASK) != XGMAC_RXDMA_FSM_STATE &&
						dma_debug != 0)) {
					netdev_dbg(phy_info->dev, "%s %d queue %d mtl_debug %#x dma_debug %#x\n",
							__func__, __LINE__, i, mtl_debug, dma_debug);
					return false;
				}
			}
		}
		if (reg_val & XGMAC_MAC_TX_FIFO_ACT) {
			/*check rx fifo and dma valid*/
			queue_count = priv->plat_ex->tx_queues_total;
			for (i = 0; i < queue_count; i++) {
				mtl_debug = readl(priv->ioaddr + XGMAC_MTL_TXQ_DEBUG(i));
				dma_debug = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(i));
				if (mtl_debug ||
					((dma_debug & XGMAC_TXDMA_FSM_STATE_MASK) != XGMAC_TXDMA_FSM_STATE &&
						dma_debug != 0)) {
					netdev_dbg(phy_info->dev, "%s %d queue %d mtl_debug %#x dma_debug %#x\n",
							__func__, __LINE__, i, mtl_debug, dma_debug);
					if (!priv->mii)
						return false;
				}
			}
		}
		if (!phy_info->mac_debug_active) {
			phy_info->mac_debug_active = true;
		} else {
			dn200_global_err(priv, DN200_DMA_DEBUG_ERR);
			phy_info->mac_debug_active = false;
		}

		return false;
	}
	/*dma operation normal, can link up*/
	return true;
}

static void dn200_clear_lks(struct dn200_phy_info *phy_info, int type, bool is_sup)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;

	if (type & DN200_SFP_TYPE_10000) {
		if (is_sup) {
			DN200_CLR_SUP(lks, 10000baseKR_Full);
			DN200_CLR_SUP(lks, 10000baseCR_Full);
			DN200_CLR_SUP(lks, 10000baseLR_Full);
			DN200_CLR_SUP(lks, 10000baseER_Full);
			DN200_CLR_SUP(lks, 10000baseLRM_Full);
			DN200_CLR_SUP(lks, 10000baseSR_Full);
		} else {
			DN200_CLR_ADV(lks, 10000baseKR_Full);
			DN200_CLR_ADV(lks, 10000baseCR_Full);
			DN200_CLR_ADV(lks, 10000baseLR_Full);
			DN200_CLR_ADV(lks, 10000baseER_Full);
			DN200_CLR_ADV(lks, 10000baseLRM_Full);
			DN200_CLR_ADV(lks, 10000baseSR_Full);
		}
	}
	if (type & DN200_SFP_TYPE_1000) {
		if (is_sup) {
			DN200_CLR_SUP(lks, 1000baseT_Full);
			DN200_CLR_SUP(lks, 100baseT_Full);
			DN200_CLR_SUP(lks, 1000baseX_Full);
		} else {
			DN200_CLR_ADV(lks, 1000baseT_Full);
			DN200_CLR_ADV(lks, 100baseT_Full);
			DN200_CLR_ADV(lks, 1000baseX_Full);
		}
	}
}

static int dn200_phy_status(struct dn200_phy_info *phy_info)
{
	bool old_link = phy_info->link_status;
	bool link_status;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	u8 sfp_rx_los = phy_info->sfp_rx_los;
	struct ethtool_link_ksettings *lks = &phy_info->lks;

	if (!test_bit(DN200_PHY_STARTED, &phy_info->phy_state))
		return 0;
	if (test_bit(DN200_PHY_IN_RESET, &phy_info->phy_state))
		return 0;
	if (!(phy_info->dev->flags & IFF_UP))
		return 0;

	if (phy_info->phydev) {
		extern_phy_read_status(phy_info->phydev);
		link_status = phy_info->phydev->link;
		if (phy_info->phy_loopback_flag) {
			link_status = DN200_LINK_UP;
			phy_info->phydev->speed = SPEED_1000;
			phy_info->phydev->duplex = DUPLEX_FULL;
			if (phy_info->last_link_speed != phy_info->speed)
				old_link = DN200_LINK_DOWN;
		}
		phy_info->speed = phy_info->phydev->speed;
		phy_info->dup = phy_info->phydev->duplex;
		if (phy_info->self_adap_reset) {
			phy_info->self_adap_reset = false;
			link_status = DN200_LINK_DOWN;
		}
	} else {
		if (phy_info->self_adap_reset)
			return 0;
		dn200_phy_sfp_present(phy_info);
		dn200_phy_sfp_rx_los(phy_info);
		dn200_phy_sfp_tx_falut(phy_info);
		if (phy_info->sfp_mod_absent || phy_info->sfp_rx_los
		    || phy_info->sfp_tx_falut) {
			dev_dbg(priv->device, "sfp_mod_absent =%d, sfp_rx_los = %d, sfp_tx_fault = %d\n",
				phy_info->sfp_mod_absent, phy_info->sfp_rx_los, phy_info->sfp_tx_falut);
			link_status = DN200_LINK_DOWN;
			phy_info->recfg_an = true;
			if (phy_info->sfp_mod_absent) {
				phy_info->sfp_speed = DN200_SFP_SPEED_UNKNOWN;
				phy_info->sfp_changed = true;
				dn200_clear_lks(phy_info, DN200_SFP_TYPE_1000 | DN200_SFP_TYPE_10000, true);
				DN200_SET_SUP(lks, 1000baseX_Full);
				DN200_SET_SUP(lks, 10000baseKR_Full);
				DN200_LM_COPY(lks, advertising, lks, supported);
				phy_info->link_modes = DN300_10000baseKR_Full | DN300_1000BASEX_Full;
			}
			if (phy_info->phy_multispeed_work.work.func)
				cancel_delayed_work_sync(&phy_info->phy_multispeed_work);
			if (phy_info->kr_train_work.func)
				cancel_work_sync(&phy_info->kr_train_work);
		} else {
			/*sfp rx power detect, reinit an */
			if (sfp_rx_los && !phy_info->sfp_rx_los) {
				if (phy_info->phy_ops->identity(phy_info) < 0)
					return DN200_LINK_DOWN;
				/*sfp only support 1G module, switch to 1G*/
				if (phy_info->sfp_speed == DN200_SFP_SPEED_1000 && phy_info->speed != SPEED_1000)
					dn200_speed_set(phy_info, SPEED_1000);
			}

			/*autoneg on, try auto firstly */
			if (phy_info->an && phy_info->recfg_an) {
				dn200_phy_link_train_config(phy_info);
				phy_info->recfg_an = false;
				return DN200_LINK_DOWN;
			}
			dn200_phy_speed_self_adapt(phy_info);
			link_status = dn200_link_status_get(phy_info);
		}
		if (link_status &&
			(phy_info->last_link_speed != phy_info->speed)) {
			phy_info->self_adap_reset = true;
			phy_info->last_link_speed = phy_info->speed;
			/* Stop the I2C controller, avoid link-partner's link status
			 * has undergone multiple changes after switch speed
			 * when vfs has been enabled.
			 */
			phy_info->sfp_tx_disable = 1;
			dn200_phy_set_sfp_tx_disable(phy_info);
			dn200_normal_reset(priv);
			return 0;
		}
	}
	phy_info->phy_ops->blink_control(phy_info, link_status);
	if (old_link == link_status)
		return 0;
	/* sfp only support 10G, but can change 1000,and link*/
	if (phy_info->sfp_speed == DN200_SFP_SPEED_10000 && phy_info->speed == SPEED_1000) {
		dn200_clear_lks(phy_info, DN200_SFP_TYPE_10000, true);
		phy_info->link_modes = phy_info->link_modes & 0x7;
		DN200_SET_ADV(lks, 1000baseX_Full);
		DN200_SET_SUP(lks, 1000baseX_Full);
	}
	if (link_status) {
		if (!dn200_mac_dma_link_check(priv, phy_info))
			return 0;
	}
	phy_info->blk_err_ck = false;
	phy_info->phy_ops->led_control(phy_info, link_status);
	phy_info->link_status = link_status;
	dn200_phy_info_state_change(phy_info);
	dn200_phy_print_status(phy_info);
	if (PRIV_SRIOV_SUPPORT(priv))
		dn200_pf_set_link_status(priv->hw, phy_info->link_status);
	if (link_status) {
		if (phy_info->phy_multispeed_work.work.func)
			cancel_delayed_work_sync(&phy_info->phy_multispeed_work);
		phy_info->mac_ops->mac_link_up(priv, phy_info->phydev, 0,
					       phy_info->phy_interface,
					       phy_info->speed, phy_info->dup,
					       phy_info->pause & MLO_PAUSE_TX,
					       phy_info->pause & MLO_PAUSE_RX);
		clear_bit(DN200_PHY_SFP_NEED_RESET, &phy_info->phy_state);
		netif_carrier_on(phy_info->dev);
		netif_tx_start_all_queues(priv->dev);
		linkwatch_fire_event(phy_info->dev);
		phy_info->last_link_speed = phy_info->speed;
		phy_info->phy_status_time_intr = DN200_PHY_STATUS_NINTR;
	} else {
		netif_carrier_off(phy_info->dev);
		netif_tx_stop_all_queues(priv->dev);
		linkwatch_fire_event(phy_info->dev);
		phy_info->mac_ops->mac_link_down(priv, 0,
						 phy_info->phy_interface);
		clear_bit(DN200_PHY_SFP_INITED, &phy_info->phy_state);
		set_bit(DN200_PHY_SFP_NEED_RESET, &phy_info->phy_state);
		if (netif_msg_ifdown(priv))
			dn200_xpcs_link_down_reg_dump(phy_info);
		phy_info->phy_status_time_intr = DN200_PHY_STATUS_DINTR;
	}
	return 0;
}

irqreturn_t dn200_phy_status_isr(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct dn200_priv *priv = netdev_priv(dev);

	dn200_phy_status(PRIV_PHY_INFO(priv));
	return IRQ_HANDLED;
}

static void dn200_vf_get_phy_info(struct dn200_phy_info *phy_info)
{
	struct dn200_sriov_phy_info sriov_info = { 0 };
	struct ethtool_link_ksettings *lks = &phy_info->lks;

	linkmode_zero(lks->link_modes.lp_advertising);
	linkmode_zero(lks->link_modes.advertising);
	linkmode_zero(lks->link_modes.supported);
	dn200_get_phy_info(((struct dn200_priv *)netdev_priv(phy_info->dev))->
			   hw, &sriov_info);

	phy_info->speed = sriov_info.speed;
	phy_info->dup = sriov_info.dup;
	phy_info->port_type = -1;
	phy_info->media_type = sriov_info.media_type;
	phy_info->an = AUTONEG_DISABLE;
	phy_info->pause = sriov_info.pause;
	phy_info->phy_interface = sriov_info.phy_interface;
	phy_info->link_modes = sriov_info.link_modes;

	if (phy_info->link_modes & DN300_100BASET_Full)
		DN200_SET_SUP(lks, 100baseT_Full);
	if (phy_info->link_modes & DN300_1000BASET_Full)
		DN200_SET_SUP(lks, 1000baseT_Full);
	if (phy_info->link_modes & DN300_1000BASEX_Full)
		DN200_SET_SUP(lks, 1000baseX_Full);
	if (phy_info->link_modes & DN300_10000baseSR_Full)
		DN200_SET_SUP(lks, 10000baseSR_Full);
	if (phy_info->link_modes & DN300_10000baseLRM_Full)
		DN200_SET_SUP(lks, 10000baseLRM_Full);
	if (phy_info->link_modes & DN300_10000baseLR_Full)
		DN200_SET_SUP(lks, 10000baseLR_Full);
	if (phy_info->link_modes & DN300_10000baseKR_Full)
		DN200_SET_SUP(lks, 10000baseKR_Full);
	if (phy_info->link_modes & DN300_10000baseCR_Full)
		DN200_SET_SUP(lks, 10000baseCR_Full);
	if (phy_info->link_modes & DN300_10000baseER_Full)
		DN200_SET_SUP(lks, 10000baseER_Full);
	DN200_LM_COPY(lks, advertising, lks, supported);
}

static int dn200_vf_phy_status(struct dn200_phy_info *phy_info)
{
	bool old_link = phy_info->link_status;
	bool link_status;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	link_status = dn200_vf_get_link_status(priv->hw);
	if (old_link == link_status)
		return 0;
	dn200_vf_get_phy_info(phy_info);
	if (link_status) {
		phy_info->mac_ops->mac_link_up(priv, phy_info->phydev, 0,
					       phy_info->phy_interface,
					       phy_info->speed, phy_info->dup,
					       phy_info->pause & MLO_PAUSE_TX,
					       phy_info->pause & MLO_PAUSE_RX);
		netif_carrier_on(phy_info->dev);
		netif_tx_start_all_queues(priv->dev);
	} else {
		netif_carrier_off(phy_info->dev);
		netif_tx_stop_all_queues(priv->dev);
	}
	phy_info->link_status = link_status;

	dn200_phy_print_status(phy_info);
	return 0;
}

static void dn200_speed_set(struct dn200_phy_info *phy_info, u32 speed)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (speed == SPEED_10000) {
		/*init as 10G */
		phy_info->speed = SPEED_10000;
		phy_info->mac_ops->mac_speed_set(priv, phy_info->phy_interface,
						 phy_info->speed);
		dn200_xpcs_switch_to_10G(phy_info);
	} else if (speed == SPEED_1000) {
		/*init as 1G */
		phy_info->speed = SPEED_1000;
		phy_info->mac_ops->mac_speed_set(priv, phy_info->phy_interface,
						 phy_info->speed);
		dn200_xpcs_switch_to_1G(phy_info);
	}
}

static void dn200_multispeed_setup(struct dn200_phy_info *phy_info)
{
	u32 highest_speed = phy_info->setting_speed;

	/*phy stop, just return */
	if (!test_bit(DN200_PHY_STARTED, &phy_info->phy_state))
		return;
	/* this sets the link speed and restarts auto-neg */
	while (test_and_set_bit(DN200_PHY_IN_SFP_INIT, &phy_info->phy_state))
		usleep_range(1000, 2000);
	netdev_dbg(phy_info->dev,
		   "%s %d link_status %d highest_speed %d sfp_speed %d current speed %d\n",
		   __func__, __LINE__, phy_info->link_status, highest_speed,
		   phy_info->sfp_speed, phy_info->speed);
	if (phy_info->link_status)
		goto clear_out;

	/*Read to clear block err */
	dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_PCS_10GBRT_STAT2);
	/* max speed 10000 and sff support multispeed speed, than init as 10G firstly, otherwise init as 1G secondly
	 * if both 10 and 1G link failed, restore the default settings.
	 */
	if (highest_speed == SPEED_10000
	    && (phy_info->sfp_speed & DN200_SFP_SPEED_10000)
	    && phy_info->speed == SPEED_1000) {
		/*init as 10G firstly */
		dn200_speed_set(phy_info, SPEED_10000);
	} else if (((highest_speed == SPEED_10000 &&
				(phy_info->sfp_speed & DN200_SFP_SPEED_1000)) ||
				(highest_speed == SPEED_1000 &&
				(phy_info->sfp_speed & DN200_SFP_SPEED_1000))) &&
				phy_info->speed == SPEED_10000) {
		/*init as 1G */
		dn200_speed_set(phy_info, SPEED_1000);
	}
	phy_info->recfg_an = true;
clear_out:
	clear_bit(DN200_PHY_IN_SFP_INIT, &phy_info->phy_state);
}

static void dn200_phy_status_service(struct work_struct *work)
{
	struct dn200_phy_info *phy_info = container_of(work,
						       struct dn200_phy_info,
						       phy_status_work);
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	bool old_link = phy_info->link_status;

	if (!phy_info->phydev) {
		/*someone else is in init, wait until next timer event */
		if (test_bit(DN200_PHY_IN_SFP_INIT, &phy_info->phy_state) ||
		    test_bit(DN200_PHY_IN_TRAIN, &phy_info->phy_state))
			return;
	}
	phy_info->phy_ops->link_status(phy_info);
	if (!PRIV_IS_VF(priv) && phy_info->link_status != old_link)
		fw_link_state_set(&priv->plat_ex->ctrl, phy_info->link_status, phy_info->dup, phy_info->speed);
}

static void dn200_phy_multispeed_service(struct work_struct *work)
{
	struct dn200_phy_info *phy_info = container_of(work,
						       struct dn200_phy_info,
						       phy_multispeed_work.work);

	dn200_multispeed_setup(phy_info);
}

static void dn200_phy_status_timer(struct timer_list *t)
{
	struct dn200_phy_info *phy_info =
	    from_timer(phy_info, t, phy_status_timer);
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	u16 reg_val = 0;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state) ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return;
	}
	if (!PRIV_IS_VF(priv) && !phy_info->phydev) {
		_phy_reg_read(phy_info, 0x20b3 + (3 - phy_info->xpcs_idx) * 0x200, &reg_val);
		if (!(reg_val & BIT(0))) {
			netdev_err(priv->dev, "%s :mpllA lock failed\n", __func__);
			dn200_global_err(priv, DN200_PHY_MPLLA_UNLOCK);
			return;
		}
	}
	queue_work(phy_info->dev_workqueue, &phy_info->phy_status_work);

	mod_timer(&phy_info->phy_status_timer, jiffies + msecs_to_jiffies(phy_info->phy_status_time_intr));
}

static void dn200_init_phy_status_timers(struct dn200_phy_info *phy_info)
{
	timer_setup(&phy_info->phy_status_timer, dn200_phy_status_timer, 0);
}

static void dn200_start_phy_status_timers(struct dn200_phy_info *phy_info)
{
	mod_timer(&phy_info->phy_status_timer, jiffies + msecs_to_jiffies(phy_info->phy_status_time_intr));
}

static void dn200_stop_phy_status_timers(struct dn200_phy_info *phy_info)
{
	del_timer_sync(&phy_info->phy_status_timer);
}

static int dn200_xpcs_an_config(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;

	if (phy_info->an == DN200_AN_DISABLE) {
		reg_val =
		    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				    MDIO_CTRL1);
		reg_val &= (~AN_CTRL_AN_EN);
		dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				 MDIO_CTRL1, reg_val);
		return 0;
	}
	if (phy_info->an) {
		/* Update xpcs counter */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_TIMER_CTRL0, GENMASK(15, 0), 0,
				   0x11e1);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_TIMER_CTRL1, GENMASK(15, 0), 0,
				   0x9502);

		/*CL73_TMR_OVR_RIDE */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_DIG_CTRL1, CL73_TMR_OVR_RIDE,
				   CL73_TMR_OVR_RIDE_S, 1);

		/* 1. Disable auto-negotiation first */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S,
				   0);
		/* Advertise support speed mode - 10G(Bit7) & 1G(Bit5) */
		dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				 SR_AN_ADV2, 0);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV2, GENMASK(15, 0), 0, 0x80);
		/* Provide remote fault to link partner */
		/*AN_ADV_RF_13 */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV1, AN_ADV_RF_13_MASK,
				   AN_ADV_RF_13_SHIFT, 1);
		/* AN_ADV_ACK */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV1, AAN_ADV_ACK_MASK,
				   AN_ADV_ACK_SHIFT, 0);
		/*AN_ADV_NP */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   SR_AN_ADV1, AN_ADV_NP, AN_ADV_NP_S, 0);

		/* Clear intr status and enable xpcs intr */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_INTR, GENMASK(2, 0), 0, 0);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_INTR_MSK, GENMASK(2, 0), 0, 0x7);

		/* disable rx data output on lane 0 */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, VR_RX_GENCTRL0,
				   RX_DT_EN_0_MASK, RX_DT_EN_0_S, 0);

		/*1: disable kr train */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr,
				   MDIO_MMD_PMAPMD, SR_PMA_KR_PMD_CTRL, TR_EN,
				   TR_EN_S, 0);
		/*2: disable an */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S,
				   0);
		/*disable an restart */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   MDIO_CTRL1, AN_CTRL_AN_RESTART,
				   AN_CTRL_AN_RESTART_S, 0);
		/*disable interrupt */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_INTR_MSK, GENMASK(2, 0), 0, 0);
		/*3: enable interrupt */
		/* Clear intr status and enable xpcs intr */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_INTR, GENMASK(2, 0), 0, 0);
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   VR_AN_INTR_MSK, GENMASK(2, 0), 0, 0x7);
		/*disable an exten np */
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
				   MDIO_CTRL1, GENMASK(13, 13), 13, 0);
	}

	phy_info->an_sucess = false;
	return 0;
}

static int dn200_phy_an_config(struct dn200_phy_info *phy_info)
{
	int ret;

	if (phy_info->phydev) {
		dn200_linkmode_and(phy_info->phydev->advertising,
				   phy_info->phydev->supported,
				   phy_info->lks.link_modes.advertising);
		phy_info->phydev->autoneg = phy_info->an;

		if (phy_info->speed == SPEED_1000 &&
			phy_info->dup != DN200_DUP_FULL)
			return -EINVAL;

		if (phy_info->an != AUTONEG_ENABLE) {
			phy_info->phydev->speed = phy_info->speed;
			phy_info->phydev->duplex = phy_info->dup;
			if (phy_info->speed == SPEED_1000) {
				clear_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT,
					phy_info->phydev->advertising);
				clear_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT,
					phy_info->phydev->advertising);
				clear_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT,
					phy_info->phydev->advertising);
				clear_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,
					phy_info->phydev->advertising);
				clear_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
					phy_info->phydev->advertising);
				set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
					phy_info->phydev->advertising);
				phy_info->phydev->autoneg = AUTONEG_ENABLE;
			}
		}

		ret = phy_start_aneg(phy_info->phydev);
		phy_info->self_adap_reset = true;
		return ret;
	}
	return dn200_xpcs_an_config(phy_info);
}

static int dn200_phy_identity(struct dn200_phy_info *phy_info)
{
	int err = 0;

	if (phy_info->phydev)
		err = dn200_extern_phy_identity(phy_info);
	if (phy_info->media_type == DN200_MEDIA_TYPE_XPCS_1000BASEX ||
	    phy_info->media_type == DN200_MEDIA_TYPE_XPCS_10GBASEKR) {
		if (!phy_info->xpcs_sfp_valid)
			return err;
		clear_bit(DN200_PHY_SFP_INITED, &phy_info->phy_state);
		err = dn200_sfp_module_identify(phy_info);
		if (err)
			return err;
		dn200_phy_sfp_detect(phy_info);
		phy_info->blk_err_ck = false;
		phy_info->blk_err_cnt = 0;
		if (phy_info->sfp_speed == DN200_SFP_SPEED_UNKNOWN)
			/*sfp speed get failed, try once */
			dn200_phy_sfp_detect(phy_info);
		if (phy_info->sfp_changed) {
			/*sfp module has been changed, change current speed*/
			if (phy_info->sfp_speed & DN200_SFP_SPEED_10000 && phy_info->setting_speed == SPEED_10000)
				dn200_speed_set(phy_info, SPEED_10000);
			if (phy_info->sfp_speed == DN200_SFP_SPEED_1000)
				dn200_speed_set(phy_info, SPEED_1000);
			phy_info->sfp_changed = false;
		}
		return 1;
	}
	return err;
}

static int dn200_phy_media_type(struct dn200_phy_info *phy_info)
{
	switch (phy_info->phy_interface) {
		/*extern 1G copper */
	case PHY_INTERFACE_MODE_RGMII_ID:
		phy_info->media_type = DN200_MEDIA_TYPE_PHY_COPPER;
		phy_info->phydev_mode = DN200_MDIO_MODE_CL22;
		break;
		/*XPCS 10G fibre */
	case PHY_INTERFACE_MODE_XGMII:
		if (phy_info->setting_speed == SPEED_1000)
			phy_info->media_type = DN200_MEDIA_TYPE_XPCS_1000BASEX;
		else
			phy_info->media_type = DN200_MEDIA_TYPE_XPCS_10GBASEKR;
		break;
		/*XPCS 1G fibre */
	case PHY_INTERFACE_MODE_GMII:
		phy_info->media_type = DN200_MEDIA_TYPE_XPCS_1000BASEX;
		break;
		/*extern 1G fibre */
	case PHY_INTERFACE_MODE_1000BASEX:
		phy_info->media_type = DN200_MEDIA_TYPE_PHY_1000BASEX;
		phy_info->phydev_mode = DN200_MDIO_MODE_CL22;
		break;
	default:
		break;
	}
	return 0;
}

static int dn200_phy_set_link_mode(struct dn200_phy_info *phy_info,
				   enum dn200_media_type link_mode)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	DN200_ZERO_SUP(lks);
	/*Set phy supported features */
	switch (link_mode) {
	case DN200_MEDIA_TYPE_PHY_COPPER:
		DN200_SET_SUP(lks, TP);
		DN200_SET_SUP(lks, 10baseT_Full);
		DN200_SET_SUP(lks, 10baseT_Half);
		DN200_SET_SUP(lks, 100baseT_Full);
		DN200_SET_SUP(lks, 100baseT_Half);
		DN200_SET_SUP(lks, 1000baseT_Full);
		DN200_SET_SUP(lks, Autoneg);
		DN200_SET_SUP(lks, Pause);
		phy_info->port_type = PORT_TP;
		if (phy_info->setting_speed <= 0) {
			phy_info->setting_speed = SPEED_1000;
			priv->flow_ctrl_an = true;
		}
		break;
	case DN200_MEDIA_TYPE_XPCS_1000BASEX:
	case DN200_MEDIA_TYPE_PHY_1000BASEX:
		phy_info->pause &= ~MLO_PAUSE_AN;
		phy_info->an = phy_info->cur_an;
		if (phy_info->setting_speed <= 0) {
			phy_info->setting_speed = SPEED_1000;
			phy_info->speed = SPEED_1000;
		}
		if (phy_info->pause) {
			DN200_SET_SUP(lks, Pause);
			DN200_SET_SUP(lks, Asym_Pause);
		}
		if (phy_info->an)
			DN200_SET_SUP(lks, Autoneg);
		phy_info->port_type = PORT_FIBRE;
		phy_info->dup = DN200_DUP_FULL;
		break;
	case DN200_MEDIA_TYPE_XPCS_10GBASEKR:
		DN200_SET_SUP(lks, FIBRE);
		phy_info->port_type = PORT_FIBRE;
		phy_info->an = phy_info->cur_an;
		if (phy_info->setting_speed <= 0) {
			phy_info->setting_speed = SPEED_10000;
			phy_info->speed = SPEED_10000;
		}
		if (phy_info->an)
			DN200_SET_SUP(lks, Autoneg);
		phy_info->pause &= ~MLO_PAUSE_AN;
		if (phy_info->pause & (MLO_PAUSE_TX | MLO_PAUSE_RX)) {
			DN200_SET_SUP(lks, Pause);
			DN200_SET_SUP(lks, Asym_Pause);
		}
		phy_info->dup = DN200_DUP_FULL;
		break;
	default:
		break;
	}
	if (!priv->mii || bitmap_empty(lks->link_modes.advertising, __ETHTOOL_LINK_MODE_MASK_NBITS))
		DN200_LM_COPY(lks, advertising, lks, supported);
	return 0;
}

#define XPCS0_CR_REG_BASE	(0x40100)
#define XPCS0_CR_CTRL		0x0
#define XPCS0_CR_ADDR		0x4
#define XPCS0_CR_DATA		0x8
static int _phy_reg_write(struct dn200_phy_info *phy_info, u16 phy_reg_addr,
			  u16 reg_val)
{
	int ret = 0;
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&dn200_xpcs_lock, flags);
	ret =
	    readl_poll_timeout_atomic(phy_info->xpcs->xpcs_regs_base +
			       XPCS0_CR_REG_BASE + XPCS0_CR_CTRL, val,
			       !(val & BIT(0)), 1, 10000);
	if (ret)
		goto out;

	writel((u32) phy_reg_addr,
	       phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
	       XPCS0_CR_ADDR);
	writel((u32) reg_val,
	       phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
	       XPCS0_CR_DATA);
	writel(0x3,
	       phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
	       XPCS0_CR_CTRL);

out:
	spin_unlock_irqrestore(&dn200_xpcs_lock, flags);
	return ret;
}

/* Used to validate xpcs phy fw write function. */
static int  _phy_reg_read(struct dn200_phy_info *phy_info,
					u16 phy_reg_addr, u16 *reg_val)
{
	int ret = 0;
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&dn200_xpcs_lock, flags);

	ret =
	    readl_poll_timeout_atomic(phy_info->xpcs->xpcs_regs_base +
			       XPCS0_CR_REG_BASE + XPCS0_CR_CTRL, val,
			       !(val & BIT(0)), 1, 10000);
	if (ret)
		goto out;

	writel((u32) phy_reg_addr,
	       phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
	       XPCS0_CR_ADDR);
	writel(0x1,
	       phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
	       XPCS0_CR_CTRL);

	ret =
	    readl_poll_timeout_atomic(phy_info->xpcs->xpcs_regs_base +
			       XPCS0_CR_REG_BASE + XPCS0_CR_CTRL, val,
			       !(val & BIT(0)), 1, 10000);
	if (ret)
		goto out;

	*reg_val =
	    (u16) readl(phy_info->xpcs->xpcs_regs_base + XPCS0_CR_REG_BASE +
			XPCS0_CR_DATA);

out:
	spin_unlock_irqrestore(&dn200_xpcs_lock, flags);
	return ret;
}

static int dn200_xpcs_switch_to_1G_tx_regset(struct dn200_phy_info *phy_info)
{
	u8 phylane_idx = 3 - phy_info->xpcs_idx;
	u16 reg_val;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	_phy_reg_write(phy_info, 0x2048 + 0x200 * phylane_idx, 0x501f);/*TX*/
	_phy_reg_write(phy_info, 0x2047 + 0x200 * phylane_idx, 0xe461);/*TX*/
	_phy_reg_write(phy_info, 0x2046 + 0x200 * phylane_idx, 0x0300);/*TX*/
	_phy_reg_write(phy_info, 0x2049 + 0x200 * phylane_idx, 0x18);/*TX*/
	_phy_reg_write(phy_info, 0x204a + 0x200 * phylane_idx, 0x0000);/*TX*/
	_phy_reg_write(phy_info, 0x100c + 0x200 * phylane_idx, 0x0180);/*TX*/
	_phy_reg_write(phy_info, 0x204c + 0x200 * phylane_idx, 0x7e28);/*TX*/
	_phy_reg_write(phy_info, 0x204d + 0x200 * phylane_idx, 0x4);/*TX*/
	_phy_reg_write(phy_info, 0x1003 + 0x200 * phylane_idx, 0x0588);/*TX*/
	_phy_reg_write(phy_info, 0x1004 + 0x200 * phylane_idx, 0x2040);/*TX*/
	_phy_reg_write(phy_info, 0x1002 + 0x200 * phylane_idx, 0x009c);/*TX*/
	if (priv->plat_ex->raid_supported)
		_phy_reg_write(phy_info, 0x1152 + 0x200 * phylane_idx, 0x80a0);/*TX*/
	/* Config PHY: choose context and set Tx width */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_TXCNTX_CTRL0, 0x0808);/*TX*/
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_TXCMCNTX_SEL, 0x0404);/*TX*/

	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TXWIDTH_CTRL, 0x1111);/*TX*/

		/* Select KX Mode */
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
			    MDIO_CTRL2);
	reg_val &= ~GENMASK(3, 0);
	reg_val |= 0x1 & GENMASK(3, 0);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PCS, MDIO_CTRL2,
			 reg_val);/*TX*/

	/* Clear USXG_EN and EN_2_5G_MODE */
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
			    VR_XS_PCS_DIG_CTRL1);
	reg_val &= ~(XPCS_PCS_BYP_PWRUP_DUP1 | XPCS_EN_2_5G_MODE);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
			 VR_XS_PCS_DIG_CTRL1, reg_val);/*TX*/

	/* Select 1G mode */
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			    MDIO_CTRL1);
	reg_val &= ~BIT(13);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 MDIO_CTRL1, reg_val);/*TX*/

	/*TX eq override */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TX_EQ_CTRL0, 0x1800);/*TX*/
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TX_EQ_CTRL1, 0x40);/*TX*/
	/* Change xge top clock mux */
	writel(0x0,
		phy_info->xpcs->xpcs_regs_base +
		XGE_XGMAC_CLK_MUX_CTRL(phy_info->xpcs_idx));
	return 0;
}

static int dn200_xpcs_switch_to_1G_rx_regset(struct dn200_phy_info *phy_info)
{
	u8 phylane_idx = 3 - phy_info->xpcs_idx;

	_phy_reg_write(phy_info, 0x2005 + 0x200 * phylane_idx, 0x060c);	/*rx ovrd */
	_phy_reg_write(phy_info, 0x2052 + 0x200 * phylane_idx, 0x0550);/*RX*/
	_phy_reg_write(phy_info, 0x2053 + 0x200 * phylane_idx, 0x4052);/*RX*/
	_phy_reg_write(phy_info, 0x2054 + 0x200 * phylane_idx, 0x2200);/*RX*/
	_phy_reg_write(phy_info, 0x204f + 0x200 * phylane_idx, 0x3014);/*RX*/
	_phy_reg_write(phy_info, 0x2050 + 0x200 * phylane_idx, 0x7514);/*RX*/
	_phy_reg_write(phy_info, 0x2056 + 0x200 * phylane_idx, 0xd841);/*RX*/
	_phy_reg_write(phy_info, 0x103d + 0x200 * phylane_idx, 0x0180);/*RX*/
	_phy_reg_write(phy_info, 0x2059 + 0x200 * phylane_idx, 0x656d);/*RX*/
	_phy_reg_write(phy_info, 0x2051 + 0x200 * phylane_idx, 0x0403);/*RX*/
	_phy_reg_write(phy_info, 0x205a + 0x200 * phylane_idx, 0x2021);/*RX*/
	_phy_reg_write(phy_info, 0x1021 + 0x200 * phylane_idx, 0x9c00);/*RX*/
	_phy_reg_write(phy_info, 0x200c + 0x200 * phylane_idx, 0x0010);/*RX*/
	_phy_reg_write(phy_info, 0x20f9 + 0x200 * phylane_idx, 0x01f4);/*TX*/
	_phy_reg_write(phy_info, 0x1022 + 0x200 * phylane_idx, 0x0550);
	/* Config PHY: choose context and set Tx/Rx width */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_RXCNTX_CTRL0, 0x0808);/*RX*/
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_RXWIDTH_CTRL, 0x1111);/*RX*/
	return 0;
}

static int dn200_xpcs_switch_to_10G_rx_regset(struct dn200_phy_info *phy_info)
{
	u8 phylane_idx = 3 - phy_info->xpcs_idx;

	_phy_reg_write(phy_info, 0x2005 + 0x200 * phylane_idx, 0x18);
	_phy_reg_write(phy_info, 0x2053 + 0x200 * phylane_idx, 0x4013);
	_phy_reg_write(phy_info, 0x204f + 0x200 * phylane_idx, 0x4110);
	_phy_reg_write(phy_info, 0x2050 + 0x200 * phylane_idx, 0x6514);
	_phy_reg_write(phy_info, 0x2056 + 0x200 * phylane_idx, 0xd841);
	_phy_reg_write(phy_info, 0x103d + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x2059 + 0x200 * phylane_idx, 0xffff);
	_phy_reg_write(phy_info, 0x205a + 0x200 * phylane_idx, 0xff7f);
	_phy_reg_write(phy_info, 0x2051 + 0x200 * phylane_idx, 0x6403);
	_phy_reg_write(phy_info, 0x200c + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x20f9 + 0x200 * phylane_idx, 0x01e0);
	/* rx_vco_ld_val=1650&&rx_ref_ld_val=20=>rx_clk */
	_phy_reg_write(phy_info, 0x1021 + 0x200 * phylane_idx, 0x0094);
	_phy_reg_write(phy_info, 0x1022 + 0x200 * phylane_idx, 0x2672);
	_phy_reg_write(phy_info, 0x2052 + 0x200 * phylane_idx, 0x2672);
	_phy_reg_write(phy_info, 0x2054 + 0x200 * phylane_idx, 0x280c);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_RXCNTX_CTRL0, 0x0d0d);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_RXWIDTH_CTRL, 0x4444);

	return 0;
}

static int dn200_xpcs_switch_to_10G_tx_regset(struct dn200_phy_info *phy_info)
{
	u8 phylane_idx = 3 - phy_info->xpcs_idx;
	u16 reg_val;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	_phy_reg_write(phy_info, 0x2048 + 0x200 * phylane_idx, 0x4595);
	_phy_reg_write(phy_info, 0x2047 + 0x200 * phylane_idx, 0xe7cf);
	_phy_reg_write(phy_info, 0x2046 + 0x200 * phylane_idx, 0x1288);
	_phy_reg_write(phy_info, 0x2049 + 0x200 * phylane_idx, 0x4110);
	_phy_reg_write(phy_info, 0x204a + 0x200 * phylane_idx, 0x6637);
	_phy_reg_write(phy_info, 0x100c + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x204c + 0x200 * phylane_idx, 0xfffe);
	_phy_reg_write(phy_info, 0x204d + 0x200 * phylane_idx, 0x0fff);
	_phy_reg_write(phy_info, 0x1003 + 0x200 * phylane_idx, 0x0180);
	_phy_reg_write(phy_info, 0x1004 + 0x200 * phylane_idx, 0x0000);
	// _phy_reg_write(phy_info, 0x1165 + 0x200 * phylane_idx, 0x10D);
	_phy_reg_write(phy_info, 0x1002 + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x2052 + 0x200 * phylane_idx, 0x257b);
	_phy_reg_write(phy_info, 0x2053 + 0x200 * phylane_idx, 0x4013);
	_phy_reg_write(phy_info, 0x2054 + 0x200 * phylane_idx, 0x220c);
	_phy_reg_write(phy_info, 0x204f + 0x200 * phylane_idx, 0x4110);
	_phy_reg_write(phy_info, 0x2050 + 0x200 * phylane_idx, 0x6514);
	_phy_reg_write(phy_info, 0x2056 + 0x200 * phylane_idx, 0xd841);
	_phy_reg_write(phy_info, 0x103d + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x2059 + 0x200 * phylane_idx, 0xffff);
	_phy_reg_write(phy_info, 0x205a + 0x200 * phylane_idx, 0xff7f);
	_phy_reg_write(phy_info, 0x2051 + 0x200 * phylane_idx, 0x6403);
	_phy_reg_write(phy_info, 0x1021 + 0x200 * phylane_idx, 0x0014);
	_phy_reg_write(phy_info, 0x200c + 0x200 * phylane_idx, 0x0000);
	_phy_reg_write(phy_info, 0x20f9 + 0x200 * phylane_idx, 0x01e0);
	if (priv->plat_ex->raid_supported)
		_phy_reg_write(phy_info, 0x1152 + 0x200 * phylane_idx, 0x80a1);
	/* Deselect KX Mode */
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
			    MDIO_CTRL2);
	reg_val &= ~GENMASK(3, 0);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PCS, MDIO_CTRL2,
			 reg_val);

	/* Deselect 1G mode */
	reg_val =
	    dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			    MDIO_CTRL1);
	reg_val |= BIT(13);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 MDIO_CTRL1, reg_val);

	/* Config PHY: choose context and set Tx/Rx width */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_TXCNTX_CTRL0, 0x0d0d);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP32G_TXCMCNTX_SEL, 0x0909);

	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TXWIDTH_CTRL, 0x4444);

	/*TX eq override */
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TX_EQ_CTRL0, 0xc04);
	dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PMAPMD,
			 VR_XS_PMA_MP25G_TX_EQ_CTRL1, 0x40);
	/* Change xge top clock mux */
	writel(0x6,
	       phy_info->xpcs->xpcs_regs_base +
	       XGE_XGMAC_CLK_MUX_CTRL(phy_info->xpcs_idx));

	return 0;
}
static int dn200_phy_find_phy_device(struct dn200_phy_info *phy_info)
{
	int ret;

	if (!phy_info->phydev)
		return -1;

	phy_info->phydev->link = 0;
	ret =
	    phy_attach_direct(phy_info->dev, phy_info->phydev,
			      phy_info->phydev->dev_flags,
			      phy_info->phy_interface);
	if (ret) {
		netdev_err(phy_info->dev, "phy_attach_direct failed %d\n", ret);
		return ret;
	}

	phy_start_aneg(phy_info->phydev);
	return 0;
}

static void dn200_phy_free_phy_device(struct dn200_phy_info *phy_info)
{
	if (phy_info->phydev) {
		phy_detach(phy_info->phydev);
		/* for some system suspend warning */
		phy_info->phydev->state = PHY_READY;
	}
}

static int dn200_phy_set_speed(struct dn200_phy_info *phy_info)
{
	return 0;		/*FIXME GUOFENG */
}

static void dn200_phy_timer_stop(struct dn200_phy_info *phy_info)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (phy_info->media_type == DN200_MEDIA_TYPE_PHY_1000BASEX ||
	    phy_info->media_type == DN200_MEDIA_TYPE_PHY_COPPER) {
		phy_info->phy_ops->stop_phy_timer(phy_info);
	} else if (phy_info->media_type == DN200_MEDIA_TYPE_XPCS_1000BASEX ||
		   phy_info->media_type == DN200_MEDIA_TYPE_XPCS_10GBASEKR) {
		phy_info->phy_ops->stop_phy_timer(phy_info);
		if (phy_info->phy_status_work.func)
			cancel_work_sync(&phy_info->phy_status_work);
		if (phy_info->kr_train_work.func)
			cancel_work_sync(&phy_info->kr_train_work);
		if (phy_info->phy_multispeed_work.work.func)
			cancel_delayed_work_sync(&phy_info->phy_multispeed_work);
	}
	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state)) {
		if (phy_info->link_status) {
			phy_info->link_status = DN200_LINK_DOWN;
			dn200_phy_print_status(phy_info);
		}
		if (PRIV_SRIOV_SUPPORT(priv))
			dn200_pf_set_link_status(priv->hw, phy_info->link_status);
	}
}
static int dn200_phy_stop(struct dn200_phy_info *phy_info)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (!test_bit(DN200_PHY_STARTED, &phy_info->phy_state))
		return 0;
	dn200_phy_timer_stop(phy_info);
	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state)) {
		phy_info->phy_ops->led_control(phy_info, DN200_LINK_DOWN);
		if (!phy_info->phydev)
			dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_DISABLE);
	}
	if (phy_info->media_type == DN200_MEDIA_TYPE_PHY_1000BASEX ||
	    phy_info->media_type == DN200_MEDIA_TYPE_PHY_COPPER) {
		dn200_phy_free_phy_device(phy_info);
	} else if (phy_info->media_type == DN200_MEDIA_TYPE_XPCS_1000BASEX ||
		   phy_info->media_type == DN200_MEDIA_TYPE_XPCS_10GBASEKR) {
		/* Stop the I2C controller */
		phy_info->sfp_tx_disable = 1;
		if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state))
			dn200_phy_set_sfp_tx_disable(phy_info);
	}
	clear_bit(DN200_PHY_STARTED, &phy_info->phy_state);
	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		fw_link_state_set(&priv->plat_ex->ctrl, DN200_LINK_DOWN, phy_info->dup, phy_info->speed);
	return 0;
}

static int dn200_vf_phy_stop(struct dn200_phy_info *phy_info)
{
	if (!test_bit(DN200_PHY_STARTED, &phy_info->phy_state))
		return 0;
	phy_info->phy_ops->stop_phy_timer(phy_info);
	if (phy_info->phy_status_work.func)
		cancel_work_sync(&phy_info->phy_status_work);
	phy_info->link_status = DN200_LINK_DOWN;
	netif_carrier_off(phy_info->dev);
	netif_tx_stop_all_queues(phy_info->dev);
	clear_bit(DN200_PHY_STARTED, &phy_info->phy_state);
	dn200_phy_print_status(phy_info);
	return 0;
}

static void dn200_sfp_phy_state_init(struct dn200_phy_info *phy_info)
{
	bitmap_empty(&phy_info->phy_state, BITS_PER_LONG);
	/*set sfp reset state to reinit sfp info */
	set_bit(DN200_PHY_SFP_NEED_RESET, &phy_info->phy_state);
	dn200_phy_sfp_reset(phy_info);
}

static int dn200_phy_start(struct dn200_phy_info *phy_info)
{
	int ret = 0;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	if (phy_info->media_type == DN200_MEDIA_TYPE_PHY_1000BASEX ||
	    phy_info->media_type == DN200_MEDIA_TYPE_PHY_COPPER) {
		ret = dn200_phy_find_phy_device(phy_info);
		if (ret) {
			netdev_err(phy_info->dev,
				   "dn200_phy_find_phy_device failed %d\n",
				   ret);
			return ret;
		}

		extern_phy_init(phy_info->phydev, priv->plat_ex->hw_rj45_type);
		phy_info->phydev->autoneg = phy_info->an;
		if (phy_info->an != AUTONEG_ENABLE) {
			phy_info->phydev->speed = phy_info->speed;
			phy_info->phydev->duplex = phy_info->dup;
		}

		phy_info->phy_ops->start_phy_timer(phy_info);
	} else if (phy_info->media_type == DN200_MEDIA_TYPE_XPCS_1000BASEX ||
		   phy_info->media_type == DN200_MEDIA_TYPE_XPCS_10GBASEKR) {
		/*reinit phy state */
		dn200_sfp_phy_state_init(phy_info);
		/* Start the I2C controller */
		phy_info->sfp_tx_disable = 0;
		dn200_phy_set_sfp_tx_disable(phy_info);
		phy_info->speed_reset_time = jiffies + DN200_SFP_RESET_TIME;
		phy_info->phy_ops->start_phy_timer(phy_info);
		usleep_range(10000, 20000);
		dn200_phy_read(phy_info, MDIO_MMD_PCS, MDIO_STAT1);
		phy_info->sfp_rx_los = 1;
		phy_info->recfg_an = true;
	}
	if (phy_info->phydev && phy_info->an)
		dn200_phy_an_config(phy_info);
	set_bit(DN200_PHY_STARTED, &phy_info->phy_state);
	phy_info->phy_status_time_intr = DN200_PHY_STATUS_DINTR;
	return ret;
}

static int dn200_vf_phy_start(struct dn200_phy_info *phy_info)
{
	phy_info->phy_ops->start_phy_timer(phy_info);
	set_bit(DN200_PHY_STARTED, &phy_info->phy_state);
	phy_info->phy_status_time_intr = DN200_PHY_STATUS_DINTR;
	phy_info->phy_ops->link_status(phy_info);
	return 0;
}

static int __maybe_unused dn200_phy_best_advertised_speed(struct dn200_phy_info *phy_info)
{
	switch (phy_info->phy_interface) {
		/*extern 1G copper */
	case PHY_INTERFACE_MODE_RGMII_ID:
		return SPEED_1000;
		/*XPCS 10G fibre */
	case PHY_INTERFACE_MODE_XGMII:
		return SPEED_10000;
		/*XPCS 1G fibre */
	case PHY_INTERFACE_MODE_GMII:
		return SPEED_1000;
		/*extern 1G fibre */
	case PHY_INTERFACE_MODE_1000BASEX:
		return SPEED_1000;
	default:
		return SPEED_UNKNOWN;
	}
	return SPEED_UNKNOWN;
}

static int dn200_phy_init(struct dn200_phy_info *phy_info)
{
	struct ethtool_link_ksettings *lks = &phy_info->lks;
	struct dn200_priv *priv = netdev_priv(phy_info->dev);
	/*disable an */
	if (!phy_info->phydev) {
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			   MDIO_CTRL1, AN_CTRL_AN_EN, AN_CTRL_AN_EN_S, 0);
		usleep_range(10000, 15000);
	}
	/*Get Current media type */
	phy_info->phy_ops->media_type_get(phy_info);
	dn200_phy_set_link_mode(phy_info, phy_info->media_type);
	phy_info->phy_ops->init_phy_timer(phy_info);
	if (!phy_info->phydev) {
		if (!phy_info->self_adap_reset)
			phy_info->speed = phy_info->setting_speed;
		if (phy_info->speed == SPEED_1000) {
			phy_info->mac_ops->mac_speed_set(priv,
							 phy_info->phy_interface,
							 phy_info->speed);
			if (phy_info->last_link_speed != phy_info->speed)
				dn200_xpcs_switch_to_1G(phy_info);
			if (netif_msg_ifup(priv))
				netdev_info(phy_info->dev,
					    "PHY %d initialized as 1G speed mode\n",
					    phy_info->xpcs_idx);
		}
		if (phy_info->speed == SPEED_10000) {
			/* we shoudl call dn200_xpcs_switch_to_10G rather than
			 * dn200_xpcs_init_as_10G because when set 1G speed last time,
			 * now we want 10G speed.
			 * if we call dn200_xpcs_init_as_10G,maybe we can't
			 * switch successfully.
			 */
			phy_info->mac_ops->mac_speed_set(priv,
							 phy_info->phy_interface,
							 phy_info->speed);
			if (phy_info->last_link_speed != phy_info->speed)
				dn200_xpcs_switch_to_10G(phy_info);
			if (netif_msg_ifup(priv))
				netdev_info(phy_info->dev,
					    "PHY %d initialized as 10GBASE-KR\n",
					    phy_info->xpcs_idx);
		}
		phy_info->last_link_speed = phy_info->speed;
		phy_info->self_adap_reset = false;
		clear_bit(DN200_PHY_IN_RESET, &phy_info->phy_state);
	}
	if (DN200_ADV(lks, Autoneg)) {
		phy_info->an = AUTONEG_ENABLE;
		if (phy_info->port_type != PORT_FIBRE) {
			phy_info->speed = SPEED_UNKNOWN;
			phy_info->dup = DUPLEX_UNKNOWN;
		}
	} else {
		phy_info->an = AUTONEG_DISABLE;
	//	phy_info->max_speed = dn200_phy_best_advertised_speed(phy_info);
		phy_info->dup = DUPLEX_FULL;
	}
	return 0;
}

static int dn200_vf_phy_init(struct dn200_phy_info *phy_info)
{
	phy_info->phy_ops->init_phy_timer(phy_info);
	return 0;
}

static int dn200_phy_reset(struct dn200_phy_info *phy_info)
{
	u16 reg_val = 0;

	if (phy_info->phydev) {
		genphy_soft_reset(phy_info->phydev);
		return phy_init_hw(phy_info->phydev);
	}

	reg_val =
		dn200_xpcs_read(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
				MDIO_CTRL1);
	reg_val |= MDIO_CTRL1_RESET;
	reg_val =
		dn200_xpcs_write(phy_info, phy_info->phy_addr, MDIO_MMD_PCS,
					MDIO_CTRL1, reg_val);
	return dn200_phy_init(phy_info);
}

const struct dn200_phy_ops dn200_phy_ops_info = {
	.media_type_get = dn200_phy_media_type,
	.identity = dn200_phy_identity,
	.link_status = dn200_phy_status,
	.read_i2c_byte = dn200_generic_i2c_byte_read,
	.write_i2c_byte = dn200_generic_i2c_byte_write,
	.read_i2c_eeprom = dn200_i2c_read_eeprom,
	.read_i2c_sff8472 = dn200_i2c_read_sff8472,
	.write_i2c_eeprom = dn200_i2c_write_eeprom,
	.init = dn200_phy_init,
	.an_config = dn200_phy_an_config,
	.start = dn200_phy_start,
	.stop = dn200_phy_stop,
	.reset = dn200_phy_reset,
	.set_speeds = dn200_phy_set_speed,
	.led_control = dn200_phy_set_led,
	.blink_control = dn200_blink_control,
	.init_phy_timer = dn200_init_phy_status_timers,
	.start_phy_timer = dn200_start_phy_status_timers,
	.stop_phy_timer = dn200_stop_phy_status_timers,
	.phy_timer_del = dn200_phy_timer_stop,
	.get_link_ksettings = dn200_get_link_ksettings,
	.set_link_ksettings = dn200_set_link_ksettings,
	.init_eee = dn200_phy_init_eee,
	.set_eee = dn200_ethtool_set_eee,
	.get_eee = dn200_ethtool_get_eee,
	.get_phy_pauseparam = dn200_get_phy_pauseparam,
	.set_phy_pauseparam = dn200_set_phy_pauseparam,
	.nway_reset = dn200_nway_reset,
	.phy_loopback = dn200_phy_loopback,
};

const struct dn200_phy_ops dn200_vf_phy_ops_info = {
	.link_status = dn200_vf_phy_status,
	.init = dn200_vf_phy_init,
	.start = dn200_vf_phy_start,
	.stop = dn200_vf_phy_stop,
	.init_phy_timer = dn200_init_phy_status_timers,
	.start_phy_timer = dn200_start_phy_status_timers,
	.stop_phy_timer = dn200_stop_phy_status_timers,
	.get_link_ksettings = dn200_get_link_ksettings,
	.set_link_ksettings = dn200_set_link_ksettings,
	.init_eee = dn200_phy_init_eee,
	.set_eee = dn200_ethtool_set_eee,
	.get_eee = dn200_ethtool_get_eee,
	.get_phy_pauseparam = dn200_get_phy_pauseparam,
};

/* dn200_hw_sideband_init :
 * init phy sideband signal
 */
void dn200_hw_sideband_init(struct dn200_phy_info *phy_info)
{
	struct dn200_priv *priv = netdev_priv(phy_info->dev);

	dn200_phy_set_led(phy_info, 0);
	if (phy_info->phydev) {
		extern_phy_force_led(phy_info->phydev, priv, 0, BLINK_DISABLE);
		return;
	}
	dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_DISABLE);
	phy_info->sfp_tx_disable = 1;
	dn200_phy_set_sfp_tx_disable(phy_info); /*disable tx */
	dn200_phy_sfp_present(phy_info);
}

static bool dn200_check_i2c_supported(struct dn200_priv *priv)
{
	struct pci_dev *pdev = to_pci_dev(priv->device);

	switch (pdev->device) {
	case DN200_DEV_ID_SFP_10G_2P_PURE_PF:
	case DN200_DEV_ID_SFP_10G_2P_SRIOV_PF:
	case DN200_DEV_ID_SFP_10G_2P_NVME_PUREPF:
	case DN200_DEV_ID_SFP_10G_4P_NVME_PUREPF:
	case DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_PF:
		return true;
	default:
		/*todo: other deviceid also need to support i2c */
		break;
	}
	return false;
}

/**
 * dn200_phy_info_init - phy info init
 * @dev: device pointer
 *
 * Description: init phy info struct
 *
 * Returns 0 on success, negative value on error
 */

int dn200_phy_info_init(struct net_device *dev,
			const struct dn200_mac_ops *mac_ops)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct dn200_phy_info *phy_info;
	struct dn200_xpcs_info *xpcs;
	struct phy_device *phydev;
	int ret = 0;
	u32 val;

	/* Some DT bindings do not set-up the PHY handle. Let's try to
	 * manually parse it
	 */
	int addr = priv->plat->phy_addr;

	phy_info = devm_kzalloc(priv->device, sizeof(*phy_info), GFP_KERNEL);
	if (!phy_info)
		return -ENOMEM;
	priv->plat_ex->phy_info = phy_info;
	phy_info->mac_ops = mac_ops;
	phy_info->dev = dev;
	phy_info->phy_interface = priv->plat->phy_interface;
	phy_info->phy_addr = addr;
	phy_info->xpcs_idx = priv->plat_ex->xpcs_index;
	phy_info->pause = priv->flow_ctrl;
	phy_info->cur_an = AUTONEG_ENABLE;
	/* Create workqueues */
	phy_info->dev_workqueue =
	    create_singlethread_workqueue(netdev_name(dev));
	if (!phy_info->dev_workqueue) {
		netdev_err(dev, "device workqueue creation failed\n");
		ret = -ENOMEM;
		goto phy_info_free;
	}

//	phy_info->dev_workqueue = priv->wq;
	INIT_WORK(&phy_info->phy_status_work, dn200_phy_status_service);
	INIT_WORK(&phy_info->kr_train_work, dn200_kr_train_work);
	if (PRIV_IS_VF(priv)) {
		phy_info->phy_ops = &dn200_vf_phy_ops_info;
		return 0;
	}
	phy_info->phy_ops = &dn200_phy_ops_info;

	if (priv->plat_ex->has_xpcs)
		goto xpcs_init;
	phydev = mdiobus_get_phy(priv->mii, addr);
	if (!phydev) {
		netdev_err(dev, "phydev get failed\n");
		ret = -ENODEV;
		goto phy_info_free;
	}

	ret = get_rj45_type(&priv->plat_ex->ctrl, &val);
	priv->plat_ex->hw_rj45_type = val;
	phy_info->mii_bus = priv->mii;
	phy_info->phydev = phydev;
	extern_phy_init(phydev, priv->plat_ex->hw_rj45_type);
	return ret;

xpcs_init:

	xpcs =
	    devm_kzalloc(priv->device, sizeof(struct dn200_xpcs_info),
			 GFP_KERNEL);
	if (!xpcs) {
		ret = -ENOMEM;
		netdev_err(dev, "Alloc phy_xpcs memory failed!\n");
		goto phy_info_free;
	}
	INIT_DELAYED_WORK(&phy_info->phy_multispeed_work,
			  dn200_phy_multispeed_service);
	xpcs->xpcs_read = dn200_xpcs_read;
	xpcs->xpcs_write = dn200_xpcs_write;
	xpcs->xpcs_regs_base = priv->ioaddr;

	phy_info->xpcs = xpcs;
	phy_info->sfp_has_gpio = true;

	phy_info->xpcs_sfp_valid = dn200_check_i2c_supported(priv);

	phy_info->gpio_data = priv->plat_ex->gpio_data;
	phy_info->gpio_base =
	    priv->ioaddr + phy_info->gpio_data->gpio_addr_offset;
	dn200_hw_sideband_init(phy_info);
	dn200_conf_pcie_common_para(phy_info);
	phy_info->max_speed = priv->plat_ex->max_speed;
	dn200_phy_fec_enable(dev, false);
	return 0;

phy_info_free:
	devm_kfree(priv->device, phy_info);
	return ret;
}

int dn200_phy_info_remove(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	u8 phylane_idx = 3 - phy_info->xpcs_idx;

	if (phy_info && phy_info->phy_ops && phy_info->phy_ops->stop)
		phy_info->phy_ops->stop(phy_info);
	if (phy_info->xpcs) {
		if (!PRIV_IS_VF(priv)) {
			/* recover the config for old driver */
			_phy_reg_write(phy_info, 0x1022 + 0x200 * phylane_idx, 0x57b);
		}
		devm_kfree(priv->device, phy_info->xpcs);
	}
	if (phy_info->phy_status_work.func)
		cancel_work_sync(&phy_info->phy_status_work);
	if (phy_info->phy_multispeed_work.work.func)
		cancel_delayed_work_sync(&phy_info->phy_multispeed_work);
	destroy_workqueue(phy_info->dev_workqueue);
	devm_kfree(priv->device, phy_info);
	return 0;
}

int dn200_phy_fec_enable(struct net_device *dev, bool enable)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);

	if (phy_info->phydev)
		return -EOPNOTSUPP;

	if (enable)
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			SR_AN_ADV3, GENMASK(15, 14), 14, 0x3);
	else
		dn200_xpcs_set_bit(phy_info, phy_info->phy_addr, MDIO_MMD_AN,
			SR_AN_ADV3, GENMASK(15, 14), 14, 0);

	if (netif_running(dev) &&
		phy_info->link_status == DN200_LINK_UP &&
		phy_info->speed == SPEED_10000) {
		dn200_phy_stop(phy_info);
		usleep_range(1000, 2000);
		dn200_phy_start(phy_info);
		return dn200_nway_reset(phy_info);
	}

	return 0;
}
