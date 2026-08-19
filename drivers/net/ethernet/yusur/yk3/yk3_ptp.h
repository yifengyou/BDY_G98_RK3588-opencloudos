/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _YK3_PTP_H
#define _YK3_PTP_H

#include "yk3_base.h"

int yk3_ptp_init(struct yk3_pdev_priv *pdev_priv);
void yk3_ptp_exit(struct yk3_pdev_priv *pdev_priv);

int yk3_ptp_enable(struct yk3_pdev_priv *pdev_priv);
void yk3_ptp_disable(struct yk3_pdev_priv *pdev_priv);

void yk3_ptp_tod_refl_clear(struct yk3_pdev_priv *pdev_priv);

void yk3_ptp_set_tx_ts(struct yk3_pdev_priv *pdev_priv, struct sk_buff *skb);
void yk3_ptp_set_rx_ts(struct yk3_pdev_priv *pdev_priv, struct sk_buff *skb, u64 rx_ns);

int yk3_ptp_hwtstamp_set(struct yk3_pdev_priv *pdev_priv, struct hwtstamp_config *hwts_config);
int yk3_ptp_hwtstamp_get(struct yk3_pdev_priv *pdev_priv, struct hwtstamp_config *hwts_config);

#ifdef YK3_HAVE_KERNEL_ETHTOOL_TS_INFO
int yk3_ptp_get_ts_info(struct yk3_pdev_priv *pdev_priv, struct kernel_ethtool_ts_info *eti);
#else
int yk3_ptp_get_ts_info(struct yk3_pdev_priv *pdev_priv, struct ethtool_ts_info *eti);
#endif
#endif /* _YK3_PTP_H */
