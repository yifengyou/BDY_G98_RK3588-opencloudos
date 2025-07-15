// SPDX-License-Identifier: BSD-3-Clause
/* Copyright(c) 2024 Broadcom
 * All rights reserved.
 */

#include <linux/vmalloc.h>
#include <linux/if_ether.h>
#include <linux/atomic.h>
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <linux/err.h>
#include <linux/limits.h>
#include "bnxt_compat.h"
#include "bnxt_hsi.h"
#include "bnxt.h"
#include "bnxt_vfr.h"
#include "bnxt_tf_ulp.h"
#include "bnxt_udcc.h"
#include "tfc.h"
#include "tfc_util.h"
#include "ulp_nic_flow.h"
#include "ulp_generic_flow_offload.h"

#if defined(CONFIG_BNXT_FLOWER_OFFLOAD)

static int roce_macvlan_flow_create(struct bnxt *bp,
				    u32 *flow_id,
				    bool is_v6, u8 *mac_addr, enum cfa_dir dir)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	struct bnxt_ulp_gen_eth_hdr eth_spec = { 0 };
	struct bnxt_ulp_gen_eth_hdr eth_mask = { 0 };
	u16 etype_spec = is_v6 ? cpu_to_be16(ETH_P_IPV6) :
		cpu_to_be16(ETH_P_IP);
	u16 etype_mask = cpu_to_be16(0xffff);
	u8 dst_spec[ETH_ALEN] = { 0 };
	u8 dst_mask[ETH_ALEN] = { 0 };
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	ether_addr_copy(dst_spec, mac_addr);
	eth_broadcast_addr(dst_mask);
	eth_spec.dst = &dst_spec[0];
	eth_mask.dst = &dst_mask[0];
	eth_spec.type = &etype_spec;
	eth_mask.type = &etype_mask;
	l2_parms.eth_spec = &eth_spec;
	l2_parms.eth_mask = &eth_mask;
	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	bth_spec.op_code = NULL;
	bth_mask.op_code = NULL;
	bth_spec.dst_qpn = NULL;
	bth_mask.dst_qpn = NULL;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT_LOOPBACK;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;
	parms.flow_id = flow_id;
	parms.counter_hndl = NULL;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;

	if (dir == CFA_DIR_RX) {
		parms.lkup_strength = FLOW_LKUP_STRENGTH_LO;
		parms.priority = ULP_NIC_FLOW_ROCE_PRI;
	} else {
		parms.lkup_strength = FLOW_LKUP_STRENGTH_HI;
		parms.priority = ULP_NIC_FLOW_ROCE_LPBK_PRI;
	}
	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: NIC FLOW MACVLAN (%s) (%s) add flow_id: %d\n",
		   __func__, (is_v6 ? "v6" : "v4"), tfc_dir_2_str(dir),
		   *flow_id);
	return rc;
}

static int roce_lpbk_flow_create(struct bnxt *bp,
				 u32 *flow_id, u64 *flow_cnt_hndl,
				 bool is_v6, enum cfa_dir dir)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	struct bnxt_ulp_gen_eth_hdr eth_spec = { 0 };
	struct bnxt_ulp_gen_eth_hdr eth_mask = { 0 };
	u16 etype_spec = is_v6 ? cpu_to_be16(ETH_P_IPV6) :
		cpu_to_be16(ETH_P_IP);
	u16 etype_mask = cpu_to_be16(0xffff);
	u8 dst_spec[ETH_ALEN] = { 0 };
	u8 dst_mask[ETH_ALEN] = { 0 };
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	ether_addr_copy(dst_spec, bp->dev->dev_addr);
	eth_broadcast_addr(dst_mask);
	eth_spec.dst = &dst_spec[0];
	eth_mask.dst = &dst_mask[0];
	eth_spec.type = &etype_spec;
	eth_mask.type = &etype_mask;
	l2_parms.eth_spec = &eth_spec;
	l2_parms.eth_mask = &eth_mask;
	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	bth_spec.op_code = NULL;
	bth_mask.op_code = NULL;
	bth_spec.dst_qpn = NULL;
	bth_mask.dst_qpn = NULL;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT_LOOPBACK;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;
	parms.flow_id = flow_id;
	parms.counter_hndl = flow_cnt_hndl;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;

	/* if DMAC is our PF mac then we just send all these packets to LPBK */
	parms.lkup_strength = FLOW_LKUP_STRENGTH_HI;
	parms.priority = ULP_NIC_FLOW_ROCE_LPBK_PRI;
	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: NIC FLOW ROCE LPBK (%s) (%s) add flow_id: %d, ctr: 0x%llx\n",
		   __func__,
		   (is_v6 ? "v6" : "v4"),
		   tfc_dir_2_str(dir),
		   *flow_id,
		   *flow_cnt_hndl);
	return rc;
}

