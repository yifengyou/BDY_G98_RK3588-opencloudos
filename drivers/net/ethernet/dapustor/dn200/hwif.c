// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include "common.h"
#include "dn200.h"
#include "dn200_ptp.h"

static u32 dn200_get_id(struct dn200_priv *priv, u32 id_reg)
{
	u32 reg = readl(priv->ioaddr + id_reg);

	if (!reg) {
		dev_info(priv->device, "Version ID not available\n");
		return 0x0;
	}

	dev_dbg(priv->device, "User ID: 0x%x, Chip ID: 0x%x\n",
		 (unsigned int)(reg & GENMASK(15, 8)) >> 8,
		 (unsigned int)(reg & GENMASK(7, 0)));
	return reg & GENMASK(7, 0);
}

static u32 dn200_get_dev_id(struct dn200_priv *priv, u32 id_reg)
{
	u32 reg = readl(priv->ioaddr + id_reg);

	if (!reg) {
		dev_info(priv->device, "Version ID not available\n");
		return 0x0;
	}

	return (reg & GENMASK(15, 8)) >> 8;
}

static const struct dn200_hwif_entry {
	bool gmac;
	bool gmac4;
	bool xgmac;
	bool sriov;
	u32 min_id;
	u32 dev_id;
	const struct dn200_regs_off regs;
	const void *desc;
	const void *dma;
	const void *mac;
	const void *hwtimestamp;
	const void *mode;
	const void *tc;
	const void *mmc;
	int (*setup)(struct dn200_priv *priv);
	int (*quirks)(struct dn200_priv *priv);
} dn200_hw[] = {
	/* NOTE: New HW versions shall go to the end of this table */
	{
		.gmac = false,
		.gmac4 = false,
		.xgmac = true,
		.sriov = true,
		.min_id = DWXGMAC_CORE_2_10,
		.dev_id = DWXGMAC_ID,
		.regs = {
			.ptp_off = PTP_XGMAC_OFFSET,
			.mmc_off = MMC_XGMAC_OFFSET,
			},
		.desc = &dwxgmac210_desc_ops,
		.dma = &dwxgmac_dma_ops,
		.mac = &dwxgmac_sriov_ops,
		.hwtimestamp = &dn200_ptp,
		.mode = NULL,
		.mmc = &dwxgmac_mmc_ops,
		.setup = dwxgmac2_setup,
		.quirks = NULL,
	},
	{
		.gmac = false,
		.gmac4 = false,
		.xgmac = true,
		.min_id = DWXGMAC_CORE_2_10,
		.dev_id = DWXGMAC_ID,
		.regs = {
				.ptp_off = PTP_XGMAC_OFFSET,
				.mmc_off = MMC_XGMAC_OFFSET,
			},
		.desc = &dwxgmac210_desc_ops,
		.dma = &dwxgmac_dma_ops,
		.mac = &dwxgmac_purepf_ops,
		.hwtimestamp = &dn200_ptp,
		.mode = NULL,
		.mmc = &dwxgmac_mmc_ops,
		.setup = dwxgmac2_setup,
		.quirks = NULL,
	},
};

bool dn200_dp_hwif_id_check(void __iomem *ioaddr)
{
	static u64 chk_count;
	static bool pre_chk_state = true;

#define DP_SKIP_CHK_COUNT  100
	if (chk_count++ % DP_SKIP_CHK_COUNT == 0)
		pre_chk_state = dn200_hwif_id_check(ioaddr);

	return pre_chk_state;
}

bool dn200_hwif_id_check(void __iomem *ioaddr)
{
	u32 id, dev_id = 0;
	u32 reg_val = 0;

	reg_val = readl(ioaddr + GMAC4_VERSION);
	id = reg_val & GENMASK(7, 0);
	dev_id = (reg_val & GENMASK(15, 8)) >> 8;
	if (id < DWXGMAC_CORE_2_10 || dev_id != DWXGMAC_ID)
		return false;

	return true;
}

int dn200_hwif_init(struct dn200_priv *priv)
{
	bool needs_xgmac = priv->plat->has_xgmac;
	bool needs_gmac4 = priv->plat->has_gmac4;
	bool needs_gmac = priv->plat->has_gmac;
	bool needs_sriov = PRIV_SRIOV_SUPPORT(priv) | PRIV_IS_VF(priv);
	const struct dn200_hwif_entry *entry;
	struct mac_device_info *mac;
	bool needs_setup = true;
	u32 id, dev_id = 0;
	int i, ret;

	if (needs_gmac) {
		id = dn200_get_id(priv, GMAC_VERSION);
	} else if (needs_gmac4 || needs_xgmac) {
		id = dn200_get_id(priv, GMAC4_VERSION);
		if (needs_xgmac)
			dev_id = dn200_get_dev_id(priv, GMAC4_VERSION);
	} else {
		id = 0;
	}

	/* Save ID for later use */
	priv->chip_id = id;

	/* Lets assume some safe values first */
	priv->ptpaddr = priv->ioaddr +
	    (needs_gmac4 ? PTP_GMAC4_OFFSET : PTP_GMAC3_X_OFFSET);
	priv->mmcaddr = priv->ioaddr +
	    (needs_gmac4 ? MMC_GMAC4_OFFSET : MMC_GMAC3_X_OFFSET);

	/* Check for HW specific setup first */
	if (priv->plat->setup) {
		mac = priv->plat->setup(priv);
		needs_setup = false;
	} else {
		mac = devm_kzalloc(priv->device, sizeof(*mac), GFP_KERNEL);
	}

	if (!mac)
		return -ENOMEM;
	/* Fallback to generic HW */
	for (i = ARRAY_SIZE(dn200_hw) - 1; i >= 0; i--) {
		entry = &dn200_hw[i];
		if (needs_gmac ^ entry->gmac)
			continue;
		if (needs_gmac4 ^ entry->gmac)
			continue;
		if (needs_xgmac ^ entry->xgmac)
			continue;
		if (needs_sriov ^ entry->sriov)
			continue;
		/* Use chip_id var because some setups can override this */
		if (priv->chip_id < entry->min_id)
			continue;
		if (needs_xgmac && (dev_id ^ entry->dev_id))
			continue;

		/* Only use generic HW helpers if needed */
		mac->desc = mac->desc ? : entry->desc;
		mac->dma = mac->dma ? : entry->dma;
		mac->mac = mac->mac ? : entry->mac;
		mac->ptp = mac->ptp ? : entry->hwtimestamp;
		mac->mode = mac->mode ? : entry->mode;
		mac->tc = mac->tc ? : entry->tc;
		mac->mmc = mac->mmc ? : entry->mmc;

		mac->priv = priv;
		priv->hw = mac;
		priv->ptpaddr = priv->ioaddr + entry->regs.ptp_off;
		priv->mmcaddr = priv->ioaddr + entry->regs.mmc_off;

		/* Entry found */
		if (needs_setup) {
			ret = entry->setup(priv);
			if (ret)
				return ret;
		}

		/* Save quirks, if needed for posterior use */
		priv->hwif_quirks = entry->quirks;
		return 0;
	}

	dev_err(priv->device, "Failed to find HW IF (id=0x%x, gmac=%d/%d)\n",
		id, needs_gmac, needs_gmac4);
	return -EINVAL;
}
