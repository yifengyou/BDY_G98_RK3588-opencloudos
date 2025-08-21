// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright(c) 2025 HiSilicon Technologies CO., All rights reserved.
 * Description: UMMU CORE API, extend the IOMMU Framework.
 */

#define pr_fmt(fmt) "[UMMU_CORE]: " fmt
#include <linux/module.h>
#include <ub/ubus/ubus.h>

#include "ummu_core_priv.h"

int ummu_core_get_resource(struct iommu_sva *sva, struct resource_args *args)
{
	struct ummu_core_device *core_device;
	const struct ummu_core_ops *ops;

	if (!sva || !sva->dev) {
		pr_err("invalid params.\n");
		return -EINVAL;
	}
	core_device = iommu_get_iommu_dev(sva->dev, struct ummu_core_device, iommu);
	ops = core_device->ops;
	if (!ops || !ops->get_resource) {
		pr_err("invalid ops.\n");
		return -ENODEV;
	}

	return ops->get_resource(to_ummu_base_domain(sva->handle.domain), args);
}

void ummu_core_put_resource(struct iommu_sva *sva, struct resource_args *args)
{
	struct ummu_core_device *core_device;
	const struct ummu_core_ops *ops;

	if (!sva || !sva->dev) {
		pr_err("invalid params.\n");
		return;
	}
	core_device = iommu_get_iommu_dev(sva->dev, struct ummu_core_device, iommu);
	ops = core_device->ops;
	if (!ops || !ops->put_resource) {
		pr_err("invalid ops.\n");
		return;
	}

	ops->put_resource(to_ummu_base_domain(sva->handle.domain), args);
}

int ummu_get_tid(struct device *dev, struct iommu_sva *sva, uint32_t *tidp)
{
	struct iommu_domain *domain;

	if (!dev || !tidp)
		return -EINVAL;

	if (sva)
		domain = sva->handle.domain;
	else
		domain = iommu_get_domain_for_dev(dev);

	if (!domain)
		return -ENODEV;

	*tidp = to_ummu_base_domain(domain)->tid;
	return 0;
}

MODULE_IMPORT_NS(UMMU_CORE_INTERNAL);
MODULE_DESCRIPTION("UMMU Framework");
MODULE_AUTHOR("HUAWEI TECHNOLOGIES CO., LTD.");
MODULE_LICENSE("GPL");
