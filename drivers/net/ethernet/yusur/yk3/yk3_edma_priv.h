/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_EDMA_PRIV_H
#define _YK3_EDMA_PRIV_H

#include "yk3_base.h"
#include "yk3_ringbase.h"
#include "yk3_scatter.h"

#define YK3_N_MAX_TXPKTLEN		(9696)
#define YK3_N_MAX_TXFRAGSIZE		(12288)

#define YK3_N_COAL_MAX			(127)
#define YK3_N_PERIOD_MAX		(65535)
#define YK3_N_RX_MINDATA		256

/* two-gear adaptive interrupt moderation (ITR) */
#define YK3_ITR_LAT_FRAMES		1
#define YK3_ITR_LAT_USECS		0
#define YK3_ITR_TP_FRAMES		128
#define YK3_ITR_TP_USECS		128
#define YK3_ITR_EVAL_NS			(2ULL * 1000 * 1000)	/* 2ms window */
#define YK3_ITR_MBPS_HI			200	/* >=200MB/s -> throughput gear */
#define YK3_ITR_MBPS_LO			50	/* <50MB/s  -> latency gear    */
enum { YK3_ITR_GEAR_LATENCY = 0, YK3_ITR_GEAR_THROUGHPUT };

/* yk3 register edma */
#define YK3_RE_BASE			0x00000000
#define YK3_RE_DMA_ID			(0x0000)
#define YK3_RE_DMA_INST			(0x0004)
#define YK3_RE_DMA_QNUM			(0x0008)
#define YK3_RE_DMA_QSETNUM		(0x000c)

#define YK3_RE_DMA_PFX_FNUM(i)		(0x0010 + ((i) * 0x4))
#define YK3_RE_DMA_PF_FTOP_GMASK	GENMASK(31, 16)
#define YK3_RE_DMA_PF_FBASE_GMASK	GENMASK(15, 0)

#define YK3_RE_DMA_QSET_OFFSET		(0x0034)
#define YK3_RE_DMA_QSET_OFFSET_GMASK	GENMASK(10, 0)
#define YK3_RE_DMA_QSET_QMAXNUM_GMASK	GENMASK(19, 16)

#define YK3_RE_DMA_PFX_QBASE(i)		(0x0040 + ((i) * 0x4))
#define YK3_RE_DMA_PF_QSTART_GMASK	GENMASK(9, 0)
#define YK3_RE_DMA_PF_QNUM_GMASK	GENMASK(25, 16)

#define YK3_RE_DMA_MDROP_STATUS		(0x0080)
#define YK3_RE_DMA_MDROP_BYPASS_GMASK	GENMASK(0, 0)
#define YK3_RE_DMA_MDROP_HEADKEEP_GMASK	GENMASK(1, 1)
#define YK3_RE_DMA_MDROP_LOSECHK_GMASK	GENMASK(2, 2)
#define YK3_RE_DMA_MDROP_LOSECLR_GMASK	GENMASK(3, 3)
#define YK3_RE_DMA_MDROP_QIDSEL_GMASK	GENMASK(4, 4)
#define YK3_RE_DMA_MDROP_MACTIVE_GMASK	GENMASK(5, 5)
#define YK3_RE_DMA_MDROP_SWSCALE_GMASK	GENMASK(7, 6)
#define YK3_RE_DMA_MDROP_SWBIAS_GMASK	GENMASK(11, 8)
#define YK3_RE_DMA_MDROP_SWRSSFR_MASK	GENMASK(12, 12)
#define YK3_RE_DMA_MDROP_MAXPKTLEN_GMASK	GENMASK(31, 16)

#define YK3_RE_DMA_DATAHDL_CTRL		(0x0300)
#define YK3_RE_DMA_DATAHDL_TC_THR	(0x0304)
#define YK3_RE_DMA_DATAHDL_QUE_THR	(0x0308)
#define YK3_RE_DMA_DATAHDL_AF_THR	(0x030c)
#define YK3_RE_DMA_TXDF_AXI4_ARID	(0x0394)
#define YK3_RE_DMA_RXDF_AXI4_ARID	(0x0398)
#define YK3_RE_DMA_DATAHDL_DATAPFC_THRX(x)	(0x03a4 + ((x) << 2))
#define YK3_RE_DMA_DATAHDL_PKTPFC_THRX(x)	(0x0424 + ((x) << 2))
#define YK3_RE_DMA_DATAHDL_DATAPFC_DIFF	(0x04f4)
#define YK3_RE_DMA_DATAHDL_PKTPFC_DIFF	(0x04f8)
#define YK3_RE_DMA_DATAHDL_QUE_PKT_THR	(0x06cc)

#define YK3_RE_DMA_FUNC_QBASE		(0x2000)
#define YK3_RE_DMA_FUNCX_QBASE(i)	(0x2000 + ((i) * 0x4))
#define YK3_RE_DMA_FUNC_QSTART_GMASK	GENMASK(9, 0)
#define YK3_RE_DMA_FUNC_QNUM_GMASK	GENMASK(25, 16)

#define YK3_RE_CLK_EN			0x22c
#define YK3_RE_HWQ_TXCNT(g_qid)		(0xc4000 + (g_qid) * 0x8)
#define YK3_RE_HWQ_RXCNT(g_qid)		(0xc0000 + (g_qid) * 0x8)
#define YK3_RE_HWQ_RXDROPCNT(g_qid)	(0xca000 + (g_qid) * 0x8)

