// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved. */

#define pr_fmt(fmt) "CDMA: " fmt
#define dev_fmt pr_fmt

#include <linux/module.h>

#include "cdma.h"
#include "cdma_dev.h"
#include "cdma_chardev.h"
#include <ub/ubase/ubase_comm_dev.h>
#include "cdma_eq.h"
#include "cdma_debugfs.h"
#include "cdma_cmd.h"
#include "cdma_types.h"
#include "cdma_mmap.h"
#include "cdma_context.h"
#include "cdma_uobj.h"
#include "cdma_event.h"

static bool is_rmmod;
DEFINE_MUTEX(g_cdma_reset_mutex);

/* Enabling jfc_arm_mode will cause jfc to report cqe; otherwise, it will not. */
uint jfc_arm_mode;
module_param(jfc_arm_mode, uint, 0444);
MODULE_PARM_DESC(jfc_arm_mode,
		 "Set the ARM mode of the JFC, default: 0(0:Always ARM, others: NO ARM)");

bool cqe_mode = true;
module_param(cqe_mode, bool, 0444);
MODULE_PARM_DESC(cqe_mode, "Set cqe reporting mode, default: 1 (0:BY_COUNT, 1:BY_CI_PI_GAP)");

struct class *cdma_cdev_class;

static int cdma_register_event(struct auxiliary_device *adev)
{
	int ret;

	ret = cdma_reg_ae_event(adev);
	if (ret)
		return ret;

	ret = cdma_reg_ce_event(adev);
	if (ret)
		goto err_ce_register;

	return 0;

err_ce_register:
	cdma_unreg_ae_event(adev);

	return ret;
}

static inline void cdma_unregister_event(struct auxiliary_device *adev)
{
	cdma_unreg_ce_event(adev);
	cdma_unreg_ae_event(adev);
}

static void cdma_reset_unmap_vma_pages(struct cdma_dev *cdev, bool is_reset)
{
	struct cdma_file *cfile;

	mutex_lock(&cdev->file_mutex);
	list_for_each_entry(cfile, &cdev->file_list, list) {
		mutex_lock(&cfile->ctx_mutex);
		cdma_unmap_vma_pages(cfile);
		if (is_reset && cfile->uctx != NULL)
			cfile->uctx->invalid = true;
		mutex_unlock(&cfile->ctx_mutex);
	}
	mutex_unlock(&cdev->file_mutex);
}

static void cdma_client_handler(struct cdma_dev *cdev,
				enum cdma_client_ops client_ops)
{
	struct dma_client *client;

	down_write(&g_clients_rwsem);
	list_for_each_entry(client, &g_client_list, list_node) {
		switch (client_ops) {
		case CDMA_CLIENT_STOP:
			if (client->stop)
				client->stop(cdev->eid);
			break;
		case CDMA_CLIENT_REMOVE:
			if (client->remove)
				client->remove(cdev->eid);
			break;
		case CDMA_CLIENT_ADD:
			if (client->add && client->add(cdev->eid))
				dev_warn(&cdev->adev->dev, "add eid:0x%x, cdev for client:%s failed.\n",
						cdev->eid, client->client_name);
			break;
		}
	}
	up_write(&g_clients_rwsem);
}

static void cdma_reset_down(struct auxiliary_device *adev)
{
	struct cdma_dev *cdev;

	mutex_lock(&g_cdma_reset_mutex);
	cdev = get_cdma_dev(adev);
	if (!cdev || cdev->status == CDMA_SUSPEND) {
		dev_warn(&adev->dev, "cdma device is not ready.\n");
		mutex_unlock(&g_cdma_reset_mutex);
		return;
	}

	cdev->status = CDMA_SUSPEND;
	cdma_cmd_flush(cdev);
	cdma_reset_unmap_vma_pages(cdev, true);
	cdma_client_handler(cdev, CDMA_CLIENT_STOP);
	cdma_unregister_event(adev);
	cdma_dbg_uninit(adev);
	mutex_unlock(&g_cdma_reset_mutex);
}

