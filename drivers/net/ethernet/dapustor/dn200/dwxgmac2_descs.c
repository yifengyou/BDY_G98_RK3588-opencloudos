// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include "dn200.h"
#include "common.h"
#include "dwxgmac_comm.h"

static int dwxgmac2_get_tx_status(void *data, struct dn200_extra_stats *x,
				  struct dma_desc *p, void __iomem *ioaddr)
{
	unsigned int tdes3 = le32_to_cpu(p->des3);
	int ret = tx_done;

	if (unlikely(tdes3 & XGMAC_TDES3_OWN))
		return tx_dma_own;
	if (likely(!(tdes3 & XGMAC_TDES3_LD)))
		return tx_not_ls;

	return ret;
}

static int dwxgmac2_get_rx_err_status(struct dn200_extra_stats *x,
				      struct dma_desc *p, bool rec_all,
				      bool csum_err_flag)
{
	unsigned int rdes3 = le32_to_cpu(p->des3);
	u8 err_result = (rdes3 & XGMAC_RDES3_COND) >> XGMAC_TDES3_CIC_SHIFT;
	/*vxlan err */
	if (csum_err_flag) {
		/*if outer err,we need commit but not add */
		if (err_result == outer_ip_header_err ||
		    err_result == outer_l4_chksum_err) {
			return good_frame;
		}
		if (err_result == inner_ip_header_err) {
			x->rx_csum_err++;
			return good_frame;
		}
		if (err_result == inner_l4_chksum_err) {
			x->rx_csum_err++;
			return good_frame;
		}

	} else { /*ordinary packet err */
		if (err_result == ip_header_err) {
			x->rx_csum_err++;
			return good_frame;
		}
		if (err_result == l4_chksum_err) {
			x->rx_csum_err++;
			return good_frame;
		}
	}
	if (!!rec_all)
		return good_frame;
	return discard_frame;
}

static int dwxgmac2_get_rx_status(void *data, struct dn200_extra_stats *x,
				  struct dma_desc *p, bool rec_all)
{
	unsigned int rdes3 = le32_to_cpu(p->des3);
	unsigned int rdes2 = le32_to_cpu(p->des2);
	unsigned int csum_err_flag = 0;

	if (rdes2 & XGMAC_RDES2_TNP)	/* tunnel packet */
		csum_err_flag = 1;

	if (unlikely(rdes3 & XGMAC_RDES3_OWN))
		return dma_own;
	if (unlikely(rdes3 & XGMAC_RDES3_CTXT))
		return discard_frame;
	if (likely(!(rdes3 & XGMAC_RDES3_LD)))
		return rx_not_ls;
	if (unlikely((rdes3 & XGMAC_RDES3_ES) && (rdes3 & XGMAC_RDES3_LD)))
		return dwxgmac2_get_rx_err_status(x, p, rec_all, csum_err_flag);

	return good_frame;
}

static int dwxgmac2_get_tx_len(struct dma_desc *p)
{
	return (le32_to_cpu(p->des2) & XGMAC_TDES2_B1L);
}

static int dwxgmac2_get_tx_owner(struct dma_desc *p)
{
	return (le32_to_cpu(p->des3) & XGMAC_TDES3_OWN) > 0;
}

static void dwxgmac2_set_tx_owner(struct dma_desc *p)
{
	p->des3 |= cpu_to_le32(XGMAC_TDES3_OWN);
}

static void dwxgmac2_set_rx_owner(struct dma_desc *p, int disable_rx_ic)
{
	p->des3 |= cpu_to_le32(XGMAC_RDES3_OWN);

	// if (!disable_rx_ic)
	//      p->des3 |= cpu_to_le32(XGMAC_RDES3_IOC);
}

static int dwxgmac2_get_tx_ls(struct dma_desc *p)
{
	return (le32_to_cpu(p->des3) & XGMAC_RDES3_LD) > 0;
}

static int dwxgmac2_get_rx_frame_len(struct dma_desc *p, int rx_coe)
{
	return (le32_to_cpu(p->des3) & XGMAC_RDES3_PL);
}

static void dwxgmac2_enable_tx_timestamp(struct dma_desc *p)
{
	p->des2 |= cpu_to_le32(XGMAC_TDES2_TTSE);
}

static int dwxgmac2_get_tx_timestamp_status(struct dma_desc *p)
{
	return 0;		/* Not supported */
}

