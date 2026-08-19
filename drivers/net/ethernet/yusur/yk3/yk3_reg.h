/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_REG_H
#define _YK3_REG_H

#include "yk3_base.h"

/* yk3 register pcie */
#define YK3_RP_PFVFID			(0x220014)
#define YK3_RP_PFID_GMASK		GENMASK(31, 16)
#define YK3_RP_VFID_GMASK		GENMASK(15, 0)

#define YK3_RP_VFX_IRQNUM(i)		(0x120000 + ((i) * 0x4))
#define YK3_RP_VFX_IRQNUM_GMASK		GENMASK(11, 0)

static inline u32 yk3_rd32(void __iomem *base, u32 off)
{
	return ioread32(base + off);
}

static inline void yk3_wr32(void __iomem *base, u32 off, u32 val)
{
	iowrite32(val, base + off);
}

static inline u64 yk3_rd64(void __iomem *base, u32 off)
{
	return (u64)yk3_rd32(base, off) | ((u64)yk3_rd32(base, off + 4) << 32);
}

static inline u64 yk3_big_rd64(void __iomem *base, u32 off)
{
	return (u64)yk3_rd32(base, off + 4) | ((u64)yk3_rd32(base, off) << 32);
}

static inline void yk3_wr64(void __iomem *base, u32 off, u64 val)
{
	yk3_wr32(base, off, val);
	yk3_wr32(base, off + 4, val >> 32);
}

static inline void yk3_big_wr64(void __iomem *base, u32 off, u64 val)
{
	yk3_wr32(base, off + 4, val);
	yk3_wr32(base, off, val >> 32);
}

static inline bool yk3_reg_err(u32 val)
{
	return (val & 0xffff0000) == 0xdead0000;
}

#endif /* _YK3_REG_H */
