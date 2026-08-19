// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_emp_priv.h"

struct yk3_card_info {
	enum yk3_chip chip;
	struct yk3_card_sn sn;		/* card serial number */
	u16 qnum;
	u32 edma_clk;
	struct yk3_emp_info emp_info;
};

struct yk3_card_mgr {
	atomic_t card_cnt;
	struct idr idr;
	struct mutex mlock;	/* card manager lock */
	struct yk3_pdev_priv *pdev_privs[9][512];
};

static struct yk3_card_mgr card_mgr = { .card_cnt = ATOMIC_INIT(0) };

static ushort pf_qnum = 16;
module_param_named(pf_qnum, pf_qnum, ushort, 0444);
MODULE_PARM_DESC(pf_qnum, "pf netdev queue number (default 16) : [1, 64]");

static ushort vf_qnum = 8;
module_param_named(vf_qnum, vf_qnum, ushort, 0444);
MODULE_PARM_DESC(vf_qnum, "vf netdev max queue number (default 8) : [1, 64]");

static char mode[32] = "";
module_param_string(mode, mode, sizeof(mode), 0444);
MODULE_PARM_DESC(mode, "mode=[t|r]");

static ushort pf_num;
module_param_named(pf_num, pf_num, ushort, 0444);
MODULE_PARM_DESC(pf_num, "pf number on the card (default 2) : [1, 4]");

struct yk3_pdev_priv *yk3_get_pdev_priv(struct yk3_pci_addr *addr)
{
	int i;
	struct yk3_card *card;
	struct yk3_pdev_priv *pdev_priv, *find = NULL;

	mutex_lock(&card_mgr.mlock);
	idr_for_each_entry(&card_mgr.idr, card, i) {
		mutex_lock(&card->pdev_priv_head_mlock);
		list_for_each_entry(pdev_priv, &card->pdev_priv_head, card_node) {
			if (pci_domain_nr(pdev_priv->pdev->bus) == addr->domain &&
			    pdev_priv->pdev->bus->number == addr->bus &&
			    PCI_SLOT(pdev_priv->pdev->devfn) == addr->devid &&
			    PCI_FUNC(pdev_priv->pdev->devfn) == addr->function) {
				find = pdev_priv;
				break;
			}
		}
		mutex_unlock(&card->pdev_priv_head_mlock);
		if (find)
			break;
	}
	mutex_unlock(&card_mgr.mlock);

	return find;
}

struct yk3_ndev_priv *yk3_get_ndev_priv(u32 ifindex)
{
	int i;
	struct yk3_card *card;
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_ndev_priv *ndev_priv, *find = NULL;

	mutex_lock(&card_mgr.mlock);
	idr_for_each_entry(&card_mgr.idr, card, i) {
		mutex_lock(&card->pdev_priv_head_mlock);
		list_for_each_entry(pdev_priv, &card->pdev_priv_head, card_node) {
			mutex_lock(&pdev_priv->ndev_priv_head_mlock);
			list_for_each_entry(ndev_priv, &pdev_priv->ndev_priv_head, pdev_node) {
				if (ndev_priv->ndev->ifindex == ifindex) {
					find = ndev_priv;
					break;
				}
			}
			mutex_unlock(&pdev_priv->ndev_priv_head_mlock);
			if (find)
				break;
		}
		mutex_unlock(&card->pdev_priv_head_mlock);
		if (find)
			break;
	}
	mutex_unlock(&card_mgr.mlock);

	return find;
}

static struct yk3_card *yk3_card_find(struct yk3_card_sn *sn)
{
	int i;
	struct yk3_card *card, *find = NULL;

	mutex_lock(&card_mgr.mlock);
	idr_for_each_entry(&card_mgr.idr, card, i) {
		if (!memcmp(sn->num, card->sn.num, sizeof(card->sn.num))) {
			find = card;
			break;
		}
	}
	mutex_unlock(&card_mgr.mlock);

	return find;
}

static void yk3_mbox_get_emp_info(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_emp_info *emp_info = pdev_priv->card->emp_info;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	u8 len, offset = msg->data[0];

	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;
	if (!offset)
		len = YK3_MBOX_DATA_LEN;
	else
		len = sizeof(*emp_info) - offset;
	memcpy((void *)ack_msg.data,
	       (const void *)emp_info + offset,
	       (size_t)len);
	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
}

static inline void yk3_emp_info_memcpy(void *buffer, void __iomem *addr, size_t size)
{
	u32 i;

	for (i = 0; i < size / sizeof(u32); i++)
		*((u32 *)buffer + i) = yk3_rd32(addr, i * sizeof(u32));
}

static const char * const yk3_temp_alarm_str[] = {
	"Chip Over-Temperature level 1 alarming!\n",
	"Chip Over-Temperature level 2 alarming!\n",
	"Chip Low-Temperature level 1 alarming!\n",
	"Chip Low-Temperature level 2 alarming!\n",
	"PCIE Over-Temperature level 1 alarming!\n",
	"PCIE Over-Temperature level 2 alarming!\n",
	"PCIE Low-Temperature level 1 alarming!\n",
	"PCIE Low-Temperature level 2 alarming!\n",
	"NP Over-Temperature level 1 alarming!\n",
	"NP Over-Temperature level 2 alarming!\n",
	"NP Low-Temperature level 1 alarming!\n",
	"NP Low-Temperature level 2 alarming!\n",
};

