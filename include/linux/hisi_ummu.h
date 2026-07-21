/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright(c) 2026 HiSilicon Technologies CO., All rights reserved.
 * Description: HiSilicon implementation of the ummu data structure definition.
 */

#ifndef _HISI_UMMU_H_
#define _HISI_UMMU_H_

struct hisi_ummu_tdev_info {
	int version;
	union {
		struct {
			u64 ummu_idx_mask;
			bool on_chip;
		} v1;
		struct {
			u64 reserved;
			bool on_chip;
			u32 tid;
		} v2;
	};
};

#endif /* _HISI_UMMU_H_ */
