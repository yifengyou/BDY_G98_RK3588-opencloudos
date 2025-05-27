// SPDX-License-Identifier: GPL-2.0-only
#include "cnic_virtio.h"
#include "cnic_virtio_config.h"
#include <linux/kernel.h>

#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/idr.h>
#include <linux/of.h>
#include <uapi/linux/virtio_ids.h>

#define DEBUG_INFO(fmt, ...) \
    pr_info("DEBUG [%s:%d] %s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define DEBUG_ERR(fmt, ...) \
    pr_err("DEBUG [%s:%d] %s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define ckw_debug_info() DEBUG_INFO("INFO")
#define ckw_debug_err() DEBUG_ERR("ERROR")

/* Unique numbering for virtio devices. */
static DEFINE_IDA(cnic_index_ida);

static ssize_t device_show(struct device *_d,
			   struct device_attribute *attr, char *buf)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	return sysfs_emit(buf, "0x%04x\n", dev->id.device);
}
static DEVICE_ATTR_RO(device);

static ssize_t vendor_show(struct device *_d,
			   struct device_attribute *attr, char *buf)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	return sysfs_emit(buf, "0x%04x\n", dev->id.vendor);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t status_show(struct device *_d,
			   struct device_attribute *attr, char *buf)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	return sysfs_emit(buf, "0x%08x\n", dev->config->get_status(dev));
}
static DEVICE_ATTR_RO(status);

static ssize_t modalias_show(struct device *_d,
			     struct device_attribute *attr, char *buf)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	return sysfs_emit(buf, "virtio:d%08Xv%08X\n",
		       dev->id.device, dev->id.vendor);
}
static DEVICE_ATTR_RO(modalias);

static ssize_t features_show(struct device *_d,
			     struct device_attribute *attr, char *buf)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	unsigned int i;
	ssize_t len = 0;

	/* We actually represent this as a bitstring, as it could be
	 * arbitrary length in future. */
	for (i = 0; i < sizeof(dev->features)*8; i++)
		len += sysfs_emit_at(buf, len, "%c",
			       __cnic_net_test_bit(dev, i) ? '1' : '0');
	len += sysfs_emit_at(buf, len, "\n");
	return len;
}
static DEVICE_ATTR_RO(features);

static struct attribute *cnic_net_dev_attrs[] = {
	&dev_attr_device.attr,
	&dev_attr_vendor.attr,
	&dev_attr_status.attr,
	&dev_attr_modalias.attr,
	&dev_attr_features.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cnic_net_dev);

static inline int cnic_net_id_match(const struct cnic_net_device *dev,
				  const struct virtio_device_id *id)
{
	if (id->device != dev->id.device && id->device != VIRTIO_DEV_ANY_ID)
		return 0;

	return id->vendor == VIRTIO_DEV_ANY_ID || id->vendor == dev->id.vendor;
}

/* This looks through all the IDs a driver claims to support.  If any of them
 * match, we return 1 and the kernel will call cnic_net_dev_probe(). */
static int cnic_net_dev_match(struct device *_dv, struct device_driver *_dr)
{
	unsigned int i;
	struct cnic_net_device *dev = dev_to_virtio(_dv);
	const struct virtio_device_id *ids;

	ids = cnic_drv_to_virtio(_dr)->id_table;
	for (i = 0; ids[i].device; i++)
		if (cnic_net_id_match(dev, &ids[i]))
			return 1;
	return 0;
}

static int cnic_net_uevent(const struct device *_dv, struct kobj_uevent_env *env)
{
	const struct cnic_net_device *dev = dev_to_virtio(_dv);

	return add_uevent_var(env, "MODALIAS=virtio:d%08Xv%08X",
			      dev->id.device, dev->id.vendor);
}

void cnic_net_check_driver_offered_feature(const struct cnic_net_device *vdev,
					 unsigned int fbit)
{
	unsigned int i;
	struct virtio_driver *drv = cnic_drv_to_virtio(vdev->dev.driver);

	for (i = 0; i < drv->feature_table_size; i++)
		if (drv->feature_table[i] == fbit)
			return;

	if (drv->feature_table_legacy) {
		for (i = 0; i < drv->feature_table_size_legacy; i++)
			if (drv->feature_table_legacy[i] == fbit)
				return;
	}

	BUG();
}

static void __cnic_virtio_config_changed(struct cnic_net_device *dev)
{
	struct virtio_driver *drv = cnic_drv_to_virtio(dev->dev.driver);

	if (!dev->config_enabled)
		dev->config_change_pending = true;
	else if (drv && drv->config_changed)
		drv->config_changed(dev);
}

