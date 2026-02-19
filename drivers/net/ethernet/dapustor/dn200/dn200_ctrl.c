// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Chen Jisi <chenjisi@dapustor.com>
 *
 * Interactive configuration with the controller
 */

#include <linux/iopoll.h>
#include "dn200_ctrl.h"
#include "dn200.h"

static int dn200_nvme_fw_cmd_exec(struct dn200_ctrl_resource *ctrl,
	void *cmd, void *info, u32 len, u32 *ret_len, u32 time_out);

static inline void dn200_lo_hi_writeq(__u64 val, void __iomem *addr)
{
	writel(val, addr);
	writel(val >> 32, addr + 4);
}

static inline __u64 dn200_lo_hi_readq(void __iomem *addr)
{
	const u32 __iomem *p = addr;
	u32 low, high;

	low = readl(p);
	high = readl(p + 1);

	return low + ((u64)high << 32);
}

static inline void ctrl_update_cq_head(struct dn200_ctrl_resource *ctrl)
{
	u32 next_idx = 0;

	next_idx = ctrl->cq_head + 1;
	if (next_idx == ctrl->q_depth) {
		ctrl->cq_head = 0;
		ctrl->cq_phase ^= 1;
	} else {
		ctrl->cq_head = next_idx;
	}
}

static void ctrl_submit_cmd(struct dn200_ctrl_resource *ctrl,
			    struct ctrl_command *cmd)
{
	memcpy(ctrl->sq_cmds + (ctrl->sq_tail << ctrl->sqes),
	       cmd, sizeof(*cmd));
	ctrl->sq_tail = ctrl->sq_tail + 1;
	if (ctrl->sq_tail == ctrl->q_depth)
		ctrl->sq_tail = 0;
	/*wmb to ensure sq_tail's value avail*/
	wmb();
	writel(ctrl->sq_tail, ctrl->dbs);
}

static inline void ctrl_process_cq(struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_completion *hcqe = &ctrl->cqes[ctrl->cq_head];

	if ((le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == ctrl->cq_phase) {
		ctrl->rdata[ctrl->cq_head] = le32_to_cpu(hcqe->result.u32);
		ctrl_update_cq_head(ctrl);
		writel(ctrl->cq_head, ctrl->bar + 0x1000 + 0x4);
	}
}

static irqreturn_t ctrl_irq(int irq, void *data)
{
	struct dn200_ctrl_resource *ctrl = (struct dn200_ctrl_resource *)data;

	dev_dbg(ctrl->dev,
		"%s %d: irq=%d q_depth=%#x sq_tail=%#x cq_head=%#x\n",
		__func__, __LINE__, irq, ctrl->q_depth, ctrl->sq_tail,
		ctrl->cq_head);
	return IRQ_HANDLED;
}

static irqreturn_t dn200_itr_upgrade(int irq, void *data)
{
	struct plat_dn200_data *plat_ex = (struct plat_dn200_data *)data;
	struct dn200_priv *priv = plat_ex->priv_back;
	u32 flags = 0;
	u32 magic_num = 0;
	u32 status = 0;

	if (priv->flag_upgrade)
		return IRQ_HANDLED;
	if (!priv->plat_ex->upgrade_with_flowing)
		return IRQ_HANDLED;
	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, nic_st, &flags);
	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, magic_num, &magic_num);
	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, rsv, &status);
	dev_dbg(priv->device,
		"%s %d flags=0x%x, magic_num = 0x%x\n",
		__func__, __LINE__, flags, magic_num);

	if (flags == DN200_UNFINISH_FLAG) {
		dev_dbg(priv->device, "[loading fw]dn200 load fw fail, for img copy to flash happened err\n");
	} else if (flags == DN200_STOP_FLAG) {
		/*here is necessary, for we need prevent other's action*/
		/*need guarantee it only exec once*/
		set_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		mod_timer(&priv->upgrade_timer, jiffies + msecs_to_jiffies(200));
	} else if (flags == DN200_JMP_FAIL_FLAG) {
		return IRQ_HANDLED; /*it not necessary,but give it*/
	} else if (flags == DN200_START_FLAG) {
		return IRQ_HANDLED; /*it not necessary,but give it*/
	}
	return IRQ_HANDLED;
}
static irqreturn_t dn200_itr_peer_noitfy(int irq, void *data)
{
	struct plat_dn200_data *plat_ex = (struct plat_dn200_data *)data;
	struct dn200_priv *priv = plat_ex->priv_back;
	u8 hw_reset = 0;
	u8 flow_state = 0;
	u8 mac_reset = 0;
	u8 link_reset = 0;
	u8 carrier_reset = 0;
	u8 rxp_task;

	DN200_ITR_SYNC_GET(priv->hw, itr_sync_app, HW_RESET_ID, &hw_reset);
	if (PRIV_IS_VF(priv))
		dev_dbg(priv->device, "%s, %d, peer to pf hw reset notify:%#x\n",
			__func__, __LINE__, hw_reset);
	if (hw_reset && !PRIV_IS_VF(priv)) {
		/* pf process err reset notification from vf, run global err reset flow
		 */
		dev_dbg(priv->device, "%s, %d, pf process rst from vf, hw_reset state:%d\n",
			__func__, __LINE__, hw_reset);
		dn200_pf_glb_err_rst_process(priv);
		DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, HW_RESET_ID, 0);
	}

	DN200_ITR_SYNC_GET(priv->hw, itr_sync_app, FLOW_STATE_ID, &flow_state);
	if (flow_state && PRIV_IS_VF(priv)) {
		DN200_ITR_SYNC_GET(priv->hw, vf_flow_state_event,
			   DN200_VF_OFFSET_GET(priv->hw), &flow_state);
		if (flow_state == FLOW_CLOSE_START) {
			DN200_ITR_SYNC_SET(priv->hw, vf_flow_state_event,
					DN200_VF_OFFSET_GET(priv->hw),
					FLOW_CLOSE_DONE);
			netif_carrier_off(priv->dev);
			set_bit(DN200_VF_FLOW_CLOSE, &priv->state);
		}
		if (flow_state == FLOW_OPEN_START) {
			DN200_ITR_SYNC_SET(priv->hw, vf_flow_state_event,
					DN200_VF_OFFSET_GET(priv->hw),
					FLOW_OPEN_DONE);
			set_bit(DN200_VF_FLOW_OPEN_SET, &priv->state);
		}
		if (!test_and_set_bit(DN200_VF_FLOW_STATE_SET, &priv->state))
			dn200_vf_work(priv);
	}

	if (PRIV_IS_VF(priv)) {
		DN200_ITR_SYNC_GET(priv->hw, vf_reset_mac_list,
				   priv->plat_ex->vf_offset, &mac_reset);
		if (mac_reset) {
			dn200_vf_mac_change(priv);
			DN200_ITR_SYNC_SET(priv->hw, vf_reset_mac_list,
					   priv->plat_ex->vf_offset, 0);
		}
		DN200_ITR_SYNC_GET(priv->hw, vf_link_list,
				   priv->plat_ex->vf_offset, &link_reset);
		if (link_reset) {
			dn200_vf_link_set(priv, link_reset);
			DN200_ITR_SYNC_SET(priv->hw, vf_reset_mac_list,
					   priv->plat_ex->vf_offset, 0);
		}

		DN200_ITR_SYNC_GET(priv->hw, pf_carrier, 0, &carrier_reset);
		netdev_dbg(priv->dev, "%s %d notify vf link off, get %d\n", __func__, __LINE__, carrier_reset);
		if (carrier_reset) {
			if (netif_running(priv->dev)) {
				netif_carrier_off(priv->dev);
				netif_tx_stop_all_queues(priv->dev);
			}
			DN200_ITR_SYNC_SET(priv->hw, vf_carrier, priv->plat_ex->vf_offset, 1);
			netdev_dbg(priv->dev, "%s %d notify pf wb vf %d\n", __func__, __LINE__, priv->plat_ex->vf_offset);
		}
	}
	if (PRIV_SRIOV_SUPPORT(priv)) {
		DN200_ITR_SYNC_GET(priv->hw, itr_sync_app, RXP_TASK, &rxp_task);
		dn200_async_rxp_work(priv);
		DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, RXP_TASK, 0);
	}
	return IRQ_HANDLED;
}

