// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 * Description: ubcore topo info file
 * Author: Liu Jiajun
 * Create: 2025-07-03
 * Note:
 * History: 2025-07-03 Create file
 */

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <ub/urma/ubcore_types.h>
#include <ub/urma/ubcore_uapi.h>
#include "ubcore_log.h"
#include "ubcore_topo_info.h"

static struct ubcore_topo_map *g_ubcore_topo_map;

struct ubcore_topo_map *
ubcore_create_global_topo_map(struct ubcore_topo_info *topo_infos,
			      uint32_t node_num)
{
	g_ubcore_topo_map =
		ubcore_create_topo_map_from_user(topo_infos, node_num);
	return g_ubcore_topo_map;
}

void ubcore_delete_global_topo_map(void)
{
	if (!g_ubcore_topo_map)
		return;
	ubcore_delete_topo_map(g_ubcore_topo_map);
	g_ubcore_topo_map = NULL;
}

struct ubcore_topo_map *ubcore_get_global_topo_map(void)
{
	return g_ubcore_topo_map;
}

struct ubcore_topo_map *
ubcore_create_topo_map_from_user(struct ubcore_topo_info *user_topo_infos,
				 uint32_t node_num)
{
	struct ubcore_topo_map *topo_map = NULL;
	int ret = 0;

	if (!user_topo_infos || node_num <= 0 ||
	    node_num > MAX_NODE_NUM) {
		ubcore_log_err("Invalid param\n");
		return NULL;
	}
	topo_map = kzalloc(sizeof(struct ubcore_topo_map), GFP_KERNEL);
	if (!topo_map)
		return NULL;
	ret = copy_from_user(topo_map->topo_infos,
			     (void __user *)user_topo_infos,
			     sizeof(struct ubcore_topo_info) * node_num);
	if (ret != 0) {
		ubcore_log_err("Failed to copy topo infos\n");
		kfree(topo_map);
		return NULL;
	}
	topo_map->node_num = node_num;
	return topo_map;
}

void ubcore_delete_topo_map(struct ubcore_topo_map *topo_map)
{
	if (!topo_map)
		return;
	kfree(topo_map);
}

bool is_eid_valid(const char *eid)
{
	int i;

	for (i = 0; i < EID_LEN; i++) {
		if (eid[i] != 0)
			return true;
	}
	return false;
}

bool is_bonding_and_primary_eid_valid(struct ubcore_topo_map *topo_map)
{
	int i, j;
	bool has_primary_eid = false;

	for (i = 0; i < topo_map->node_num; i++) {
		if (!is_eid_valid(topo_map->topo_infos[i].bonding_eid))
			return false;
		has_primary_eid = false;
		for (j = 0; j < IODIE_NUM; j++) {
			if (is_eid_valid(topo_map->topo_infos[i]
						 .io_die_info[j]
						 .primary_eid))
				has_primary_eid = true;
		}
		if (!has_primary_eid)
			return false;
	}
	return true;
}

static int find_cur_node_index(struct ubcore_topo_map *topo_map,
			       uint32_t *node_index)
{
	int i;

	for (i = 0; i < topo_map->node_num; i++) {
		if (topo_map->topo_infos[i].is_cur_node) {
			*node_index = i;
			break;
		}
	}
	if (i == topo_map->node_num) {
		ubcore_log_err("can't find cur node\n");
		return -EINVAL;
	}
	return 0;
}

static bool compare_eids(const char *eid1, const char *eid2)
{
	return memcmp(eid1, eid2, EID_LEN) == 0;
}

static int update_peer_port_eid(struct ubcore_topo_info *new_topo_info,
				struct ubcore_topo_info *old_topo_info)
{
	int i, j;
	char *new_peer_port_eid;
	char *old_peer_port_eid;

	for (i = 0; i < IODIE_NUM; i++) {
		for (j = 0; j < MAX_PORT_NUM; j++) {
			if (!is_eid_valid(
				    new_topo_info->io_die_info[i].port_eid[j]))
				continue;

			new_peer_port_eid =
				new_topo_info->io_die_info[i].peer_port_eid[j];
			old_peer_port_eid =
				old_topo_info->io_die_info[i].peer_port_eid[j];

			if (!is_eid_valid(new_peer_port_eid))
				continue;
			if (is_eid_valid(old_peer_port_eid) &&
			    !compare_eids(new_peer_port_eid,
					  old_peer_port_eid)) {
				ubcore_log_err(
					"peer port eid is not same, new: " EID_FMT
					", old: " EID_FMT "\n",
					EID_RAW_ARGS(new_peer_port_eid),
					EID_RAW_ARGS(old_peer_port_eid));
				return -EINVAL;
			}
			(void)memcpy(old_peer_port_eid, new_peer_port_eid,
				     EID_LEN);
		}
	}
	return 0;
}