/* edma queue */
#define YK3_RE_QUEUE_BASE		(YK3_RE_BASE + 0x40000)
#define YK3_RE_QX_BASE(i)		(YK3_RE_QUEUE_BASE + ((i) * 0x100))

#define YK3_RE_TXQ_ADDR_L		0x00
#define YK3_RE_TXQ_ADDR_H		0x04
#define YK3_RE_TXQ_HEAD			0x08
#define YK3_RE_TXQ_TAIL			0x0c
#define YK3_RE_TXQ_DEPTH		0x10
#define YK3_RE_TXQ_DEPTH_MAX		0x14
#define YK3_RE_TXQ_FRAGSIZE		0x18
#define YK3_RE_TXQ_FRAGSIZE_MAX		0x1c
#define YK3_RE_TXQ_CTRL			0x20

#define YK3_RE_TXCQ_HEAD_ADDR_L		0x48
#define YK3_RE_TXCQ_HEAD_ADDR_H		0x4c
#define YK3_RE_TXCQ_HEAD		0x50
#define YK3_RE_TXCQ_IRQ_VECTOR		0x54
#define YK3_RE_TXCQ_COAL		0x58
#define YK3_RE_TXCQ_PERIOD		0x5c
#define YK3_RE_TXCQ_IRQ_DISABLE		0x60
#define YK3_RE_TXCQ_CPLLEN		0x64

enum yk3_txcq_cpllen {
	CPLLEN_2K = 0,
	CPLLEN_4K,
	CPLLEN_8K,
	CPLLEN_16K,
	CPLLEN_32K,
	CPLLEN_64K,
	CPLLEN_128K,
	CPLLEN_NOLIMIT,
};

#define YK3_RE_RXQ_ADDR_L		0x80
#define YK3_RE_RXQ_ADDR_H		0x84
#define YK3_RE_RXQ_HEAD			0x88
#define YK3_RE_RXQ_TAIL			0x8c
#define YK3_RE_RXQ_DEPTH		0x90
#define YK3_RE_RXQ_DEPTH_MAX		0x94
#define YK3_RE_RXQ_FRAGSIZE		0x98
#define YK3_RE_RXQ_FRAGSIZE_MAX		0x9c
#define YK3_RE_RXQ_CTRL			0xa0

#define YK3_RE_RXCQ_ADDR_L		0xc0
#define YK3_RE_RXCQ_ADDR_H		0xc4
#define YK3_RE_RXCQ_HEAD_ADDR_L		0xc8
#define YK3_RE_RXCQ_HEAD_ADDR_H		0xcc
#define YK3_RE_RXCQ_HEAD		0xd0
#define YK3_RE_RXCQ_IRQ_VECTOR		0xd4
#define YK3_RE_RXCQ_IRQ_PERIOD		0xd8
#define YK3_RE_RXCQ_IRQ_COAL		0xdc
#define YK3_RE_RXCQ_PERIOD		0xe0
#define YK3_RE_RXCQ_COAL		0xe4
#define YK3_RE_RXCQ_IRQ_DISABLE		0xe8

#define YK3_V_RXQ_RXCLR			BIT(1)
#define YK3_V_RXQ_RXEMPTY		BIT(4)
#define YK3_V_RXQ_PROTECTOFF		BIT(4)

#define YK3_RE_QSET2Q_BASE		(YK3_RE_BASE + 0x4000)
#define YK3_RE_QSET2Q(i)		(YK3_RE_QSET2Q_BASE + ((i) * 0x4))

#define YK3_RE_QSET2Q_QSTART_GMASK	GENMASK(9, 0)
#define YK3_RE_QSET2Q_RSS_EN_GMASK	GENMASK(10, 10)
#define YK3_RE_QSET2Q_QNUM_GMASK	GENMASK(18, 11)
#define YK3_RE_QSET2Q_VALID_GMASK	GENMASK(19, 19)
#define YK3_RE_QSET2Q_PRITYPE_GMASK	GENMASK(25, 20)
#define YK3_RE_QSET2Q_CBSBASE_GMASK	GENMASK(31, 26)

#define YK3_RE_Q2QSET_BASE		(YK3_RE_BASE + 0x8000)
#define YK3_RE_Q2QSET(i)		(YK3_RE_Q2QSET_BASE + ((i) * 0x4))
#define YK3_RE_Q2QSET_QSETID_GMASK	GENMASK(9, 0)

#define YK3_RE_MDROP_PRITBL(i)		(YK3_RE_BASE + 0x5000 + ((i) << 5))

/* edma function */
#define YK3_RE_FUNC_BASE		(YK3_RE_BASE + 0x2000)
#define YK3_RE_FUNCX_QUEUE(i)		(0x04 * (i))

/* edma meter drops */
#define YK3_RM_STATUS			0x80
#define YK3_RM_STATUS_BYPASS_GMASK	GENMASK(0, 0)
#define YK3_RM_STATUS_ACTIVE_GMASK	GENMASK(5, 5)

#define YK3_RM_CBS(qsetid)		(0x6000 + ((qsetid) << 2))
#define YK3_RM_CBS_GMASK		GENMASK(19, 0)
#define YK3_RM_CIR(qsetid)		(0x7000 + ((qsetid) << 2))
#define YK3_RM_CIR_GMASK		GENMASK(16, 0)
#define YK3_N_RM_CIR_MAX		(0xFEFE)

