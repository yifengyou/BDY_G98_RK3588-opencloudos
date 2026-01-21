/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 * Description：ubdevshm defines and function prototypes
 */

#ifndef _UB_UBDEVSHM_UBDEVSHM_H_
#define _UB_UBDEVSHM_UBDEVSHM_H_

#include <linux/kernel.h>
#include <linux/kabi.h>

/**
 * struct uba_eid - eid info of ub address.
 * @eid: eid info.
 * @reserved0: will be ignored.
 * @reserved1: will be ignored.
 */
struct uba_eid {
	u64 eid : 20; /* bus_instance_eid */
	u64 reserved0 : 44;
	u64 reserved1 : 64;

	KABI_RESERVE(1)
	KABI_RESERVE(2)
	KABI_RESERVE(3)
	KABI_RESERVE(4)
	KABI_RESERVE(5)
	KABI_RESERVE(6)
};

/**
 * union uba_attr - permissions info of ub address.
 * @bs.readable: read permissions.
 * @bs.writeable: write permissions.
 * @bs.executeable: execute permissions.
 * @bs.token_value_required: enable token value.
 * @reserved: will be ignored.
 * @value:This parameter is used when the detail permission information
 * is not concerned.
 */
union uba_attr {
	struct {
		u64 readable : 1;
		u64 writeable : 1;
		u64 executeable : 1;
		u64 token_value_required : 1;
		u64 reserved : 60;

		KABI_RESERVE(1)
		KABI_RESERVE(2)
		KABI_RESERVE(3)
		KABI_RESERVE(4)
		KABI_RESERVE(5)
		KABI_RESERVE(6)
		KABI_RESERVE(7)
	} bs;
	u64 value;
};

/**
 * union acquire_attr - acquire uba address info.
 * @bs.require_pin: memory should pin when memory user acquire uba address.
 * @bs.reserved: will be ignored.
 * @value: This parameter is used when the acquire information
 * is not concerned.
 */
union acquire_attr {
	struct {
		u64 require_pin : 1;
		u64 require_invalidate : 1;
		u64 reserved : 62;

		KABI_RESERVE(1)
		KABI_RESERVE(2)
		KABI_RESERVE(3)
		KABI_RESERVE(4)
		KABI_RESERVE(5)
		KABI_RESERVE(6)
		KABI_RESERVE(7)
	} bs;
	u64 value;
};

/**
 * struct mem_uva - share memory info.
 * @va: va start.
 * @size: va size.
 */
struct mem_uva {
	u64 va;
	u64 size;
	u64 invalidate_tag;

	KABI_RESERVE(1)
	KABI_RESERVE(2)
	KABI_RESERVE(3)
	KABI_RESERVE(4)
	KABI_RESERVE(5)
};

/**
 * struct mem_uba - uba address info.
 * @uba: uba start.
 * @size: uba size.
 * @token_id: token value.
 * @eid: eid info.
 * @attr: permissions info.
 * @mem_handle: corresponding memory provider handle.
 */

struct mem_uba {
	u64 uba;
	u64 size;
	u32 token_id; /* BIT[0:19] is valid for token_id */
	struct uba_eid eid;
	union uba_attr attr;

	void *mem_handle; /* used by ubdevshm inner */

	KABI_RESERVE(1)
	KABI_RESERVE(2)
	KABI_RESERVE(3)
	KABI_RESERVE(4)
};

typedef int (*invalidate)(u64 invalidate_tag);

#define UBDEV_SHM_DRIVER_NAME_LENGTH 128

struct ubdevshm_mem_ops {
	char name[UBDEV_SHM_DRIVER_NAME_LENGTH];
	u32 version;

	/* Request access to the virtual address range of the given VA */
	int (*acquire)(struct mem_uva *va, union acquire_attr *attr,
		       invalidate func, struct mem_uba *uba);

	/* Release the uba address */
	int (*release)(struct mem_uba *uba);

	KABI_RESERVE(1)
	KABI_RESERVE(2)
	KABI_RESERVE(3)
	KABI_RESERVE(4)
	KABI_RESERVE(5)
};

/**
 * ubdevshm_is_inited() - ubdevshm initialization check interface.
 *
 * Return whether ubdevshm is initialization.
 *
 * Context: Any context.
 * Return: true indicates ubdevshm is avaialbe, false indicates not.
 */
bool ubdevshm_is_inited(void);

/*
 * NOTE:
 * pid/tgid is implicit in the calling context and not passed explicitly
 * in the following interfaces.
 */

/**
 * ubdevshm_register_ops() - Memory provider registration interface.
 * @ops: memory provider specific alloc/free callbacks.
 * @handle: corresponding memory provider handle.
 *
 * Memory provider registration address acquire function.
 *
 * Context: Any context.
 * return: 0 indicates success, while any other value indicates failure.
 */
int ubdevshm_register_ops(struct ubdevshm_mem_ops *ops, unsigned long *handle);

/**
 * ubdevshm_unregister_ops() - Memory provider unregistration interface.
 * @handle: corresponding memory provider handle.
 *
 * Memory provider unregistration address acquire function, Use
 * the corresponding handle to apply for memory.
 *
 * Context: Any context.
 * return: 0 indicates success, while any other value indicates failure.
 */
int ubdevshm_unregister_ops(unsigned long *handle);

#endif /* _UB_UBDEVSHM_UBDEVSHM_H_ */