static void yk3_mbox_emp_temp_alarm(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u32 alarm_code = (u32)msg->data[0];
	struct yk3_card *card = pdev_priv->card;

	if (alarm_code < YK3_TEMP_CODE_START || alarm_code > YK3_TEMP_CODE_VALID_END) {
		yk3_dev_err("Unknown type Temperature alarming!\n");
		return;
	}

	yk3_dev_err("%s", yk3_temp_alarm_str[alarm_code - YK3_TEMP_CODE_START]);

	if (alarm_code & 0x1) {
		card->emp_temp_l2_alarm++;
		yk3_dev_err("Alarming l2 temperature, URGENT, device will stop!");
		card->emp_close_hw = true;
	}
}

static const char * const yk3_vol_alarm_str[] = {
	"Chip Over-Voltage level 1 alarming!\n",
	"Chip Over-Voltage level 2 alarming!\n",
	"Chip Low-Voltage level 1 alarming!\n",
	"Chip Low-Voltage level 2 alarming!\n",
	"PCIE Over-Voltage level 1 alarming!\n",
	"PCIE Over-Voltage level 2 alarming!\n",
	"PCIE Low-Voltage level 1 alarming!\n",
	"PCIE Low-Voltage level 2 alarming!\n",
	"NP Over-Voltage level 1 alarming!\n",
	"NP Over-Voltage level 2 alarming!\n",
	"NP Low-Voltage level 1 alarming!\n",
	"NP Low-Voltage level 2 alarming!\n",
};

static void yk3_mbox_emp_vol_alarm(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u32 alarm_code = (u32)msg->data[0];
	struct yk3_card *card = pdev_priv->card;

	if (alarm_code < YK3_VOL_CODE_START || alarm_code > YK3_VOL_CODE_VALID_END) {
		yk3_dev_err("Unknown type Voltage alarming!\n");
		return;
	}

	yk3_dev_err("%s", yk3_vol_alarm_str[alarm_code - YK3_VOL_CODE_START]);

	if (alarm_code & 0x1)
		card->emp_vol_l2_alarm++;
	if (card->emp_vol_l2_alarm == YK3_ALARM_URGENT_MAX) {
		yk3_dev_err("Alarming l2 voltage 3 times, URGENT, device will stop!");
		card->emp_close_hw = true;
	}
}

struct ecc_subsystem_info {
	const char *name;
	int bit_offset;
};

static struct ecc_subsystem_info ecc_subsystems[] = {
	{"emp", YK3_ECC_DATA_EMP_BIT},
	{"m0ppp", YK3_ECC_DATA_M0PPP_BIT},
	{"m1ppp", YK3_ECC_DATA_M1PPP_BIT},
	{"hppp", YK3_ECC_DATA_HPPP_BIT},
	{"sppp", YK3_ECC_DATA_SPPP_BIT},
	{"hdma", YK3_ECC_DATA_HDMA_BIT},
	{"sdma", YK3_ECC_DATA_SDMA_BIT},
	{"mac0", YK3_ECC_DATA_MAC0_BIT},
	{"mac1", YK3_ECC_DATA_MAC1_BIT},
	{"np", YK3_ECC_DATA_NP_BIT},
	{"doe", YK3_ECC_DATA_DOE_BIT},
	{"hqos", YK3_ECC_DATA_HQOS_BIT},
	{"host", YK3_ECC_DATA_HOST_BIT},
	{"soc", YK3_ECC_DATA_SOC_BIT},
	{"lan", YK3_ECC_DATA_LAN_BIT},
	{"nvme", YK3_ECC_DATA_NVME_BIT},
	{"fdma", YK3_ECC_DATA_FDMA_BIT},
	{"hmail", YK3_ECC_DATA_HMAIL_BIT},
	{"smail", YK3_ECC_DATA_SMAIL_BIT},
	{"mems", YK3_ECC_DATA_MEMS_BIT},
	{"pcie", YK3_ECC_DATA_ADAP_BIT},
};

static void yk3_mbox_emp_ecc_alarm(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u32 alarm_code = (u32)msg->data[0];
	u32 alarm_data = (u32)msg->data[4];
	struct ecc_subsystem_info *sub;
	bool ecc_one_bit;
	int num_subsystems;
	int i;

	if (alarm_code == YK3_ECC_CODE_START) {
		ecc_one_bit = true;
	} else if (alarm_code == YK3_ECC_CODE_END) {
		ecc_one_bit = false;
	} else {
		yk3_dev_err("Unknown ECC alarming code, %u\n", alarm_code);
		return;
	}

	num_subsystems = ARRAY_SIZE(ecc_subsystems);
	for (i = 0; i < num_subsystems; i++) {
		sub = &ecc_subsystems[i];
		if ((alarm_data >> sub->bit_offset) & 0x1) {
			if (ecc_one_bit)
				yk3_dev_err("ECC error, 0x%08x, %u\n", alarm_data, sub->bit_offset);
			else
				yk3_dev_err("ECC fault, 0x%08x, %u\n", alarm_data, sub->bit_offset);
		}
	}
}

