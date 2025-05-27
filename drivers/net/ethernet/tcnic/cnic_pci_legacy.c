// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Virtio PCI driver - legacy device support
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

#include "linux/virtio_pci_legacy.h"
#include "cnic_pci_common.h"

/* virtio config->get_features() implementation */
static u64 cnic_vp_get_features(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	/* When someone needs more than 32 feature bits, we'll need to
	 * steal a bit to indicate that the rest are somewhere else. */
	// return vp_legacy_get_features(&vp_dev->ldev);
	// return ioread32(ldev->ioaddr + VIRTIO_PCI_HOST_FEATURES);
	return ioread32(vp_dev->ldev.ioaddr + VIRTIO_PCI_HOST_FEATURES);
}

/* virtio config->finalize_features() implementation */
static int cnic_vp_finalize_features(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	/* Give virtio_ring a chance to accept features. */
	cnic_vring_transport_features(vdev);

	/* Make sure we don't have any features > 32 bits! */
	BUG_ON((u32)vdev->features != vdev->features);

	/* We only support 32 feature bits. */
	// vp_legacy_set_features(&vp_dev->ldev, vdev->features);
	iowrite32(vdev->features, vp_dev->ldev.ioaddr + VIRTIO_PCI_GUEST_FEATURES);

	return 0;
}

/* virtio config->get() implementation */
static void cnic_vp_get(struct cnic_net_device *vdev, unsigned int offset,
		   void *buf, unsigned int len)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	void __iomem *ioaddr = vp_dev->ldev.ioaddr +
			VIRTIO_PCI_CONFIG_OFF(vp_dev->msix_enabled) +
			offset;
	u8 *ptr = buf;
	int i;

	for (i = 0; i < len; i++)
		ptr[i] = ioread8(ioaddr + i);
}

/* the config->set() implementation.  it's symmetric to the config->get()
 * implementation */
static void cnic_vp_set(struct cnic_net_device *vdev, unsigned int offset,
		   const void *buf, unsigned int len)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	void __iomem *ioaddr = vp_dev->ldev.ioaddr +
			VIRTIO_PCI_CONFIG_OFF(vp_dev->msix_enabled) +
			offset;
	const u8 *ptr = buf;
	int i;

	for (i = 0; i < len; i++)
		iowrite8(ptr[i], ioaddr + i);
}

/*
 * cnic_vp_legacy_get_status - get the device status
 * @ldev: the legacy virtio-pci device
 *
 * Returns the status read from device
 */
u8 cnic_vp_legacy_get_status(struct virtio_pci_legacy_device *ldev)
{
	return ioread8(ldev->ioaddr + VIRTIO_PCI_STATUS);
}

/* config->{get,set}_status() implementations */
static u8 cnic_vp_get_status(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	return cnic_vp_legacy_get_status(&vp_dev->ldev);
}


/*
 * cnic_vp_legacy_set_status - set status to device
 * @ldev: the legacy virtio-pci device
 * @status: the status set to device
 */
void cnic_vp_legacy_set_status(struct virtio_pci_legacy_device *ldev,
	u8 status)
{
iowrite8(status, ldev->ioaddr + VIRTIO_PCI_STATUS);
}


static void cnic_vp_set_status(struct cnic_net_device *vdev, u8 status)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	/* We should never be setting status to 0. */
	BUG_ON(status == 0);
	cnic_vp_legacy_set_status(&vp_dev->ldev, status);
}

static void cnic_vp_reset(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	/* 0 status means a reset. */
	cnic_vp_legacy_set_status(&vp_dev->ldev, 0);
	/* Flush out the status write, and flush in device writes,
	 * including MSi-X interrupts, if any. */
	cnic_vp_legacy_get_status(&vp_dev->ldev);
	/* Flush pending VQ/configuration callbacks. */
	cnic_vp_synchronize_vectors(vdev);
}


/*
 * cnic_vp_legacy_config_vector - set the vector for config interrupt
 * @ldev: the legacy virtio-pci device
 * @vector: the config vector
 *
 * Returns the config vector read from the device
 */
u16 cnic_vp_legacy_config_vector(struct virtio_pci_legacy_device *ldev,
	u16 vector)
{
/* Setup the vector used for configuration events */
iowrite16(vector, ldev->ioaddr + VIRTIO_MSI_CONFIG_VECTOR);
/* Verify we had enough resources to assign the vector */
/* Will also flush the write out to device */
return ioread16(ldev->ioaddr + VIRTIO_MSI_CONFIG_VECTOR);
}


static u16 cnic_vp_config_vector(struct cnic_net_pci_device *vp_dev, u16 vector)
{
	return cnic_vp_legacy_config_vector(&vp_dev->ldev, vector);
}


/*
 * cnic_vp_legacy_set_queue_address - set the virtqueue address
 * @ldev: the legacy virtio-pci device
 * @index: the queue index
 * @queue_pfn: pfn of the virtqueue
 */
void cnic_vp_legacy_set_queue_address(struct virtio_pci_legacy_device *ldev,
	u16 index, u32 queue_pfn)
{
iowrite16(index, ldev->ioaddr + VIRTIO_PCI_QUEUE_SEL);
iowrite32(queue_pfn, ldev->ioaddr + VIRTIO_PCI_QUEUE_PFN);
}
u16 cnic_vp_legacy_queue_vector(struct virtio_pci_legacy_device *ldev,
	u16 index, u16 vector);
