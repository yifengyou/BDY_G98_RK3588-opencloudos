// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 * Description：UBM MAPPING CORE API
 */
#define pr_fmt(fmt) "UBMEMPFD: " fmt

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/bitfield.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/property.h>
#include <linux/iommu.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <asm/page.h>
#include <linux/ummu_core.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <uapi/ub/ubmempfd/ubmempfd.h>

#define UBMEMPFD_MISC_NAME "ubmempfd"

struct ubmempfd_ctx {
	struct rw_semaphore mapping_wr_lock;
	struct device *dev;
	struct iommu_domain *domain;
	bool work_state;
};

static int ubmempfd_open(struct inode *inode, struct file *filp)
{
	struct ubmempfd_ctx *ctx = kzalloc(sizeof(struct ubmempfd_ctx), GFP_KERNEL);

	if (!ctx)
		return -ENOMEM;

	init_rwsem(&ctx->mapping_wr_lock);
	ctx->dev = NULL;
	ctx->domain = NULL;
	ctx->work_state = true;
	filp->private_data = (void *)ctx;
	pr_info("ubmempfd open, pid %d, comm %s\n", current->pid, current->comm);
	return 0;
}

static int ubmempfd_close(struct inode *inode, struct file *filp)
{
	struct ubmempfd_ctx *ctx;

	ctx = (struct ubmempfd_ctx *)filp->private_data;
	if (!ctx)
		return 0;

	down_write(&ctx->mapping_wr_lock);
	if (ctx->dev) {
		ummu_core_free_tdev(ctx->dev);
		ctx->dev = NULL;
		ctx->domain = NULL;
	}
	ctx->work_state = false;
	filp->private_data = NULL;
	up_write(&ctx->mapping_wr_lock);
	kfree(ctx);

	pr_info("ubmempfd close, pid %d, comm %s\n", current->pid, current->comm);
	return 0;
}

static const struct file_operations ubmempfd_misc_fops = {
	.owner = THIS_MODULE,
	.open = ubmempfd_open,
	.release = ubmempfd_close,
};

static struct miscdevice ubmempfd_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = UBMEMPFD_MISC_NAME,
	.fops = &ubmempfd_misc_fops,
	.mode = 0666,
};

static int __init ubmempfd_core_init(void)
{
	return misc_register(&ubmempfd_misc_device);
}

static void __exit ubmempfd_core_exit(void)
{
	misc_deregister(&ubmempfd_misc_device);
}

module_init(ubmempfd_core_init);
module_exit(ubmempfd_core_exit);

MODULE_DESCRIPTION("Hisilicon UB Memory Provider File Descriptor Driver For Qemu");
MODULE_LICENSE("GPL");
