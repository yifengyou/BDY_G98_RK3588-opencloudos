/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Chen Jisi <chenjisi@dapustor.com>
 *
 * Interactive configuration with the controller
 */

#ifndef __DN200_CTRL_H__
#define __DN200_CTRL_H__
// #include "common.h"
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/reset.h>
#include <net/page_pool/helpers.h>
#include <net/page_pool/types.h>
#include <linux/clk-provider.h>
#include <linux/pci.h>
#include <linux/dmi.h>
#include <linux/delay.h>
#include <linux/if.h>

#define SQ_DEPTH 32

#define IRQ_INFO_PFVF_RELEASE 0x22
#define GET_FUNCID 0x32
#define IRQ_INFO_NOTICE 0x3a
#define IRQ_INFO_CONFIG 0xc2
#define REG_WRITE 0x8
#define REG_READ 0x0
#define PCB_TYPE 0x55
#define RJ45_TYPE 0x65
#define FW_UP_COMMIT 0x6d
#define FW_UP_STATUS 0x75
#define FW_SLOT_GET 0x7d
#define LRAM_RXP_LOCK 0x1a
#define FW_I2C_RW 0x92
#define FW_PHY_RST 0x9a
#define FW_PHY_EYE 0xba
#define FW_PWM_CTRL 0xca
#define FW_STATE_GET 0x5d

#define ADMIN_TIMEOUT 1
#define ADMIN_MAX_WAIT_TIME (2 * 1000)
#define ADMIN_IRQ_RELEASE_MSEC 100
#define DN200_POLL_CQ_MAX_TIMES 8000000
#define DN200_POLL_CQ_MAX_TIMES_ATOMIC 100000

#define CSTS_NSSRO BIT(4)
#define CTRL_CAP_CSS_CSI BIT(6)
#define CTRL_CC_CSS_CSI (6 << 4)
#define CTRL_CC_CSS_NVM (0 << 4)
#define CTRL_CC_IOSQES (6 << 16)
#define CTRL_CC_IOCQES (4 << 20)
#define CTRL_CC_ENABLE BIT(0)
#define CTRL_CC_SHN_MASK (3 << 14)
#define CTRL_CC_SHN_NORMAL BIT(14)
#define CAP_STRIDE(cap) (((cap) >> 32) & 0xf)
#define CTRL_MAX_SEG_LEN 0x1000
#define MAX_NVME_NUM 512
#define MAX_FW_CMD_LEN 120
#define DN200_NVME_PASSTHRU _IOWR('D', 0x10, struct dn200_user_passth_command)
#define DN200_NVME_GET_CARD_INFO _IOR('D', 0x11, struct dn200_card_info)
#define DN200_NVME_GET_FW_CMD_LOG _IOWR('D', 0x12, struct dn200_user_passth_command)

#define dn200_admin_set_features 0x9
#define DN200_FEAT_TIMESTAMP 0x0e
#define DN200_VER_CNS 0x1

#define DN200_FW_UPGRADE_COMIT_IDLE		(0)
#define DN200_FW_UPGRADE_COMIT_DOING	(1)
#define DN200_FW_UPGRADE_COMIT_FINISH	(2)

struct dn200_ver {
	u8 type;
	u8 product_type;
	u8 rsv;
	u8 is_fw;
	u8 publish;
	u8 number0;
	u8 number1;
	u8 number2;
};

enum {
	REG_READ_WAIT = 0,
	REG_WRITE_WAIT,
	REG_READ_FAST,
	REG_WRITE_FAST,
};

union ctrl_data_ptr {
	struct {
		__le64 prp1;
		__le64 prp2;
	};
};

struct ctrl_features {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 nsid;
	__u64 rsvd2[2];
	union ctrl_data_ptr dptr;
	__le32 fid;
	__le32 dword11;
	__le32 dword12;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

struct ctrl_identify {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 nsid;
	__u64 rsvd2[2];
	union ctrl_data_ptr dptr;
	__le32 cns;
	__le32 dword11;
	__le32 dword12;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

enum ctrl_admin_dn200_opcode {
	ctrl_admin_activate_fw = 0x10,
	ctrl_admin_download_fw = 0x11,
	ctrl_admin_vendor_start = 0xC0,
	ctrl_admin_prod_info_set = 0xC1,
	ctrl_admin_prod_info_get = 0xC2,
	ctrl_admin_fw_cmd_write = 0xCD,
	ctrl_admin_fw_cmd_read = 0xCE,
	ctrl_admin_fw_identify = 0x6,
	ctrl_admin_eeprom_write = 0xD9,
	ctrl_admin_eeprom_read = 0xDA,
};

struct dn200_reg_rw_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 nsid;
	__le32 cdw3;
	__le32 cdw4;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 ndt;
	__le32 ndm;
	__le32 set;
	__le32 addr;
	__le32 value;
	__le32 mask;
};

struct dn200_irq_share_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 nsid;
	__le32 cdw3;
	__le32 cdw4;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 ndt;
	__le32 ndm;
	__le32 set;
	__le32 tx_q;
	__le32 rx_q;
	__le32 dword15;
};

