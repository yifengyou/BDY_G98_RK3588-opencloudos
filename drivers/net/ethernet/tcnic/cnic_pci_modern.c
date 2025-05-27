// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Virtio PCI driver - modern (virtio 1.0) device support
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

#include <linux/delay.h>
#define VIRTIO_PCI_NO_LEGACY
#define VIRTIO_RING_NO_LEGACY
#include "cnic_pci_common.h"



/**
 * __virtio_test_bit - helper to test feature bits. For use by transports.
 *                     Devices should normally use cnic_net_has_feature,
 *                     which includes more checks.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline bool __virtio_test_bit(const struct cnic_net_device *vdev,
	unsigned int fbit)
{
/* Did you forget to fix assumptions on max features? */
if (__builtin_constant_p(fbit))
BUILD_BUG_ON(fbit >= 64);
else
BUG_ON(fbit >= 64);

return vdev->features & BIT_ULL(fbit);
}


/**
 * __virtio_set_bit - helper to set feature bits. For use by transports.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline void __virtio_set_bit(struct cnic_net_device *vdev,
	unsigned int fbit)
{
/* Did you forget to fix assumptions on max features? */
if (__builtin_constant_p(fbit))
BUILD_BUG_ON(fbit >= 64);
else
BUG_ON(fbit >= 64);

vdev->features |= BIT_ULL(fbit);
}


// /**
//  * __cnic_net_clear_bit - helper to clear feature bits. For use by transports.
//  * @vdev: the device
//  * @fbit: the feature bit
//  */
// static inline void __cnic_net_clear_bit(struct cnic_net_device *vdev,
// 	unsigned int fbit)
// {
// /* Did you forget to fix assumptions on max features? */
// if (__builtin_constant_p(fbit))
// BUILD_BUG_ON(fbit >= 64);
// else
// BUG_ON(fbit >= 64);

// vdev->features &= ~BIT_ULL(fbit);
// }

static u64 cnic_vp_get_features(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	return vp_modern_get_features(&vp_dev->mdev);
}

static void cnic_vp_transport_features(struct cnic_net_device *vdev, u64 features)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct pci_dev *pci_dev = vp_dev->pci_dev;

	if ((features & BIT_ULL(VIRTIO_F_SR_IOV)) &&
			pci_find_ext_capability(pci_dev, PCI_EXT_CAP_ID_SRIOV))
		__virtio_set_bit(vdev, VIRTIO_F_SR_IOV);

	if (features & BIT_ULL(VIRTIO_F_RING_RESET))
		__virtio_set_bit(vdev, VIRTIO_F_RING_RESET);
}

/* virtio config->finalize_features() implementation */
static int cnic_vp_finalize_features(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	u64 features = vdev->features;

	/* Give virtio_ring a chance to accept features. */
	cnic_vring_transport_features(vdev);

	/* Give virtio_pci a chance to accept features. */
	cnic_vp_transport_features(vdev, features);

	if (!__virtio_test_bit(vdev, VIRTIO_F_VERSION_1)) {
		dev_err(&vdev->dev, "virtio: device uses modern interface "
			"but does not have VIRTIO_F_VERSION_1\n");
		return -EINVAL;
	}
//todo-ckw 这里要引入新的头文件
	vp_modern_set_features(&vp_dev->mdev, vdev->features);

	return 0;
}

/* virtio config->get() implementation */
static void cnic_vp_get(struct cnic_net_device *vdev, unsigned int offset,
		   void *buf, unsigned int len)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	void __iomem *device = mdev->device;
	u8 b;
	__le16 w;
	__le32 l;

	BUG_ON(offset + len > mdev->device_len);

	switch (len) {
	case 1:
		b = ioread8(device + offset);
		memcpy(buf, &b, sizeof b);
		break;
	case 2:
		w = cpu_to_le16(ioread16(device + offset));
		memcpy(buf, &w, sizeof w);
		break;
	case 4:
		l = cpu_to_le32(ioread32(device + offset));
		memcpy(buf, &l, sizeof l);
		break;
	case 8:
		l = cpu_to_le32(ioread32(device + offset));
		memcpy(buf, &l, sizeof l);
		l = cpu_to_le32(ioread32(device + offset + sizeof l));
		memcpy(buf + sizeof l, &l, sizeof l);
		break;
	default:
		BUG();
	}
}

