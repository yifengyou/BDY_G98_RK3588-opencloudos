/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _YK3_ESW_H
#define _YK3_ESW_H

#include "yk3.h"
#include "yk3_lan_regs.h"

#define YK3_ESW_QSET_BITMAP	32
#define YK3_ESW_QSET_BITMAP_BITS (YK3_ESW_QSET_BITMAP * 32)

struct yk3_esw_manager {
	u32 hw_type;
	u32 nic_mode;
	void __iomem *hw_base;
#ifdef ALL_MC_FEATURE
	u32 all_mc_qset[16]; // only k3 chip need driver support all mc feature
#endif
	/* mac table */
	u32 uc_key_used;
	u32 uc_qbmp_used;
	u32 uc_qbmp_idx_res[YK3_LAN_ESW_MAC_QSET_BMP_NUMB]; // uc qset bitmap max number 2048
	struct list_head l2uc_key_list;
	struct list_head l2uc_qbmp_list;
	u32 mc_key_used;
	u32 mc_qbmp_used;
	u32 mc_qbmp_idx_res[YK3_LAN_ESW_MAC_QSET_BMP_NUMB]; // mc qset bitmap max number 2048
	struct list_head l2mc_key_list;
	struct list_head l2mc_qbmp_list;
	struct yk3_esw_cuckoo_table *lan_mac_table;
	/* vlan table */
	struct yk3_esw_vlan_entry *vlan_entrys; // 8K vlan entry = 802.1Q 4K + 802.1ad 4K
	u32 vlan_qbmp_num;
	u32 vlan_qbmp_used;
	u32 vlan_qbmp_index[128]; // vlan qset bitmap number max is 4096
	struct list_head vlan_qset_bmp_list;
	/* resource limit */
	u32 uc_key_limit;
	u32 mc_key_limit;
	u32 uc_qbmp_base;
	u32 uc_qbmp_limit;
	u32 mc_qbmp_base;
	u32 mc_qbmp_limit;
	/* debug counter */
	/* debugfs */
	struct dentry *dbgfs_esw_dir;
	struct dentry *dbgfs_esw_mac_file;
	struct dentry *dbgfs_esw_vlan_file;
};

struct yk3_esw_mac_filter_hw {
	u8 mac[ETH_ALEN];
	u16 qset_bmp_idx : 11;
	u16 enable : 1;
	u16 rsvd : 4;
};

struct yk3_esw_mac_filter {
	u8 mac[ETH_ALEN];
	u32 qset_bmp_idx;
	u32 ref_cnt;
	u8 qset_ref[YK3_ESW_QSET_BITMAP_BITS];
	u32 bitmap[YK3_ESW_QSET_BITMAP];
	struct list_head key_node;
};

struct yk3_esw_qset_bitmap {
	u32 bitmap[YK3_ESW_QSET_BITMAP];
	u32 index;
	u32 ref_cnt;
	struct list_head qbmp_node;
};

struct yk3_esw_vlan_entry {
	u32 bitmap[YK3_ESW_QSET_BITMAP];
	int qbmp_idx;
	u32 member_cnt;
};

struct yk3_esw_vlan_qbmp {
	u32 bitmap[YK3_ESW_QSET_BITMAP];
	int index;
	u32 ref_cnt;
	struct list_head vlan_qbmp_node;
};

int yk3_init_esw(struct yk3_pdev_priv *pdev_priv);
int yk3_uninit_esw(struct yk3_pdev_priv *pdev_priv);

int yk3_esw_add_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset);
int yk3_esw_del_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset);
int yk3_esw_upd_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *old_mac, u8 *new_mac, u16 qset);
int yk3_esw_add_mc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset);
int yk3_esw_del_mc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset);

int yk3_esw_add_vlan(struct yk3_pdev_priv *pdev_priv, u16 vid, __be16 tpid, u16 qset);
int yk3_esw_del_vlan(struct yk3_pdev_priv *pdev_priv, u16 vid, __be16 tpid, u16 qset);
#ifdef ALL_MC_FEATURE
int yk3_esw_set_all_mc(struct yk3_pdev_priv *pdev_priv, u16 qset, bool enable);
#endif

#endif /* _YK3_ESW_H */
