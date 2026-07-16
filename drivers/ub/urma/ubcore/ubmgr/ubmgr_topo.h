/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 *
 * Description: ubmgr topo implementation file
 * Author: Wang Hang
 * Create: 2026-02-03
 * Note:
 * History: 2026-02-03 Create file
 */

#ifndef UBMGR_TOPO_H
#define UBMGR_TOPO_H

#include <ub/urma/ubcore_types.h>
#include "ubcore_topo_info.h"

int ubmgr_get_first_primary_eid(struct ubcore_device *dev,
				struct ubcore_eid_info *primary_eid_info);

enum ubmgr_event_type {
	UBMGR_EVENT_PRIMARY_EID_CHANGE,
};

typedef void (*ubmgr_event_cb_t)(enum ubmgr_event_type event_type,
				 void *event_data, void *priv);

struct ubmgr_event_notifier {
	ubmgr_event_cb_t cb;
	void *priv;
	struct list_head node;
};

void ubmgr_register_event_notifier(struct ubmgr_event_notifier *notifier);
void ubmgr_unregister_event_notifier(struct ubmgr_event_notifier *notifier);
void ubmgr_notify_mgmt_event(struct ubcore_mgmt_event *event);

// will delete after whole topo feature move in ubmgr
void ubmgr_notify_set_topo(void);

#endif // UBMGR_TOPO_H
