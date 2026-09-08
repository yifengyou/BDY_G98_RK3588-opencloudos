// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_np_priv.h"

enum YK3_NP_CMD_TYPE {
	YK3_NP_CMD_REG_CFG_SET     = 0,
	YK3_NP_CMD_RSP,
};

struct yk3_np_mbox_cmd {
	enum YK3_NP_CMD_TYPE cmd_type;
	union {
		struct {
			int type;
			u16 val;
		} cfg;
		struct {
			int rc;
		} rsp;
	} cmd_data;
};

static int yk3_np_set_doe_access(struct yk3_pdev_priv *pdev_priv, bool access)
{
	int ret = 0;
	bool protect = !access;
	bool ready = access;

	ret = yk3_doe_set_protect(pdev_priv, protect);
	if (ret) {
		yk3_dev_err("np doe protect failed, ret = %d", ret);
		return ret;
	}

	ret = yk3_np_set_tbl_ready(pdev_priv, ready);
	if (ret) {
		yk3_dev_err("np set table ready failed, ret = %d", ret);
		return ret;
	}

	return 0;
}

static int yk3_np_base_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;

	ret = yk3_np_set_doe_access(pdev_priv, false);
	if (ret) {
		yk3_dev_err("disable doe access, ret = %d.", ret);
		return ret;
	}

	/* Table create */
	ret = yk3_np_doe_tbl_init(pdev_priv);
	if (ret) {
		yk3_dev_err("doe table init failed, ret = %d.", ret);
		return ret;
	}

	ret = yk3_np_set_doe_access(pdev_priv, true);
	if (ret) {
		yk3_dev_err("Enable doe access failed, ret = %d.", ret);
		goto fail;
	}

fail:
	if (ret)
		yk3_np_doe_tbl_fini(pdev_priv);

	return ret;
}

static void yk3_np_base_fini(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;

	ret = yk3_np_set_doe_access(pdev_priv, false);
	if (ret)
		yk3_dev_err("disable doe access, ret = %d.", ret);

	yk3_np_doe_tbl_fini(pdev_priv);
}

static int yk3_np_legacy_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;

	ret = yk3_np_base_init(pdev_priv);
	if (ret) {
		yk3_err("[NP] Base init failed, ret %d.\n", ret);
		return ret;
	}

	ret = yk3_np_set_simple_forward(pdev_priv, false);
	if (ret) {
		yk3_err("[NP] Set simple forward failed, ret %d.\n", ret);
		goto fail;
	}

	return 0;

fail:
	/* Error handling */
	yk3_np_base_fini(pdev_priv);
	return ret;
}

static void yk3_np_legacy_fini(struct yk3_pdev_priv *pdev_priv)
{
	yk3_np_base_fini(pdev_priv);
}

static inline u32 yk3_value_recompute(u32 old_val, u16 set_val, u16 mask, u32 shift)
{
	u32 set_val32 = set_val;
	u32 mask32 = mask;

	set_val32 <<= shift;
	mask32 <<= shift;

	return ((old_val & ~mask32) | (set_val32 & mask32));
}

struct yk3_np_cfg {
	const char *name;
	const enum ysc_np_cfg_type type;
	const u16 mask;
	const u8 val_shift;
	const u8 reg_shift;
	const enum YK3_NP_REG_FIELD field_type;
	const u32 cls_bitmap;
};

static const struct yk3_np_cfg yk3_np_cfg_legacy_list[] = {
	{
		.name = "Doe table ready",
		.type = YK3_NP_CFG_DOE_TBL_READY,
		.mask = 1,
		.val_shift = 0,
		.reg_shift = 0,
		.field_type = YK3_NP_REG_F_HOST_FW_MSG1_L,
		.cls_bitmap = BIT(YK3_NP_CLS_ROLE_PARSER) |
			      BIT(YK3_NP_CLS_ROLE_MIRROR_METER) |
			      BIT(YK3_NP_CLS_ROLE_LRO),
	},
	{
		.name = "EMP meter",
		.type = YK3_NP_CFG_EMP_METER,
		.mask = (1 << 1),
		.val_shift = 1,
		.reg_shift = 0,
		.field_type = YK3_NP_REG_F_HOST_FW_MSG1_L,
		.cls_bitmap = BIT(YK3_NP_CLS_ROLE_PARSER),
	},
	{
		.name = "simple forward mode",
		.type = YK3_NP_CFG_SIMPLE_FORWARD,
		.mask = (1 << 2),
		.val_shift = 2,
		.reg_shift = 0,
		.field_type = YK3_NP_REG_F_HOST_FW_MSG1_L,
		.cls_bitmap = BIT(YK3_NP_CLS_ROLE_PARSER),
	},
};

