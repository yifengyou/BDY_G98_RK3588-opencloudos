// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include "udma_common.h"
#include "udma_dev.h"
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_jetty.h"
#include "udma_jfs.h"
#include "udma_jfc.h"
#include "udma_db.h"
#include "udma_ctrlq_tp.h"
#include <ub/urma/udma/udma_ctl.h>
#include "udma_def.h"

static int send_cmd_query_cqe_aux_info(struct udma_dev *udma_dev,
				       struct udma_cmd_query_cqe_aux_info *info)
{
	struct ubase_cmd_buf cmd_in, cmd_out;
	int ret;

	udma_fill_buf(&cmd_in, UDMA_CMD_GET_CQE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_cqe_aux_info), info);
	udma_fill_buf(&cmd_out, UDMA_CMD_GET_CQE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_cqe_aux_info), info);

	ret = ubase_cmd_send_inout(udma_dev->comdev.adev, &cmd_in, &cmd_out);
	if (ret)
		dev_err(udma_dev->dev,
			"failed to query cqe aux info, ret = %d.\n", ret);

	return ret;
}

static void free_kernel_cqe_aux_info(struct udma_cqe_aux_info_out *user_aux_info_out,
				     struct udma_cqe_aux_info_out *aux_info_out)
{
	if (!user_aux_info_out->aux_info_type)
		return;

	kfree(aux_info_out->aux_info_type);
	aux_info_out->aux_info_type = NULL;

	kfree(aux_info_out->aux_info_value);
	aux_info_out->aux_info_value = NULL;
}

static int copy_out_cqe_data_from_user(struct udma_dev *udma_dev,
				       struct ubcore_user_ctl_out *out,
				       struct udma_cqe_aux_info_out *aux_info_out,
				       struct ubcore_ucontext *uctx,
				       struct udma_cqe_aux_info_out *user_aux_info_out)
{
	if (out->addr != 0 && out->len == sizeof(struct udma_cqe_aux_info_out)) {
		memcpy(aux_info_out, (void *)(uintptr_t)out->addr,
		       sizeof(struct udma_cqe_aux_info_out));
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			if (aux_info_out->aux_info_num > MAX_CQE_AUX_INFO_TYPE_NUM) {
				dev_err(udma_dev->dev,
					"invalid cqe aux info num %u.\n",
					aux_info_out->aux_info_num);
				return -EINVAL;
			}

			user_aux_info_out->aux_info_type = aux_info_out->aux_info_type;
			user_aux_info_out->aux_info_value = aux_info_out->aux_info_value;
			aux_info_out->aux_info_type =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(enum udma_cqe_aux_info_type), GFP_KERNEL);
			if (!aux_info_out->aux_info_type)
				return -ENOMEM;

			aux_info_out->aux_info_value =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(uint32_t), GFP_KERNEL);
			if (!aux_info_out->aux_info_value) {
				kfree(aux_info_out->aux_info_type);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static int copy_out_cqe_data_to_user(struct udma_dev *udma_dev,
				     struct ubcore_user_ctl_out *out,
				     struct udma_cqe_aux_info_out *aux_info_out,
				     struct ubcore_ucontext *uctx,
				     struct udma_cqe_aux_info_out *user_aux_info_out)
{
	unsigned long byte;

	if (out->addr != 0 && out->len == sizeof(struct udma_cqe_aux_info_out)) {
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_type,
					    (void *)aux_info_out->aux_info_type,
					    aux_info_out->aux_info_num *
					    sizeof(enum udma_cqe_aux_info_type));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info type failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_value,
					    (void *)aux_info_out->aux_info_value,
					    aux_info_out->aux_info_num *
					    sizeof(uint32_t));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info value failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			kfree(aux_info_out->aux_info_type);
			kfree(aux_info_out->aux_info_value);
			aux_info_out->aux_info_type = user_aux_info_out->aux_info_type;
			aux_info_out->aux_info_value = user_aux_info_out->aux_info_value;
		}
		memcpy((void *)(uintptr_t)out->addr, aux_info_out,
		       sizeof(struct udma_cqe_aux_info_out));
	}

	return 0;
}

