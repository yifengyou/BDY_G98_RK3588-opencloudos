// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 * Description: UBMEM-VMMU Device's Implementation
 */
#define pr_fmt(fmt) "UBMEM_VMMU: " fmt

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/xarray.h>
#include <linux/ummu_core.h>
#include <ub/ubfi/ubfi.h>
#include <linux/hash.h>
#include "../ummu_cfg_v1.h"
#include "../logic_ummu/logic_ummu.h"

/* ubmem_vmmu driver version release no. */
#define UBMEM_VMMU_DRV_VER_NO "01"
#define UBMEM_VMMU_DRV_NAME "ubmem_vmmu"
#define UBMEM_VMMU_PAGE_SIZE_4K (1UL << 12) // 4K
#define UBMEM_VMMU_ONCE_MAX_MAP_AREA_NUM 262144 // 1G
#define HASH_BUCKETS  64
#define MIN_PASIDS 65
#define MAX_PASIDS ((1UL << 20) - 1)

struct ubm_request {
	u32 opcode;
	u32 tid;
	u64 uba;
	u64 size;
	u64 areas_num;
	struct {
		u64 gpa;
		u64 size;
	} areas[];
};

struct response_slot {
	union {
		struct {
			int state;
			int res;
		} inner;
		u64 val;
	};
};

struct ubmem_vmmu_device {
	struct ummu_core_device core_dev;
	struct device *dev;
	void __iomem *doorbell;
	void __iomem *rsp_slot;
	void __iomem *ring;
	unsigned long *slot_bitmap;
	u64 slot_num;
	u64 ring_size;
	u64 max_req_area_num;
	u64 max_req_size;
	u64 map_info_size;
	spinlock_t slot_lock;
};

struct ubmem_vmmu_map_info {
	struct list_head link;
	u64 start_areas_idx;
	struct ubm_request req;
};

struct ubmem_vmmu_domain {
	struct ubmem_vmmu_device *mmu;
	struct ummu_base_domain base_domain;
	struct {
		spinlock_t lock;
		struct list_head ctx_head;
	} map_hash[HASH_BUCKETS];
};

struct ubmem_vmmu_master {
	struct ubmem_vmmu_device *ubmem_vmmu_dev;
};

static struct ubmem_vmmu_device *global_ubmem_vmmu_dev;

static bool ubmem_vmmu_support_call_back(struct tdev_attr *attr, bool *select)
{
	*select = false;

	return true;
}

static bool ubmem_vmmu_tdev_support_attr(struct ummu_core_device *core_device,
					struct tdev_attr *attr)
{
	struct hisi_ummu_tdev_info *info;

	if (!attr->priv || !attr->priv_len)
		return false;

	if (attr->priv_len < sizeof(struct hisi_ummu_tdev_info)) {
		pr_err("ubmem vmmu: para len is invalid.\n");
		return false;
	}

	info = (struct hisi_ummu_tdev_info *)attr->priv;
	if (info->v2.tid && info->v2.tid < core_device->iommu.min_pasids) {
		pr_info("ubmem vmmu: match success\n");
		return true;
	}

	pr_info("ubmem vmmu: mismatch, tid %u\n", info->v2.tid);
	return false;
}

static struct ubmem_vmmu_domain *ubmem_vmmu_domain_alloc_helper(unsigned int type)
{
	struct ubmem_vmmu_domain *ubmem_vmmu_dom;
	u32 i;

	ubmem_vmmu_dom = kzalloc(sizeof(struct ubmem_vmmu_domain), GFP_KERNEL);
	if (!ubmem_vmmu_dom)
		return NULL;

	for (i = 0; i < HASH_BUCKETS; i++) {
		spin_lock_init(&ubmem_vmmu_dom->map_hash[i].lock);
		INIT_LIST_HEAD(&ubmem_vmmu_dom->map_hash[i].ctx_head);
	}
	ubmem_vmmu_dom->base_domain.tid = UMMU_INVALID_TID;
	return ubmem_vmmu_dom;
}

