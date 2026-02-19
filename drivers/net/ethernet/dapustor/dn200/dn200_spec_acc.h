/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __DN200_SEPC_ACC_H__
#define __DN200_SEPC_ACC_H__

enum FEATURE_ID {
	FEAT_MTU_JUMB = 0,
	FEAT_RSS,
	FEAT_XXX,

	FEAT_MAX_ID
};

bool dn200_feat_support(struct dn200_priv *priv, enum FEATURE_ID feat_id);
int dn200_max_mtu_get(struct dn200_priv *priv, int *max_mtu, int *min_mtu);

#endif
