/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _DRIVERS_CNIC_PCI_COMMON_H
#define _DRIVERS_CNIC_PCI_COMMON_H
/*
 * Virtio PCI driver - APIs for common functionality for all device versions
 *
 * This module allows virtio devices to be used over a virtual PCI device.
 * This can be used with QEMU based VMMs like KVM or Xen.
 *
 * Copyright IBM Corp. 2007
 * Copyright Red Hat, Inc. 2014
 *
 * Authors:
 *  Anthony Liguori  <aliguori@us.ibm.com>
 *  Rusty Russell <rusty@rustcorp.com.au>
 *  Michael S. Tsirkin <mst@redhat.com>
 */

#include <linux/module.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include  "cnic_virtio.h"

#include "cnic_virtio_config.h"

#include "cnic_virtio_ring.h"


#include <linux/virtio_pci.h>
#include <linux/virtio_pci_legacy.h>
#include <linux/virtio_pci_modern.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>

struct cnic_net_pci_vq_info {
	/* the actual virtqueue */
	struct virtqueue *vq;

	/* the list node for the virtqueues list */
	struct list_head node;

	/* MSI-X vector (or none) */
	unsigned int msix_vector;
};

/* Our device structure */
struct cnic_net_pci_device {
	struct cnic_net_device vdev;
	struct pci_dev *pci_dev;
	union {
		struct virtio_pci_legacy_device ldev;
		struct virtio_pci_modern_device mdev;
	};
	bool is_legacy;

	/* Where to read and clear interrupt */
	u8 __iomem *isr;

	/* a list of queues so we can dispatch IRQs */
	spinlock_t lock;
	struct list_head virtqueues;

	/* array of all queues for house-keeping */
	struct cnic_net_pci_vq_info **vqs;

	/* MSI-X support */
	int msix_enabled;
	int intx_enabled;
	cpumask_var_t *msix_affinity_masks;
	/* Name strings for interrupts. This size should be enough,
	 * and I'm too lazy to allocate each name separately. */
	char (*msix_names)[256];
	/* Number of available vectors */
	unsigned int msix_vectors;
	/* Vectors allocated, excluding per-vq vectors if any */
	unsigned int msix_used_vectors;

	/* Whether we have vector per vq */
	bool per_vq_vectors;

	struct virtqueue *(*setup_vq)(struct cnic_net_pci_device *vp_dev,
				      struct cnic_net_pci_vq_info *info,
				      unsigned int idx,
				      void (*callback)(struct virtqueue *vq),
				      const char *name,
				      bool ctx,
				      u16 msix_vec);
	void (*del_vq)(struct cnic_net_pci_vq_info *info);

	u16 (*config_vector)(struct cnic_net_pci_device *vp_dev, u16 vector);
};

/* Constants for MSI-X */
/* Use first vector for configuration changes, second and the rest for
 * virtqueues Thus, we need at least 2 vectors for MSI. */
enum {
	VP_MSIX_CONFIG_VECTOR = 0,
	VP_MSIX_VQ_VECTOR = 1,
};

/* Convert a generic virtio device to our structure */
static struct cnic_net_pci_device *cnic_to_vp_device(struct cnic_net_device *vdev)
{
	return container_of(vdev, struct cnic_net_pci_device, vdev);
}

/* wait for pending irq handlers */
void cnic_vp_synchronize_vectors(struct cnic_net_device *vdev);
/* the notify function used when creating a virt queue */
bool cnic_vp_notify(struct virtqueue *vq);
/* the config->del_vqs() implementation */
void cnic_vp_del_vqs(struct cnic_net_device *vdev);
/* the config->find_vqs() implementation */
int cnic_vp_find_vqs(struct cnic_net_device *vdev, unsigned int nvqs,
		struct virtqueue *vqs[], vq_callback_t *callbacks[],
		const char * const names[], const bool *ctx,
		struct irq_affinity *desc);
const char * cnic_vp_bus_name(struct cnic_net_device *vdev);

/* Setup the affinity for a virtqueue:
 * - force the affinity for per vq vector
 * - OR over all affinities for shared MSI
 * - ignore the affinity request if we're using INTX
 */
int cnic_vp_set_vq_affinity(struct virtqueue *vq, const struct cpumask *cpu_mask);

const struct cpumask *cnic_vp_get_vq_affinity(struct cnic_net_device *vdev, int index);

#if IS_ENABLED(CONFIG_VIRTIO_PCI_LEGACY)
int cnic_net_pci_legacy_probe(struct cnic_net_pci_device *);
void cnic_net_pci_legacy_remove(struct cnic_net_pci_device *);
#else
static inline int cnic_net_pci_legacy_probe(struct cnic_net_pci_device *vp_dev)
{
	return -ENODEV;
}
static inline void cnic_net_pci_legacy_remove(struct cnic_net_pci_device *vp_dev)
{
}
#endif
int cnic_net_pci_modern_probe(struct cnic_net_pci_device *);
void cnic_net_pci_modern_remove(struct cnic_net_pci_device *);

#endif