/* bar filter for edma */
#define YK3_RB_BASE			(0x2000000)
#define YK3_RB_EDMA_BASE		(0x90)
#define YK3_RB_EDMA_TOP			(0x94)
#define YK3_RB_EDMA_GMASK		GENMASK(26, 0)
#define YK3_V_EDMA_BASE			(0)
#define YK3_V_EDMA_TOP			(0x100000)

#define YK3_RE_DATAHDL_CTRL		0x300

struct yk3_funcbase {
	u16 top;
	u16 base;
};

struct yk3_edma_info {
	u32 dma_id;
	u32 dma_inst;
	u32 dma_qmaxnum;
	u32 dma_max_qsetnum;

	u16 dma_qset_offset;
	u16 dma_qset_qmaxnum;

	struct dentry *dbgfs_info_file;

	struct yk3_pdev_priv *pdev_priv[YK3_N_MAX_NETPF];
};

#define YK3_N_EDMA_QNUM		1024

struct yk3_edma_pf {
	struct yk3_funcbase fbase;
	struct yk3_queuebase g_qbase;
	struct yk3_queuebase fx_p_qbase[YK3_N_PF_MAX_FUNC];
	struct yk3_queuebase fx_g_qbase[YK3_N_PF_MAX_FUNC];
	DECLARE_BITMAP(qbitmap, YK3_N_EDMA_QNUM);
	atomic_t qfree;
};

struct yk3_edma_priv {
	void __iomem *hw_addr;
	struct dma_pool *cd_head_pool;

	struct yk3_queuebase qbase[YK3_QUEUE_T_MAX];

	struct dentry *dbgfs_info_file;

	struct yk3_edma_pf pf[];
};

enum yk3_edma_msgid {
	MSG_QBASE_GET,
	MSG_MDROP_SET_RATE,
};

struct yk3_edma_msg {
	enum yk3_edma_msgid msgid;
	union {
		struct {
			union {
				struct {
					u16 vf_id;
					enum yk3_queue_type type;
				} req;
				struct {
					struct yk3_queuebase qbase;
				} rsp;
			};
		} qbase_get;
		struct {
			union {
				struct {
					u16 qsetid;
					u32 speed;
					u32 rate;
				} req;
				struct {
					int ret;
				} rsp;
			};
		} mdrop_rate;
	};
};

static inline void __iomem *yk3_edma_hwaddr(struct yk3_pdev_priv *pdev_priv)
{
	return pdev_priv->edma_priv->hw_addr;
}

static inline u32 yk3_edma_usec_to_cycles(struct yk3_pdev_priv *pdev_priv, u32 usec)
{
	u32 period = pdev_priv->card->edma_clk;

	period /= 1000 * 1000; /* 1 usec */
	period = period ?: 1;
	period *= usec;

	return period / 256;
}

/* desc */
struct yk3_txd {
	u64 addr;
	union {
		u64 value;
		struct {
#ifndef __BIG_ENDIAN_BITFIELD
			u64 size:16;

			u64 reserved1:1;
			u64 lan_pars_disable:1;		/* turn off lan parser */
			u64 ppp_sysm_disable:1;		/* ppp can not modify sysmeta */
			u64 ppp_bypass:1;		/* ??? */
			u64 local_priority:3;
			u64 q_group:5;
			u64 pass_through:1;
			u64 outer_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 outer_l4_csum_en:1;		/* support tcp & udp */
			u64 inner_l3_csum_en:1;		/* support ipv4 & ipv6 */

			u64 inner_l4_csum_en:1;		/* support tcp & udp */
			u64 tso_fixed_id:1;
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 ptp_sync:1;
			u64 soft_def:8;			/* used with np */
			u64 vlan_id_h:4;

			u64 vlan_valid:1;
			u64 vlan_pri:3;
			u64 vlan_id_l:8;
			u64 control:1;
			u64 interrupt:1;
			u64 fd:1;
			u64 ld:1;
#else
			u64 size:16;

			u64 q_group_1:1;		/* q_group least significant bit */
			u64 local_priority:3;
			u64 ppp_bypass:1;		/* ??? */
			u64 ppp_sysm_disable:1;		/* ppp can not modify sysmeta */
			u64 lan_pars_disable:1;		/* turn off lan parser */
			u64 reserved1:1;
			u64 inner_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 outer_l4_csum_en:1;		/* support tcp & udp */
			u64 outer_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 pass_through:1;
			u64 q_group_2:4;		/* q_group most significant bit */

			u64 soft_def_1:4		/* used with np, least significant bit */
			u64 ptp_sync:1;
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 tso_fixed_id:1;
			u64 inner_l4_csum_en:1;		/* support tcp & udp */
			u64 vlan_id_h:4;
			u64 soft_def_2:4		/* used with np, most significant bit */

			u64 vlan_id_l_1:4;		/* vlan_id_l least significant bit */
			u64 vlan_pri:3;
			u64 vlan_valid:1;
			u64 ld:1;
			u64 fd:1;
			u64 interrupt:1;
			u64 control:1;
			u64 vlan_id_l_2:4;		/* vlan_id_l most significant bit */
#endif
		};
	};
};

