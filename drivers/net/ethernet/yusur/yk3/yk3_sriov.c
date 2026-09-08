// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"

static void generate_vf_mac(const u8 *base_mac, u8 *dst_mac, u16 stride)
{
	u32 complex_mac;

	if (!base_mac || !dst_mac)
		return;

	complex_mac = base_mac[3];
	complex_mac |= (base_mac[4] << 8);
	complex_mac |= (base_mac[5] << 16);
	complex_mac += stride;
	memcpy(dst_mac, base_mac, ETH_ALEN);
	dst_mac[5] = (complex_mac >> 16) & 0xff;
	dst_mac[4] = (complex_mac >> 8) & 0xff;
	dst_mac[3] = complex_mac & 0xff;
}

struct yk3_sriov_priv *yk3_sriov_get_priv(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_sriov_priv *priv;

	if (!yk3_pdev_is_pf(pdev_priv))
		return NULL;

	priv = pdev_priv->sriov_priv;

	if (!priv || test_and_set_bit(0, priv->state))
		return NULL;

	if (!pdev_priv->sriov_priv->num_vfs) {
		clear_bit(0, priv->state);
		priv = NULL;
	}

	return priv;
}

void yk3_sriov_put_priv(struct yk3_sriov_priv *priv)
{
	clear_bit(0, priv->state);
}

static void yk3_sriov_mbox_umd_vf_enable(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_sriov_mbox_umd_vf_msg *umd_vf;
	struct yk3_sriov_mbox_ack_msg *ack;
	struct yk3_sriov_priv *sriov_priv = NULL;
	struct yk3_vf_info *vf_info = NULL;
	u16 retry = 5;
	u16 vf_id;
	u32 ret = YK3_SRIOV_MBOX_OK;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	while (!sriov_priv && retry != 0) {
		sriov_priv = yk3_sriov_get_priv(pdev_priv);
		usleep_range(1000, 2000);
		retry--;
	}

	if (!sriov_priv) {
		ret = YK3_SRIOV_MBOX_SRIOV_BUSY;
		goto send_ack;
	}

	vf_id = yk3_mbox_get_src_vf_id(msg->src_id);
	if (vf_id == YK3_MBOX_SRC_ID_NULL) {
		yk3_dev_err("recv umd vf msg from invalid vf src id %d!\n",
			    msg->src_id);
		ret = YK3_SRIOV_MBOX_INVALID_VF;
		goto send_ack;
	}

	umd_vf = (struct yk3_sriov_mbox_umd_vf_msg *)msg->data;
	if (umd_vf->vf_id != vf_id) {
		yk3_dev_err("recv umd vf %d msg from invalid vf src id %d!\n",
			    umd_vf->vf_id, vf_id);
		ret = YK3_SRIOV_MBOX_INVALID_VF;
		goto send_ack;
	}

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf_id);
	if (!vf_info) {
		yk3_dev_err("recv umd vf %d msg get vf info failed!\n",
			    umd_vf->vf_id);
		ret = YK3_SRIOV_MBOX_INVALID_VF;
		goto send_ack;
	}
	vf_info->umd_enable = umd_vf->enable ? true : false;

send_ack:
	if (sriov_priv)
		yk3_sriov_put_priv(sriov_priv);
	ack = (struct yk3_sriov_mbox_ack_msg *)mbox_msg.data;
	ack->ret = ret;
	mbox_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_msg.dst_id = msg->src_id;
	mbox_msg.seqno = msg->seqno;
	mbox_opt.wait_reply = MB_NO_REPLY;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0)
		yk3_dev_err("send sriov umd vf ack mbox message errno %d failed!\n", ret);
}

static int yk3_check_vf_umd(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_sriov_priv *sriov_priv = NULL;
	struct yk3_vf_info *vf_info;
	int num_vfs, i;
	u16 retry = 5;
	u32 umd = 0;

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	while (!sriov_priv && retry) {
		sriov_priv = yk3_sriov_get_priv(pdev_priv);
		usleep_range(1000, 2000);
		retry--;
	}

	if (!sriov_priv) {
		yk3_dev_err("sriov check vf umd status failed!");
		return -1;
	}

	num_vfs = pdev_priv->sriov_priv->num_vfs;
	for (i = 0; i < num_vfs; i++) {
		vf_info = &pdev_priv->sriov_priv->vf_info[i];
		if (vf_info->umd_enable) {
			umd = 1;
			break;
		}
	}
	yk3_sriov_put_priv(sriov_priv);

	return umd != 0 ? -1 : 0;
}

