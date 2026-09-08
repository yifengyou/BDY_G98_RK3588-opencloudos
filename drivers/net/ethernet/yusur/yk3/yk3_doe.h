/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_DOE_H
#define _YK3_DOE_H
#include "yk3_doe_kapi.h"
/******************************************************************************/
int yk3_doe_param_init(void);

int yk3_doe_init(struct yk3_pdev_priv *pdev_priv);
void yk3_doe_exit(struct yk3_pdev_priv *pdev_priv);
/******************************************************************************/
#endif /* _YK3_DOE_H */