static inline void yk3_txd_set_qgroup(struct yk3_txd *txd, u16 qg)
{
#ifndef __BIG_ENDIAN_BITFIELD
	txd->q_group = qg & 0x1f;
#else
	txd->q_group_1 = qg & 0x1;
	txd->q_group_2 = (qg >> 1) & 0xf;
#endif
}

static inline void yk3_txd_set_softdef(struct yk3_txd *txd, u8 softdef)
{
#ifndef __BIG_ENDIAN_BITFIELD
	txd->soft_def = softdef;
#else
	txd->soft_def_1 = softdef & 0xf;
	txd->soft_def_2 = (softdef >> 4) & 0xf;
#endif
}

static inline void
yk3_txd_set_vlan(struct yk3_txd *txd, u16 vlan_id, u8 vlan_pri, bool is_8021ad)
{
	txd->vlan_valid = 1;
#ifndef __BIG_ENDIAN_BITFIELD
	txd->vlan_id_l = vlan_id & 0xff;
#else
	txd->vlan_id_l_1 = vlan_id & 0xf;
	txd->vlan_id_l_2 = (vlan_id >> 4) & 0xf;
#endif
	txd->vlan_id_h = (vlan_id >> 8) & 0xf;
	txd->vlan_pri = vlan_pri & 0x7;

	txd->vlan_protocol_type = is_8021ad ? 1 : 0;
}

struct yk3_atxd {
	__le64 addr;
	union {
		u64 value;
		struct {
#ifndef __BIG_ENDIAN_BITFIELD
			u64 size:14;
			u64 pktaggr:1;
			u64 reserved0:1;

			u64 pkt_num:5;
			u64 reserved1:3;

			u64 payload_size:14;

			u64 reserved2:22;

			u64 control:1;
			u64 interrupt:1;
			u64 fd:1;
			u64 ld:1;
#else
			u64 size_1:8;
			u64 reserved0:1;
			u64 pktaggr:1;
			u64 size_2:6;

			u64 reserved1:3;
			u64 pkt_num:5;

			u64 payload_size_1:8;
			u64 reserved2:2;
			u64 payload_size_2:6;

			u64 reserved3:16;

			u64 ld:1;
			u64 fd:1;
			u64 interrupt:1;
			u64 control:1;
			u64 reserved4:4;
#endif
		};
	};
};

static inline void yk3_atxd_set_size(struct yk3_atxd *atxd, u16 size)
{
#ifndef __BIG_ENDIAN_BITFIELD
	atxd->size = size & 0x3fff;
#else
	atxd->size_1 = size & 0xff;
	atxd->size_2 = (size >> 8) & 0x3f;
#endif
}

static inline void yk3_atxd_set_payloadsize(struct yk3_atxd *atxd, u16 size)
{
#ifndef __BIG_ENDIAN_BITFIELD
	atxd->payload_size = size & 0x3fff;
#else
	atxd->payload_size_1 = size & 0xff;
	atxd->payload_size_2 = (size >> 8) & 0x3f;
#endif
}

struct yk3_ad {
	union {
		u64 value;
		struct {
#ifndef __BIG_ENDIAN_BITFIELD
			u64 size:14;
			u64 reserved0:2;

			u64 reserved1:1;
			u64 lan_pars_disable:1;		/* turn off lan parser */
			u64 ppp_sysm_disable:1;		/* ppp can not modify sysmeta */
			u64 ppp_bypass:1;		/* ??? */
			u64 local_priority:3;
			u64 q_group:5;
			u64 pass_through:1;
			u64 outer_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 outer_l4_csum_en:1;		/* support tcp & udp */
			u64 inner_l3_csum_en:1;		/* support ipv4 & ipv6 */

			u64 inner_l4_csum_en:1;		/* support tcp & udp */
			u64 reserved2:1;
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 ptp_sync:1;
			u64 soft_def:8;			/* used with np */
			u64 vlan_id_h:4;

			u64 vlan_valid:1;
			u64 vlan_pri:3;
			u64 vlan_id_l:8;
			u64 control:1;
			u64 interrupt:1;
			u64 fd:1;
			u64 ld:1;
#else
			u64 size_1:8;
			u64 reservd0:2;
			u64 size_2:6;

			u64 q_group_1:1;		/* q_group least significant bit */
			u64 local_priority:3;
			u64 ppp_bypass:1;		/* ??? */
			u64 ppp_sysm_disable:1;		/* ppp can not modify sysmeta */
			u64 lan_pars_disable:1;		/* turn off lan parser */
			u64 reserved1:1;
			u64 inner_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 outer_l4_csum_en:1;		/* support tcp & udp */
			u64 outer_l3_csum_en:1;		/* support ipv4 & ipv6 */
			u64 pass_through:1;
			u64 q_group_2:4;		/* q_group most significant bit */

			u64 soft_def_1:4		/* used with np, least significant bit */
			u64 ptp_sync:1;
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 reserved2:1;
			u64 inner_l4_csum_en:1;		/* support tcp & udp */
			u64 vlan_id_h:4;
			u64 soft_def_2:4		/* used with np, most significant bit */

			u64 vlan_id_l_1:4;		/* vlan_id_l least significant bit */
			u64 vlan_pri:3;
			u64 vlan_valid:1;
			u64 ld:1;
			u64 fd:1;
			u64 interrupt:1;
			u64 control:1;
			u64 vlan_id_l_2:4;		/* vlan_id_l most significant bit */
#endif
		};
	};
};

