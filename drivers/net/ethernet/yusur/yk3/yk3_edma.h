/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_EDMA_H
#define _YK3_EDMA_H

#include "yk3_base.h"

struct yk3_stats_base {
	u64 packets;
	u64 bytes;
	u64 errors;
	u64 drops;
};

/* Per-TC statistics for QoS monitoring */
#define YK3_MAX_TC	8

#ifndef CONFIG_MAX_SKB_FRAGS
#define CONFIG_MAX_SKB_FRAGS		(17)
#endif
#define YK3_N_MAX_SCTLIST		(CONFIG_MAX_SKB_FRAGS + 1)
#define YK3_N_MAX_SCTFRAGS		(16)
#define YK3_N_MAX_SCTSEGS		(128)
#define YK3_N_MAX_TXD		(16)

#define YK3_N_DEFAULT_DEPTH	4096
#define YK3_N_TSO_MAXSIZE	65535
#define YK3_N_TSO_MAXSEGS	720

#define YK3_N_MAX_QDEPTH		(32768U)
#define YK3_N_MIN_QDEPTH		(256U)

int yk3_edma_init(struct yk3_pdev_priv *pdev_priv);
void yk3_edma_exit(struct yk3_pdev_priv *pdev_priv);

int yk3_edma_early_init(struct yk3_pdev_priv *pdev_priv);

int yk3_edma_create_queues(struct yk3_ndev_priv *ndev_priv);
void yk3_edma_destroy_queues(struct yk3_ndev_priv *ndev_priv);

int yk3_edma_alloc_p_qbase(struct yk3_pdev_priv *pdev_priv, u16 qnum, struct yk3_queuebase *qbase);
void yk3_edma_free_p_qbase(struct yk3_pdev_priv *pdev_priv, struct yk3_queuebase qbase);

struct yk3_queuebase
yk3_edma_p_qbase_cast(struct yk3_pdev_priv *pdev_priv, enum yk3_queue_type type,
		      struct yk3_queuebase p_qbase);
int yk3_edma_get_qbase(struct yk3_pdev_priv *pdev_priv, enum yk3_queue_type type,
		       struct yk3_queuebase *qbase);
void yk3_edma_set_fx_qbase(struct yk3_pdev_priv *pdev_priv, u16 vf_id,
			   struct yk3_queuebase p_qbase);
int yk3_edma_remain_p_qbase(struct yk3_pdev_priv *pdev_priv);

int yk3_edma_start(struct yk3_ndev_priv *ndev_priv, u16 txqnum, u16 rxqnum);
void yk3_edma_stop(struct yk3_ndev_priv *ndev_priv);

netdev_tx_t yk3_edma_start_xmit(struct sk_buff *skb, struct net_device *ndev);

void yk3_edma_update_stat(struct yk3_ndev_priv *ndev_priv);

int yk3_edma_et_get_sset_count(struct yk3_ndev_priv *ndev_priv);
void yk3_edma_et_get_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail);
int yk3_edma_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data);

void yk3_edma_set_coal(struct yk3_ndev_priv *ndev_priv);
int yk3_edma_set_rx_rate(struct yk3_ndev_priv *ndev_priv, u32 rate);
int yk3_edma_set_queue_group(struct yk3_ndev_priv *ndev_priv, u16 queue, u8 qgroup);

#endif /* _YK3_EDMA_H */
