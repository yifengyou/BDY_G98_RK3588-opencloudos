# SPDX-License-Identifier: GPL-2.0

SELFTEST_DIR := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ifneq ($(KERNELRELEASE),)
obj-m += rue_testmod.o
else
KERNEL_SRC := $(abspath $(SELFTEST_DIR)/../../../..)
ifeq ($(KBUILD_OUTPUT),)
KERNEL_BUILD := $(KERNEL_SRC)
else
KERNEL_BUILD := $(abspath $(KBUILD_OUTPUT))
KBUILD_OUTPUT_ARG := O=$(KERNEL_BUILD)
endif

TEST_GEN_FILES := netbind_test signal_test
ifneq ($(shell grep -s -w register_rue_ops $(KERNEL_BUILD)/Module.symvers),)
TEST_GEN_FILES += rue_testmod.ko
endif
TEST_PROGS := netbind.sh killprotect.sh killblock.sh irqlatency.sh \
	memcg_async.sh
TEST_FILES := tkernel.sh

EXTRA_CLEAN := .*.cmd Module.symvers modules.order rue_testmod.mod \
	rue_testmod.mod.c rue_testmod.o

include ../lib.mk

$(OUTPUT)/rue_testmod.ko: rue_testmod.c
	+$(MAKE) -C $(KERNEL_SRC) $(KBUILD_OUTPUT_ARG) M=$(SELFTEST_DIR) modules
	@if [ "$(abspath $@)" != "$(SELFTEST_DIR)/rue_testmod.ko" ]; then \
		cp "$(SELFTEST_DIR)/rue_testmod.ko" "$@"; \
	fi
endif
