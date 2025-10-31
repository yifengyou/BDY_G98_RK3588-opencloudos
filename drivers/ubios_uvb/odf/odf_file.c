// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: ODF file validation and retrieval functions
 * Author: zhangrui
 * Create: 2025-04-18
 */
#include <linux/string.h>
#include <linux/printk.h>
#include "include/libodf.h"

bool is_od_root_valid(struct ubios_od_root *root)
{
	if (!root) {
		pr_err(ERR_PRE "odf: root is NULL\n");
		return false;
	}

	if (!odf_is_checksum_ok(&(root->header))) {
		pr_err(ERR_PRE "odf: root checksum error.\n");
		return false;
	}

	if (strcmp(root->header.name, UBIOS_OD_ROOT_NAME)) {
		pr_err(ERR_PRE "odf: root name[%s] mismatch\n", root->header.name);
		return false;
	}

	return true;
}

bool is_od_file_valid(u8 *file)
{
	struct ubios_od_header *header = (struct ubios_od_header *)file;

	if (!header) {
		pr_err(ERR_PRE "odf: file is NULL\n");
		return false;
	}

	if (!odf_is_checksum_ok(header)) {
		pr_err(ERR_PRE "odf: file checksum error.\n");
		return false;
	}

	return true;
}

/**
@brief Search all pointer in od root, return the specific od file matched the input name.
@param[in] root         start of od root
@param[in] name         name of od
@return
@retval = NULL, not found.
@retval != NULL, found.
*/
u8 *odf_get_od_file(struct ubios_od_root *root, char *name)
{
	u64 i;

	if (!is_od_root_valid(root))
		return NULL;

	if (!name)
		return NULL;

	for (i = 0; i < root->count; i++) {
		if (root->odfs[i] == UBIOS_OD_EMPTY)
			continue;

		if (strcmp(name, (char *)(u64)root->odfs[i]) == 0)
			return (u8 *)(u64)root->odfs[i];
	}

	return NULL;
}