static int ctrl_alloc_queue(struct dn200_ctrl_resource *ctrl)
{
	int ret = 0;

	if (ctrl->addr64 > 32)
		ret = dma_set_mask_and_coherent(ctrl->dev, DMA_BIT_MASK(32));
	ctrl->cqes =
	    dma_alloc_coherent(ctrl->dev,
			       ctrl->q_depth * sizeof(struct ctrl_completion),
			       &ctrl->cq_dma_addr, GFP_KERNEL);
	if (!ctrl->cqes)
		goto free_ctrlq;

	ctrl->sq_cmds = dma_alloc_coherent(ctrl->dev, ctrl->q_depth * 64,
					   &ctrl->sq_dma_addr, GFP_KERNEL);
	if (!ctrl->sq_cmds)
		goto free_cqdma;
	if (ctrl->addr64 > 32)
		ret = dma_set_mask_and_coherent(ctrl->dev, DMA_BIT_MASK(64));

	return ret;

free_cqdma:
	dma_free_coherent(ctrl->dev,
			  ctrl->q_depth * sizeof(struct ctrl_completion),
			  (void *)ctrl->cqes, ctrl->cq_dma_addr);
free_ctrlq:
	return -ENOMEM;
}

static void ctrl_init_queue(struct dn200_ctrl_resource *ctrl)
{
	ctrl->sq_tail = 0;
	ctrl->last_sq_tail = 0;
	ctrl->cq_head = 0;
	ctrl->cq_phase = 1;
	memset((void *)ctrl->cqes, 0,
	       ctrl->q_depth * sizeof(struct ctrl_completion));
	wmb();			/* ensure the first interrupt sees the initialization */
}

static void ctrl_init(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl)
{
	memset(ctrl, 0, sizeof(*ctrl));
	ctrl->pcie_ava = true;
	ctrl->bar = pcim_iomap_table(pdev)[0];
	ctrl->dev = &pdev->dev;
	ctrl->q_depth = SQ_DEPTH;
	ctrl->sqes = 6;
	ctrl->cap = dn200_lo_hi_readq(ctrl->bar + REG_CAP);
	ctrl->dbs = ctrl->bar + 4096;
	ctrl->qid = 0;		/*only admin q */
}

static void shutdown_nvme_ctrl(struct dn200_ctrl_resource *ctrl)
{
	u32 config = 0;

	config = readl(ctrl->bar + REG_CC);
	config &= ~CTRL_CC_SHN_MASK;
	config |= CTRL_CC_SHN_NORMAL;
	writel(config, ctrl->bar + REG_CC);
	mdelay(100);
}

void shutdown_ctrl(struct dn200_ctrl_resource *ctrl)
{
	struct msix_entry *msix_entries = ctrl->msix_entries;

	if (ctrl->pcie_ava)
		shutdown_nvme_ctrl(ctrl);
	dma_free_coherent(ctrl->dev,
			  ctrl->q_depth * sizeof(struct ctrl_completion),
			  (void *)ctrl->cqes, ctrl->cq_dma_addr);
	dma_free_coherent(ctrl->dev, ctrl->q_depth * 64, ctrl->sq_cmds,
			  ctrl->sq_dma_addr);
	synchronize_irq(msix_entries[0].vector);
	devm_free_irq(ctrl->dev, msix_entries[0].vector, ctrl);
}

static void disable_ctrl(struct dn200_ctrl_resource *ctrl)
{
	u32 data;
	int ret;

	clear_bit(ADMIN_QUEUE_INITED, &ctrl->admin_state);

	writel(0, ctrl->bar + REG_CC);	/*disable_ctrl */
	if (ctrl->is_extern_phy)
		ret = readl_poll_timeout(ctrl->bar +
				REG_CSTS, data, !(data & BIT(0)), 100, 30000000);
	else
		ret = readl_poll_timeout_atomic(ctrl->bar +
				REG_CSTS, data, !(data & BIT(0)), 100, 30000000);
	if (ret)
		dev_err(ctrl->dev, "func %s, line %d: ctrl cc disable timeout\n",
			__func__, __LINE__);
}

static void enable_ctrl(struct dn200_ctrl_resource *ctrl)
{
	u32 config;

	ctrl->cap = dn200_lo_hi_readq(ctrl->bar + REG_CAP);	/*enable ctrl */
	if (((ctrl->cap >> 37) & 0xff) & CTRL_CAP_CSS_CSI)
		config = CTRL_CC_CSS_CSI;
	else
		config = 0;
	config |= CTRL_CC_IOSQES | CTRL_CC_IOCQES;

	writel(config, ctrl->bar + REG_CC);
}

int dn200_ctrl_ccena(struct pci_dev *pdev, bool off, bool on, bool is_atomic)
{
	u32 data;
	int ret0 = 0, ret1 = 0;

	if (off) {
		writel(readl(pcim_iomap_table(pdev)[0] + REG_CC) & 0xfffe,
		       pcim_iomap_table(pdev)[0] + REG_CC);
		if (is_atomic)
			ret0 =
				readl_poll_timeout_atomic(pcim_iomap_table(pdev)[0] +
							REG_CSTS, data, !(data & BIT(0)),
							100, 10000000);
		else
			ret0 =
				readl_poll_timeout(pcim_iomap_table(pdev)[0] +
							REG_CSTS, data, !(data & BIT(0)),
							100, 30000000);
	}
	if (on) {
		writel(readl(pcim_iomap_table(pdev)[0] + REG_CC) | 1,
		       pcim_iomap_table(pdev)[0] + REG_CC);
		if (is_atomic)
			ret1 =
				readl_poll_timeout_atomic(pcim_iomap_table(pdev)[0] +
							REG_CSTS, data, (data & BIT(0)),
							100, 30000000);
		else
			ret1 =
				readl_poll_timeout(pcim_iomap_table(pdev)[0] +
							REG_CSTS, data, (data & BIT(0)),
							100, 30000000);
	}
	/* mask nvme intr pin */
	writel(0xffffffff, pcim_iomap_table(pdev)[0] + REG_INTMS);
	return ret0 || ret1;
}

int dn200_ena_msix_range(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl,
			 int tx_queues_to_use, int rx_queues_to_use,
			 bool is_purepf)
{
	int i, ret;
	int num_vectors, total_vecs, irq_num;
	struct msix_entry *msix_entries;
	struct plat_dn200_data *plat_ex;

	/* clear nvme intr mask */
	writel(0xffffffff, ctrl->bar + REG_INTMC);
	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	/*tx, rx, admin, others, notify, upgrade*/
	num_vectors =
	    tx_queues_to_use + rx_queues_to_use + 1 + (plat_ex->is_vf ? 0 : 4) +
	    1 + 1;
	if (num_vectors > pci_msix_vec_count(pdev)) {
		dev_err(ctrl->dev,
			"func %s, line %d: num_vectors = %d, tx_queues_to_use = %d, rx_queues_to_use = %d.\n",
			__func__, __LINE__, num_vectors, tx_queues_to_use,
			rx_queues_to_use);
		return DN200_FAILURE;
	}

	msix_entries =
	    devm_kzalloc(ctrl->dev, (sizeof(struct msix_entry) * num_vectors),
			 GFP_KERNEL);
	if (!msix_entries)
		return -ENOMEM;

	for (i = 0; i < num_vectors; i++)
		msix_entries[i].entry = i;

	total_vecs =
	    pci_enable_msix_range(pdev, msix_entries, num_vectors, num_vectors);
	if (total_vecs < num_vectors) {
		dev_err(ctrl->dev,
			"func %s, line %d: alloc vec failed! total_vecs = %d, need %d\n",
			__func__, __LINE__, total_vecs, num_vectors);
		ret = total_vecs;
		goto no_msix;
	}
	ctrl->msix_entries = msix_entries;

	irq_num = msix_entries[0].vector;
	memset(ctrl->itr_name_ctrl, 0, sizeof(ctrl->itr_name_ctrl));
	sprintf(ctrl->itr_name_ctrl, "%s(%d:%d):%s-%d", DN200_RESOURCE_NAME,
		PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
		"ctrl", plat_ex->funcid);
	ret =
	    devm_request_irq(ctrl->dev, irq_num, ctrl_irq, 0,
			     ctrl->itr_name_ctrl, ctrl);
	if (ret) {
		dev_err(ctrl->dev,
			"func %s, line %d: ctrl irq_num = %d register fail.\n",
			__func__, __LINE__, irq_num);
		goto err_ctrl_irq;
	}

	irq_num = msix_entries[num_vectors - 2].vector;
	memset(ctrl->itr_name_peer_notify, 0,
	       sizeof(ctrl->itr_name_peer_notify));
	sprintf(ctrl->itr_name_peer_notify, "%s(%d:%d):%s-%d",
		DN200_RESOURCE_NAME, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn), "notify", plat_ex->funcid);
	ret =
	    devm_request_irq(ctrl->dev, irq_num, dn200_itr_peer_noitfy, 0,
			     ctrl->itr_name_peer_notify, plat_ex);
	if (ret) {
		dev_err(ctrl->dev,
			"func %s, line %d: notice irq_num = %d register fail.\n",
			__func__, __LINE__, irq_num);
		goto err_notice_irq;
	}

	irq_num = msix_entries[num_vectors - 1].vector;
	memset(ctrl->itr_name_upgrade, 0,
	       sizeof(ctrl->itr_name_upgrade));
	sprintf(ctrl->itr_name_upgrade, "%s(%d:%d):%s-%d",
		DN200_RESOURCE_NAME, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn), "upgrade", plat_ex->funcid);
	ret =
	    devm_request_irq(ctrl->dev, irq_num, dn200_itr_upgrade, 0,
			     ctrl->itr_name_upgrade, plat_ex);
	if (ret) {
		dev_err(ctrl->dev,
			"func %s, line %d: notice irq_num = %d register fail.\n",
			__func__, __LINE__, irq_num);
		goto err_upgrade_irq;
	}
	plat_ex->total_irq = num_vectors;
	return 0;
