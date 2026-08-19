/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_RSS_H_
#define _YK3_RSS_H_

void yk3_rss_indir_table_set(struct net_device *ndev, const u32 *indir, u16 qcount);
void yk3_rss_indir_table_get(struct net_device *ndev, u32 *indir);
void yk3_rss_key_set(struct net_device *ndev, const u8 *key);
void yk3_rss_key_get(struct net_device *ndev, u8 *key);
int yk3_rss_indir_table_init(struct yk3_ndev_priv *ndev_priv, u16 rxqnum);
u32 yk3_rss_key_size(void);
int yk3_rss_init(struct yk3_pdev_priv *pdev_priv);
void yk3_rss_exit(struct yk3_pdev_priv *pdev_priv);
int yk3_rss_ndev_init(struct yk3_ndev_priv *ndev_priv);
void yk3_rss_ndev_exit(struct yk3_ndev_priv *ndev_priv);

int yk3_rss_indir_table_init(struct yk3_ndev_priv *ndev_priv, u16 rxqnum);

#endif /*_YK3_RSS_H_*/