static bool yk3_np_cfg_cls_valid(struct yk3_np *np,
				 u16 cls_id,
				 const struct yk3_np_cfg *cfg)
{
	if (cls_id >= ARRAY_SIZE(np->cls_role))
		return false;

	if (cfg->cls_bitmap & BIT(np->cls_role[cls_id]))
		return true;

	return false;
}

static const char *
yk3_np_cls_role_str(enum yk3_np_cls_role_type role)
{
	switch (role) {
	case YK3_NP_CLS_ROLE_DISABLED:
		return "disabled";
	case YK3_NP_CLS_ROLE_PARSER:
		return "parser";
	case YK3_NP_CLS_ROLE_MIRROR_METER:
		return "mirror/meter";
	case YK3_NP_CLS_ROLE_LRO:
		return "lro";
	default:
		return "unknown role";
	}
}

static const struct yk3_np_cfg *yk3_np_legacy_get_cfg(int type)
{
	size_t i = 0;
	const struct yk3_np_cfg *cfg = NULL;

	for (i = 0; i < ARRAY_SIZE(yk3_np_cfg_legacy_list); i++) {
		cfg = &yk3_np_cfg_legacy_list[i];
		if (cfg->type == type)
			return cfg;
	}

	return NULL;
}

struct yk3_np_ops {
	int (*init)(struct yk3_pdev_priv *pdev_priv);
	void (*fini)(struct yk3_pdev_priv *pdev_priv);
	const struct yk3_np_cfg *(*get_cfg)(int type);
};

static const struct yk3_np_ops yk3_np_legacy_ops = {
	.init = yk3_np_legacy_init,
	.fini = yk3_np_legacy_fini,
	.get_cfg = yk3_np_legacy_get_cfg,
};

static int yk3_np_do_set_cfg(struct yk3_pdev_priv *pdev_priv,
			     enum ysc_np_cfg_type type, u16 val)
{
	int ret = 0;
	size_t cls_id = 0;
	u32 value = 0;
	u32 reg_val = 0;
	struct yk3_np *np = NULL;
	const struct yk3_np_cfg *cfg = NULL;

	np = pdev_priv->card->np;
	if (IS_ERR_OR_NULL(np)) {
		yk3_dev_err("np sw not found for set cfg, probe might failed.");
		return -EINVAL;
	}

	cfg = np->ops->get_cfg(type);
	if (!cfg) {
		yk3_dev_err("Got unknown cfg type: %d.", type);
		return -EINVAL;
	}

	/* Protect cfg from process context and mbox handler softirq. */
	spin_lock_bh(&np->cfg_lock);
	for (cls_id = 0; cls_id < YK3_NP_PPE_CLUSTE_NUM; cls_id++) {
		if (!yk3_np_cfg_cls_valid(np, cls_id, cfg))
			continue;

		ret = yk3_np_frd32(pdev_priv, cls_id, cfg->field_type, &value);
		if (ret)
			goto out;

		if (value == YK3_NP_REG_MAGIC) {
			ret = -EINVAL;
			goto out;
		}
		reg_val = yk3_value_recompute(value, (val << cfg->val_shift),
					      cfg->mask, cfg->reg_shift);

		ret = yk3_np_fwr32(pdev_priv, cls_id, cfg->field_type, reg_val);
		if (ret)
			goto out;
	}
out:
	spin_unlock_bh(&np->cfg_lock);
	/* Put error output outside spinlock. */
	if (ret)
		yk3_dev_err("Invalid reg found for cluster %lu.", cls_id);
	return ret;
}