static inline void yk3_ad_set_size(struct yk3_ad *ad, u16 size)
{
#ifndef __BIG_ENDIAN_BITFIELD
	ad->size = size & 0x3fff;
#else
	ad->size_1 = size & 0xff;
	ad->size_2 = (size >> 8) & 0x3f;
#endif
}

static inline void
yk3_ad_set_vlan(struct yk3_ad *ad, u16 vlan_id, u8 vlan_pri, bool is_8021ad)
{
	ad->vlan_valid = 1;
#ifndef __BIG_ENDIAN_BITFIELD
	ad->vlan_id_l = vlan_id & 0xff;
#else
	ad->vlan_id_l_1 = vlan_id & 0xf;
	ad->vlan_id_l_2 = (vlan_id >> 4) & 0xf;
#endif
	ad->vlan_id_h = (vlan_id >> 8) & 0xf;
	ad->vlan_pri = vlan_pri & 0x7;

	ad->vlan_protocol_type = is_8021ad ? 1 : 0;
}

static inline void yk3_ad_set_qgroup(struct yk3_ad *ad, u16 qg)
{
#ifndef __BIG_ENDIAN_BITFIELD
	ad->q_group = qg & 0x1f;
#else
	ad->q_group_1 = qg & 0x1;
	ad->q_group_2 = (qg >> 1) & 0xf;
#endif
}

struct yk3_ctld {
	union {
		u64 value1;
		struct {
#ifndef __BIG_ENDIAN_BITFIELD
			u64 reserved1:8;
			u64 mss_num:7;
			u64 tso_en:1;
			u64 mss_h:6;
			u64 tso_first:1;
			u64 tso_last:1;
			u64 mss_l:8;
#else
			u64 reserved1:8;
			u64 tso_en:1;
			u64 mss_num:7;
			u64 tso_last:1;
			u64 tso_first:1;
			u64 mss_h:6;
			u64 mss_l:8;
#endif
			u64 desc_ctrl:16;
			u64 parser_hash_l:8;
			u64 last_offset:8;
		};
	};

	union {
		u64 value2;
		struct {
			u64 bit_flag:32;
			u64 parser_hash_h:16;
			u64 reserved3:8;
#ifndef __BIG_ENDIAN_BITFIELD
			u64 reserved4:4;
			u64 control:1;
			u64 interrupt:1;
			u64 fd:1;
			u64 ld:1;
#else
			u64 ld:1;
			u64 fd:1;
			u64 interrupt:1;
			u64 control:1;
			u64 reserved4:4;
#endif
		};
	};
};

static inline void yk3_ctld_set_mss(struct yk3_ctld *ctld, u16 mss)
{
	ctld->mss_l = mss & 0xff;
	ctld->mss_h = (mss >> 8) & 0x3f;
}

struct yk3_rxd {
	__le64 addr;
};

struct yk3_rxcd {
	union {
		u64 value1;
		struct {
			u64 size:16;
			u64 desc_id:16;
#ifndef __BIG_ENDIAN_BITFIELD
			u64 fcs_error:1;		/* used by mac, stats and drop it */
			u64 vlan_tag_remove:1;
			u64 mtu_error:1;  /* used by lan, exceed mtu setting, stats and drop it */
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 ptp_rec:1;
			u64 soft_def:8;
			u64 vlan_id_h:4;
			u64 vlan_valid:1;
			u64 vlan_pri:3;
			u64 vlan_id_l:8;
			u64 edma_error:1;		/* used by edma, stats and drop it */
			u64 fd:1;
			u64 ld:1;
#else
			u64 soft_def_1:3		/* least significant bit */
			u64 ptp_rec:1;
			u64 vlan_protocol_type:1;	/* 0: 802.1q, 1: 802.1ad used with vlan */
			u64 mtu_error:1;  /* used by lan, exceed mtu setting, stats and drop it */
			u64 vlan_tag_remove:1;
			u64 fcs_error:1;		/* used by mac, drop it */

			u64 vlan_id_h_1:3;
			u64 soft_def_2:5;

			u64 vlan_id_l_1:3;
			u64 vlan_pri:3;
			u64 vlan_valid:1;
			u64 vlan_id_h_2:1;

			u64 ld:1;
			u64 fd:1;
			u64 edma_error:1;		/* used by edma, stats and drop it */
			u64 vlan_id_l_2:5;
#endif
		};
	};
	union {
		u64 value2;
		struct {
#ifndef __BIG_ENDIAN_BITFIELD
			u64 hash_result:16;		/* used by lan for RXHASH */

			u64 inner_l4_csum_unchk:1;
			u64 inner_l4_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 inner_l3_csum_unchk:1;
			u64 inner_l3_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 outer_l4_csum_unchk:1;
			u64 outer_l4_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 outer_l3_csum_unchk:1;
			u64 outer_l3_csum_error:1;	/* used by lan or np, stats and drop it */

			u64 cast_type:2;		/* 00b : unicast, 01b : multicast */
			u64 pkt_timeout:1;		/* used by lan, stats and drop it */
			u64 pkt_cutoff:1;		/* used by lan, stats and drop it */
			u64 csum_complete_valid:1;	/* used by np for checksum complete */
			u64 lro_valid:1;		/* used by np for lro */
			u64 reserved2:2;

