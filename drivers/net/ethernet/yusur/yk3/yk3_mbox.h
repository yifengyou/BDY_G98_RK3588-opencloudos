/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_MBOX_H_
#define __YK3_MBOX_H_

#include "yk3.h"

#define YK3_MBOX_MSG_LEN			128
#define YK3_MBOX_DATA_LEN			112
#define YK3_MBOX_MAX_CHANNEL			0x800
#define YK3_MBOX_EMP_ID				(0x180 + 25)
#define YK3_MBOX_MASTER_PF_ID			0x100

struct yk3_mbox_id {
	u16 func_id : 9;
	u16 type : 1;
	u16 gateway : 6;
};

#if !K3_IN_K2U
enum {
	MB_VF = 0,
	MB_PF,
	MB_EMP,
	MB_MASTER,
};

enum {
	MB_VF_TYPE = 0,
	MB_PF_OR_EMP_TYPE,
};

enum yk3_mbox_reply_mode {
	MB_NO_REPLY = 0,
	MB_WAIT_REPLY = 1
};
#endif

enum {
	YK3_MBOX_SEND_OK = 0,
	YK3_MBOX_SEND_L2_FAILED,
	YK3_MBOX_SEND_L3_NO_ACK,
};

enum {
	YK3_MBOX_OPCODE_MASK_COMMAND = 0,
	YK3_MBOX_OPCODE_MASK_ACK = 15,
};

enum {
	YK3_MBOX_OPCODE_IRQ_UNREGISTER       = 0x00,
	YK3_MBOX_OPCODE_TEST                 = 0x01,

	YK3_MBOX_OPCODE_SET_TX_INFO          = 0x02,
	YK3_MBOX_OPCODE_SET_RX_INFO          = 0x04,
	YK3_MBOX_OPCODE_SET_TX_ASSIGNMENT    = 0x06,
	YK3_MBOX_OPCODE_SET_RX_ASSIGNMENT    = 0x08,
	YK3_MBOX_OPCODE_SET_VLAN	     = 0x09,
	YK3_MBOX_OPCODE_GET_EEPROM_MAC       = 0x0A,
	YK3_MBOX_OPCODE_SET_MAC              = 0x0B,
	YK3_MBOX_OPCODE_UPD_VF_MAC           = 0x0C,
	YK3_MBOX_OPCODE_SET_VF_MAC           = 0x0D,
	YK3_MBOX_OPCODE_SET_MTU              = 0x0E,
	YK3_MBOX_OPCODE_SET_RSS_HASH         = 0x0F,
	YK3_MBOX_OPCODE_INIT_NDEV	     = 0x10,
	YK3_MBOX_OPCODE_SET_RXFH             = 0x11,
	YK3_MBOX_OPCODE_GET_HASH_MODE        = 0x12,
	YK3_MBOX_OPCODE_SET_NDEV_FEATURES    = 0x13,
	YK3_MBOX_OPCODE_SET_NDEV_FLAGS       = 0x14,
	YK3_MBOX_OPCODE_SET_VLAN_FEATURES    = 0x15,
	YK3_MBOX_OPCODE_SET_ETHTOOL_PRIV     = 0x16,
	YK3_MBOX_OPCODE_SET_ALL_MC	     = 0x17,
	YK3_MBOX_OPCODE_UMD_VF_ENABLE	     = 0x18,
	YK3_MBOX_OPCODE_UMD_SET_PVID	     = 0x19,
	YK3_MBOX_OPCODE_UMD_SET_SPOOFCHK     = 0x1A,
	YK3_MBOX_OPCODE_UMD_RESET_VF_CONF    = 0x1B,
	YK3_MBOX_OPCODE_QSET                 = 0x22,
	YK3_MBOX_OPCODE_EDMA                 = 0x23,
	YK3_MBOX_OPCODE_SET_FILTER           = 0x24,
	YK3_MBOX_OPCODE_SET_PRIV             = 0x2A,
	YK3_MBOX_OPCODE_SET_PORT_STATUS      = 0x2C,
	YK3_MBOX_OPCODE_GET_PIO_RES          = 0x2E,
	YK3_MBOX_OPCODE_DEL_VF_CFG           = 0x30,
	YK3_MBOX_OPCODE_VF_ETHTOOL_STATS     = 0x31,
	YK3_MBOX_OPCODE_PF_TO_PF             = 0x32,
	YK3_MBOX_OPCODE_NP_OPT               = 0x34,
	YK3_MBOX_OPCODE_RSS_INDIRECT         = 0x36,
	YK3_MBOX_OPCODE_GET_PF_MAC_INFO      = 0x37,
	YK3_MBOX_OPCODE_SET_VF_MAC_INFO      = 0x38,
	YK3_MBOX_OPCODE_GET_EMP_INFO         = 0x39,
	YK3_MBOX_OPCODE_QOS                  = 0x40,
	YK3_MBOX_OPCODE_EME_SET_RULE         = 0x41,
	YK3_MBOX_OPCODE_EME_SET_TABLE        = 0x42,
	YK3_MBOX_OPCODE_SET_UDP_TUNNEL       = 0x43,
	YK3_MBOX_OPCODE_SET_VF_PORT_INFO     = 0x44,
	YK3_MBOX_OPCODE_FWD_TO_PF_PORT_INFO  = 0x45,
	YK3_MBOX_OPCODE_PF_SET_IFG           = 0x46,
	/* interaction with EMP */
	YK3_MBOX_OPCODE_EMP_TEMP_ALARM       = 0x0400,
	YK3_MBOX_OPCODE_EMP_VOL_ALARM        = 0x0401,
	YK3_MBOX_OPCODE_EMP_ECC_ALARM        = 0x0402,
	YK3_MBOX_OPCODE_EMP_I2C_ALARM        = 0x0403,
	YK3_MBOX_OPCODE_EMP_CHIP_ALARM       = 0x0404,
	YK3_MBOX_OPCODE_EMP_ENABLE_BLINK     = 0x0411,
	YK3_MBOX_OPCODE_EMP_DISABLE_BLINK    = 0x0412,
	YK3_MBOX_OPCODE_EMP_GET_MODULE_BASIC = 0x0413,
	YK3_MBOX_OPCODE_EMP_GET_EERPOM       = 0x0414,
	YK3_MBOX_OPCODE_EMP_GET_UMAC_STATUS  = 0x0415,
	YK3_MBOX_OPCODE_EMP_ENABLE_LOOPBACK  = 0x0416,
	YK3_MBOX_OPCODE_EMP_DISABLE_LOOPBACK = 0x0417,
	YK3_MBOX_OPCODE_EMP_SET_SPEED        = 0x0418,
	YK3_MBOX_OPCODE_EMP_GET_SPEED        = 0x0419,
	YK3_MBOX_OPCODE_EMP_SET_FEC          = 0x041A,
	YK3_MBOX_OPCODE_EMP_GET_FEC          = 0x041B,
	YK3_MBOX_OPCODE_EMP_SET_PFC          = 0x041C,
	YK3_MBOX_OPCODE_EMP_SET_PFC_FILTER   = 0x041D,
	YK3_MBOX_OPCODE_EMP_SET_FC           = 0x041E,
	YK3_MBOX_OPCODE_EMP_SET_FC_FILTER    = 0x041F,
	YK3_MBOX_OPCODE_EMP_SET_PAUSE_SRCMAC = 0x0420,
	YK3_MBOX_OPCODE_LINK_DOWN_ON_CLOSE   = 0x0421,
	YK3_MBOX_OPCODE_EMP_SET_LLDP_MODE    = 0x0430,
	YK3_MBOX_OPCODE_EMP_GET_LLDP_MODE    = 0x0431,
	YK3_MBOX_OPCODE_EMP_REPORT_XCVR_STS  = 0x0432,
	YK3_MBOX_OPCODE_EMP_SET_IFG          = 0x0433,
};

