// SPDX-License-Identifier: GPL-2.0+
/*
 * Tencent zram_drv
 *
 * Copyright (c) 2023 Tencent Corporation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int zram_drv_mod_init(void)
{
	pr_info("zram_drv mod init\n");
	return 0;
}

static void zram_drv_mod_exit(void)
{
	pr_info("zram_drv mod exit\n");
}

module_init(zram_drv_mod_init);
module_exit(zram_drv_mod_exit);
MODULE_AUTHOR("Tencent Corporation");
MODULE_LICENSE("GPL v2");
