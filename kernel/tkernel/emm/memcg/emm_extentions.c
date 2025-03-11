// SPDX-License-Identifier: GPL-2.0+
/*
 * Tencent emm_extentions
 *
 * Copyright (c) 2023 Tencent Corporation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int emm_extentions_mod_init(void)
{
	pr_info("emm_extentions mod init\n");
	return 0;
}

static void emm_extentions_mod_exit(void)
{
	pr_info("emm_extentions mod exit\n");
}

module_init(emm_extentions_mod_init);
module_exit(emm_extentions_mod_exit);
MODULE_AUTHOR("Tencent Corporation");
MODULE_LICENSE("GPL v2");
