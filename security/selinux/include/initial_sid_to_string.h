/* SPDX-License-Identifier: GPL-2.0 */

#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

static const char *const initial_sid_to_string[] = {
	NULL,
	"kernel",
	"security",
	"unlabeled",
	NULL,
	"file",
	NULL,
	NULL,
	"any_socket",
	"port",
	"netif",
	"netmsg",
	"node",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"devnull",
};