int udma_query_cqe_aux_info(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
				   struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out)
{
	struct udma_cqe_aux_info_out user_aux_info_out = {};
	struct udma_cqe_aux_info_out aux_info_out = {};
	struct udma_cmd_query_cqe_aux_info info = {};
	struct udma_cqe_info_in cqe_info_in = {};
	struct udma_dev *udev = to_udma_dev(dev);
	int ret;

	if (udma_check_base_param(in->addr, in->len, sizeof(struct udma_cqe_info_in))) {
		dev_err(udev->dev, "parameter invalid in query cqe aux info, in_len = %u.\n",
			in->len);
		return -EINVAL;
	}
	memcpy(&cqe_info_in, (void *)(uintptr_t)in->addr,
	       sizeof(struct udma_cqe_info_in));

	ret = copy_out_cqe_data_from_user(udev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udev->dev,
			"copy out data from user failed, ret = %d.\n", ret);
		return ret;
	}

	info.status = cqe_info_in.status;
	info.is_client = !(cqe_info_in.s_r & 1);

	ret = send_cmd_query_cqe_aux_info(udev, &info);
	if (ret) {
		dev_err(udev->dev,
			"send cmd query aux info failed, ret = %d.\n",
			ret);
		free_kernel_cqe_aux_info(&user_aux_info_out, &aux_info_out);
		return ret;
	}

	ret = copy_out_cqe_data_to_user(udev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udev->dev,
			"copy out data to user failed, ret = %d.\n", ret);
		free_kernel_cqe_aux_info(&user_aux_info_out, &aux_info_out);
	}

	return ret;
}

static int to_hw_ae_event_type(struct udma_dev *udma_dev, uint32_t event_type,
			       struct udma_cmd_query_ae_aux_info *info)
{
	switch (event_type) {
	case UBCORE_EVENT_TP_FLUSH_DONE:
		info->event_type = UBASE_EVENT_TYPE_TP_FLUSH_DONE;
		break;
	case UBCORE_EVENT_TP_ERR:
		info->event_type = UBASE_EVENT_TYPE_TP_LEVEL_ERROR;
		break;
	case UBCORE_EVENT_JFS_ERR:
	case UBCORE_EVENT_JETTY_ERR:
		info->event_type = UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR;
		info->sub_type = UBASE_SUBEVENT_TYPE_JFS_CHECK_ERROR;
		break;
	case UBCORE_EVENT_JFC_ERR:
		info->event_type = UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR;
		info->sub_type = UBASE_SUBEVENT_TYPE_JFC_CHECK_ERROR;
		break;
	default:
		dev_err(udma_dev->dev, "Invalid event type %u.\n", event_type);
		return -EINVAL;
	}

	return 0;
}

static int send_cmd_query_ae_aux_info(struct udma_dev *udma_dev,
				      struct udma_cmd_query_ae_aux_info *info)
{
	struct ubase_cmd_buf cmd_in, cmd_out;
	int ret;

	udma_fill_buf(&cmd_in, UDMA_CMD_GET_AE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_ae_aux_info), info);
	udma_fill_buf(&cmd_out, UDMA_CMD_GET_AE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_ae_aux_info), info);

	ret = ubase_cmd_send_inout(udma_dev->comdev.adev, &cmd_in, &cmd_out);
	if (ret)
		dev_err(udma_dev->dev,
			"failed to query ae aux info, ret = %d.\n", ret);

	return ret;
}

static void free_kernel_ae_aux_info(struct udma_ae_aux_info_out *user_aux_info_out,
				    struct udma_ae_aux_info_out *aux_info_out)
{
	if (!user_aux_info_out->aux_info_type)
		return;

	kfree(aux_info_out->aux_info_type);
	aux_info_out->aux_info_type = NULL;

	kfree(aux_info_out->aux_info_value);
	aux_info_out->aux_info_value = NULL;
}

static int copy_out_ae_data_from_user(struct udma_dev *udma_dev,
				      struct ubcore_user_ctl_out *out,
				      struct udma_ae_aux_info_out *aux_info_out,
				      struct ubcore_ucontext *uctx,
				      struct udma_ae_aux_info_out *user_aux_info_out)
{
	if (out->addr != 0 && out->len == sizeof(struct udma_ae_aux_info_out)) {
		memcpy(aux_info_out, (void *)(uintptr_t)out->addr,
		       sizeof(struct udma_ae_aux_info_out));
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			if (aux_info_out->aux_info_num > MAX_AE_AUX_INFO_TYPE_NUM) {
				dev_err(udma_dev->dev,
					"invalid ae aux info num %u.\n",
					aux_info_out->aux_info_num);
				return -EINVAL;
			}

			user_aux_info_out->aux_info_type = aux_info_out->aux_info_type;
			user_aux_info_out->aux_info_value = aux_info_out->aux_info_value;
			aux_info_out->aux_info_type =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(enum udma_ae_aux_info_type), GFP_KERNEL);
			if (!aux_info_out->aux_info_type)
				return -ENOMEM;