static inline void dwxgmac2_get_timestamp(void *desc, u32 ats, u64 *ts)
{
	struct dma_desc *p = (struct dma_desc *)desc;
	u64 ns = 0;

	ns += le32_to_cpu(p->des1) * 1000000000ULL;
	ns += le32_to_cpu(p->des0);

	*ts = ns;
}

static int dwxgmac2_rx_check_timestamp(void *desc)
{
	struct dma_desc *p = (struct dma_desc *)desc;
	unsigned int rdes3 = le32_to_cpu(p->des3);
	bool desc_valid, ts_valid;

	dma_rmb();

	desc_valid = !(rdes3 & XGMAC_RDES3_OWN) && (rdes3 & XGMAC_RDES3_CTXT);
	ts_valid = !(rdes3 & XGMAC_RDES3_TSD) && (rdes3 & XGMAC_RDES3_TSA);

	if (likely(desc_valid && ts_valid)) {
		if (p->des0 == 0xffffffff && p->des1 == 0xffffffff)
			return -EINVAL;
		return 0;
	}

	return -EINVAL;
}

static int dwxgmac2_get_rx_timestamp_status(void *desc, void *next_desc,
					    u32 ats)
{
	struct dma_desc *p = (struct dma_desc *)desc;
	unsigned int rdes3 = le32_to_cpu(p->des3);
	int ret = -EBUSY;

	if (likely(rdes3 & XGMAC_RDES3_CDA))
		ret = dwxgmac2_rx_check_timestamp(next_desc);

	return !ret;
}

static void dwxgmac2_init_rx_desc(struct dma_desc *p, int disable_rx_ic,
				  int mode, int end, int bfsize)
{
	dwxgmac2_set_rx_owner(p, disable_rx_ic);
}

static void dwxgmac2_init_tx_desc(struct dma_desc *p, int mode, int end)
{
	p->des0 = 0;
	p->des1 = 0;
	p->des2 = 0;
	p->des3 = 0;
}

static void dwxgmac2_prepare_tx_desc(struct dma_desc *p, int is_fs, int len,
				     bool csum_flag, int mode, bool tx_own,
				     bool ls, unsigned int tot_pkt_len)
{
	unsigned int tdes3 = le32_to_cpu(p->des3);

	p->des2 |= cpu_to_le32(len & XGMAC_TDES2_B1L);

	tdes3 |= tot_pkt_len & XGMAC_TDES3_FL;
	if (is_fs)
		tdes3 |= XGMAC_TDES3_FD;
	else
		tdes3 &= ~XGMAC_TDES3_FD;

	if (csum_flag)
		tdes3 |= 0x3 << XGMAC_TDES3_CIC_SHIFT;
	else
		tdes3 &= ~XGMAC_TDES3_CIC;

	if (ls)
		tdes3 |= XGMAC_TDES3_LD;
	else
		tdes3 &= ~XGMAC_TDES3_LD;

	/* Finally set the OWN bit. Later the DMA will start! */
	if (tx_own)
		tdes3 |= XGMAC_TDES3_OWN;

	if (is_fs && tx_own)
		/* When the own bit, for the first frame, has to be set, all
		 * descriptors for the same frame has to be set before, to
		 * avoid race condition.
		 */
		dma_wmb();

	p->des3 = cpu_to_le32(tdes3);
}

static void dwxgmac2_prepare_tso_tx_desc(struct dma_desc *p, int tso_flag,
					 int len1, int len2, bool tx_own,
					 bool ls, unsigned int tcphdrlen,
					 unsigned int tcppayloadlen)
{
	unsigned int tdes3 = le32_to_cpu(p->des3);

	if (len1)
		p->des2 |= cpu_to_le32(len1 & XGMAC_TDES2_B1L);
	if (len2)
		p->des2 |= cpu_to_le32((len2 << XGMAC_TDES2_B2L_SHIFT) &
				       XGMAC_TDES2_B2L);
	if (tso_flag & TSO_DESC_IS_FIRST) {
		tdes3 |= (XGMAC_TDES3_FD | XGMAC_TDES3_TSE);

		tdes3 |= (tcphdrlen << XGMAC_TDES3_THL_SHIFT) & XGMAC_TDES3_THL;
		tdes3 |= tcppayloadlen & XGMAC_TDES3_TPL;
	} else {
		tdes3 &= ~XGMAC_TDES3_FD;
	}

	if (tso_flag & TSO_DESC_IS_TUNNEL)
		tdes3 |= (3 << XGMAC_TDES3_SAIC_SHIFT) & XGMAC_TDES3_SAIC;

	if (ls)
		tdes3 |= XGMAC_TDES3_LD;
	else
		tdes3 &= ~XGMAC_TDES3_LD;

	/* Finally set the OWN bit. Later the DMA will start! */
	if (tx_own)
		tdes3 |= XGMAC_TDES3_OWN;

	if ((tso_flag & TSO_DESC_IS_FIRST) && tx_own)
		/* When the own bit, for the first frame, has to be set, all
		 * descriptors for the same frame has to be set before, to
		 * avoid race condition.
		 */
		dma_wmb();

	p->des3 = cpu_to_le32(tdes3);
}

