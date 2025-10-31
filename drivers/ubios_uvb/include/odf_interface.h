/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: odf interface header
 * Author: zhangrui
 * Create: 2025-04-18
 */
#ifndef ODF_INTERFACE_H
#define ODF_INTERFACE_H
#include <linux/acpi.h>

/* UBRT table info */
#define ACPI_SIG_UBRT           "UBRT"
#define UBRT_UB_CONTROLLER      0
#define UBRT_UMMU               1
#define UBRT_UB_MEMORY          2
#define UBRT_VIRTUAL_BUS        3
#define UBRT_CALL_ID_SERVICE    4

struct ubios_od_value_struct {
	char *name;
	u8 type;
	u32 data_length;
	void *data;
};

struct ubios_od_header {
	char name[16];
	u32 total_size;
	u8 version;
	u8 reserved[3];
	u32 remaining_size;
	u32 checksum;
};

/*
Data structure of UBIOS OD Root Table show below:
|----ubios_od_root----|
|  Header           |
|  count            |
|  reserved         |
|  odfs[0]          | if not 0  --point to-->  a od file
|  odfs[...]        | if not 0  --point to-->  a od file
|  odfs[count - 1]  | if not 0  --point to-->  a od file
*/
struct ubios_od_root {
	struct ubios_od_header header;
	u16 count;
	u8 reserved[6];
	u64 odfs[];
};

struct ubios_od_table_info {
	char *table_name;
	u16 row;
	u8 col;
	char *sub_name_start;
	void *value_start;
	void *table_end;
	u32 length_per_row;
};

struct ubios_od_list_info {
	char *name;
	u8 data_type;        /* not include list type */
	u16 count;           /* value count in the list */
	void *start;         /* pointer to the first value in the list */
	void *end;           /* end of list, not include */
};

/**
 * struct ubrt_sub_tables - UBRT Sub tables
 * @type:			type of tables
 * @pointer:		address to tables
 */
struct ubrt_sub_tables {
	u8 type;
	u8 reserved[7];
	u64 pointer;
};

/**
 * struct ubios_ubrt_table - UBRT info
 * @count:			count of tables
 * @sub tables:		Sub tables[count]
 */

struct ubios_ubrt_table {
	struct acpi_table_header header;
	u32 count;
	struct ubrt_sub_tables sub_tables[];
};

#endif