/* the config->set() implementation.  it's symmetric to the config->get()
 * implementation */
static void cnic_vp_set(struct cnic_net_device *vdev, unsigned int offset,
		   const void *buf, unsigned int len)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	void __iomem *device = mdev->device;
	u8 b;
	__le16 w;
	__le32 l;

	BUG_ON(offset + len > mdev->device_len);

	switch (len) {
	case 1:
		memcpy(&b, buf, sizeof b);
		iowrite8(b, device + offset);
		break;
	case 2:
		memcpy(&w, buf, sizeof w);
		iowrite16(le16_to_cpu(w), device + offset);
		break;
	case 4:
		memcpy(&l, buf, sizeof l);
		iowrite32(le32_to_cpu(l), device + offset);
		break;
	case 8:
		memcpy(&l, buf, sizeof l);
		iowrite32(le32_to_cpu(l), device + offset);
		memcpy(&l, buf + sizeof l, sizeof l);
		iowrite32(le32_to_cpu(l), device + offset + sizeof l);
		break;
	default:
		BUG();
	}
}

static u32 cnic_vp_generation(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	return vp_modern_generation(&vp_dev->mdev);
}

/* config->{get,set}_status() implementations */
static u8 cnic_vp_get_status(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	return vp_modern_get_status(&vp_dev->mdev);
}

static void cnic_vp_set_status(struct cnic_net_device *vdev, u8 status)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);

	/* We should never be setting status to 0. */
	BUG_ON(status == 0);
	vp_modern_set_status(&vp_dev->mdev, status);
}

static void cnic_vp_reset(struct cnic_net_device *vdev)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;

	/* 0 status means a reset. */
	vp_modern_set_status(mdev, 0);
	/* After writing 0 to device_status, the driver MUST wait for a read of
	 * device_status to return 0 before reinitializing the device.
	 * This will flush out the status write, and flush in device writes,
	 * including MSI-X interrupts, if any.
	 */
	while (vp_modern_get_status(mdev))
		msleep(1);
	/* Flush pending VQ/configuration callbacks. */
	cnic_vp_synchronize_vectors(vdev);
}
extern unsigned int cnic_virtqueue_get_vring_size(const struct virtqueue *_vq);
extern dma_addr_t cnic_virtqueue_get_used_addr(const struct virtqueue *_vq);
extern dma_addr_t cnic_virtqueue_get_desc_addr(const struct virtqueue *_vq);
extern dma_addr_t cnic_virtqueue_get_avail_addr(const struct virtqueue *_vq);

static int cnic_vp_active_vq(struct virtqueue *vq, u16 msix_vec)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vq->vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	unsigned long index;

	index = vq->index;

	/* activate the queue */
	vp_modern_set_queue_size(mdev, index, cnic_virtqueue_get_vring_size(vq));
	vp_modern_queue_address(mdev, index, cnic_virtqueue_get_desc_addr(vq),
				cnic_virtqueue_get_avail_addr(vq),
				cnic_virtqueue_get_used_addr(vq));

	if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
		msix_vec = vp_modern_queue_vector(mdev, index, msix_vec);
		if (msix_vec == VIRTIO_MSI_NO_VECTOR)
			return -EBUSY;
	}

	return 0;
}

static int cnic_vp_modern_disable_vq_and_reset(struct virtqueue *vq)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vq->vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	struct cnic_net_pci_vq_info *info;
	unsigned long flags;

	if (!cnic_net_has_feature(vq->vdev, VIRTIO_F_RING_RESET))
		return -ENOENT;

	vp_modern_set_queue_reset(mdev, vq->index);

	info = vp_dev->vqs[vq->index];

	/* delete vq from irq handler */
	spin_lock_irqsave(&vp_dev->lock, flags);
	list_del(&info->node);
	spin_unlock_irqrestore(&vp_dev->lock, flags);

	INIT_LIST_HEAD(&info->node);

