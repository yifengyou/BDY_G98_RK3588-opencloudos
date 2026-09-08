// SPDX-License-Identifier: GPL-2.0

#include "yk3_ppp_priv.h"

#undef PPP_DEBUG

static struct ivar_section ivar_section[PPP_MAX] = {{0}, {0}};
static struct ivar ivar_list[PPP_MAX][YK3_PPP_IVAR_MAX] = {{{0}}, {{0}}};

static struct action_entry_raw action_entry_read(struct yk3_pdev_priv *pdev_priv,
						 u8 ppp_id, u16 act_id)
{
	u32 buf[4] = {0};
	struct action_entry_raw ret = {{0}};
	void __iomem *addr = NULL;
	int i = 0;
	u8 *p = (u8 *)&buf;

	if (!pdev_priv->ppp)
		return ret;

	addr = pdev_priv->ppp->addr;
	addr += ppp_id == PPP_ALL ? YK3_PPP_ALL_BASE : YK3_PPP_LITE_BASE;

	if (act_id >= YK3_PPP_ACTION_MAX_SIZE)
		return ret;

	yk3_wr32(addr, YK3_PPP_ACTION_RADDR, act_id);
	buf[0] = yk3_rd32(addr, YK3_PPP_ACTION_DATA);
	buf[1] = yk3_rd32(addr, YK3_PPP_ACTION_DATA);
	buf[2] = yk3_rd32(addr, YK3_PPP_ACTION_DATA);
	buf[3] = yk3_rd32(addr, YK3_PPP_ACTION_DATA);

#ifdef PPP_DEBUG
	yk3_dev_debug(" %s:", ppp_id == PPP_ALL ? "PPP_ALL" : "PPP_LITE");
	yk3_dev_debug("buf[0] = 0x%08x", buf[0]);
	yk3_dev_debug("buf[1] = 0x%08x", buf[1]);
	yk3_dev_debug("buf[2] = 0x%08x", buf[2]);
	yk3_dev_debug("buf[3] = 0x%08x", buf[3]);
#endif

	for (i = 0; i < 15; i++) {
#ifdef PPP_DEBUG
		yk3_dev_debug("p[%d] = 0x%02x", i, p[i]);
#endif
		ret.raw[i] = p[i];
	}

	return ret;
}

static int action_entry_write(struct yk3_pdev_priv *pdev_priv,
			      u8 ppp_id, u16 act_id, struct action_entry_raw ent)
{
#ifdef PPP_DEBUG
	int i = 0;
	u8 *p;
#endif
	u32 buf[4] = {0};
	void __iomem *addr = NULL;

	if (!pdev_priv->ppp)
		return -EINVAL;

	addr = pdev_priv->ppp->addr;
	addr += ppp_id == PPP_ALL ? YK3_PPP_ALL_BASE : YK3_PPP_LITE_BASE;

	if (act_id >= YK3_PPP_ACTION_MAX_SIZE)
		return -EINVAL;

	memcpy(buf, &ent, sizeof(ent));
	yk3_wr32(addr, YK3_PPP_ACTION_WADDR, act_id);
	yk3_wr32(addr, YK3_PPP_ACTION_DATA, buf[0]);
	yk3_wr32(addr, YK3_PPP_ACTION_DATA, buf[1]);
	yk3_wr32(addr, YK3_PPP_ACTION_DATA, buf[2]);
	yk3_wr32(addr, YK3_PPP_ACTION_DATA, buf[3]);

#ifdef PPP_DEBUG
	p = (u8 *)&buf;

	yk3_dev_debug(" %s:", ppp_id == PPP_ALL ? "PPP_ALL" : "PPP_LITE");
	yk3_dev_debug("buf[0] = 0x%08x", buf[0]);
	yk3_dev_debug("buf[1] = 0x%08x", buf[1]);
	yk3_dev_debug("buf[2] = 0x%08x", buf[2]);
	yk3_dev_debug("buf[3] = 0x%08x", buf[3]);

	for (i = 0; i < 15; i++)
		yk3_dev_debug("p[%d] = 0x%02x", i, p[i]);
#endif

	return 0;
}

static struct action_entry parse_action(struct action_entry_raw ent)
{
#ifdef PPP_DEBUG
	int i;
#endif
	struct action_entry ret = {.sysmeta.raw = {0, 0, 0, 0, 0, 0}, .chn.raw = 0};

