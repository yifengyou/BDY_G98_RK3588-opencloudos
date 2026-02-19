// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Dapustor Corporation .
 * dn200 features(e.g. iatu) register read write.
 */

#include "dn200.h"
#include "dwxgmac_comm.h"
#include "dn200_iatu.h"
#include "dn200_reg.h"

void dn200_iatu_tbl_entry_write(void __iomem *ioaddr,
				struct dn200_iatu_tbl_entry *iatu_entry,
				u8 iatu_index, bool enable)
{
	u32 type = 0;
	u32 region_off = (u32)(iatu_index * 0x200);
	void __iomem *iatu_reg_base = ioaddr + region_off;
	u64 base_addr = iatu_entry->base_addr;
	u8 pf_nmb = iatu_entry->pf_id;
	bool is_vf = iatu_entry->is_vf;
	u8 vf_nmb = iatu_entry->vf_offset;
	u32 limit = iatu_entry->limit_mask;
	u64 tgt_addr = iatu_entry->tgt_addr;

	pr_debug("PF[%d].VF[%d] Configure Outbound: region:%d, base 0x%llx to target 0x%llx limit 0x%llx\n",
		iatu_entry->pf_id, iatu_entry->vf_offset, iatu_index,
		iatu_entry->base_addr, iatu_entry->tgt_addr,
		iatu_entry->limit_mask);

	if (iatu_index >= IATU_IATU_MAX_REGION) {
		pr_err("HW supports only max 32 address region, err occur in iatu write!\n");
		return;
	}
	if (!enable)
		writel(0x7fffffff & readl(iatu_reg_base + 0x04), iatu_reg_base + 0x04);
	/* 1. Setup the Region Base and Limit Address Registers. */
	writel((u32)(base_addr), iatu_reg_base + 0x08);
	writel((u32)(base_addr >> 32), iatu_reg_base + 0x0C);
	writel((u32)limit, iatu_reg_base + 0x10);

	/* 2. Setup the Target Address Registers. */
	writel((u32)(tgt_addr), iatu_reg_base + 0x14);
	writel((u32)(tgt_addr >> 32), iatu_reg_base + 0x18);

	/* 3. Configure the region through the Region Control 1 Register. */
	writel(((pf_nmb << 20) & GENMASK(22, 20)) + (type & IATU_OB_TYPE),
	       iatu_reg_base + 0x00);
	if (is_vf)
		writel((1 << 31) + vf_nmb, iatu_reg_base + 0x1C);

	/* 4. Enable the region and set BDF from application. */
	if (enable)
		writel(IATU_OB_REGION_EN, iatu_reg_base + 0x04);
}

void dn200_iatu_tgt_addr_updt(void __iomem *ioaddr, u8 iatu_index,
			      u64 tgt_addr)
{
	u32 region_off = (u32) (iatu_index * 0x200);
	void __iomem *iatu_reg_base = ioaddr + region_off;

	pr_debug("%s, %d, Configure Outbound: iatu_index:%d, target 0x%llx\n",
		 __func__, __LINE__, iatu_index, tgt_addr);

	if (iatu_index >= IATU_IATU_MAX_REGION) {
		pr_info("HW supports only max 32 address region, err occur in iatu update!\n");
		return;
	}

	/* Setup the Target Address Registers. */
	writel((u32)(tgt_addr), iatu_reg_base + 0x14);
	writel((u32)(tgt_addr >> 32), iatu_reg_base + 0x18);
}

void dn200_enable_tx_dma_irq(void __iomem *ioaddr, u32 chan,
				    struct mac_device_info *hw)
{
	u32 value;

	chan += DN200_RXQ_START_GET(hw);
	value = XGMAC_DMA_INT_DEFAULT_EN | XGMAC_DMA_INT_DEFAULT_TX;

	writel(value, ioaddr + XGMAC_DMA_CH_INT_EN(chan));
}

void dn200_disable_tx_dma_irq(void __iomem *ioaddr, u32 chan,
				     struct mac_device_info *hw)
{
	u32 value;

	chan += DN200_RXQ_START_GET(hw);
	value = XGMAC_DMA_INT_DEFAULT_EN & (~XGMAC_DMA_INT_DEFAULT_TX);

	writel(value, ioaddr + XGMAC_DMA_CH_INT_EN(chan));
}