static int roce_flow_create(struct bnxt *bp,
			    u32 *flow_id, u64 *flow_cnt_hndl,
			    bool is_v6, enum cfa_dir dir)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	bth_spec.op_code = NULL;
	bth_mask.op_code = NULL;
	bth_spec.dst_qpn = NULL;
	bth_mask.dst_qpn = NULL;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;
	parms.flow_id = flow_id;
	parms.counter_hndl = flow_cnt_hndl;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;

	/* must be lower priority than ROCE CNP */
	parms.lkup_strength = FLOW_LKUP_STRENGTH_LO;
	parms.priority = ULP_NIC_FLOW_ROCE_PRI;
	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: NIC FLOW ROCE(%s) (%s) add flow_id: %d, ctr: 0x%llx\n",
		   __func__,
		   (is_v6 ? "v6" : "v4"),
		   tfc_dir_2_str(dir),
		   *flow_id,
		   *flow_cnt_hndl);
	return rc;
}

static int roce_cnp_flow_create(struct bnxt *bp,
				u32 *cnp_flow_id, u64 *cnp_flow_cnt_hndl,
				bool is_v6, enum cfa_dir dir)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	u16 op_code = cpu_to_be16(0x81); /* RoCE CNP */
	u16 op_code_mask = cpu_to_be16(0xffff);
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	bth_spec.op_code = &op_code;
	bth_mask.op_code = &op_code_mask;
	bth_spec.dst_qpn = NULL;
	bth_mask.dst_qpn = NULL;
	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;

	parms.flow_id = cnp_flow_id;
	parms.counter_hndl = cnp_flow_cnt_hndl;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;
	/* must be higher priority than ROCE non-CNP */
	if (!BNXT_UDCC_DCQCN_EN(bp))
		parms.lkup_strength = FLOW_LKUP_STRENGTH_HI;
	parms.priority = ULP_NIC_FLOW_ROCE_CNP_PRI;

	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: ROCE(%s) CNP(%s) flow_id: %d, ctr: 0x%llx\n",
		   __func__,
		   (is_v6 ? "v6" : "v4"),
		   tfc_dir_2_str(dir),
		   *cnp_flow_id,
		   *cnp_flow_cnt_hndl);

	return rc;
}

static int roce_ack_flow_create(struct bnxt *bp,
				u32 *cnp_flow_id, u64 *cnp_flow_cnt_hndl,
				bool is_v6, enum cfa_dir dir)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	u16 op_code = cpu_to_be16(0x11); /* RoCE ACK */
	u16 op_code_mask = cpu_to_be16(0xffff);
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	bth_spec.op_code = &op_code;
	bth_mask.op_code = &op_code_mask;
	bth_spec.dst_qpn = NULL;
	bth_mask.dst_qpn = NULL;
	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;

	parms.flow_id = cnp_flow_id;
	parms.counter_hndl = cnp_flow_cnt_hndl;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;

	if (!BNXT_UDCC_DCQCN_EN(bp))
		parms.lkup_strength = FLOW_LKUP_STRENGTH_HI;
	parms.priority = ULP_NIC_FLOW_ROCE_CNP_PRI;

	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: ROCE(%s) CNP ACK(%s) flow_id: %d, ctr: 0x%llx\n",
		   __func__,
		   (is_v6 ? "v6" : "v4"),
		   tfc_dir_2_str(dir),
		   *cnp_flow_id,
		   *cnp_flow_cnt_hndl);

	return rc;
}

