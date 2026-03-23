// SPDX-License-Identifier: GPL-2.0
/*
 * Handle ARM processor vendor specific error info.
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */

#include <linux/init.h>
#include <linux/acpi.h>
#include <acpi/ghes.h>
#include <acpi/apei.h>
#include "apei-internal.h"

#define HISI_OEM	BIT(0)

static int vender_oem __ro_after_init;

#ifdef CONFIG_ARCH_HISI

#define HISI_VENDOR_MAGIC_NUM		0xCC08CC08CC08CC08
#define HISI_VENDOR_CRITICAL_ERR	BIT(0)

struct hisi_armp_vendor_info {
	u64 magic_num;
	u32 ver_info;
	u32 err_flag;	/* bit0:critical error, others: reserved */
	u32 *regs;
} __packed;

static bool ghes_hisi_critical_hw_error(struct cper_sec_proc_arm *err, bool sync)
{
	struct hisi_armp_vendor_info *vendor_info;
	unsigned long err_info_sz;
	char *p;

	if (!sync)
		return false;

	if (!(err->validation_bits & CPER_ARM_VALID_VENDOR_INFO))
		return false;

	p = (char *)(err + 1);
	err_info_sz = sizeof(struct cper_arm_err_info) * err->err_info_num;
	if (!err->context_info_num) {
		vendor_info = (struct hisi_armp_vendor_info *)
			(p + err_info_sz);
	} else {
		struct cper_arm_ctx_info *ctx_info = (struct cper_arm_ctx_info *)
			(p + err_info_sz);

		vendor_info = (struct hisi_armp_vendor_info *)
			(p + err_info_sz +
			ctx_info->size * err->context_info_num);
	}

	if (vendor_info->magic_num != HISI_VENDOR_MAGIC_NUM)
		return false;

	return (bool)(vendor_info->err_flag & HISI_VENDOR_CRITICAL_ERR);
}
#else
static inline bool ghes_hisi_critical_hw_error(struct cper_sec_proc_arm *err, bool sync)
{
	return false;
}
#endif

bool ghes_armp_vendor_critical_error(struct cper_sec_proc_arm *err, bool sync)
{
	if (vender_oem & HISI_OEM)
		return ghes_hisi_critical_hw_error(err, sync);

	return false;
}

static int __init ghes_check_oem_table(void)
{
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;

	status = acpi_get_table(ACPI_SIG_HEST, 0, &tbl);
	if (ACPI_FAILURE(status) || !tbl)
		return -ENODEV;

	if (!memcmp(tbl->oem_id, "HISI  ", ACPI_OEM_ID_SIZE))
		vender_oem |= HISI_OEM;

	acpi_put_table(tbl);
	return 0;
}
subsys_initcall(ghes_check_oem_table);