	/* parse sysmeta modify action */
	ret.sysmeta.raw[0] = (FIELD_GET(GENMASK(4, 0), ent.raw[1]) << 8) + ent.raw[0];
	ret.sysmeta.raw[1] = (FIELD_GET(GENMASK(1, 0), ent.raw[3]) << 11) + (ent.raw[2] << 3) +
			     FIELD_GET(GENMASK(7, 5), ent.raw[1]);
	ret.sysmeta.raw[2] = (FIELD_GET(GENMASK(6, 0), ent.raw[4]) << 6) +
			     FIELD_GET(GENMASK(7, 2), ent.raw[3]);
	ret.sysmeta.raw[3] = (FIELD_GET(GENMASK(3, 0), ent.raw[6]) << 9) + (ent.raw[5] << 1) +
			     FIELD_GET(GENMASK(7, 7), ent.raw[4]);
	ret.sysmeta.raw[4] = (FIELD_GET(GENMASK(0, 0), ent.raw[8]) << 12)  + (ent.raw[7] << 4) +
			     FIELD_GET(GENMASK(7, 4), ent.raw[6]);
	ret.sysmeta.raw[5] = (FIELD_GET(GENMASK(5, 0), ent.raw[9]) << 7) +
			     FIELD_GET(GENMASK(7, 1), ent.raw[8]);
	/* parse channel action */
	ret.chn.ma0_vport = (FIELD_GET(GENMASK(1, 0), ent.raw[10]) << 2) +
			    FIELD_GET(GENMASK(7, 6), ent.raw[9]);
	ret.chn.ma0_p     = FIELD_GET(GENMASK(2, 2), ent.raw[10]);
	ret.chn.ma0_pri   = FIELD_GET(GENMASK(5, 3), ent.raw[10]);
	ret.chn.ma0_v     = FIELD_GET(GENMASK(6, 6), ent.raw[10]);
	ret.chn.ma1_vport = (FIELD_GET(GENMASK(2, 0), ent.raw[11]) << 1) +
			    FIELD_GET(GENMASK(7, 7), ent.raw[10]);
	ret.chn.ma1_p     = FIELD_GET(GENMASK(3, 3), ent.raw[11]);
	ret.chn.ma1_pri   = FIELD_GET(GENMASK(6, 4), ent.raw[11]);
	ret.chn.ma1_v     = FIELD_GET(GENMASK(7, 7), ent.raw[11]);
	ret.chn.ma2_vport = FIELD_GET(GENMASK(3, 0), ent.raw[12]);
	ret.chn.ma2_p     = FIELD_GET(GENMASK(4, 4), ent.raw[12]);
	ret.chn.ma2_pri   = FIELD_GET(GENMASK(7, 5), ent.raw[12]);
	ret.chn.ma2_v     = FIELD_GET(GENMASK(0, 0), ent.raw[13]);
	ret.chn.mir_idx   = (FIELD_GET(GENMASK(0, 0), ent.raw[14]) << 7) +
			    FIELD_GET(GENMASK(7, 1), ent.raw[13]);
	ret.chn.mir_vport = FIELD_GET(GENMASK(2, 1), ent.raw[14]);
	ret.chn.mir_p     = FIELD_GET(GENMASK(3, 3), ent.raw[14]);
	ret.chn.mir_pri   = FIELD_GET(GENMASK(6, 4), ent.raw[14]);
	ret.chn.mir_v     = FIELD_GET(GENMASK(7, 7), ent.raw[14]);
#ifdef PPP_DEBUG
	yk3_debug("--------------------BBBB--------------------");

	for (i = 0; i < 6; i++)
		yk3_debug("v:%d pri: %d val:0x%02x",
			  ret.sysmeta.field[i].v,
			  ret.sysmeta.field[i].pri,
			  ret.sysmeta.field[i].val);

	yk3_debug("chn: 0x%016llx", ret.chn.raw);
	yk3_debug("ma0_vport: %d",  ret.chn.ma0_vport);
	yk3_debug("ma0_p: %d",      ret.chn.ma0_p);
	yk3_debug("ma0_pri: %d",    ret.chn.ma0_pri);
	yk3_debug("ma0_v: %d",      ret.chn.ma0_v);
	yk3_debug("ma1_vport: %d",  ret.chn.ma1_vport);
	yk3_debug("ma1_p: %d",      ret.chn.ma1_p);
	yk3_debug("ma1_pri: %d",    ret.chn.ma1_pri);
	yk3_debug("ma1_v: %d",      ret.chn.ma1_v);
	yk3_debug("ma2_vport: %d",  ret.chn.ma2_vport);
	yk3_debug("ma2_p: %d",      ret.chn.ma2_p);
	yk3_debug("ma2_pri: %d",    ret.chn.ma2_pri);
	yk3_debug("ma2_v: %d",      ret.chn.ma2_v);
	yk3_debug("mir_idx: %d",    ret.chn.mir_idx);
	yk3_debug("mir_vport: %d",  ret.chn.mir_vport);
	yk3_debug("mir_p: %d",      ret.chn.mir_p);
	yk3_debug("mir_pri: %d",    ret.chn.mir_pri);
	yk3_debug("mir_v: %d",      ret.chn.mir_v);
	yk3_debug("unused: %d",     ret.chn.unused);

