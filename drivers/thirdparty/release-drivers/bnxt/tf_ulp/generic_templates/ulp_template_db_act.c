// SPDX-License-Identifier: BSD-3-Clause
/* Copyright(c) 2014-2025 Broadcom
 * All rights reserved.
 */

#include "ulp_template_db_enum.h"
#include "ulp_template_db_field.h"
#include "ulp_template_struct.h"
#include "ulp_template_db_tbl.h"

/* Array for the act matcher list */
struct bnxt_ulp_act_match_info ulp_act_match_list[] = {
	[1] = {
	.act_bitmap = { .bits =
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[2] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[3] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[4] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_ACT_BIT_VXLAN_DECAP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[5] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_DECAP |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[6] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_DST |
		BNXT_ULP_ACT_BIT_SET_TP_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[7] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_DST |
		BNXT_ULP_ACT_BIT_SET_TP_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[8] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[9] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[10] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_IP_DECAP |
		BNXT_ULP_ACT_BIT_L2_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[11] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_QUEUE |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[12] = {
	.act_bitmap = { .bits =
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[13] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[14] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[15] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_ACT_BIT_VXLAN_DECAP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[16] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_DECAP |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[17] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_DST |
		BNXT_ULP_ACT_BIT_SET_TP_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[18] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_DST |
		BNXT_ULP_ACT_BIT_SET_TP_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[19] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[20] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[21] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_IP_DECAP |
		BNXT_ULP_ACT_BIT_L2_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[22] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_QUEUE |
		BNXT_ULP_FLOW_DIR_BITMASK_ING },
	.act_tid = 1
	},
	[23] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[24] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[25] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[26] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_ENCAP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[27] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[28] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[29] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[30] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[31] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[32] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[33] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[34] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[35] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[36] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[37] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[38] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[39] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[40] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[41] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[42] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[43] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[44] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[45] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_GOTO_CHAIN |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[46] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_GOTO_CHAIN |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[47] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[48] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[49] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_L2_DECAP |
		BNXT_ULP_ACT_BIT_IP_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[50] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_L2_DECAP |
		BNXT_ULP_ACT_BIT_IP_ENCAP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[51] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[52] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[53] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[54] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VXLAN_ENCAP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[55] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[56] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[57] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[58] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[59] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[60] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[61] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[62] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[63] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[64] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[65] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[66] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[67] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[68] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_VF_TO_VF |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[69] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[70] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV4_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[71] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[72] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_SET_MAC_SRC |
		BNXT_ULP_ACT_BIT_SET_MAC_DST |
		BNXT_ULP_ACT_BIT_SET_IPV6_SRC |
		BNXT_ULP_ACT_BIT_SET_TP_SRC |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[73] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_GOTO_CHAIN |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[74] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_GOTO_CHAIN |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[75] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[76] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_DROP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[77] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_L2_DECAP |
		BNXT_ULP_ACT_BIT_IP_ENCAP |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[78] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_COUNT |
		BNXT_ULP_ACT_BIT_L2_DECAP |
		BNXT_ULP_ACT_BIT_IP_ENCAP |
		BNXT_ULP_ACT_BIT_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 2
	},
	[79] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_METER_PROFILE |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 3
	},
	[80] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_SHARED_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 3
	},
	[81] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_DELETE |
		BNXT_ULP_ACT_BIT_METER_PROFILE |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 3
	},
	[82] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_DELETE |
		BNXT_ULP_ACT_BIT_SHARED_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 3
	},
	[83] = {
	.act_bitmap = { .bits =
		BNXT_ULP_ACT_BIT_UPDATE |
		BNXT_ULP_ACT_BIT_SHARED_METER |
		BNXT_ULP_FLOW_DIR_BITMASK_EGR },
	.act_tid = 3
	}
};

