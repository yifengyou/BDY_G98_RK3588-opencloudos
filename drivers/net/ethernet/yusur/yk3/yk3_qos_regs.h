/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_QOS_REGS_H
#define _YK3_QOS_REGS_H

#define YK3_MAX_RATE		25000  //25G in Mbps
#define QOS_TRAFFIC_CLASS	8

/* Do not change the order. */
enum {
	QOS_BUCKET_MODE_SHAPER,
	QOS_BUCKET_MODE_SCHEDLER,
	QOS_BUCKET_MODE_UNUSED,
	QOS_BUCKET_MODE_DCB,	//double color bucket
	QOS_BUCKET_MODE_MAX,
};

#define	QOS_PORT_LAYER_ID	4
#define	QOS_TC_LAYER_ID		3
#define	QOS_GROUP_LAYER_ID	2
#define	QOS_QSET_LAYER_ID	1
#define	QOS_QUEUE_LAYER_ID	0

#define QOS_L4_PORT_NUM		32
#define QOS_L3_TC_NUM		32
#define QOS_L2_GROUP_NUM	32
#define QOS_L1_QSET_NUM		512
#define QOS_L0_QUEUE_NUM	1024

#define QOS_L4_NODE_HEAD	1
#define QOS_L3_NODE_HEAD	(QOS_L4_NODE_HEAD + QOS_L4_PORT_NUM)  //33
#define QOS_L2_NODE_HEAD	(QOS_L3_NODE_HEAD + QOS_L3_TC_NUM)  //65
#define QOS_L1_NODE_HEAD	(QOS_L2_NODE_HEAD + QOS_L2_GROUP_NUM)  //97
#define QOS_L0_NODE_HEAD	(QOS_L1_NODE_HEAD + QOS_L1_QSET_NUM)  //609
#define QOS_TOTAL_NODE_NUM	(QOS_L0_NODE_HEAD + QOS_L0_QUEUE_NUM)  //1633

#define QOS_RDMA_QP_OFFSET	512
#define QOS_RDMA_QP_MAXNUM	512

#define YK3_QOS_BASE_ADDR	0x800000	//TODO

/* global reg */
#define QOS_BLOBAL_REG_BASE	0
#define QOS_DATA_OFF(i)		((i) << 3)
#define QOS_MASK_OFF(i)		(0x20 + ((i) << 2))
#define QOS_ADDR_OFF		0x40
#define QOS_VALID_OFF		0x44
#define QOS_RDWR_STATUS_OFF	0x48
#define QOS_READ_DATA		0x4C

#define QOS_PULSE_SWITCH	0xE4
#define QOS_GLOBAL_ENABLE	0xE8
#define QOS_SYS_CLK_PERIOD	0xF8

#define QOS_PRE_CREDIT0		0x6C
#define QOS_PRE_CREDIT1		0x124
#define QOS_PRE_CREDIT2		0x128
#define QOS_PRE_CREDIT3		0x12C

/* root reg */
#define QOS_ROOT_REG_BASE	0x4000

/* stream context: 0:edma host 1:np 2:edma soc 3:rdma */
#define QOS_ST_CONTEXT_0(i)	(0x4080 + ((i) * 4))
#define QOS_ST_CONTEXT_1(i)	(0x40A0 + ((i) * 4))
#define QOS_ST_CONTEXT_2(i)	(0x40C0 + ((i) * 4))
#define QOS_ST_CONTEXT_3(i)	(0x40E0 + ((i) * 4))
#define QOS_ST_CONTEXT_EN	0x4100

/* 0:edma host 1:np 2:edma soc 3:rdma */
#define QOS_MASK_ARRAY_0	0x4104
#define QOS_MASK_ARRAY_1	0x4108
#define QOS_MASK_ARRAY_2	0x410c
#define QOS_MASK_ARRAY_3	0x4110

/* routing dest */
#define QOS_RT_DEST_MASK	0xff
#define QOS_RT_DEST_OFFSET	0x4114
#define QOS_ROUTINGDEST(i)	(QOS_RT_DEST_OFFSET + (i) * 4)

#define QOS_RTD_QRAGE_MASK	0xffff
#define QOS_RTD_QRANGE_OFFSET	0x4154
#define QOS_RTD_QRANGE(i)	(QOS_RTD_QRANGE_OFFSET + (i) * 4)

/* resource management */
#define QOS_RESC_BASE		(QOS_ROOT_REG_BASE + 0x3000)
#define QOS_PFC_RESC_EN		(QOS_RESC_BASE + 0xf4)
#define QOS_PFC_BUF_ALL		(QOS_RESC_BASE + 0xf8)