void cnic_virtio_config_changed(struct cnic_net_device *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->config_lock, flags);
	__cnic_virtio_config_changed(dev);
	spin_unlock_irqrestore(&dev->config_lock, flags);
}

static void cnic_net_config_disable(struct cnic_net_device *dev)
{
	spin_lock_irq(&dev->config_lock);
	dev->config_enabled = false;
	spin_unlock_irq(&dev->config_lock);
}

static void cnic_net_config_enable(struct cnic_net_device *dev)
{
	spin_lock_irq(&dev->config_lock);
	dev->config_enabled = true;
	if (dev->config_change_pending)
	__cnic_virtio_config_changed(dev);
	dev->config_change_pending = false;
	spin_unlock_irq(&dev->config_lock);
}

void cnic_net_add_status(struct cnic_net_device *dev, unsigned int status)
{
	might_sleep();
	dev->config->set_status(dev, dev->config->get_status(dev) | status);
}



static bool cnic_virtio_no_restricted_mem_acc(struct cnic_net_device  *dev)
{
	return false;
}

bool (*cnic_virtio_check_mem_acc_cb)(struct cnic_net_device *dev) =
	cnic_virtio_no_restricted_mem_acc;
//








/* Do some validation, then set FEATURES_OK */
static int cnic_net_features_ok(struct cnic_net_device *dev)
{
	unsigned int status;

	might_sleep();

	if (cnic_virtio_check_mem_acc_cb(dev)) {
		if (!cnic_net_has_feature(dev, VIRTIO_F_VERSION_1)) {
			dev_warn(&dev->dev,
				 "device must provide VIRTIO_F_VERSION_1\n");
			return -ENODEV;
		}

		if (!cnic_net_has_feature(dev, VIRTIO_F_ACCESS_PLATFORM)) {
			dev_warn(&dev->dev,
				 "device must provide VIRTIO_F_ACCESS_PLATFORM\n");
			return -ENODEV;
		}
	}

	if (!cnic_net_has_feature(dev, VIRTIO_F_VERSION_1))
		return 0;

	cnic_net_add_status(dev, VIRTIO_CONFIG_S_FEATURES_OK);
	status = dev->config->get_status(dev);
	if (!(status & VIRTIO_CONFIG_S_FEATURES_OK)) {
		dev_err(&dev->dev, "virtio: device refuses features: %x\n",
			status);
		return -ENODEV;
	}
	return 0;
}

/**
 * cnic_net_reset_device - quiesce device for removal
 * @dev: the device to reset
 *
 * Prevents device from sending interrupts and accessing memory.
 *
 * Generally used for cleanup during driver / device removal.
 *
 * Once this has been invoked, caller must ensure that
 * cnic_virtqueue_notify / cnic_virtqueue_kick are not in progress.
 *
 * Note: this guarantees that vq callbacks are not in progress, however caller
 * is responsible for preventing access from other contexts, such as a system
 * call/workqueue/bh.  Invoking cnic_virtio_break_device then flushing any such
 * contexts is one way to handle that.
 * */
void cnic_net_reset_device(struct cnic_net_device *dev)
{
#ifdef CONFIG_VIRTIO_HARDEN_NOTIFICATION
	/*
	 * The below virtio_synchronize_cbs() guarantees that any
	 * interrupt for this line arriving after
	 * virtio_synchronize_vqs() has completed is guaranteed to see
	 * vq->broken as true.
	 */
	cnic_virtio_break_device(dev);
	virtio_synchronize_cbs(dev);
#endif

	dev->config->reset(dev);
}