static void yk3_mbox_emp_i2c_alarm(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u32 alarm_code = (u32)msg->data[0];

	if (alarm_code < YK3_I2C_CODE_START || alarm_code > YK3_I2C_CODE_END) {
		yk3_dev_err("Unknown I2C alarming!\n");
		return;
	}

	yk3_dev_err("Transceiver i2c alarming!\n");
}

struct chip_subsystem_info {
	const char *name;
	int start_bit;
	int end_bit;
};

static struct chip_subsystem_info chip_subsystems[] = {
	{"pcie", YK3_CHIP_DATA_PCIE_STRAT_BIT, YK3_CHIP_DATA_PCIE_END_BIT},
	{"doe", YK3_CHIP_DATA_DOE_STRAT_BIT, YK3_CHIP_DATA_DOE_END_BIT},
	{"emp", YK3_CHIP_DATA_EMP_STRAT_BIT, YK3_CHIP_DATA_EMP_END_BIT},
	{"bar", YK3_CHIP_DATA_BAR_STRAT_BIT, YK3_CHIP_DATA_BAR_END_BIT},
	{"hqos", YK3_CHIP_DATA_HQOS_STRAT_BIT, YK3_CHIP_DATA_HQOS_END_BIT},
	{"edma", YK3_CHIP_DATA_EDMA_STRAT_BIT, YK3_CHIP_DATA_EDMA_END_BIT},
	{"np", YK3_CHIP_DATA_NP_STRAT_BIT, YK3_CHIP_DATA_NP_END_BIT},
	{"noc", YK3_CHIP_DATA_NOC_STRAT_BIT, YK3_CHIP_DATA_NOC_END_BIT},
	{"mailbox", YK3_CHIP_DATA_MBOX_STRAT_BIT, YK3_CHIP_DATA_MBOX_END_BIT},
	{"clock", YK3_CHIP_DATA_CLK_STRAT_BIT, YK3_CHIP_DATA_CLK_END_BIT}
};

static bool yk3_check_alarm(struct yk3_pdev_priv *pdev_priv,
			    const u32 alarm_data[], int start_bit, int end_bit)
{
	int bit, word_index, bit_offset;

	for (bit = start_bit; bit <= end_bit; bit++) {
		word_index = bit / 32;
		bit_offset = bit % 32;
		if (word_index >= YK3_CHIP_ALARM_DATA_SIZE)
			return false;
		if ((alarm_data[word_index] >> bit_offset) & 0x1) {
			yk3_dev_info("subsystem alarm wordindex %d bitoffset %d\n",
				     word_index, bit_offset);
			return true;
		}
	}

	return false;
}

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
bool yk3_vxlan_lookup_port(struct yk3_ndev_priv *ndev_priv, u16 port)
{
	int i;
	bool found = false;

	for (i = 0; i < YK3_VXLAN_PORT_MAX + YK3_VXLAN_RESERVE_PORT; i++) {
		if (ndev_priv->udp_tnl_port->vxlan_entry[i].port == port) {
			found = true;
			break;
		}
	}

	return found;
}

