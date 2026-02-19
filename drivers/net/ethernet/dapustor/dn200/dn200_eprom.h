/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Wang Peixiang <peixiang@dapustor.com>
 *
 * Update firmware for DN200
 */

#ifndef __DN200_EPROM_H__
#define __DN200_EPROM_H__

struct mac_addr_t {
	u8 addr[6];
} __packed;

union product_ver_t {
	u64 all;
	struct {
		u64 ver:16;
		u64 minor_ver:8;
		u64 major_ver:8;
		u64 prod_class:8;
		u64 reserved:24;
	} b;
};

struct product_info_t {
	u32 magic_num;
	struct mac_addr_t mac_addr[6];
	u8 sn[32];
	union product_ver_t prod_ver;
	u8 flag[4];
	u8 mn[64];
	u8 mac_bitmap;
	u8 pn[32];
} __packed;

#define PRODUCT_MAGIC_NUMBER            (0x5A6C7D8E)

#define FW_PACKAGE_FILE_NAME_SIZE   32
#define HEADER_FILE_ITEM_COUNT      21
#define HEADER_FILE_MAGIC			0xAC69CF5D

struct header_file_entry_t {
	u8 name[FW_PACKAGE_FILE_NAME_SIZE];
	u32 crc;
	u32 len;
	u32 real_len;
	u8 bmp;
} __packed;

struct header_file_t {
	u32 magic_num;
	u8 count;
	u8 bmp;
	u16 preloader_sz;
	u32 loader_offset[2];
	u32 loader_sz;
	u32 body_crc;
	struct header_file_entry_t entry[HEADER_FILE_ITEM_COUNT];
	u8 rsv[30];
	u32 board_id;
	u8 fwrev[16];
	u8 plat_id;
	u32 crc;
} __packed;

enum _board_type_e {
	board_type_null,
	board_type_boot_raid,
	board_type_xgmac_combo,
	board_type_xgmac_only,
	board_type_gmac_combo, //gmac +combo*4
	board_type_fengshen1,
	board_type_ram, // for ramdisk test
};

int dn200_load_firmware(struct net_device *netdev, struct ethtool_flash *efl);
void dn200_get_mac_from_firmware(struct dn200_priv *priv, struct dn200_resources *res);

#endif
