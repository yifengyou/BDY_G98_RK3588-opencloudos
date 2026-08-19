/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_IRQ_H
#define _YK3_IRQ_H

#include "yk3_base.h"

#define YK3_F_IRQ_EXCLUSIVE	BIT(0)
#define YK3_F_IRQ_AFFINITY	BIT(1)

struct yk3_irq_param {
	char name[YK3_N_NAME_LEN];
	int vector;
	irq_handler_t handler;
	void *data;
	u64 flags;
};

/* Interrupt vector */
enum yk3_irq_type {
	YK3_MBOX_IRQ = 0,

	YK3_DOE_IRQ_EVQ	= 1,
	YK3_DOE_IRQ_EVQ_W = YK3_DOE_IRQ_EVQ + 0, // 1
	YK3_DOE_IRQ_EVQ_R = YK3_DOE_IRQ_EVQ + 1, // 2

	YK3_IRQ_MAX = 32,
};

int yk3_irq_request(struct yk3_pdev_priv *pdev_priv, struct yk3_irq_param *param);
void yk3_irq_free(struct yk3_pdev_priv *pdev_priv, struct yk3_irq_param *param);
void yk3_irq_set_limit(struct yk3_pdev_priv *pdev_priv, u16 limit);
void yk3_irq_add_reserve(struct yk3_pdev_priv *pdev_priv, u16 reserve);
int yk3_irq_get_freenum(struct yk3_pdev_priv *pdev_priv);
void yk3_irq_set_func_irqnum(struct yk3_pdev_priv *pdev_priv, u16 func, u16 num);
int yk3_irq_init(struct yk3_pdev_priv *pdev_priv);
void yk3_irq_exit(struct yk3_pdev_priv *pdev_priv);

void yk3_irq_sriov_init(struct yk3_pdev_priv *pdev_priv);
void yk3_irq_rdma_init(struct yk3_pdev_priv *pdev_priv);
int yk3_irq_rdma_get(struct yk3_pdev_priv *pdev_priv, int *vec_start, int *vec_num);

#endif /* _YK3_IRQ_H */