err_upgrade_irq:
	devm_free_irq(ctrl->dev, msix_entries[num_vectors - 1].vector, plat_ex);
err_notice_irq:
	devm_free_irq(ctrl->dev, msix_entries[num_vectors - 2].vector, plat_ex);
err_ctrl_irq:
	devm_free_irq(ctrl->dev, msix_entries[0].vector, ctrl);

	pci_disable_msix(pdev);
no_msix:
	kfree(msix_entries);
	return ret;
}

static inline bool nvme_cqe_pending(struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_completion *hcqe = &ctrl->cqes[ctrl->cq_head];

	return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == ctrl->cq_phase;
}

static inline int ctrl_poll_process_cq(struct dn200_ctrl_resource *ctrl, bool is_interupt, u32 delay_times)
{
	struct ctrl_completion *hcqe;
	int found = 0;
	int try = 0;
	int ret = 0;

	do {
		if (is_interupt)
			usleep_range(5, 10);
		else
			udelay(5);
		try++;
		while (nvme_cqe_pending(ctrl)) {
			found++;
			/* load-load control dependency between phase and the rest of
			 * the cqe requires a full read memory barrier
			 */
			dma_rmb();
			hcqe = &ctrl->cqes[ctrl->cq_head];
			ctrl->rdata[ctrl->cq_head] =
			    le64_to_cpu(hcqe->result.u64);
			ctrl_update_cq_head(ctrl);
		}
		if (found && ctrl->cq_head == ctrl->sq_tail)
			break;
	} while (try < delay_times);
	dev_dbg(ctrl->dev, "found=%d, try = %d\n", found, try);
	if (found) {
		writel(ctrl->cq_head, ctrl->dbs + 0x4);
		return ret;
	}

	dev_dbg(ctrl->dev, "driver not found cq! func=%s, line=%d\n",
		__func__, __LINE__);
	return -EIO;
}

static void dn200_admin_process_atomic(struct dn200_ctrl_resource *ctrl,
			 struct ctrl_command *c, int *ret_val, u32 *cq_val);
static void dn200_admin_process(struct dn200_ctrl_resource *ctrl,
			 struct ctrl_command *c, int *ret_val, u32 *cq_val);
static const struct admin_process_ops dn200_admin_process_ops_atomic = {
	.dn200_admin_process = dn200_admin_process_atomic,
};

static const struct admin_process_ops dn200_admin_process_ops = {
	.dn200_admin_process = dn200_admin_process,
};

static int get_funcid(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl);
int admin_queue_configure(struct pci_dev *pdev,
			  struct dn200_ctrl_resource *ctrl, bool is_purepf, bool is_extern_phy)
{
	u32 aqa;
	int ret;
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	ctrl_init(pdev, ctrl);
	ctrl->is_extern_phy = is_extern_phy;
	if ((readl(ctrl->bar + REG_CSTS) & CSTS_NSSRO))
		writel(CSTS_NSSRO, ctrl->bar + REG_CSTS);

	disable_ctrl(ctrl);

	if (ctrl_alloc_queue(ctrl))
		return -ENOMEM;

	aqa = ctrl->q_depth - 1;
	aqa |= aqa << 16;
	writel(aqa, ctrl->bar + REG_AQA);
	dn200_lo_hi_writeq(ctrl->sq_dma_addr, ctrl->bar + REG_ASQ);
	dn200_lo_hi_writeq(ctrl->cq_dma_addr, ctrl->bar + REG_ACQ);

	enable_ctrl(ctrl);

	ctrl_init_queue(ctrl);

	ret = dn200_ctrl_ccena(pdev, 1, 1, false);
	if (ret) {
		dev_err(ctrl->dev, "func %s, line %d: ctrl cc enable timeout\n",
			__func__, __LINE__);
		return ret;
	}
	set_bit(ADMIN_QUEUE_INITED, &ctrl->admin_state);

	if (ctrl->is_extern_phy) {
		mutex_init(&ctrl->mlock);
		ctrl->dn200_admin_process_ops = &dn200_admin_process_ops;
	} else {
		spin_lock_init(&ctrl->lock);
		ctrl->dn200_admin_process_ops = &dn200_admin_process_ops_atomic;
	}
	if (is_purepf) {
		plat_ex->funcid = pdev->devfn;
		return 0;
	}
	ret = get_funcid(pdev, ctrl);
	if (ret < 0) {
		dma_free_coherent(ctrl->dev,
			  ctrl->q_depth * sizeof(struct ctrl_completion),
			  (void *)ctrl->sq_cmds, ctrl->sq_dma_addr);
		dma_free_coherent(ctrl->dev,
			  ctrl->q_depth * sizeof(struct ctrl_completion),
			  (void *)ctrl->cqes, ctrl->cq_dma_addr);

		shutdown_nvme_ctrl(ctrl);
	}
	return ret;
}

int ctrl_reinitial(struct dn200_ctrl_resource *ctrl)
{
	int ret = 0;
	u32 aqa;
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (!dn200_hwif_id_check(plat_ex->io_addr))
		return -EIO;

	aqa = ctrl->q_depth - 1;
	aqa |= aqa << 16;
	writel(aqa, ctrl->bar + REG_AQA);
	dn200_lo_hi_writeq(ctrl->sq_dma_addr, ctrl->bar + REG_ASQ);
	dn200_lo_hi_writeq(ctrl->cq_dma_addr, ctrl->bar + REG_ACQ);
	enable_ctrl(ctrl);
	ctrl_init_queue(ctrl);
	if (ctrl->is_extern_phy)
		ret = dn200_ctrl_ccena(plat_ex->pdev, 1, 1, false);
	else
		ret = dn200_ctrl_ccena(plat_ex->pdev, 1, 1, true);
	if (ret) {
		dev_err(ctrl->dev, "func %s, line %d:cc enable timeout\n",
				__func__, __LINE__);
		return ret;
	}
	udelay(10);
	set_bit(ADMIN_QUEUE_INITED, &ctrl->admin_state);
	return ret;
}

