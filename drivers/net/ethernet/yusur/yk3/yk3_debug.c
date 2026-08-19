// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_emp_priv.h"

static struct dentry *yk3_debugfs_root;
/******************************************************************************/
static int yk3_card_info_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_card *card = seq->private;
	struct yk3_pdev_priv *pdev_priv;
	int i, cpu;
	u8 tmp[32];

	seq_printf(seq, "\t    %-16s : %-16s\n", "name", card->name);
	seq_printf(seq, "\t    %-16s : %-16d\n", "id", card->id);
	seq_printf(seq, "\t    %-16s : %-16d\n", "chip", card->chip);
	seq_printf(seq, "\t    %-16s : %-20s\n", "sn", card->sn.num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "mode", card->mode);
	seq_printf(seq, "\t    %-16s : 0x%-16llx\n", "hw_bits", card->hw_bits);
	seq_printf(seq, "\t    %-16s : 0x%-16llx\n", "cap_bits", card->cap_bits);
	seq_printf(seq, "\t    %-16s : %-16d\n", "numa", card->numa);
	seq_printf(seq, "\t    %-16s : %-16d\n", "qnum", card->qnum);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf_num", card->pf_num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "port_type", card->port_type);
	seq_printf(seq, "\t    %-16s : %-16d\n", "edma_clk", card->edma_clk);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf_ndev_qnum", card->pf_ndev_qnum);
	seq_printf(seq, "\t    %-16s : %-16d\n", "vf_ndev_qnum", card->vf_ndev_qnum);
	seq_printf(seq, "\t    %-16s : %-16d\n", "coalesce_max_usecs", card->coalesce_max_usecs);
	seq_printf(seq, "\t    %-16s : %-16d\n", "coalesce_max_frames", card->coalesce_max_frames);
	seq_printf(seq, "\t    %-16s : %-16d\n", "qsetid_base", card->qsetid_base);
	seq_printf(seq, "\t    %-16s : %-16d\n", "qsetid_num", card->qsetid_num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "mac_ch_num", card->mac_ch_num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "affinity_seqnum", card->affinity_seqnum);
	for (i = 0; i < card->mac_ch_num; i++)
		seq_printf(seq, "\t    %-14s-%d : %-16d\n", "mac_chs", i, card->mac_chs[i]);
	seq_printf(seq, "\t    %-16s : ", "local_cpus");
	for_each_cpu(cpu, card->local_cpumask) {
		seq_printf(seq, "%-4d ", cpu);
	}
	seq_printf(seq, "\n\t    %-16s : ", "neigh_cpus");
	for_each_cpu(cpu, card->neigh_cpumask) {
		seq_printf(seq, "%-4d ", cpu);
	}
	seq_printf(seq, "\n\t    %-16s : ", "remote_cpus");
	for_each_cpu(cpu, card->remote_cpumask) {
		seq_printf(seq, "%-4d ", cpu);
	}
	seq_puts(seq, "\n");
	seq_printf(seq, "\t    %-16s : %-16d\n", "link_speed_cfg", card->link_speed_cfg);
	seq_printf(seq, "\t    %-16s : %-16d\n", "link_speed_cfg_autoneg",
		   card->link_speed_cfg_autoneg);

	mutex_lock(&card->pdev_priv_head_mlock);
	list_for_each_entry(pdev_priv, &card->pdev_priv_head, card_node) {
		seq_printf(seq, "\t    %-16s : %04x:%02x:%02x.%d\n", "pci",
			   pci_domain_nr(pdev_priv->pdev->bus),
			   pdev_priv->pdev->bus->number,
			   PCI_SLOT(pdev_priv->pdev->devfn),
			   PCI_FUNC(pdev_priv->pdev->devfn));
		if (yk3_pdev_is_mgr(pdev_priv)) {
			seq_printf(seq, "\t    %-16s : %-16u\n", "magic_id",
				   card->emp_info->vpd.magic_id);
			seq_printf(seq, "\t    %-16s : %-16u\n", "ver",
				   card->emp_info->vpd.ver);
			seq_printf(seq, "\t    %-16s : %-16u\n", "cksum",
				   card->emp_info->vpd.cksum);
			seq_printf(seq, "\t    %-16s : %-16u\n", "flag",
				   card->emp_info->vpd.flag);
			seq_printf(seq, "\t    %-16s : %02u.%02u.%02u.%02u\n",
				   "img_version",
				   ((u8 *)&card->emp_info->vpd.img_version)[0],
				   ((u8 *)&card->emp_info->vpd.img_version)[1],
				   ((u8 *)&card->emp_info->vpd.img_version)[2],
				   ((u8 *)&card->emp_info->vpd.img_version)[3]);
			seq_printf(seq, "\t    %-16s : %02u.%02u.%02u.%02u\n",
				   "fw_version",
				   card->emp_info->vpd.fw_version[0],
				   card->emp_info->vpd.fw_version[1],
				   card->emp_info->vpd.fw_version[2],
				   card->emp_info->vpd.fw_version[3]);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n",
				   "np_version",
				   card->emp_info->vpd.np_version);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n",
				   "ppp0_version",
				   card->emp_info->vpd.ppp0_version);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n",
				   "ppp1_version",
				   card->emp_info->vpd.ppp1_version);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n",
				   "ppp2_version",
				   card->emp_info->vpd.ppp2_version);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n",
				   "ppp3_version",
				   card->emp_info->vpd.ppp3_version);
			seq_printf(seq, "\t    %-16s : %02u.%02u.%02u.%02u\n",
				   "pxe_version",
				   ((u8 *)&card->emp_info->vpd.pxe_version)[0],
				   ((u8 *)&card->emp_info->vpd.pxe_version)[1],
				   ((u8 *)&card->emp_info->vpd.pxe_version)[2],
				   ((u8 *)&card->emp_info->vpd.pxe_version)[3]);
			seq_printf(seq, "\t    %-16s : %-16u\n", "chip_type",
				   card->emp_info->vpd.chip_type);
			seq_printf(seq, "\t    %-16s : %-16u\n", "board_type",
				   card->emp_info->vpd.board_type);
			seq_printf(seq, "\t    %-16s : %-16u\n", "nic_type",
				   card->emp_info->vpd.nic_type);
			seq_printf(seq, "\t    %-16s : 0x%-16x\n", "porttype",
				   card->emp_info->vpd.porttype);
			seq_printf(seq, "\t    %-16s : %-16u\n", "runmode",
				   card->emp_info->vpd.runmode);
			seq_printf(seq, "\t    %-16s : %-16u\n", "pxe_enable",
				   card->emp_info->vpd.pxe_enable);
			seq_printf(seq, "\t    %-16s : %-16u\n", "pxe_type",
				   card->emp_info->vpd.pxe_type);
			seq_printf(seq, "\t    %-16s : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				   "mac_addr",
				   card->emp_info->vpd.mac_addr[0],
				   card->emp_info->vpd.mac_addr[1],
				   card->emp_info->vpd.mac_addr[2],
				   card->emp_info->vpd.mac_addr[3],
				   card->emp_info->vpd.mac_addr[4],
				   card->emp_info->vpd.mac_addr[5],
				   card->emp_info->vpd.mac_addr[6],
				   card->emp_info->vpd.mac_addr[7]);
			snprintf(tmp, SERIAL_NUM_LEN_MAX, "%s", card->emp_info->vpd.sn);
			seq_printf(seq, "\t    %-16s : %-16s\n", "sn", tmp);
			snprintf(tmp, PRODUCT_NUM_LEN_MAX, "%s", card->emp_info->vpd.pn);
			seq_printf(seq, "\t    %-16s : %-16s\n", "pn", tmp);
			seq_printf(seq, "\t    %-16s : %-16u\n", "board_id",
				   card->emp_info->vpd.board_id);
			seq_printf(seq, "\t    %-16s : %-16u\n", "pcb_ver",
				   card->emp_info->vpd.pcb_ver);
			seq_printf(seq, "\t    %-16s : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				   "default_mac_addr",
				   card->emp_info->vpd.default_mac_addr[0],
				   card->emp_info->vpd.default_mac_addr[1],
				   card->emp_info->vpd.default_mac_addr[2],
				   card->emp_info->vpd.default_mac_addr[3],
				   card->emp_info->vpd.default_mac_addr[4],
				   card->emp_info->vpd.default_mac_addr[5],
				   card->emp_info->vpd.default_mac_addr[6],
				   card->emp_info->vpd.default_mac_addr[7]);
			snprintf(tmp, PRODUCT_NAME_LEN_MAX, "%s", card->emp_info->vpd.product_name);
			seq_printf(seq, "\t    %-16s : %-16s\n", "product_name", tmp);
			snprintf(tmp, VENDOR_NAME_LEN_MAX, "%s", card->emp_info->vpd.vendor_name);
			seq_printf(seq, "\t    %-16s : %-16s\n", "vendor_name", tmp);
			snprintf(tmp, MODEL_NAME_LEN_MAX, "%s", card->emp_info->vpd.model_name);
			seq_printf(seq, "\t    %-16s : %-16s\n", "model_name", tmp);
		}
	}
	mutex_unlock(&card->pdev_priv_head_mlock);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_card_info_debugfs);