static int roce_probe_flow_create(struct bnxt *bp,
				  u32 *cnp_flow_id, u64 *cnp_flow_cnt_hndl,
				  bool is_v6, enum cfa_dir dir, u16 pad_cnt)
{
	struct bnxt_ulp_gen_bth_hdr bth_spec = { 0 }, bth_mask = { 0 };
	struct bnxt_ulp_gen_ipv6_hdr v6_spec = { 0 }, v6_mask = { 0 };
	struct bnxt_ulp_gen_ipv4_hdr v4_spec = { 0 }, v4_mask = { 0 };
	struct bnxt_ulp_gen_l2_hdr_parms l2_parms = { 0 };
	struct bnxt_ulp_gen_l3_hdr_parms l3_parms = { 0 };
	struct bnxt_ulp_gen_l4_hdr_parms l4_parms = { 0 };
	struct bnxt_ulp_gen_action_parms actions = { 0 };
	struct bnxt_ulp_gen_flow_parms parms = { 0 };
	u32 dst_qpn = cpu_to_be32(0x1); /* RoCE QP 1 for PROBE */
	u32 dst_qpn_mask = cpu_to_be32(0xffffffff);
	u16 flags_mask = cpu_to_be16(0x3);
	u16 flags = cpu_to_be16(pad_cnt);
	u8 l4_proto = IPPROTO_UDP;
	u8 l4_proto_mask = 0xff;
	int rc = 0;

	l2_parms.type = BNXT_ULP_GEN_L2_L2_HDR;

	if (is_v6) {
		/* Pack the L3 Data */
		v6_spec.proto6 = &l4_proto;
		v6_mask.proto6 = &l4_proto_mask;
		v6_spec.dip6 = NULL;
		v6_mask.dip6 = NULL;
		v6_spec.sip6 = NULL;
		v6_mask.sip6 = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV6;
		l3_parms.v6_spec = &v6_spec;
		l3_parms.v6_mask = &v6_mask;
	} else {
		/* Pack the L3 Data */
		v4_spec.proto = &l4_proto;
		v4_mask.proto = &l4_proto_mask;
		v4_spec.dip = NULL;
		v4_mask.dip = NULL;
		v4_spec.sip = NULL;
		v4_mask.sip = NULL;

		l3_parms.type = BNXT_ULP_GEN_L3_IPV4;
		l3_parms.v4_spec = &v4_spec;
		l3_parms.v4_mask = &v4_mask;
	}

	/* Pack the L4 Data */
	bth_spec.dst_qpn = &dst_qpn;
	bth_mask.dst_qpn = &dst_qpn_mask;
	bth_spec.op_code = NULL;
	bth_mask.op_code = NULL;
	bth_spec.bth_flags = &flags;
	bth_mask.bth_flags = &flags_mask;

	l4_parms.type = BNXT_ULP_GEN_L4_BTH;
	l4_parms.bth_spec = &bth_spec;
	l4_parms.bth_mask = &bth_mask;

	/* Pack the actions - NIC template will use RoCE VNIC always by default */
	if (dir == CFA_DIR_RX) {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_DROP;
	} else {
		actions.enables = BNXT_ULP_GEN_ACTION_ENABLES_COUNT |
			BNXT_ULP_GEN_ACTION_ENABLES_REDIRECT;
	}
	actions.dst_fid = bp->pf.fw_fid;

	if (dir == CFA_DIR_RX)
		parms.dir = BNXT_ULP_GEN_RX;
	else
		parms.dir = BNXT_ULP_GEN_TX;

	parms.flow_id = cnp_flow_id;
	parms.counter_hndl = cnp_flow_cnt_hndl;
	parms.l2 = &l2_parms;
	parms.l3 = &l3_parms;
	parms.l4 = &l4_parms;
	parms.actions = &actions;
	/* must be higher priority than ROCE non-CNP */
	parms.priority = ULP_NIC_FLOW_ROCE_PROBE_PRI;

	rc = bnxt_ulp_gen_flow_create(bp, bp->pf.fw_fid, &parms);
	if (rc)
		return rc;

	netdev_dbg(bp->dev, "%s: ROCE(%s) PROBE(%s) flow_id: %d, ctr: 0x%llx\n",
		   __func__,
		   (is_v6 ? "v6" : "v4"),
		   tfc_dir_2_str(dir),
		   *cnp_flow_id,
		   *cnp_flow_cnt_hndl);
	return rc;
}

