# SPDX-License-Identifier: GPL-2.0

# Extented Memory Cgroup Interfaces
ccflags-y += -g \
             -DCONFIG_ZRAM_FOR_THP \
             -I$(srctree)/$(src)/driver/block/zram

emm_extentions-y := memcg/emm_extentions.o
obj-m += emm_extentions.o

# Evaluations based on workingset statistics
emm_coreutils-y := memcg/emm_coreutils.o
obj-m += emm_coreutils.o

# Enhanced ZRAM
emm_zram-y := driver/block/zram/zram_drv.o driver/block/zram/zram_memcg.o driver/block/zram/zcomp.o

CONFIG_ZRAM_BACKEND_LZO := y
CONFIG_ZRAM_BACKEND_LZ4 := y
CONFIG_ZRAM_BACKEND_LZ4HC := y
CONFIG_ZRAM_BACKEND_ZSTD := y
CONFIG_ZRAM_BACKEND_DEFLATE := y
CONFIG_ZRAM_BACKEND_842 := y

ccflags-y += -DCONFIG_ZRAM_BACKEND_LZO=1
ccflags-y += -DCONFIG_ZRAM_BACKEND_LZ4=1
ccflags-y += -DCONFIG_ZRAM_BACKEND_LZ4HC=1
ccflags-y += -DCONFIG_ZRAM_BACKEND_ZSTD=1
ccflags-y += -DCONFIG_ZRAM_BACKEND_DEFLATE=1
ccflags-y += -DCONFIG_ZRAM_BACKEND_842=1

emm_zram-$(CONFIG_ZRAM_BACKEND_LZO)	+= driver/block/zram/backend_lzorle.o driver/block/zram/backend_lzo.o
emm_zram-$(CONFIG_ZRAM_BACKEND_LZ4)	+= driver/block/zram/backend_lz4.o
emm_zram-$(CONFIG_ZRAM_BACKEND_LZ4HC)	+= driver/block/zram/backend_lz4hc.o
emm_zram-$(CONFIG_ZRAM_BACKEND_ZSTD)	+= driver/block/zram/backend_zstd.o
emm_zram-$(CONFIG_ZRAM_BACKEND_DEFLATE)	+= driver/block/zram/backend_deflate.o
emm_zram-$(CONFIG_ZRAM_BACKEND_842)	+= driver/block/zram/backend_842.o

obj-m += emm_zram.o