	yk3_debug("--------------------EEEE--------------------");
#endif
	return ret;
}

static struct action_entry_raw deparse_action(struct action_entry act)
{
#ifdef PPP_DEBUG
	int i;
#endif
	struct action_entry_raw ret = {{0}};

	ret.raw[0]  = FIELD_GET(GENMASK(7, 0), act.sysmeta.raw[0]);
	ret.raw[1]  = (FIELD_GET(GENMASK(2, 0), act.sysmeta.raw[1]) << 5) +
		      FIELD_GET(GENMASK(12, 8), act.sysmeta.raw[0]);
	ret.raw[2]  = FIELD_GET(GENMASK(10, 3), act.sysmeta.raw[1]);
	ret.raw[3]  = (FIELD_GET(GENMASK(5, 0), act.sysmeta.raw[2]) << 2) +
		      FIELD_GET(GENMASK(12, 11), act.sysmeta.raw[1]);
	ret.raw[4]  = (FIELD_GET(GENMASK(0, 0), act.sysmeta.raw[3]) << 7) +
		      FIELD_GET(GENMASK(12, 6), act.sysmeta.raw[2]);
	ret.raw[5]  = FIELD_GET(GENMASK(8, 1), act.sysmeta.raw[3]);
	ret.raw[6]  = (FIELD_GET(GENMASK(3, 0), act.sysmeta.raw[4]) << 4) +
		      FIELD_GET(GENMASK(12, 9), act.sysmeta.raw[3]);
	ret.raw[7]  = FIELD_GET(GENMASK(11, 4), act.sysmeta.raw[4]);
	ret.raw[8]  = (FIELD_GET(GENMASK(6, 0), act.sysmeta.raw[5]) << 1) +
		      FIELD_GET(GENMASK(12, 12), act.sysmeta.raw[4]);
	ret.raw[9]  = (FIELD_GET(GENMASK(1, 0), act.chn.raw) << 6) +
		      (FIELD_GET(GENMASK(12, 7), act.sysmeta.raw[5]));
	ret.raw[10] = FIELD_GET(GENMASK(9, 2), act.chn.raw);
	ret.raw[11] = FIELD_GET(GENMASK(17, 10), act.chn.raw);
	ret.raw[12] = FIELD_GET(GENMASK(25, 18), act.chn.raw);
	ret.raw[13] = FIELD_GET(GENMASK(33, 26), act.chn.raw);
	ret.raw[14] = FIELD_GET(GENMASK(41, 34), act.chn.raw);
#ifdef PPP_DEBUG
	for (i = 0; i < 15; i++)
		yk3_debug("action_entry_raw[%d]=0x%02x", i, ret.raw[i]);
#endif
	return ret;
}

static struct ivar parse_ivar(struct action_entry_raw ent)
{
#ifdef PPP_DEBUG
	int i;
#endif
	struct ivar ret = {0};

	ret.code = ent.raw[14];
	ret.type = ent.raw[13];
	ret.length = ent.raw[12];
	memcpy(&ret.value, &ent.raw, 12);

#ifdef PPP_DEBUG
	yk3_debug("parserd ivar: ");
	yk3_debug("code: 0x%02x", ret.code);
	yk3_debug("type: 0x%02x", ret.type);
	yk3_debug("length: 0x%02x", ret.length);
	yk3_debug("value:");
	for (i = 0; i < 12; i++)
		yk3_debug("[%d]=0x%02x", i, ret.value[i]);
#endif
	return ret;
}

static struct action_entry_raw deparse_ivar(struct ivar ivar)
{
#ifdef PPP_DEBUG
	int i;
#endif
	struct action_entry_raw ret = {{0}};

	ret.raw[14] = ivar.code;
	ret.raw[13] = ivar.type;
	ret.raw[12] = ivar.length;
	memcpy(&ret.raw, &ivar.value, 12);

#ifdef PPP_DEBUG
	yk3_debug("deparserd ivar: ");
	for (i = 0; i < 15; i++)
		yk3_debug("[%d]=0x%02x", i, ret.raw[i]);
#endif

	return ret;
}

static void load_all_ivar(struct yk3_pdev_priv *pdev_priv, u8 ppp_id)
{
	int i;
	u32 addr, len;
	struct action_entry_raw raw = {{0}};

	addr = ivar_section[ppp_id].addr;
	len  = ivar_section[ppp_id].len;

	for (i = addr; i < addr + len; i++) {
		raw = action_entry_read(pdev_priv, ppp_id, i);
		ivar_list[ppp_id][i - addr] = parse_ivar(raw);
#ifdef PPP_DEBUG
		yk3_dev_debug("ivar_list[%s][%d].code  = 0x%02x",
			      ppp_id == PPP_ALL ? "PPP_ALL" : "PPP_LITE",
			      i - addr, ivar_list[ppp_id][i - addr].code);
		yk3_dev_debug("ivar_list[%s][%d].type  = 0x%02x",
			      ppp_id == PPP_ALL ? "PPP_ALL" : "PPP_LITE",
			      i - addr, ivar_list[ppp_id][i - addr].type);
		yk3_dev_debug("ivar_list[%s][%d].length= 0x%02x",
			      ppp_id == PPP_ALL ? "PPP_ALL" : "PPP_LITE",
			      i - addr, ivar_list[ppp_id][i - addr].length);
#endif
	}
}

