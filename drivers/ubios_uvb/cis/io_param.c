// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: CIS message processing, handles output preparation.
 * Author: zhangrui
 * Create: 2025-04-18
 */

#include <linux/slab.h>
#include <linux/io.h>
#include "io_param.h"

void ubios_uvb_free_io_param(struct cis_message *param, u8 free_flag)
{
	if (free_flag == 1 && param->input)
		kfree(param->input);
	if (free_flag == 1 && param->output)
		kfree(param->output);
	if (free_flag == 0 && param->input)
		memunmap(param->input);

	kfree(param);
}

void ubios_prepare_output_data(struct cis_message *io_param, void *output, u32 *output_size)
{
	memcpy(output, io_param->output, *(io_param->p_output_size));
	*output_size = *(io_param->p_output_size);
}
