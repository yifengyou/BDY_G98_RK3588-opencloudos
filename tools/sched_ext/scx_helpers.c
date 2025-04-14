// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "scx_helpers.h"

static int __is_file_accessed(const char *str)
{
	if (access(str, F_OK) != -1)
		return 1;

	return 0;
}

bool is_cgroup2(void)
{
	if (__is_file_accessed("/sys/fs/cgroup/cgroup.controllers"))
		return true;

	return false;
}

/**
 * get_cgroup1_hierarchy_id - Retrieves the ID of a cgroup1 hierarchy from the cgroup1 subsys name.
 * @subsys_name: The cgroup1 subsys name, which can be retrieved from /proc/self/cgroup. It can be
 * a named cgroup like "name=systemd", a controller name like "cpu", or multi-controllers like
 * "cpu,cpuacct".
 */
int get_cgroup1_hierarchy_id(const char *subsys_name)
{
	char *c, *c2, *c3, *c4;
	bool found = false;
	char line[1024];
	FILE *file;
	int i, id;

	if (!subsys_name)
		return -1;

	file = fopen("/proc/self/cgroup", "r");
	if (!file) {
		log_err("fopen /proc/self/cgroup");
		return -1;
	}

	while (fgets(line, 1024, file)) {
		i = 0;
		for (c = strtok_r(line, ":", &c2); c && i < 2; c = strtok_r(NULL, ":", &c2)) {
			if (i == 0) {
				id = strtol(c, NULL, 10);
			} else if (i == 1) {
				if (!strcmp(c, subsys_name)) {
					found = true;
					break;
				}

				/* Multiple subsystems may share one single mount point */
				for (c3 = strtok_r(c, ",", &c4); c3;
				     c3 = strtok_r(NULL, ",", &c4)) {
					if (!strcmp(c, subsys_name)) {
						found = true;
						break;
					}
				}
			}
			i++;
		}
		if (found)
			break;
	}
	fclose(file);
	return found ? id : -1;
}
