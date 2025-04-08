# SPDX-License-Identifier: GPL-2.0

# Extented Memory Cgroup Interfaces
emm_extentions-y := memcg/emm_extentions.o
obj-m += emm_extentions.o

# Evaluations based on workingset statistics
emm_coreutils-y := memcg/emm_coreutils.o
obj-m += emm_coreutils.o

# Enhanced ZRAM
emm_zram-y := driver/block/zram/zram_drv.o driver/block/zram/zram_memcg.o driver/block/zram/zcomp.o
obj-m += emm_zram.o
