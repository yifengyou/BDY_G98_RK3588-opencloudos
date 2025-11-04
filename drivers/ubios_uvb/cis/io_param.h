/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: io param header
 * Author: zhangrui
 * Create: 2025-04-18
 */

#ifndef UBIOS_IO_PARAM_H
#define UBIOS_IO_PARAM_H

#include <ubios/cis.h>

void ubios_uvb_free_io_param(struct cis_message *param, u8 free_flag);
void ubios_prepare_output_data(struct cis_message *io_param, void *output, u32 *output_size);

#endif
