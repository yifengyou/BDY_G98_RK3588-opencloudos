// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 * Description：OBMM Framework's implementations.
 */

#include <linux/cacheflush.h>
#include <asm/tlbflush.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include "obmm_cache.h"
#include "obmm_export_region_ops.h"
#include "obmm_import.h"
#include "obmm_shm_dev.h"

static dev_t obmm_devt;

static const char *obmm_shm_region_name = "OBMM_SHMDEV";
static const char *obmm_shm_rootdev_name = "obmm";
static struct device *obmm_shm_rootdev;

static int obmm_shm_fops_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int obmm_shm_fops_flush(struct file *file __always_unused, fl_owner_t owner __always_unused)
{
	return 0;
}

static int obmm_shm_fops_release(struct inode *inode __always_unused, struct file *file)
{
	return 0;
}

static int obmm_shm_fops_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -ENOTTY;
}

static long obmm_shm_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

const struct file_operations obmm_shm_fops = { .owner = THIS_MODULE,
					       .unlocked_ioctl = obmm_shm_fops_ioctl,
					       .mmap = obmm_shm_fops_mmap,
					       .open = obmm_shm_fops_open,
					       .flush = obmm_shm_fops_flush,
					       .release = obmm_shm_fops_release };

static void obmm_shm_dev_release(struct device *dev)
{
	struct obmm_region *reg;

	reg = container_of(dev, struct obmm_region, device);
	module_put(THIS_MODULE);
}

int obmm_shm_dev_add(struct obmm_region *reg)
{
	int ret;
	dev_t devt;

	if (!try_module_get(THIS_MODULE)) {
		pr_err("Module is dying. Reject all memory requests\n");
		return -EPERM;
	}

	devt = MKDEV(MAJOR(obmm_devt), reg->regionid);
	cdev_init(&reg->cdevice, &obmm_shm_fops);
	reg->cdevice.owner = THIS_MODULE;
	reg->device.devt = devt;
	reg->device.release = obmm_shm_dev_release;
	reg->device.parent = obmm_shm_rootdev;
	device_initialize(&reg->device);

	ret = dev_set_name(&reg->device, "obmm_shmdev%d", reg->regionid);
	if (ret) {
		pr_err("Failed to set name for shmdev %d. ret=%pe\n", reg->regionid, ERR_PTR(ret));
		goto err_put_dev;
	}

	ret = cdev_device_add(&reg->cdevice, &reg->device);
	if (ret) {
		pr_err("Failed to add shm device %d. ret=%pe\n", reg->regionid, ERR_PTR(ret));
		goto err_put_dev;
	}

	return 0;

	/* NOTE: If the device is properly initialized, the refcount of module
	 * should be maintained by device kobject (and the associated
	 * obmm_shm_dev_release function). The refcount of region is always
	 * recovered by kobject-triggered release function.
	 */
err_put_dev:
	put_device(&reg->device);
	return ret;
}

void obmm_shm_dev_del(struct obmm_region *reg)
{
	cdev_device_del(&reg->cdevice, &reg->device);
	put_device(&reg->device);
}

int obmm_shm_dev_init(void)
{
	int ret;

	pr_info("shmdev: root device initialization started\n");
	ret = alloc_chrdev_region(&obmm_devt, OBMM_MIN_VALID_REGIONID, OBMM_REGIONID_MAX_COUNT,
				  obmm_shm_region_name);
	if (ret) {
		pr_err("Failed to allocate char device ID. ret=%pe\n", ERR_PTR(ret));
		goto err_reg_alloc;
	}

	obmm_shm_rootdev = root_device_register(obmm_shm_rootdev_name);
	if (IS_ERR_OR_NULL(obmm_shm_rootdev)) {
		pr_err("error register obmm root device\n");
		ret = -ENOMEM;
		goto err_rootdev;
	}

	pr_info("shmdev: root device initialization completed\n");
	return 0;
err_rootdev:
	unregister_chrdev_region(obmm_devt, OBMM_REGIONID_MAX_COUNT);
err_reg_alloc:
	return ret;
}

void obmm_shm_dev_exit(void)
{
	pr_info("shmdev: root device starts shutting down\n");
	root_device_unregister(obmm_shm_rootdev);
	unregister_chrdev_region(obmm_devt, OBMM_REGIONID_MAX_COUNT);
	pr_info("shmdev: root device shut down completed\n");
}
