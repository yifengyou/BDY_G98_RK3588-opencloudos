/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 * Description: ubcore topo info head file
 * Author: Liu Jiajun
 * Create: 2025-07-03
 * Note:
 * History: 2025-07-03 Create file
 */

#ifndef UBCORE_TOPO_INFO_H
#define UBCORE_TOPO_INFO_H

#include <ub/urma/ubcore_types.h>

#define EID_LEN (16)
#define MAX_PORT_NUM (9)
#define MAX_NODE_NUM (16)
#define IODIE_NUM (2)
#define PORT_NUM (9)
#define DEV_NUM (128)
#define MAX_NODE_NUM (16)

struct ubcore_topo_ue {
	uint32_t chip_id;
	char primary_eid[EID_LEN];
	char port_eid[PORT_NUM][EID_LEN];
};

struct ubcore_topo_agg_dev {
	char agg_eid[EID_LEN];
	struct ubcore_topo_ue ues[IODIE_NUM];
};

struct ubcore_topo_link {
	uint32_t peer_node; // node id
	uint32_t peer_iodie; // iodie idx
	uint32_t peer_port; // port idx, UINT32_MAX indicates no connection
};

struct ubcore_topo_node {
	uint32_t id;
	uint32_t is_current;
	struct ubcore_topo_link links[IODIE_NUM][PORT_NUM];
	struct ubcore_topo_agg_dev agg_devs[DEV_NUM];
};

struct ubcore_topo_map {
	struct ubcore_topo_node topo_infos[MAX_NODE_NUM];
	uint32_t node_num;
};

struct ubcore_topo_map *
ubcore_create_global_topo_map(struct ubcore_topo_node *topo_infos,
			      uint32_t node_num);
void ubcore_delete_global_topo_map(void);
struct ubcore_topo_map *ubcore_get_global_topo_map(void);
struct ubcore_topo_map *
ubcore_create_topo_map_from_user(struct ubcore_topo_node *user_topo_infos,
				 uint32_t node_num);
void ubcore_delete_topo_map(struct ubcore_topo_map *topo_map);
bool is_agg_dev_valid(struct ubcore_topo_agg_dev *agg_dev);
bool is_eid_valid(const char *eid);
struct ubcore_topo_node *
ubcore_get_cur_topo_info(struct ubcore_topo_map *topo_map);
int ubcore_update_topo_map(struct ubcore_topo_map *new_topo_map,
			   struct ubcore_topo_map *old_topo_map);
void ubcore_show_topo_map(struct ubcore_topo_map *topo_map);
int ubcore_get_primary_eid(union ubcore_eid *eid,
			   union ubcore_eid *primary_eid);

int ubcore_get_primary_eid_by_agg_eid(union ubcore_eid *agg_eid,
	union ubcore_eid *primary_eid);

#endif // UBCORE_TOPO_INFO_H
