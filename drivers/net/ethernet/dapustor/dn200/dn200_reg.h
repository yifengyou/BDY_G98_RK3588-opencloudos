/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 *
 */
#ifndef __DN200_REG_H__
#define __DN200_REG_H__
#include "common.h"

#define IATU_IATU_MAX_REGION    32
#define IATU_OB_TYPE            GENMASK(4, 0)
#define IATU_OB_REGION_EN       BIT(31)
#define IATU_OB_FUNC_BYPASS     BIT(19)

void dn200_iatu_tbl_entry_write(void __iomem *ioaddr,
				struct dn200_iatu_tbl_entry *iatu_entry,
				u8 iatu_index, bool enable);
void dn200_iatu_tgt_addr_updt(void __iomem *ioaddr, u8 iatu_index,
			      u64 tgt_addr);

void dn200_enable_tx_dma_irq(void __iomem *ioaddr, u32 chan,
				    struct mac_device_info *hw);
void dn200_disable_tx_dma_irq(void __iomem *ioaddr, u32 chan,
				     struct mac_device_info *hw);
#endif
