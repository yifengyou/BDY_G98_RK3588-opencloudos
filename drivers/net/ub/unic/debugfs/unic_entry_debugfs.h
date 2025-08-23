/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#ifndef __UNIC_ENTRY_DEBUGFS_H__
#define __UNIC_ENTRY_DEBUGFS_H__

#include <linux/in6.h>
#include <ub/ubase/ubase_comm_debugfs.h>

int unic_dbg_dump_ip_tbl_spec(struct seq_file *s, void *data);
int unic_dbg_dump_ip_tbl_list(struct seq_file *s, void *data);

#endif /* _UNIC_ENTRY_DEBUGFS_H */
