/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __YK3_MBOX_PRIV_H_
#define __YK3_MBOX_PRIV_H_

#include <linux/pci.h>
#include <linux/kfifo.h>
#include "yk3.h"
#include "yk3_mbox.h"

/* hardware related */
#define YK3_MBOX_BAR				YK3_BAR0
#define YK3_MBOX_IRQ				0
#define YK3_MBOX_BASE				(yk3_mbox_base)
#define YK3_MBOX_DPU_HOST_BASE			0x300000
#define YK3_MBOX_DPU_SOC_BASE			0x380000
#define YK3_MBOX_EMP_PF_ID			(0x180 + 25)
#define YK3_MBOX_MASTER_PF_ID			0x100

#define YK3_MBOX_PF2VF_IRQ_TRIGGER		(YK3_MBOX_BASE + 0x40)
#define YK3_MBOX_PF2VF_IRQ_VECTOR		(YK3_MBOX_BASE + 0x44)
#define YK3_MBOX_PF2VF_IRQ_PENDING		(YK3_MBOX_BASE + 0x48)
#define YK3_MBOX_PF2VF_IRQ_P_STATUS		GENMASK(27, 27)
#define YK3_MBOX_PF2VF_IRQ_P_VF_ID		GENMASK(8, 0)
#define YK3_MBOX_PF2VF_IRQ_P_VECTOR		GENMASK(26, 15)

#define YK3_MBOX_G_REG				(YK3_MBOX_BASE + 0x1000)
#define YK3_MBOX_G_BUF_SIZE			(YK3_MBOX_G_REG + 0x00)
#define YK3_MBOX_G_TIMEOUT_ENABLE		(YK3_MBOX_G_REG + 0x20)
#define YK3_MBOX_G_TIMEOUT_CNT			(YK3_MBOX_G_REG + 0x24)
#define YK3_MBOX_G_EMP_IRQ_OUT_CNT		(YK3_MBOX_G_REG + 0x28)
#define YK3_MBOX_G_MAILBOX_VERSION		(YK3_MBOX_G_REG + 0x30)
#define YK3_MBOX_G_MAILBOX_INIT_DONE		(YK3_MBOX_G_REG + 0x38)
#define YK3_MBOX_G_PF_IRQ_TIME_OUT_ALARM	(YK3_MBOX_G_REG + 0x50)
#define YK3_MBOX_G_VF_IRQ_TIME_OUT_ALARM	(YK3_MBOX_G_REG + 0x54)
#define YK3_MBOX_G_EMP_IRQ_TIME_OUT_ALARM	(YK3_MBOX_G_REG + 0x58)
#define YK3_MBOX_G_TIMEOUT			5000000

#define YK3_MBOX_PF_BASE			(YK3_MBOX_BASE + 0x2000)
#define YK3_MBOX_MASTER_PREEMPT			(YK3_MBOX_PF_BASE + 0x00)
#define YK3_MBOX_MASTER_SEL			BIT(0)
#define YK3_MBOX_MASTER_OPTION			(YK3_MBOX_PF_BASE + 0x04)
#define YK3_MBOX_LF_START			0
#define YK3_MBOX_LF_END				8
#define YK3_MBOX_LFX_MEM_OFFSET(lf_id)		(YK3_MBOX_PF_BASE + 0x8 + (lf_id) * 4)
#define YK3_MBOX_LFX_MEM_PF_ID			GENMASK(21, 16)
#define YK3_MBOX_LFX_MEM_ADDR			GENMASK(15, 0)
#define YK3_MBOX_PF2PF_IRQ_TRIGGER		(YK3_MBOX_PF_BASE + 0x30)
#define YK3_MBOX_PF2PF_IRQ_VECTOR		(YK3_MBOX_PF_BASE + 0x34)
#define YK3_MBOX_PF2PF_IRQ_PENDING		(YK3_MBOX_PF_BASE + 0x38)
#define YK3_MBOX_PF2PF_IRQ_P_PF_ID		GENMASK(8, 0)
#define YK3_MBOX_PF2PF_IRQ_P_STATUS		GENMASK(27, 27)
#define YK3_MBOX_PF2PF_IRQ_P_VECTOR		GENMASK(26, 15)