			u64 csum_complete:16;		/* csum complete value from ip */
			u64 lro_id:16;			/* used by np for lro */
#else
			u64 hash_result:16;		/* used by lan for RXHASH */

			u64 outer_l3_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 outer_l3_csum_unchk:1;
			u64 outer_l4_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 outer_l4_csum_unchk:1;
			u64 inner_l3_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 inner_l3_csum_unchk:1;
			u64 inner_l4_csum_error:1;	/* used by lan or np, stats and drop it */
			u64 inner_l4_csum_unchk:1;

			u64 reserved2:2;
			u64 lro_valid:1;		/* used by np for lro */
			u64 csum_complete_valid:1;	/* used by np for checksum complete */
			u64 pkt_cutoff:1;		/* used by lan, stats and drop it */
			u64 pkt_timeout:1;		/* used by lan, stats and drop it */
			u64 cast_type:2;		/* 00b : unicast, 01b : multicast */

			u64 csum_complete:16;		/* csum complete value from ip */
			u64 lro_id:16;			/* used by np for lro */
#endif
		};

		/* ptp rx timestamp. only the low 48 bits is used */
		struct __packed {
			u64 resv3:16;
			u64 ts:48;
		};
	};
};

static inline u16 yk3_rxcd_get_vlanid(struct yk3_rxcd *rxcd)
{
#ifndef __BIG_ENDIAN_BITFIELD
	return (rxcd->vlan_id_h << 8) | rxcd->vlan_id_l;
#else
	return (rxcd->vlan_id_h_2 << (8 + 3)) | (rxcd->vlan_id_h_1 << 8) |
	       (rxcd->vlan_id_l_2 << 3) | (rxcd->vlan_id_l_1);
#endif
}

/* rxq */
struct yk3_rxc_stats_sw {
	u64 num_interrupt;
	u64 num_schedule;
	u64 num_handler;
	u64 num_unicast_desc;
	u64 num_unicast_pkt;
	u64 num_multicast_desc;
	u64 num_multicast_pkt;
	u64 num_broadcast_desc;
	u64 num_broadcast_pkt;
	u64 num_vlan_8021ad;
	u64 num_vlan_8021q;
	u64 num_vlan_remove;
	u64 num_lro_desc;
	u64 num_lro_pkt;
	u64 num_chkcpl_desc;
	u64 num_chkcpl_pkt;
	u64 num_csum_unchk[16];
	u64 num_ptp_pkt;
	u64 err_nopage;
	u64 err_rcvsize;
	u64 err_fcs_desc;
	u64 err_fcs_pkt;
	u64 err_mtu_desc;
	u64 err_mtu_pkt;
	u64 err_edma_desc;
	u64 err_edma_pkt;
	u64 err_ol3_csum_desc;
	u64 err_ol3_csum_pkt;
	u64 err_ol4_csum_desc;
	u64 err_ol4_csum_pkt;
	u64 err_il3_csum_desc;
	u64 err_il3_csum_pkt;
	u64 err_il4_csum_desc;
	u64 err_il4_csum_pkt;
	u64 err_pktcutoff_desc;
	u64 err_pktcutoff_pkt;
	u64 err_pkttimeo_desc;
	u64 err_pkttimeo_pkt;
	u64 err_unknown_desc;
	u64 err_alloc_skb;
	u64 err_build_skb;
	u64 err_gather;
	u64 err_csum_pkt;	/* total csum err pkt num */
	u64 num_csum_pkt;	/* total csum good pkt num */
	u64 err_rxcd_head;
	u64 err_rxcd_descid;
	u64 num_watchdog;
};

struct yk3_rxc_stats_rss_indir {
	u64 num_rss_indir_idx[256];
};

struct yk3_rx_stats_sw {
	u64 err_alloc_page;
	u64 err_map_page;
};

struct yk3_page;
struct yk3_rxi {
	struct yk3_page *page;
	u16 fragidx;
	struct sk_buff *skb;
};

struct yk3_rxcq {
	struct yk3_ringbase rxcdrb;
	struct yk3_rxcd *rxcd;
	struct yk3_rxq *rxq;
	__le16 *rxcd_head;
	struct yk3_stats_base stats_base;

	/* more stats */
	struct yk3_rxc_stats_sw stats_sw;
	struct yk3_rxc_stats_rss_indir stats_rss_indir;

	/* config params */
	u32 irq_vector;
	u32 irq_period;
	u32 irq_coal;
	u32 period;
	u32 coal;
	u32 irq_disable;

	/* head dma */
	dma_addr_t rxcd_dma_addr;
	dma_addr_t rxc_head_dma_addr;

	void __iomem *hw_addr;
	struct napi_struct napi;

	/* two-gear ITR state */
	u64 itr_ts;
	u64 itr_bytes;
	u8 itr_gear;

	/* timer */
	struct timer_list watchdog;
};

struct yk3_rxq {
	/* rx rxc rxi */
	struct yk3_ringbase rxdrb;
	struct yk3_rxd *rxd;
	struct yk3_rxi *rxi;

	struct device *dev;
	struct yk3_rxcq *rxcq;
	struct yk3_pagepool *pp;

	struct yk3_rx_stats_sw stats_sw;

	/* hw addr */
	void __iomem *hw_addr;

