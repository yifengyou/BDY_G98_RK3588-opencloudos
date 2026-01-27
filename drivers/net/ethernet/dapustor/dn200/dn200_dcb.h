/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __DN200_DCB_H__
#define __DN200_DCB_H__

#define DN200_TRUST_DSCP			64
#define DN200_TRUST_UP				8

#include <linux/dcbnl.h>
#include "dn200.h"
void dn200_dcbnl_init(struct dn200_priv *priv, bool init);
const struct dcbnl_rtnl_ops *dn200_get_dcbnl_ops(void);
#endif
