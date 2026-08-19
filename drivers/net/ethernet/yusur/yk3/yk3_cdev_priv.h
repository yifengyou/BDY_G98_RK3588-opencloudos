/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_CDEV_PRIV_H
#define _YK3_CDEV_PRIV_H

#include "yk3_base.h"
#include "yk3_dmamap.h"

#ifdef YK3_HAVE_IOMMU_PRESENT
#define yk3_iommu_present(dev) iommu_present((dev)->bus)
#else
#define yk3_iommu_present(dev) device_iommu_mapped(dev)
#endif

struct yk3_cdev_priv {
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_ndev_priv *ndev_priv;
	bool umd_enabled;

	/* config save */
	struct list_head cfgsave_head;
	u64 cfgchange_flags;

	/* dma map */
	bool domain;
	struct yk3_dmamap_table *dmamap_table;

	/* led state */
	bool phys_id_active;
};

typedef long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);

struct yk3_cfgsave {
	struct list_head node;
	int type;
	u8 data[];
};

struct yk3_page_map {
	u64 vaddr;
	size_t npages;
	struct page **pages;
};

#define YK3_RSS_FLOW_MAX 4

struct yk3_rss_entry {
	u32 flow_type;
	u64 data;
};

struct yk3_rss_cfg {
	u32 num;
	struct yk3_rss_entry entry[YK3_RSS_FLOW_MAX];
};

#endif /* _YK3_CDEV_PRIV_H */