#ifdef CONFIG_VIRTIO_HARDEN_NOTIFICATION
	cnic__virtqueue_break(vq);
#endif

	/* For the case where vq has an exclusive irq, call synchronize_irq() to
	 * wait for completion.
	 *
	 * note: We can't use disable_irq() since it conflicts with the affinity
	 * managed IRQ that is used by some drivers.
	 */
	if (vp_dev->per_vq_vectors && info->msix_vector != VIRTIO_MSI_NO_VECTOR)
		synchronize_irq(pci_irq_vector(vp_dev->pci_dev, info->msix_vector));

	vq->reset = true;

	return 0;
}

static int cnic_vp_modern_enable_vq_after_reset(struct virtqueue *vq)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vq->vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	struct cnic_net_pci_vq_info *info;
	unsigned long flags, index;
	int err;

	if (!vq->reset)
		return -EBUSY;

	index = vq->index;
	info = vp_dev->vqs[index];

	if (vp_modern_get_queue_reset(mdev, index))
		return -EBUSY;

	if (vp_modern_get_queue_enable(mdev, index))
		return -EBUSY;

	err = cnic_vp_active_vq(vq, info->msix_vector);
	if (err)
		return err;

	if (vq->callback) {
		spin_lock_irqsave(&vp_dev->lock, flags);
		list_add(&info->node, &vp_dev->virtqueues);
		spin_unlock_irqrestore(&vp_dev->lock, flags);
	} else {
		INIT_LIST_HEAD(&info->node);
	}

#ifdef CONFIG_VIRTIO_HARDEN_NOTIFICATION
	cnic__virtqueue_unbreak(vq);
#endif

	vp_modern_set_queue_enable(&vp_dev->mdev, index, true);
	vq->reset = false;

	return 0;
}

static u16 cnic_vp_config_vector(struct cnic_net_pci_device *vp_dev, u16 vector)
{
	return vp_modern_config_vector(&vp_dev->mdev, vector);
}

static bool cnic_vp_notify_with_data(struct virtqueue *vq)
{
	u32 data = cnic_vring_notification_data(vq);

	iowrite32(data, (void __iomem *)vq->priv);

	return true;
}
extern void cnic_vring_del_virtqueue(struct virtqueue *_vq);
static struct virtqueue *cnic_setup_vq(struct cnic_net_pci_device *vp_dev,
				  struct cnic_net_pci_vq_info *info,
				  unsigned int index,
				  void (*callback)(struct virtqueue *vq),
				  const char *name,
				  bool ctx,
				  u16 msix_vec)
{

	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	bool (*notify)(struct virtqueue *vq);
	struct virtqueue *vq;
	u16 num;
	int err;

	if (__virtio_test_bit(&vp_dev->vdev, VIRTIO_F_NOTIFICATION_DATA))
		notify = cnic_vp_notify_with_data;
	else
		notify = cnic_vp_notify;

	if (index >= vp_modern_get_num_queues(mdev))
		return ERR_PTR(-EINVAL);

	/* Check if queue is either not available or already active. */
	num = vp_modern_get_queue_size(mdev, index);
	if (!num || vp_modern_get_queue_enable(mdev, index))
		return ERR_PTR(-ENOENT);

	info->msix_vector = msix_vec;

	/* create the cnic_vring */
	vq = cnic_vring_create_virtqueue(index, num,
				    SMP_CACHE_BYTES, &vp_dev->vdev,
				    true, true, ctx,
				    notify, callback, name);
	if (!vq)
		return ERR_PTR(-ENOMEM);

	vq->num_max = num;

	err = cnic_vp_active_vq(vq, msix_vec);
	if (err)
		goto err;

	vq->priv = (void __force *)vp_modern_map_vq_notify(mdev, index, NULL);
	if (!vq->priv) {
		err = -ENOMEM;
		goto err;
	}

	return vq;

err:
	cnic_vring_del_virtqueue(vq);
	return ERR_PTR(err);
}

