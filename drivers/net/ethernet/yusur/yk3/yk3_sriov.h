/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_SRIOV_H
#define _YK3_SRIOV_H

#include "yk3_base.h"

struct yk3_vf_info {
	u16 vf_idx;
	u16 qsetid;
	u8 mac_addr[ETH_ALEN];
	struct yk3_queuebase p_qbase;
	__be16 vf_vlan_tpid;
	u16 vf_vlan;
	u8 vf_vlan_qos;
	u8 spoofchk;
	u8 trusted;
	u32 ndev_flags;
	u32 min_tx_rate;
	u32 max_tx_rate;
	bool umd_enable;
};

struct yk3_sriov_priv {
	int num_vfs;
	DECLARE_BITMAP(state, 32);
	struct yk3_vf_info *vf_info;
};

struct yk3_sriov_mbox_umd_vf_msg {
	u16 vf_id;
	u16 enable;
};

#define YK3_SRIOV_MBOX_OK		0
#define YK3_SRIOV_MBOX_SRIOV_BUSY	1
#define YK3_SRIOV_MBOX_INVALID_VF	2

struct yk3_sriov_mbox_ack_msg {
	u32 ret;
};

static inline struct yk3_vf_info *
yk3_sriov_get_vfinfo(struct yk3_sriov_priv *priv, u16 vfidx)
{
	return (vfidx < priv->num_vfs) ? &priv->vf_info[vfidx] : NULL;
}

int yk3_sriov_configure(struct pci_dev *pdev, int num_vfs);

int yk3_sriov_init(struct yk3_pdev_priv *pdev_priv);
void yk3_sriov_exit(struct yk3_pdev_priv *pdev_priv);

struct yk3_sriov_priv *yk3_sriov_get_priv(struct yk3_pdev_priv *pdev_priv);
void yk3_sriov_put_priv(struct yk3_sriov_priv *priv);

#endif /* _YK3_SRIOV_H */
