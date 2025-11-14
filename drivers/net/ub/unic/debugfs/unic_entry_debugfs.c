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

static int unic_dbg_check_dev_state(struct unic_dev *unic_dev)
{
	if (__unic_resetting(unic_dev))
		return -EBUSY;

	return 0;
}

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

static int unic_common_query_addr_list(struct unic_dev *unic_dev, u32 total_size,
				       u32 size, struct list_head *list,
				       int (*query_list)(struct unic_dev *, u32 *,
							 struct list_head *,
							 bool *complete))
{
#define UNIC_LOOP_COUNT(total_size, size) ((total_size) / (size) + 1)

	u32 idx = 0, cnt = 0;
	bool complete;
	int ret = 0;

	while (cnt < UNIC_LOOP_COUNT(total_size, size)) {
		complete = false;
		ret = query_list(unic_dev, &idx, list, &complete);
		if (ret) {
			unic_err(unic_dev,
				 "failed to query addr list, ret = %d.\n", ret);
			break;
		}

		if (complete)
			break;
		cnt++;
	}

	return ret == -EPERM ? -EOPNOTSUPP : ret;
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

static int unic_query_mac_list_hw(struct unic_dev *unic_dev, u32 *mac_idx,
				  struct list_head *list, bool *complete)
{
	struct unic_dbg_comm_addr_node *mac_node;
	struct unic_dbg_mac_entry *mac_entry;
	struct unic_dbg_mac_head req = {0};
	struct unic_dbg_mac_head *head;
	struct ubase_cmd_buf in, out;
	int ret;
	u8 i;

	head = kzalloc(UNIC_QUERY_MAC_LEN, GFP_ATOMIC);
	if (!head)
		return -ENOMEM;

	mac_entry = head->mac_entry;
	req.mac_idx = cpu_to_le32(*mac_idx);

	ubase_fill_inout_buf(&in, UBASE_OPC_QUERY_MAC_TBL, true, sizeof(req),
			     &req);
	ubase_fill_inout_buf(&out, UBASE_OPC_QUERY_MAC_TBL, true,
			     UNIC_QUERY_MAC_LEN, head);
	ret = ubase_cmd_send_inout(unic_dev->comdev.adev, &in, &out);
	if (ret) {
		unic_err(unic_dev,
			 "failed to query mac hw tbl, ret = %d.\n", ret);
		goto err_out;
	}

	if (head->cur_mac_cnt > UNIC_DBG_MAC_NUM) {
		ret = -EINVAL;
		unic_err(unic_dev,
			 "invalid cur_mac_cnt(%u).\n", head->cur_mac_cnt);
		goto err_out;
	}

	for (i = 0; i < head->cur_mac_cnt; i++) {
		mac_node = kzalloc(sizeof(*mac_node), GFP_ATOMIC);
		if (!mac_node) {
			ret = -ENOMEM;
			goto err_out;
		}

		memcpy(&mac_node->mac_addr, &mac_entry[i].mac_addr,
		       sizeof(mac_node->mac_addr));
		mac_node->eport = le32_to_cpu(mac_entry[i].eport);
		list_add_tail(&mac_node->node, list);
	}

	*complete = head->cur_mac_cnt < UNIC_DBG_MAC_NUM;

	*mac_idx = le32_to_cpu(head->mac_idx);

err_out:
	kfree(head);

	return ret;
}

int unic_dbg_dump_mac_tbl_list_hw(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_dbg_comm_addr_node *mac_node, *next_node;
	struct unic_caps *caps = &unic_dev->caps;
	struct list_head list;
	int ret, cnt = 0;
	u32 size;

	ret = unic_dbg_check_dev_state(unic_dev);
	if (ret)
		return ret;

	if (!unic_dev_cfg_mac_supported(unic_dev))
		return -EOPNOTSUPP;

	size = caps->uc_mac_tbl_size + caps->mc_mac_tbl_size;

	INIT_LIST_HEAD(&list);
	ret = unic_common_query_addr_list(unic_dev, size,
					  UNIC_DBG_MAC_NUM, &list,
					  unic_query_mac_list_hw);
	if (ret)
		goto release_list;

	seq_printf(s, "No     %-28sEXTEND_INFO\n", "MAC_ADDR");

	list_for_each_entry(mac_node, &list, node) {
		seq_printf(s, "%-7d", cnt++);
		seq_printf(s, "%-28pM", &mac_node->mac_addr);
		seq_printf(s, "0x%08x\n", mac_node->eport);
	}

release_list:
	list_for_each_entry_safe(mac_node, next_node, &list, node) {
		list_del(&mac_node->node);
		kfree(mac_node);
	}

	return ret;
}