static void dwxgmac2_release_tx_desc(struct dma_desc *p, int mode)
{
	p->des0 = 0;
	p->des1 = 0;
	p->des2 = 0;
	p->des3 = 0;
}

static void dwxgmac2_set_tx_ic(struct dma_desc *p)
{
	p->des2 |= cpu_to_le32(XGMAC_TDES2_IOC);
}

static void dwxgmac2_set_mss(struct dma_desc *p, unsigned int mss)
{
	p->des0 = 0;
	p->des1 = 0;
	p->des2 = cpu_to_le32(mss);
	p->des3 = cpu_to_le32(XGMAC_TDES3_CTXT | XGMAC_TDES3_TCMSSV);
}

static void dwxgmac2_get_addr(struct dma_desc *p, unsigned int *addr)
{
	*addr = le32_to_cpu(p->des0);
}

static void dwxgmac2_set_addr(struct dma_desc *p, dma_addr_t addr,
			      struct mac_device_info *hw)
{
	p->des0 = cpu_to_le32(lower_32_bits(addr));
	p->des1 = cpu_to_le32(upper_32_bits(addr));
}

static void dwxgmac2_clear(struct dma_desc *p)
{
	p->des0 = 0;
	p->des1 = 0;
	p->des2 = 0;
	p->des3 = 0;
}

static int dwxgmac2_get_rx_hash(struct dma_desc *p, u32 *hash,
				enum pkt_hash_types *type)
{
	unsigned int rdes3 = le32_to_cpu(p->des3);
	u32 ptype;

	if (rdes3 & XGMAC_RDES3_RSV) {
		ptype = (rdes3 & XGMAC_RDES3_L34T) >> XGMAC_RDES3_L34T_SHIFT;

		switch (ptype) {
		case XGMAC_L34T_IP4TCP:
		case XGMAC_L34T_IP4UDP:
		case XGMAC_L34T_IP6TCP:
		case XGMAC_L34T_IP6UDP:
			*type = PKT_HASH_TYPE_L4;
			break;
		default:
			*type = PKT_HASH_TYPE_L3;
			break;
		}

		*hash = le32_to_cpu(p->des1);
		return 0;
	}

	return -EINVAL;
}

static int dwxgmac2_get_ovt(struct dma_desc *p)
{
	unsigned int rdes0 = le32_to_cpu(p->des0);
	unsigned int rdes2 = le32_to_cpu(p->des2);

	if (rdes2 & XGMAC_RDES2_TNP)
		return 0;

	return (rdes0 & XGMAC_RDES0_OVT);
}

static void dwxgmac2_get_rx_header_len(struct dma_desc *p, unsigned int *len)
{
	if (le32_to_cpu(p->des3) & XGMAC_RDES3_L34T)
		*len = le32_to_cpu(p->des2) & XGMAC_RDES2_HL;
}

static void dwxgmac2_set_sec_addr(struct dma_desc *p, dma_addr_t addr,
				  bool is_valid, struct mac_device_info *hw)
{
	p->des2 = cpu_to_le32(lower_32_bits(addr));
	p->des3 = cpu_to_le32(upper_32_bits(addr));
}

static void dwxgmac2_set_sarc(struct dma_desc *p, u32 sarc_type)
{
	sarc_type <<= XGMAC_TDES3_SAIC_SHIFT;
	p->des3 |= cpu_to_le32(sarc_type & XGMAC_TDES3_SAIC);
}

static void dwxgmac2_set_vxlan(struct dma_desc *p)
{
	p->des3 |= GENMASK(24, 23);
	p->des3 &= (~BIT(25));
}