static int yk3_vxlan_tunnel_entry(struct yk3_pdev_priv *pdev_priv, u16 port, u16 op)
{
	struct yk3_ppp_vxlan_val vxlan_val;
	struct yk3_udp_tnl_entry *vxlan_entry = pdev_priv->udp_tnl_port->vxlan_entry;
	bool found = false;
	u32 idx;
	int i, ret = 0;

	if (port == YK3_VXLAN_DEFAULT_PORT) {
		if (op == YK3_SET_UDP_TUNNEL) {
			if (vxlan_entry[YK3_VXLAN_PORT_MAX].ref_count++ == 0) {
				vxlan_val.protocol_num = YK3_VXLAN_DEFAULT_PORT;
				vxlan_val.enable = 1;
				vxlan_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_3,
							  &vxlan_val);
				if (ret) {
					vxlan_entry[YK3_VXLAN_PORT_MAX].ref_count = 0;
					yk3_dev_warn("ppp set vxlan 4789 port errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
			}
		} else if (op == YK3_UNSET_UDP_TUNNEL) {
			if (vxlan_entry[YK3_VXLAN_PORT_MAX].ref_count-- == 1) {
				vxlan_val.protocol_num = YK3_VXLAN_DEFAULT_PORT;
				vxlan_val.enable = 0;
				vxlan_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_3,
							  &vxlan_val);
				if (ret) {
					vxlan_entry[YK3_VXLAN_PORT_MAX].ref_count = 1;
					yk3_dev_warn("ppp unset vxlan 4789 port errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
			}
		}

		return YK3_UDP_TNL_OK;
	}

	if (op == YK3_SET_UDP_TUNNEL) {
		for (i = 0; i < YK3_VXLAN_PORT_MAX; i++) {
			if (vxlan_entry[i].port == port) {
				vxlan_entry[i].ref_count++;
				found = true;
				break;
			}
		}
		/* alloc new entry for vxlan port */
		if (!found) {
			for (i = 0; i < YK3_VXLAN_PORT_MAX; i++) {
				if (!vxlan_entry[i].port) {
					idx = i;
					found = true;
					break;
				}
			}
			if (found) {
				vxlan_entry[idx].port = port;
				vxlan_entry[idx].ref_count = 1;
				vxlan_val.protocol_num = port;
				vxlan_val.enable = 1;
				vxlan_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, idx + 0x12, &vxlan_val);
				if (ret) {
					vxlan_entry[idx].port = 0;
					vxlan_entry[idx].ref_count = 0;
					yk3_dev_warn("ppp set vxlan port failed errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
			} else {
				return YK3_UDP_TNL_NO_ENTRY;
			}
		}
	} else if (op == YK3_UNSET_UDP_TUNNEL) {
		for (i = 0; i < YK3_VXLAN_PORT_MAX; i++) {
			if (vxlan_entry[i].port == port) {
				vxlan_entry[i].ref_count--;
				idx = i;
				found = true;
				break;
			}
		}
		/* free this port entry */
		if (found) {
			if (vxlan_entry[idx].ref_count == 0) {
				vxlan_val.protocol_num = 0;
				vxlan_val.enable = 0;
				vxlan_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, idx + 0x12, &vxlan_val);
				if (ret) {
					vxlan_entry[idx].ref_count++;
					yk3_dev_warn("ppp unset vxlan port failed errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
				vxlan_entry[idx].port = 0;
			}
		} else {
			return YK3_UDP_TNL_NO_ENTRY;
		}
	}

	return YK3_UDP_TNL_OK;
}

static int yk3_geneve_tunnel_entry(struct yk3_pdev_priv *pdev_priv, u16 port, u16 op)
{
	struct yk3_ppp_geneve_val geneve_val;
	bool found = false;
	u32 idx;
	int i, ret = 0;

	if (op == YK3_SET_UDP_TUNNEL) {
		for (i = 0; i < YK3_GENEVE_PORT_MAX; i++) {
			if (pdev_priv->udp_tnl_port->geneve_entry[i].port == port) {
				pdev_priv->udp_tnl_port->geneve_entry[i].ref_count++;
				found = true;
				break;
			}
		}
		/* alloc new entry for geneve port */
		if (!found) {
			for (i = 0; i < YK3_GENEVE_PORT_MAX; i++) {
				if (!pdev_priv->udp_tnl_port->geneve_entry[i].port) {
					idx = i;
					found = true;
					break;
				}
			}
			if (found) {
				pdev_priv->udp_tnl_port->geneve_entry[idx].port = port;
				pdev_priv->udp_tnl_port->geneve_entry[idx].ref_count = 1;
				geneve_val.protocol_num = port;
				geneve_val.enable = 1;
				geneve_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, idx + 0x16, &geneve_val);
				if (ret) {
					pdev_priv->udp_tnl_port->geneve_entry[idx].port = 0;
					pdev_priv->udp_tnl_port->geneve_entry[idx].ref_count = 0;
					yk3_dev_warn("ppp set geneve port failed errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
			} else {
				return YK3_UDP_TNL_NO_ENTRY;
			}
		}
	} else if (op == YK3_UNSET_UDP_TUNNEL) {
		for (i = 0; i < YK3_GENEVE_PORT_MAX; i++) {
			if (pdev_priv->udp_tnl_port->geneve_entry[i].port == port) {
				pdev_priv->udp_tnl_port->geneve_entry[i].ref_count--;
				idx = i;
				found = true;
				break;
			}
		}
		/* free this port entry */
		if (found) {
			if (pdev_priv->udp_tnl_port->geneve_entry[idx].ref_count == 0) {
				geneve_val.protocol_num = 0;
				geneve_val.enable = 0;
				geneve_val.mask = 0xffff;
				ret = yk3_ppp_ivar_modify(pdev_priv, idx + 0x16, &geneve_val);
				if (ret) {
					pdev_priv->udp_tnl_port->geneve_entry[idx].ref_count = 1;
					yk3_dev_warn("ppp unset geneve port failed errno %d", ret);
					return YK3_UDP_TNL_HW_ERR;
				}
				pdev_priv->udp_tnl_port->geneve_entry[idx].port = 0;
			}
		} else {
			return YK3_UDP_TNL_NO_ENTRY;
		}
	}

	return YK3_UDP_TNL_OK;
}

static void yk3_mbox_udp_tunnel_set(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_udp_tunnel_mbox_msg *udp_tunnel = (struct yk3_udp_tunnel_mbox_msg *)msg->data;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	int ret = 0;

	if (!pdev_priv->udp_tnl_port) {
		yk3_dev_warn("Manager udp tunnel port is not initialized\n");
		ret = YK3_UDP_TNL_UNINIT;
		goto send_msg;
	}

	switch (udp_tunnel->tnl_type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		ret = yk3_vxlan_tunnel_entry(pdev_priv, udp_tunnel->port, udp_tunnel->opcode);
		break;
	case UDP_TUNNEL_TYPE_GENEVE:
		ret = yk3_geneve_tunnel_entry(pdev_priv, udp_tunnel->port, udp_tunnel->opcode);
		break;
	default:
		yk3_dev_warn("Unsupported tunnel type %u\n", udp_tunnel->tnl_type);
		ret = YK3_UDP_TNL_INVALID_PARAM;
	}
send_msg:
	mbox_msg.data[0] = ret;
	mbox_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_msg.dst_id = msg->src_id;
	mbox_msg.seqno = msg->seqno;
	mbox_opt.wait_reply = MB_NO_REPLY;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0)
		yk3_dev_err("%s send ack mbox message errno %d failed!\n", __func__, ret);
}

static int yk3_udp_tnl_mbox_ops(struct yk3_ndev_priv *ndev_priv, u16 tnl_type, u16 port, u16 op)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_udp_tnl_entry *vxlan_entry = ndev_priv->udp_tnl_port->vxlan_entry;
	struct yk3_udp_tnl_entry *geneve_entry = ndev_priv->udp_tnl_port->geneve_entry;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	struct yk3_udp_tunnel_mbox_msg *udp_tunnel;
	int i;
	int ret = 0;

	udp_tunnel = (struct yk3_udp_tunnel_mbox_msg *)mbox_msg.data;
	udp_tunnel->port = port;
	udp_tunnel->opcode = op;
	udp_tunnel->tnl_type = tnl_type;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_UDP_TUNNEL;
	mbox_msg.dst_id = yk3_mbox_master_id();
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 300;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &ack_msg);
	if (ret) {
		yk3_net_err("%s send mbox message errno %d failed!\n", __func__, ret);
		return -EFAULT;
	}

	if (ack_msg.data[0] != 0) {
		if (ack_msg.data[0] == YK3_UDP_TNL_NO_ENTRY) {
			yk3_net_debug("%s set tunnel type %u, port %u failed no hw entry\n",
				      __func__, tnl_type, port);
			return 0;
		}

		yk3_net_debug("%s set tunnel type %u, port %u failed errno %d\n",
			      __func__, tnl_type, port, ack_msg.data[0]);
		return -EFAULT;
	}

	if (tnl_type == UDP_TUNNEL_TYPE_VXLAN) {
		for (i = 0; i < YK3_VXLAN_PORT_MAX + YK3_VXLAN_RESERVE_PORT; i++) {
			if (op == YK3_SET_UDP_TUNNEL && vxlan_entry[i].port == 0) {
				vxlan_entry[i].port = port;
				ret = 1;
				break;
			} else if (op == YK3_UNSET_UDP_TUNNEL && vxlan_entry[i].port == port) {
				vxlan_entry[i].port = 0;
				ret = 1;
				break;
			}
		}
	} else if (tnl_type == UDP_TUNNEL_TYPE_GENEVE) {
		for (i = 0; i < YK3_GENEVE_PORT_MAX; i++) {
			if (op == YK3_SET_UDP_TUNNEL && geneve_entry[i].port == 0) {
				geneve_entry[i].port = port;
				ret = 1;
				break;
			} else if (op == YK3_UNSET_UDP_TUNNEL && geneve_entry[i].port == port) {
				geneve_entry[i].port = 0;
				ret = 1;
				break;
			}
		}
	}

	if (ret != 1) {
		yk3_net_err("%s set udp tunnel %s, type %u, port %u failed!\n", __func__,
			    (op == YK3_SET_UDP_TUNNEL) ? "add" : "del", tnl_type, port);
		return -EFAULT;
	}

	return 0;
}

int yk3_udp_tunnel_set(struct net_device *ndev, struct udp_tunnel_info *ti, u16 op)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0;

	if (yk3_pdev_is_vf(pdev_priv)) {
		if (ti->type == UDP_TUNNEL_TYPE_VXLAN) {
			if (ntohs(ti->port) != YK3_VXLAN_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default vxlan port!\n");
				return 0;
			}
		} else if (ti->type == UDP_TUNNEL_TYPE_GENEVE) {
			if (ntohs(ti->port) != YK3_GENEVE_DEFAULT_PORT) {
				yk3_dev_debug("Unsupport VF set non-default geneve port!\n");
				return 0;
			}
		}
	}

	ret = yk3_udp_tnl_mbox_ops(ndev_priv, ti->type, ntohs(ti->port), op);
	if (ret) {
		yk3_net_warn("Failed to set udp tunnel, err is %d", ret);
		return -1;
	}

	return 0;
}

static int yk3_udp_tunnel_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_udp_tunnel_port *udp_tnl_port;

	udp_tnl_port = kzalloc(sizeof(*udp_tnl_port), GFP_KERNEL);
	if (!udp_tnl_port)
		return -ENOMEM;

	pdev_priv->udp_tnl_port = udp_tnl_port;

	return 0;
}
#endif