static int cnic_vp_modern_find_vqs(struct cnic_net_device *vdev, unsigned int nvqs,
			      struct virtqueue *vqs[],
			      vq_callback_t *callbacks[],
			      const char * const names[], const bool *ctx,
			      struct irq_affinity *desc)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct virtqueue *vq;
	int rc = cnic_vp_find_vqs(vdev, nvqs, vqs, callbacks, names, ctx, desc);

	if (rc)
		return rc;

	/* Select and activate all queues. Has to be done last: once we do
	 * this, there's no way to go back except reset.
	 */
	list_for_each_entry(vq, &vdev->vqs, list)
		vp_modern_set_queue_enable(&vp_dev->mdev, vq->index, true);

	return 0;
}

static void cnic_del_vq(struct cnic_net_pci_vq_info *info)
{
	struct virtqueue *vq = info->vq;
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vq->vdev);
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;

	if (vp_dev->msix_enabled)
		vp_modern_queue_vector(mdev, vq->index,
				       VIRTIO_MSI_NO_VECTOR);

	if (!mdev->notify_base)
		pci_iounmap(mdev->pci_dev, (void __force __iomem *)vq->priv);

	cnic_vring_del_virtqueue(vq);
}

static int cnic_virtio_pci_find_shm_cap(struct pci_dev *dev, u8 required_id,
				   u8 *bar, u64 *offset, u64 *len)
{
	int pos;

	for (pos = pci_find_capability(dev, PCI_CAP_ID_VNDR); pos > 0;
	     pos = pci_find_next_capability(dev, pos, PCI_CAP_ID_VNDR)) {
		u8 type, cap_len, id, res_bar;
		u32 tmp32;
		u64 res_offset, res_length;

		pci_read_config_byte(dev, pos + offsetof(struct virtio_pci_cap,
							 cfg_type), &type);
		if (type != VIRTIO_PCI_CAP_SHARED_MEMORY_CFG)
			continue;

		pci_read_config_byte(dev, pos + offsetof(struct virtio_pci_cap,
							 cap_len), &cap_len);
		if (cap_len != sizeof(struct virtio_pci_cap64)) {
			dev_err(&dev->dev, "%s: shm cap with bad size offset:"
				" %d size: %d\n", __func__, pos, cap_len);
			continue;
		}

		pci_read_config_byte(dev, pos + offsetof(struct virtio_pci_cap,
							 id), &id);
		if (id != required_id)
			continue;

		pci_read_config_byte(dev, pos + offsetof(struct virtio_pci_cap,
							 bar), &res_bar);
		if (res_bar >= PCI_STD_NUM_BARS)
			continue;

		/* Type and ID match, and the BAR value isn't reserved.
		 * Looks good.
		 */

		/* Read the lower 32bit of length and offset */
		pci_read_config_dword(dev, pos + offsetof(struct virtio_pci_cap,
							  offset), &tmp32);
		res_offset = tmp32;
		pci_read_config_dword(dev, pos + offsetof(struct virtio_pci_cap,
							  length), &tmp32);
		res_length = tmp32;

		/* and now the top half */
		pci_read_config_dword(dev,
				      pos + offsetof(struct virtio_pci_cap64,
						     offset_hi), &tmp32);
		res_offset |= ((u64)tmp32) << 32;
		pci_read_config_dword(dev,
				      pos + offsetof(struct virtio_pci_cap64,
						     length_hi), &tmp32);
		res_length |= ((u64)tmp32) << 32;

		*bar = res_bar;
		*offset = res_offset;
		*len = res_length;

		return pos;
	}
	return 0;
}

