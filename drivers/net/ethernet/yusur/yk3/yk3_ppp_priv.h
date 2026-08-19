/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_PPP_PRIV_H
#define _YK3_PPP_PRIV_H

#include "yk3_ppp.h"

#define YK3_PPP_BAR				(0)
#define YK3_PPP_ALL_BASE			(0x01c00000)
#define YK3_PPP_LITE_BASE			(0x01e00000)

/* interactive variable */
#define YK3_PPP_IVAR_SECTION			(0x1c) // ppp_state.RESV0_REG
#define YK3_PPP_IVAR_CODE_MAGIC			(0x76)
#define YK3_PPP_IVAR_MAX			(24)
#define YK3_PPP_IVAR_VALUE_MAX_LEN		(12)

/* parser vliw */
#define YK3_PPP_PSR_VLIW_WADDR			(0x300c)
#define YK3_PPP_PSR_VLIW_DATA			(0x3010)
#define YK3_PPP_PSR_VLIW_RADDR			(0x3014)

/* action ram */
#define YK3_PPP_ACTION_PTP			(15)
#define YK3_PPP_ACTION_MAX_SIZE			(1024)
#define YK3_PPP_ACTION_WADDR			(0x7000)
#define YK3_PPP_ACTION_DATA			(0x7004)
#define YK3_PPP_ACTION_RADDR			(0x7008)

/* priority map */
#define YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN0	(0x8020)
#define YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN1	(0x8024)
#define YK3_PPP_PKTTX0_PRI_WADDR		(0x8164)
#define YK3_PPP_PKTTX0_PRI_DATA			(0x8168)
#define YK3_PPP_PTTTX0_PRI_RADDR		(0x816c)
#define YK3_PPP_PKTTX1_PRI_WADDR		(0x8170)
#define YK3_PPP_PKTTX1_PRI_DATA			(0x8174)
#define YK3_PPP_PTTTX1_PRI_RADDR		(0x8178)

struct action_entry_raw {
	u8 raw[15];
};

/* 映射硬件ActionRAM条目 */
struct action_entry {
	union {
		u16 raw[6];
		struct {
			u16 val		: 8;
			u16 m		: 1;
			u16 pri		: 3;
			u16 v		: 1;
			u16 unused	: 3;
		} field[6];
	} sysmeta;

	union {
		u64 raw;
		struct {
			u64 ma0_vport	: 4;
			u64 ma0_p	: 1;
			u64 ma0_pri	: 3;
			u64 ma0_v	: 1;
			u64 ma1_vport	: 4;
			u64 ma1_p	: 1;
			u64 ma1_pri	: 3;
			u64 ma1_v	: 1;
			u64 ma2_vport	: 4;
			u64 ma2_p	: 1;
			u64 ma2_pri	: 3;
			u64 ma2_v	: 1;
			u64 mir_idx	: 8;
			u64 mir_vport	: 2;
			u64 mir_p	: 1;
			u64 mir_pri	: 3;
			u64 mir_v	: 1;
			u64 unused	: 22;
		};
	} chn;
};

struct ivar_section {
	u32 addr;
	u32 len;
};

struct ivar {
	u8 code;
	u8 type;
	u8 length;
	u8 value[YK3_PPP_IVAR_VALUE_MAX_LEN];
};

struct vliw_branch {
	u8 next_type;
	u16 mask;
	u16 protocol_num;
} __packed;

struct vliw {
	union {
		u32 raw[11];
		struct {
			u32 part0;			// 0-31bit
			u32 part1;			// 32-63bit
			u32 part2;			// 64-95bit
			u32 part3;			// 96-127bit
			struct vliw_branch branch[4];	// 128-287bit
			u32 part9;			// 288-319bit
			u32 part10;			// 320-351bit
		};
	};
};

struct yk3_ppp {
	void __iomem *addr;
};

#endif /* _YK3_PPP_PRIV_H */