static void yk3_mbox_emp_chip_alarm(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	u32 alarm_code = (u32)msg->data[0];
	u32 *alarm_data = (u32 *)&msg->data[4];
	struct chip_subsystem_info *sub;
	int num_subsystems;
	bool alarm;
	int i;

	if (alarm_code != YK3_CHIP_CODE_START) {
		yk3_dev_err("Chip alarming code error, code %u\n", alarm_code);
		return;
	}

	num_subsystems = ARRAY_SIZE(chip_subsystems);
	for (i = 0; i < num_subsystems; i++) {
		sub = &chip_subsystems[i];
		alarm = yk3_check_alarm(pdev_priv, alarm_data, sub->start_bit, sub->end_bit);
		if (alarm)
			yk3_dev_err("Chip subsystem %s alarming!\n", sub->name);
	}
}

static u32 yk3_emp_no_tick_cnt;

static void yk3_check_emp_cb(struct timer_list *timer)
{
	struct yk3_card *card = container_of(timer, struct yk3_card, check_emp);
	struct yk3_pdev_priv *pdev_priv = card->mgr_pdev_priv;
	void __iomem *hw_addr;
	u32 val;

	if (!pdev_priv)
		return;

	hw_addr = pdev_priv->bar_addr[YK3_BAR0];
	val = yk3_rd32(hw_addr, YK3_EMP_TICK_ADDR);
	if (val != card->emp_tick) {
		card->emp_tick = val;
	} else {
		if (yk3_emp_no_tick_cnt % 60 == 0)
			yk3_dev_err("EMP has no tick!\n");
		yk3_emp_no_tick_cnt++;
		/* TODO */
	}

	mod_timer(timer, jiffies + msecs_to_jiffies(5000));
}