#define QOS_PFC_BUF_COUNT	32
#define QOS_PFC_BUF_N(n)	(QOS_RESC_BASE + 0xfc + (n) * 4)
#define QOS_PFC_PRE_SLICE_N(n)	(QOS_RESC_BASE + 0x17c + (n) * 4)
#define QOS_PFC_SLICE_PROC_N(n)	(QOS_RESC_BASE + 0x1fc + (n) * 4)

#define QOS_DATA_SLICE_COUNT	33
#define QOS_DATA_SLICE_HOST_N(n)(QOS_RESC_BASE + 0x27c + (n) * 4)
#define QOS_DATA_SLICE_SOC_N(n)	(QOS_RESC_BASE + 0x300 + (n) * 4)

/* leaf reg */
#define QOS_LEAF_REG_BASE	0x8000
#define QOS_LEAF_CAP		0x8004
#define QOS_LEAF_REG_SEL	0x8008
#define QOS_LEAF_REG_WSTRB	0x800c

#define QOS_LEAF_EN_FIELD	GENMASK(4, 4)
#define QOS_LEAF_PORT_FIELD	GENMASK(3, 0)

enum qos_leaf_port {
	QOS_LEAF_PORT_HOST = 0,
	QOS_LEAF_PORT_NP   = 1,
	QOS_LEAF_PORT_SOC  = 2,  //dpu
	QOS_LEAF_PORT_RDMA = 0,  //k3max
};

#define QOS_LEAF_N_FILTER_PORT	0x83C0
#define QOS_LEAF_N_QUEUE_CFG(queue)	(0x8400 + ((queue) * 4))

/* level reg */
#define QOS_LEVEL_REG_BASE(level)	(0x18000 + ((level) * 0x4000))

/* Do not change the order. */
enum {
	QOS_CLASSID_MODE_NOCHANGE,
	QOS_CLASSID_MODE_QID = 1,
	QOS_CLASSID_MODE_PORT,
	QOS_CLASSID_MODE_TC,
	QOS_CLASSID_MODE_PORT_TC,
};

/* Do not change the order. */
enum {
	QOS_BUFFID_MODE_NOCHANGE,
	QOS_BUFFID_MODE_QUEUE,
	QOS_BUFFID_MODE_PORT,
	QOS_BUFFID_MODE_TC,
};

#define QOS_CLASSID_MODE_OFFSET(level)	(0 + QOS_LEVEL_REG_BASE(level))
#define QOS_BUFF_ID_MODE_OFFSET(level)	(4 + QOS_LEVEL_REG_BASE(level))
#define QOS_DISTRIBUT_EN_OFFSET(level)	(0x19c + QOS_LEVEL_REG_BASE(level))
#define QOS_EXTSWITCH_EN_OFFSET(level)	(0x30c4 + QOS_LEVEL_REG_BASE(level))
#define QOS_EIR_BASE_OFFSET(level)	(0x3098 + QOS_LEVEL_REG_BASE(level))

struct qos_node_entry {
	u64 data[4];
	u32 mask[8];
	u32 addr;
	u32 valid;
};

struct qos_sched_entry {
	u8 weight0;
	u8 weight1;
	u8 weight2;
	u8 weight3;
	u8 weight4;
	u8 weight5;
	u8 weight6;
	u8 weight7;

	u8 sp_en;

	u8 entry_en	:1;  //1bit [72:72]
	u8 work_type	:2;  //1bit [73:74]
	u8 color_en	:1;  //1bit [75:75]
	u8 a_group	:1;  //1ibt [76:76]
	u8 always_irri	:1;  //1bit [77:77]
	u8 self_irri	:1;  //1bit [78:78]
	u8 granule	:1;  //1bit [79:79]

	u8 be_en;
	u8 wrr_priority;
};

struct yk3_qos_node {
	struct yk3_qos *qos;

	struct yk3_qos_node *parent;
	struct yk3_qos_node **children;
	struct qos_node_entry entry;

	u16 layer_id;
	u16 node_id;

	u32 min_rate;  //used for check
	u32 max_rate;  //used for check

	u8 prio_slot;  //next buffer id
	u8 sched_mode;  //shaper:0/scheduler:1/unused:2/dcb:3
	u8 in_use;  //flow enable

	u16 num_child;
	u8 owner;	//edma:0,rdma:1
};

