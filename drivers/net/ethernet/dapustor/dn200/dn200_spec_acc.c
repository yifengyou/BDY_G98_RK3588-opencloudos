// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Wang Ai-Yong <aiyong@dapustor.com>
 *
 * Get special configurations for dn200
 */

#include "common.h"
#include "dn200_spec_acc.h"
#include "dn200_spec_def.h"
#include "dn200_sriov.h"
#include "dn200.h"

static bool feat_has_priv(struct dn200_priv *priv, enum FEATURE_ID feat_id)
{
	int feat_priv = 0;

	WARN_ON(spec_table[feat_id].feat_id != feat_id);

	if (!PRIV_SRIOV_SUPPORT(priv))
		feat_priv = FEAT_PRIV_PURE_PF;
	else if (PRIV_SRIOV_SUPPORT(priv))
		feat_priv = FEAT_PRIV_SRIOV_PF;
	else if (PRIV_IS_VF(priv))
		feat_priv = FEAT_PRIV_VF;
	else
		dev_err(priv->device, "%s: ERROR: don't exist valid driver type.\n",
			__func__);

	return ((spec_table[feat_id].feat_priv & feat_priv) != 0);
}

static enum DRV_TYPE drv_type_get(struct dn200_priv *priv)
{
	if (!PRIV_SRIOV_SUPPORT(priv) && !PRIV_IS_VF(priv))
		return DRV_PURE_PF;

	if (PRIV_SRIOV_SUPPORT(priv))
		return DRV_SRIOV_PF;

	if (PRIV_IS_VF(priv))
		return DRV_VF;

	/* Can't reach here, should have valid driver type */
	WARN_ON(true);
	return DRV_PURE_PF;
}

/**
 * dn200_feat_support - Get the feature supported status
 * @feat_id: feature id
 * @priv: driver private structure
 */
bool dn200_feat_support(struct dn200_priv *priv, enum FEATURE_ID feat_id)
{
	if (feat_has_priv(priv, feat_id))
		return true;

	return false;
}

/**
 * dn200_max_mtu_get - Get max mtu
 * @priv: driver private structure
 * @mtu: max mtu
 */
int dn200_max_mtu_get(struct dn200_priv *priv, int *max_mtu, int *min_mtu)
{
	enum FEATURE_ID feat_id = FEAT_MTU_JUMB;
	enum DRV_TYPE drv_type = drv_type_get(priv);
	int pf_id = priv->plat_ex->pf_id;
	struct feat_mtu_spec *mtu_spec = spec_table[feat_id].feat_spec;

	if (feat_has_priv(priv, feat_id)) {
		*max_mtu = mtu_spec[drv_type].max_mtu[pf_id];
		*min_mtu = mtu_spec[drv_type].min_mtu;
		return 0;
	}

	return -EACCES;
}