static int yk3_card_get_empinfo(struct yk3_pdev_priv *pdev_priv, struct yk3_emp_info *emp_info)
{
	int ret;
	void __iomem *hw_addr;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};

	if (yk3_pdev_is_mgr(pdev_priv)) {
		hw_addr = pdev_priv->bar_addr[YK3_BAR0];
		yk3_emp_info_memcpy((void *)emp_info,
				    (hw_addr + YK3_EMP_INFO_ADDR),
				    sizeof(*emp_info));
	} else {
		msg.dst_id = yk3_mbox_master_id();
		msg.opcode = YK3_MBOX_OPCODE_GET_EMP_INFO;
		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 100;
		msg.data[0] = 0;
		ret = yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg);
		if (ret) {
			yk3_dev_err("%s yk3_mbox_send_msg() failed, pf %u ret %d\n",
				    __func__, pdev_priv->pf_id, ret);
			return -EIO;
		}
		memcpy((void *)emp_info,
		       (const void *)ack_msg.data,
		       YK3_MBOX_DATA_LEN);
		msg.data[0] = YK3_MBOX_DATA_LEN;
		ret = yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg);
		if (ret) {
			yk3_dev_err("%s yk3_mbox_send_msg() failed, pf %u ret %d\n",
				    __func__, pdev_priv->pf_id, ret);
			return -EIO;
		}
		memcpy((void *)emp_info + YK3_MBOX_DATA_LEN,
		       (const void *)ack_msg.data,
		       sizeof(*emp_info) - YK3_MBOX_DATA_LEN);
	}

	return 0;
}

static int yk3_card_init_mac_port_type(struct yk3_pdev_priv *pdev_priv,
				       struct yk3_card *card,
				       struct yk3_card_info *info)
{
	void __iomem *hw_addr;
	u32 val;
	u8 type, num;

	if (yk3_pdev_is_mgr(pdev_priv) || yk3_pdev_is_pf(pdev_priv)) {
		if (info->emp_info.vpd.chip_type == YK3_CHIPTYPE_ASIC) {
			card->hw_bits |= YK3_HW_BIT_UMAC;
			type = (info->emp_info.vpd.porttype & YK3_SPEED_MASK) >> YK3_SPEED_SHIFT;
			num = info->emp_info.vpd.porttype & YK3_PORTNUM_MASK;
			yk3_dev_info("this is UMAC, porttype 0x%02x type %u num %u\n",
				     info->emp_info.vpd.porttype, type, num);
			switch (type) {
			case YK3_PORTTYPE_SPEED_10G:
				card->port_type = YK3_10G;
				break;
			case YK3_PORTTYPE_SPEED_25G:
				card->port_type = YK3_25G;
				break;
			case YK3_PORTTYPE_SPEED_40G:
				card->port_type = YK3_40G;
				break;
			case YK3_PORTTYPE_SPEED_100G:
				card->port_type = YK3_100G;
				break;
			default:
				return -EIO;
			}
			switch (num) {
			case 1:
				card->mac_ch_num = 1;
				card->mac_chs[0] = YK3_MAC_CH_0;
				break;
			case 2:
				card->mac_ch_num = 2;
				card->mac_chs[0] = YK3_MAC_CH_0;
				card->mac_chs[1] = YK3_MAC_CH_1;
				break;
			case 4:
				card->mac_ch_num = 4;
				card->mac_chs[0] = YK3_MAC_CH_0;
				card->mac_chs[1] = YK3_MAC_CH_1;
				card->mac_chs[2] = YK3_MAC_CH_2;
				card->mac_chs[3] = YK3_MAC_CH_3;
				break;
			default:
				return -EIO;
			}
		} else {
			hw_addr = pdev_priv->bar_addr[YK3_BAR0];
			val = yk3_rd32(hw_addr, UMACX_CHY_STATUS(0, 0));
			if (!val) {
				yk3_dev_info("this is XMAC, status_reg val %u\n", val);
				card->hw_bits |= YK3_HW_BIT_XMAC;
				card->port_type = YK3_10G;
			} else {
				yk3_dev_info("this is UMAC, status_reg val %u\n", val);
				card->hw_bits |= YK3_HW_BIT_UMAC;
			}
		}
	}