#define YK3_MBOX_H2S_IRQ_TRIGGER		(YK3_MBOX_PF_BASE + 0x40)
#define YK3_MBOX_S2H_IRQ_VECTOR			(YK3_MBOX_PF_BASE + 0x44)

#define YK3_MBOX_H2M_IRQ_TRIGGER		(YK3_MBOX_PF_BASE + 0x50)
#define YK3_MBOX_M2H_IRQ_VECTOR			(YK3_MBOX_PF_BASE + 0x54)

#define YK3_MBOX_VF_BASE			(YK3_MBOX_BASE + 0x8000)
#define YK3_MBOX_VF_IRQ_TRIGGER			(YK3_MBOX_VF_BASE + 0x0)
#define YK3_MBOX_VF_IRQ_VECTOR			(YK3_MBOX_VF_BASE + 0x4)
#define YK3_MBOX_VF_IRQ_PENDING			(YK3_MBOX_VF_BASE + 0x8)
#define YK3_MBOX_VF2PF_IRQ_P_STATUS		GENMASK(27, 27)
#define YK3_MBOX_VF2PF_IRQ_P_VF_ID		GENMASK(8, 0)
#define YK3_MBOX_VF2PF_IRQ_P_VECTOR		GENMASK(26, 15)

#define YK3_MBOX_VF_PF_BUF_BASE			(YK3_MBOX_BASE + 0x10000)

#define YK3_MBOX_VF_PF_BUF_OFFSET(vf_id)	(YK3_MBOX_VF_PF_BUF_BASE + 0x100 * (vf_id))

#define YK3_MBOX_PF2PF_BUF_BASE			(YK3_MBOX_BASE + 0x30000)

#define YK3_MBOX_PF2PF_BUF_OFFSET(pf_id)	(YK3_MBOX_PF2PF_BUF_BASE + 0x100 * (pf_id))

#define YK3_MBOX_M2EMP_CHN			(YK3_MBOX_PF2PF_BUF_BASE + 0x900)
#define YK3_MBOX_M2M_CHN			(YK3_MBOX_PF2PF_BUF_BASE + 0xa00)

#define YK3_MBOX_MAX_PF				64
#define YK3_MBOX_MAX_VF				511

#define YK3_MBOX_SH_BUF_SIZE			0x100
#define YK3_MBOX_SH_BUF_TH			0x0
#define YK3_MBOX_SH_BUF_BH			0x80

#define YK3_MBOX_DEF_VAL			0x0
#define YK3_MBOX_ERR_VAL			0xdeaddec2 //0xdeadbed2
#define YK3_MBOX_DEF_VAL_ERR_MSG		"default value error"
#define YK3_MBOX_VFS_NUM			64

