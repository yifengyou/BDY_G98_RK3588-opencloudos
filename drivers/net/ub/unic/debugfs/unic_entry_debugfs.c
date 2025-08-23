// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include <net/page_pool/types.h>
#include <linux/debugfs.h>

#include "unic_comm_addr.h"
#include "unic_dev.h"
#include "unic_debugfs.h"
#include "unic_entry_debugfs.h"

static const char * const unic_entry_state_str[] = {
	"TO_ADD", "TO_DEL", "ACTIVE"
};

int unic_dbg_dump_ip_tbl_spec(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	u32 total_ip_tbl_size, total_ue_num;
	struct ubase_caps *ubase_caps;

	ubase_caps = ubase_get_dev_caps(unic_dev->comdev.adev);
	total_ue_num = ubase_caps->total_ue_num;
	total_ip_tbl_size = unic_dev->caps.total_ip_tbl_size;

	seq_printf(s, "total_ue_num\t: %u\n", total_ue_num);
	seq_printf(s, "total_ip_tbl_size\t: %u\n", total_ip_tbl_size);

	return 0;
}

int unic_dbg_dump_ip_tbl_list(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_comm_addr_node *ip_node;
	struct unic_addr_tbl *ip_tbl;
	struct list_head *list;
	u16 i = 0;

	seq_printf(s, "No  %-43sSTATE    IP_MASK\n", "IP_ADDR");

	ip_tbl = &unic_dev->vport.addr_tbl;
	list = &ip_tbl->ip_list;
	spin_lock_bh(&ip_tbl->ip_list_lock);
	list_for_each_entry(ip_node, list, node) {
		seq_printf(s, "%-4d", i++);
		seq_printf(s, "%-43pI6c", &ip_node->ip_addr.s6_addr);
		seq_printf(s, "%-9s", unic_entry_state_str[ip_node->state]);
		seq_printf(s, "%-3u", ip_node->node_mask);

		seq_puts(s, "\n");
	}
	spin_unlock_bh(&ip_tbl->ip_list_lock);

	return 0;
}