static struct iommu_domain *ubmem_vmmu_domain_alloc(unsigned int type)
{
	struct ubmem_vmmu_domain *ubmem_vmmu_dom;

	switch (type) {
	case IOMMU_DOMAIN_DMA:
	case IOMMU_DOMAIN_DMA_FQ:
		ubmem_vmmu_dom = ubmem_vmmu_domain_alloc_helper(type);
		if (!ubmem_vmmu_dom) {
			pr_err("Failed to allocate ubmem vmmu domain\n");
			return (struct iommu_domain *)ERR_PTR(-ENOMEM);
		}
		break;
	default:
		return (struct iommu_domain *)ERR_PTR(-EINVAL);
	}

	return &ubmem_vmmu_dom->base_domain.domain;
}

static struct iommu_device *ubmem_vmmu_probe_device(struct device *dev)
{
	struct ubmem_vmmu_device *ubmem_vmmu_dev = global_ubmem_vmmu_dev;
	struct iommu_fwspec *dev_iommu_fwspec;
	struct fwnode_handle *iommu_fwnode;
	struct ubmem_vmmu_master *master;

	dev_iommu_fwspec = dev_iommu_fwspec_get(dev);
	if (!dev_iommu_fwspec)
		return (struct iommu_device *)ERR_PTR(-ENODEV);

	iommu_fwnode = dev_iommu_fwspec->iommu_fwnode;
	if (!iommu_fwnode) {
		pr_err("ubm mmu probe device iommu_fwnode not exist!\n");
		return (struct iommu_device *)ERR_PTR(-ENODEV);
	}

	if (ubmem_vmmu_dev->core_dev.iommu.fwnode != iommu_fwnode) {
		pr_err("ubm mmu probe device %s failed, fwnode is not match!\n", dev_name(dev));
		return (struct iommu_device *)ERR_PTR(-ENODEV);
	}

	master = kzalloc(sizeof(*master), GFP_KERNEL);
	if (!master)
		return (struct iommu_device *)ERR_PTR(-ENOMEM);

	master->ubmem_vmmu_dev = ubmem_vmmu_dev;
	dev_iommu_priv_set(dev, master);
	pr_info("ubm mmu probe device %s successful!\n", dev_name(dev));
	return &ubmem_vmmu_dev->core_dev.iommu;
}

static void ubmem_vmmu_release_device(struct device *dev)
{
	struct ubmem_vmmu_master *master = (struct ubmem_vmmu_master *)dev_iommu_priv_get(dev);

	dev_iommu_priv_set(dev, NULL);
	kfree(master);
}

static struct iommu_group *ubmem_vmmu_device_group(struct device *dev)
{
	return generic_device_group(dev);
}

static struct ummu_core_ops ubmem_vmmu_core_ops = {
	.tdev_support_attr = ubmem_vmmu_tdev_support_attr,
};

const struct iommu_ops ubmem_vmmu_iommu_ops = {
	.domain_alloc = ubmem_vmmu_domain_alloc,
	.probe_device = ubmem_vmmu_probe_device,
	.release_device = ubmem_vmmu_release_device,
	.device_group = ubmem_vmmu_device_group,
	.pgsize_bitmap = -1UL,
	.owner = THIS_MODULE,
};

static void ubmem_vmmu_device_ubrt_probe(struct ubmem_vmmu_device *ubmem_vmmu)
{
	ubmem_vmmu->core_dev.iommu.min_pasids = MIN_PASIDS;
	ubmem_vmmu->core_dev.iommu.max_pasids = MAX_PASIDS;
}

static int ubmem_vmmu_init_ummu_device(struct ubmem_vmmu_device *ubmem_vmmu,
				const struct iommu_ops *iommu_ops,
				const struct ummu_core_ops *core_ops)
{
	struct ummu_core_init_args args = { 0 };

	args.iommu_ops = iommu_ops;
	args.core_ops = core_ops;
	args.hwdev = ubmem_vmmu->dev;
	args.tid_args.tid_ops = NULL;

	return ummu_core_device_init(&ubmem_vmmu->core_dev, &args);
}