int bnxt_ulp_nic_flows_roce_add(struct bnxt *bp,
				struct ulp_nic_flows *flows,
				enum cfa_dir dir)
{
	bool is_v6;
	int rc;

	if (!flows) {
		netdev_info(bp->dev, "%s bad parameters\n", __func__);
		return -EINVAL;
	}

	memset(flows, 0, sizeof(struct ulp_nic_flows));

	rc = roce_flow_create(bp,
			      &flows->id[NIC_FLOW_TYPE_ROCE_V4],
			      &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V4],
			      is_v6 = false, dir);
	if (rc)
		goto cleanup;
	rc = roce_flow_create(bp,
			      &flows->id[NIC_FLOW_TYPE_ROCE_V6],
			      &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V6],
			      is_v6 = true, dir);
	if (rc)
		goto cleanup;

	rc = roce_cnp_flow_create(bp,
				  &flows->id[NIC_FLOW_TYPE_ROCE_V4_CNP],
				  &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V4_CNP],
				  is_v6 = false, dir);
	if (rc)
		goto cleanup;

	rc = roce_cnp_flow_create(bp,
				  &flows->id[NIC_FLOW_TYPE_ROCE_V6_CNP],
				  &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V6_CNP],
				  is_v6 = true, dir);
	if (rc)
		goto cleanup;

	if (dir == CFA_DIR_RX) {
		rc = bnxt_hwrm_udcc_qcfg(bp);
		if (rc && rc != -EOPNOTSUPP)
			goto cleanup;

		if (!rc && bp->udcc_info->pad_cnt) {
			netdev_dbg(bp->dev, "UDCC probe config pad count 0x%x\n",
				   bp->udcc_info->pad_cnt);
			rc = roce_probe_flow_create(bp,
						    &flows->id[NIC_FLOW_TYPE_ROCE_V4_PROBE],
						    &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V4_PROBE],
						    is_v6 = false, dir,
						    (u16)bp->udcc_info->pad_cnt);
			if (rc)
				goto cleanup;

			rc = roce_probe_flow_create(bp,
						    &flows->id[NIC_FLOW_TYPE_ROCE_V6_PROBE],
						    &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V6_PROBE],
						    is_v6 = true, dir,
						    (u16)bp->udcc_info->pad_cnt);
			if (rc)
				goto cleanup;
		} else {	/* rc == -EOPNOTSUPP || !pad_cnt */
			rc = 0;
		}
	}

	if (dir == CFA_DIR_TX) {
		rc = roce_ack_flow_create(bp,
					  &flows->id[NIC_FLOW_TYPE_ROCE_V4_ACK],
					  &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V4_ACK],
					  is_v6 = false, dir);
		if (rc)
			goto cleanup;

		rc = roce_ack_flow_create(bp,
					  &flows->id[NIC_FLOW_TYPE_ROCE_V6_ACK],
					  &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V6_ACK],
					  is_v6 = true, dir);
		if (rc)
			goto cleanup;

		rc = roce_lpbk_flow_create(bp,
					   &flows->id[NIC_FLOW_TYPE_ROCE_V4_LPBK],
					   &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V4_LPBK],
					   is_v6 = false, dir);
		if (rc)
			goto cleanup;

		rc = roce_lpbk_flow_create(bp,
					   &flows->id[NIC_FLOW_TYPE_ROCE_V6_LPBK],
					   &flows->cnt_hndl[NIC_FLOW_TYPE_ROCE_V6_LPBK],
					   is_v6 = true, dir);
		if (rc)
			goto cleanup;
	}

	return rc;

