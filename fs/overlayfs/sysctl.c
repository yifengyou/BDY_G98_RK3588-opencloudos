// SPDX-License-Identifier: GPL-2.0
/*
 * Sysctl interface to overlayfs parameters
 */
#include <linux/sysctl.h>

extern unsigned int fuse_alive_ignore_lower;
static struct ctl_table_header *ovl_table_header;

static struct ctl_table ovl_sysctl_table[] = {
	{
		.procname	= "ignore_lower",
		.data		= &fuse_alive_ignore_lower,
		.maxlen		= sizeof(fuse_alive_ignore_lower),
		.mode		= 0644,
		.proc_handler	= proc_douintvec_minmax,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_ONE,
	},
};

int ovl_sysctl_register(void)
{
	ovl_table_header = register_sysctl("fs/overlayfs", ovl_sysctl_table);
	if (!ovl_table_header)
		return -ENOMEM;
	return 0;
}

void ovl_sysctl_unregister(void)
{
	unregister_sysctl_table(ovl_table_header);
	ovl_table_header = NULL;
}
