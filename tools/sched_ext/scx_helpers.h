/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SCX_HELPERS_H
#define __SCX_HELPERS_H

#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_err(MSG, ...) fprintf(stderr, "(%s:%d: errno: %s) " MSG "\n", \
	__FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

bool is_cgroup2(void);
int get_cgroup1_hierarchy_id(const char *subsys_name);

#endif /* __SCX_HELPERS_H */