static void dwxgmac2_set_vlan_tag(struct dma_desc *p, u16 tag, u16 inner_tag,
				  u32 inner_type)
{
	p->des0 = 0;
	p->des1 = 0;
	p->des2 = 0;
	p->des3 = 0;

	/* Inner VLAN */
	if (inner_type) {
		u32 des = inner_tag << XGMAC_TDES2_IVT_SHIFT;

		des &= XGMAC_TDES2_IVT;
		p->des2 = cpu_to_le32(des);

		des = inner_type << XGMAC_TDES3_IVTIR_SHIFT;
		des &= XGMAC_TDES3_IVTIR;
		p->des3 = cpu_to_le32(des | XGMAC_TDES3_IVLTV);
	}

	/* Outer VLAN */
	p->des3 |= cpu_to_le32(tag & XGMAC_TDES3_VT);
	p->des3 |= cpu_to_le32(XGMAC_TDES3_VLTV);

	p->des3 |= cpu_to_le32(XGMAC_TDES3_CTXT);
}

static void dwxgmac2_set_vlan(struct dma_desc *p, u32 type)
{
	type <<= XGMAC_TDES2_VTIR_SHIFT;
	p->des2 |= cpu_to_le32(type & XGMAC_TDES2_VTIR);
}

static void dwxgmac2_display_ring(void *head, unsigned int size, bool not_tbl,
				  dma_addr_t dma_rx_phy, unsigned int desc_size,
				  struct mac_device_info *hw)
{
	int i = 0;
	dma_addr_t dma_addr;
	struct dma_desc *p;

	if (desc_size != sizeof(struct dma_desc))
		return;
	p = (struct dma_desc *)head;

	for (i = 0; i < size; i++) {
		dma_addr = dma_rx_phy + i * desc_size;
		if (desc_size == sizeof(struct dma_desc)) {
			dev_info(hw->priv->device,
				 "%d [%pad]: 0x%x 0x%x 0x%x 0x%x\n", i,
				 &dma_addr, le32_to_cpu(p->des0),
				 le32_to_cpu(p->des1), le32_to_cpu(p->des2),
				 le32_to_cpu(p->des3));
			p++;
		}
		dev_info(hw->priv->device, "\n");
	}
}

const struct dn200_desc_ops dwxgmac210_desc_ops = {
	.tx_status = dwxgmac2_get_tx_status,
	.rx_status = dwxgmac2_get_rx_status,
	.get_tx_len = dwxgmac2_get_tx_len,
	.get_tx_owner = dwxgmac2_get_tx_owner,
	.set_tx_owner = dwxgmac2_set_tx_owner,
	.set_rx_owner = dwxgmac2_set_rx_owner,
	.get_tx_ls = dwxgmac2_get_tx_ls,
	.get_rx_frame_len = dwxgmac2_get_rx_frame_len,
	.enable_tx_timestamp = dwxgmac2_enable_tx_timestamp,
	.get_tx_timestamp_status = dwxgmac2_get_tx_timestamp_status,
	.get_rx_timestamp_status = dwxgmac2_get_rx_timestamp_status,
	.get_timestamp = dwxgmac2_get_timestamp,
	.set_tx_ic = dwxgmac2_set_tx_ic,
	.prepare_tx_desc = dwxgmac2_prepare_tx_desc,
	.prepare_tso_tx_desc = dwxgmac2_prepare_tso_tx_desc,
	.release_tx_desc = dwxgmac2_release_tx_desc,
	.init_rx_desc = dwxgmac2_init_rx_desc,
	.init_tx_desc = dwxgmac2_init_tx_desc,
	.set_mss = dwxgmac2_set_mss,
	.get_addr = dwxgmac2_get_addr,
	.set_addr = dwxgmac2_set_addr,
	.clear = dwxgmac2_clear,
	.get_rx_hash = dwxgmac2_get_rx_hash,
	.get_rx_header_len = dwxgmac2_get_rx_header_len,
	.set_sec_addr = dwxgmac2_set_sec_addr,
	.set_sarc = dwxgmac2_set_sarc,
	.set_vlan_tag = dwxgmac2_set_vlan_tag,
	.set_vlan = dwxgmac2_set_vlan,
	.get_ovt = dwxgmac2_get_ovt,
	.set_vxlan = dwxgmac2_set_vxlan,
	.display_ring = dwxgmac2_display_ring,
};
