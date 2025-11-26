/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#ifndef __UBASE_STATS_H__
#define __UBASE_STATS_H__

#include <ub/ubase/ubase_comm_stats.h>

struct ubase_query_mac_stats_cmd {
	__le16 port_id;
	u8 resv[2];
	__le64 stats_val[];
};

int __ubase_get_eth_port_stats(struct ubase_dev *udev,
			       struct ubase_eth_mac_stats *data);
int ubase_update_eth_stats_trylock(struct ubase_dev *udev);
void ubase_update_activate_stats(struct ubase_dev *udev, bool activate,
				 int result);

#endif /* _UBASE_STATS_H */
