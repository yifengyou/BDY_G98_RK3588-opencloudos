// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) HiSilicon Technologies Co., Ltd. 2025. All rights reserved.
 */

#define pr_fmt(fmt) "ubus hisi memory: " fmt

#include <ub/ubus/ubus.h>
#include <ub/ubus/ub-mem-decoder.h>

#include "../../ubus.h"
#include "../../memory.h"
#include "hisi-ubus.h"

#define DRAIN_ENABLE_REG_OFFSET		0x24
#define DRAIN_STATE_REG_OFFSET		0x28

#define hpa_gen(addr_h, addr_l) (((u64)(addr_h) << 32) | (addr_l))

struct ub_mem_decoder {
	struct device *dev;
	struct ub_entity *uent;
	void *base_reg;
};

static bool hi_mem_validate_pa(struct ub_bus_controller *ubc,
			       u64 pa_start, u64 pa_end, bool cacheable);

static void hi_mem_drain_start(struct ub_mem_device *mem_device)
{
	struct ub_mem_decoder *decoder, *data = mem_device->priv_data;

	if (!data) {
		dev_err(mem_device->dev, "ubc mem_decoder is null.\n");
		return;
	}

	for (int i = 0; i < MEM_INFO_NUM; i++) {
		decoder = &data[i];
		writel(0, decoder->base_reg + DRAIN_ENABLE_REG_OFFSET);
		writel(1, decoder->base_reg + DRAIN_ENABLE_REG_OFFSET);
	}
}

static int hi_mem_drain_state(struct ub_mem_device *mem_device)
{
	struct ub_mem_decoder *decoder, *data = mem_device->priv_data;
	int val = 0;

	if (!data) {
		dev_err(mem_device->dev, "ubc mem_decoder is null.\n");
		return 0;
	}

	for (int i = 0; i < MEM_INFO_NUM; i++) {
		decoder = &data[i];
		val = readb(decoder->base_reg + DRAIN_STATE_REG_OFFSET) & 0x1;
		dev_info_ratelimited(decoder->dev, "ub memory decoder[%d] drain state, val=%d\n",
					i, val);
		if (!val)
			return val;
	}

	return val;
}

static const struct ub_mem_device_ops device_ops = {
	.mem_drain_start = hi_mem_drain_start,
	.mem_drain_state = hi_mem_drain_state,
	.mem_validate_pa = hi_mem_validate_pa,
};

static int hi_mem_decoder_create_one(struct ub_bus_controller *ubc, int mar_id)
{
	struct hi_ubc_private_data *data = (struct hi_ubc_private_data *)ubc->data;
	struct ub_mem_decoder *decoder, *priv_data = ubc->mem_device->priv_data;

	decoder = &priv_data[mar_id];
	decoder->dev = &ubc->dev;
	decoder->uent = ubc->uent;

	decoder->base_reg = ioremap(data->mem_pa_info[mar_id].decode_addr,
				    SZ_64);
	if (!decoder->base_reg) {
		dev_err(decoder->dev, "ub mem decoder base reg ioremap failed.\n");
		return -ENOMEM;
	}

	return 0;
}

static void hi_mem_decoder_remove_one(struct ub_bus_controller *ubc, int mar_id)
{
	struct ub_mem_decoder *priv_data = ubc->mem_device->priv_data;

	iounmap(priv_data[mar_id].base_reg);
}

int hi_mem_decoder_create(struct ub_bus_controller *ubc)
{
	struct ub_mem_device *mem_device;
	void *priv_data;
	int ret;

	mem_device = kzalloc(sizeof(*mem_device), GFP_KERNEL);
	if (!mem_device)
		return -ENOMEM;

	priv_data = kcalloc(MEM_INFO_NUM, sizeof(struct ub_mem_decoder),
			    GFP_KERNEL);
	if (!priv_data) {
		kfree(mem_device);
		return -ENOMEM;
	}

	mem_device->dev = &ubc->dev;
	mem_device->uent = ubc->uent;
	mem_device->ubmem_irq_num = -1;
	mem_device->ops = &device_ops;
	mem_device->priv_data = priv_data;
	ubc->mem_device = mem_device;

	for (int i = 0; i < MEM_INFO_NUM; i++) {
		ret = hi_mem_decoder_create_one(ubc, i);
		if (ret) {
			dev_err(&ubc->dev, "hi mem create decoder %d failed\n", i);
			for (int j = i - 1; j >= 0; j--)
				hi_mem_decoder_remove_one(ubc, j);

			kfree(mem_device->priv_data);
			kfree(mem_device);
			ubc->mem_device = NULL;
			return ret;
		}
	}

	return ret;
}

void hi_mem_decoder_remove(struct ub_bus_controller *ubc)
{
	if (!ubc->mem_device)
		return;

	for (int i = 0; i < MEM_INFO_NUM; i++)
		hi_mem_decoder_remove_one(ubc, i);

	kfree(ubc->mem_device->priv_data);
	kfree(ubc->mem_device);
	ubc->mem_device = NULL;
}

#define MB_SIZE_OFFSET 20

static bool ub_hpa_valid(u64 pa_start, u64 pa_end, u32 base_addr, u32 size)
{
	if (pa_start >= ((u64)base_addr << MB_SIZE_OFFSET) &&
	    pa_end < (((u64)base_addr + (u64)size) << MB_SIZE_OFFSET))
		return true;

	return false;
}

static bool hi_mem_validate_pa(struct ub_bus_controller *ubc,
			       u64 pa_start, u64 pa_end, bool cacheable)
{
	struct hi_ubc_private_data *data;

	if (!ubc->data) {
		dev_err(&ubc->dev, "Ubc data is null.\n");
		return false;
	}

	if (pa_end < pa_start) {
		dev_err(&ubc->dev, "pa_start is over pa_end.\n");
		return false;
	}

	data = (struct hi_ubc_private_data *)ubc->data;
	for (u16 i = 0; i < MEM_INFO_NUM; i++) {
		if (ub_hpa_valid(pa_start, pa_end,
				 data->mem_pa_info[i].cc_base_addr,
				 data->mem_pa_info[i].cc_base_size) &&
		    cacheable)
			return true;

		if (ub_hpa_valid(pa_start, pa_end,
				 data->mem_pa_info[i].nc_base_addr,
				 data->mem_pa_info[i].nc_base_size) &&
		    !cacheable)
			return true;
	}

	dev_err(&ubc->dev, "pa_start-pa_end is invalid.\n");
	return false;
}