static void yk3_np_mbox_cmd_handler(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = param;

	const struct yk3_np_mbox_cmd *cmd = (struct yk3_np_mbox_cmd *)msg->data;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_np_mbox_cmd *cmd_ack = NULL;
	struct yk3_mbox_option opt = {0};

	int ret = 0;
	int cfg_type = 0;
	u16 cfg_val = 0;

	cmd_ack = (struct yk3_np_mbox_cmd *)ack_msg.data;
	cmd_ack->cmd_type = YK3_NP_CMD_RSP;
	yk3_dev_info("np mailbox cmd handler: cmd type:%d", cmd->cmd_type);

	cfg_type = cmd->cmd_data.cfg.type;
	cfg_val = cmd->cmd_data.cfg.val;
	switch (cmd->cmd_type) {
	case YK3_NP_CMD_REG_CFG_SET:
		ret = yk3_np_do_set_cfg(pdev_priv, cfg_type, cfg_val);
		if (ret) {
			yk3_dev_err("np set reg cfg failed, ret %d, type %x, value %x",
				    ret, cfg_type, cfg_val);
			cmd_ack->cmd_data.rsp.rc = -1;
		} else {
			cmd_ack->cmd_data.rsp.rc = 0;
		}
		break;

	default:
		yk3_dev_err("np mailbox unknown cmd type:0x%x", cmd->cmd_type);
		cmd_ack->cmd_data.rsp.rc = -2;
		break;
	}

	// response message
	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	ret = yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
	if (ret)
		yk3_dev_err("np mbox send ack msg failed, ret = %d.", ret);
}

static int yk3_np_cfg_debugfs_show(struct seq_file *seq, void *data)
{
	struct yk3_np *np = seq->private;
	int cls = 0;

	seq_puts(seq, "NP config show by cluster.\n");
	for (cls = 0; cls < YK3_NP_PPE_CLUSTE_NUM; cls++) {
		if (np->cls_role[cls] == YK3_NP_CLS_ROLE_DISABLED)
			continue;

		seq_printf(seq, "Cluster %d role : %s\n", cls,
			   yk3_np_cls_role_str(np->cls_role[cls]));
	}
	seq_puts(seq, "\n");

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_np_cfg_debugfs);

static int yk3_np_mbox_cmd_send(struct yk3_pdev_priv *pdev_priv,
				const struct yk3_np_mbox_cmd *cmd)
{
	int ret = 0;
	struct yk3_mbox_msg msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt = {0};
	struct yk3_np_mbox_cmd *cmd_ack = NULL;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_dev_err("No need to run mbox on master node.");
		return -EOPNOTSUPP;
	}

	if (sizeof(struct yk3_np_mbox_cmd) > sizeof(msg.data))
		return -EFAULT;
	memcpy(msg.data, cmd, sizeof(struct yk3_np_mbox_cmd));

	// Only master pf can access np reg. VF/PF can both mailbox to master.
	msg.dst_id = yk3_mbox_master_id();
	msg.opcode = YK3_MBOX_OPCODE_NP_OPT;

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	ret = yk3_mbox_send_msg(pdev_priv, &msg, &opt, &ack_msg);
	if (ret) {
		yk3_dev_err("np mbox cmd type %d failed, ret %d.", cmd->cmd_type, ret);
		return -EINVAL;
	}

	cmd_ack = (struct yk3_np_mbox_cmd *)ack_msg.data;
	if (cmd_ack->cmd_type == YK3_NP_CMD_RSP && cmd_ack->cmd_data.rsp.rc == 0)
		return 0;

	yk3_dev_err("np mbox rsp failed, cmd type %d, ret = %d.",
		    cmd_ack->cmd_type, cmd_ack->cmd_data.rsp.rc);
	return -EINVAL;
}

static int yk3_np_mbox_set_cfg(struct yk3_pdev_priv *pdev_priv,
			       enum ysc_np_cfg_type type, u16 val)
{
	const struct yk3_np_mbox_cmd cmd = {
		.cmd_type = YK3_NP_CMD_REG_CFG_SET,
		.cmd_data.cfg = {
			.type = type,
			.val = val,
		},
	};

	return yk3_np_mbox_cmd_send(pdev_priv, &cmd);
}

static int yk3_np_set_cfg(struct yk3_pdev_priv *pdev_priv, enum ysc_np_cfg_type type, u16 val)
{
	int ret = 0;

	if (yk3_pdev_is_mgr(pdev_priv))
		ret = yk3_np_do_set_cfg(pdev_priv, type, val);
	else
		ret = yk3_np_mbox_set_cfg(pdev_priv, type, val);
	return ret;
}

int yk3_np_set_tbl_ready(struct yk3_pdev_priv *pdev_priv, bool ready)
{
	u16 val = ready ? 1 : 0;

	return yk3_np_set_cfg(pdev_priv, YK3_NP_CFG_DOE_TBL_READY, val);
}

int yk3_np_set_emp_meter(struct yk3_pdev_priv *pdev_priv, bool enable)
{
	u16 val = enable ? 1 : 0;

	return yk3_np_set_cfg(pdev_priv, YK3_NP_CFG_EMP_METER, val);
}

