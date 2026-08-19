/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_QSET_H
#define _YK3_QSET_H

#include "yk3_base.h"

struct yk3_qset_table {
	struct idr idr;
	struct idr rep_idr;
	spinlock_t slock;	/* for idr */
	u16 idr_start;
	u16 idr_end;
	u16 rep_idr_start;
	u16 rep_idr_end;
};

int yk3_qset_init(struct yk3_pdev_priv *pdev_priv);
void yk3_qset_exit(struct yk3_pdev_priv *pdev_priv);

int yk3_qsetid_alloc(struct yk3_pdev_priv *pdev_priv, enum yk3_ndev_type type);
void yk3_qsetid_free(struct yk3_pdev_priv *pdev_priv, u16 qsetid);
int yk3_qsetid_get_peer(struct yk3_pdev_priv *pdev_priv, struct yk3_ndev_priv *ndev_priv);
int yk3_qset_start(struct yk3_ndev_priv *ndev_priv, u16 txqnum, u16 rxqnum);
void yk3_qset_stop(struct yk3_ndev_priv *ndev_priv);

#endif /* _YK3_QSET_H */
