// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 * Description：OBMM Framework's implementations.
 */

#include <linux/module.h>
static int __init obmm_init(void)
{
	return 0;
}

static void __exit obmm_exit(void)
{
}

module_init(obmm_init);
module_exit(obmm_exit);

MODULE_DESCRIPTION("OBMM Framework's implementations.");
MODULE_AUTHOR("Huawei Tech. Co., Ltd.");
MODULE_LICENSE("GPL");