static int ubmem_vmmu_init_device_resource(struct platform_device *pdev,
					   struct ubmem_vmmu_device *ubmem_vmmu)
{
	u32 slot_size, bitmap_size, area_num;
	struct device *dev = &pdev->dev;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(dev, "IO resource 0 is null\n");
		return -EINVAL;
	}

	ubmem_vmmu->doorbell = devm_ioremap_resource(ubmem_vmmu->dev, res);
	if (IS_ERR(ubmem_vmmu->doorbell)) {
		dev_err(dev, "IO resource doorbell ioremap error\n");
		return PTR_ERR(ubmem_vmmu->doorbell);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (unlikely(res == NULL)) {
		dev_err(dev, "IO resource 1 is null\n");
		return -EINVAL;
	}

	ubmem_vmmu->rsp_slot = devm_ioremap_resource(ubmem_vmmu->dev, res);
	if (IS_ERR(ubmem_vmmu->rsp_slot)) {
		dev_err(dev, "IO resource rsp_slot ioremap error\n");
		return PTR_ERR(ubmem_vmmu->rsp_slot);
	}

	ubmem_vmmu->slot_num = ioread64(ubmem_vmmu->rsp_slot);
	ubmem_vmmu->rsp_slot += sizeof(u64);
	slot_size = sizeof(struct response_slot) * ubmem_vmmu->slot_num;
	ubmem_vmmu->ring = ubmem_vmmu->rsp_slot + slot_size;
	ubmem_vmmu->ring_size = (res->end - res->start) - (slot_size + sizeof(u64));
	area_num = (ubmem_vmmu->ring_size -
		    sizeof(struct ubm_request)) / sizeof(((struct ubm_request *)0)->areas[0]);
	ubmem_vmmu->max_req_area_num = (area_num > UBMEM_VMMU_ONCE_MAX_MAP_AREA_NUM) ?
				       UBMEM_VMMU_ONCE_MAX_MAP_AREA_NUM : area_num;
	ubmem_vmmu->max_req_size = ubmem_vmmu->max_req_area_num * UBMEM_VMMU_PAGE_SIZE_4K;
	ubmem_vmmu->map_info_size =
	sizeof(struct ubmem_vmmu_map_info) +
	sizeof(((struct ubm_request *)0)->areas[0]) * ubmem_vmmu->max_req_area_num;
	spin_lock_init(&ubmem_vmmu->slot_lock);

	pr_info("ubmem_vmmu_test max_req_area_num %llu, max_req_size %llu, slot_num %llu, ring_size %llu\n",
		ubmem_vmmu->max_req_area_num, ubmem_vmmu->max_req_size,
		ubmem_vmmu->slot_num, ubmem_vmmu->ring_size);

	bitmap_size = BITS_TO_COMPAT_LONGS(ubmem_vmmu->slot_num);
	ubmem_vmmu->slot_bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	if (ubmem_vmmu->slot_bitmap == NULL) {
		dev_err(dev, "Alloc bitmap failed, size %u\n", bitmap_size);
		return -ENOMEM;
	}

	return 0;
}