/* IEEE 802.1Qaz TSA values (matching IEEE_8021QAZ_TSA_* from dcbnl.h) */
#define ETS_TSA_STRICT	0	/* Strict Priority */
#define ETS_TSA_ETS	2	/* Enhanced Transmission Selection (WRR) */

#define ETS_BW_DEFAULT	100

struct yk3_qos {
	struct pci_dev *pdev;
	void __iomem *hw_base;
	u32 qos_clk;
	u8 leaf_num;
	u16 qnum_per_leaf;

	spinlock_t lock;	/* protect node tree */
	struct yk3_qos_node *root;
	u32 maxrate_ratio;
	u32 link_speed;

	struct dentry *dbgfs_info_file;
};

#define QOS_ATTR_TC_ID_GM			GENMASK(2, 0)
#define QOS_ATTR_PORT_ID_GM			GENMASK(5, 3)
#define QOS_ATTR_NEXT_CLASS_ID_GM		GENMASK(21, 6)
#define QOS_ATTR_PFC_ID_GM			GENMASK(26, 22)

struct qos_queue_attr {
	union {
		u32 attr;
		struct {
			u32 tc_id		:3;
			u32 port_id		:3;
			u32 next_class_id	:16;
			u32 pfc_id		:5;
		};
	};
};

#define QOS_ENTRY_MASK_DFLT \
{ \
	0xffffffff, \
	0xffffffff, \
	0x00000fff, \
	0x00000000, \
	0x06000000, \
	0x00007000, \
	0x00000000, \
	0x00000000, \
}

#define BUCKET_LEVEL_BASE	7
#define SCHED_LEVEL_BASE	1

/* Scheduler entry bit fields - packed into data[0] and data[1] */
/* data[0]: 8 weights, 8 bits each */
#define SCHED_WEIGHT0_SHIFT	0
#define SCHED_WEIGHT0_MASK	GENMASK_ULL(7, 0)
#define SCHED_WEIGHT1_SHIFT	8
#define SCHED_WEIGHT1_MASK	GENMASK_ULL(15, 8)
#define SCHED_WEIGHT2_SHIFT	16
#define SCHED_WEIGHT2_MASK	GENMASK_ULL(23, 16)
#define SCHED_WEIGHT3_SHIFT	24
#define SCHED_WEIGHT3_MASK	GENMASK_ULL(31, 24)
#define SCHED_WEIGHT4_SHIFT	32
#define SCHED_WEIGHT4_MASK	GENMASK_ULL(39, 32)
#define SCHED_WEIGHT5_SHIFT	40
#define SCHED_WEIGHT5_MASK	GENMASK_ULL(47, 40)
#define SCHED_WEIGHT6_SHIFT	48
#define SCHED_WEIGHT6_MASK	GENMASK_ULL(55, 48)
#define SCHED_WEIGHT7_SHIFT	56
#define SCHED_WEIGHT7_MASK	GENMASK_ULL(63, 56)

/* data[1]: sp_en, control bits, be_en, wrr_priority */
#define SCHED_SP_EN_SHIFT	0
#define SCHED_SP_EN_MASK	GENMASK_ULL(7, 0)
#define SCHED_ENTRY_EN_SHIFT	8
#define SCHED_ENTRY_EN_MASK	GENMASK_ULL(8, 8)
#define SCHED_WORK_TYPE_SHIFT	9
#define SCHED_WORK_TYPE_MASK	GENMASK_ULL(10, 9)
#define SCHED_COLOR_EN_SHIFT	11
#define SCHED_COLOR_EN_MASK	GENMASK_ULL(11, 11)
#define SCHED_A_GROUP_SHIFT	12
#define SCHED_A_GROUP_MASK	GENMASK_ULL(12, 12)
#define SCHED_ALWAYS_IRRI_SHIFT	13
#define SCHED_ALWAYS_IRRI_MASK	GENMASK_ULL(13, 13)
#define SCHED_SELF_IRRI_SHIFT	14
#define SCHED_SELF_IRRI_MASK	GENMASK_ULL(14, 14)
#define SCHED_GRANULE_SHIFT	15
#define SCHED_GRANULE_MASK	GENMASK_ULL(15, 15)
#define SCHED_BE_EN_SHIFT	16
#define SCHED_BE_EN_MASK	GENMASK_ULL(23, 16)
#define SCHED_WRR_PRIO_SHIFT	24
#define SCHED_WRR_PRIO_MASK	GENMASK_ULL(31, 24)

#define QOS_SCHED_ENTRY_MASK \
{ \
	0xffffffff, \
	0xffffffff, \
	0xffffffff, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
}