struct yk3_mbox_msg {
	u16 magic;
	u16 opcode;
	u8 data_length;
	u8 data_chksum;
	u16 dst_id;
	u16 src_id;
	u16 flag;
	s32 seqno;
	u8 data[YK3_MBOX_DATA_LEN];
};

struct yk3_mbox_option {
	enum yk3_mbox_reply_mode wait_reply;
	u32 timeout;
};

#define YK3_MBOX_SRC_ID_NULL			0xfefe
static inline u16 yk3_mbox_get_src_vf_id(u16 src_id)
{
	struct yk3_mbox_id *id;

	id = (struct yk3_mbox_id *)&src_id;
	if (id->type == MB_VF_TYPE)
		return id->func_id;
	else
		return YK3_MBOX_SRC_ID_NULL;
}

static inline u16 yk3_mbox_get_src_pf_id(u16 src_id)
{
	struct yk3_mbox_id *id;

	id = (struct yk3_mbox_id *)&src_id;
	if (id->type == MB_PF_OR_EMP_TYPE)
		if (id->func_id != YK3_MBOX_EMP_ID)
			return id->func_id;

	return YK3_MBOX_SRC_ID_NULL;
}

static inline u16 yk3_mbox_master_id(void)
{
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;

	mb_id->type = MB_PF_OR_EMP_TYPE;
	mb_id->func_id = YK3_MBOX_MASTER_PF_ID;

	return id;
}

static inline u16 yk3_mbox_pf_id(u16 pf_id)
{
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;

	mb_id->type = MB_PF_OR_EMP_TYPE;
	mb_id->func_id = pf_id;

	return id;
}

static inline u16 yk3_mbox_vf_id(u16 vf_id)
{
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;

	mb_id->type = MB_VF_TYPE;
	mb_id->func_id = vf_id;

	return id;
}

static inline u16 yk3_mbox_emp_id(void)
{
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;

	mb_id->type = MB_PF_OR_EMP_TYPE;
	mb_id->func_id = YK3_MBOX_EMP_ID;

	return id;
}

int yk3_mbox_init(struct yk3_pdev_priv *pdev_priv);
void yk3_mbox_exit(struct yk3_pdev_priv *pdev_priv);
int yk3_mbox_register_callback(struct yk3_pdev_priv *pdev_priv,
			       u16 opcode,
			       void (*callback)(struct yk3_mbox_msg *msg, void *param),
			       void *param);
void yk3_mbox_unregister_callback(struct yk3_pdev_priv *pdev_priv, u16 opcode);
int yk3_mbox_send_msg_atomic(struct yk3_pdev_priv *pdev_priv, struct yk3_mbox_msg *send_msg);
int yk3_mbox_send_msg(struct yk3_pdev_priv *pdev_priv,
		      struct yk3_mbox_msg *send_msg,
		      struct yk3_mbox_option *option,
		      struct yk3_mbox_msg *recv_msg);
u32 ys_mbox_sysfs_send(struct pci_dev *pdev, const char *buf);

#endif /* __YK3_MBOX_H_ */