struct dn200_irq_release_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 nsid;
	__le32 cdw3;
	__le32 cdw4;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 ndt;
	__le32 ndm;
	__le32 set;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

struct dn200_fw_download_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 cdw1;
	__le32 cdw2;
	__le32 cdw3;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 numd;
	__le32 ofst;
	__le32 dword12;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

struct dn200_fw_commit_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 cdw1;
	__le32 cdw2;
	__le32 cdw3;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 dword10;
	__le32 dword11;
	__le32 dword12;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

struct dn200_fw_i2c_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 cdw1;
	__le32 cdw2;
	__le32 cdw3;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 dword10;
	__le32 dword11;
	__le32 set;
	__le32 dword13;
	__le32 dword14;
	__le32 dword15;
};

struct dn200_common_command {
	__u8 opcode;
	__u8 flags;
	__u16 command_id;
	__le32 cdw1;
	__le32 cdw2;
	__le32 cdw3;
	__le64 metadata;
	union ctrl_data_ptr dptr;
	__le32 cdw10;
	__le32 cdw11;
	__le32 cdw12;
	__le32 cdw13;
	__le32 cdw14;
	__le32 cdw15;
};

struct dn200_user_passth_command {
	__u8	opcode;
	__u8	flags;
	__u16	rsvd1;
	__u32	cdw1;
	__u32	cdw2;
	__u32	cdw3;
	__u64	metadata;
	__u64	addr;
	__u32	metadata_len;
	__u32	data_len;
	__u32	cdw10;
	__u32	cdw11;
	__u32	cdw12;
	__u32	cdw13;
	__u32	cdw14;
	__u32	cdw15;
	__u32	timeout_ms;
	__u32   rsvd2;
	__u64	result;
};

struct ctrl_command {
	union {
		struct ctrl_identify ctrl_identify;
		struct ctrl_features features;
		struct dn200_reg_rw_command rw_command;
		struct dn200_irq_share_command irq_share_command;
		struct dn200_irq_release_command irq_release_command;
		struct dn200_fw_download_command fw_download_command;
		struct dn200_fw_commit_command fw_commit_command;
		struct dn200_fw_i2c_command fw_i2c_command;
		struct dn200_common_command common_command;
	};
};

enum {
	REG_CAP = 0x0000,	/* Controller Capabilities */
	REG_VS = 0x0008,	/* Version */
	REG_INTMS = 0x000c,	/* Interrupt Mask Set */
	REG_INTMC = 0x0010,	/* Interrupt Mask Clear */
	REG_CC = 0x0014,	/* Controller Configuration */
	REG_CSTS = 0x001c,	/* Controller Status */
	REG_NSSR = 0x0020,	/* NVM Subsystem Reset */
	REG_AQA = 0x0024,	/* Admin Queue Attributes */
	REG_ASQ = 0x0028,	/* Admin SQ Base Address */
	REG_ACQ = 0x0030,	/* Admin CQ Base Address */
	REG_CMBLOC = 0x0038,	/* Controller Memory Buffer Location */
	REG_CMBSZ = 0x003c,	/* Controller Memory Buffer Size */
	REG_BPINFO = 0x0040,	/* Boot Partition Information */
	REG_BPRSEL = 0x0044,	/* Boot Partition Read Select */
	REG_BPMBL = 0x0048,	/* Boot Partition Memory Buffer* Location */
	REG_CMBMSC = 0x0050,	/* Controller Memory Buffer Memory* Space Control */
	REG_PMRCAP = 0x0e00,	/* Persistent Memory Capabilities */
	REG_PMRCTL = 0x0e04,	/* Persistent Memory Region Control */
	REG_PMRSTS = 0x0e08,	/* Persistent Memory Region Status */
	REG_PMREBS = 0x0e0c,	/* Persistent Memory Region Elasticity* Buffer Size */
	REG_PMRSWTP = 0x0e10,	/* Persistent Memory Region Sustained* Write Throughput */
	REG_DBS = 0x1000,	/* SQ 0 Tail Doorbell */
};

struct ctrl_completion {
	/* Used by Admin and Fabrics commands to return data: */
	union ctrl_result {
		__le16 u16;
		__le32 u32;
		__le64 u64;
	} result;
	__le16 sq_head;		/* how much of this queue may be reclaimed */
	__le16 sq_id;		/* submission queue that generated this entry */
	__u16 command_id;	/* of the command which completed */
	__le16 status;		/* did the command fail, and if so, why? */
};