			aux_info_out->aux_info_value =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(uint32_t), GFP_KERNEL);
			if (!aux_info_out->aux_info_value) {
				kfree(aux_info_out->aux_info_type);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static int copy_out_ae_data_to_user(struct udma_dev *udma_dev,
				    struct ubcore_user_ctl_out *out,
				    struct udma_ae_aux_info_out *aux_info_out,
				    struct ubcore_ucontext *uctx,
				    struct udma_ae_aux_info_out *user_aux_info_out)
{
	unsigned long byte;

	if (out->addr != 0 && out->len == sizeof(struct udma_ae_aux_info_out)) {
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_type,
					    (void *)aux_info_out->aux_info_type,
					    aux_info_out->aux_info_num *
					    sizeof(enum udma_ae_aux_info_type));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info type failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_value,
					    (void *)aux_info_out->aux_info_value,
					    aux_info_out->aux_info_num *
					    sizeof(uint32_t));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info value failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			kfree(aux_info_out->aux_info_type);
			kfree(aux_info_out->aux_info_value);
			aux_info_out->aux_info_type = user_aux_info_out->aux_info_type;
			aux_info_out->aux_info_value = user_aux_info_out->aux_info_value;
		}
		memcpy((void *)(uintptr_t)out->addr, aux_info_out,
		       sizeof(struct udma_ae_aux_info_out));
	}

	return 0;
}

int udma_query_ae_aux_info(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			   struct ubcore_user_ctl_in *in,
			   struct ubcore_user_ctl_out *out)
{
	struct udma_ae_aux_info_out user_aux_info_out = {};
	struct udma_ae_aux_info_out aux_info_out = {};
	struct udma_dev *udma_dev = to_udma_dev(dev);
	struct udma_cmd_query_ae_aux_info info = {};
	struct udma_ae_info_in ae_info_in = {};
	int ret;

	if (udma_check_base_param(in->addr, in->len, sizeof(struct udma_ae_info_in))) {
		dev_err(udma_dev->dev, "parameter invalid in query ae aux info, in_len = %u.\n",
			in->len);
		return -EINVAL;
	}
	memcpy(&ae_info_in, (void *)(uintptr_t)in->addr,
	       sizeof(struct udma_ae_info_in));
	ret = to_hw_ae_event_type(udma_dev, ae_info_in.event_type, &info);
	if (ret)
		return ret;

	ret = copy_out_ae_data_from_user(udma_dev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udma_dev->dev,
			"copy out data from user failed, ret = %d.\n", ret);
		return ret;
	}

	ret = send_cmd_query_ae_aux_info(udma_dev, &info);
	if (ret) {
		dev_err(udma_dev->dev,
			"send cmd query aux info failed, ret = %d.\n",
			ret);
		free_kernel_ae_aux_info(&user_aux_info_out, &aux_info_out);
		return ret;
	}

	ret = copy_out_ae_data_to_user(udma_dev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udma_dev->dev,
			"copy out data to user failed, ret = %d.\n", ret);
		free_kernel_ae_aux_info(&user_aux_info_out, &aux_info_out);
	}

	return ret;
}

static udma_user_ctl_ops g_udma_user_ctl_k_ops[] = {
	[UDMA_USER_CTL_NPU_REGISTER_INFO_CB] = udma_register_npu_cb,
	[UDMA_USER_CTL_NPU_UNREGISTER_INFO_CB] = udma_unregister_npu_cb,
	[UDMA_USER_CTL_QUERY_CQE_AUX_INFO] = udma_query_cqe_aux_info,
	[UDMA_USER_CTL_QUERY_AE_AUX_INFO] = udma_query_ae_aux_info,
};

static udma_user_ctl_ops g_udma_user_ctl_u_ops[] = {
	[UDMA_USER_CTL_CREATE_JFS_EX] = NULL,
	[UDMA_USER_CTL_DELETE_JFS_EX] = NULL,
	[UDMA_USER_CTL_CREATE_JFC_EX] = NULL,
	[UDMA_USER_CTL_DELETE_JFC_EX] = NULL,
	[UDMA_USER_CTL_SET_CQE_ADDR] = NULL,
	[UDMA_USER_CTL_QUERY_UE_INFO] = NULL,
	[UDMA_USER_CTL_GET_DEV_RES_RATIO] = NULL,
	[UDMA_USER_CTL_NPU_REGISTER_INFO_CB] = NULL,
	[UDMA_USER_CTL_NPU_UNREGISTER_INFO_CB] = NULL,
	[UDMA_USER_CTL_QUERY_CQE_AUX_INFO] = udma_query_cqe_aux_info,
	[UDMA_USER_CTL_QUERY_AE_AUX_INFO] = udma_query_ae_aux_info,
	[UDMA_USER_CTL_QUERY_UBMEM_INFO] = NULL,
	[UDMA_USER_CTL_QUERY_PAIR_DEVNUM] = NULL,
};