static int yk3_enable_sriov(struct yk3_pdev_priv *pdev_priv, int num_vfs)
{
	int exist_vfs;
	struct yk3_sriov_priv *priv;
	struct yk3_vf_info *vf_info;
	size_t size;
	struct yk3_ndev_priv *ndev_priv;
	int i, ret;
	int qfree_num, vf_qnum;
	int irqfree_num, vf_irqnum;
	struct yk3_queuebase qbase;
	int failed_count;
	bool clear_state = true;

	exist_vfs = pci_num_vf(pdev_priv->pdev);
	if (exist_vfs && exist_vfs != num_vfs) {
		yk3_dev_warn("SR-IOV already enabled with %d VFs", exist_vfs);
		return -EBUSY;
	}
	if (exist_vfs)
		return num_vfs;

	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_PF, -1);
	if (!ndev_priv)
		return -ENODEV;

	if (ndev_priv->umd_enable)
		return -EBUSY;

	qfree_num = yk3_edma_remain_p_qbase(pdev_priv);
	if (qfree_num < num_vfs) {
		yk3_dev_warn("Not enough free queue for VFs");
		return -ENOSPC;
	}

	/* vf need 2 irq leastly(mbox and edma) */
	irqfree_num = yk3_irq_get_freenum(pdev_priv);
	if (irqfree_num < num_vfs * 2) {
		yk3_dev_warn("Not enough free IRQ for VFs");
		return -ENOSPC;
	}

	vf_qnum = qfree_num / num_vfs;
	vf_qnum = min_t(int, vf_qnum, pdev_priv->card->vf_ndev_qnum);
	vf_irqnum = irqfree_num / num_vfs;

	size = num_vfs * sizeof(struct yk3_vf_info);
	vf_info = kzalloc(size, GFP_KERNEL);
	if (!vf_info)
		return -ENOMEM;

	priv = pdev_priv->sriov_priv;
	while (test_and_set_bit(0, priv->state))
		usleep_range(1000, 2000);

	priv->vf_info = vf_info;
	priv->num_vfs = num_vfs;

	for (i = 0; i < num_vfs; i++) {
		vf_info = &priv->vf_info[i];
		vf_info->vf_idx = i;

		ret = yk3_edma_alloc_p_qbase(pdev_priv, vf_qnum, &qbase);
		if (ret) {
			failed_count = i;
			yk3_dev_err("Failed to allocate queue for VF %d", i);
			goto p_qbase_failed;
		}
		vf_info->p_qbase = qbase;
	}

	for (i = 0; i < num_vfs; i++) {
		vf_info = &priv->vf_info[i];

		ret = yk3_qsetid_alloc(pdev_priv, YK3_NDEV_T_VF);
		if (ret < 0) {
			failed_count = i;
			yk3_dev_err("Failed to allocate qsetid for VF %d", i);
			goto qsetid_failed;
		}
		vf_info->qsetid = ret;
	}

	for (i = 0; i < num_vfs; i++) {
		vf_info = &priv->vf_info[i];

		yk3_edma_set_fx_qbase(pdev_priv, vf_info->vf_idx + 1, vf_info->p_qbase);
		yk3_irq_set_func_irqnum(pdev_priv, vf_info->vf_idx + 1, vf_irqnum);
		generate_vf_mac(ndev_priv->ndev->dev_addr, vf_info->mac_addr,
				vf_info->vf_idx + 1);
	}

	yk3_lan_sriov_init(pdev_priv, true);

	clear_bit(0, priv->state);

	ret = pci_enable_sriov(pdev_priv->pdev, num_vfs);
	if (ret) {
		yk3_dev_err("Failed to enable SR-IOV");
		failed_count = num_vfs;
		goto enable_sriov_failed;
	}

	yk3_dev_info("SR-IOV enabled with %d VFs, vf_qnum %d", num_vfs, vf_qnum);

	return num_vfs;