struct ubcore_topo_info *
ubcore_get_cur_topo_info(struct ubcore_topo_map *topo_map)
{
	uint32_t cur_node_index = 0;

	if (find_cur_node_index(topo_map, &cur_node_index) != 0) {
		ubcore_log_err("find cur node index failed\n");
		return NULL;
	}
	return &(topo_map->topo_infos[cur_node_index]);
}

int ubcore_update_topo_map(struct ubcore_topo_map *new_topo_map,
			   struct ubcore_topo_map *old_topo_map)
{
	struct ubcore_topo_info *new_cur_node_info;
	struct ubcore_topo_info *old_cur_node_info;
	uint32_t new_cur_node_index = 0;
	uint32_t old_cur_node_index = 0;

	if (!new_topo_map || !old_topo_map) {
		ubcore_log_err("Invalid topo map\n");
		return -EINVAL;
	}
	if (!is_bonding_and_primary_eid_valid(new_topo_map)) {
		ubcore_log_err("Invalid primary eid\n");
		return -EINVAL;
	}
	if (find_cur_node_index(new_topo_map, &new_cur_node_index) != 0) {
		ubcore_log_err("find cur node index failed in new topo map\n");
		return -EINVAL;
	}
	new_cur_node_info = &(new_topo_map->topo_infos[new_cur_node_index]);
	if (find_cur_node_index(old_topo_map, &old_cur_node_index) != 0) {
		ubcore_log_err("find cur node index failed in old topo map\n");
		return -EINVAL;
	}
	old_cur_node_info = &(old_topo_map->topo_infos[old_cur_node_index]);

	if (update_peer_port_eid(new_cur_node_info, old_cur_node_info) != 0) {
		ubcore_log_err("update peer port eid failed\n");
		return -EINVAL;
	}
	return 0;
}

void ubcore_show_topo_map(struct ubcore_topo_map *topo_map)
{
	int i, j, k;
	struct ubcore_topo_info *cur_node_info;

	ubcore_log_info(
		"========================== topo map start =============================\n");
	for (i = 0; i < topo_map->node_num; i++) {
		cur_node_info = topo_map->topo_infos + i;
		if (!is_eid_valid(cur_node_info->bonding_eid))
			continue;

		ubcore_log_info(
			"===================== node %d start =======================\n",
			i);
		ubcore_log_info("bonding eid: " EID_FMT "\n",
				EID_RAW_ARGS(cur_node_info->bonding_eid));
		for (j = 0; j < IODIE_NUM; j++) {
			ubcore_log_info(
				"**primary eid %d: " EID_FMT "\n", j,
				EID_RAW_ARGS(cur_node_info->io_die_info[j]
						     .primary_eid));
			for (k = 0; k < MAX_PORT_NUM; k++) {
				ubcore_log_info(
					"****port eid %d: " EID_FMT "\n", k,
					EID_RAW_ARGS(
						cur_node_info->io_die_info[j]
							.port_eid[k]));
				ubcore_log_info(
					"****peer_port eid %d: " EID_FMT "\n",
					k,
					EID_RAW_ARGS(
						cur_node_info->io_die_info[j]
							.peer_port_eid[k]));
			}
		}
		ubcore_log_info(
			"===================== node %d end =======================\n",
			i);
	}
	ubcore_log_info(
		"========================== topo map end =============================\n");
}