static void cdma_reset_uninit(struct auxiliary_device *adev)
{
	enum ubase_reset_stage stage;
	struct cdma_dev *cdev;

	mutex_lock(&g_cdma_reset_mutex);
	cdev = get_cdma_dev(adev);
	if (!cdev) {
		dev_info(&adev->dev, "cdma device is not exist.\n");
		mutex_unlock(&g_cdma_reset_mutex);
		return;
	}

	stage = ubase_get_reset_stage(adev);
	if (stage == UBASE_RESET_STAGE_UNINIT && cdev->status == CDMA_SUSPEND) {
		cdma_client_handler(cdev, CDMA_CLIENT_REMOVE);
		cdma_destroy_dev(cdev, is_rmmod);
	}
	mutex_unlock(&g_cdma_reset_mutex);
}

static int cdma_init_dev_info(struct auxiliary_device *auxdev, struct cdma_dev *cdev)
{
	int ret;

	ret = cdma_register_event(auxdev);
	if (ret)
		return ret;

	/* query eu failure does not affect driver loading, as eu can be updated. */
	ret = cdma_ctrlq_query_eu(cdev);
	if (ret)
		dev_warn(&auxdev->dev, "query eu failed, ret = %d.\n", ret);

	ret = cdma_dbg_init(auxdev);
	if (ret)
		dev_warn(&auxdev->dev, "init cdma debugfs failed, ret = %d.\n",
			 ret);

	return 0;
}

static void cdma_free_cfile_uobj(struct cdma_dev *cdev)
{
	struct cdma_file *cfile, *next_cfile;
	struct cdma_jfae *jfae;

	mutex_lock(&cdev->file_mutex);
	list_for_each_entry_safe(cfile, next_cfile, &cdev->file_list, list) {
		list_del(&cfile->list);
		mutex_lock(&cfile->ctx_mutex);
		cdma_cleanup_context_uobj(cfile, CDMA_REMOVE_DRIVER_REMOVE);
		cfile->cdev = NULL;
		if (cfile->uctx) {
			jfae = (struct cdma_jfae *)cfile->uctx->jfae;
			if (jfae)
				wake_up_interruptible(&jfae->jfe.poll_wait);
			cdma_cleanup_context_res(cfile->uctx);
		}
		cfile->uctx = NULL;
		mutex_unlock(&cfile->ctx_mutex);
	}
	mutex_unlock(&cdev->file_mutex);
}

static int cdma_init_dev(struct auxiliary_device *auxdev)
{
	struct cdma_dev *cdev;
	bool is_remove = true;
	int ret;

	dev_dbg(&auxdev->dev, "%s called, matched aux dev(%s.%u).\n",
		 __func__, auxdev->name, auxdev->id);

	cdev = cdma_create_dev(auxdev);
	if (!cdev)
		return -ENOMEM;

	ret = cdma_create_chardev(cdev);
	if (ret) {
		cdma_destroy_dev(cdev, is_remove);
		return ret;
	}

	ret = cdma_init_dev_info(auxdev, cdev);
	if (ret) {
		cdma_destroy_chardev(cdev);
		cdma_destroy_dev(cdev, is_remove);
		return ret;
	}

	cdma_client_handler(cdev, CDMA_CLIENT_ADD);
	return ret;
}

static void cdma_uninit_dev(struct auxiliary_device *auxdev)
{
	struct cdma_dev *cdev;
	int ret;

	dev_dbg(&auxdev->dev, "%s called, matched aux dev(%s.%u).\n",
		 __func__, auxdev->name, auxdev->id);

	mutex_lock(&g_cdma_reset_mutex);
	cdev = dev_get_drvdata(&auxdev->dev);
	if (!cdev) {
		dev_err(&auxdev->dev, "get drvdata from ubase failed.\n");
		ubase_reset_unregister(auxdev);
		mutex_unlock(&g_cdma_reset_mutex);
		return;
	}

	cdev->status = CDMA_SUSPEND;
	cdma_cmd_flush(cdev);
	cdma_client_handler(cdev, CDMA_CLIENT_STOP);
	cdma_client_handler(cdev, CDMA_CLIENT_REMOVE);
	cdma_reset_unmap_vma_pages(cdev, false);

	if (!is_rmmod) {
		ret = ubase_deactivate_dev(auxdev);
		dev_info(&auxdev->dev, "ubase deactivate dev ret = %d.\n", ret);
	}

	ubase_reset_unregister(auxdev);
	cdma_dbg_uninit(auxdev);
	cdma_unregister_event(auxdev);
	cdma_destroy_chardev(cdev);
	cdma_free_cfile_uobj(cdev);
	cdma_destroy_dev(cdev, true);
	mutex_unlock(&g_cdma_reset_mutex);
}

