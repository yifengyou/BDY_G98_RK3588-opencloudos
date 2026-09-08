/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_CHIP_H
#define _YK3_CHIP_H

#include "yk3_base.h"
/******************************************************************************/
// 0xc   doe_work_mode   bit0-5:   aie,laie,cie,mie,hie,lhie
// 0x330 clk_gate_en     bit0-6:   aie,laie,cie,mie,hie,miu,lhie # fuck !!!
// 0xb4  doe_protect_ack bit0-5:   aie,laie,cie,mie,hie,lhie(np)
//                       bit16-21: aie,laie,cie,mie,hie,lhie(host)
enum yk3_doe_xie {
	// keep sort !!!
	YK3_DOE_AIE	= 0,
	YK3_DOE_LAIE	= 1,
	YK3_DOE_CIE	= 2,
	YK3_DOE_MIE	= 3,
	YK3_DOE_HIE	= 4,
	YK3_DOE_LHIE	= 5,

	YK3_DOE_XIE_END
};

static inline bool
is_good_doe_xie(int xie) {
	return is_good_enum(xie, YK3_DOE_XIE_END);
}

enum yk3_doe_cache_mode {
	YK3_DOE_CACHE_MODE_LONG	= 0,	// is short = false
	YK3_DOE_CACHE_MODE_SHORT = 1,	// is short = true

	YK3_DOE_CACHE_MODE_END,
	YK3_DOE_CACHE_MODE_DEFT = YK3_DOE_CACHE_MODE_LONG,
};

struct yk3_chip_doe_spec {
	u32 version;
	u32 protect_ack;
	u32 hash_table_limit;
	u32 index_sram_size;	// Unit: index(4B)
	u32 ddr_channel;
	u32 cache_entry_count[YK3_DOE_XIE_END][YK3_DOE_CACHE_MODE_END];
};

/******************************************************************************/
struct yk3_chip_spec {
	struct yk3_chip_doe_spec doe;
};

extern const struct yk3_chip_spec yk3_chip_specs[];
/******************************************************************************/
#endif
