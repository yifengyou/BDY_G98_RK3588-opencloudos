obj-$(CONFIG_THIRDPARTY_TCNIC_NET) += tcnic-net.o
tcnic-net-objs := cnic_pci_common.o cnic_pci_modern.o cnic_virtio_ring.o cnic_virtio.o cnic_virtio_net.o

ifeq ($(CONFIG_VIRTIO_PCI_LEGACY), y)
tcnic-net-objs += cnic_pci_legacy.o
endif
