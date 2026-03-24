// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 * Description: ubagg topo info file
 * Author: Ma Chuan
 * Create: 2025-06-07
 * Note:
 * History: 2025-06-07 Create file
 */
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "ubagg_log.h"
#include "ubagg_topo_info.h"

static struct ubagg_topo_map *g_topo_map;

struct ubagg_topo_map *
create_global_ubagg_topo_map(struct ubagg_topo_node *topo_infos,
			     uint32_t node_num)
{
	g_topo_map = create_ubagg_topo_map_from_user(topo_infos, node_num);
	return g_topo_map;
}

void delete_global_ubagg_topo_map(void)
{
	if (g_topo_map == NULL)
		return;
	delete_ubagg_topo_map(g_topo_map);
	g_topo_map = NULL;
}

struct ubagg_topo_map *get_global_ubagg_map(void)
{
	return g_topo_map;
}

static struct ubagg_topo_node *get_current_topo_node(void)
{
	if (g_topo_map == NULL)
		return NULL;

	for (uint32_t i = 0; i < g_topo_map->node_num; i++)
		if (g_topo_map->topo_infos[i].is_current)
			return &g_topo_map->topo_infos[i];
	return NULL;
}

static struct ubagg_topo_node *get_topo_node(union ubcore_eid *eid)
{
	if (g_topo_map == NULL)
		return NULL;

	for (uint32_t i = 0; i < g_topo_map->node_num; i++) {
		struct ubagg_topo_node *node = &g_topo_map->topo_infos[i];

		for (uint32_t j = 0; j < DEV_NUM; j++) {
			if (memcmp(&node->agg_devs[j].agg_eid, eid->raw,
				   EID_LEN) == 0) {
				return node;
			}
		}
	}
	return NULL;
}

int find_linked_port(union ubcore_eid *dst_eid,
		     uint32_t ports[IODIE_NUM][MAX_PORT_NUM])
{
	struct ubagg_topo_node *src_node = get_current_topo_node();
	struct ubagg_topo_node *dst_node = get_topo_node(dst_eid);

	if (src_node == NULL) {
		ubagg_log_err("Failed to get current topo node\n");
		return -EINVAL;
	}
	if (dst_node == NULL) {
		ubagg_log_err("Failed to get target topo node\n");
		return -EINVAL;
	}

	for (uint32_t i = 0; i < IODIE_NUM; i++) {
		for (uint32_t j = 0; j < MAX_PORT_NUM; j++) {
			struct ubagg_topo_link *link = &src_node->links[i][j];

			// Ignore iodie id, since it is not relevant for port mapping
			if (src_node->id == dst_node->id)
				ports[i][j] = j;
			else if (link->peer_node == dst_node->id)
				ports[i][j] = link->peer_port;
		}
	}
	return 0;
}

struct ubagg_topo_map *
create_ubagg_topo_map_from_user(struct ubagg_topo_node *user_topo_infos,
				uint32_t node_num)
{
	struct ubagg_topo_map *topo_map = NULL;
	int ret = 0;

	if (user_topo_infos == NULL || node_num <= 0 ||
	    node_num > MAX_NODE_NUM) {
		ubagg_log_err("Invalid param\n");
		return NULL;
	}
	topo_map = kzalloc(sizeof(struct ubagg_topo_map), GFP_KERNEL);
	if (topo_map == NULL)
		return NULL;
	ret = copy_from_user(topo_map->topo_infos,
			     (void __user *)user_topo_infos,
			     sizeof(struct ubagg_topo_node) * node_num);
	if (ret != 0) {
		ubagg_log_err("Failed to copy topo info\n");
		kfree(topo_map);
		return NULL;
	}
	topo_map->node_num = node_num;
	return topo_map;
}

void delete_ubagg_topo_map(struct ubagg_topo_map *topo_map)
{
	if (topo_map == NULL)
		return;
	kfree(topo_map);
}