static int udma_user_data(struct ubcore_device *dev,
			  struct ubcore_user_ctl *k_user_ctl)
{
	struct udma_dev *udev = to_udma_dev(dev);
	struct ubcore_user_ctl_out out = {};
	struct ubcore_user_ctl_in in = {};
	unsigned long byte;
	int ret;

	if (k_user_ctl->in.len >= UDMA_HW_PAGE_SIZE || k_user_ctl->out.len >= UDMA_HW_PAGE_SIZE) {
		dev_err(udev->dev, "The len exceeds the maximum value in user ctrl.\n");
		return -EINVAL;
	}

	in.opcode = k_user_ctl->in.opcode;
	if (!g_udma_user_ctl_u_ops[in.opcode]) {
		dev_err(udev->dev, "invalid user opcode: 0x%x.\n", in.opcode);
		return -EINVAL;
	}

	if (k_user_ctl->in.len) {
		in.addr = (uint64_t)kzalloc(k_user_ctl->in.len, GFP_KERNEL);
		if (!in.addr)
			return -ENOMEM;

		in.len = k_user_ctl->in.len;
		byte = copy_from_user((void *)(uintptr_t)in.addr,
			(void __user *)(uintptr_t)k_user_ctl->in.addr,
			k_user_ctl->in.len);
		if (byte) {
			dev_err(udev->dev,
				"failed to copy user data in user ctrl, byte = %lu.\n", byte);
			kfree((void *)in.addr);
			return -EFAULT;
		}
	}

	if (k_user_ctl->out.len) {
		out.addr = (uint64_t)kzalloc(k_user_ctl->out.len, GFP_KERNEL);
		if (!out.addr) {
			kfree((void *)in.addr);

			return -ENOMEM;
		}
		out.len = k_user_ctl->out.len;

		if (k_user_ctl->out.addr) {
			byte = copy_from_user((void *)(uintptr_t)out.addr,
				(void __user *)(uintptr_t)k_user_ctl->out.addr,
				k_user_ctl->out.len);
			if (byte) {
				dev_err(udev->dev,
					"failed to copy user data out user ctrl, byte = %lu.\n",
					byte);
				kfree((void *)out.addr);
				kfree((void *)in.addr);

				return -EFAULT;
			}
		}
	}

	ret = g_udma_user_ctl_u_ops[in.opcode](dev, k_user_ctl->uctx, &in, &out);
	kfree((void *)in.addr);

	if (out.addr) {
		byte = copy_to_user((void __user *)(uintptr_t)k_user_ctl->out.addr,
				    (void *)(uintptr_t)out.addr, min(out.len, k_user_ctl->out.len));
		if (byte) {
			dev_err(udev->dev,
				"copy resp to user failed in user ctrl, byte = %lu.\n", byte);
			ret = -EFAULT;
		}

		kfree((void *)out.addr);
	}

	return ret;
}

int udma_user_ctl(struct ubcore_device *dev, struct ubcore_user_ctl *k_user_ctl)
{
	struct udma_dev *udev;

	if (dev == NULL || k_user_ctl == NULL)
		return -EINVAL;

	udev = to_udma_dev(dev);

	if (k_user_ctl->in.opcode >= UDMA_USER_CTL_MAX) {
		dev_err(udev->dev, "invalid opcode: 0x%x.\n", k_user_ctl->in.opcode);
		return -EINVAL;
	}

	if (k_user_ctl->uctx)
		return udma_user_data(dev, k_user_ctl);

	if (!g_udma_user_ctl_k_ops[k_user_ctl->in.opcode]) {
		dev_err(udev->dev, "invalid user opcode: 0x%x.\n", k_user_ctl->in.opcode);
		return -EINVAL;
	}

	return g_udma_user_ctl_k_ops[k_user_ctl->in.opcode](dev, k_user_ctl->uctx, &k_user_ctl->in,
	       &k_user_ctl->out);
}