static int cnic_net_dev_probe(struct device *_d)
{
	int err, i;
	struct cnic_net_device *dev = dev_to_virtio(_d);
	struct virtio_driver *drv = cnic_drv_to_virtio(dev->dev.driver);
	u64 device_features;
	u64 driver_features;
	u64 driver_features_legacy;

	/* We have a driver! */
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_DRIVER);

	/* Figure out what features the device supports. */
	device_features = dev->config->get_features(dev);

	/* Figure out what features the driver supports. */
	driver_features = 0;
	for (i = 0; i < drv->feature_table_size; i++) {
		unsigned int f = drv->feature_table[i];
		BUG_ON(f >= 64);
		driver_features |= (1ULL << f);
	}

	/* Some drivers have a separate feature table for virtio v1.0 */
	if (drv->feature_table_legacy) {
		driver_features_legacy = 0;
		for (i = 0; i < drv->feature_table_size_legacy; i++) {
			unsigned int f = drv->feature_table_legacy[i];
			BUG_ON(f >= 64);
			driver_features_legacy |= (1ULL << f);
		}
	} else {
		driver_features_legacy = driver_features;
	}

	if (device_features & (1ULL << VIRTIO_F_VERSION_1))
		dev->features = driver_features & device_features;
	else
		dev->features = driver_features_legacy & device_features;

	/* Transport features always preserved to pass to finalize_features. */
	for (i = VIRTIO_TRANSPORT_F_START; i < VIRTIO_TRANSPORT_F_END; i++)
		if (device_features & (1ULL << i))
			__cnic_net_set_bit(dev, i);

	err = dev->config->finalize_features(dev);
	if (err)
		goto err;

	if (drv->validate) {
		u64 features = dev->features;

		err = drv->validate(dev);
		if (err)
			goto err;

		/* Did validation change any features? Then write them again. */
		if (features != dev->features) {
			err = dev->config->finalize_features(dev);
			if (err)
				goto err;
		}
	}

	err = cnic_net_features_ok(dev);
	if (err)
		goto err;

	err = drv->probe(dev);
	if (err)
		goto err;

	/* If probe didn't do it, mark device DRIVER_OK ourselves. */
	if (!(dev->config->get_status(dev) & VIRTIO_CONFIG_S_DRIVER_OK))
		cnic_net_device_ready(dev);

	if (drv->scan)
		drv->scan(dev);
//todo:ckw 这里需要修改，
	// virtio_config_enable(dev);
	cnic_net_config_enable(dev);

	return 0;
err:
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_FAILED);
	return err;

}

static void cnic_net_dev_remove(struct device *_d)
{
	struct cnic_net_device *dev = dev_to_virtio(_d);
	struct virtio_driver *drv = cnic_drv_to_virtio(dev->dev.driver);

	cnic_net_config_disable(dev);

	drv->remove(dev);

	/* Driver should have reset device. */
	WARN_ON_ONCE(dev->config->get_status(dev));

	/* Acknowledge the device's existence again. */
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_ACKNOWLEDGE);

	of_node_put(dev->dev.of_node);
}

static struct bus_type cnic_virtio_bus = {
	.name  = "cnic_virtio",
	.match = cnic_net_dev_match,
	.dev_groups = cnic_net_dev_groups,
	.uevent = cnic_net_uevent,
	.probe = cnic_net_dev_probe,
	.remove = cnic_net_dev_remove,
};

int cnic_register_virtio_driver(struct virtio_driver *driver)
{
	/* Catch this early. */
	BUG_ON(driver->feature_table_size && !driver->feature_table);
	driver->driver.bus = &cnic_virtio_bus;
	return driver_register(&driver->driver);
}

void cnic_unregister_virtio_driver(struct virtio_driver *driver)
{
	driver_unregister(&driver->driver);
}

static int virtio_device_of_init(struct cnic_net_device *dev)
{
	struct device_node *np, *pnode = dev_of_node(dev->dev.parent);
	char compat[] = "virtio,deviceXXXXXXXX";
	int ret, count;

	if (!pnode)
		return 0;

	count = of_get_available_child_count(pnode);
	if (!count)
		return 0;

	/* There can be only 1 child node */
	if (WARN_ON(count > 1))
		return -EINVAL;

	np = of_get_next_available_child(pnode, NULL);
	if (WARN_ON(!np))
		return -ENODEV;

	ret = snprintf(compat, sizeof(compat), "virtio,device%x", dev->id.device);
	BUG_ON(ret >= sizeof(compat));

	/*
	 * On powerpc/pseries virtio devices are PCI devices so PCI
	 * vendor/device ids play the role of the "compatible" property.
	 * Simply don't init of_node in this case.
	 */
	if (!of_device_is_compatible(np, compat)) {
		ret = 0;
		goto out;
	}

	dev->dev.of_node = np;
	return 0;

out:
	of_node_put(np);
	return ret;
}

/**
 * cnic_register_virtio_device - register virtio device
 * @dev        : virtio device to be registered
 *
 * On error, the caller must call put_device on &@dev->dev (and not kfree),
 * as another code path may have obtained a reference to @dev.
 *
 * Returns: 0 on suceess, -error on failure
 */