	/* config params */
	u32 qdepth;
	u32 qfragsize;
	u32 fragorder;

	/* property */
	struct yk3_queueid qid;
	u16 active:1;
	u16 qdepth_max_power;
	u32 qdepth_max;
	u32 qfragsize_max;

	/* pointer */
	struct yk3_ndev_priv *ndev_priv;

	dma_addr_t rxd_dma_addr;

	/* debug */
	struct dentry *debugfs_info_file;
	struct dentry *debugfs_rxd_file;
	struct dentry *debugfs_rxcd_file;
};

static inline void yk3_rxcq_irq_enable(struct yk3_rxcq *rxcq)
{
	dma_wmb();	/* guarantee sequence */
	rxcq->irq_disable = 0;
	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_IRQ_DISABLE, 0);
}

static inline void yk3_rxcq_irq_disable(struct yk3_rxcq *rxcq)
{
	dma_wmb();	/* guarantee sequence */
	rxcq->irq_disable = 1;
	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_IRQ_DISABLE, 1);
}

static inline void yk3_rxq_doorbell(struct yk3_rxq *rxq)
{
	dma_wmb();	/* guarantee sequence */
	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_HEAD, yk3_ringb_head_orig(&rxq->rxdrb));
}

int yk3_create_rxq(struct yk3_ndev_priv *ndev_priv, u16 idx, u32 depth);
void yk3_destroy_rxq(struct yk3_rxq *rxq);
int yk3_activate_rxq(struct yk3_rxq *rxq);
void yk3_deactivate_rxq(struct yk3_rxq *rxq);
void yk3_clean_rxq(struct yk3_rxq *rxq);

/* txq */
struct yk3_tx_stats_sw {
	u64 num_smalltso;
	u64 num_bigtso;
	u64 num_txd;
	u64 num_txdummy;
	u64 num_txdfd;
	u64 num_txdld;
	u64 num_vlaninsert;
	u64 over_fragsize;
	u64 over_pktsize;
	u64 err_dmasg;
	u64 err_linearize;
	u64 err_scatter;
	u64 err_notxd;
	u64 num_qstop;
	u64 num_qwakeup;
	u64 num_aggr_pkts;
	u64 num_aggr_bytes;
	u64 num_aggr_xmit;
	u64 err_aggr_xmit;
	u64 err_aggr_pkts;
	u64 err_aggr_bytes;
};

struct yk3_txc_stats_sw {
	u64 num_freeskb;
	u64 num_interrupt;
	u64 num_schedule;
	u64 num_handler;
	u64 num_packets;
	u64 num_bytes;
	u64 err_txcd_head;
};

/* tx info */
/* todo : too big, need to optimize */
#define YK3_N_FLAG_NORMAL	BIT(0)
#define YK3_N_FLAG_AGGR		BIT(1)
#define YK3_N_FLAG_PTP		BIT(2)

struct yk3_txi {
	u64 flags;
	union {
		struct {
			struct sk_buff *skb;
			dma_addr_t addr;
			u64 len;
		} n;
		struct {
			struct sk_buff **skb;
			u16 num;
			struct page *page;
			dma_addr_t addr;
			u64 len;
		} a;
	};
} ____cacheline_aligned;

#if (PAGE_SIZE < 8192)
#define YK3_N_AGGR_PAGEPARAM	2
#elif (PAGE_SIZE < 16384)
#define YK3_N_AGGR_PAGEPARAM	1
#else
#define YK3_N_AGGR_PAGEPARAM	0
#endif
#define YK3_N_AGGR_PKT_MAXNUM	32
#define YK3_N_AGGR_PKT_MAXSIZE	256
#define YK3_M_AGGR_ALIGN(x)	ALIGN(x, 64)
#define YK3_M_AGGR_ADSIZE(x)	ALIGN((x) << 3, 64)

struct yk3_aggr {
	struct kmem_cache *cache;
	struct sk_buff **skb;
	struct page *page;
	dma_addr_t dma_addr;
	u16 num;
	u16 data_size;
	u64 data_bytes;
};

struct yk3_txq {
	/* tx & txc & txi */
	struct yk3_ringbase txdrb;
	union {
		struct yk3_txd *txd;
		struct yk3_atxd *atxd;
		struct yk3_ctld *ctld;
	};
	struct yk3_txi *txi;

	struct device *dev;
	struct yk3_stats_base stats_base;

	/* hw addr */
	void __iomem *hw_addr;

	/* more stats */
	struct yk3_tx_stats_sw stats_sw;

	/* config params */
	u16 qgroup:5;
	u16 tc:3;
	u16 qgroup_request;
	u32 qdepth;
	u32 qfragsize;

	/* property */
	struct yk3_queueid qid;
	u16 active:1;
	u16 qdepth_max_power;
	u32 qdepth_max;
	u32 qfragsize_max;
	u32 qpktsize_max;
	u32 reserve;	/* for future use */

	/* pointer */
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_txcq *txcq;
	struct netdev_queue *tx_queue;

	dma_addr_t txd_dma_addr;
	/* debug */
	struct dentry *debugfs_info_file;
	struct dentry *debugfs_txd_file;

	/* scatter & aggr */
	struct yk3_scatter scatter;
	/* struct yk3_aggr aggr; */
	struct yk3_aggr aggr;
} ____cacheline_aligned;

