// SPDX-License-Identifier: GPL-2.0+
/*
 * Tencent async_fork
 *
 * Copyright (c) 2023 Tencent Corporation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int async_fork_mod_init(void)
{
	pr_info("async_fork mod init\n");
	return 0;
}

static void async_fork_mod_exit(void)
{
	pr_info("async_fork mod exit\n");
}

module_init(async_fork_mod_init);
module_exit(async_fork_mod_exit);
MODULE_AUTHOR("Tencent Corporation");
MODULE_LICENSE("GPL v2");