int yk3_np_set_simple_forward(struct yk3_pdev_priv *pdev_priv, bool enable)
{
	u16 val = enable ? 1 : 0;

	return yk3_np_set_cfg(pdev_priv, YK3_NP_CFG_SIMPLE_FORWARD, val);
}

int yk3_np_set_ign_frag_l4_port(struct yk3_pdev_priv *pdev_priv, bool ignore)
{
	u16 val = ignore ? 1 : 0;

	return yk3_np_set_cfg(pdev_priv, YK3_NP_CFG_IGN_FRAG_L4_PORT, val);
}

int yk3_np_set_tbl_cache_miss(struct yk3_pdev_priv *pdev_priv, bool cache_miss)
{
	u16 val = cache_miss ? 1 : 0;

	return yk3_np_set_cfg(pdev_priv, YK3_NP_CFG_TBL_CACHE_MISS, val);
}

u32 yk3_np_rd32(struct yk3_pdev_priv *pdev_priv, u32 offset_to_np)
{
	void  __iomem *baddr = pdev_priv->bar_addr[YK3_NP_REGS_BAR];
	u32 hw_reg_offset = 0;

	if (offset_to_np >= YK3_NP_REG_OFFSET_MAX)
		return 0;

	hw_reg_offset = YK3_NP_BASE + offset_to_np;
	return yk3_rd32(baddr, hw_reg_offset);
}

void yk3_np_wr32(struct yk3_pdev_priv *pdev_priv, u32 offset_to_np, u32 val)
{
	void __iomem *baddr = pdev_priv->bar_addr[YK3_NP_REGS_BAR];
	u32 hw_reg_offset = 0;

	if (offset_to_np >= YK3_NP_REG_OFFSET_MAX)
		return;

	hw_reg_offset = YK3_NP_BASE + offset_to_np;
	yk3_wr32(baddr, hw_reg_offset, val);
}

struct yk3_np_reg_module {
	u32 base;
	u16 elem_num;
	u32 elem_size;
};

static const struct yk3_np_reg_module yk3_np_reg_module_info[] = {
	[YK3_NP_REG_M_PPE_CLUSTER] = {
		.base = 0x000000,
		.elem_num = YK3_NP_PPE_CLUSTE_NUM,
		.elem_size = 0x20000,
	},
	[YK3_NP_REG_M_INGRESS_MGR] = {
		.base = 0x300000,
		.elem_num = 5,
		.elem_size = 0x20000,
	},
	[YK3_NP_REG_M_EGRESS_MGR] = {
		.base = 0x3a0000,
		.elem_num = 5,
		.elem_size = 0x20000,
	},
	[YK3_NP_REG_M_PACKET_BUF] = {
		.base = 0x440000,
		.elem_num = 8,
		.elem_size = 0x4000,
	},
	[YK3_NP_REG_M_NP_GOBAL_CTRL] = {
		.base = 0x460000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_NP_BIST] = {
		.base =  0x470000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_META] = {
		.base = 0x480000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_LOG_PACKETER] = {
		.base = 0x490000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_TIMER] = {
		.base = 0x4a0000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_MQBUF] = {
		.base = 0x4b0000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_PCIE_MSIX] = {
		.base = 0x4c0000,
		.elem_num = 1,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_MONITOR_CNT] = {
		.base = 0x4d0000,
		.elem_num = 2,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_SENSOR] = {
		.base = 0x4f0000,
		.elem_num = 2,
		.elem_size = 0x10000,
	},
	[YK3_NP_REG_M_UNIT] = {
		.base = 0x000000,
		.elem_num = 6,
		.elem_size = 0x80000,
	},
};

static int yk3_np_reg_module_get_offset(struct yk3_pdev_priv *pdev_priv,
					enum YK3_NP_REG_MODULE module,
					u16 module_elem_idx,
					u32 offset_to_module,
					u32 *offset_to_np)
{
	const struct yk3_np_reg_module *info = NULL;

	// TODO: get module info accroding to card type.
	if (module < 0 || module >= ARRAY_SIZE(yk3_np_reg_module_info))
		return -EINVAL;

	info = &yk3_np_reg_module_info[module];
	if (module_elem_idx >= info->elem_num)
		return -EINVAL;

	if (offset_to_module >= info->elem_size)
		return -EINVAL;

	*offset_to_np = info->base + module_elem_idx * info->elem_size;
	*offset_to_np += offset_to_module;
	return 0;
}