struct yk3_txcq {
	/* txcd, txq */
	struct yk3_ringbase txcdrb;
	__le16 *txcd_head;
	struct yk3_txq *txq;

	struct yk3_txc_stats_sw stats_sw;

	/* config params */
	enum yk3_txcq_cpllen qcpllen;
	u32 irq_vector;
	u32 coal;
	u32 period;
	u32 irq_disable;

	/* head dma */
	dma_addr_t txc_head_dma_addr;

	void __iomem *hw_addr;
	struct napi_struct napi;

	/* two-gear ITR state */
	u64 itr_ts;
	u64 itr_bytes;
	u8 itr_gear;
} ____cacheline_aligned;

struct yk3_edma_qpair {
	struct yk3_txq *txq;
	struct yk3_txcq *txcq;
	struct yk3_rxq *rxq;
	struct yk3_rxcq *rxcq;
};

static inline void yk3_txcq_irq_enable(struct yk3_txcq *txcq)
{
	dma_wmb();	/* guarantee sequence */
	txcq->irq_disable = 0;
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_IRQ_DISABLE, 0);
}

static inline void yk3_txcq_irq_disable(struct yk3_txcq *txcq)
{
	dma_wmb();	/* guarantee sequence */
	txcq->irq_disable = 1;
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_IRQ_DISABLE, 1);
}

static inline void yk3_txq_doorbell(struct yk3_txq *txq)
{
	dma_wmb();	/* guarantee sequence */
	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_HEAD, yk3_ringb_head_orig(&txq->txdrb));
}

static inline bool yk3_txq_is_full(struct yk3_txq *txq)
{
	return (yk3_ringb_left(&txq->txdrb) < 8);
}

int yk3_create_txq(struct yk3_ndev_priv *ndev_priv, u16 idx, u32 depth);
void yk3_destroy_txq(struct yk3_txq *txq);
int yk3_activate_txq(struct yk3_txq *txq);
void yk3_deactivate_txq(struct yk3_txq *txq);
void yk3_clean_txq(struct yk3_txq *txq);
void yk3_txcq_set_coal(struct yk3_txcq *txcq, u16 packets, u16 usecs);
void yk3_rxcq_set_coal(struct yk3_rxcq *rxcq, u16 packets, u16 usecs);

#define YK3_N_PP_PAGEFRAG_MAX	(32 * 8)
#define YK3_N_PP_GROUPSIZE	(4 * 1024 * 1024 / PAGE_SIZE)
#define YK3_N_PP_PAGEGROUP_MAX	(512)
#define YK3_N_PP_FRAGSIZE_MAX	(4096)

struct yk3_page {
	u16 gidx;
	u16 pidx;
	u16 freenum;
	u16 maxnum;
	u16 power;
	u16 reserve[3];
	struct page *page;
	dma_addr_t dma_addr;
	DECLARE_BITMAP(freebits, YK3_N_PP_PAGEFRAG_MAX);
};

struct yk3_pagegroup {
	u16 gidx;
	u16 freenum;
	u32 freefragnum;
	struct yk3_ringbase rb;
	struct yk3_page pages[YK3_N_PP_GROUPSIZE];
};

struct yk3_stat {
	u32 total_page;
	u32 used_page;
	u32 total_frag;
	u32 used_frag;

	u64 num_recycle;
	u64 num_page_recycle;
	u64 num_create_group;
	u64 num_destroy_group;

	u64 err_page_init;
};

struct yk3_pagepool {
	struct yk3_pdev_priv *pdev_priv;
	struct device *dev;

	u16 groupnum;
	u16 fragpower;
	u16 pagefragmax;
	u16 resv;

	struct yk3_stat stat;

	DECLARE_BITMAP(emptybits, YK3_N_PP_PAGEGROUP_MAX);
	DECLARE_BITMAP(freebits, YK3_N_PP_PAGEGROUP_MAX);
	struct yk3_pagegroup *groups[YK3_N_PP_PAGEGROUP_MAX];
};

static inline void *yk3_page_addr(struct yk3_page *page, u32 fidx)
{
	return page_address(page->page) + (fidx << page->power);
}

static inline dma_addr_t yk3_page_dma_addr(struct yk3_page *page, u32 fidx)
{
	return page->dma_addr + (fidx << page->power);
}

static inline u32 yk3_page_size(struct yk3_page *page)
{
	return (1 << page->power);
}

static inline struct page *yk3_page_page(struct yk3_page *page)
{
	return page->page;
}

static inline u32 yk3_page_offset(struct yk3_page *page, u32 fidx)
{
	return (fidx << page->power);
}

static inline u32 yk3_pp_size(struct yk3_pagepool *pp)
{
	return (1 << pp->fragpower);
}

struct yk3_pagepool *yk3_pp_create(struct yk3_pdev_priv *pdev_priv, u16 size);
void yk3_pp_destroy(struct yk3_pagepool *pp);
void yk3_pp_recycle(struct yk3_pagepool *pp);
int yk3_pp_alloc(struct yk3_pagepool *pp, struct yk3_page **page_ret);
void yk3_pp_free(struct yk3_pagepool *pp, struct yk3_page *page, u16 fidx);
void yk3_pp_debugfs_show(struct seq_file *seq, struct yk3_pagepool *pp);

#endif /* _YK3_EDMA_PRIV_H */