cleanup:
	bnxt_ulp_nic_flows_roce_del(bp, flows, dir);

	return rc;
}

int bnxt_ulp_nic_flows_roce_del(struct bnxt *bp,
				struct ulp_nic_flows *flows,
				enum cfa_dir dir)
{
	enum ulp_nic_flow_type type;
	int rc_save = 0, rc = 0;

	for (type = 0; type < NIC_FLOW_TYPE_MAX; type++) {
		if (flows->id[type]) {
			rc = bnxt_ulp_gen_flow_destroy(bp, bp->pf.fw_fid, flows->id[type]);
			if (rc) {
				netdev_dbg(bp->dev, "%s: delete flow_id(%s): %d failed %d\n",
					   __func__, tfc_dir_2_str(dir),
					   flows->id[type], rc);
				rc_save = rc;
			}
		}
	}
	return rc_save;
}

int bnxt_tf_l2_filter_create(struct bnxt *bp,
			     struct bnxt_l2_filter *fltr)
{
	int rc = 0;
	u32 flow_id = 0, idx = 0;
	enum cfa_dir dir;

	/* Truflow is not initialized; we should not create L2 filters */
	if (!(bp->tf_flags & BNXT_TF_FLAG_INITIALIZED))
		return 0;

	/* Create ipv4 Rx Rule */
	dir = CFA_DIR_RX;
	rc = roce_macvlan_flow_create(bp, &flow_id, false,
				      fltr->l2_key.dst_mac_addr, dir);
	if (rc) {
		netdev_dbg(bp->dev, "TF l2 flt add: %pM failed v4\n",
			   fltr->l2_key.dst_mac_addr);
		goto error;
	}
	fltr->base.tf_l2_filter_flow_id[idx++] = flow_id;

	/* Create ipv6 Rx Rule */
	dir = CFA_DIR_RX;
	rc = roce_macvlan_flow_create(bp, &flow_id, true,
				      fltr->l2_key.dst_mac_addr, dir);
	if (rc) {
		netdev_dbg(bp->dev, "TF l2 flt add: %pM failed v6\n",
			   fltr->l2_key.dst_mac_addr);
		goto error;
	}
	fltr->base.tf_l2_filter_flow_id[idx++] = flow_id;

	/* Create Tx Rule */
	dir = CFA_DIR_TX;
	rc = roce_macvlan_flow_create(bp, &flow_id, false,
				      fltr->l2_key.dst_mac_addr, dir);
	if (rc) {
		netdev_dbg(bp->dev, "TF l2 flt add: %pM failed tx\n",
			   fltr->l2_key.dst_mac_addr);
		goto error;
	}
	fltr->base.tf_l2_filter_flow_id[idx++] = flow_id;
	return rc;
error:
	bnxt_tf_l2_filter_delete(bp, fltr);
	return rc;
}

int bnxt_tf_l2_filter_delete(struct bnxt *bp,
			     struct bnxt_l2_filter *fltr)
{
	u32 flow_id = 0, idx;

	for (idx = 0; idx < BNXT_TF_L2_FILT_HNDLS; idx++) {
		flow_id = fltr->base.tf_l2_filter_flow_id[idx];
		if (!flow_id)
			continue;
		bnxt_ulp_gen_flow_destroy(bp, bp->pf.fw_fid, flow_id);
		fltr->base.tf_l2_filter_flow_id[idx] = 0;
	}
	return 0;
}

#endif /* if defined(CONFIG_BNXT_FLOWER_OFFLOAD) */
