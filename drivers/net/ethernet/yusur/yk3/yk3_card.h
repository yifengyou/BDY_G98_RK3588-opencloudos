/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_CARD_H
#define _YK3_CARD_H

#include "yk3_base.h"

struct yk3_pdev_priv *yk3_get_pdev_priv(struct yk3_pci_addr *addr);
struct yk3_ndev_priv *yk3_get_ndev_priv(u32 ifindex);

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
bool yk3_vxlan_lookup_port(struct yk3_ndev_priv *ndev_priv, u16 port);
int yk3_udp_tunnel_set(struct net_device *ndev, struct udp_tunnel_info *ti, u16 op);
#endif

int yk3_card_init(struct yk3_pdev_priv *pdev_priv);
void yk3_card_exit(struct yk3_pdev_priv *pdev_priv);

#endif /* _YK3_CARD_H */