__maybe_unused
static int ivar_modify(struct yk3_pdev_priv *pdev_priv, u8 ppp_id, u8 type, void *val)
{
	int i;
	struct action_entry_raw ent = {{0}};

	if (ppp_id >= PPP_MAX)
		return -EINVAL;
	if (!val)
		return -EINVAL;

	load_all_ivar(pdev_priv, ppp_id);
	for (i = 0; i < YK3_PPP_IVAR_MAX; i++) {
		if (ivar_list[ppp_id][i].type == type) {
			memcpy(ivar_list[ppp_id][i].value, val, YK3_PPP_IVAR_VALUE_MAX_LEN);
			ent = deparse_ivar(ivar_list[ppp_id][i]);
			action_entry_write(pdev_priv, ppp_id, ivar_section[ppp_id].addr + i, ent);
			return 0;
		}
	}

	return -EPERM;
}

static int ivar_query(struct yk3_pdev_priv *pdev_priv, u8 ppp_id, u8 type, void *val)
{
	int i;

	if (ppp_id >= PPP_MAX)
		return -EINVAL;
	if (!val)
		return -EINVAL;

	load_all_ivar(pdev_priv, ppp_id);
	for (i = 0; i < YK3_PPP_IVAR_MAX; i++) {
		if (ivar_list[ppp_id][i].type == type) {
			memcpy(val, ivar_list[ppp_id][i].value, YK3_PPP_IVAR_VALUE_MAX_LEN);
			return 0;
		}
	}

	return -EPERM;
}

int yk3_ppp_primap_mode_get(struct yk3_pdev_priv *pdev_priv)
{
	void __iomem *addr = NULL;
	u32 val;

	if (!pdev_priv)
		return -EINVAL;

	addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;

	val = yk3_rd32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN0);

	yk3_dev_debug(" PPP_ALL: primap_mode = 0x%08x", val);

	if (val & 0xF)
		return PRIMAP_IP;

	return PRIMAP_VLAN;
}

int yk3_ppp_primap_mode_set(struct yk3_pdev_priv *pdev_priv, u8 primap_mode)
{
	void __iomem *addr = NULL;
	u32 val;

	if (!pdev_priv)
		return -EINVAL;

	addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;

	val = yk3_rd32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN0);

	if (primap_mode == PRIMAP_IP) {
		yk3_wr32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN0,
			 val | (0x1 << pdev_priv->mac->mac_ch));
		yk3_wr32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN1,
			 val | (0x1 << pdev_priv->mac->mac_ch));
		yk3_dev_debug(" PPP_ALL: primap_mode = 0x%08x",
			      val | (0x1 << pdev_priv->mac->mac_ch));
	} else {
		yk3_wr32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN0,
			 val & ~(0x1 << pdev_priv->mac->mac_ch));
		yk3_wr32(addr, YK3_PPP_CFG_GPR0_BITMIN_CTR_CHAN1,
			 val & ~(0x1 << pdev_priv->mac->mac_ch));
		yk3_dev_debug(" PPP_ALL: primap_mode = 0x%08x",
			      val & ~(0x1 << pdev_priv->mac->mac_ch));
	}

	return 0;
}

int yk3_ppp_primap_dscp_map_set(struct yk3_pdev_priv *pdev_priv, u8 *primap)
{
	void __iomem *addr = NULL;
	int i;

	if (!pdev_priv || !pdev_priv->ppp || !pdev_priv->mac)
		return -EINVAL;

	addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;

	if (pdev_priv->mac->mac_ch == YK3_MAC_CH_0) { /* channel 0 */
		yk3_wr32(addr, YK3_PPP_PKTTX0_PRI_WADDR, 0);
		for (i = 0; i < 256; i++)
			yk3_wr32(addr, YK3_PPP_PKTTX0_PRI_DATA, primap[i % 64]);
	}

	if (pdev_priv->mac->mac_ch == YK3_MAC_CH_1) { /* channel 1 */
		yk3_wr32(addr, YK3_PPP_PKTTX1_PRI_WADDR, 0);
		for (i = 0; i < 256; i++)
			yk3_wr32(addr, YK3_PPP_PKTTX1_PRI_DATA, primap[i % 64]);
	}

	return 0;
}