static struct virtqueue *cnic_net_setup_vq(struct cnic_net_pci_device *vp_dev,
				  struct cnic_net_pci_vq_info *info,
				  unsigned int index,
				  void (*callback)(struct virtqueue *vq),
				  const char *name,
				  bool ctx,
				  u16 msix_vec)
{
	struct virtqueue *vq;
	u16 num;
	int err;
	u64 q_pfn;

	/* Check if queue is either not available or already active. */
	num = vp_legacy_get_queue_size(&vp_dev->ldev, index);
	if (!num || vp_legacy_get_queue_enable(&vp_dev->ldev, index))
		return ERR_PTR(-ENOENT);

	info->msix_vector = msix_vec;

	/* create the cnic_vring */
	vq = cnic_vring_create_virtqueue(index, num,
				    VIRTIO_PCI_VRING_ALIGN, &vp_dev->vdev,
				    true, false, ctx,
				    cnic_vp_notify, callback, name);
	if (!vq)
		return ERR_PTR(-ENOMEM);

	vq->num_max = num;

	q_pfn = cnic_virtqueue_get_desc_addr(vq) >> VIRTIO_PCI_QUEUE_ADDR_SHIFT;
	if (q_pfn >> 32) {
		dev_err(&vp_dev->pci_dev->dev,
			"platform bug: legacy virtio-pci must not be used with RAM above 0x%llxGB\n",
			0x1ULL << (32 + PAGE_SHIFT - 30));
		err = -E2BIG;
		goto out_del_vq;
	}

	/* activate the queue */
	cnic_vp_legacy_set_queue_address(&vp_dev->ldev, index, q_pfn);

	vq->priv = (void __force *)vp_dev->ldev.ioaddr + VIRTIO_PCI_QUEUE_NOTIFY;

	if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
		msix_vec = cnic_vp_legacy_queue_vector(&vp_dev->ldev, index, msix_vec);
		if (msix_vec == VIRTIO_MSI_NO_VECTOR) {
			err = -EBUSY;
			goto out_deactivate;
		}
	}

	return vq;

out_deactivate:
	cnic_vp_legacy_set_queue_address(&vp_dev->ldev, index, 0);
out_del_vq:
	cnic_vring_del_virtqueue(vq);
	return ERR_PTR(err);
}


/*
 * cnic_vp_legacy_queue_vector - set the MSIX vector for a specific virtqueue
 * @ldev: the legacy virtio-pci device
 * @index: queue index
 * @vector: the config vector
 *
 * Returns the config vector read from the device
 */
u16 cnic_vp_legacy_queue_vector(struct virtio_pci_legacy_device *ldev,
	u16 index, u16 vector)
{
iowrite16(index, ldev->ioaddr + VIRTIO_PCI_QUEUE_SEL);
iowrite16(vector, ldev->ioaddr + VIRTIO_MSI_QUEUE_VECTOR);
/* Flush the write out to device */
return ioread16(ldev->ioaddr + VIRTIO_MSI_QUEUE_VECTOR);
}


static void cnic_net_del_vq(struct cnic_net_pci_vq_info *info)
{
	struct virtqueue *vq = info->vq;
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vq->vdev);

	if (vp_dev->msix_enabled) {
		cnic_vp_legacy_queue_vector(&vp_dev->ldev, vq->index,
				VIRTIO_MSI_NO_VECTOR);
		/* Flush the write out to device */
		ioread8(vp_dev->ldev.ioaddr + VIRTIO_PCI_ISR);
	}

	/* Select and deactivate the queue */
	cnic_vp_legacy_set_queue_address(&vp_dev->ldev, vq->index, 0);

	cnic_vring_del_virtqueue(vq);
}

static const struct virtio_config_ops cnic_pci_config_ops = {
	.get		= cnic_vp_get,
	.set		= cnic_vp_set,
	.get_status	= cnic_vp_get_status,
	.set_status	= cnic_vp_set_status,
	.reset		= cnic_vp_reset,
	.find_vqs	= cnic_vp_find_vqs,
	.del_vqs	= cnic_vp_del_vqs,
	.synchronize_cbs = cnic_vp_synchronize_vectors,
	.get_features	= cnic_vp_get_features,
	.finalize_features = cnic_vp_finalize_features,
	.bus_name	= cnic_vp_bus_name,
	.set_vq_affinity = cnic_vp_set_vq_affinity,
	.get_vq_affinity = cnic_vp_get_vq_affinity,
};

/* the PCI probing function */
int cnic_net_pci_legacy_probe(struct cnic_net_pci_device *vp_dev)
{
	struct virtio_pci_legacy_device *ldev = &vp_dev->ldev;
	struct pci_dev *pci_dev = vp_dev->pci_dev;
	int rc;

	ldev->pci_dev = pci_dev;
	//ckw-todo 检查是否需要移植virtio_pci_legacy_probe函数
	rc = vp_legacy_probe(ldev);
	if (rc)
		return rc;

	vp_dev->isr = ldev->isr;
	vp_dev->vdev.id = ldev->id;

	vp_dev->vdev.config = &cnic_pci_config_ops;

	vp_dev->config_vector = cnic_vp_config_vector;
	vp_dev->setup_vq = cnic_net_setup_vq;
	vp_dev->del_vq = cnic_net_del_vq;
	vp_dev->is_legacy = true;

	return 0;
}

void cnic_net_pci_legacy_remove(struct cnic_net_pci_device *vp_dev)
{
	struct virtio_pci_legacy_device *ldev = &vp_dev->ldev;
    //ckw-todo 检查是否需要移植vp_legacy_remove函数
	vp_legacy_remove(ldev);
}
