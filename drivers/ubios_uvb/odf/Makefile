# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
# Create      : 2025-04-18
# Description : odf Makefile

obj-y = odf_get_fdt.o

obj-$(CONFIG_UDFI_ODF) += odf.o
odf-objs := odf_trans.o odf_data.o odf_file.o odf_helper.o

ccflags-y += -I$(srctree)/$(src)/../include
