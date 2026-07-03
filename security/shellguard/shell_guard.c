// SPDX-License-Identifier: GPL-2.0-only

#include <linux/lsm_hooks.h>

static int __init shellguard_stub_init(void)
{
	/* Intentionally empty: no security hooks are registered. */
	return 0;
}

DEFINE_LSM(shellguard) = {
	.name = "shellguard",
	.init = shellguard_stub_init,
};