int ctrl_reset(struct dn200_ctrl_resource *ctrl, bool reset_irq)
{
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (!dn200_hwif_id_check(plat_ex->io_addr))
		return -EIO;
	if (test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	shutdown_nvme_ctrl(ctrl);
	disable_ctrl(ctrl);
	return ctrl_reinitial(ctrl);
}

static void dn200_admin_process_atomic(struct dn200_ctrl_resource *ctrl,
			 struct ctrl_command *c, int *ret_val, u32 *cq_val)
{
	unsigned long flags;
	int idx = 0;

	if (!test_bit(ADMIN_QUEUE_INITED, &ctrl->admin_state))
		return;

	spin_lock_irqsave(&ctrl->lock, flags);
	idx = ctrl->cq_head;
	ctrl_submit_cmd(ctrl, c);
	*ret_val = ctrl_poll_process_cq(ctrl, false, DN200_POLL_CQ_MAX_TIMES_ATOMIC);
	*cq_val = ctrl->rdata[idx];
	spin_unlock_irqrestore(&ctrl->lock, flags);
}

static void dn200_admin_process(struct dn200_ctrl_resource *ctrl,
			 struct ctrl_command *c, int *ret_val, u32 *cq_val)
{
	int idx = 0;

	if (!test_bit(ADMIN_QUEUE_INITED, &ctrl->admin_state))
		return;

	mutex_lock(&ctrl->mlock);
	idx = ctrl->cq_head;
	ctrl_submit_cmd(ctrl, c);
	*ret_val = ctrl_poll_process_cq(ctrl, true, DN200_POLL_CQ_MAX_TIMES);
	*cq_val = ctrl->rdata[idx];
	mutex_unlock(&ctrl->mlock);
}

int irq_queue_map(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl,
		  int tx_queues_to_use, int rx_queues_to_use, bool need_retry)
{
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	struct ctrl_command c = { };
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);

	if (tx_queues_to_use == 0 || rx_queues_to_use == 0) {
		dev_err(ctrl->dev,
			"func %s, line %d: tx_queues_to_use =%d, rx_queues_to_use =%d\n",
			__func__, __LINE__, tx_queues_to_use, rx_queues_to_use);
		return DN200_FAILURE;
	}
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
again:
	memset(&c, 0, sizeof(c));
	c.irq_share_command.opcode = ctrl_admin_vendor_start;
	c.irq_share_command.set = IRQ_INFO_CONFIG;
	c.irq_share_command.tx_q =
	    tx_queues_to_use << 16 | plat_ex->tx_queue_start;
	c.irq_share_command.rx_q =
	    rx_queues_to_use << 16 | plat_ex->rx_queue_start;
	c.irq_share_command.dword15 = 0;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret && need_retry) {
		dev_err(&pdev->dev, "%s, %d, poll cq fail\n", __func__,
			__LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, false);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
	}
	return ret;
}

int irq_info_pfvf_release(struct pci_dev *pdev,
			  struct dn200_ctrl_resource *ctrl, bool need_retry)
{
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	struct ctrl_command c = { };
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.irq_release_command.opcode = ctrl_admin_vendor_start;
	c.irq_release_command.set = IRQ_INFO_PFVF_RELEASE;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret && need_retry) {
		dev_err(&pdev->dev, "%s, %d, poll cq fail\n", __func__,
			__LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, false);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
	}
	return ret;
}

static int get_funcid(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl)
{
	struct plat_dn200_data *plat_ex;
	struct ctrl_command c = { };
	int ret = 0;
	int idx = 0;
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	idx = ctrl->cq_head;
	c.irq_release_command.opcode = ctrl_admin_vendor_start;
	c.irq_release_command.set = GET_FUNCID;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	plat_ex->funcid = cq_val;
	if (ret) {
		dev_err(&pdev->dev, "%s, %d, poll cq fail\n", __func__,
			__LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, false);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
	}
	return ret;
}

int fw_reg_write(struct dn200_ctrl_resource *ctrl, u32 reg, u32 value)
{
	struct ctrl_command c = { };
	int ret = 0;
	struct plat_dn200_data *plat_ex = NULL;
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = REG_WRITE;
	c.rw_command.addr = reg;
	c.rw_command.value = value;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret) {
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		dev_err(&plat_ex->pdev->dev, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

int fw_reg_read(struct dn200_ctrl_resource *ctrl, u32 reg, u32 *value)
{
	struct ctrl_command c = { };
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (plat_ex->priv_back && test_bit(DN200_PCIE_UNAVAILD, &plat_ex->priv_back->state))
		return -EIO;
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = REG_READ;
	c.rw_command.addr = reg;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, value);
	if (ret) {
		dev_err(&plat_ex->pdev->dev, "%s, %d, poll cq fail\n",
			__func__, __LINE__);

		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

int fw_link_state_set(struct dn200_ctrl_resource *ctrl, u8 link_state, u8 duplex, u32 speed)
{
	struct ctrl_command c = { };
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	u32 cq_val;
	u8 speed_sel = 0;
	bool have_reset = false;

	if (ctrl->is_extern_phy)
		return 0;
	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	while (speed) {
		speed = speed / 10;
		if (speed == 1)
			break;
		speed_sel++;
	}
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.irq_release_command.opcode = ctrl_admin_vendor_start;
	c.irq_release_command.set = FW_STATE_GET;
	c.irq_release_command.dword13 |= (link_state & 0x1);
	c.irq_release_command.dword13 |= ((duplex & 0x1) << 1);
	c.irq_release_command.dword13 |= ((speed_sel & 0x3) << 2);
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

int lram_and_rxp_lock_and_unlock(struct dn200_ctrl_resource *ctrl, u32 value,
				 u32 *ret_val)
{
	struct ctrl_command c = { };
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = LRAM_RXP_LOCK;
	c.rw_command.value = value;
	c.rw_command.addr = plat_ex->pf_id;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	*ret_val = cq_val & 0xff;
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	if (*ret_val == 0xff)
		dev_err(plat_ex->priv_back->device, "%s %d lock failed ! invalid cmd.\n",
			__func__, __LINE__);
	else
		*ret_val &= 0x1;

	return ret;
}

static int get_hw_type(struct dn200_ctrl_resource *ctrl, u32 *value, u32 type)
{
	struct ctrl_command c = { };
	int ret = 0;
	int idx;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	idx = ctrl->cq_head;
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = type;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, value);
	if (ret) {
		dev_err(ctrl->dev, "%s, %d, poll cq fail\n", __func__,
			__LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, false);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
	}
	return ret;
}

int get_pcb_type(struct dn200_ctrl_resource *ctrl, u32 *value)
{
	return get_hw_type(ctrl, value, PCB_TYPE);
}

int get_rj45_type(struct dn200_ctrl_resource *ctrl, u32 *value)
{
	return get_hw_type(ctrl, value, RJ45_TYPE);
}

int dn200_led_blink_ctrl(struct dn200_ctrl_resource *ctrl, bool is_enable)
{
	struct ctrl_command c = { };
	int ret = 0;
	struct plat_dn200_data *plat_ex;
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_vendor_start;
	c.common_command.cdw12 = FW_PWM_CTRL;
	c.common_command.cdw13 = is_enable;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret) {
		dev_err(ctrl->dev, "%s, %d, poll cq fail\n", __func__,
			__LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

/**
 * irq_peer_notify - notify peer(pf or vf) through hw interrupt.
 *  @pdev : pcie device
 *  @ctrl: ctrl structure
 *  Description:
 *  this is a fast notification function called by vf or pf to send msg to peer.
 *  for pf, will send the hw interrupt to all vfs
 *  for vf, just send the hw interrupt to pf
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
int irq_peer_notify(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_command c = { };
	struct plat_dn200_data *plat_ex;
	int ret = 0;
	u32 cq_val = 0;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	c.irq_release_command.opcode = ctrl_admin_vendor_start;
	c.irq_release_command.set = IRQ_INFO_NOTICE;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret) {
		if (plat_ex->priv_back)
			dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
				__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

/**
 * dn200_ctrl_res_free - free ctrl all resources.
 *  @pdev : pcie device
 *  @ctrl: ctrl structure
 *  Description:
 *  this is the free function called by pcie remove, suppend, etc.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
int dn200_ctrl_res_free(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl)
{
	int ret;
	int peer_notify_vect;
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	peer_notify_vect = plat_ex->total_irq - 2;
	if (ctrl->pcie_ava && dn200_hwif_id_check(plat_ex->io_addr)) {
		ret = irq_info_pfvf_release(pdev, ctrl, true);
		if (ret)
			dev_err(&pdev->dev,
				"func %s, line %d: pfvf_release fail\n",
				__func__, __LINE__);
	}
	shutdown_ctrl(ctrl);
	synchronize_irq(ctrl->msix_entries[peer_notify_vect].vector);
	devm_free_irq(ctrl->dev, ctrl->msix_entries[peer_notify_vect].vector,
		      plat_ex);

	synchronize_irq(ctrl->msix_entries[peer_notify_vect + 1].vector);
	devm_free_irq(ctrl->dev, ctrl->msix_entries[peer_notify_vect + 1].vector,
		      plat_ex);
	if (ctrl->is_extern_phy)
		mutex_destroy(&ctrl->mlock);
	return 0;
}

static void dn200_parse_fw_load_err(struct device *dev, int err_code)
{
	int i;
	struct fw_load_err {
		int err;
		char *info;
	} err_info[] = {
		{0x4006, "the NIC system is busy"},
		{0x400c, "the NIC upgrade in progress"},
		{0x4002, "the NIC nvme parameter err"},
		{0x4013, "the NIC nvme prp para err"},
		{0x4007, "the NIC upgrade req abort/fw is invalid"},
	};

	for (i = 0; i < ARRAY_SIZE(err_info); i++) {
		if (err_info[i].err == err_code) {
			dev_err(dev, "[loading fw]%s, sf %#x.\n", err_info[i].info, err_code);
			return;
		}
	}

	dev_err(dev, "[loading fw]fw download cmd, sf %#x.\n", err_code);
}

int dn200_nvme_fw_load(struct dn200_ctrl_resource *ctrl, const char *fw, size_t fw_size)
{
	struct ctrl_command c = {};
	int ret = 0;
	int idx;
	u32 pos, len, seg_len;
	u32 cq_val = 0;
	void *buf;
	dma_addr_t addr;
	struct page *page;
	struct dn200_priv *priv;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	priv = plat_ex->priv_back;
	page = dn200_alloc_dma_page_dir(priv, &addr, DMA_TO_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);

	len = fw_size;
	seg_len = CTRL_MAX_SEG_LEN;

	for (pos = 0; pos < len; pos += seg_len) {
		seg_len = min(seg_len, len - pos);
		memcpy(buf, fw + pos, seg_len);

		/*wmb to protect buf avail*/
		wmb();
		if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state)) {
			dn200_free_dma_page_dir(priv, page, addr, DMA_TO_DEVICE);
			return -EIO;
		}
again:
		memset(&c, 0, sizeof(c));
		c.fw_download_command.opcode = ctrl_admin_download_fw;
		c.fw_download_command.dptr.prp1 = cpu_to_le64(addr);
		c.fw_download_command.numd =
			(cpu_to_le32(seg_len) >> 2) > 0 ?
				((cpu_to_le32(seg_len) >> 2) - 1) :
				0;
		c.fw_download_command.ofst = cpu_to_le32(pos) >> 2;
		ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
		if (ret) {
			dev_err(ctrl->dev,
				 "[loading fw]downloading, get cq fail.\n");
			if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) &&
					 !have_reset) {
				ret = ctrl_reset(ctrl, true);
				have_reset = true;
				if (!ret)
					goto again;
			}
			set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
			if (plat_ex->priv_back)
				dn200_fw_err_dev_close(plat_ex->priv_back);
			break;
		}

		idx = (ctrl->cq_head + ctrl->q_depth - 1) % ctrl->q_depth;
		/* check status field */
		ret = ctrl->cqes[idx].status >> 1;
		if (ret) {
			dn200_parse_fw_load_err(ctrl->dev, ret);
			break;
		}
	}

	dn200_free_dma_page_dir(priv, page, addr, DMA_TO_DEVICE);
	return ret;
}