int yk3_ppp_primap_dscp_map_get(struct yk3_pdev_priv *pdev_priv, u8 *primap)
{
	void __iomem *addr = NULL;
	int i;

	if (!pdev_priv || !pdev_priv->ppp || !pdev_priv->mac)
		return -EINVAL;

	addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;

	if (pdev_priv->mac->mac_ch == YK3_MAC_CH_0) { /* channel 0 */
		yk3_wr32(addr, YK3_PPP_PTTTX0_PRI_RADDR, 0);
		for (i = 0; i < 64; i++)
			primap[i] = yk3_rd32(addr, YK3_PPP_PKTTX0_PRI_DATA);
	}

	if (pdev_priv->mac->mac_ch == YK3_MAC_CH_1) { /* channel 1 */
		yk3_wr32(addr, YK3_PPP_PTTTX1_PRI_RADDR, 0);
		for (i = 0; i < 64; i++)
			primap[i] = yk3_rd32(addr, YK3_PPP_PKTTX1_PRI_DATA);
	}

	return 0;
}

int yk3_ppp_action_modify(struct yk3_pdev_priv *pdev_priv, u8 act_type, u16 act_id, void *val)
{
	struct action_entry_raw raw = {{0}};
	struct action_entry_raw new = {{0}};
	struct action_entry act = {.sysmeta.raw = {0, 0, 0, 0, 0, 0}, .chn.raw = 0};
	int old = 0;
	struct yk3_ppp_rdma_val old_rdma_val = {0};

	if (!pdev_priv || !val)
		return -EINVAL;

	switch (act_type) {
	case YK3_PPP_PTP_ACTION:
		raw = action_entry_read(pdev_priv, PPP_ALL, YK3_PPP_ACTION_PTP);
		act = parse_action(raw);
		old = act.sysmeta.field[3].val;
		act.sysmeta.field[3].val = ((u8 *)val)[0];
		act.sysmeta.field[3].v = 1;
		new = deparse_action(act);
		action_entry_write(pdev_priv, PPP_ALL, YK3_PPP_ACTION_PTP, new);
		yk3_dev_debug("PPP PTP Action Modified! old Pri: %d, new Pri : %d",
			      old, act.sysmeta.field[3].val);
		break;
	case YK3_PPP_RDMA_RTDEST:
		raw = action_entry_read(pdev_priv, PPP_ALL, act_id);
		act = parse_action(raw);
		old_rdma_val.vport = act.chn.ma0_vport;
		old_rdma_val.rtdest = act.sysmeta.field[1].val;
		act.chn.ma0_vport = ((struct yk3_ppp_rdma_val *)val)->vport;
		act.chn.ma0_p = 1;
		act.chn.ma0_v = 1;
		act.sysmeta.field[1].val = ((struct yk3_ppp_rdma_val *)val)->rtdest;
		act.sysmeta.field[1].v = 1;
		new = deparse_action(act);
		action_entry_write(pdev_priv, PPP_ALL, act_id, new);
		yk3_dev_debug("PPP RDMA_RTDEST Action Modified! vport %d -> %d, rtdest %d -> %d",
			      old_rdma_val.vport, act.chn.ma0_vport,
			      old_rdma_val.rtdest, act.sysmeta.field[1].val);
		break;
	case YK3_PPP_RDMA_QBASE:
		raw = action_entry_read(pdev_priv, PPP_ALL, act_id);
		act = parse_action(raw);
		old_rdma_val.vport = act.chn.ma0_vport;
		old_rdma_val.qbase = act.sysmeta.field[2].val;
		act.chn.ma0_vport = ((struct yk3_ppp_rdma_val *)val)->vport;
		act.chn.ma0_p = 1;
		act.chn.ma0_v = 1;
		act.sysmeta.field[2].val = ((struct yk3_ppp_rdma_val *)val)->qbase;
		act.sysmeta.field[2].v = 1;
		new = deparse_action(act);
		action_entry_write(pdev_priv, PPP_ALL, act_id, new);
		yk3_dev_debug("PPP RDMA_QBASE Action Modified! vport %d -> %d, qbase %d -> %d",
			      old_rdma_val.vport, act.chn.ma0_vport,
			      old_rdma_val.qbase, act.sysmeta.field[2].val);
		break;
	default:
		yk3_dev_err("unsupported ppp action type!");
		return -EINVAL;
	}

	return 0;
}