int yk3_np_mrd32(struct yk3_pdev_priv *pdev_priv,
		 enum YK3_NP_REG_MODULE module,
		 u16 module_elem_idx,
		 u32 offset_to_module,
		 u32 *val)
{
	u32 offset_to_np = 0;
	int ret = 0;

	ret = yk3_np_reg_module_get_offset(pdev_priv, module, module_elem_idx,
					   offset_to_module, &offset_to_np);
	if (ret)
		return ret;

	*val = yk3_np_rd32(pdev_priv, offset_to_np);
	return 0;
}

int yk3_np_mwr32(struct yk3_pdev_priv *pdev_priv,
		 enum YK3_NP_REG_MODULE module,
		 u16 module_elem_idx,
		 u32 offset_to_module,
		 u32 val)
{
	u32 offset_to_np = 0;
	int ret = 0;

	ret = yk3_np_reg_module_get_offset(pdev_priv, module, module_elem_idx,
					   offset_to_module, &offset_to_np);
	if (ret)
		return ret;

	yk3_np_wr32(pdev_priv, offset_to_np, val);
	return 0;
}

struct yk3_np_reg_field {
	enum YK3_NP_REG_MODULE module;
	u32 base;
	u8 bit_size;
};

static const struct yk3_np_reg_field yk3_np_reg_field_info[] = {
	[YK3_NP_REG_F_HOST_FW_MSG0_L] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x01000,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_HOST_FW_MSG0_H] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x01004,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_HOST_FW_MSG1_L] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x01008,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_HOST_FW_MSG1_H] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x0100C,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_HOST_FW_MSG2_L] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x01040,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_HOST_FW_MSG2_H] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x01044,
		.bit_size = 32,
	},
	[YK3_NP_REG_F_SHM_ATOM] = {
		.module = YK3_NP_REG_M_PPE_CLUSTER,
		.base = 0x1001C,
		.bit_size = 32,
	},
};

int yk3_np_frd32(struct yk3_pdev_priv *pdev_priv,
		 u16 module_elem_idx,
		 enum YK3_NP_REG_FIELD field_type,
		 u32 *val)
{
	const struct yk3_np_reg_field *field = NULL;

	if (field_type < 0 || field_type >= ARRAY_SIZE(yk3_np_reg_field_info))
		return -EINVAL;

	field = &yk3_np_reg_field_info[field_type];
	if (!field->bit_size)
		return -EINVAL;

	return yk3_np_mrd32(pdev_priv, field->module,
			    module_elem_idx, field->base, val);
}

int yk3_np_fwr32(struct yk3_pdev_priv *pdev_priv,
		 u16 module_elem_idx,
		 enum YK3_NP_REG_FIELD field_type,
		 u32 val)
{
	const struct yk3_np_reg_field *field = NULL;

	if (field_type < 0 || field_type >= ARRAY_SIZE(yk3_np_reg_field_info))
		return -EINVAL;

	field = &yk3_np_reg_field_info[field_type];
	if (!field->bit_size)
		return -EINVAL;

	return yk3_np_mwr32(pdev_priv, field->module,
			    module_elem_idx, field->base, val);
}

#define YK3_NP_SHM_ADDR_MAX 0x1FFF
#define YK3_NP_SHM_ADDR_MIN 0x1000

int yk3_np_swr16(struct yk3_pdev_priv *pdev_priv,
		 u16 cls_id,
		 u16 shm_addr,
		 u16 val)
{
	u32 addr_val = 0;

	if (shm_addr > YK3_NP_SHM_ADDR_MAX || shm_addr < YK3_NP_SHM_ADDR_MIN) {
		yk3_dev_err("Write share memory option addr %x overflow.", shm_addr);
		return -EOVERFLOW;
	}

	/* HW only support atomic write, no atomic read. */
	addr_val = (shm_addr << 16) | val;
	return yk3_np_fwr32(pdev_priv, cls_id, YK3_NP_REG_F_SHM_ATOM, addr_val);
}

static int yk3_np_cls_role(struct yk3_pdev_priv *pdev_priv, struct yk3_np *np)
{
	int i = 0;
	int role = 0;
	int ret = 0;
	u32 role_data = 0;

	ret = yk3_np_frd32(pdev_priv, 1, YK3_NP_REG_F_HOST_FW_MSG0_L, &role_data);
	if (ret) {
		yk3_dev_err("np read cluster 1 host_msg0_l failed, ret %d.", ret);
		return ret;
	}

	for (i = 0; i < 8; i++) {
		role = (role_data >> (i * 4)) & 0x0F;
		if (role >= YK3_NP_CLS_ROLE_MAX) {
			yk3_dev_err("invalid np cluster %d, role = %d.", i, role);
			return -EINVAL;
		}

		/*
		 * 0: disable
		 * 1: parse
		 * 2: mirror/meter
		 * 3: lro
		 */
		yk3_dev_info("np cluster %d, role %s.", i, yk3_np_cls_role_str(role));
		np->cls_role[i] = role;
	}

	return 0;
}

