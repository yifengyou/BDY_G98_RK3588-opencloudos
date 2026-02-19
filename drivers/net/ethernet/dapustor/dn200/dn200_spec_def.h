/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Wang Ai-Yong <aiyong@dapustor.com>
 *
 * Get special configurations for dn200
 */

#ifndef __DN200_SEPC_DEF_H__
#define __DN200_SEPC_DEF_H__

#include "common.h"

/* feature privileges */
#define FEAT_PRIV_PURE_PF	0x01	/* Feature can run on pure pf (Pure PF don't support SRIOV) */
#define FEAT_PRIV_SRIOV_PF	0x20	/* Feature can run on SRIOV pf */
#define FEAT_PRIV_VF		0x40	/* Feature can run on vf */
#define FEAT_PRIV_NONE		0x0	/* Feature can't run on any condition */

/* MTU or Jumbo feature for every driver type */
const struct feat_mtu_spec {
	int max_mtu[XGE_NUM];
	int min_mtu;
} mtu_spec[DRV_TYPE_MAX] = {
	[DRV_PURE_PF] = {
			 /*Hardware scheduling takes up space,a certain space must be reserved */
			 .max_mtu[0] = 9600,
			 .max_mtu[1] = 9600,
			 .max_mtu[2] = 9600,	/* 9600 */
			 .max_mtu[3] = 9600,	/* 9600 */
			 .min_mtu = 68,
			  },
	[DRV_SRIOV_PF] = {
			  .max_mtu[0] = 9600,
			  .max_mtu[1] = 9600,
			  .max_mtu[2] = 9600,	/* 9600 */
			  .max_mtu[3] = 9600,	/* 9600 */
			  .min_mtu = 68,
			   },
	[DRV_VF] = {
				/* 2KB, VF FIFO size is 3KB(refer DN200_01_VF_TX_FIFO_SIZE),
				 * 2K mtu TSO works fine
				 */
				.max_mtu[0] = 1500,
				.max_mtu[1] = 1500,	/* 2KB */
				.max_mtu[2] = 9600,	/* 9600 */
				.max_mtu[3] = 9600,	/* 9600 */
				.min_mtu = 68,
			},
};

/* XXX feature */

const struct dn200_feat_spec {
	int feat_id;
	int feat_priv;
	void *feat_spec;
} spec_table[FEAT_MAX_ID] = {
	{
		.feat_id = FEAT_MTU_JUMB,
		.feat_priv = FEAT_PRIV_PURE_PF | FEAT_PRIV_SRIOV_PF | FEAT_PRIV_VF,
		.feat_spec = (void *)mtu_spec,
	},
	{
		.feat_id = FEAT_RSS,
		.feat_priv = FEAT_PRIV_PURE_PF | FEAT_PRIV_SRIOV_PF,
		.feat_spec = NULL,
	},
	{
		.feat_id = FEAT_XXX,
		.feat_priv = FEAT_PRIV_PURE_PF,
		.feat_spec = NULL,
	}
};

#endif /* __DN200_SEPC_DEF_H__ */