/* word0 */
#define BUCKET_EN_SHIFT		0
#define BUCKET_EN_MASK		GENMASK_ULL(0, 0)

#define CIR_FACTOR_SHIFT	1
#define CIR_FACTOR_MASK		GENMASK_ULL(28, 1)

#define YFACTOR_SHIFT		29
#define YFACTOR_MASK		GENMASK_ULL(36, 29)

#define CBS_SHIFT		37
#define CBS_MASK		GENMASK_ULL(56, 37)

#define NEXT_BUF_ID_SHIFT	57
#define NEXT_BUF_ID_MASK	GENMASK_ULL(59, 57)

#define NEXT_CLS_ID_W0_SHIFT	60
#define NEXT_CLS_ID_W0_MASK	GENMASK_ULL(63, 60)
/* word1 */
#define NEXT_CLS_ID_W1_SHIFT	0
#define NEXT_CLS_ID_W1_MASK	GENMASK_ULL(11, 0)

/* word2 */
#define SCHED_MODE_SHIFT	25
#define SCHED_MODE_MASK		GENMASK_ULL(26, 25)

#define NEXT_LEVEL_ID_SHIFT	44
#define NEXT_LEVEL_ID_MASK	GENMASK_ULL(46, 44)

/* word3 not used */

#define TC_ID_SHIFT		0
#define TC_ID_MASK		GENMASK_ULL(2, 0)

#define PORT_ID_SHIFT		3
#define PORT_ID_MASK		GENMASK_ULL(5, 3)

#define PFC_ID_SHIFT		22
#define PFC_ID_MASK		GENMASK_ULL(26, 22)

enum yk3_qos_msgid {
	MSG_NDEV_TOPO_INIT,
	MSG_QUEUE_SET_RATE,
	MSG_FUNC_SET_RATE,
	MSG_FUNC_GET_RATE,
	MSG_DQISC_SET_TC_MAP,
	MSG_DQISC_SET_TC_MIN_RATE,
	MSG_DQISC_SET_TC_MAX_RATE,
	MSG_DQISC_CLEANUP_TC,		/* MQPRIO cleanup on qdisc deletion */
	MSG_QP_SET_TC,			/* RDMA QP to TC mapping */
	MSG_QP_SET_RATE,		/* RDMA QP rate limiting */
	MSG_QP_SET_RT_DST,
	MSG_SET_LINK_SPEED,
	MSG_SET_ETS,
	MSG_QOS_MAX,
};

struct tc_map {
	u8 num_tc;
	u8 prio_tc_map[QOS_TRAFFIC_CLASS];
	u16 count[QOS_TRAFFIC_CLASS];
	u16 offset[QOS_TRAFFIC_CLASS];
};

struct qos_rt_dst {
	u8 qrange_id;
	u32 qrange;
	u8 rt_dst_id;
	u32 rt_dst;
};

struct yk3_qos_msg {
	enum yk3_qos_msgid msgid;
	union {
		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					u32 qset_id;
					u32 g_qbase;
					u32 qnum;
				} req;
			};
		} ndev_init;

		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					u32 g_qid;
					u32 maxrate;
				} req;
			};
		} queue_rate;

		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					u32 qset_id;
					u32 minrate;
					u32 maxrate;
				} req;
			};
		} func_rate;

		struct {
			u32 pf_id:8;
			u32 g_qbase:16;
			struct tc_map tc_qopt;
		} tc;

		struct {
			u32 pf_id;
			u64 min_rate[QOS_TRAFFIC_CLASS];
		} tc_shaper_min;

		struct {
			u32 pf_id;
			u64 max_rate[QOS_TRAFFIC_CLASS];
		} tc_shaper_max;

		/* RDMA QP to TC mapping */
		struct {
			u32 qp_id;
			u32 tc_id;
			u32 pf_id;
		} qp_tc;

		/* RDMA QP rate limiting */
		struct {
			u32 qp_id;
			u32 maxrate;	/* Mbps */
			u32 pf_id;
		} qp_rate;

		struct qos_rt_dst rt_dst;

		/* MQPRIO cleanup */
		struct {
			u32 pf_id;
			u32 g_qbase;
			u32 qnum;
		} tc_cleanup;

		/* link speed */
		struct {
			u32 pf_id;
			u32 speed_mbps;
		} link_speed;

		/* ets */
		struct {
			u32 pf_id;
			struct yk3_ets_cfg ets;
		} ets_cfg;
	};
};

#endif