static int ubmem_vmmu_device_probe(struct platform_device *pdev)
{
	struct ubmem_vmmu_device *ubmem_vmmu;
	struct device *dev = &pdev->dev;
	int ret = 0;

	ubmem_vmmu = devm_kzalloc(dev, sizeof(*ubmem_vmmu), GFP_KERNEL);
	if (!ubmem_vmmu)
		return -ENOMEM;

	ubmem_vmmu->dev = dev;

	ubmem_vmmu_device_ubrt_probe(ubmem_vmmu);

	ret = ubmem_vmmu_init_device_resource(pdev, ubmem_vmmu);
	if (ret) {
		dev_err(dev, "io device resource is null\n");
		return ret;
	}

	platform_set_drvdata(pdev, ubmem_vmmu);

	ret = ubmem_vmmu_init_ummu_device(ubmem_vmmu, &ubmem_vmmu_iommu_ops, &ubmem_vmmu_core_ops);
	if (ret) {
		dev_err(dev, "setup ubmem_vmmu device failed, ret: %d.\n", ret);
		goto bitmap_free;
	}

	ret = iommu_device_sysfs_add(&ubmem_vmmu->core_dev.iommu, ubmem_vmmu->dev, NULL,
					"ubmem_vmmu_%s", dev_name(ubmem_vmmu->dev));
	if (ret) {
		dev_err(dev, "add sysfs failed, ret=%d\n", ret);
		goto deinit_ummu_core;
	}

	ret = ummu_core_device_register(&ubmem_vmmu->core_dev, REGISTER_TYPE_NORMAL);
	if (ret) {
		dev_err(dev, "register to ummu core failed, ret=%d\n", ret);
		goto remove_sysfs;
	}

	ret = logic_ummu_register_support_attr(ubmem_vmmu_support_call_back);
	if (ret) {
		dev_err(dev, "register support attr to logic ummu failed, ret=%d\n", ret);
		goto register_scb_err;
	}

	global_ubmem_vmmu_dev = ubmem_vmmu;
	dev_info(dev, "register ubmem_vmmu to ummu core success");
	return 0;

register_scb_err:
	ummu_core_device_unregister(&ubmem_vmmu->core_dev);
remove_sysfs:
	iommu_device_sysfs_remove(&ubmem_vmmu->core_dev.iommu);
deinit_ummu_core:
	ummu_core_device_deinit(&ubmem_vmmu->core_dev);
bitmap_free:
	kfree(ubmem_vmmu->slot_bitmap);

	return ret;
}

static int ubmem_vmmu_device_remove(struct platform_device *pdev)
{
	struct ubmem_vmmu_device *ubmem_vmmu = platform_get_drvdata(pdev);

	if (!ubmem_vmmu) {
		pr_err("device remove get invalid platform device!\n");
		return -ENODEV;
	}
	logic_ummu_unregister_support_attr(ubmem_vmmu_support_call_back);
	ummu_core_device_unregister(&ubmem_vmmu->core_dev);
	iommu_device_sysfs_remove(&ubmem_vmmu->core_dev.iommu);
	ummu_core_device_deinit(&ubmem_vmmu->core_dev);
	kfree(ubmem_vmmu->slot_bitmap);
	ubmem_vmmu->slot_bitmap = NULL;
	global_ubmem_vmmu_dev = NULL;
	dev_info(&pdev->dev, "remove ubmem vmmu successful!\n");
	return 0;
}

static const struct of_device_id hisi_ubmem_vmmu_of_match[] = {
	{ .compatible = "hisi,ubmem_vmmu", },
	{ },
};
MODULE_DEVICE_TABLE(of, hisi_ubmem_vmmu_of_match);

static const struct acpi_device_id hisi_ubmem_vmmu_acpi_match[] = {
	{"HISI0591", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, hisi_ubmem_vmmu_acpi_match);

struct platform_driver ubmem_vmmu_driver = {
	.driver = {
		.name = UBMEM_VMMU_DRV_NAME,
		.suppress_bind_attrs = true,
		.of_match_table = hisi_ubmem_vmmu_of_match,
		.acpi_match_table = hisi_ubmem_vmmu_acpi_match,
	},
	.probe = ubmem_vmmu_device_probe,
	.remove = ubmem_vmmu_device_remove,
};

static int __init ubmem_vmmu_driver_register(struct platform_driver *drv)
{
	return platform_driver_register(drv);
}

static void __exit ubmem_vmmu_driver_unregister(struct platform_driver *drv)
{
	platform_driver_unregister(drv);
}

module_driver(ubmem_vmmu_driver, ubmem_vmmu_driver_register, ubmem_vmmu_driver_unregister);

MODULE_IMPORT_NS(UMMU_CORE_DRIVER);
MODULE_IMPORT_NS(UMMU_INTERNAL);
MODULE_DESCRIPTION("Hisilicon ub memory vmmu driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" UBMEM_VMMU_DRV_NAME);