int yk3_ppp_action_query(struct yk3_pdev_priv *pdev_priv, u8 act_type, u16 act_id, void *val)
{
	struct action_entry_raw raw = {{0}};
	struct action_entry act = {.sysmeta.raw = {0, 0, 0, 0, 0, 0}, .chn.raw = 0};

	if (!pdev_priv || !val)
		return -EINVAL;

	switch (act_type) {
	case YK3_PPP_PTP_ACTION:
		raw = action_entry_read(pdev_priv, PPP_ALL, YK3_PPP_ACTION_PTP);
		act = parse_action(raw);
		((u8 *)val)[0] = act.sysmeta.field[3].val;
		yk3_dev_debug("PPP PTP Action query! Pri:%d", act.sysmeta.field[3].val);
		break;
	case YK3_PPP_RDMA_RTDEST:
		raw = action_entry_read(pdev_priv, PPP_ALL, act_id);
		act = parse_action(raw);
		((struct yk3_ppp_rdma_val *)val)->vport = act.chn.ma0_vport;
		((struct yk3_ppp_rdma_val *)val)->rtdest = act.sysmeta.field[1].val;
		yk3_dev_debug("PPP RDMA_RTDEST Action query!");
		break;
	case YK3_PPP_RDMA_QBASE:
		raw = action_entry_read(pdev_priv, PPP_ALL, act_id);
		act = parse_action(raw);
		((struct yk3_ppp_rdma_val *)val)->vport = act.chn.ma0_vport;
		((struct yk3_ppp_rdma_val *)val)->qbase = act.sysmeta.field[2].val;
		yk3_dev_debug("PPP RDMA_QBASE Action query!");
		break;
	default:
		yk3_dev_err("unsupported ppp action type!");
		return -EINVAL;
	}

	return 0;
}