#define YK3_MBOX_LF0_LF1_VF_NUM			(YK3_MBOX_BASE + 0x1060)
#define YK3_MBOX_LF2_LF3_VF_NUM			(YK3_MBOX_BASE + 0x1064)
#define YK3_MBOX_LF4_LF5_VF_NUM			(YK3_MBOX_BASE + 0x1068)
#define YK3_MBOX_LF6_LF7_VF_NUM			(YK3_MBOX_BASE + 0x106c)
#define YK3_MBOX_LF0_LF1_REMAP_PF_ID		(YK3_MBOX_BASE + 0x1070)
#define YK3_MBOX_LF2_LF3_REMAP_PF_ID		(YK3_MBOX_BASE + 0x1074)
#define YK3_MBOX_LF4_LF5_REMAP_PF_ID		(YK3_MBOX_BASE + 0x1078)
#define YK3_MBOX_LF6_LF7_REMAP_PF_ID		(YK3_MBOX_BASE + 0x107C)
#define YK3_MBOX_LF8_REMAP_PF_ID		(YK3_MBOX_BASE + 0x1080)
#define YK3_MBOX_HOST_IRQ_CNT			(YK3_MBOX_BASE + 0x1090)
#define YK3_MBOX_EMP_MASTER_IRQ_CNT		(YK3_MBOX_BASE + 0x1094)
#define YK3_MBOX_EMP_LF_IRQ_CNT			(YK3_MBOX_BASE + 0x1098)
#define YK3_MBOX_TRIG_VF2PF_IRQ_CNT		(YK3_MBOX_BASE + 0x109C)
#define YK3_MBOX_TRIG_PF2VF_IRQ_CNT		(YK3_MBOX_BASE + 0x10A0)
#define YK3_MBOX_TRIG_PF2PF_IRQ_CNT		(YK3_MBOX_BASE + 0x10A4)
#define YK3_MBOX_TRIG_EMP_MASTER_IRQ_CNT	(YK3_MBOX_BASE + 0x10A8)
#define YK3_MBOX_TRIG_EMP_LF_IRQ_CNT		(YK3_MBOX_BASE + 0x10AC)
#define YK3_MBOX_HOST_SOC_SEL			(YK3_MBOX_BASE + 0x10B0)
#define YK3_MBOX_APB_TRIG_ADDR1			(YK3_MBOX_BASE + 0x10B4)
#define YK3_MBOX_APB_TRIG_ADDR2			(YK3_MBOX_BASE + 0x10B8)
#define YK3_MBOX_APB_WDATA			(YK3_MBOX_BASE + 0x10BC)
#define YK3_MBOX_MAILBOX_FIFO_EMPTY		(YK3_MBOX_BASE + 0x10C0)
#define YK3_MBOX_MAILBOX_FIFO_FULL		(YK3_MBOX_BASE + 0x10C4)
#define YK3_MBOX_IRQ_OUT_DATA			(YK3_MBOX_BASE + 0x10C8)
#define YK3_MBOX_PF_VF_IRQ_FLAG0		(YK3_MBOX_BASE + 0x10CC)
#define YK3_MBOX_PF_VF_IRQ_FLAG1		(YK3_MBOX_BASE + 0x10D0)
#define YK3_MBOX_PF_VF_IRQ_FLAG2		(YK3_MBOX_BASE + 0x10D4)
#define YK3_MBOX_PF_VF_IRQ_FLAG3		(YK3_MBOX_BASE + 0x10D8)
#define YK3_MBOX_PF_VF_IRQ_FLAG4		(YK3_MBOX_BASE + 0x10DC)
#define YK3_MBOX_PF_VF_IRQ_FLAG5		(YK3_MBOX_BASE + 0x10E0)
#define YK3_MBOX_PF_VF_IRQ_FLAG6		(YK3_MBOX_BASE + 0x10E4)
#define YK3_MBOX_PF_VF_IRQ_FLAG7		(YK3_MBOX_BASE + 0x10E8)
#define YK3_MBOX_PF_VF_IRQ_FLAG8		(YK3_MBOX_BASE + 0x10EC)
#define YK3_MBOX_PF_VF_IRQ_FLAG9		(YK3_MBOX_BASE + 0x10F0)
#define YK3_MBOX_PF_VF_IRQ_FLAG10		(YK3_MBOX_BASE + 0x10F4)
#define YK3_MBOX_PF_VF_IRQ_FLAG11		(YK3_MBOX_BASE + 0x10F8)
#define YK3_MBOX_PF_VF_IRQ_FLAG12		(YK3_MBOX_BASE + 0x10FC)
#define YK3_MBOX_PF_VF_IRQ_FLAG13		(YK3_MBOX_BASE + 0x1100)
#define YK3_MBOX_PF_VF_IRQ_FLAG14		(YK3_MBOX_BASE + 0x1104)
#define YK3_MBOX_PF_VF_IRQ_FLAG15		(YK3_MBOX_BASE + 0x1108)

/* for driver */
#define YK3_MBOX_MAGIC_DATA			0x1f47
#define YK3_MBOX_REPLY_TIMEOUT			1000
#define YK3_MBOX_UNLOCK_TIMEOUT			200
#define YK3_MBOX_FLAG_NO_REPLY_BIT		0
#define YK3_MBOX_FLAG_NO_REPLY			GENMASK(0, 0)
#define YK3_MBOX_FLAG_NO_CHKSUM_BIT		1
#define YK3_MBOX_FLAG_NO_CHKSUM			GENMASK(1, 1)
#define YK3_MBOX_CB_NUM				256
#define YK3_MBOX_EMP_CB_NUM			64
#define YK3_MBOX_EMP_CB_START			1024
#define YK3_MBOX_WAIT_REPLY_SIZE		64
#define YK3_MBOX_FIFO_SIZE			16

