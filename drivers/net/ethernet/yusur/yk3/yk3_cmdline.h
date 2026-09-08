/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_CMDLINE_H
#define _YK3_CMDLINE_H

#include "yk3_uapi.h"
/******************************************************************************/
#ifndef YK3_CMDLINE
#define YK3_CMDLINE	0
#endif

#ifndef YK3_CMDLINE_VALUE_COUNT
#define YK3_CMDLINE_VALUE_COUNT 16
#endif

#define YK3_CMDLINE_KEY_INVALID ":."

enum yk3_cmdline_var_type {
	YK3_CMDLINE_VAR_VALUE,
	YK3_CMDLINE_VAR_ARRAY, // only support value array, now

	YK3_CMDLINE_VAR_END
};

enum yk3_cmdline_var_flag {
	YK3_CMDLINE_VAR_ENABLE	= 0x01,
	YK3_CMDLINE_VAR_REF	= 0x01,
};

struct yk3_cmdline_var {
	u16 type;	// enum yk3_cmdline_var_type
	u16 subtype;	// NOT used now, later support other array
	u16 flag;	// enum yk3_cmdline_var_flag
	u16 count;	// array

	union {
		u64 value;
		u64 *array;
	};
};

static inline bool
is_yk3_cmdline_var_enable(struct yk3_cmdline_var *var) {
	return !!(var->flag & YK3_CMDLINE_VAR_ENABLE);
}

static inline u64
yk3_cmdline_var_value(struct yk3_cmdline_var *var) {
	return is_yk3_cmdline_var_enable(var) ? var->value : 0;
}

static inline int
yk3_cmdline_var_array_size(struct yk3_cmdline_var *var) {
	return is_yk3_cmdline_var_enable(var) ? var->count : 0;
}

static inline u64
yk3_cmdline_var_array_value(struct yk3_cmdline_var *var, int idx) {
	return (is_yk3_cmdline_var_enable(var) && idx < var->count) ? var->array[idx] : 0;
}

/******************************************************************************/
// NUMA format: is number
// CARD format: is 0000:01:00
// DEV  format: is 0000:01:00.2
//
// * {
//	hmc_size = 8192;
// }
//
// NUMA {
//	...
// }
//
// CARD {
//	hmc_address = 0xFFFFFFFF;
// }
//
// DEV {
//	hmc_huge2m = 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC;
//	hmc_huge1g = 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC;
// }
#define YK3_CMDLINE_FIELD(_symbol)	YK3_CMDLINE_FIELD_##_symbol
#include "yk3_cmdline.h.def"

// get cfg @ global
//	the cfg is read-only, NOT need release
void yk3_cmdline_cfg_getbyglobal(struct yk3_cmdline_cfg *cfg);

// get cfg @ numa or global
//	the cfg is read-only, NOT need release
void yk3_cmdline_cfg_getbynuma(int numa, struct yk3_cmdline_cfg *cfg);

// get cfg @ card or numa or global
//	the cfg is read-only, NOT need release
void yk3_cmdline_cfg_getbycard(struct yk3_card_id card, int numa, struct yk3_cmdline_cfg *cfg);

// get cfg @ bdf or card or numa or global
//	the cfg is read-only, NOT need release
void yk3_cmdline_cfg_getbydev(struct yk3_bdf bdf, int numa, struct yk3_cmdline_cfg *cfg);
/******************************************************************************/
int  yk3_cmdline_init(void);
void yk3_cmdline_fini(void);
/******************************************************************************/
#endif