// 入口B
int cnic_register_virtio_device(struct cnic_net_device *dev)
{
	int err;

	dev->dev.bus = &cnic_virtio_bus;
	device_initialize(&dev->dev);

	/* Assign a unique device index and hence name. */
	err = ida_alloc(&cnic_index_ida, GFP_KERNEL);
	if (err < 0)
		goto out;

	dev->index = err;
	err = dev_set_name(&dev->dev, "cnic-virtio%u", dev->index);
	if (err)
		goto out_ida_remove;

	err = virtio_device_of_init(dev);
	if (err)
		goto out_ida_remove;

	spin_lock_init(&dev->config_lock);
	dev->config_enabled = false;
	dev->config_change_pending = false;

	INIT_LIST_HEAD(&dev->vqs);
	spin_lock_init(&dev->vqs_list_lock);

	/* We always start by resetting the device, in case a previous
	 * driver messed it up.  This also tests that code path a little. */
	cnic_net_reset_device(dev);

	/* Acknowledge that we've seen the device. */
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_ACKNOWLEDGE);

	/*
	 * device_add() causes the bus infrastructure to look for a matching
	 * driver.
	 */
	err = device_add(&dev->dev);
	if (err)
		goto out_of_node_put;

	return 0;

out_of_node_put:
	of_node_put(dev->dev.of_node);
out_ida_remove:
	ida_free(&cnic_index_ida, dev->index);
out:
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_FAILED);
	return err;
}

bool cnic_is_virtio_device(struct device *dev)
{
	return dev->bus == &cnic_virtio_bus;
}

void cnic_unregister_net_device(struct cnic_net_device *dev)
{
	int index = dev->index; /* save for after device release */

	device_unregister(&dev->dev);
	ida_free(&cnic_index_ida, index);
}

#ifdef CONFIG_PM_SLEEP
int cnic_net_device_freeze(struct cnic_net_device *dev)
{
	struct virtio_driver *drv = cnic_drv_to_virtio(dev->dev.driver);
	int ret;

	cnic_net_config_disable(dev);

	dev->failed = dev->config->get_status(dev) & VIRTIO_CONFIG_S_FAILED;

	if (drv && drv->freeze) {
		ret = drv->freeze(dev);
		if (ret) {
			cnic_net_config_enable(dev);
			return ret;
		}
	}

	return 0;
}

int cnic_net_device_restore(struct cnic_net_device *dev)
{
	struct virtio_driver *drv = cnic_drv_to_virtio(dev->dev.driver);
	int ret;

	/* We always start by resetting the device, in case a previous
	 * driver messed it up. */
	cnic_net_reset_device(dev);

	/* Acknowledge that we've seen the device. */
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_ACKNOWLEDGE);

	/* Maybe driver failed before freeze.
	 * Restore the failed status, for debugging. */
	if (dev->failed)
		cnic_net_add_status(dev, VIRTIO_CONFIG_S_FAILED);

	if (!drv)
		return 0;

	/* We have a driver! */
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_DRIVER);

	ret = dev->config->finalize_features(dev);
	if (ret)
		goto err;

	ret = cnic_net_features_ok(dev);
	if (ret)
		goto err;

	if (drv->restore) {
		ret = drv->restore(dev);
		if (ret)
			goto err;
	}

	/* If restore didn't do it, mark device DRIVER_OK ourselves. */
	if (!(dev->config->get_status(dev) & VIRTIO_CONFIG_S_DRIVER_OK))
		cnic_net_device_ready(dev);

	cnic_net_config_enable(dev);

	return 0;

err:
	cnic_net_add_status(dev, VIRTIO_CONFIG_S_FAILED);
	return ret;
}
#endif
extern int  cnic_net_pci_driver_init(void);
extern void cnic_net_pci_driver_exit(void);

extern int cnic_net_driver_init(void);
extern void cnic_net_driver_exit(void);

static int __init cnic_net_init(void)
{
	if (bus_register(&cnic_virtio_bus) != 0) {
		pr_err("cnic virtio bus registration failed\n");
		goto err_bus;
	}

	if (cnic_net_pci_driver_init() != 0) {
		pr_err("cnic pcie driver register failed\n");
		goto err_driver;
	}
	if (cnic_net_driver_init() != 0) {
		pr_err("cnic pcie(virtio) driver register failed\n");
		goto err_driver_virtio;
	}
	return 0;

err_driver_virtio:
	cnic_net_pci_driver_exit();
err_driver:
	bus_unregister(&cnic_virtio_bus);
err_bus:
	return -1;
}

static void __exit cnic_net_exit(void)
{
	cnic_net_driver_exit();
	cnic_net_pci_driver_exit();
	bus_unregister(&cnic_virtio_bus);
	ida_destroy(&cnic_index_ida);
}
module_init(cnic_net_init);
module_exit(cnic_net_exit);

MODULE_LICENSE("GPL");
