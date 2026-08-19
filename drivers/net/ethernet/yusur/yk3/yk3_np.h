/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_NP_H_
#define __YK3_NP_H_

#include <linux/types.h>

#include "yk3_base.h"

int yk3_np_init(struct yk3_pdev_priv *pdev_priv);
void yk3_np_exit(struct yk3_pdev_priv *pdev_priv);

int yk3_np_set_tbl_ready(struct yk3_pdev_priv *pdev_priv, bool ready);
int yk3_np_set_emp_meter(struct yk3_pdev_priv *pdev_priv, bool enable);
int yk3_np_set_simple_forward(struct yk3_pdev_priv *pdev_priv, bool enable);

int yk3_doe_set_protect(struct yk3_pdev_priv *pdev_priv, bool protect);

enum YK3_NP_REG_MODULE {
	YK3_NP_REG_M_PPE_CLUSTER = 0,
	YK3_NP_REG_M_INGRESS_MGR,
	YK3_NP_REG_M_EGRESS_MGR,
	YK3_NP_REG_M_PACKET_BUF,
	YK3_NP_REG_M_NP_GOBAL_CTRL,
	YK3_NP_REG_M_NP_BIST,
	YK3_NP_REG_M_META,
	YK3_NP_REG_M_LOG_PACKETER,
	YK3_NP_REG_M_TIMER,
	YK3_NP_REG_M_MQBUF,
	YK3_NP_REG_M_PCIE_MSIX,
	YK3_NP_REG_M_MONITOR_CNT,
	YK3_NP_REG_M_SENSOR,
	YK3_NP_REG_M_UNIT,
	YK3_NP_REG_M_MAX,
};

/*
 * Note:
 * There're hunders of register fields in the following enum.
 * Fields listed here are usd by other modules so far.
 * Feel free to add more at request.
 */
enum YK3_NP_REG_FIELD {
/* PPE module */
	/* Cluster Queue */

	/* Cluster Control */
	YK3_NP_REG_F_HOST_FW_MSG0_L = 0,
	YK3_NP_REG_F_HOST_FW_MSG0_H,
	YK3_NP_REG_F_HOST_FW_MSG1_L,
	YK3_NP_REG_F_HOST_FW_MSG1_H,
	YK3_NP_REG_F_HOST_FW_MSG2_L,
	YK3_NP_REG_F_HOST_FW_MSG2_H,

	/* Cross TRIG & IRQ */

	/* Debug Control */

	/* MGR bus monitor */

	/* PPE */

	/* Cluster Program RAM */

	/* Share Mem */
	YK3_NP_REG_F_SHM_ATOM,

	/* Unit mon cnt */

/* Ingress Manager */

/* Engress Manager */

/* Packet Buffer */

/* NP Gobal Control */

/* BIST */

/* Meta Ingress Egres */

/* Log Packeter */

/* Timer */

/* MQBUF */

/* PCIE-MSIX */

/* MONITOR CNT */

/* Sensor */

	YK3_NP_REG_F_MAX,
};

u32 yk3_np_rd32(struct yk3_pdev_priv *pdev_priv, u32 offset_to_np);
void yk3_np_wr32(struct yk3_pdev_priv *pdev_priv, u32 offset_to_np, u32 val);

int yk3_np_mrd32(struct yk3_pdev_priv *pdev_priv,
		 enum YK3_NP_REG_MODULE module,
		 u16 module_elem_idx,
		 u32 offset_to_module,
		 u32 *val);
int yk3_np_mwr32(struct yk3_pdev_priv *pdev_priv,
		 enum YK3_NP_REG_MODULE module,
		 u16 module_elem_idx,
		 u32 offset_to_module,
		 u32 val);

int yk3_np_frd32(struct yk3_pdev_priv *pdev_priv,
		 u16 module_elem_idx,
		 enum YK3_NP_REG_FIELD field_type,
		 u32 *val);
int yk3_np_fwr32(struct yk3_pdev_priv *pdev_priv,
		 u16 module_elem_idx,
		 enum YK3_NP_REG_FIELD field_type,
		 u32 val);
/*
 * NP share memory write function.
 * Due to hw limit, only support 2 bytes addr/val.
 * @return 0 on success, others on fail.
 * Note:
 *   No share memory read function due to hw limit.
 */
int yk3_np_swr16(struct yk3_pdev_priv *pdev_priv,
		 u16 cls_id,
		 u16 shm_addr,
		 u16 val);
#endif /* __YK3_NP_H_ */
