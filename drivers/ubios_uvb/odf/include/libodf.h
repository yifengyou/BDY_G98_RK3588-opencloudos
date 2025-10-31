/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: libodf header
 * Author: zhangrui
 * Create: 2025-04-18
 */
#ifndef LIBODF_H
#define LIBODF_H
#include "cis_uvb_interface.h"
#include "libodf_handle.h"

#define UBIOS_OD_NAME_LEN_MAX           16
#define UBIOS_OD_VERSION                1
#define UBIOS_OD_EMPTY                  0

#define UBIOS_OD_TYPE_U8        0x1
#define UBIOS_OD_TYPE_U16       0x2
#define UBIOS_OD_TYPE_U32       0x3
#define UBIOS_OD_TYPE_U64       0x4
#define UBIOS_OD_TYPE_S8        0x5
#define UBIOS_OD_TYPE_S16       0x6
#define UBIOS_OD_TYPE_S32       0x7
#define UBIOS_OD_TYPE_S64       0x8
#define UBIOS_OD_TYPE_BOOL      0x10
#define UBIOS_OD_TYPE_CHAR      0x20
#define UBIOS_OD_TYPE_STRING    0x21
#define UBIOS_OD_TYPE_STRUCT    0x30
#define UBIOS_OD_TYPE_TABLE     0x40
#define UBIOS_OD_TYPE_FILE      0x50
#define UBIOS_OD_TYPE_LIST      0x80

#define UBIOS_OD_ROOT_NAME              "root_table"

#define UBIOS_OD_INVALID_INDEX          0xFFFF

#define UBIOS_OD_PATH_SEPARATOR         '/'
#endif