struct dn200_ctrl_resource {
	struct device *dev;
	struct ctrl_completion *cqes;
	dma_addr_t sq_dma_addr;
	dma_addr_t cq_dma_addr;
	void *sq_cmds;
	u64 cap;
	u32 q_depth;
	u16 cq_vector;
	u16 sq_tail;
	u16 last_sq_tail;
	u16 cq_head;
	u16 qid;
	u8 cq_phase;
	u8 sqes;
	u32 db_stride;
	u8 funcid;
	void __iomem *dbs;
	void __iomem *bar;
	unsigned long flags;
	struct msix_entry *msix_entries;
	struct timer_list ctrl_timer;
	const struct dn200_ctrl_ops *ctrl_ops;
	char itr_name_ctrl[IFNAMSIZ + 9];
	char itr_name_peer_notify[IFNAMSIZ + 9];
	char itr_name_upgrade[IFNAMSIZ + 9];
	const struct admin_process_ops *dn200_admin_process_ops;
	u32 rdata[SQ_DEPTH];
	spinlock_t lock; /* dn200 ctrl spinlock. */
	struct mutex mlock;
	unsigned long admin_state;
	u8 addr64;
	dev_t devt;
	struct cdev cdev;
	bool pcie_ava;
	bool is_extern_phy;
};

enum admin_queue_state {
	ADMIN_QUEUE_IDLE,
	ADMIN_QUEUE_CQ_NO_RETURN,
	ADMIN_QUEUE_INITED,
	ADMIN_UP_GRADE_FLAG,
};

struct dn200_card_info {
	u32 link;
	u32 speed;
	u32 duplex;
	u32 type;
	char eth_name[64];
	char eth_temp[64];
};

struct admin_process_ops {
	void (*dn200_admin_process)(struct dn200_ctrl_resource *ctrl,
			 struct ctrl_command *c, int *ret_val, u32 *cq_val);
};

int ctrl_reset(struct dn200_ctrl_resource *ctrl, bool reset_irq);
int admin_queue_configure(struct pci_dev *pdev,
			  struct dn200_ctrl_resource *ctrl, bool is_purepf, bool is_extern_phy);
void shutdown_ctrl(struct dn200_ctrl_resource *ctrl);
int irq_info_pfvf_release(struct pci_dev *pdev,
			  struct dn200_ctrl_resource *ctrl, bool need_retry);
int dn200_ena_msix_range(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl,
			 int tx_queues_to_use, int rx_queues_to_use,
			 bool is_purepf);
int irq_queue_map(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl,
		  int tx_queues_to_use, int rx_queues_to_use, bool need_retry);
int dn200_ctrl_ccena(struct pci_dev *pdev, bool off, bool on, bool is_atomic);
int fw_reg_write(struct dn200_ctrl_resource *ctrl, u32 reg, u32 value);
int fw_reg_read(struct dn200_ctrl_resource *ctrl, u32 reg, u32 *value);
int irq_peer_notify(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl);
int dn200_ctrl_res_free(struct pci_dev *pdev, struct dn200_ctrl_resource *ctrl);
int get_pcb_type(struct dn200_ctrl_resource *ctrl, u32 *value);
int get_rj45_type(struct dn200_ctrl_resource *ctrl, u32 *value);
int lram_and_rxp_lock_and_unlock(struct dn200_ctrl_resource *ctrl, u32 value,
				 u32 *ret_val);
int dn200_nvme_fw_load(struct dn200_ctrl_resource *ctrl, const char *fw,
		       size_t fw_size);
int dn200_nvme_fw_commit(struct dn200_ctrl_resource *ctrl);
int dn200_fw_i2c_rw_commit(struct dn200_ctrl_resource *ctrl, u32 dev_addr,
			   u8 *value, u32 offset, bool rw);
int fw_phy_set(struct dn200_ctrl_resource *ctrl, bool type);
int dn200_nvme_product_info_get(struct dn200_ctrl_resource *ctrl, void *info, u32 len);
int dn200_dev_temp_get(struct dn200_ctrl_resource *ctrl, void *info, u32 len);
void dn200_register_nvme_device(struct dn200_ctrl_resource *ctrl);
void dn200_unregister_nvme_device(struct dn200_ctrl_resource *ctrl);
int dn200_led_blink_ctrl(struct dn200_ctrl_resource *ctrl, bool is_enable);
int fw_link_state_set(struct dn200_ctrl_resource *ctrl, u8 link_state, u8 duplex, u32 speed);
int dn200_configure_timestamp(struct dn200_ctrl_resource *ctrl);
int dn200_get_fw_ver(struct dn200_ctrl_resource *ctrl, struct dn200_ver *dn200_ver);
int dn200_eeprom_read(struct dn200_ctrl_resource *ctrl, u32 offset, u32 len, u8 *data);
int dn200_eeprom_write(struct dn200_ctrl_resource *ctrl, u32 offset, u32 len, u8 *data);
int ctrl_reinitial(struct dn200_ctrl_resource *ctrl);
#endif
