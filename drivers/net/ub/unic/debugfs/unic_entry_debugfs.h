/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#ifndef __UNIC_ENTRY_DEBUGFS_H__
#define __UNIC_ENTRY_DEBUGFS_H__

#include <linux/in6.h>
#include <ub/ubase/ubase_comm_debugfs.h>

#include "unic_comm_addr.h"

#define UNIC_BITMAP_LEN		8
#define UNIC_DBG_MAC_NUM	16
#define UNIC_QUERY_MAC_LEN	(sizeof(struct unic_dbg_mac_head) + \
				 sizeof(struct unic_dbg_mac_entry) * UNIC_DBG_MAC_NUM)

struct unic_dbg_mac_entry {
	u8 mac_addr[ETH_ALEN];
	__le32 eport;
};

struct unic_dbg_mac_head {
	__le32 mac_idx;
	u8 cur_mac_cnt;
	u8 rsv[3];
	struct unic_dbg_mac_entry mac_entry[];
};

struct unic_dbg_comm_addr_node {
	struct list_head	node;
	u16			ue_id;
	u32			ue_bitmap[UNIC_BITMAP_LEN];
	u32			port_bitmap;
	union {
		u8		guid[UNIC_ADDR_LEN];
		struct {
			struct	in6_addr ip_addr;
			u32	extend_info;
		};
		struct {
			u8 mac_addr[ETH_ALEN];
			u32	eport;
	};
	};
};

int unic_dbg_dump_ip_tbl_spec(struct seq_file *s, void *data);
int unic_dbg_dump_ip_tbl_list(struct seq_file *s, void *data);
int unic_dbg_dump_mac_tbl_list_hw(struct seq_file *s, void *data);

#endif /* _UNIC_ENTRY_DEBUGFS_H */