int ubcore_get_primary_eid(union ubcore_eid *eid, union ubcore_eid *primary_eid)
{
	int i, j, k;
	struct ubcore_topo_info *cur_node_info;

	if (!g_ubcore_topo_map) {
		ubcore_log_info(
			"ubcore topo map doesn't exist, eid is primary_eid.\n");
		(void)memcpy(primary_eid, eid, EID_LEN);
		return 0;
	}

	for (i = 0; i < g_ubcore_topo_map->node_num; i++) {
		cur_node_info = g_ubcore_topo_map->topo_infos + i;
		if (compare_eids(cur_node_info->bonding_eid,
				 (char *)eid->raw)) {
			ubcore_log_err("input eid is bonding eid\n");
			return -EINVAL;
		}
		for (j = 0; j < IODIE_NUM; j++) {
			if (compare_eids(
				    cur_node_info->io_die_info[j].primary_eid,
				    (char *)eid->raw)) {
				(void)memcpy(primary_eid,
					     cur_node_info->io_die_info[j]
						     .primary_eid,
					     EID_LEN);
				ubcore_log_info("input eid is primary eid\n");
				return 0;
			}
			for (k = 0; k < MAX_PORT_NUM; k++) {
				if (compare_eids(cur_node_info->io_die_info[j]
							 .port_eid[k],
						 (char *)eid->raw)) {
					(void)memcpy(primary_eid,
						     cur_node_info
							     ->io_die_info[j]
							     .primary_eid,
						     EID_LEN);
					ubcore_log_info(
						"find primary eid by port eid\n");
					return 0;
				}
			}
		}
	}
	ubcore_log_err("can't find primary eid\n");
	return -EINVAL;
}

static struct ubcore_topo_info *
	ubcore_get_topo_info_by_bonding_eid(union ubcore_eid *bonding_eid)
{
	struct ubcore_topo_map *topo_map;
	int i;

	topo_map = g_ubcore_topo_map;
	for (i = 0; i < topo_map->node_num; i++) {
		if (!memcmp(bonding_eid, topo_map->topo_infos[i].bonding_eid,
			sizeof(*bonding_eid)))
			return &topo_map->topo_infos[i];
	}

	ubcore_log_err(
		"Failed to get topo info, bonding_eid: "EID_FMT".\n",
		EID_ARGS(*bonding_eid));
	return NULL;
}

static int ubcore_get_route_port_eid(union ubcore_eid *src_v_eid,
	union ubcore_eid *dst_v_eid, struct ubcore_route_list *route_list)
{
	struct ubcore_topo_info *src_topo_info = NULL;
	struct ubcore_topo_info *dst_topo_info = NULL;
	uint32_t num = route_list->route_num;
	int i, j;

	src_topo_info =
		ubcore_get_topo_info_by_bonding_eid(src_v_eid);
	if (IS_ERR_OR_NULL(src_topo_info)) {
		ubcore_log_err("Failed to get src_topo_info.\n");
		return -EINVAL;
	}

	dst_topo_info =
		ubcore_get_topo_info_by_bonding_eid(dst_v_eid);
	if (IS_ERR_OR_NULL(dst_topo_info)) {
		ubcore_log_err("Failed to get dst_topo_info.\n");
		return -EINVAL;
	}

	for (int k = 0; k < IODIE_NUM; k++) {
		for (i = 0; i < MAX_PORT_NUM; i++) {
			if (!is_eid_valid(src_topo_info->io_die_info[k].port_eid[i]) ||
				!is_eid_valid(src_topo_info->io_die_info[k].peer_port_eid[i])) {
				continue;
			}
			for (j = 0; j < MAX_PORT_NUM; j++) {
				if (num >= UBCORE_MAX_ROUTE_NUM) {
					ubcore_log_warn("Invalid route num.\n");
					return -EINVAL;
				}
				if (compare_eids(
					src_topo_info->io_die_info[k].peer_port_eid[i],
					dst_topo_info->io_die_info[k].port_eid[j])) {
					(void)memcpy(&route_list->buf[num].src,
						src_topo_info->io_die_info[k].port_eid[i],
						sizeof(union ubcore_eid));
					(void)memcpy(&route_list->buf[num].dst,
						src_topo_info->io_die_info[k].peer_port_eid[i],
						sizeof(union ubcore_eid));
					route_list->buf[num].flag.bs.rtp = 1;
					route_list->buf[num].flag.bs.ctp = 1;
					route_list->buf[num].flag.bs.utp = 1;
					num++;
				}
			}
		}
	}

	if (route_list->route_num == num) {
		ubcore_log_err(
			"Failed to get topo port eid, route_num: %u.\n", num);
		return -EINVAL;
	}

	route_list->route_num = num;
	return 0;
}