enable_sriov_failed:
	clear_state = false;
	for (i = 0; i < failed_count; i++) {
		qbase.start = 0;
		qbase.num = 0;
		yk3_edma_set_fx_qbase(pdev_priv, priv->vf_info[i].vf_idx + 1, qbase);
		yk3_irq_set_func_irqnum(pdev_priv, priv->vf_info[i].vf_idx + 1, 0);
	}
qsetid_failed:
	for (i = 0; i < failed_count; i++)
		yk3_qsetid_free(pdev_priv, priv->vf_info[i].qsetid);
	failed_count = num_vfs;
p_qbase_failed:
	for (i = 0; i < failed_count; i++)
		yk3_edma_free_p_qbase(pdev_priv, priv->vf_info[i].p_qbase);

	priv->num_vfs = 0;
	vf_info = priv->vf_info;
	priv->vf_info = NULL;
	if (clear_state)
		clear_bit(0, priv->state);
	kfree(vf_info);

	return ret;
}

static void yk3_disable_sriov(struct yk3_pdev_priv *pdev_priv)
{
	int i;
	int num_vfs;
	struct yk3_queuebase qbase;
	struct yk3_vf_info *vf_info;
	struct yk3_sriov_priv *priv;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	priv = pdev_priv->sriov_priv;
	while (test_and_set_bit(0, priv->state))
		usleep_range(1000, 2000);

	if (!pdev_priv->sriov_priv->num_vfs) {
		clear_bit(0, priv->state);
		return;
	}

	pci_disable_sriov(pdev_priv->pdev);

	yk3_lan_sriov_init(pdev_priv, false);

	num_vfs = pdev_priv->sriov_priv->num_vfs;
	for (i = 0; i < num_vfs; i++) {
		vf_info = &pdev_priv->sriov_priv->vf_info[i];

		yk3_irq_set_func_irqnum(pdev_priv, vf_info->vf_idx + 1, 0);

		qbase.start = 0;
		qbase.num = 0;
		yk3_edma_set_fx_qbase(pdev_priv, vf_info->vf_idx + 1, qbase);

		yk3_qsetid_free(pdev_priv, vf_info->qsetid);
		yk3_edma_free_p_qbase(pdev_priv, vf_info->p_qbase);
	}
	pdev_priv->sriov_priv->num_vfs = 0;

	priv->num_vfs = 0;
	vf_info = priv->vf_info;
	priv->vf_info = NULL;
	clear_bit(0, priv->state);

	kfree(vf_info);
}

int yk3_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	int ret = 0;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(pdev);

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EPERM;

	if (num_vfs) {
		ret = yk3_enable_sriov(pdev_priv, num_vfs);
	} else {
		ret = pci_vfs_assigned(pdev);
		if (ret) {
			yk3_dev_warn("VFs are assigned, can't disable SR-IOV");
			ret = -EBUSY;
			goto out;
		}

		if (yk3_check_vf_umd(pdev_priv) != 0) {
			yk3_dev_warn("Some VF take over by umd, can't disable SR-IOV");
			ret = -EBUSY;
			goto out;
		}

		yk3_disable_sriov(pdev_priv);
	}

out:
	return ret;
}

int yk3_sriov_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret;
	struct yk3_sriov_priv *priv;

	if (!yk3_pdev_is_pf(pdev_priv))
		return 0;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->num_vfs = 0;
	pdev_priv->sriov_priv = priv;

	ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_UMD_VF_ENABLE,
					 yk3_sriov_mbox_umd_vf_enable, (void *)pdev_priv);
	if (ret) {
		yk3_dev_err("sriov init register mailbox handler failed!\n");
		goto err_sriov_init;
	}

	yk3_irq_sriov_init(pdev_priv);
	ret = yk3_debug_sriov_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_debug_sriov_init() failed\n");
		goto err_sriov_init;
	}

	return 0;

err_sriov_init:
	pdev_priv->sriov_priv = NULL;
	kfree(priv);
	return ret;
}

void yk3_sriov_exit(struct yk3_pdev_priv *pdev_priv)
{
	yk3_disable_sriov(pdev_priv);
	yk3_debug_sriov_exit(pdev_priv);
	kfree(pdev_priv->sriov_priv);
	pdev_priv->sriov_priv = NULL;
}
