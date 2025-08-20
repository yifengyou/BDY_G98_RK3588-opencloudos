// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt
#define pr_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <ub/urma/ubcore_uapi.h>
#include "udma_dev.h"
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_jfr.h"
#include "udma_jfs.h"
#include "udma_jfc.h"
#include "udma_jetty.h"

bool well_known_jetty_pgsz_check = true;

static int udma_specify_rsvd_jetty_id(struct udma_dev *udma_dev, uint32_t cfg_id)
{
	struct udma_ida *ida_table = &udma_dev->rsvd_jetty_ida_table;
	int id;

	id = ida_alloc_range(&ida_table->ida, cfg_id, cfg_id, GFP_KERNEL);
	if (id < 0) {
		dev_err(udma_dev->dev, "user specify id %u has been used, ret = %d.\n", cfg_id, id);
		return id;
	}

	return 0;
}

static int udma_user_specify_jetty_id(struct udma_dev *udma_dev, uint32_t cfg_id)
{
	if (cfg_id < udma_dev->caps.jetty.start_idx)
		return udma_specify_rsvd_jetty_id(udma_dev, cfg_id);

	return udma_specify_adv_id(udma_dev, &udma_dev->jetty_table.bitmap_table,
				   cfg_id);
}

int udma_alloc_jetty_id(struct udma_dev *udma_dev, uint32_t *idx,
			struct udma_res *jetty_res)
{
	struct udma_group_bitmap *bitmap = &udma_dev->jetty_table.bitmap_table;
	struct ida *ida = &udma_dev->rsvd_jetty_ida_table.ida;
	uint32_t min = jetty_res->start_idx;
	uint32_t next = jetty_res->next_idx;
	uint32_t max;
	int ret;

	if (jetty_res->max_cnt == 0) {
		dev_err(udma_dev->dev, "ida alloc failed max_cnt is 0.\n");
		return -EINVAL;
	}

	max = jetty_res->start_idx + jetty_res->max_cnt - 1;

	if (jetty_res != &udma_dev->caps.jetty) {
		ret = ida_alloc_range(ida, next, max, GFP_KERNEL);
		if (ret < 0) {
			ret = ida_alloc_range(ida, min, max, GFP_KERNEL);
			if (ret < 0) {
				dev_err(udma_dev->dev,
					"ida alloc failed %d.\n", ret);
				return ret;
			}
		}

		*idx = (uint32_t)ret;
	} else {
		ret = udma_adv_id_alloc(udma_dev, bitmap, idx, false, next);
		if (ret) {
			ret = udma_adv_id_alloc(udma_dev, bitmap, idx, false, min);
			if (ret) {
				dev_err(udma_dev->dev,
					"bitmap alloc failed %d.\n", ret);
				return ret;
			}
		}
	}

	jetty_res->next_idx = (*idx + 1) > max ? min : (*idx + 1);

	return 0;
}

static int udma_alloc_normal_jetty_id(struct udma_dev *udma_dev, uint32_t *idx)
{
	int ret;

	ret = udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.jetty);
	if (ret == 0)
		return 0;

	ret = udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.user_ctrl_normal_jetty);
	if (ret == 0)
		return 0;

	return udma_alloc_jetty_id(udma_dev, idx, &udma_dev->caps.public_jetty);
}

#define CFGID_CHECK(a, b) ((a) >= (b).start_idx && (a) < (b).start_idx + (b).max_cnt)