int ubcore_get_primary_eid_by_bonding_eid(union ubcore_eid *bonding_eid,
	union ubcore_eid *primary_eid)
{
	struct ubcore_topo_map *topo_map;
	int i;

	topo_map = ubcore_get_global_topo_map();
	if (!topo_map) {
		ubcore_log_err("Failed get global topo map");
		return -EINVAL;
	}

	for (i = 0; i < topo_map->node_num; i++) {
		if (!memcmp(bonding_eid, topo_map->topo_infos[i].bonding_eid,
			sizeof(*bonding_eid))) {
			*primary_eid = *((union ubcore_eid *)
				topo_map->topo_infos[i].io_die_info[0].primary_eid);
			return 0;
		}
	}
	return -EINVAL;
}

static int ubcore_get_route_primary_eid(union ubcore_eid *src_v_eid,
	union ubcore_eid *dst_v_eid, struct ubcore_route_list *route_list)
{
	uint32_t num = route_list->route_num;
	struct ubcore_topo_info *topo_info = g_ubcore_topo_map->topo_infos;
	bool src_match = false;
	bool dst_match = false;

	for (int i = 0; i < g_ubcore_topo_map->node_num; i++) {
		if (num >= UBCORE_MAX_ROUTE_NUM - 1) {
			ubcore_log_warn("Finish to query topo map.\n");
			return -EINVAL;
		}
		if (!memcmp(src_v_eid, topo_info[i].bonding_eid,
			sizeof(*src_v_eid))) {
			route_list->buf[num].flag.bs.ctp = 1;
			route_list->buf[num + 1].flag.bs.ctp = 1;
			route_list->buf[num].hops = 0;
			route_list->buf[num + 1].hops = 0;
			(void)memcpy(&route_list->buf[num].src,
				topo_info[i].io_die_info[0].primary_eid,
				sizeof(union ubcore_eid));
			(void)memcpy(&route_list->buf[num + 1].src,
				topo_info[i].io_die_info[1].primary_eid,
				sizeof(union ubcore_eid));
			src_match = true;
		}
		if (!memcmp(dst_v_eid, topo_info[i].bonding_eid,
			sizeof(*dst_v_eid))) {
			route_list->buf[num].flag.bs.ctp = 1;
			route_list->buf[num + 1].flag.bs.ctp = 1;
			route_list->buf[num].hops = 0;
			route_list->buf[num + 1].hops = 0;
			(void)memcpy(&route_list->buf[num].dst,
				topo_info[i].io_die_info[0].primary_eid,
				sizeof(union ubcore_eid));
			(void)memcpy(&route_list->buf[num + 1].dst,
				topo_info[i].io_die_info[1].primary_eid,
				sizeof(union ubcore_eid));
			dst_match = true;
		}
		if (src_match && dst_match) {
			num += 2;
			break;
		}
	}

	route_list->route_num = num;
	return (num == 0) ? -EINVAL : 0;
}

int ubcore_get_route_list(struct ubcore_route *route,
	struct ubcore_route_list *route_list)
{
	int ret = 0;

	// check valid pointer to v_eid
	if (IS_ERR_OR_NULL(route) || IS_ERR_OR_NULL(route_list)) {
		ubcore_log_err("Invalid parameter.\n");
		return -EINVAL;
	}
	union ubcore_eid *src_v_eid = &route->src;
	union ubcore_eid *dst_v_eid = &route->dst;

	if (!g_ubcore_topo_map) {
		ubcore_log_err(
			"Failed to get p_eid, ubcore topo map doesn't exist.\n");
		return -EINVAL;
	}

	(void)memset(route_list, 0, sizeof(struct ubcore_route_list));

	ret = ubcore_get_route_primary_eid(src_v_eid, dst_v_eid, route_list);
	if (ret != 0) {
		ubcore_log_err("Failed to get primary eid, ret: %d.\n", ret);
		return ret;
	}

	ret = ubcore_get_route_port_eid(src_v_eid, dst_v_eid, route_list);
	if (ret != 0) {
		ubcore_log_err("Failed to get port eid, ret: %d.\n", ret);
		return ret;
	}

	ubcore_log_info("Finish to query primary port eid, route_num: %u.\n",
		route_list->route_num);
	return 0;
}
EXPORT_SYMBOL(ubcore_get_route_list);
