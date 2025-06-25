/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2025 Beijing WangXun Technology Co., Ltd. */
#ifndef _TXGBE_BP_H_
#define _TXGBE_BP_H_

#include "txgbe.h"
#include "txgbe_type.h"
#include "txgbe_hw.h"

typedef enum {
	ABILITY_1000BASE_KX,
	ABILITY_10GBASE_KX4,
	ABILITY_10GBASE_KR,
	ABILITY_40GBASE_KR4,
	ABILITY_40GBASE_CR4,
	ABILITY_100GBASE_CR10,
	ABILITY_100GBASE_KP4,
	ABILITY_100GBASE_KR4,
	ABILITY_100GBASE_CR4,
	ABILITY_25GBASE_KRCR_S,
	ABILITY_25GBASE_KRCR,
	ABILITY_MAX,
} ability_filed_encding;

/* Backplane AN73 Base Page Ability struct*/
typedef struct TBKPAN73ABILITY {
	unsigned int nextPage;    //Next Page (bit0)
	unsigned int linkAbility; //Link Ability (bit[7:0])
	unsigned int fecAbility;  //FEC Request (bit1), FEC Enable  (bit0)
	unsigned int currentLinkMode; //current link mode for local device
} bkpan73ability;

#define kr_dbg(KR_MODE, fmt, arg...) \
	do { \
		if (KR_MODE) \
			e_dev_info(fmt, ##arg); \
	} while (0)

void txgbe_bp_down_event(struct txgbe_adapter *adapter);
void txgbe_bp_watchdog_event(struct txgbe_adapter *adapter);
int txgbe_bp_mode_setting(struct txgbe_adapter *adapter);
void txgbe_bp_close_protect(struct txgbe_adapter *adapter);
int handle_bkp_an73_flow(unsigned char bp_link_mode, struct txgbe_adapter *adapter);
int get_bkp_an73_ability(bkpan73ability *pt_bkp_an73_ability, unsigned char byLinkPartner,
			 struct txgbe_adapter *adapter);
int chk_bkp_an73_ability(bkpan73ability tBkpAn73Ability, bkpan73ability tLpBkpAn73Ability,
			 struct txgbe_adapter *adapter);
#endif

