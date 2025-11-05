# SPDX-License-Identifier: GPL-2.0

obj-y				:= core.o
obj-$(CONFIG_DAMON_PADDR)	+= ops-common.o paddr.o
obj-$(CONFIG_DAMON_SYSFS)	+= sysfs-common.o sysfs-schemes.o sysfs.o
obj-$(CONFIG_DAMON_DBGFS)	+= dbgfs.o
obj-$(CONFIG_DAMON_RECLAIM)	+= modules-common.o reclaim.o
obj-$(CONFIG_DAMON_LRU_SORT)	+= modules-common.o lru_sort.o

# Set frame-larger-than 2048 + 128*6 = 2816
KBUILD_CFLAGS += -Wframe-larger-than=2816
obj-$(CONFIG_DAMON_VADDR)	+= ops-common.o vaddr.o