static int ppp_np_rdma_switch_change(struct yk3_pdev_priv *pdev_priv, bool enable)
{
	int ret = 0;
	u32 vliw_offset = 0;
	u32 branch_id = 0;
	u8 val[YK3_PPP_IVAR_VALUE_MAX_LEN] = {0};
	void __iomem *addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;
	int i;
	struct vliw vliw = {{{0}}};

/*
 * NP RDMA动态使能开关，type（0x30），branch（3）
 * PD_DEF,Ox30,NP_RDMA_ENABLE
 *     PDU8,0x01			value[11]
 *     PSR_BRANCH_U16,udp[1], 3		udp[1] = vliw_offset = value[10] | branch_id = 3 = value[9]
 * End
 */
	ret = ivar_query(pdev_priv, PPP_ALL, IVAR_NP_RDMA_ENABLE, &val);
	if (ret)
		return ret;

	// | PDU8 ||offset|branch|
	// | val11|| val10|  val9|
	vliw_offset = val[YK3_PPP_IVAR_VALUE_MAX_LEN - 2];
	branch_id = val[YK3_PPP_IVAR_VALUE_MAX_LEN - 3];
	yk3_dev_debug("IVAR rdma_switch: vliw_off = %d, branch_id = %d", vliw_offset, branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_RADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		vliw.raw[i] = yk3_rd32(addr, YK3_PPP_PSR_VLIW_DATA);
/*
 * BRANCH 0 valid : bit 288
 * BRANCH 1 valid : bit 289
 * BRANCH 2 valid : bit 290
 * BRANCH 3 valid : bit 291
 */
	yk3_dev_debug("read from hw BRANCH valid: 0x%x", vliw.raw[9] & 0xF);

	if (enable)
		vliw.raw[9] |= BIT(branch_id);
	else
		vliw.raw[9] &= ~BIT(branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_WADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		yk3_wr32(addr, YK3_PPP_PSR_VLIW_DATA, vliw.raw[i]);
	yk3_dev_debug("write to hw BRANCH valid: 0x%x", vliw.raw[9] & 0xF);
	return 0;
}

int yk3_ppp_rdma_enable(struct yk3_pdev_priv *pdev_priv)
{
	if (!pdev_priv)
		return -EINVAL;

	return yk3_ppp_ivar_modify(pdev_priv, IVAR_NP_RDMA_ENABLE, (void *)true);
}

int yk3_ppp_rdma_disable(struct yk3_pdev_priv *pdev_priv)
{
	if (!pdev_priv)
		return -EINVAL;

	return yk3_ppp_ivar_modify(pdev_priv, IVAR_NP_RDMA_ENABLE, (void *)false);
}

static int vxlan_custom_port_set(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val)
{
	int ret = 0;
	u32 vliw_offset = 0;
	u32 branch_id = 0;
	u8 tmp[YK3_PPP_IVAR_VALUE_MAX_LEN] = {0};
	void __iomem *addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;
	int i;
	struct vliw vliw = {{{0}}};

	ret = ivar_query(pdev_priv, PPP_ALL, ivar_type, &tmp);
	if (ret)
		return ret;

	vliw_offset = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 2];
	branch_id = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 3];
	yk3_dev_debug("IVAR vxlan: vliw_off = %d, branch_id = %d", vliw_offset, branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_RADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		vliw.raw[i] = yk3_rd32(addr, YK3_PPP_PSR_VLIW_DATA);

	vliw.branch[branch_id].protocol_num = ((struct yk3_ppp_vxlan_val *)val)->protocol_num;
	vliw.branch[branch_id].mask = ((struct yk3_ppp_vxlan_val *)val)->mask;

	if (((struct yk3_ppp_vxlan_val *)val)->enable)
		vliw.raw[9] |= BIT(branch_id);
	else
		vliw.raw[9] &= ~BIT(branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_WADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		yk3_wr32(addr, YK3_PPP_PSR_VLIW_DATA, vliw.raw[i]);
	yk3_dev_debug("write to hw vxlan custom port: protocol_num = 0x%x, mask = 0x%x",
		      vliw.branch[branch_id].protocol_num, vliw.branch[branch_id].mask);
	return 0;
}

static int vxlan_custom_port_get(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val)
{
	int ret = 0;
	u32 vliw_offset = 0;
	u32 branch_id = 0;
	u8 tmp[YK3_PPP_IVAR_VALUE_MAX_LEN] = {0};
	void __iomem *addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;
	int i;
	struct vliw vliw = {{{0}}};

	ret = ivar_query(pdev_priv, PPP_ALL, ivar_type, &tmp);
	if (ret)
		return ret;

	vliw_offset = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 2];
	branch_id = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 3];
	yk3_dev_debug("IVAR vxlan: vliw_off = %d, branch_id = %d", vliw_offset, branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_RADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		vliw.raw[i] = yk3_rd32(addr, YK3_PPP_PSR_VLIW_DATA);

	((struct yk3_ppp_vxlan_val *)val)->enable = (vliw.raw[9] & BIT(branch_id)) ? 1 : 0;
	((struct yk3_ppp_vxlan_val *)val)->protocol_num = vliw.branch[branch_id].protocol_num;
	((struct yk3_ppp_vxlan_val *)val)->mask = vliw.branch[branch_id].mask;

	return 0;
}

static int geneve_custom_port_set(struct yk3_pdev_priv *pdev_priv, void *val)
{
	int ret = 0;
	u32 vliw_offset = 0;
	u32 branch_id = 0;
	u8 tmp[YK3_PPP_IVAR_VALUE_MAX_LEN] = {0};
	void __iomem *addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;
	int i;
	struct vliw vliw = {{{0}}};

	ret = ivar_query(pdev_priv, PPP_ALL, IVAR_GENEVE_PORT, &tmp);
	if (ret)
		return ret;

	vliw_offset = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 2];
	branch_id = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 3];
	yk3_dev_debug("IVAR geneve: vliw_off = %d, branch_id = %d", vliw_offset, branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_RADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		vliw.raw[i] = yk3_rd32(addr, YK3_PPP_PSR_VLIW_DATA);

	vliw.branch[branch_id].protocol_num = ((struct yk3_ppp_geneve_val *)val)->protocol_num;
	vliw.branch[branch_id].mask = ((struct yk3_ppp_geneve_val *)val)->mask;

	if (((struct yk3_ppp_geneve_val *)val)->enable)
		vliw.raw[9] |= BIT(branch_id);
	else
		vliw.raw[9] &= ~BIT(branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_WADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		yk3_wr32(addr, YK3_PPP_PSR_VLIW_DATA, vliw.raw[i]);
	yk3_dev_debug("write to hw geneve custom port: protocol_num = 0x%x, mask = 0x%x",
		      vliw.branch[branch_id].protocol_num, vliw.branch[branch_id].mask);
	return 0;
}

static int geneve_custom_port_get(struct yk3_pdev_priv *pdev_priv, void *val)
{
	int ret = 0;
	u32 vliw_offset = 0;
	u32 branch_id = 0;
	u8 tmp[YK3_PPP_IVAR_VALUE_MAX_LEN] = {0};
	void __iomem *addr = pdev_priv->ppp->addr + YK3_PPP_ALL_BASE;
	int i;
	struct vliw vliw = {{{0}}};

	ret = ivar_query(pdev_priv, PPP_ALL, IVAR_GENEVE_PORT, &tmp);
	if (ret)
		return ret;

	vliw_offset = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 2];
	branch_id = tmp[YK3_PPP_IVAR_VALUE_MAX_LEN - 3];
	yk3_dev_debug("IVAR geneve: vliw_off = %d, branch_id = %d", vliw_offset, branch_id);

	yk3_wr32(addr, YK3_PPP_PSR_VLIW_RADDR, vliw_offset);
	for (i = 0; i < 11; i++)
		vliw.raw[i] = yk3_rd32(addr, YK3_PPP_PSR_VLIW_DATA);

	((struct yk3_ppp_geneve_val *)val)->enable = (vliw.raw[9] & BIT(branch_id)) ? 1 : 0;
	((struct yk3_ppp_geneve_val *)val)->protocol_num = vliw.branch[branch_id].protocol_num;
	((struct yk3_ppp_geneve_val *)val)->mask = vliw.branch[branch_id].mask;

	return 0;
}