static int dn200_ctrl_check_cq(struct dn200_ctrl_resource *ctrl,
	u32 interval, u32 times, struct ctrl_completion **cqe)
{
	struct ctrl_completion *hcqe;
	ktime_t timeout;
	int try = 0;
	int ret = -EBUSY;

	while (try++ < times) {
		timeout = ktime_add_us(ktime_get(), interval);
		while (ktime_compare(ktime_get(), timeout) < 0) {
			if (ctrl->is_extern_phy)
				usleep_range(1, 5);
			else
				udelay(1);
		}

		while (nvme_cqe_pending(ctrl)) {
			dma_rmb();
			hcqe = &ctrl->cqes[ctrl->cq_head];
			ctrl->rdata[ctrl->cq_head] = le64_to_cpu(hcqe->result.u64);
			if (cqe)
				*cqe = hcqe;
			ctrl_update_cq_head(ctrl);
		}
		if (ctrl->cq_head == ctrl->sq_tail) {
			writel(ctrl->cq_head, ctrl->dbs + 0x4);
			ret = 0;
			break;
		}
	}

	return ret;
}

static int dn200_nvme_fw_slot_get(struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_command c = {};
	int ret = -EBUSY;
	u32 cq_val = 0;
	struct plat_dn200_data *plat_ex;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state))
		return -EIO;

	memset(&c, 0, sizeof(c));
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = FW_SLOT_GET;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		return ret;
	}

	return cq_val;
}

static int dn200_nvme_fw_get_state(struct dn200_ctrl_resource *ctrl, u32 *cq_val)
{
	struct ctrl_command c = {};
	int ret = -EBUSY;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state))
		return -EIO;
again:
	memset(&c, 0, sizeof(c));
	c.rw_command.opcode = ctrl_admin_vendor_start;
	c.rw_command.set = FW_UP_STATUS;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, cq_val);
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	return ret;
}

static int dn200_nvme_fw_commit_polling(struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_command c = {};
	int ret = -EBUSY;
	u32 cq_val = 0;
	struct plat_dn200_data *plat_ex;
	int slot = 2;
	bool have_reset = false;
	unsigned long in_time;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	dn200_nvme_fw_get_state(ctrl, &cq_val);
	if ((cq_val >> 16) == DN200_FW_UPGRADE_COMIT_DOING) {
		dev_err(plat_ex->priv_back->device, "%s, %d,cq_val %d fw commit is not idle\n",
			__func__, __LINE__, cq_val);
		return -EIO;
	}
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state))
		return -EIO;
	/* update the backup slot */
	slot = dn200_nvme_fw_slot_get(ctrl);
	if (slot == 1 || slot == 2)
		slot = 3 - slot;
	else
		slot = 2;

	dev_info(plat_ex->priv_back->device, "commit fw to slot %d.\n", slot);
again:
	memset(&c, 0, sizeof(c));
	c.fw_commit_command.opcode = ctrl_admin_vendor_start;
	c.fw_commit_command.dword12 = FW_UP_COMMIT;
	/* update to slot 2, -s 2 -a 1 */
	c.fw_commit_command.dword13 = slot;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	/* when input invalid slot, output the slot val */
	if (c.fw_commit_command.dword13 == cq_val) {
		ret = -EINVAL;
		return ret;
	}
	if (cq_val == 0xff) {
		ret = -EOPNOTSUPP; /* fw not support the cmd, use old cmd */
		return ret;
	}
	if (ret < 0) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}
	if (ret)
		return ret;

	in_time = jiffies;
	/* check status of update */
	do {
		if (time_after(jiffies, in_time + msecs_to_jiffies(20000))) {
			dev_info(plat_ex->priv_back->device, "fw get state exceed time\n");
			break;
		}
		msleep(1000);
		ret = dn200_nvme_fw_get_state(ctrl, &cq_val);
	} while (!ret && (cq_val >> 16) == DN200_FW_UPGRADE_COMIT_DOING);

	if (!ret && (cq_val >> 16) == DN200_FW_UPGRADE_COMIT_FINISH)
		return cq_val & 0xffff;
	dev_err(plat_ex->priv_back->device, "%s, %d, dn200 can not get commit finish state\n",
			__func__, __LINE__);
	return ret;
}

int dn200_nvme_fw_commit(struct dn200_ctrl_resource *ctrl)
{
	struct ctrl_completion *hcqe;
	struct ctrl_command c = {};
	int ret = -EBUSY;
	int slot = 2;
	unsigned long flags = 0;
	char *buf;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state))
		return -EIO;

	if (!ctrl->is_extern_phy)
		return dn200_nvme_fw_commit_polling(ctrl);
	/* update the backup slot */
	slot = dn200_nvme_fw_slot_get(ctrl);
	if (slot == 1 || slot == 2)
		slot = 3 - slot;
	else
		slot = 2;

	if (slot == 1) {
		buf = vmalloc(CTRL_MAX_SEG_LEN);
		if (test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state)) {
			clear_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state);
			(void)dn200_nvme_fw_cmd_exec(ctrl, "rom_protect 0", buf, CTRL_MAX_SEG_LEN, NULL, 0);
			set_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state);
		} else {
			(void)dn200_nvme_fw_cmd_exec(ctrl, "rom_protect 0", buf, CTRL_MAX_SEG_LEN, NULL, 0);
		}
		vfree(buf);
	}
	dev_info(plat_ex->priv_back->device, "commit fw to slot %d.\n", slot);