static int udma_verify_jetty_type_dwqe(struct udma_dev *udma_dev,
				       uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.stars_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, cache lock st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.stars_jetty.start_idx,
			udma_dev->caps.stars_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_ccu(struct udma_dev *udma_dev,
				      uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.ccu_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, ccu st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.ccu_jetty.start_idx,
			udma_dev->caps.ccu_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_normal(struct udma_dev *udma_dev,
					 uint32_t cfg_id)
{
	if (!CFGID_CHECK(cfg_id, udma_dev->caps.user_ctrl_normal_jetty)) {
		dev_err(udma_dev->dev,
			"user id %u error, user ctrl normal st idx %u cnt %u.\n",
			cfg_id,
			udma_dev->caps.user_ctrl_normal_jetty.start_idx,
			udma_dev->caps.user_ctrl_normal_jetty.max_cnt);
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type_urma_normal(struct udma_dev *udma_dev,
					      uint32_t cfg_id)
{
	if (!(CFGID_CHECK(cfg_id, udma_dev->caps.public_jetty) ||
		CFGID_CHECK(cfg_id, udma_dev->caps.hdc_jetty) ||
		CFGID_CHECK(cfg_id, udma_dev->caps.jetty))) {
		dev_err(udma_dev->dev,
			"user id %u error, ccu st idx %u cnt %u, stars st idx %u, normal st idx %u cnt %u.\n",
			cfg_id, udma_dev->caps.ccu_jetty.start_idx,
			udma_dev->caps.ccu_jetty.max_cnt,
			udma_dev->caps.stars_jetty.start_idx,
			udma_dev->caps.jetty.start_idx,
			udma_dev->caps.jetty.max_cnt);
		return -EINVAL;
	}

	if (well_known_jetty_pgsz_check && PAGE_SIZE != UDMA_HW_PAGE_SIZE) {
		dev_err(udma_dev->dev, "Does not support specifying Jetty ID on non-4KB page systems.\n");
		return -EINVAL;
	}

	return 0;
}

static int udma_verify_jetty_type(struct udma_dev *udma_dev,
				  enum udma_jetty_type jetty_type, uint32_t cfg_id)
{
	int (*udma_cfg_id_check[UDMA_JETTY_TYPE_MAX])(struct udma_dev *udma_dev,
						      uint32_t cfg_id) = {
		udma_verify_jetty_type_dwqe,
		udma_verify_jetty_type_ccu,
		udma_verify_jetty_type_normal,
		udma_verify_jetty_type_urma_normal
	};

	if (jetty_type < UDMA_JETTY_TYPE_MAX) {
		if (!cfg_id)
			return 0;

		return udma_cfg_id_check[jetty_type](udma_dev, cfg_id);
	}

	dev_err(udma_dev->dev, "invalid jetty type 0x%x.\n", jetty_type);
	return -EINVAL;
}

static int udma_alloc_jetty_id_own(struct udma_dev *udma_dev, uint32_t *id,
				   enum udma_jetty_type jetty_type)
{
	int ret;

	switch (jetty_type) {
	case UDMA_CACHE_LOCK_DWQE_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id,
			&udma_dev->caps.stars_jetty);
		break;
	case UDMA_NORMAL_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id,
			&udma_dev->caps.user_ctrl_normal_jetty);
		break;
	case UDMA_CCU_JETTY_TYPE:
		ret = udma_alloc_jetty_id(udma_dev, id, &udma_dev->caps.ccu_jetty);
		break;
	default:
		ret = udma_alloc_normal_jetty_id(udma_dev, id);
		break;
	}

	if (ret)
		dev_err(udma_dev->dev,
			"udma alloc jetty id own failed, type = %d, ret = %d.\n",
			jetty_type, ret);

	return ret;
}

int alloc_jetty_id(struct udma_dev *udma_dev, struct udma_jetty_queue *sq,
		   uint32_t cfg_id, struct ubcore_jetty_group *jetty_grp)
{
	int ret;

	if (udma_verify_jetty_type(udma_dev, sq->jetty_type, cfg_id))
		return -EINVAL;

	if (cfg_id > 0 && !jetty_grp) {
		ret = udma_user_specify_jetty_id(udma_dev, cfg_id);
		if (ret)
			return ret;

		sq->id = cfg_id;
	} else {
		ret = udma_alloc_jetty_id_own(udma_dev, &sq->id, sq->jetty_type);
	}

	return ret;
}

void udma_set_query_flush_time(struct udma_jetty_queue *sq, uint8_t err_timeout)
{
#define UDMA_TA_TIMEOUT_MAX_INDEX 3
	uint32_t time[] = {
				UDMA_TA_TIMEOUT_128MS,
				UDMA_TA_TIMEOUT_1000MS,
				UDMA_TA_TIMEOUT_8000MS,
				UDMA_TA_TIMEOUT_64000MS,
			};
	uint8_t index;

	index = to_ta_timeout(err_timeout);
	if (index > UDMA_TA_TIMEOUT_MAX_INDEX)
		index = UDMA_TA_TIMEOUT_MAX_INDEX;

	sq->ta_timeout = time[index];
}