	return 0;
}

static inline int yk3_get_emp_pf_num(u8 board_type, u16 *pf_num)
{
	int ret = 0;

	switch (board_type) {
	case K3_2XSFP:
		*pf_num = 2;
		break;
	case K3_1XQSFP:
		*pf_num = 1;
		break;
	case K3_4XSFP:
		*pf_num = 4;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

int yk3_card_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_card *card;
	int ret;
	struct yk3_card_info info = {0};
	int nid;

	/* 0. init card manager */
	if (!atomic_read(&card_mgr.card_cnt)) {
		idr_init(&card_mgr.idr);
		mutex_init(&card_mgr.mlock);
		memset(card_mgr.pdev_privs, 0, sizeof(card_mgr.pdev_privs));
	}

	card_mgr.pdev_privs[pdev_priv->pf_id][pdev_priv->vf_id] = pdev_priv;
	/* 1. should get card info by mbox */
	/* 1.1 should irq and mbox */
	/* 1.2 get card info by mbox, but now fake */
	info.chip = YK3_K3;
	info.qnum = 1024;
	info.edma_clk = 350 * 1000 * 1000;

	ret = yk3_card_get_empinfo(pdev_priv, &info.emp_info);
	if (ret) {
		yk3_dev_err("Get card emp info failed");
		return ret;
	}
	memcpy(info.sn.num, info.emp_info.vpd.sn, sizeof(info.sn.num));

	/* 2. find card */
	card = yk3_card_find(&info.sn);
	if (card) {
		/* 3. insert */
		mutex_lock(&card->pdev_priv_head_mlock);
		list_add_tail(&pdev_priv->card_node, &card->pdev_priv_head);
		mutex_unlock(&card->pdev_priv_head_mlock);
		pdev_priv->card = card;
		return 0;
	}

	/* 4. if no card then create card */
	card = kzalloc(sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	card->emp_info = kmemdup(&info.emp_info, sizeof(info.emp_info), GFP_KERNEL);
	if (!card->emp_info) {
		kfree(card);
		return -ENOMEM;
	}

	/* 4. alloc id */
	mutex_lock(&card_mgr.mlock);
	ret = idr_alloc(&card_mgr.idr, card, 0, 0, GFP_KERNEL);
	mutex_unlock(&card_mgr.mlock);
	if (ret < 0) {
		yk3_dev_err("alloc card id failed");
		goto err_alloc_id;
	}

	/* 5. fill card */
	card->id = ret;
	card->chip = info.chip;
	card->sn = info.sn;
	card->hw_bits = YK3_HW_BIT_EDMA | YK3_HW_BIT_LAN | YK3_HW_BIT_MBOX;
	card->board_type = info.emp_info.vpd.board_type;

	/* Alternative to determine UMAC or XMAC, init on cardinfo, ONLY ONCE */
	ret = yk3_card_init_mac_port_type(pdev_priv, card, &info);
	if (ret < 0) {
		yk3_dev_err("get emp macporttype failed\n");
		goto err_macporttype;
	}

	card->cap_bits = 0;
	if (mode[0] == 't')
		card->mode = YK3_MODE_TCARD;
	else if (mode[0] == 'r')
		card->mode = YK3_MODE_RCARD;
	else
		card->mode = yk3_pdev_is_rdma(pdev_priv) ? YK3_MODE_RCARD : YK3_MODE_TCARD;

	card->qnum = info.qnum;
	if (card->mode == YK3_MODE_RCARD)
		card->qnum = 512;

	if (pf_num != 0) {
		card->pf_num = pf_num;
	} else {
		ret = yk3_get_emp_pf_num(card->board_type, &card->pf_num);
		if (ret) {
			yk3_dev_err("invalid pf num, board_type %u\n", card->board_type);
			goto err_pf_num;
		}
	}

	pf_qnum = min_t(ushort, pf_qnum, 64);
	pf_qnum = max_t(ushort, pf_qnum, 1);
	card->pf_ndev_qnum = pf_qnum;
	vf_qnum = min_t(ushort, vf_qnum, 64);
	vf_qnum = max_t(ushort, vf_qnum, 1);
	card->vf_ndev_qnum = vf_qnum;
	card->numa = dev_to_node(pdev_priv->dev);
	card->edma_clk = info.edma_clk;
	card->coalesce_max_usecs = 65535 * 256 / (info.edma_clk / 1000 / 1000);
	card->coalesce_max_usecs = min_t(u32, 30000, card->coalesce_max_usecs);
	card->coalesce_max_frames = 128;
	INIT_LIST_HEAD(&card->pdev_priv_head);
	mutex_init(&card->pdev_priv_head_mlock);
	snprintf(card->name, sizeof(card->name), "card-%d", card->id);
	card->qsetid_base = 0;
	card->qsetid_num = 512;

	if (!zalloc_cpumask_var(&card->local_cpumask, GFP_KERNEL))
		goto err_alloc_local;

	if (!zalloc_cpumask_var(&card->neigh_cpumask, GFP_KERNEL))
		goto err_alloc_neigh;

	if (!zalloc_cpumask_var(&card->remote_cpumask, GFP_KERNEL))
		goto err_alloc_remote;

	if (card->numa < 0) {
		cpumask_or(card->local_cpumask, card->local_cpumask, cpu_online_mask);
	} else {
		for_each_online_node(nid) {
			if (nid == card->numa) {
				cpumask_or(card->local_cpumask, card->local_cpumask,
					   cpumask_of_node(nid));
				continue;
			}
			if (node_distance(card->numa, nid) <= REMOTE_DISTANCE) {
				cpumask_or(card->neigh_cpumask, card->neigh_cpumask,
					   cpumask_of_node(nid));
				continue;
			}

			cpumask_or(card->remote_cpumask, card->remote_cpumask,
				   cpumask_of_node(nid));
		}
	}

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_GET_EMP_INFO,
					   yk3_mbox_get_emp_info,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_TEMP_ALARM,
					   yk3_mbox_emp_temp_alarm,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_VOL_ALARM,
					   yk3_mbox_emp_vol_alarm,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_ECC_ALARM,
					   yk3_mbox_emp_ecc_alarm,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_I2C_ALARM,
					   yk3_mbox_emp_i2c_alarm,
					   (void *)pdev_priv);
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_EMP_CHIP_ALARM,
					   yk3_mbox_emp_chip_alarm,
					   (void *)pdev_priv);
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
		yk3_mbox_register_callback(pdev_priv,
					   YK3_MBOX_OPCODE_SET_UDP_TUNNEL,
					   yk3_mbox_udp_tunnel_set,
					   (void *)pdev_priv);
#endif
		timer_setup(&card->check_emp, yk3_check_emp_cb, 0);
		mod_timer(&card->check_emp, jiffies + msecs_to_jiffies(1000));
	}

	card->emp_tick = 0;
	card->emp_temp_l2_alarm = 0;
	card->emp_vol_l2_alarm = 0;
	card->emp_close_hw = false;

	if (yk3_pdev_is_mgr(pdev_priv))
		card->mgr_pdev_priv = pdev_priv;
	pdev_priv->card = card;

	mutex_lock(&card->pdev_priv_head_mlock);
	list_add_tail(&pdev_priv->card_node, &card->pdev_priv_head);
	mutex_unlock(&card->pdev_priv_head_mlock);
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	if (yk3_pdev_is_mgr(pdev_priv)) {
		ret = yk3_udp_tunnel_init(pdev_priv);
		if (ret)
			goto err_udp_tnl_port;
	}