again:
	if (ctrl->is_extern_phy)
		mutex_lock(&ctrl->mlock);
	else
		spin_lock_irqsave(&ctrl->lock, flags);
	memset(&c, 0, sizeof(c));
	c.fw_commit_command.opcode = ctrl_admin_activate_fw;
	/* The newly fw is activated at the next Controller Level Reset, -s 2 -a 1 */
	if (plat_ex->upgrade_with_flowing) {
		c.fw_commit_command.dword10 = (0x3 << 3) | slot;
		ctrl_submit_cmd(ctrl, &c);
		ret = 0;
		if (ctrl->is_extern_phy)
			mutex_unlock(&ctrl->mlock);
		else
			spin_unlock_irqrestore(&ctrl->lock, flags);
		return ret;
	}

	c.fw_commit_command.dword10 = (0x1 << 3) | slot;
	ctrl_submit_cmd(ctrl, &c);
	ret = dn200_ctrl_check_cq(ctrl, 1000, 30000, &hcqe);
	if (ctrl->is_extern_phy)
		mutex_unlock(&ctrl->mlock);
	else
		spin_unlock_irqrestore(&ctrl->lock, flags);
	if (ret) {
		dev_err(ctrl->dev, "[loading fw]fw commit cmd, get cq fail.\n");
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) &&
					!have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	} else {
		ret = hcqe->status >> 1;
	}
	return ret;
}

int dn200_fw_i2c_rw_commit(struct dn200_ctrl_resource *ctrl,
		 u32 dev_addr, u8 *value, u32 offset, bool rw)
{
	struct ctrl_command c = {};
	int ret = -EBUSY;
	int ret_val = 0;
	u32 cq_val = 0;
	int idx;
	struct plat_dn200_data *plat_ex;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
again:
	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;
	memset(&c, 0, sizeof(c));
	idx = ctrl->cq_head;
	c.fw_i2c_command.opcode = ctrl_admin_vendor_start;
	c.fw_i2c_command.set = FW_I2C_RW;
	c.fw_i2c_command.dword13 = dev_addr;
	c.fw_i2c_command.dword14 = rw;
	if (rw)
		c.fw_i2c_command.dword14 |= ((u32)(*value) << 16);
	c.fw_i2c_command.dword15 = offset;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (!rw)
		*value = cq_val & 0xff;
	ret_val = (cq_val >> 24) & 0xff;
	if (ret < 0) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n",
			__func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	}

	return ret < 0 ? ret : ret_val;
}

int dn200_nvme_product_info_get(struct dn200_ctrl_resource *ctrl, void *info, u32 len)
{
	dma_addr_t addr;
	struct page *page;
	void *buf;
	u32 cq_val = 0;
	struct ctrl_command c = {};
	struct plat_dn200_data *plat_ex;
	int ret = -EBUSY;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);

	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_FROM_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);

	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_prod_info_get;
	c.common_command.dptr.prp1 = addr;
	/* aligned to 4 */
	c.common_command.cdw10 = cpu_to_le32(len + 3) >> 2;

	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &cq_val);
	if (ret)
		dev_err(plat_ex->priv_back->device,
			 "%s, %d, poll cq fail\n", __func__, __LINE__);
	else
		memcpy(info, buf, len);

	dn200_free_dma_page_dir(plat_ex->priv_back,
			 page, addr, DMA_FROM_DEVICE);
	return ret;
}

static int dn200_nvme_fw_cmd_exec(struct dn200_ctrl_resource *ctrl,
	void *cmd, void *info, u32 len, u32 *ret_len, u32 time_out)
{
	dma_addr_t addr, addr1 = 0;
	dma_addr_t *prp_list = NULL;
	struct page *page = NULL;
	struct page *page1 = NULL;
	struct page **page_list = NULL;
	void *buf = NULL;
	void *buf1 = NULL;
	unsigned long flags = 0;
	struct ctrl_command c = {};
	struct plat_dn200_data *plat_ex;
	int idx;
	int ret = -EBUSY;
	u32 read_len;
	u32 entries;
	u32 i, j;
	u32 mem_len = 0;
	bool have_reset = false;

	/* cmd len limit 120 */
	if (strlen(cmd) >= MAX_FW_CMD_LEN)
		return -ENOMEM;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	/* send fw command */
	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_TO_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);
	strscpy(buf, cmd, MAX_FW_CMD_LEN);
again:
	if (ctrl->is_extern_phy)
		mutex_lock(&ctrl->mlock);
	else
		spin_lock_irqsave(&ctrl->lock, flags);
	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_fw_cmd_write;
	c.common_command.dptr.prp1 = addr;
	/* aligned to 4 */
	c.common_command.cdw10 = (MAX_FW_CMD_LEN + 3) >> 2;
	ctrl_submit_cmd(ctrl, &c);
	/* default wait time 10s */
	if (!time_out)
		time_out = 30000;
	ret = dn200_ctrl_check_cq(ctrl, 5, 200 * time_out, NULL);
	idx = (ctrl->cq_head + ctrl->q_depth - 1) % ctrl->q_depth;

	dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_TO_DEVICE);
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n", __func__, __LINE__);
		if (ctrl->is_extern_phy)
			mutex_unlock(&ctrl->mlock);
		else
			spin_unlock_irqrestore(&ctrl->lock, flags);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
		return ret;
	}

	if (ret_len)
		*ret_len = ctrl->rdata[idx];

	/* receive fw command log */
	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_FROM_DEVICE);
	if (!page) {
		if (ctrl->is_extern_phy)
			mutex_unlock(&ctrl->mlock);
		else
			spin_unlock_irqrestore(&ctrl->lock, flags);
		return -ENOMEM;
	}
	buf = page_address(page);

	if (ctrl->rdata[idx] > CTRL_MAX_SEG_LEN) {
		page1 = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr1, DMA_FROM_DEVICE);
		if (!page1) {
			dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_FROM_DEVICE);
			if (ctrl->is_extern_phy)
				mutex_unlock(&ctrl->mlock);
			else
				spin_unlock_irqrestore(&ctrl->lock, flags);
			return -ENOMEM;
		}
		buf1 = page_address(page1);
	}

	read_len = ctrl->rdata[idx] > len ? len : ctrl->rdata[idx];
	entries = ALIGN(read_len, CTRL_MAX_SEG_LEN) / CTRL_MAX_SEG_LEN;

	if (entries > 2) {
		prp_list = buf1;
		page_list = vmalloc(entries * sizeof(page_list));
		if (!page_list) {
			if (ctrl->is_extern_phy)
				mutex_unlock(&ctrl->mlock);
			else
				spin_unlock_irqrestore(&ctrl->lock, flags);
			ret = -ENOMEM;
			goto free_page;
		}
		for (i = 0; i < entries - 1; i++) {
			page_list[i] = dn200_alloc_dma_page_dir(plat_ex->priv_back,
					 &prp_list[i], DMA_FROM_DEVICE);
			if (!page_list[i]) {
				for (j = 0; j < i; j++)
					dn200_free_dma_page_dir(plat_ex->priv_back,
						 page_list[j], prp_list[j], DMA_FROM_DEVICE);

				if (ctrl->is_extern_phy)
					mutex_unlock(&ctrl->mlock);
				else
					spin_unlock_irqrestore(&ctrl->lock, flags);
				vfree(page_list);
				ret = -ENOMEM;
				goto free_page;
			}
		}
	}

	have_reset = false;
reagain:
	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_fw_cmd_read;
	c.common_command.dptr.prp1 = addr;
	c.common_command.dptr.prp2 = addr1;
	/* aligned to 4 */
	c.common_command.cdw10 = (read_len + 3) >> 2;
	ctrl_submit_cmd(ctrl, &c);
	/* max wait time 10s */
	ret = dn200_ctrl_check_cq(ctrl, 5, 2000000, NULL);
	if (ctrl->is_extern_phy)
		mutex_unlock(&ctrl->mlock);
	else
		spin_unlock_irqrestore(&ctrl->lock, flags);
	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n", __func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret) {
				if (ctrl->is_extern_phy)
					mutex_lock(&ctrl->mlock);
				else
					spin_lock_irqsave(&ctrl->lock, flags);
				goto reagain;
			}
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	} else {
		i = 0;
		if (entries > 2) {
			memcpy(info, buf, CTRL_MAX_SEG_LEN);
			mem_len = CTRL_MAX_SEG_LEN;
			while (mem_len < read_len) {
				if (mem_len + CTRL_MAX_SEG_LEN <= read_len) {
					memcpy(info + (i + 1) * CTRL_MAX_SEG_LEN,
						 page_address(page_list[i]), CTRL_MAX_SEG_LEN);
					mem_len += CTRL_MAX_SEG_LEN;
				} else {
					memcpy(info + (i + 1) * CTRL_MAX_SEG_LEN,
						 page_address(page_list[i]), read_len - mem_len);
					mem_len = read_len;
				}
				i++;
			}
		} else if (entries > 1) {
			memcpy(info, buf, CTRL_MAX_SEG_LEN);
			memcpy(info + CTRL_MAX_SEG_LEN, buf1, read_len - CTRL_MAX_SEG_LEN);
		} else {
			memcpy(info, buf, read_len);
		}
	}

	if (entries > 2) {
		for (i = 0; i < entries - 1; i++)
			dn200_free_dma_page_dir(plat_ex->priv_back,
				 page_list[i], prp_list[i], DMA_FROM_DEVICE);
		vfree(page_list);
	}
