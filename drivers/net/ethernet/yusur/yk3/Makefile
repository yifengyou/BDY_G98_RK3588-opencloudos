# SPDX-License-Identifier: GPL-2.0

ccflags-y += -I$(src)
ccflags-y += -include $(src)/ver.h
ccflags-y += -DALL_MC_FEATURE

ifeq ($(CC), clang)
	ccflags-remove-y += -fconserve-stack
	ccflags-remove-y += -fno-inline-functions-called-once
	ccflags-remove-y += -Wno-maybe-uninitialized
	ccflags-remove-y += -mfunction-return=thunk-extern

	ccflags-y += -Wno-unknown-attributes
	ccflags-y += -Wno-address-of-packed-member
	ccflags-y += -Wno-gnu-variable-sized-type-not-at-end
	ccflags-y += -Wno-uninitialized
	ccflags-y += -Wno-pointer-sign
endif

obj-$(CONFIG_YK3) += yk3.o

yk3-y := yk3_main.o
yk3-y += yk3_card.o
yk3-y += yk3_cdev.o
yk3-y += yk3_chip.o
yk3-y += yk3_cmdline.o
yk3-$(CONFIG_DCB) += yk3_dcbnl.o
yk3-y += yk3_debug.o
yk3-y += yk3_dmamap.o
yk3-y += yk3_doe.o
yk3-y += yk3_doe_kapi.o
yk3-y += yk3_doe_misc.o
yk3-y += yk3_doe_process.o
yk3-y += yk3_edma.o
yk3-y += yk3_edma_pp.o
yk3-y += yk3_edma_rx.o
yk3-y += yk3_edma_tx.o
yk3-y += yk3_esw.o
yk3-y += yk3_esw_cuckoo.o
yk3-y += yk3_ethtool.o
yk3-y += yk3_irq.o
yk3-y += yk3_lan.o
yk3-y += yk3_lan_trace.o
yk3-y += yk3_mac.o
yk3-y += yk3_mbox.o
yk3-y += yk3_ndev.o
yk3-y += yk3_np.o
yk3-y += yk3_np_doe.o
yk3-y += yk3_ppp.o
yk3-y += yk3_ppp_eme.o
yk3-y += yk3_ptp.o
yk3-y += yk3_qos.o
yk3-y += yk3_qset.o
yk3-y += yk3_rss.o
yk3-y += yk3_scatter.o
yk3-y += yk3_sriov.o
