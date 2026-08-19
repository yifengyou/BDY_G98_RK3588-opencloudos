/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_NDEV_H
#define _YK3_NDEV_H

#include "yk3_base.h"

int yk3_ndev_bw_limited(struct yk3_ndev_priv *ndev_priv);
int yk3_ndev_init(struct yk3_pdev_priv *pdev_priv);
void yk3_ndev_exit(struct yk3_pdev_priv *pdev_priv);

#endif /* _YK3_NDEV_H */
