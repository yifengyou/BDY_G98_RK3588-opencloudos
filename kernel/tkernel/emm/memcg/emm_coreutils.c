// SPDX-License-Identifier: GPL-2.0+
/*
 * Tencent emm_coreutils
 *
 * Copyright (c) 2023 Tencent Corporation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int emm_coreutils_mod_init(void)
{
	pr_info("emm_coreutils mod init\n");
	return 0;
}

static void emm_coreutils_mod_exit(void)
{
	pr_info("emm_coreutils mod exit\n");
}

module_init(emm_coreutils_mod_init);
module_exit(emm_coreutils_mod_exit);
MODULE_AUTHOR("Tencent Corporation");
MODULE_LICENSE("GPL v2");
