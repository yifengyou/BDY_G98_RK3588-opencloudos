/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: uvb info process header
 * Author: zhangrui
 * Create: 2025-04-18
 */

#ifndef UVB_INFO_PROCESS_H
#define UVB_INFO_PROCESS_H

int uvb_poll_window(void *data);
u32 checksum32(const void *data, u32 size);
#endif