free_page:
	dn200_free_dma_page_dir(plat_ex->priv_back,
		 page, addr, DMA_FROM_DEVICE);
	if (ctrl->rdata[idx] > CTRL_MAX_SEG_LEN)
		dn200_free_dma_page_dir(plat_ex->priv_back,
			 page1, addr1, DMA_FROM_DEVICE);
	return ret;
}

int dn200_dev_temp_get(struct dn200_ctrl_resource *ctrl, void *info, u32 len)
{
	return dn200_nvme_fw_cmd_exec(ctrl, "core_temp", info, len, NULL, 0);
}

static int dn200_nvme_dev_open(struct inode *inode, struct file *file)
{
	struct dn200_ctrl_resource *ctrl =
		container_of(inode->i_cdev, struct dn200_ctrl_resource, cdev);

	file->private_data = ctrl;
	return 0;
}

static int dn200_nvme_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long dn200_nvme_dev_passthru(struct dn200_ctrl_resource *ctrl, unsigned long arg)
{
	struct ctrl_completion *hcqe;
	struct dn200_user_passth_command command;
	struct dn200_user_passth_command __user *ucmd = (void __user *)arg;
	struct ctrl_command c;
	void *buf = NULL;
	dma_addr_t addr;
	struct page *page = NULL;
	unsigned long flags = 0;
	unsigned int timeout = 0;
	int ret = -EBUSY;
	struct dn200_priv *priv;
	struct plat_dn200_data *plat_ex;
	enum dma_data_direction dma_dir = DMA_TO_DEVICE;
	bool have_reset = false;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	priv = plat_ex->priv_back;

	if (copy_from_user(&command, ucmd, sizeof(command)))
		return -EFAULT;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) ||
		 test_bit(ADMIN_UP_GRADE_FLAG, &ctrl->admin_state))
		return -EIO;

	memset(&c, 0, sizeof(c));
	c.common_command.opcode = command.opcode;
	c.common_command.flags = command.flags;
	c.common_command.cdw1 = cpu_to_le32(command.cdw1);
	c.common_command.cdw2 = cpu_to_le32(command.cdw2);
	c.common_command.cdw3 = cpu_to_le32(command.cdw3);
	c.common_command.cdw10 = cpu_to_le32(command.cdw10);
	c.common_command.cdw11 = cpu_to_le32(command.cdw11);
	c.common_command.cdw12 = cpu_to_le32(command.cdw12);
	c.common_command.cdw13 = cpu_to_le32(command.cdw13);
	c.common_command.cdw14 = cpu_to_le32(command.cdw14);
	c.common_command.cdw15 = cpu_to_le32(command.cdw15);

	if (command.data_len > CTRL_MAX_SEG_LEN)
		return -EFAULT;

	if (command.opcode == ctrl_admin_fw_cmd_read ||
		command.opcode == ctrl_admin_prod_info_get ||
		command.rsvd1) {
		dma_dir = DMA_FROM_DEVICE;
	}

	if (command.addr && command.data_len) {
		page = dn200_alloc_dma_page_dir(priv, &addr, dma_dir);
		if (!page)
			return -ENOMEM;
		buf = page_address(page);
		if (dma_dir == DMA_TO_DEVICE &&
			copy_from_user(buf, (void __user *)command.addr, command.data_len)) {
			dn200_free_dma_page_dir(priv, page, addr, dma_dir);
			dev_err(ctrl->dev, "[user command]copy from user fail, len %d.\n",
					 command.data_len);
			return -EFAULT;
		}

		c.common_command.dptr.prp1 = addr;
		if (!c.common_command.cdw10) {
			/* aligned to 4 */
			c.common_command.cdw10 = (command.data_len + 3) >> 2;
			/* fw download len is 0 base val */
			if (command.opcode == ctrl_admin_download_fw && c.common_command.cdw10)
				c.common_command.cdw10 -= 1;
		}
		c.common_command.cdw11 = (command.cdw11 + 3) >> 2;
	}

	timeout = command.timeout_ms;
again:
	if (ctrl->is_extern_phy)
		mutex_lock(&ctrl->mlock);
	else
		spin_lock_irqsave(&ctrl->lock, flags);
	ctrl_submit_cmd(ctrl, &c);

	ret = dn200_ctrl_check_cq(ctrl, 1000, timeout, &hcqe);
	if (ctrl->is_extern_phy)
		mutex_unlock(&ctrl->mlock);
	else
		spin_unlock_irqrestore(&ctrl->lock, flags);

	if (command.opcode == ctrl_admin_activate_fw)
		return 0;

	if (!ret && dma_dir == DMA_FROM_DEVICE &&
		copy_to_user((void __user *)command.addr, buf, command.data_len)) {
		dn200_free_dma_page_dir(priv, page, addr, dma_dir);
		dev_err(ctrl->dev, "[user command]copy to user fail, len %d.\n", command.data_len);
		return -EFAULT;
	}

	if (command.addr && command.data_len)
		dn200_free_dma_page_dir(priv, page, addr, dma_dir);

	if (ret) {
		dev_err(plat_ex->priv_back->device, "%s, %d, poll cq fail\n", __func__, __LINE__);
		if (!test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state) && !have_reset) {
			ret = ctrl_reset(ctrl, true);
			have_reset = true;
			if (!ret)
				goto again;
		}
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &ctrl->admin_state);
		if (plat_ex->priv_back)
			dn200_fw_err_dev_close(plat_ex->priv_back);
	} else {
		ret = hcqe->status >> 1;
		command.result = le64_to_cpu(hcqe->result.u64);
		if (put_user(command.result, &ucmd->result))
			return -EFAULT;
	}

	return ret;
}

static long dn200_card_info_get(struct dn200_ctrl_resource *ctrl, unsigned long arg)
{
	struct plat_dn200_data *plat_ex;
	struct net_device *ndev;
	struct ethtool_link_ksettings cmd;
	struct dn200_card_info info = {};

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	ndev = plat_ex->priv_back->dev;

	memcpy(info.eth_name, ndev->name, IFNAMSIZ);

	info.link = !!netif_carrier_ok(ndev);
	if (ndev->ethtool_ops->get_link_ksettings(ndev, &cmd))
		return -EINVAL;
	info.speed = cmd.base.speed;
	info.duplex = cmd.base.duplex;

	if (dn200_dev_temp_get(ctrl, info.eth_temp, sizeof(info.eth_temp)))
		sprintf(info.eth_temp, "Unknown!\n");

	return copy_to_user((void __user *)arg, &info, sizeof(struct dn200_card_info));
}

static long dn200_nvme_fw_cmd_exec_info_get(struct dn200_ctrl_resource *ctrl, unsigned long arg)
{
	struct dn200_user_passth_command command;
	struct dn200_user_passth_command __user *ucmd = (void __user *)arg;
	char *buf;
	char cmd[MAX_FW_CMD_LEN] = {};
	int ret;

	if (copy_from_user(&command, ucmd, sizeof(command)))
		return -EFAULT;

	if (command.metadata_len > MAX_FW_CMD_LEN)
		return -ENOMEM;

	if (copy_from_user(cmd, (void __user *)command.metadata, command.metadata_len)) {
		dev_err(ctrl->dev, "[fw cmd exec]copy from user fail, len %d.\n",
				 command.metadata_len);
		return -EFAULT;
	}

	buf = vmalloc(ALIGN(command.data_len, CTRL_MAX_SEG_LEN));
	if (!buf)
		return -ENOMEM;

	ret = dn200_nvme_fw_cmd_exec(ctrl, cmd, buf,
		 command.data_len, (u32 *)&command.result, command.timeout_ms);
	if (ret) {
		vfree(buf);
		return ret;
	}

	if (copy_to_user((void __user *)command.addr, buf, command.data_len)) {
		dev_err(ctrl->dev, "[fw cmd exec]copy to user fail, len %d.\n",
				 command.data_len);
		vfree(buf);
		return -EFAULT;
	}
	vfree(buf);
	if (put_user(command.result, &ucmd->result)) {
		dev_err(ctrl->dev, "[fw cmd exec]put user fail.\n");
		return -EFAULT;
	}
	return 0;
}

