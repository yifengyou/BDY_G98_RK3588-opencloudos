// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2025. All rights reserved.
 *
 * Description: ubcore tp implementation
 * Author: Yan Fangfang
 * Create: 2022-08-25
 * Note:
 * History: 2022-08-25: Create file
 */

#include <linux/list.h>
#include "ubcore_priv.h"
#include "ubcore_log.h"

int ubcore_get_tp_list(struct ubcore_device *dev, struct ubcore_get_tp_cfg *cfg,
		       uint32_t *tp_cnt, struct ubcore_tp_info *tp_list,
		       struct ubcore_udata *udata)
{
	int ret;

	if (dev == NULL || dev->ops == NULL || dev->ops->get_tp_list == NULL ||
	    cfg == NULL || tp_cnt == NULL || tp_list == NULL || *tp_cnt == 0) {
		ubcore_log_err("Invalid parameter.\n");
		return -EINVAL;
	}

	if (ubcore_check_trans_mode_valid(cfg->trans_mode) != true) {
		ubcore_log_err("Invalid parameter, trans_mode: %d.\n",
			       (int)cfg->trans_mode);
		return -EINVAL;
	}

	ret = dev->ops->get_tp_list(dev, cfg, tp_cnt, tp_list, udata);
	if (ret != 0)
		ubcore_log_err("Failed to get to list, ret: %d.\n", ret);

	return ret;
}
EXPORT_SYMBOL(ubcore_get_tp_list);

int ubcore_set_tp_attr(struct ubcore_device *dev, const uint64_t tp_handle,
		       const uint8_t tp_attr_cnt, const uint32_t tp_attr_bitmap,
		       const struct ubcore_tp_attr_value *tp_attr,
		       struct ubcore_udata *udata)
{
	int ret;

	if (dev == NULL || dev->ops == NULL || dev->ops->set_tp_attr == NULL ||
	    tp_attr == NULL) {
		ubcore_log_err("Invalid parameter.\n");
		return -EINVAL;
	}

	ret = dev->ops->set_tp_attr(dev, tp_handle, tp_attr_cnt, tp_attr_bitmap,
				    tp_attr, udata);
	if (ret != 0)
		ubcore_log_err("Failed to set tp attributions, ret: %d.\n",
			       ret);

	return ret;
}
EXPORT_SYMBOL(ubcore_set_tp_attr);

int ubcore_get_tp_attr(struct ubcore_device *dev, const uint64_t tp_handle,
		       uint8_t *tp_attr_cnt, uint32_t *tp_attr_bitmap,
		       struct ubcore_tp_attr_value *tp_attr,
		       struct ubcore_udata *udata)
{
	int ret;

	if (dev == NULL || dev->ops == NULL || dev->ops->get_tp_attr == NULL ||
	    tp_attr_cnt == NULL || tp_attr_bitmap == NULL || tp_attr == NULL) {
		ubcore_log_err("Invalid parameter.\n");
		return -EINVAL;
	}

	ret = dev->ops->get_tp_attr(dev, tp_handle, tp_attr_cnt, tp_attr_bitmap,
				    tp_attr, udata);
	if (ret != 0)
		ubcore_log_err("Failed to get tp attributions, ret: %d.\n",
			       ret);

	return ret;
}
EXPORT_SYMBOL(ubcore_get_tp_attr);

