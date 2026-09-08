/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_DEBUG_H
#define _YK3_DEBUG_H

#include "yk3_dump.h"
/******************************************************************************/
#ifndef YK3_DEBUG
#define YK3_DEBUG 0
#endif

#define yk3_emerg(f, arg...) pr_emerg("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_alert(f, arg...) pr_alert("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_crit(f, arg...) pr_crit("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_err(f, arg...) pr_err("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_warn(f, arg...) pr_warn("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_info(f, arg...) pr_info("%s: " f, YK3_HW_NAME, ##arg)
#define yk3_debug(f, arg...) \
	pr_debug("%s:[%s:%d]: " f, YK3_HW_NAME, __func__, __LINE__, ##arg)

#define yk3_net_crit(f, arg...) \
	netdev_crit(ndev_priv->ndev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_net_err(f, arg...) \
	netdev_err(ndev_priv->ndev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_net_warn(f, arg...) \
	netdev_warn(ndev_priv->ndev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_net_info(f, arg...) \
	netdev_info(ndev_priv->ndev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_net_debug(f, arg...) \
	netdev_dbg(ndev_priv->ndev, "%s:[%s:%d]: " f, YK3_HW_NAME, __func__, \
		   __LINE__, ##arg)

#define yk3_dev_crit(f, arg...) \
	dev_crit(pdev_priv->dev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_dev_err(f, arg...) \
	dev_err(pdev_priv->dev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_dev_warn(f, arg...) \
	dev_warn(pdev_priv->dev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_dev_info(f, arg...) \
	dev_info(pdev_priv->dev, "%s: " f, YK3_HW_NAME, ##arg)
#define yk3_dev_debug(f, arg...) \
	dev_dbg(pdev_priv->dev, "%s:[%s:%d]: " f, YK3_HW_NAME, __func__, \
		__LINE__, ##arg)
/******************************************************************************/
void yk3_debug_init(void);
void yk3_debug_exit(void);

int  yk3_debug_card_init(struct yk3_card *card);
void yk3_debug_card_exit(struct yk3_card *card);

int  yk3_debug_pdev_init(struct yk3_pdev_priv *pdev_priv);
void yk3_debug_pdev_exit(struct yk3_pdev_priv *pdev_priv);

int  yk3_debug_ndev_init(struct yk3_ndev_priv *ndev_priv);
void yk3_debug_ndev_exit(struct yk3_ndev_priv *ndev_priv);

int  yk3_debug_sriov_init(struct yk3_pdev_priv *pdev_priv);
void yk3_debug_sriov_exit(struct yk3_pdev_priv *pdev_priv);
/******************************************************************************/
#endif /* _YK3_DEBUG_H */