#endif
	ret = yk3_debug_card_init(card);
	if (ret) {
		yk3_dev_err("debug card init failed");
		goto err_debug;
	}

	atomic_inc(&card_mgr.card_cnt);

	return 0;

err_debug:
	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_GET_EMP_INFO);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_TEMP_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_VOL_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_ECC_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_I2C_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_CHIP_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_SET_UDP_TUNNEL);
		del_timer_sync(&card->check_emp);
	}
	free_cpumask_var(card->remote_cpumask);
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	kfree(pdev_priv->udp_tnl_port);
err_udp_tnl_port:
#endif
err_alloc_remote:
	free_cpumask_var(card->neigh_cpumask);
err_alloc_neigh:
	free_cpumask_var(card->local_cpumask);
err_alloc_local:
err_pf_num:
err_macporttype:
	mutex_lock(&card_mgr.mlock);
	idr_remove(&card_mgr.idr, card->id);
	mutex_unlock(&card_mgr.mlock);
err_alloc_id:
	kfree(card->emp_info);
	kfree(card);
	return ret;
}

void yk3_card_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_card *card = pdev_priv->card;

	if (!card)
		return;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_GET_EMP_INFO);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_TEMP_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_VOL_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_ECC_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_I2C_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EMP_CHIP_ALARM);
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_SET_UDP_TUNNEL);
		del_timer_sync(&card->check_emp);
	}

	mutex_lock(&card->pdev_priv_head_mlock);
	list_del(&pdev_priv->card_node);
	mutex_unlock(&card->pdev_priv_head_mlock);
	pdev_priv->card = NULL;
	card_mgr.pdev_privs[pdev_priv->pf_id][pdev_priv->vf_id] = NULL;
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	kfree(pdev_priv->udp_tnl_port);
#endif
	if (!list_empty(&card->pdev_priv_head))
		return;
	atomic_dec(&card_mgr.card_cnt);
	yk3_debug_card_exit(card);
	free_cpumask_var(card->remote_cpumask);
	free_cpumask_var(card->neigh_cpumask);
	free_cpumask_var(card->local_cpumask);
	mutex_lock(&card_mgr.mlock);
	idr_remove(&card_mgr.idr, card->id);
	mutex_unlock(&card_mgr.mlock);
	kfree(card->emp_info);
	kfree(card);
}
