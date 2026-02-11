/* SPDX-License-Identifier: GPL-2.0 */
/* Huawei UNIC Linux driver
 * Copyright (c) 2024-2025 Hisilicon Limited.
 *
 */

#ifndef UNIC_MAC_H
#define UNIC_MAC_H

enum unic_mac_addr_type {
	UNIC_MAC_ADDR_UC,
	UNIC_MAC_ADDR_MC,
};

int unic_cfg_mac_address(struct unic_dev *unic_dev, u8 *mac_addr);
int unic_add_uc_mac(struct net_device *netdev, const u8 *mac_addr);
int unic_del_uc_mac(struct net_device *netdev, const u8 *mac_addr);
int unic_add_mc_mac(struct net_device *netdev, const u8 *mac_addr);
int unic_del_mc_mac(struct net_device *netdev, const u8 *mac_addr);
int unic_init_mac_addr(struct unic_dev *unic_dev);
void unic_uninit_mac_addr(struct unic_dev *unic_dev);
void unic_sync_mac_table(struct unic_dev *unic_dev);
void unic_uninit_mac_table(struct unic_dev *unic_dev);
void unic_deactivate_mac_table(struct unic_dev *unic_dev);
void unic_activate_mac_table(struct unic_dev *unic_dev);

#endif