static void cdma_reset_init(struct auxiliary_device *adev)
{
	struct cdma_dev *cdev;

	mutex_lock(&g_cdma_reset_mutex);
	cdev = get_cdma_dev(adev);
	if (!cdev) {
		dev_err(&adev->dev, "cdma device is not exist.\n");
		mutex_unlock(&g_cdma_reset_mutex);
		return;
	}

	if (cdma_register_crq_event(adev)) {
		mutex_unlock(&g_cdma_reset_mutex);
		return;
	}

	if (cdma_create_arm_db_page(cdev))
		goto unregister_crq;

	if (cdma_init_dev_info(adev, cdev))
		goto destory_arm_db_page;

	idr_init(&cdev->ctx_idr);
	spin_lock_init(&cdev->ctx_lock);
	atomic_set(&cdev->cmdcnt, 1);
	cdev->status = CDMA_NORMAL;
	cdma_client_handler(cdev, CDMA_CLIENT_ADD);
	mutex_unlock(&g_cdma_reset_mutex);
	return;

destory_arm_db_page:
	cdma_destroy_arm_db_page(cdev);
unregister_crq:
	cdma_unregister_crq_event(adev);
	mutex_unlock(&g_cdma_reset_mutex);
}

static void cdma_reset_handler(struct auxiliary_device *adev,
			enum ubase_reset_stage stage)
{
	if (!adev)
		return;

	switch (stage) {
	case UBASE_RESET_STAGE_DOWN:
		cdma_reset_down(adev);
		break;
	case UBASE_RESET_STAGE_UNINIT:
		cdma_reset_uninit(adev);
		break;
	case UBASE_RESET_STAGE_INIT:
		if (!is_rmmod)
			cdma_reset_init(adev);
		break;
	default:
		break;
	}
}

static int cdma_probe(struct auxiliary_device *auxdev,
		      const struct auxiliary_device_id *auxdev_id)
{
	int ret;

	ret = cdma_init_dev(auxdev);
	if (ret)
		return ret;

	ubase_reset_register(auxdev, cdma_reset_handler);

	return 0;
}

static void cdma_remove(struct auxiliary_device *auxdev)
{
	cdma_uninit_dev(auxdev);
	pr_info("cdma device remove success.\n");
}

static const struct auxiliary_device_id cdma_id_table[] = {
	{
		.name = UBASE_ADEV_NAME ".cdma",
	},
	{}
};
MODULE_DEVICE_TABLE(auxiliary, cdma_id_table);

static struct auxiliary_driver cdma_driver = {
	.probe = cdma_probe,
	.remove = cdma_remove,
	.name = "cdma",
	.id_table = cdma_id_table,
};

static int __init cdma_init(void)
{
	int ret;

	cdma_cdev_class = class_create("cdma");
	if (IS_ERR(cdma_cdev_class)) {
		pr_err("create cdma class failed.\n");
		return PTR_ERR(cdma_cdev_class);
	}

	ret = auxiliary_driver_register(&cdma_driver);
	if (ret) {
		pr_err("auxiliary register failed.\n");
		goto free_class;
	}

	return 0;

free_class:
	class_destroy(cdma_cdev_class);

	return ret;
}

static void __exit cdma_exit(void)
{
	is_rmmod = true;
	auxiliary_driver_unregister(&cdma_driver);
	class_destroy(cdma_cdev_class);
}

module_init(cdma_init);
module_exit(cdma_exit);
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Hisilicon UBus Crystal DMA Driver");