int dn200_configure_timestamp(struct dn200_ctrl_resource *ctrl)
{
	__le64 ts;
	struct ctrl_command c;
	struct page *page;
	void *buf;
	dma_addr_t addr;
	int ret, value;
	struct plat_dn200_data *plat_ex;

	ts = cpu_to_le64(ktime_to_ms(ktime_get_real()));
	memset(&c, 0, sizeof(c));
	c.features.opcode = dn200_admin_set_features;
	c.features.fid = DN200_FEAT_TIMESTAMP;
	c.features.dword11 = 0;
	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_TO_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);
	memcpy(buf, &ts, sizeof(ts));
	c.features.dptr.prp1 = addr;

	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &value);
	dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_TO_DEVICE);
	return ret;
}

int dn200_get_fw_ver(struct dn200_ctrl_resource *ctrl, struct dn200_ver *dn200_ver)
{
	struct ctrl_command c;
	struct page *page;
	void *buf;
	dma_addr_t addr;
	int ret, value;
	struct plat_dn200_data *plat_ex;

	memset(&c, 0, sizeof(c));
	c.ctrl_identify.opcode = ctrl_admin_fw_identify;
	c.ctrl_identify.cns = DN200_VER_CNS;
	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_FROM_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);
	c.ctrl_identify.dptr.prp1 = addr;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &value);
	/*process ver*/
	dn200_ver->type = *(u8 *)(buf + 64);
	dn200_ver->product_type = *(u8 *)(buf + 65);
	dn200_ver->rsv = *(u8 *)(buf + 66);
	dn200_ver->is_fw = *(u8 *)(buf + 67);
	dn200_ver->publish = *(u8 *)(buf + 68);
	dn200_ver->number0 = *(u8 *)(buf + 69);
	dn200_ver->number1 = *(u8 *)(buf + 70);
	dn200_ver->number2 = *(u8 *)(buf + 71);
	dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_FROM_DEVICE);
	return ret;
}

int dn200_eeprom_read(struct dn200_ctrl_resource *ctrl, u32 offset, u32 len, u8 *data)
{
	void *buf = NULL;
	dma_addr_t addr;
	struct page *page = NULL;
	struct plat_dn200_data *plat_ex;
	int ret;
	u32 val;
	struct ctrl_command c;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);

	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_eeprom_read;
	c.common_command.cdw12 = 0x01;
	c.common_command.cdw13 = offset;
	c.common_command.cdw10 = ALIGN(len, 4) >> 2;

	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_FROM_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);

	c.common_command.dptr.prp1 = addr;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &val);

	if (!ret)
		memcpy(data, buf, len);

	dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_FROM_DEVICE);

	return ret;
}

int dn200_eeprom_write(struct dn200_ctrl_resource *ctrl, u32 offset, u32 len, u8 *data)
{
	void *buf = NULL;
	dma_addr_t addr;
	struct page *page = NULL;
	struct plat_dn200_data *plat_ex;
	int ret;
	u32 val;
	struct ctrl_command c;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);

	memset(&c, 0, sizeof(c));
	c.common_command.opcode = ctrl_admin_eeprom_write;
	c.common_command.cdw12 = 0x02;
	c.common_command.cdw13 = offset;
	c.common_command.cdw10 = ALIGN(len, 4) >> 2;

	page = dn200_alloc_dma_page_dir(plat_ex->priv_back, &addr, DMA_TO_DEVICE);
	if (!page)
		return -ENOMEM;
	buf = page_address(page);

	memcpy(buf, data, len);

	c.common_command.dptr.prp1 = addr;
	ctrl->dn200_admin_process_ops->dn200_admin_process(ctrl, &c, &ret, &val);

	dn200_free_dma_page_dir(plat_ex->priv_back, page, addr, DMA_TO_DEVICE);

	return ret;
}

static long dn200_nvme_dev_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	long ret = 0;
	struct dn200_ctrl_resource *ctrl = file->private_data;

	switch (cmd) {
	case DN200_NVME_PASSTHRU:
		ret = dn200_nvme_dev_passthru(ctrl, arg);
		break;
	case DN200_NVME_GET_CARD_INFO:
		ret = dn200_card_info_get(ctrl, arg);
		break;
	case DN200_NVME_GET_FW_CMD_LOG:
		ret = dn200_nvme_fw_cmd_exec_info_get(ctrl, arg);
		break;
	default:
		ret = dn200_nvme_dev_passthru(ctrl, arg);
		break;
	}

	return ret;
}

static int dn200_nvme_mmap_bar4(struct file *file, struct vm_area_struct *vma)
{
	struct dn200_ctrl_resource *ctrl = file->private_data;
	struct plat_dn200_data *plat_ex;
	unsigned long base;
	unsigned long size;
	unsigned long offset;
	unsigned long pfn;
	unsigned long vsize;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	base = plat_ex->pdev->resource[4].start;
	size = resource_size(&plat_ex->pdev->resource[4]);

	offset = vma->vm_pgoff << PAGE_SHIFT;
	pfn = (base + offset) >> PAGE_SHIFT;
	vsize = vma->vm_end - vma->vm_start;

	if (vsize > size || offset >= size)
		return -EINVAL;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return remap_pfn_range(vma, vma->vm_start, pfn, vsize, vma->vm_page_prot);
}

static const struct file_operations dn200_nvme_ops = {
	.owner		= THIS_MODULE,
	.open		= dn200_nvme_dev_open,
	.mmap		= dn200_nvme_mmap_bar4,
	.release	= dn200_nvme_dev_release,
	.unlocked_ioctl	= dn200_nvme_dev_ioctl,
};

static u32 dn200_nvme_major;
static u32 dn200_nvme_num;
static struct class *nvme_class;
static unsigned long mask[BITS_TO_LONGS(MAX_NVME_NUM)];

void dn200_register_nvme_device(struct dn200_ctrl_resource *ctrl)
{
	char dev_name[64] = {0};
	struct plat_dn200_data *plat_ex;
	dev_t devt;
	unsigned long i;

	plat_ex = container_of(ctrl, struct plat_dn200_data, ctrl);
	if (snprintf(dev_name, sizeof(dev_name), "dn200_nvme%x-%x-%x",
		 pci_domain_nr(plat_ex->pdev->bus), plat_ex->pdev->bus->number, plat_ex->funcid) < 0) {
		dev_err(plat_ex->priv_back->device, "nvme device name is too long\n");
		return;
	}
	if (!dn200_nvme_major) {
		if (alloc_chrdev_region(&devt, 0, MAX_NVME_NUM, dev_name) < 0) {
			dev_err(plat_ex->priv_back->device, "alloc chrdev region fail\n");
			return;
		}
		dn200_nvme_major = MAJOR(devt);

		nvme_class = class_create("dn200_nvme");
		if (IS_ERR(nvme_class)) {
			dev_err(plat_ex->priv_back->device, "class creat fail\n");
			goto err_chrdev_unreg;
		}
	}

	for_each_clear_bit(i, mask, MAX_NVME_NUM) {
		set_bit(i, mask);
		ctrl->devt = MKDEV(dn200_nvme_major, i);
		break;
	}

	dn200_nvme_num++;

	cdev_init(&ctrl->cdev, &dn200_nvme_ops);

	if (cdev_add(&ctrl->cdev, ctrl->devt, 1)) {
		dev_err(plat_ex->priv_back->device, "cdev add fail\n");
		goto err_class_release;
	}

	if (IS_ERR(device_create(nvme_class, NULL, ctrl->devt, NULL, dev_name))) {
		dev_err(plat_ex->priv_back->device, "device create fail\n");
		goto err_dev_del;
	}
	return;
err_dev_del:
	cdev_del(&ctrl->cdev);
err_class_release:
	class_destroy(nvme_class);
err_chrdev_unreg:
	unregister_chrdev_region(ctrl->devt, MAX_NVME_NUM);
}

void dn200_unregister_nvme_device(struct dn200_ctrl_resource *ctrl)
{
	if (nvme_class)
		device_destroy(nvme_class, ctrl->devt);
	cdev_del(&ctrl->cdev);
	clear_bit(MINOR(ctrl->devt), mask);
	dn200_nvme_num--;
	if (nvme_class && !dn200_nvme_num) {
		dn200_nvme_major = 0;
		class_destroy(nvme_class);
		unregister_chrdev_region(ctrl->devt, MAX_NVME_NUM);
	}
}