struct yk3_mbox_offset_ctx {
	u32 offset;
	u32 trigger_offset;
	u32 trigger_id;
};

struct yk3_mbox_priv {
	u32 pf2lf_table[YK3_MBOX_MAX_PF];
};

struct yk3_mbox_irq_info {
	u32 vf_num;
	u32 pf_num;
	u16 msg_id;
	u32 vector;
	u32 irq_status;
	u32 pf_triger;
};

struct yk3_mbox_wait_reply_node {
	struct hlist_node node;
	struct completion comp;
	u16 opcode;
	s32 seqno;
};

/* must be power of 2 */
#define RB_SIZE									(32)
#define RB_LINE_SIZE								(256)
#define RB_MASK									(RB_SIZE - 1)
#define dbgf(f, arg...)								\
{										\
	u64 ns = ktime_get_ns();						\
	unsigned long sec  = ns / 1000000000;					\
	unsigned long usec = (ns % 1000000000) / 1000;				\
	int __idx = atomic_fetch_add(1, &pdev_priv->mbox->stats.cur) & RB_MASK;	\
	sprintf(pdev_priv->mbox->stats.buffer[__idx],				\
		"[%5lu.%06lu] "f, sec, usec, ##arg);				\
}

struct yk3_mbox {
	struct pci_dev *pdev;
	int role;
	void *mb_priv;
	void __iomem *addr;

	/* debugfs */
	struct dentry *dbgfs_info_file;
	struct {
		atomic_t cur;
		char buffer[RB_SIZE][RB_LINE_SIZE];
		atomic_t err_locked;
		atomic_t err_not_take;
		atomic_t err_resp_timeout;
		u32 err_invalid_msg;
		u32 err_irq_lost;

		atomic_t cnt_tx;
		u32 cnt_rx;
		u32 cnt_fwd;
		u32 cnt_atomic;
	} stats;

	/* atomic_fifo_lock */
	spinlock_t atomic_fifo_lock;
	DECLARE_KFIFO(atomic_fifo, struct yk3_mbox_msg, YK3_MBOX_FIFO_SIZE);
	struct work_struct work_atomic;

	/* per-channel mlock */
	struct mutex chn_mlock[YK3_MBOX_MAX_CHANNEL];
	DECLARE_BITMAP(chn_bitmap, YK3_MBOX_MAX_CHANNEL);
	struct work_struct work_bh;

	DECLARE_KFIFO(req_fifo, struct yk3_mbox_msg, YK3_MBOX_FIFO_SIZE);
	struct work_struct work_req;

	DECLARE_KFIFO(resp_fifo, struct yk3_mbox_msg, YK3_MBOX_FIFO_SIZE);
	struct work_struct work_resp;
	struct hlist_head wait_reply_hlist[YK3_MBOX_WAIT_REPLY_SIZE];
	/* wait_reply_mlock */
	struct mutex wait_reply_mlock;
	struct yk3_mbox_msg resp_msg[YK3_MBOX_WAIT_REPLY_SIZE];

	DECLARE_KFIFO(fwd_fifo, struct yk3_mbox_msg, YK3_MBOX_FIFO_SIZE);
	struct work_struct work_fwd;

	void (*opcode_cb[YK3_MBOX_CB_NUM])(struct yk3_mbox_msg *, void *);
	void *opcode_cb_param[YK3_MBOX_CB_NUM];
	atomic_t opcode_cb_run_cnt[YK3_MBOX_CB_NUM];
	DECLARE_BITMAP(opcode_cb_stopping_bitmap, YK3_MBOX_CB_NUM);
	atomic_t opcode_tx_req_cnt[YK3_MBOX_CB_NUM];
	atomic_t opcode_tx_resp_cnt[YK3_MBOX_CB_NUM];
	atomic_t opcode_rx_req_cnt[YK3_MBOX_CB_NUM];
	atomic_t opcode_rx_resp_cnt[YK3_MBOX_CB_NUM];
};
#endif /* __YK3_MBOX_PRIV_H_ */