int yk3_debug_card_init(struct yk3_card *card)
{
	if (!yk3_debugfs_root)
		return -ENOENT;

	card->dbgfs_dir = debugfs_create_dir(card->name, yk3_debugfs_root);
	if (!card->dbgfs_dir) {
		yk3_err("Failed to create debugfs directory for card %s", card->name);
		return -ENOMEM;
	}

	card->dbgfs_info_file = debugfs_create_file("info", 0444, card->dbgfs_dir,
						    card, &yk3_card_info_debugfs_fops);
	if (!card->dbgfs_info_file) {
		yk3_err("Failed to create debugfs info file for card %s", card->name);
		debugfs_remove(card->dbgfs_dir);
		card->dbgfs_dir = NULL;
		return -ENOMEM;
	}

	debugfs_create_u32("linkup_debug", 0644, card->dbgfs_dir, &card->linkup_dbg);

	return 0;
}

void yk3_debug_card_exit(struct yk3_card *card)
{
	debugfs_remove_recursive(card->dbgfs_dir);
}

static int yk3_pdev_info_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_rdma_data *rdma_data;
	struct yk3_mac *mac;

	seq_printf(seq, "\t    %-16s : %-16s\n", "name", pdev_priv->name);
	seq_printf(seq, "\t    %-16s : 0x%-16x\n", "vendor", pdev_priv->vendor);
	seq_printf(seq, "\t    %-16s : 0x%-16x\n", "device", pdev_priv->device);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf_id", pdev_priv->pf_id);
	seq_printf(seq, "\t    %-16s : %-16d\n", "vf_id", pdev_priv->vf_id);

	mutex_lock(&pdev_priv->ndev_priv_head_mlock);
	list_for_each_entry(ndev_priv, &pdev_priv->ndev_priv_head, pdev_node) {
		seq_printf(seq, "\t    %-16s : %s\n", "ndev", ndev_priv->ndev->name);
	}
	mutex_unlock(&pdev_priv->ndev_priv_head_mlock);

	if (pdev_priv->rdma_data) {
		rdma_data = pdev_priv->rdma_data;
		seq_printf(seq, "\t    %-16s : %-16d\n", "version_major",
			   rdma_data->version_major);
		seq_printf(seq, "\t    %-16s : %-16d\n", "version_minor",
			   rdma_data->version_minor);
		seq_printf(seq, "\t    %-16s : %-16d\n", "vector_start",
			   rdma_data->vector_start);
		seq_printf(seq, "\t    %-16s : %-16d\n", "vector_num",
			   rdma_data->vector_num);
	}

	if (pdev_priv->mac) {
		mac = pdev_priv->mac;
		seq_printf(seq, "\t    %-16s : %-16u\n", "mac_ch", mac->mac_ch);
		seq_printf(seq, "\t    %-16s : %-16u\n", "irq_vector", mac->irq_vector);
		seq_printf(seq, "\t    %-16s : %-16u\n", "sts_regval", mac->sts_regval);
		seq_printf(seq, "\t    %-16s : %-16d\n", "speed", (int)mac->speed);
		seq_printf(seq, "\t    %-16s : %-16u\n", "fec_cfg", mac->fec_cfg);
		seq_printf(seq, "\t    %-16s : 0x%-16x\n", "port", mac->port);
		seq_printf(seq, "\t    %-16s : %-16u\n", "phy_stat_err", mac->phy_stat_err);
		seq_printf(seq, "\t    %-16s : %-16u\n", "link_int_cnt", mac->link_int_cnt);
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_pdev_info_debugfs);