static bool cnic_vp_get_shm_region(struct cnic_net_device *vdev,
			      struct virtio_shm_region *region, u8 id)
{
	struct cnic_net_pci_device *vp_dev = cnic_to_vp_device(vdev);
	struct pci_dev *pci_dev = vp_dev->pci_dev;
	u8 bar;
	u64 offset, len;
	phys_addr_t phys_addr;
	size_t bar_len;

	if (!cnic_virtio_pci_find_shm_cap(pci_dev, id, &bar, &offset, &len))
		return false;

	phys_addr = pci_resource_start(pci_dev, bar);
	bar_len = pci_resource_len(pci_dev, bar);

	if ((offset + len) < offset) {
		dev_err(&pci_dev->dev, "%s: cap offset+len overflow detected\n",
			__func__);
		return false;
	}

	if (offset + len > bar_len) {
		dev_err(&pci_dev->dev, "%s: bar shorter than cap offset+len\n",
			__func__);
		return false;
	}

	region->len = len;
	region->addr = (u64) phys_addr + offset;

	return true;
}

static const struct virtio_config_ops cnic_pci_config_nodev_ops = {
	.get		= NULL,
	.set		= NULL,
	.generation	= cnic_vp_generation,
	.get_status	= cnic_vp_get_status,
	.set_status	= cnic_vp_set_status,
	.reset		= cnic_vp_reset,
	.find_vqs	= cnic_vp_modern_find_vqs,
	.del_vqs	= cnic_vp_del_vqs,
	.synchronize_cbs = cnic_vp_synchronize_vectors,
	.get_features	= cnic_vp_get_features,
	.finalize_features = cnic_vp_finalize_features,
	.bus_name	= cnic_vp_bus_name,
	.set_vq_affinity = cnic_vp_set_vq_affinity,
	.get_vq_affinity = cnic_vp_get_vq_affinity,
	.get_shm_region  = cnic_vp_get_shm_region,
	.disable_vq_and_reset = cnic_vp_modern_disable_vq_and_reset,
	.enable_vq_after_reset = cnic_vp_modern_enable_vq_after_reset,
};

static const struct virtio_config_ops cnic_pci_config_ops = {
	.get		= cnic_vp_get,
	.set		= cnic_vp_set,
	.generation	= cnic_vp_generation,
	.get_status	= cnic_vp_get_status,
	.set_status	= cnic_vp_set_status,
	.reset		= cnic_vp_reset,
	.find_vqs	= cnic_vp_modern_find_vqs,
	.del_vqs	= cnic_vp_del_vqs,
	.synchronize_cbs = cnic_vp_synchronize_vectors,
	.get_features	= cnic_vp_get_features,
	.finalize_features = cnic_vp_finalize_features,
	.bus_name	= cnic_vp_bus_name,
	.set_vq_affinity = cnic_vp_set_vq_affinity,
	.get_vq_affinity = cnic_vp_get_vq_affinity,
	.get_shm_region  = cnic_vp_get_shm_region,
	.disable_vq_and_reset = cnic_vp_modern_disable_vq_and_reset,
	.enable_vq_after_reset = cnic_vp_modern_enable_vq_after_reset,
};

/* the PCI probing function */
int cnic_net_pci_modern_probe(struct cnic_net_pci_device *vp_dev)
{
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;
	struct pci_dev *pci_dev = vp_dev->pci_dev;
	int err;

	mdev->pci_dev = pci_dev;

	err = vp_modern_probe(mdev);
	if (err)
		return err;

	if (mdev->device)
		vp_dev->vdev.config = &cnic_pci_config_ops;
	else
		vp_dev->vdev.config = &cnic_pci_config_nodev_ops;

	vp_dev->config_vector = cnic_vp_config_vector;
	vp_dev->setup_vq = cnic_setup_vq;
	vp_dev->del_vq = cnic_del_vq;
	vp_dev->isr = mdev->isr;
	vp_dev->vdev.id = mdev->id;

	return 0;
}

void cnic_net_pci_modern_remove(struct cnic_net_pci_device *vp_dev)
{
	struct virtio_pci_modern_device *mdev = &vp_dev->mdev;

	vp_modern_remove(mdev);
}