int yk3_np_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;
	struct yk3_np *np = NULL;
	char work_q_name[32] = {0};

	/* Get info from pdev_priv */
	const u32 card_id = pdev_priv->card->id;
	const int mode = pdev_priv->card->mode;
	const int chip = pdev_priv->card->chip;
	struct dentry *debugfs_node = pdev_priv->card->dbgfs_dir;

#if YK3_SKIP_NP
	(void)yk3_np_mbox_cmd_handler;
	return 0;
#endif
	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	switch (mode) {
	case YK3_MODE_TCARD:
		break;

	case YK3_MODE_RCARD:
		return 0;

	case YK3_MODE_ECARD:
		yk3_dev_err("dpu mode %d not valid yet.", mode);
		return -EOPNOTSUPP;
	default:
		yk3_dev_err("Unknown dpu mode %d found.", mode);
		return -EINVAL;
	}

	switch (chip) {
	case YK3_K3:
		break;
	case YK3_K3MAX:
		yk3_dev_err("chip type %d not valid yet.", chip);
		return -EOPNOTSUPP;
	default:
		yk3_dev_err("Unknown chip type %d found.", chip);
		return -EINVAL;
	}

	np = kzalloc(sizeof(*np), GFP_KERNEL);
	if (!np)
		return -ENOMEM;

	INIT_LIST_HEAD(&np->table_head);
	spin_lock_init(&np->cfg_lock);
	np->pdev_priv = pdev_priv;
	np->ops = &yk3_np_legacy_ops;

	/* workqueue. */
	snprintf(work_q_name, sizeof(work_q_name), "yk3_np_work_%u", card_id);
	np->wq = create_singlethread_workqueue(work_q_name);
	if (!np->wq) {
		yk3_dev_err("Failed to create workqueue %s.", work_q_name);
		ret = -EINVAL;
		goto fail_with_alloc;
	}

	/* debugfs */
	if (debugfs_node) {
		np->debugfs_root = debugfs_create_dir("np", debugfs_node);
		if (IS_ERR(np->debugfs_root)) {
			yk3_dev_err("Failed to create np debugfs node.");
			ret = -EINVAL;
			np->debugfs_root = NULL;
		} else if (!np->debugfs_root) {
			/* Move on when debugfs is disabled. */
			yk3_dev_info("The debugfs is disabled or node already exists.");
		}
		if (ret)
			goto fail_with_wq;
	}

	/* np cfg debugfs, no need to check return code. */
	if (np->debugfs_root)
		debugfs_create_file("cfg", 0400, np->debugfs_root, np, &yk3_np_cfg_debugfs_fops);

	ret = yk3_np_cls_role(pdev_priv, np);
	if (ret) {
		yk3_dev_err("np failed to get cluster role, ret %d.", ret);
		goto fail_with_dbg_fs;
	}

	pdev_priv->card->np = np;
	/* Run init until pointer connect done. */
	ret = np->ops->init(pdev_priv);
	if (ret) {
		yk3_dev_err("Failed to run ops init, ret = %d.", ret);
		goto fail_with_dbg_fs;
	}

	ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_NP_OPT,
					 yk3_np_mbox_cmd_handler, pdev_priv);
	if (ret) {
		yk3_dev_err("np register qset mbox callback failed.");
		goto fail_with_ops_init;
	}

	yk3_dev_info("np init success.");
	return 0;

fail_with_ops_init:
	if (np)
		np->ops->fini(pdev_priv);
fail_with_dbg_fs:
	pdev_priv->card->np = NULL;
	debugfs_remove_recursive(np->debugfs_root);
fail_with_wq:
	destroy_workqueue(np->wq);
fail_with_alloc:
	kfree(np);
	return ret;
}

void yk3_np_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_np *np = NULL;

#if YK3_SKIP_NP
	return;
#endif
	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	np = pdev_priv->card->np;
	if (!np)
		return;

	yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_NP_OPT);

	np->ops->fini(pdev_priv);
	debugfs_remove_recursive(np->debugfs_root);
	destroy_workqueue(np->wq);
	kfree(np);

	pdev_priv->card->np = NULL;
	yk3_dev_info("np exit.");
}