static ssize_t yk3_temperature_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct yk3_pdev_priv *pdev_priv = file->private_data;
	void __iomem *hw_addr;
	char buf_val[16];
	int len;
	u32 temp;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	temp = yk3_rd32(hw_addr, YK3_RUNTIME_INFO_ADDR);
	pdev_priv->temperature = temp / 100 - 100;
	len = snprintf(buf_val, sizeof(buf_val), "%u\n", pdev_priv->temperature);
	return simple_read_from_buffer(buf, count, ppos, buf_val, len);
}

static const struct file_operations yk3_temperature_fops = {
	.read    = yk3_temperature_read,
	.open    = simple_open,
	.llseek  = generic_file_llseek,
};

int yk3_debug_pdev_init(struct yk3_pdev_priv *pdev_priv)
{
	struct dentry *entry;

	if (!yk3_debugfs_root)
		return -ENOENT;

	pdev_priv->dbgfs_dir = debugfs_create_dir(pdev_priv->name, yk3_debugfs_root);
	if (!pdev_priv->dbgfs_dir) {
		yk3_err("Failed to create debugfs directory for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}

	entry = debugfs_create_file("info", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_pdev_info_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs info file for pdev %s", pdev_priv->name);
		debugfs_remove(pdev_priv->dbgfs_dir);
		pdev_priv->dbgfs_dir = NULL;
		return -ENOMEM;
	}
	pdev_priv->dbgfs_info_file = entry;

	if (yk3_pdev_is_mgr(pdev_priv))
		debugfs_create_file("temperature", 0644, pdev_priv->dbgfs_dir,
				    pdev_priv, &yk3_temperature_fops);

	return 0;
}

void yk3_debug_pdev_exit(struct yk3_pdev_priv *pdev_priv)
{
	debugfs_remove_recursive(pdev_priv->dbgfs_dir);
}

static int yk3_ndev_info_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_ndev_priv *ndev_priv = seq->private;
	struct yk3_queuebase qbase;
	int i;

	seq_printf(seq, "\t    %-16s : %-16s\n", "name", ndev_priv->ndev->name);
	seq_printf(seq, "\t    %-16s : %-16d\n", "qsetid", ndev_priv->qsetid);
	seq_printf(seq, "\t    %-16s : %-16d\n", "type", ndev_priv->type);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf_id", ndev_priv->pf_id);
	seq_printf(seq, "\t    %-16s : %-16d\n", "vf_id", ndev_priv->vf_id);
	qbase = ndev_priv->qbase[YK3_QUEUE_T_LOCAL];
	seq_printf(seq, "\t    %-16s : %-16d\n", "local qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "local qnum", qbase.num);
	qbase = ndev_priv->qbase[YK3_QUEUE_T_FUNC];
	seq_printf(seq, "\t    %-16s : %-16d\n", "func qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "func qnum", qbase.num);
	qbase = ndev_priv->qbase[YK3_QUEUE_T_PF];
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf qnum", qbase.num);
	qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
	seq_printf(seq, "\t    %-16s : %-16d\n", "global qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "glocal qnum", qbase.num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "txq_depth", ndev_priv->txq_depth);
	seq_printf(seq, "\t    %-16s : %-16d\n", "rxq_depth", ndev_priv->rxq_depth);
	seq_printf(seq, "\t    %-16s : %-16d\n", "txq_real_num", ndev_priv->txq_real_num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "rxq_real_num", ndev_priv->rxq_real_num);

	seq_printf(seq, "\t    %-16s : %-16d\n", "umd_enable", ndev_priv->umd_enable);
	seq_printf(seq, "\t    %-16s : %-16d\n", "itr_rx_enable", ndev_priv->itr_rx_enable);
	seq_printf(seq, "\t    %-16s : %-16d\n", "itr_tx_enable", ndev_priv->itr_tx_enable);
	seq_printf(seq, "\t    %-16s : %-16d\n", "rx_coalesce_usecs",
		   ndev_priv->rx_coalesce_usecs);
	seq_printf(seq, "\t    %-16s : %-16d\n", "rx_max_coalesced_frames",
		   ndev_priv->rx_max_coalesced_frames);
	seq_printf(seq, "\t    %-16s : %-16d\n", "tx_coalesce_usecs",
		   ndev_priv->tx_coalesce_usecs);
	seq_printf(seq, "\t    %-16s : %-16d\n", "tx_max_coalesced_frames",
		   ndev_priv->tx_max_coalesced_frames);
	seq_printf(seq, "\t    %-16s : %-16x\n", "rxfh_cfg", ndev_priv->rxfh_cfg);
	seq_printf(seq, "\t    %-16s : %-16x\n", "netdev_flags", ndev_priv->netdev_flags);
	seq_printf(seq, "\t    %-16s : %-16x\n", "ethtool_priv_flags",
		   ndev_priv->ethtool_priv_flags);
	seq_printf(seq, "\t    %-16s : %-16d\n", "mdrop_rx_rate", ndev_priv->mdrop_rx_rate);
	seq_printf(seq, "\t    %-16s : %-16d\n", "mdrop_tx_rate", ndev_priv->hqos_tx_rate);
	seq_printf(seq, "\t    %-16s : %-16d\n", "link_speed_cfg", (int)ndev_priv->link_speed_cfg);
	seq_printf(seq, "\t    %-16s : %-16u\n", "link_speed_cfg_autoneg",
		   ndev_priv->link_speed_cfg_autoneg);
	seq_printf(seq, "\t    %-16s : %-16d\n", "link_speed", (int)ndev_priv->link_speed);
	seq_printf(seq, "\t    %-16s : %-16u\n", "link_fec_cfg", ndev_priv->link_fec_cfg);
	seq_printf(seq, "\t    %-16s : %-16u\n", "link_status", ndev_priv->link_status);
	seq_printf(seq, "\t    %-16s : %-16u\n", "link_duplex", ndev_priv->link_duplex);
	seq_printf(seq, "\t    %-16s : 0x%-16x\n", "port", ndev_priv->port);

	if (yk3_ndev_is_pf(ndev_priv)) {
		seq_printf(seq, "\t    %-16s : %-16u\n", "ena_tc", ndev_priv->tc_cfg.ena_tc);
		seq_printf(seq, "\t    %-16s : %-16lu\n", "tc_flags", *ndev_priv->tc_cfg.flags);
		seq_printf(seq, "\t    %-16s : %-16u\n", "willing", ndev_priv->etscfg->willing);
		for (i = 0; i < YK3_MAX_TRAFFIC_CLASS; i++) {
			seq_printf(seq, "\t    tc %d -> prio %d, tcbw %3d, tsa %d, maxrate %llu\n",
				   i, ndev_priv->etscfg->prio_table[i],
				   ndev_priv->etscfg->tcbwtable[i],
				   ndev_priv->etscfg->tsatable[i],
				   ndev_priv->etscfg->max_rate[i]);
		}
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_ndev_info_debugfs);

int yk3_debug_ndev_init(struct yk3_ndev_priv *ndev_priv)
{
	struct dentry *entry;

	if (!yk3_debugfs_root)
		return -ENOENT;

	ndev_priv->dbgfs_dir = debugfs_create_dir(ndev_priv->name, yk3_debugfs_root);
	if (!ndev_priv->dbgfs_dir) {
		yk3_net_err("Failed to create debugfs directory for ndev %s", ndev_priv->name);
		return -ENOMEM;
	}

	entry = debugfs_create_file("info", 0444, ndev_priv->dbgfs_dir, ndev_priv,
				    &yk3_ndev_info_debugfs_fops);
	if (!entry) {
		yk3_net_err("Failed to create debugfs info file for ndev %s", ndev_priv->name);
		debugfs_remove(ndev_priv->dbgfs_dir);
		ndev_priv->dbgfs_dir = NULL;
		return -ENOMEM;
	}
	ndev_priv->dbgfs_info_file = entry;

	return 0;
}

void yk3_debug_ndev_exit(struct yk3_ndev_priv *ndev_priv)
{
	debugfs_remove_recursive(ndev_priv->dbgfs_dir);
	ndev_priv->dbgfs_dir = NULL;
	ndev_priv->dbgfs_info_file = NULL;
}

static int yk3_sriov_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_sriov_priv *sriov_priv = yk3_sriov_get_priv(pdev_priv);
	struct yk3_vf_info *vf_info;
	int i;

	if (!sriov_priv) {
		seq_printf(seq, "\t    %-16s : sriov info operation pending\n", pdev_priv->name);
		return 0;
	}

	seq_printf(seq, "\t    %-16s : %-16d\n", "num_vfs", sriov_priv->num_vfs);

	for (i = 0; i < sriov_priv->num_vfs; i++) {
		vf_info = &sriov_priv->vf_info[i];
		seq_printf(seq, "\t    %-16s : %-16d\n", "vf_idx", vf_info->vf_idx);
		seq_printf(seq, "\t    %-16s : %2x:%2x:%2x:%2x:%2x:%2x\n", "mac_addr",
			   vf_info->mac_addr[0], vf_info->mac_addr[1], vf_info->mac_addr[2],
			   vf_info->mac_addr[3], vf_info->mac_addr[4], vf_info->mac_addr[5]);
		seq_printf(seq, "\t    %-16s : %-16d\n", "qsetid", vf_info->qsetid);
		seq_printf(seq, "\t    %-16s : %-16d\n", "p_qbase start", vf_info->p_qbase.start);
		seq_printf(seq, "\t    %-16s : %-16d\n", "p_qbase num", vf_info->p_qbase.num);
		seq_printf(seq, "\t    %-16s : %-16d\n", "vf_vlan_tpid", vf_info->vf_vlan_tpid);
		seq_printf(seq, "\t    %-16s : %-16d\n", "vf_vlan", vf_info->vf_vlan);
		seq_printf(seq, "\t    %-16s : %-16d\n", "vf_vlan_qos", vf_info->vf_vlan_qos);
		seq_printf(seq, "\t    %-16s : %-16d\n", "spoofchk", vf_info->spoofchk);
		seq_printf(seq, "\t    %-16s : %-16d\n", "trusted", vf_info->trusted);
		seq_printf(seq, "\t    %-16s : %-16d\n", "ndev_flags", vf_info->ndev_flags);
		seq_printf(seq, "\t    %-16s : %-16d\n", "umd_enable", vf_info->umd_enable);
	}
	yk3_sriov_put_priv(sriov_priv);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_sriov_debugfs);

int yk3_debug_sriov_init(struct yk3_pdev_priv *pdev_priv)
{
	struct dentry *entry;

	if (!yk3_pdev_is_pf(pdev_priv))
		return 0;

	if (!pdev_priv->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("sriov", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_sriov_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs sriov file for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	pdev_priv->dbgfs_sriov_file = entry;

	return 0;
}

void yk3_debug_sriov_exit(struct yk3_pdev_priv *pdev_priv)
{
	debugfs_remove(pdev_priv->dbgfs_sriov_file);
	pdev_priv->dbgfs_sriov_file = NULL;
}

void yk3_debug_init(void)
{
	yk3_debugfs_root = debugfs_create_dir("yk3", NULL);
	if (IS_ERR(yk3_debugfs_root))
		yk3_err("Failed to create debugfs root directory");
}

void yk3_debug_exit(void)
{
	debugfs_remove_recursive(yk3_debugfs_root);
}