int yk3_ppp_ivar_modify(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val)
{
	switch (ivar_type) {
	case IVAR_VXLAN_PORT_0:
	case IVAR_VXLAN_PORT_1:
	case IVAR_VXLAN_PORT_2:
	case IVAR_VXLAN_PORT_3:
		return vxlan_custom_port_set(pdev_priv, ivar_type, val);
	case IVAR_GENEVE_PORT:
		return geneve_custom_port_set(pdev_priv, val);
	case IVAR_NP_RDMA_ENABLE:
		return ppp_np_rdma_switch_change(pdev_priv, (bool)val);
	default:
		break;
	}
	return 0;
}

int yk3_ppp_ivar_query(struct yk3_pdev_priv *pdev_priv, u8 ivar_type, void *val)
{
	switch (ivar_type) {
	case IVAR_VXLAN_PORT_0:
	case IVAR_VXLAN_PORT_1:
	case IVAR_VXLAN_PORT_2:
	case IVAR_VXLAN_PORT_3:
		return vxlan_custom_port_get(pdev_priv, ivar_type, val);
	case IVAR_GENEVE_PORT:
		return geneve_custom_port_get(pdev_priv, val);
	default:
		break;
	}
	return 0;
}

int yk3_ppp_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ppp *ppp;
	u32 reg;
	u32 vld;
	u32 addr, len;
	u8 default_primap[64] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7
	};
	struct yk3_ppp_vxlan_val vxlan_val = {
		.enable = 0,
		.protocol_num = 4789,
		.mask = 0xffff
	};
	struct yk3_ppp_geneve_val geneve_val = {
		.enable = 0,
		.protocol_num = 6081,
		.mask = 0xffff
	};
	int ret = 0;

	ppp = kzalloc(sizeof(*ppp), GFP_KERNEL);
	if (!ppp) {
		ret = -ENOMEM;
		goto err_out;
	}

	ppp->addr = (void __iomem *)pdev_priv->bar_addr[YK3_PPP_BAR];
	pdev_priv->ppp = ppp;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		/* interactive variable */
		/* PPP_ALL */
		reg  = yk3_rd32(ppp->addr, YK3_PPP_ALL_BASE + YK3_PPP_IVAR_SECTION);
		vld  = (reg >> 31) & 0x1;      // bit[31]
		addr = (reg >> 8) & 0xFFFF;    // bit[23:8]
		len  = reg & 0xFF;             // bit[7:0]
		if (!vld) {
			yk3_dev_err("PPP_ALL IVAR not valid!");
			ret = -EBUSY;
			goto err_out;
		}

		ivar_section[PPP_ALL].addr = addr;
		ivar_section[PPP_ALL].len = len;
		yk3_dev_debug("ppp_all ivar section: %d - %d", ivar_section[PPP_ALL].addr,
			      ivar_section[PPP_ALL].addr + len - 1);

		load_all_ivar(pdev_priv, PPP_ALL);

		/* PPP_LITE */
		reg  = yk3_rd32(ppp->addr, YK3_PPP_LITE_BASE + YK3_PPP_IVAR_SECTION);
		vld  = (reg >> 31) & 0x1;      // bit[31]
		addr = (reg >> 8) & 0xFFFF;    // bit[23:8]
		len  = reg & 0xFF;             // bit[7:0]

		if (!vld) {
			yk3_dev_err("PPP_LITE IVAR not valid!");
			ret = -EBUSY;
			goto err_out;
		}

		ivar_section[PPP_LITE].addr = addr;
		ivar_section[PPP_LITE].len = len;
		yk3_dev_debug("ppp_lite ivar section: %d - %d", ivar_section[PPP_LITE].addr,
			      ivar_section[PPP_LITE].addr + len - 1);

		load_all_ivar(pdev_priv, PPP_LITE);

		yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_0, &vxlan_val);
		yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_1, &vxlan_val);
		yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_2, &vxlan_val);
		yk3_ppp_ivar_modify(pdev_priv, IVAR_VXLAN_PORT_3, &vxlan_val);
		yk3_ppp_ivar_modify(pdev_priv, IVAR_GENEVE_PORT, &geneve_val);

		yk3_dev_debug("%s mgr ok!", __func__);
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		/* priority map */
		yk3_ppp_primap_mode_set(pdev_priv, PRIMAP_IP);
		yk3_ppp_primap_dscp_map_set(pdev_priv, (u8 *)&default_primap);
		yk3_dev_debug("%s pf ok!", __func__);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		yk3_dev_debug("%s vf ok!", __func__);
	}

	ret = yk3_ppp_eme_init(pdev_priv);
	if (ret)
		goto err_out;

	return 0;

err_out:
	kfree(ppp);
	pdev_priv->ppp = NULL;
	return ret;
}

void yk3_ppp_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ppp *ppp = pdev_priv->ppp;

	yk3_ppp_eme_uninit(pdev_priv);
	kfree(ppp);
	pdev_priv->ppp = NULL;
}
