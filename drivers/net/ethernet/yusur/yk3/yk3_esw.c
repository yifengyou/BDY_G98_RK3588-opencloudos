// SPDX-License-Identifier: GPL-2.0

#include "yk3_esw.h"
#include "yk3_esw_cuckoo.h"
#include "yk3_lan_regs.h"
#include "yk3_lan_priv.h"

#define YK3_HW_TYPE_K3		0
#define YK3_HW_TYPE_K3M		1

#define YK3_ESW_UNUSED_INDEX -1

#define YK3_ESW_MC_QBMP_INDEX_MIN (mc_qbmp_base)
#define YK3_ESW_MC_QBMP_INDEX_MAX (mc_qbmp_base + mc_qbmp_limit)

static inline int yk3_esw_iowrite32(void __iomem *base, u32 reg, u32 val)
{
	u32 retry = 3;

	do {
		yk3_wr32(base, reg, val);
		if (yk3_rd32(base, reg) == val)
			break;
	} while (retry--);

	if (retry == 0)
		return -1;

	return 0;
}

static inline int yk3_esw_bmp_set_bit(u32 *bitmap, u32 bmp_size, u32 qset, bool set)
{
	int j, k;

	j = qset / 32;
	k = qset % 32;

	if (j >= bmp_size / 32)
		return -1;

	if (set)
		bitmap[j] |= 1 << k;
	else
		bitmap[j] &= ~(1 << k);

	return 0;
}

#ifdef ALL_MC_FEATURE
static inline int yk3_esw_bmp_get_bit(u32 *bitmap, u32 bmp_size, u32 qset)
{
	int j, k;

	j = qset / 32;
	k = qset % 32;

	if (j >= bmp_size / 32)
		return -1;

	if (bitmap[j] & (1 << k))
		return 1;
	else
		return 0;
}

static inline void yk3_esw_bmp_merge(u32 *dst_bitmap, u32 *src_bitmap, u32 bmp_size)
{
	u32 i;

	for (i = 0; i < bmp_size / 32; i++)
		dst_bitmap[i] |= src_bitmap[i];
}

static inline int yk3_esw_bmp_get_bit_ref(u8 *ref_bmp, u32 bmp_size, u32 qset)
{
	if (qset >= bmp_size)
		return 0;

	if (ref_bmp[qset])
		return 1;
	else
		return 0;
}

static inline int yk3_esw_get_mc_qbmp(struct yk3_esw_manager *esw_mgr,
				      int qbmp_index,
				      u32 *bitmap)
{
	u32 i;
	void __iomem *entry_addr;

	entry_addr = esw_mgr->hw_base +
		YK3_LAN_ESW_MAC_QSET_BMP_ENTRY_ADDR(qbmp_index);
	if (esw_mgr->lan_mac_table->type == YK3_CUCKOO_TYPE_K3_LAN_MAC) {
		for (i = 0; i < YK3_LAN_ESW_MAC_QSET_ENTRY_LEN / 2 / 4; i++)
			bitmap[i] = yk3_rd32(entry_addr, i * sizeof(u32));
	} else {
		yk3_err("yk3 esw set mc qset bitmap with unknown table type %d!",
			esw_mgr->lan_mac_table->type);
		return -EINVAL;
	}

	return 0;
}
#endif

static inline int yk3_esw_bmp_set_bit_ref(u32 *bitmap, u8 *ref_bmp, u32 bmp_size,
					  u32 qset, bool set)
{
	int j, k;

	j = qset / 32;
	k = qset % 32;

	if (j >= bmp_size / 32)
		return -1;

	if (set) {
		if (ref_bmp[qset] == 0xff)
			return -1;
		if (ref_bmp[qset]++ != 0)
			return 1;
		bitmap[j] |= 1 << k;
	} else {
		if (ref_bmp[qset] == 0)
			return -1;
		if (--ref_bmp[qset] != 0)
			return 1;
		bitmap[j] &= ~(1 << k);
	}

	return 0;
}

static inline int yk3_esw_bmp_set_bit_ref_once(u32 *bitmap, u8 *ref_bmp, u32 bmp_size,
					       u32 qset, bool set)
{
	int j, k;

	j = qset / 32;
	k = qset % 32;

	if (j >= bmp_size / 32)
		return -1;

	if (set) {
		if (ref_bmp[qset] == 1)
			return 1;
		ref_bmp[qset] = 1;
		bitmap[j] |= 1 << k;
	} else {
		if (ref_bmp[qset] != 1)
			return -1;
		ref_bmp[qset] = 0;
		bitmap[j] &= ~(1 << k);
	}

	return 0;
}

static inline int yk3_esw_bmp_clear_bit(u32 *bitmap, u32 bmp_size, u32 qset)
{
	u32 j, k, tmp_bitmap;

	j = qset / 32;
	k = qset % 32;

	if (j >= bmp_size / 32)
		return -1;

	tmp_bitmap = bitmap[j] & (1 << k);
	if (tmp_bitmap == 0)
		return -1;
	bitmap[j] &= ~tmp_bitmap;

	return 0;
}

static inline int yk3_esw_bmp_find_first_zero_bit(u32 *bitmap, u32 bmp_size, bool zero)
{
	u32 i, j, k;

	j = bmp_size / 32;

	for (i = 0; i < j; i++) {
		for (k = 0; k < 32; k++) {
			if (zero) {
				if ((bitmap[i] & (1 << k)) == 0)
					return i * 32 + k;
			} else {
				if ((bitmap[i] & (1 << k)) > 0)
					return i * 32 + k;
			}
		}
	}

	return bmp_size;
}

static inline int yk3_esw_compare_qset_bmp(u32 *target_qbmp, u32 *qbmp, u32 len)
{
	if (len < 1 || len >= 4096)
		return -1;

	return memcmp(target_qbmp, qbmp, len * 4);
}

static inline int yk3_esw_set_mc_qbmp(struct yk3_esw_manager *esw_mgr,
				      struct yk3_esw_qset_bitmap *l2mc_qbmp)
{
	u32 i;
	void __iomem *entry_addr;

	entry_addr = esw_mgr->hw_base +
		YK3_LAN_ESW_MAC_QSET_BMP_ENTRY_ADDR(l2mc_qbmp->index);
	if (esw_mgr->lan_mac_table->type == YK3_CUCKOO_TYPE_K3_LAN_MAC) {
		for (i = 0; i < YK3_LAN_ESW_MAC_QSET_ENTRY_LEN / 2 / 4; i++) {
			if (yk3_esw_iowrite32(entry_addr, i * sizeof(u32),
					      l2mc_qbmp->bitmap[i]) != 0) {
				yk3_err("yk3 esw set mc qset bitmap failed!");
				return -EAGAIN;
			}
		}
	} else {
		yk3_err("yk3 esw set mc qset bitmap with unknown table type %d!",
			esw_mgr->lan_mac_table->type);
		return -EINVAL;
	}

	return 0;
}

static inline int yk3_esw_set_uc_qbmp(struct yk3_esw_manager *esw_mgr,
				      struct yk3_esw_qset_bitmap *l2uc_qbmp)
{
	u32 i;
	void __iomem *entry_addr;

	entry_addr = esw_mgr->hw_base +
		YK3_LAN_ESW_MAC_QSET_BMP_ENTRY_ADDR(l2uc_qbmp->index);
	if (esw_mgr->lan_mac_table->type == YK3_CUCKOO_TYPE_K3_LAN_MAC) {
		for (i = 0; i < YK3_LAN_ESW_MAC_QSET_ENTRY_LEN / 2 / 4; i++) {
			if (yk3_esw_iowrite32(entry_addr, i * sizeof(u32),
					      l2uc_qbmp->bitmap[i]) != 0) {
				yk3_err("yk3 esw set uc qset bitmap failed!");
				return -EAGAIN;
			}
		}
	} else {
		yk3_err("yk3 esw set uc qset bitmap with unknown table type %d!",
			esw_mgr->lan_mac_table->type);
		return -EINVAL;
	}

	return 0;
}

static inline int yk3_esw_set_vlan_qbmp(struct yk3_esw_manager *esw_mgr,
					u16 qbmp_idx, u32 *vlan_qbmp)
{
	u32 i;
	void __iomem *entry_addr;
	u32 empty_qbmp[YK3_ESW_QSET_BITMAP] = {0};

	if (!vlan_qbmp)
		vlan_qbmp = empty_qbmp;

	entry_addr = esw_mgr->hw_base +
		YK3_LAN_ESW_VLAN_QSET_BMP_ENTRY_ADDR(qbmp_idx);
	for (i = 0; i < YK3_LAN_ESW_VLAN_QSET_BMP_ENTRY_LEN / 2 / 4; i++) {
		if (yk3_esw_iowrite32(entry_addr, i * sizeof(u32),
				      vlan_qbmp[i]) != 0) {
			yk3_err("yk3 esw set vlan qset bitmap failed!");
			return -EAGAIN;
		}
	}

	return 0;
}

static int yk3_esw_alloc_l2uc_qset_bmp(struct yk3_esw_manager *esw_mgr,
				       int old_qbmp_index, u32 *bitmap)
{
	struct yk3_esw_qset_bitmap *l2uc_qbmp = NULL;
	struct yk3_esw_qset_bitmap *temp;
	struct yk3_esw_qset_bitmap *l2uc_qbmp_exist = NULL;
	bool new_qbmp_match = false;
	bool old_qbmp_match = false;

	list_for_each_entry_safe(l2uc_qbmp, temp, &esw_mgr->l2uc_qbmp_list,
				 qbmp_node) {
		if (yk3_esw_compare_qset_bmp(l2uc_qbmp->bitmap, bitmap,
					     YK3_ESW_QSET_BITMAP) == 0) {
			if (new_qbmp_match)
				yk3_err("yk3 esw panic add ref uc qbmp list has duplicate entry!");
			l2uc_qbmp->ref_cnt++;
			l2uc_qbmp_exist = l2uc_qbmp;
			new_qbmp_match = true;
			if (l2uc_qbmp_exist->index == old_qbmp_index) {
				l2uc_qbmp->ref_cnt--;
				yk3_err("yk3 esw alloc duplicate uc qbmp for index %d ref %d!",
					l2uc_qbmp_exist->index, l2uc_qbmp_exist->ref_cnt);
				return -EFAULT;
			}
		}

		if (old_qbmp_index != YK3_ESW_UNUSED_INDEX &&
		    l2uc_qbmp->index == old_qbmp_index) {
			if (old_qbmp_match)
				yk3_err("yk3 esw panic del ref uc qbmp list has duplicate entry!");
			/* dereference old qset bitmap */
			if (0 == --l2uc_qbmp->ref_cnt) {
				old_qbmp_index -= esw_mgr->uc_qbmp_base;
				if (yk3_esw_bmp_clear_bit(esw_mgr->uc_qbmp_idx_res,
							  esw_mgr->uc_qbmp_limit,
							  old_qbmp_index) != 0) {
					yk3_err("yk3 esw panic remove no exist qset bitmap!");
				}
				list_del(&l2uc_qbmp->qbmp_node);
				esw_mgr->uc_qbmp_used--;
				kfree(l2uc_qbmp);
			}
			old_qbmp_match = true;
		}
	}

	if (l2uc_qbmp_exist)
		return l2uc_qbmp_exist->index;

	/* alloc new l2uc qset bitmap */
	l2uc_qbmp = kzalloc(sizeof(*l2uc_qbmp), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(l2uc_qbmp)) {
		yk3_err("yk3 esw can not alloc unicast qset bitmap memory!");
		return -ENOMEM;
	}
	l2uc_qbmp->index = yk3_esw_bmp_find_first_zero_bit(esw_mgr->uc_qbmp_idx_res,
							   esw_mgr->uc_qbmp_limit,
							   true);
	if (l2uc_qbmp->index == esw_mgr->uc_qbmp_limit) {
		yk3_err("yk3 esw no more unicast qset bitmap resource!");
		kfree(l2uc_qbmp);
		return -EAGAIN;
	}
	yk3_esw_bmp_set_bit(esw_mgr->uc_qbmp_idx_res, esw_mgr->uc_qbmp_limit,
			    l2uc_qbmp->index, true);
	memcpy(l2uc_qbmp->bitmap, bitmap, sizeof(l2uc_qbmp->bitmap));
	l2uc_qbmp->ref_cnt = 1;
	l2uc_qbmp->index += esw_mgr->uc_qbmp_base;
	if (yk3_esw_set_uc_qbmp(esw_mgr, l2uc_qbmp) != 0) {
		yk3_err("yk3 esw set uc qset bitmap failed!");
		yk3_esw_bmp_set_bit(esw_mgr->uc_qbmp_idx_res,
				    esw_mgr->uc_qbmp_limit,
				    l2uc_qbmp->index - esw_mgr->uc_qbmp_base,
				    false);
		kfree(l2uc_qbmp);
		return -EAGAIN;
	}
	list_add(&l2uc_qbmp->qbmp_node, &esw_mgr->l2uc_qbmp_list);
	esw_mgr->uc_qbmp_used++;

	return l2uc_qbmp->index;
}

static int yk3_esw_free_l2uc_qset_bmp(struct yk3_esw_manager *esw_mgr,
				      struct yk3_esw_mac_filter *l2uc_key,
				      u16 qset)
{
	struct yk3_esw_qset_bitmap *l2uc_qbmp = NULL;
	struct yk3_esw_qset_bitmap *temp;
	struct yk3_esw_qset_bitmap *l2uc_qbmp_exist = NULL;
	bool empty_qbmp = false;
	int ret;

	ret = yk3_esw_bmp_set_bit_ref(l2uc_key->bitmap, l2uc_key->qset_ref,
				      YK3_ESW_QSET_BITMAP_BITS, qset, false);
	if (ret == 1) {
		return l2uc_key->qset_bmp_idx;
	} else if (ret == -1) {
		yk3_err("uc mac entry set qset %d bmp ref failed!", qset);
		return -1;
	}

	ret = yk3_esw_bmp_find_first_zero_bit(l2uc_key->bitmap,
					      YK3_ESW_QSET_BITMAP_BITS,
					      false);
	if (ret == YK3_ESW_QSET_BITMAP_BITS)
		empty_qbmp = true;

	list_for_each_entry_safe(l2uc_qbmp, temp, &esw_mgr->l2uc_qbmp_list,
				 qbmp_node) {
		if (l2uc_qbmp->index == l2uc_key->qset_bmp_idx) {
			/* dereference old qset bitmap */
			if (0 == --l2uc_qbmp->ref_cnt) {
				l2uc_qbmp->index -= esw_mgr->uc_qbmp_base;
				if (yk3_esw_bmp_clear_bit(esw_mgr->uc_qbmp_idx_res,
							  esw_mgr->uc_qbmp_limit,
							  l2uc_qbmp->index) != 0) {
					yk3_err("yk3 esw panic %s remove no exist qset bitmap!",
						__func__);
				}
				list_del(&l2uc_qbmp->qbmp_node);
				esw_mgr->uc_qbmp_used--;
				kfree(l2uc_qbmp);
			}
		} else if (!empty_qbmp &&
			   yk3_esw_compare_qset_bmp(l2uc_qbmp->bitmap,
						    l2uc_key->bitmap,
						    YK3_ESW_QSET_BITMAP) == 0) {
			l2uc_qbmp->ref_cnt++;
			l2uc_qbmp_exist = l2uc_qbmp;
		}
	}

	if (empty_qbmp)
		return -1;

	if (l2uc_qbmp_exist)
		return l2uc_qbmp_exist->index;

	l2uc_qbmp = kzalloc(sizeof(*l2uc_qbmp), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(l2uc_qbmp)) {
		yk3_err("yk3 esw can not alloc unicast qset bitmap memory!");
		return -ENOMEM;
	}
	l2uc_qbmp->index =
		yk3_esw_bmp_find_first_zero_bit(esw_mgr->uc_qbmp_idx_res,
						esw_mgr->uc_qbmp_limit,
						true);
	if (l2uc_qbmp->index == esw_mgr->uc_qbmp_limit) {
		yk3_err("yk3 esw no more unicast qset bitmap resource!");
		kfree(l2uc_qbmp);
		return -EAGAIN;
	}
	yk3_esw_bmp_set_bit(esw_mgr->uc_qbmp_idx_res,
			    esw_mgr->uc_qbmp_limit,
			    l2uc_qbmp->index, true);
	memcpy(l2uc_qbmp->bitmap, l2uc_key->bitmap, sizeof(l2uc_qbmp->bitmap));
	l2uc_qbmp->ref_cnt = 1;
	l2uc_qbmp->index += esw_mgr->uc_qbmp_base;
	if (yk3_esw_set_uc_qbmp(esw_mgr, l2uc_qbmp) != 0) {
		yk3_err("yk3 esw set uc qset bitmap failed!");
		yk3_esw_bmp_set_bit(esw_mgr->uc_qbmp_idx_res,
				    esw_mgr->uc_qbmp_limit,
				    l2uc_qbmp->index - esw_mgr->uc_qbmp_base,
				    false);
		kfree(l2uc_qbmp);
		return -EAGAIN;
	}
	list_add(&l2uc_qbmp->qbmp_node, &esw_mgr->l2uc_qbmp_list);
	esw_mgr->uc_qbmp_used++;

	return l2uc_qbmp->index;
}

int yk3_esw_add_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	struct yk3_esw_mac_filter *l2uc_key = NULL;
	struct yk3_esw_mac_filter_hw uc_value = {{0}};
	int uc_qbmp_index = YK3_ESW_UNUSED_INDEX;
	u8 mac_key[6];
	int found = 0;
	int ret;

	/* search l2uc key list */
	list_for_each_entry(l2uc_key, &esw_mgr->l2uc_key_list, key_node) {
		if (memcmp(l2uc_key->mac, mac, ETH_ALEN) == 0) {
			yk3_debug("%s new mac %pM qset %d",
				  __func__, mac, qset);
			yk3_debug("%s match exist uc entry qbmp index %d ref count %d",
				  __func__, l2uc_key->qset_bmp_idx, l2uc_key->ref_cnt);
			found = 1;
			break;
		}
	}

	/* reverse mac order porting hw */
	mac_key[5] = mac[0];
	mac_key[4] = mac[1];
	mac_key[3] = mac[2];
	mac_key[2] = mac[3];
	mac_key[1] = mac[4];
	mac_key[0] = mac[5];

	if (found) {
		ret = yk3_esw_bmp_set_bit_ref(l2uc_key->bitmap, l2uc_key->qset_ref,
					      YK3_ESW_QSET_BITMAP_BITS, qset, true);
		if (ret == 1) {
			l2uc_key->ref_cnt++;
			return 0;
		}
		uc_qbmp_index = yk3_esw_alloc_l2uc_qset_bmp(esw_mgr,
							    l2uc_key->qset_bmp_idx,
							    l2uc_key->bitmap);
		if (uc_qbmp_index < 0)
			return -EFAULT;

		l2uc_key->qset_bmp_idx = uc_qbmp_index;
		l2uc_key->ref_cnt++;
		memcpy(uc_value.mac, mac_key, ETH_ALEN);
		uc_value.qset_bmp_idx = l2uc_key->qset_bmp_idx;
		uc_value.enable = 1;
		ret = yk3_esw_cuckoo_change(esw_mgr->lan_mac_table, mac_key, (u8 *)&uc_value);
		if (ret != 0) {
			yk3_esw_free_l2uc_qset_bmp(esw_mgr, l2uc_key, qset);
			yk3_err("uc mac entry insert failed!");
		}
	} else {
		/* allow uc mac add over limit 1 entry because kernel virt interface
		 * change mac logic is add new mac first then del old mac
		 */
		if (esw_mgr->uc_key_used >= esw_mgr->uc_key_limit + 1) {
			yk3_err("yk3 esw unicast mac address reach limit %d!",
				esw_mgr->uc_key_limit);
			return -EAGAIN;
		}
		l2uc_key = kzalloc(sizeof(*l2uc_key), GFP_ATOMIC);
		if (IS_ERR_OR_NULL(l2uc_key))
			return -ENOMEM;

		memcpy(l2uc_key->mac, mac, ETH_ALEN);
		l2uc_key->qset_bmp_idx = YK3_ESW_UNUSED_INDEX;
		ret = yk3_esw_bmp_set_bit_ref(l2uc_key->bitmap, l2uc_key->qset_ref,
					      YK3_ESW_QSET_BITMAP_BITS, qset, true);
		if (ret != 0) {
			kfree(l2uc_key);
			yk3_err("uc mac entry alloc qset bitmap for qset %d failed!", qset);
			return -EFAULT;
		}
		uc_qbmp_index = yk3_esw_alloc_l2uc_qset_bmp(esw_mgr,
							    l2uc_key->qset_bmp_idx,
							    l2uc_key->bitmap);
		if (uc_qbmp_index < 0) {
			kfree(l2uc_key);
			return -EFAULT;
		}
		l2uc_key->qset_bmp_idx = uc_qbmp_index;
		l2uc_key->ref_cnt = 1;
		memcpy(uc_value.mac, mac_key, ETH_ALEN);
		uc_value.qset_bmp_idx = uc_qbmp_index;
		uc_value.enable = 1;
		ret = yk3_esw_cuckoo_insert(esw_mgr->lan_mac_table, mac_key, (u8 *)&uc_value);
		if (ret != 0) {
			yk3_err("uc mac entry insert hash table failed!");
			yk3_esw_free_l2uc_qset_bmp(esw_mgr, l2uc_key, qset);
			kfree(l2uc_key);
		} else {
			list_add(&l2uc_key->key_node, &esw_mgr->l2uc_key_list);
			esw_mgr->uc_key_used++;
			yk3_debug("uc mac entry insert %d success!", esw_mgr->uc_key_used);
		}
	}

	return 0;
}

int yk3_esw_del_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	struct yk3_esw_mac_filter *l2uc_key = NULL;
	struct yk3_esw_mac_filter_hw uc_value = {{0}};
	int uc_qbmp_index = YK3_ESW_UNUSED_INDEX;
	u8 mac_key[6];
	int found = 0;
	int ret = 0;

	/* search l2uc key list */
	list_for_each_entry(l2uc_key, &esw_mgr->l2uc_key_list,
			    key_node) {
		if (memcmp(l2uc_key->mac, mac, ETH_ALEN) == 0) {
			yk3_debug("%s old mac %pM qset %d",
				  __func__, mac, qset);
			yk3_debug("%s match exist uc entry qbmp index %d ref count %d",
				  __func__, l2uc_key->qset_bmp_idx, l2uc_key->ref_cnt);
			found = 1;
			break;
		}
	}

	if (!found) {
		yk3_info("%s old mac %pM qset %d", __func__, mac, qset);
		yk3_info("%s panic delete entry no exist!", __func__);
		return -1;
	}

	mac_key[5] = mac[0];
	mac_key[4] = mac[1];
	mac_key[3] = mac[2];
	mac_key[2] = mac[3];
	mac_key[1] = mac[4];
	mac_key[0] = mac[5];

	uc_qbmp_index = yk3_esw_free_l2uc_qset_bmp(esw_mgr, l2uc_key, qset);
	if (--l2uc_key->ref_cnt == 0) {
		ret = yk3_esw_cuckoo_delete(esw_mgr->lan_mac_table, mac_key);
		if (ret != 0) {
			yk3_err("%s old mac %pM qset %d", __func__, mac, qset);
			yk3_err("%s yk3 esw hash table delete failed return %d!",
				__func__, ret);
		}
		list_del(&l2uc_key->key_node);
		kfree(l2uc_key);
		esw_mgr->uc_key_used--;
	} else {
		if (uc_qbmp_index < 0) {
			yk3_err("%s old mac %pM qset %d", __func__, mac, qset);
			yk3_err("%s panic update old mac return %d qset bitmap index!",
				__func__, uc_qbmp_index);
			return -1;
		}
		l2uc_key->qset_bmp_idx = uc_qbmp_index;
		memcpy(uc_value.mac, mac_key, ETH_ALEN);
		uc_value.qset_bmp_idx = l2uc_key->qset_bmp_idx;
		uc_value.enable = 1;
		ret = yk3_esw_cuckoo_change(esw_mgr->lan_mac_table, mac_key, (u8 *)&uc_value);
		if (ret != 0) {
			yk3_err("%s old mac %pM qset %d",
				__func__, mac, qset);
			yk3_err("%s panic hash table change old mac failed return %d!",
				__func__, ret);
		}
	}

	return ret;
}

int yk3_esw_upd_uc_mac(struct yk3_pdev_priv *pdev_priv, u8 *old_mac, u8 *new_mac, u16 qset)
{
	u8 mac_empty[6] = {0};
	int ret;

	/* delete old mac */
	if (old_mac && memcmp(mac_empty, old_mac, ETH_ALEN) != 0) {
		ret = yk3_esw_del_uc_mac(pdev_priv, old_mac, qset);
		if (ret != 0) {
			yk3_err("%s old mac %pM qset %d", __func__, old_mac, qset);
			yk3_err("%s del old mac entry failed!", __func__);
			return ret;
		}
	}
	/* add new mac */
	ret = yk3_esw_add_uc_mac(pdev_priv, new_mac, qset);
	if (ret != 0) {
		yk3_err("%s new mac %pM qset %d", __func__, new_mac, qset);
		yk3_err("%s add new mac entry failed!", __func__);
		return ret;
	}

	return 0;
}

static int yk3_esw_alloc_l2mc_qset_bmp(struct yk3_esw_manager *esw_mgr,
				       int old_qbmp_index, u32 *bitmap)
{
	struct yk3_esw_qset_bitmap *l2mc_qbmp;
	struct yk3_esw_qset_bitmap *temp;
	struct yk3_esw_qset_bitmap *l2mc_qbmp_exist = NULL;
	bool new_qbmp_match = false;
	bool old_qbmp_match = false;

	list_for_each_entry_safe(l2mc_qbmp, temp, &esw_mgr->l2mc_qbmp_list,
				 qbmp_node) {
		if (yk3_esw_compare_qset_bmp(l2mc_qbmp->bitmap, bitmap,
					     YK3_ESW_QSET_BITMAP) == 0) {
			if (new_qbmp_match) {
				yk3_err("yk3 esw found mc qbmp %p has duplicate entry!",
					bitmap);
				yk3_err("prev match index %d ref %d, current index %d ref %d",
					l2mc_qbmp_exist->index, l2mc_qbmp_exist->ref_cnt,
					l2mc_qbmp->index, l2mc_qbmp->ref_cnt);
			}
			l2mc_qbmp->ref_cnt++;
			l2mc_qbmp_exist = l2mc_qbmp;
			new_qbmp_match = true;
			yk3_debug("%s yk3 esw add ref %d for mc qbmp %p index %d",
				  __func__, l2mc_qbmp->ref_cnt, l2mc_qbmp, l2mc_qbmp->index);
			if (l2mc_qbmp_exist->index == old_qbmp_index) {
				l2mc_qbmp->ref_cnt--;
				yk3_err("yk3 esw alloc duplicate mc qbmp for index %d ref %d!",
					l2mc_qbmp_exist->index, l2mc_qbmp_exist->ref_cnt);
				return -EFAULT;
			}
		}

		if (old_qbmp_index != YK3_ESW_UNUSED_INDEX &&
		    l2mc_qbmp->index == old_qbmp_index) {
			if (old_qbmp_match) {
				yk3_err("yk3 esw del ref mc qbmp %p has duplicate entry!",
					bitmap);
				yk3_err("dupliate entry %p index %d ref %d",
					l2mc_qbmp, l2mc_qbmp->index, l2mc_qbmp->ref_cnt);
			}
			/* dereference old qset bitmap */
			if (0 == --l2mc_qbmp->ref_cnt) {
				l2mc_qbmp->index -= esw_mgr->mc_qbmp_base;
				if (yk3_esw_bmp_clear_bit(esw_mgr->mc_qbmp_idx_res,
							  esw_mgr->mc_qbmp_limit,
							  l2mc_qbmp->index) != 0) {
					yk3_err("yk3 esw mc remove no exist qbmp!");
					yk3_err("mc qbmp %p index %d", l2mc_qbmp,
						l2mc_qbmp->index + esw_mgr->mc_qbmp_base);
				}
				list_del(&l2mc_qbmp->qbmp_node);
				esw_mgr->mc_qbmp_used--;
				yk3_debug("yk3 esw %s remove mc qbmp %p", __func__, l2mc_qbmp);
				kfree(l2mc_qbmp);
			} else {
				yk3_debug("%s yk3 esw dec mc qbmp %p index %d ref %d",
					  __func__, l2mc_qbmp, l2mc_qbmp->index,
					  l2mc_qbmp->ref_cnt);
			}
			old_qbmp_match = true;
		}
	}

	if (l2mc_qbmp_exist)
		return l2mc_qbmp_exist->index;

	/* alloc new l2mc qset bitmap */
	l2mc_qbmp = kzalloc(sizeof(*l2mc_qbmp), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(l2mc_qbmp)) {
		yk3_err("yk3 esw can not alloc multicast qset bitmap memory!");
		return -ENOMEM;
	}
	l2mc_qbmp->index = yk3_esw_bmp_find_first_zero_bit(esw_mgr->mc_qbmp_idx_res,
							   esw_mgr->mc_qbmp_limit,
							   true);
	if (l2mc_qbmp->index == esw_mgr->mc_qbmp_limit) {
		yk3_err("yk3 esw no more multicast qset bitmap resource!");
		yk3_err("yk3 esw alloc mc qbmp for %p old index %d!",
			bitmap, old_qbmp_index);
		kfree(l2mc_qbmp);
		return -EAGAIN;
	}
	yk3_esw_bmp_set_bit(esw_mgr->mc_qbmp_idx_res, esw_mgr->mc_qbmp_limit,
			    l2mc_qbmp->index, true);
	//mc mac use last YK3_LAN_ESW_MC_MAC_QSET_BMP_NUMB MAC FILTER BITMAP
	memcpy(l2mc_qbmp->bitmap, bitmap, sizeof(l2mc_qbmp->bitmap));
	l2mc_qbmp->ref_cnt = 1;
	l2mc_qbmp->index += esw_mgr->mc_qbmp_base;
	if (yk3_esw_set_mc_qbmp(esw_mgr, l2mc_qbmp) != 0) {
		yk3_err("yk3 esw set mc qset bitmap failed!");
		yk3_esw_bmp_set_bit(esw_mgr->mc_qbmp_idx_res,
				    esw_mgr->mc_qbmp_limit,
				    l2mc_qbmp->index - esw_mgr->mc_qbmp_base,
				    false);
		kfree(l2mc_qbmp);
		return -EAGAIN;
	}
	list_add(&l2mc_qbmp->qbmp_node, &esw_mgr->l2mc_qbmp_list);
	esw_mgr->mc_qbmp_used++;
	yk3_debug("yk3 esw new mc qbmp %p index %d", l2mc_qbmp, l2mc_qbmp->index);

	return l2mc_qbmp->index;
}

static int yk3_esw_free_l2mc_qset_bmp(struct yk3_esw_manager *esw_mgr,
				      struct yk3_esw_mac_filter *l2mc_key,
				      u16 qset)
{
	struct yk3_esw_qset_bitmap *l2mc_qbmp;
	struct yk3_esw_qset_bitmap *temp;
	struct yk3_esw_qset_bitmap *l2mc_qbmp_exist = NULL;
	bool empty_qbmp = false;
	u8 *mac;
	u32 ret;

	ret = yk3_esw_bmp_set_bit_ref_once(l2mc_key->bitmap, l2mc_key->qset_ref,
					   YK3_ESW_QSET_BITMAP_BITS, qset, false);
	if (ret == -1) {
		yk3_err("mc mac entry set qset %d bmp unref failed!", qset);
		return -1;
	}

	ret = yk3_esw_bmp_find_first_zero_bit(l2mc_key->bitmap,
					      YK3_ESW_QSET_BITMAP_BITS,
					      false);
	if (ret == YK3_ESW_QSET_BITMAP_BITS)
		empty_qbmp = true;

	mac = l2mc_key->mac;
	list_for_each_entry_safe(l2mc_qbmp, temp, &esw_mgr->l2mc_qbmp_list,
				 qbmp_node) {
		if (l2mc_qbmp->index == l2mc_key->qset_bmp_idx) {
			yk3_debug("%s mc mac %pM qset %d", __func__, mac, qset);
			/* dereference old qset bitmap */
			if (0 == --l2mc_qbmp->ref_cnt) {
				l2mc_qbmp->index -= esw_mgr->mc_qbmp_base;
				if (yk3_esw_bmp_clear_bit(esw_mgr->mc_qbmp_idx_res,
							  esw_mgr->mc_qbmp_limit,
							  l2mc_qbmp->index) != 0) {
					yk3_err("%s yk3 esw del no exist qbmp index %d!",
						__func__, l2mc_qbmp->index);
				}
				list_del(&l2mc_qbmp->qbmp_node);
				esw_mgr->mc_qbmp_used--;
				yk3_debug("%s yk3 esw remove mc qbmp %p index %d ok",
					  __func__, l2mc_qbmp,
					  l2mc_qbmp->index + esw_mgr->mc_qbmp_base);
				kfree(l2mc_qbmp);
			} else {
				yk3_debug("%s yk3 esw dec ref %d for mc qbmp %p index %d",
					  __func__, l2mc_qbmp->ref_cnt, l2mc_qbmp,
					  l2mc_qbmp->index);
			}
		} else if (!empty_qbmp &&
			   yk3_esw_compare_qset_bmp(l2mc_qbmp->bitmap,
						    l2mc_key->bitmap,
						    YK3_ESW_QSET_BITMAP) == 0) {
			yk3_debug("%s mc mac %pM qset %d", __func__, mac, qset);
			l2mc_qbmp->ref_cnt++;
			if (l2mc_qbmp_exist) {
				yk3_err("%s yk3 esw match new mc qbmp duplicate!",
					__func__);
				yk3_err("%s prev %p index %d, current %p index %d!",
					__func__, l2mc_qbmp_exist, l2mc_qbmp_exist->index,
					l2mc_qbmp, l2mc_qbmp->index);
			}
			l2mc_qbmp_exist = l2mc_qbmp;
		}
	}

	yk3_debug("%s mc mac %pM qset %d", __func__, mac, qset);

	if (empty_qbmp) {
		yk3_debug("%s yk3 esw free mc qbmp last qset!", __func__);
		return YK3_ESW_UNUSED_INDEX;
	}

	if (l2mc_qbmp_exist) {
		yk3_debug("%s yk3 esw get new mc qbmp index %d!",
			  __func__, l2mc_qbmp_exist->index);
		return l2mc_qbmp_exist->index;
	}

	l2mc_qbmp = kzalloc(sizeof(*l2mc_qbmp), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(l2mc_qbmp)) {
		yk3_err("yk3 esw can not alloc mc qbmp memory!");
		return -ENOMEM;
	}
	l2mc_qbmp->index = yk3_esw_bmp_find_first_zero_bit(esw_mgr->mc_qbmp_idx_res,
							   esw_mgr->mc_qbmp_limit,
							   true);
	if (l2mc_qbmp->index == esw_mgr->mc_qbmp_limit) {
		yk3_err("%s yk3 esw no more mc qbmp resource!", __func__);
		kfree(l2mc_qbmp);
		return -EAGAIN;
	}
	yk3_esw_bmp_set_bit(esw_mgr->mc_qbmp_idx_res,
			    esw_mgr->mc_qbmp_limit,
			    l2mc_qbmp->index, true);
	//mc mac use last YK3_LAN_ESW_MC_MAC_QSET_BMP_NUMB MAC FILTER BITMAP
	memcpy(l2mc_qbmp->bitmap, l2mc_key->bitmap, sizeof(l2mc_qbmp->bitmap));
	l2mc_qbmp->ref_cnt = 1;
	l2mc_qbmp->index += esw_mgr->mc_qbmp_base;
	if (yk3_esw_set_mc_qbmp(esw_mgr, l2mc_qbmp) != 0) {
		yk3_err("yk3 esw set mc qset bitmap failed!");
		yk3_esw_bmp_set_bit(esw_mgr->mc_qbmp_idx_res,
				    esw_mgr->mc_qbmp_limit,
				    l2mc_qbmp->index - esw_mgr->mc_qbmp_base,
				    false);
		kfree(l2mc_qbmp);
		return -EAGAIN;
	}
	list_add(&l2mc_qbmp->qbmp_node, &esw_mgr->l2mc_qbmp_list);
	esw_mgr->mc_qbmp_used++;
	yk3_debug("%s yk3 esw use mc qbmp %p index %d for %p", __func__, l2mc_qbmp,
		  l2mc_qbmp->index, l2mc_key);

	return l2mc_qbmp->index;
}

int yk3_esw_add_mc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	struct yk3_esw_mac_filter *l2mc_key = NULL, *entry = NULL;
	struct yk3_esw_mac_filter_hw mc_value = {{0}};
#ifdef ALL_MC_FEATURE
	struct yk3_esw_qset_bitmap all_mc_bitmap = {{0}};
#endif
	u8 mac_key[6];
	int mc_qbmp_index = YK3_ESW_UNUSED_INDEX;
	int ret = 0;

	/* search ipv4 l2mc key list */
	list_for_each_entry(entry, &esw_mgr->l2mc_key_list,
			    key_node) {
		if (memcmp(entry->mac, mac, ETH_ALEN) == 0) {
			yk3_debug("%s new mc mac %pM qset %d", __func__, mac, qset);
			yk3_debug("%s match exist entry qset bitmap index %d", __func__,
				  entry->qset_bmp_idx);
			l2mc_key = entry;
			break;
		}
	}

	/* init mac key & entry value */
	mac_key[0] = mac[5];
	mac_key[1] = mac[4];
	mac_key[2] = mac[3];
	mac_key[3] = mac[2];
	mac_key[4] = mac[1];
	mac_key[5] = mac[0];
	mc_value.enable = 1;
	memcpy(mc_value.mac, mac_key, sizeof(mac_key));

	if (l2mc_key) {
		/* update l2mc entry qset bitmap index */
		ret = yk3_esw_bmp_set_bit_ref_once(l2mc_key->bitmap, l2mc_key->qset_ref,
						   YK3_ESW_QSET_BITMAP_BITS, qset, true);
		if (ret == 1) {
			yk3_debug("mc mac entry qset %d set more than 1!", qset);
			return 0;
		}
		mc_qbmp_index = yk3_esw_alloc_l2mc_qset_bmp(esw_mgr,
							    l2mc_key->qset_bmp_idx,
							    l2mc_key->bitmap);
		if (mc_qbmp_index < 0)
			return -EFAULT;
#ifdef ALL_MC_FEATURE
		if (l2mc_key->qset_bmp_idx != mc_qbmp_index) {
			memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap,
			       sizeof(l2mc_key->bitmap) / 2);
			yk3_esw_bmp_merge(all_mc_bitmap.bitmap, esw_mgr->all_mc_qset,
					  YK3_LAN_RX_QSET_NUM);
			all_mc_bitmap.index = mc_qbmp_index;
			ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
			if (ret != 0) {
				yk3_err("yk3 esw set all mc qset bitmap failed!");
				memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap,
				       sizeof(l2mc_key->bitmap) / 2);
				ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
				if (ret != 0)
					yk3_err("yk3 esw restore mc original qset bitmap failed!");
			}
		}
#endif
		mc_value.qset_bmp_idx = mc_qbmp_index;
		ret = yk3_esw_cuckoo_change(esw_mgr->lan_mac_table, mac_key, (u8 *)&mc_value);
		if (ret != 0) {
			yk3_err("%s mc mac %pM qset %d", __func__, mac, qset);
			yk3_err("mc mac entry insert failed!");
			yk3_esw_free_l2mc_qset_bmp(esw_mgr, l2mc_key, qset);
		} else {
			l2mc_key->qset_bmp_idx = mc_qbmp_index;
			yk3_debug("%s mc mac %pM qset %d", __func__, mac, qset);
			yk3_debug("mc mac entry %p upd qbmp index %d into hash table ok!",
				  l2mc_key, l2mc_key->qset_bmp_idx);
		}
	} else {
		if (esw_mgr->mc_key_used >= esw_mgr->mc_key_limit) {
			yk3_err("yk3 esw multicast mac address reach limit %d!",
				esw_mgr->mc_key_limit);
			return -EAGAIN;
		}
		/* add new l2mc entry */
		l2mc_key = kzalloc(sizeof(*l2mc_key), GFP_ATOMIC);
		if (IS_ERR_OR_NULL(l2mc_key))
			return -ENOMEM;
		memcpy(l2mc_key->mac, mac, sizeof(mc_value.mac));
		ret = yk3_esw_bmp_set_bit_ref_once(l2mc_key->bitmap, l2mc_key->qset_ref,
						   YK3_ESW_QSET_BITMAP_BITS, qset, true);
		if (ret != 0) {
			kfree(l2mc_key);
			yk3_err("mc mac entry alloc qset bitmap for qset %d failed!", qset);
			return -EFAULT;
		}
		mc_qbmp_index = yk3_esw_alloc_l2mc_qset_bmp(esw_mgr, YK3_ESW_UNUSED_INDEX,
							    l2mc_key->bitmap);
		if (mc_qbmp_index < 0) {
			kfree(l2mc_key);
			yk3_err("%s mc mac %pM qset %d", __func__, mac, qset);
			yk3_err("%s get mc_qbmp_index error %d", __func__, mc_qbmp_index);
			return -EFAULT;
		}
#ifdef ALL_MC_FEATURE
		memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap, sizeof(l2mc_key->bitmap) / 2);
		yk3_esw_bmp_merge(all_mc_bitmap.bitmap, esw_mgr->all_mc_qset,
				  YK3_LAN_RX_QSET_NUM);
		all_mc_bitmap.index = mc_qbmp_index;
		ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
		if (ret != 0) {
			yk3_err("yk3 esw set all mc qset bitmap failed!");
			memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap,
			       sizeof(l2mc_key->bitmap) / 2);
			ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
			if (ret != 0) {
				yk3_err("yk3 esw restore mc original qset bitmap failed!");
				yk3_esw_free_l2mc_qset_bmp(esw_mgr, l2mc_key, qset);
				kfree(l2mc_key);
				return -EFAULT;
			}
		}
#endif
		mc_value.qset_bmp_idx = mc_qbmp_index;
		ret = yk3_esw_cuckoo_insert(esw_mgr->lan_mac_table, mac_key, (u8 *)&mc_value);
		if (ret != 0) {
			yk3_esw_free_l2mc_qset_bmp(esw_mgr, l2mc_key, qset);
			kfree(l2mc_key);
			yk3_err("%s mc mac %pM qset %d", __func__, mac, qset);
			yk3_err("mc mac entry insert hash table failed!");
		} else {
			l2mc_key->qset_bmp_idx = mc_qbmp_index;
			esw_mgr->mc_key_used++;
			list_add(&l2mc_key->key_node, &esw_mgr->l2mc_key_list);
			yk3_debug("%s mc mac %pM qset %d", __func__, mac, qset);
			yk3_debug("mc mac entry %p qbmp index %d insert hash table ok!",
				  l2mc_key, l2mc_key->qset_bmp_idx);
		}
	}

	return 0;
}

int yk3_esw_del_mc_mac(struct yk3_pdev_priv *pdev_priv, u8 *mac, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	struct yk3_esw_mac_filter *l2mc_key = NULL, *entry = NULL;
	struct yk3_esw_mac_filter_hw mc_value = {{0}};
	u8 mac_key[6];
#ifdef ALL_MC_FEATURE
	struct yk3_esw_qset_bitmap all_mc_bitmap = {{0}};
#endif
	int mc_qbmp_index = YK3_ESW_UNUSED_INDEX;
	int ret = 0;

	/* search ipv4 l2mc key list */
	list_for_each_entry(entry, &esw_mgr->l2mc_key_list, key_node) {
		if (memcmp(entry->mac, mac, ETH_ALEN) == 0) {
			l2mc_key = entry;
			yk3_debug("%s del mc mac %pM qset %d", __func__, mac, qset);
			yk3_debug("%s match exist mc entry %p qbmp index %d",
				  __func__, l2mc_key, l2mc_key->qset_bmp_idx);
			break;
		}
	}

	/* init mac key & entry value */
	mac_key[0] = mac[5];
	mac_key[1] = mac[4];
	mac_key[2] = mac[3];
	mac_key[3] = mac[2];
	mac_key[4] = mac[1];
	mac_key[5] = mac[0];
	mc_value.enable = 1;
	memcpy(mc_value.mac, mac_key, sizeof(mac_key));

	yk3_debug("%s del mc mac %pM qset %d", __func__, mac, qset);

	if (IS_ERR_OR_NULL(l2mc_key)) {
		yk3_err("%s no exist mc entry!", __func__);
		return -EFAULT;
	}

	/* l2mc entry qset bitmap remove this qset */
	mc_qbmp_index = yk3_esw_free_l2mc_qset_bmp(esw_mgr, l2mc_key, qset);
	if (mc_qbmp_index < 0 && mc_qbmp_index != YK3_ESW_UNUSED_INDEX) {
		yk3_err("%s free mc entry %p qbmp index %d failed!",
			__func__, l2mc_key, l2mc_key->qset_bmp_idx);
		return -EFAULT;
	}

	if (mc_qbmp_index == YK3_ESW_UNUSED_INDEX) {
		/* l2mc entry need remove */
		ret = yk3_esw_cuckoo_delete(esw_mgr->lan_mac_table, mac_key);
		if (ret != 0) {
			yk3_err("%s mc entry %p delete from hash table ret %d failed!",
				__func__, l2mc_key, ret);
		} else {
			yk3_debug("%s mc entry %p delete form hash table ok!",
				  __func__, l2mc_key);
		}
		list_del(&l2mc_key->key_node);
		kfree(l2mc_key);
		esw_mgr->mc_key_used--;
	} else {
#ifdef ALL_MC_FEATURE
		memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap, sizeof(l2mc_key->bitmap) / 2);
		yk3_esw_bmp_merge(all_mc_bitmap.bitmap, esw_mgr->all_mc_qset,
				  YK3_LAN_RX_QSET_NUM);
		all_mc_bitmap.index = mc_qbmp_index;
		ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
		if (ret != 0) {
			yk3_err("yk3 esw set all mc qset bitmap failed!");
			memcpy(all_mc_bitmap.bitmap, l2mc_key->bitmap,
			       sizeof(l2mc_key->bitmap) / 2);
			ret = yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap);
			if (ret != 0)
				yk3_err("yk3 esw restore mc original qset bitmap failed!");
		}
#endif
		mc_value.qset_bmp_idx = mc_qbmp_index;
		ret = yk3_esw_cuckoo_change(esw_mgr->lan_mac_table, mac_key,
					    (u8 *)&mc_value);
		if (ret != 0) {
			yk3_err("%s mc entry %p update qbmp index %d ret %d failed!",
				__func__, l2mc_key, mc_qbmp_index, ret);
		} else {
			l2mc_key->qset_bmp_idx = mc_qbmp_index;
			yk3_debug("%s mc entry %p update qbmp index %d ok!",
				  __func__, l2mc_key, mc_qbmp_index);
		}
	}

	return 0;
}

static int yk3_esw_update_vlan_qset_bmp(struct yk3_esw_manager *esw_mgr,
					int old_qbmp_index, u32 *bitmap)
{
	struct yk3_esw_vlan_qbmp *vlan_qbmp;
	struct yk3_esw_vlan_qbmp *temp;
	struct yk3_esw_vlan_qbmp *vlan_qbmp_exist = NULL;
	bool new_qbmp_match = false;
	bool old_qbmp_match = false;

	list_for_each_entry_safe(vlan_qbmp, temp, &esw_mgr->vlan_qset_bmp_list,
				 vlan_qbmp_node) {
		if (yk3_esw_compare_qset_bmp(vlan_qbmp->bitmap, bitmap,
					     YK3_ESW_QSET_BITMAP) == 0) {
			if (new_qbmp_match) {
				yk3_err("%s yk3 esw add ref vlan qbmp has duplicate entry!",
					__func__);
				yk3_err("prev match index %d ref %d, current index %d ref %d",
					vlan_qbmp_exist->index, vlan_qbmp_exist->ref_cnt,
					vlan_qbmp->index, vlan_qbmp->ref_cnt);
			}
			vlan_qbmp->ref_cnt++;
			vlan_qbmp_exist = vlan_qbmp;
			new_qbmp_match = true;
			yk3_debug("%s yk3 esw vlan qset bmp %p add ref cnt %d!",
				  __func__, vlan_qbmp, vlan_qbmp->ref_cnt);
		}

		if (old_qbmp_index != YK3_ESW_UNUSED_INDEX &&
		    vlan_qbmp->index == old_qbmp_index) {
			if (old_qbmp_match) {
				yk3_err("yk3 esw del ref vlan qbmp %p has duplicate entry!",
					bitmap);
				yk3_err("dupliate entry %p index %d ref %d",
					vlan_qbmp, vlan_qbmp->index, vlan_qbmp->ref_cnt);
			}
			/* dereference old qset bitmap */
			if (0 == --vlan_qbmp->ref_cnt) {
				// non-bnic mode kernel vlan use first half vlan qbmp resource
				if (esw_mgr->nic_mode != 0)
					old_qbmp_index -= esw_mgr->vlan_qbmp_num;
				if (yk3_esw_bmp_clear_bit(esw_mgr->vlan_qbmp_index,
							  YK3_LAN_ESW_VLAN_FILTER_ENTRY_NUMB,
							  old_qbmp_index) != 0) {
					yk3_err("yk3 esw remove no exist vlan qbmp!");
					yk3_err("vlan qbmp %p index %d", vlan_qbmp,
						vlan_qbmp->index);
				}
				list_del(&vlan_qbmp->vlan_qbmp_node);
				esw_mgr->vlan_qbmp_used--;
				yk3_debug("%s vlan qset bmp %p qbmp index %d ref cnt 0!",
					  __func__, vlan_qbmp, old_qbmp_index);
				kfree(vlan_qbmp);
			} else {
				yk3_debug("%s vlan qset bmp %p dec ref cnt %d!",
					  __func__, vlan_qbmp, vlan_qbmp->ref_cnt);
			}
			old_qbmp_match = true;
		}
	}

	if (vlan_qbmp_exist)
		return vlan_qbmp_exist->index;

	if (yk3_esw_bmp_find_first_zero_bit(bitmap,
					    YK3_ESW_QSET_BITMAP_BITS,
					    false) == YK3_ESW_QSET_BITMAP_BITS) {
		yk3_debug("yk3 esw empty vlan qset bitmap return -1!");
		return -1;
	}

	/* alloc new vlan qset bitmap */
	vlan_qbmp = kzalloc(sizeof(*vlan_qbmp), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(vlan_qbmp)) {
		yk3_err("yk3 esw can not alloc vlan qset bitmap memory!");
		return -ENOMEM;
	}
	vlan_qbmp->index = yk3_esw_bmp_find_first_zero_bit(esw_mgr->vlan_qbmp_index,
							   esw_mgr->vlan_qbmp_num,
							   true);
	if (vlan_qbmp->index == esw_mgr->vlan_qbmp_num) {
		yk3_err("yk3 esw no more vlan qset bitmap resource!");
		kfree(vlan_qbmp);
		return -EAGAIN;
	}

	yk3_esw_bmp_set_bit(esw_mgr->vlan_qbmp_index,
			    YK3_LAN_ESW_VLAN_QSET_BMP_NUMS,
			    vlan_qbmp->index, true);
	// non-legacy mode kernel vlan use first half vlan qbmp resource
	if (esw_mgr->nic_mode != 0)
		vlan_qbmp->index += esw_mgr->vlan_qbmp_num;
	memcpy(vlan_qbmp->bitmap, bitmap, sizeof(vlan_qbmp->bitmap));
	vlan_qbmp->ref_cnt = 1;
	if (yk3_esw_set_vlan_qbmp(esw_mgr, vlan_qbmp->index, vlan_qbmp->bitmap) != 0) {
		yk3_err("yk3 esw set vlan qset bitmap failed!");
		if (esw_mgr->nic_mode != 0)
			vlan_qbmp->index -= esw_mgr->vlan_qbmp_num;
		yk3_esw_bmp_set_bit(esw_mgr->vlan_qbmp_index,
				    YK3_LAN_ESW_VLAN_QSET_BMP_NUMS,
				    vlan_qbmp->index, false);
		kfree(vlan_qbmp);
		return -EAGAIN;
	}
	list_add(&vlan_qbmp->vlan_qbmp_node, &esw_mgr->vlan_qset_bmp_list);
	esw_mgr->vlan_qbmp_used++;
	yk3_debug("new vlan qset bmp %p ref cnt 1!", vlan_qbmp);

	return vlan_qbmp->index;
}

int yk3_esw_add_vlan(struct yk3_pdev_priv *pdev_priv, u16 vid, __be16 tpid, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	u32 reg;
	struct yk3_esw_vlan_entry *vlan_entry;
	u32 vlan_entry_idx;
	u32 offset;

	yk3_debug("%s yk3 esw qset %d add vlan id %d tpid %04x!",
		  __func__, qset, vid, ntohs(tpid));

	vlan_entry_idx = vid;
	if (tpid == htons(ETH_P_8021AD)) {
		vlan_entry_idx += 4096;
		offset = YK3_LAN_ESW_VLAN_FILTER_TYPE1_BASE +
			 YK3_LAN_ESW_VLAN_FILTER_VLAN_ENTRY_OFFSET(vid);
	} else {
		offset = YK3_LAN_ESW_VLAN_FILTER_TYPE0_BASE +
			 YK3_LAN_ESW_VLAN_FILTER_VLAN_ENTRY_OFFSET(vid);
	}

	vlan_entry = &esw_mgr->vlan_entrys[vlan_entry_idx];

	yk3_esw_bmp_set_bit(vlan_entry->bitmap, YK3_ESW_QSET_BITMAP_BITS,
			    qset, true);
	vlan_entry->qbmp_idx = yk3_esw_update_vlan_qset_bmp(esw_mgr,
							    vlan_entry->qbmp_idx,
							    vlan_entry->bitmap);
	if (vlan_entry->qbmp_idx < 0) {
		yk3_err("%s yk3 esw alloc vlan qbmp ret %d failed!", __func__,
			vlan_entry->qbmp_idx);
		return -1;
	}
	vlan_entry->member_cnt++;

	reg = FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_ENABLE, 1) |
	      FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_QSET_BMP_INDEX,
			 vlan_entry->qbmp_idx & 0xfff);
	yk3_wr32(esw_mgr->hw_base, offset, reg);

	yk3_debug("%s update vlan_entry id %d member num %d qbmp index %d ok!",
		  __func__, vlan_entry_idx, vlan_entry->member_cnt, vlan_entry->qbmp_idx);

	return 0;
}

int yk3_esw_del_vlan(struct yk3_pdev_priv *pdev_priv, u16 vid, __be16 tpid, u16 qset)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	u32 reg;
	struct yk3_esw_vlan_entry *vlan_entry;
	u32 vlan_entry_idx;
	u32 offset;

	yk3_debug("%s yk3 esw qset %d del vlan id %d tpid %04x!",
		  __func__, qset, vid, ntohs(tpid));

	vlan_entry_idx = vid;
	if (tpid == htons(ETH_P_8021AD)) {
		vlan_entry_idx += 4096;
		offset = YK3_LAN_ESW_VLAN_FILTER_TYPE1_BASE +
			 YK3_LAN_ESW_VLAN_FILTER_VLAN_ENTRY_OFFSET(vid);
	} else {
		offset = YK3_LAN_ESW_VLAN_FILTER_TYPE0_BASE +
			 YK3_LAN_ESW_VLAN_FILTER_VLAN_ENTRY_OFFSET(vid);
	}

	vlan_entry = &esw_mgr->vlan_entrys[vlan_entry_idx];
	yk3_esw_bmp_set_bit(vlan_entry->bitmap, YK3_ESW_QSET_BITMAP_BITS,
			    qset, false);

	if (0 == --vlan_entry->member_cnt) {
		if (YK3_ESW_QSET_BITMAP_BITS !=
		    yk3_esw_bmp_find_first_zero_bit(vlan_entry->bitmap,
						    YK3_ESW_QSET_BITMAP_BITS,
						    false)) {
			yk3_err("%s yk3 esw vlan qbmp not empty but member count is 0!",
				__func__);
			yk3_esw_bmp_set_bit(vlan_entry->bitmap, YK3_ESW_QSET_BITMAP_BITS,
					    qset, true);
			vlan_entry->member_cnt++;
			return -1;
		}
		yk3_esw_update_vlan_qset_bmp(esw_mgr, vlan_entry->qbmp_idx, vlan_entry->bitmap);
		reg = FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_ENABLE, 0) |
		      FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_QSET_BMP_INDEX, 0);
		yk3_wr32(esw_mgr->hw_base, offset, reg);
		memset(vlan_entry, 0, sizeof(*vlan_entry));
		vlan_entry->qbmp_idx = YK3_ESW_UNUSED_INDEX;
	} else {
		vlan_entry->qbmp_idx = yk3_esw_update_vlan_qset_bmp(esw_mgr,
								    vlan_entry->qbmp_idx,
								    vlan_entry->bitmap);
		if (vlan_entry->qbmp_idx < 0) {
			yk3_err("%s yk3 esw alloc new vlan qbmp ret %d failed!",
				__func__, vlan_entry->qbmp_idx);
			yk3_esw_bmp_set_bit(vlan_entry->bitmap, YK3_ESW_QSET_BITMAP_BITS,
					    qset, true);
			vlan_entry->member_cnt++;
			return -1;
		}
		reg = FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_ENABLE, 1) |
		      FIELD_PREP(YK3_LAN_ESW_VLAN_FILTER_QSET_BMP_INDEX,
				 vlan_entry->qbmp_idx & 0xfff);
		yk3_wr32(esw_mgr->hw_base, offset, reg);
	}

	yk3_debug("%s update vlan_entry id %d member num %d qbmp index %d ok!",
		  __func__, vlan_entry_idx, vlan_entry->member_cnt, vlan_entry->qbmp_idx);

	return 0;
}

#ifdef ALL_MC_FEATURE
static int yk3_esw_apply_all_mc(struct yk3_pdev_priv *pdev_priv, u16 qset, bool enable)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;
	struct yk3_esw_qset_bitmap *l2mc_qbmp;
	struct yk3_esw_qset_bitmap all_mc_bitmap;
	struct yk3_esw_mac_filter *mc_entry, *match_mc_entry;
	int mc_qbmp_index;

	yk3_debug("%s yk3 esw qset %d apply all mc!", __func__, qset);
	list_for_each_entry(l2mc_qbmp, &esw_mgr->l2mc_qbmp_list, qbmp_node) {
		memset(all_mc_bitmap.bitmap, 0, sizeof(all_mc_bitmap.bitmap));
		match_mc_entry = NULL;
		if (enable) {
			memcpy(all_mc_bitmap.bitmap, l2mc_qbmp->bitmap,
			       sizeof(l2mc_qbmp->bitmap) / 2);
			yk3_esw_bmp_merge(all_mc_bitmap.bitmap, esw_mgr->all_mc_qset,
					  YK3_LAN_RX_QSET_NUM);
			all_mc_bitmap.index = l2mc_qbmp->index;
			yk3_esw_bmp_set_bit(all_mc_bitmap.bitmap, YK3_ESW_QSET_BITMAP_BITS / 2,
					    qset, true);
			if (yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap) != 0) {
				yk3_err("yk3 esw apply all mc for qset %d to qbmp %d failed!",
					qset, l2mc_qbmp->index);
				return -1;
			}
		} else {
			mc_qbmp_index = l2mc_qbmp->index;
			list_for_each_entry(mc_entry, &esw_mgr->l2mc_key_list, key_node) {
				if (mc_entry->qset_bmp_idx == mc_qbmp_index) {
					match_mc_entry = mc_entry;
					break;
				}
			}
			if (!match_mc_entry) {
				yk3_err("yk3 esw apply all mc can not find mc entry use qbmp %d!",
					mc_qbmp_index);
				continue;
			}
			// user specified multicast entry used by this qset, skip disable process
			if (yk3_esw_bmp_get_bit_ref(mc_entry->qset_ref,
						    YK3_LAN_RX_QSET_NUM, qset) == 1)
				continue;

			memcpy(all_mc_bitmap.bitmap, l2mc_qbmp->bitmap,
			       sizeof(l2mc_qbmp->bitmap) / 2);
			yk3_esw_bmp_merge(all_mc_bitmap.bitmap, esw_mgr->all_mc_qset,
					  YK3_LAN_RX_QSET_NUM);
			all_mc_bitmap.index = l2mc_qbmp->index;
			if (yk3_esw_set_mc_qbmp(esw_mgr, &all_mc_bitmap) != 0) {
				yk3_err("yk3 esw disable all mc for qset %d to qbmp %d failed!",
					qset, l2mc_qbmp->index);
				return -1;
			}
		}
	}

	return 0;
}

int yk3_esw_set_all_mc(struct yk3_pdev_priv *pdev_priv, u16 qset, bool enable)
{
	struct yk3_esw_manager *esw_mgr = pdev_priv->lan->esw_mgr;

	yk3_debug("%s yk3 esw qset %d set all mc %s!", __func__, qset,
		  enable ? "enable" : "disable");

	if (enable) {
		if (yk3_esw_bmp_get_bit(esw_mgr->all_mc_qset,
					YK3_LAN_RX_QSET_NUM, qset)) {
			yk3_dev_debug("yk3 esw all mc already enabled for qset %d!", qset);
			return 0;
		}
		if (yk3_esw_bmp_set_bit(esw_mgr->all_mc_qset,
					YK3_LAN_RX_QSET_NUM, qset, true) != 0) {
			yk3_dev_err("yk3 esw set qset %d all mc bitmap failed!", qset);
			return -1;
		}
		if (yk3_esw_apply_all_mc(pdev_priv, qset, true) != 0) {
			yk3_dev_err("yk3 esw set all mc for qset %d failed!", qset);
			if (yk3_esw_bmp_set_bit(esw_mgr->all_mc_qset,
						YK3_LAN_RX_QSET_NUM, qset, false) != 0) {
				yk3_dev_err("yk3 esw rollback qset %d all mc bitmap failed!",
					    qset);
			}
			return -1;
		}
	} else {
		if (yk3_esw_bmp_get_bit(esw_mgr->all_mc_qset,
					YK3_LAN_RX_QSET_NUM, qset) != 1) {
			yk3_dev_debug("yk3 esw all mc not enabled for qset %d!", qset);
			return 0;
		}
		if (yk3_esw_bmp_set_bit(esw_mgr->all_mc_qset,
					YK3_LAN_RX_QSET_NUM, qset, false) != 0) {
			yk3_dev_err("yk3 esw unset qset %d all mc bitmap failed!", qset);
			return -1;
		}
		if (yk3_esw_apply_all_mc(pdev_priv, qset, false) != 0) {
			yk3_dev_err("yk3 esw disable all mc for qset %d failed!", qset);
			if (yk3_esw_bmp_set_bit(esw_mgr->all_mc_qset,
						YK3_LAN_RX_QSET_NUM, qset, true) != 0) {
				yk3_dev_err("yk3 esw rollback qset %d all mc bitmap failed!",
					    qset);
			}
			return -1;
		}
	}

	return 0;
}
#endif

static int yk3_dbgfs_esw_mac_show(struct seq_file *seq, void *v)
{
	struct yk3_esw_manager *esw_mgr = seq->private;

	if (!esw_mgr) {
		seq_puts(seq, "no esw info\n");
		return 0;
	}

	seq_puts(seq, "esw mac table config:\n");
	seq_printf(seq, "\t%-24s : %-8d\n", "uc mac limit",
		   esw_mgr->uc_key_limit);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc mac limit",
		   esw_mgr->mc_key_limit);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc qset bitmap base",
		   esw_mgr->uc_qbmp_base);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc qset bitmap limit",
		   esw_mgr->uc_qbmp_limit);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc qset bitmap base",
		   esw_mgr->mc_qbmp_base);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc qset bitmap limit",
		   esw_mgr->mc_qbmp_limit);
	seq_puts(seq, "\n");

	seq_puts(seq, "esw mac table usage:\n");
	seq_printf(seq, "\t%-24s : %-16d\n", "uc mac used",
		   esw_mgr->uc_key_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "mc mac used",
		   esw_mgr->mc_key_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "uc qset bitmap used",
		   esw_mgr->uc_qbmp_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "mc qset bitmap used",
		   esw_mgr->mc_qbmp_used);
	seq_puts(seq, "\n");

	seq_printf(seq, "\t%-24s : %-16d\n", "uc mac free",
		   esw_mgr->uc_key_limit - esw_mgr->uc_key_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "mc mac free",
		   esw_mgr->mc_key_limit - esw_mgr->mc_key_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "uc qset bitmap free",
		   esw_mgr->uc_qbmp_limit - esw_mgr->uc_qbmp_used);
	seq_printf(seq, "\t%-24s : %-16d\n", "mc qset bitmap free",
		   esw_mgr->mc_qbmp_limit - esw_mgr->mc_qbmp_used);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(yk3_dbgfs_esw_mac);

static int yk3_dbgfs_esw_vlan_show(struct seq_file *seq, void *v)
{
	int i;
	struct yk3_esw_manager *esw_mgr = seq->private;

	if (!esw_mgr) {
		seq_puts(seq, "\tno esw info\n");
		return 0;
	}

	seq_puts(seq, "802.1q vlan:\n");
	for (i = 1; i < 4095; i++)
		if (esw_mgr->vlan_entrys[i].member_cnt != 0)
			seq_printf(seq, "\tvlan%d member : %4d\n", i,
				   esw_mgr->vlan_entrys[i].member_cnt);
	seq_puts(seq, "\n");

	seq_puts(seq, "802.1ad vlan:\n");
	for (i = 4097; i < 8191; i++)
		if (esw_mgr->vlan_entrys[i].member_cnt != 0)
			seq_printf(seq, "\tvlan%d member : %4d\n", i - 4096,
				   esw_mgr->vlan_entrys[i].member_cnt);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(yk3_dbgfs_esw_vlan);

static void yk3_init_esw_udf(struct yk3_esw_manager *esw_mgr)
{
#define YK3_ESW_UC_MAC_LIMIT_DEFAULT 2048
#define YK3_ESW_MC_MAC_LIMIT_DEFAULT 2048
#define YK3_ESW_UC_QSET_BMP_BASE_DEFAULT 0
#define YK3_ESW_UC_QSET_BMP_LIMIT_DEFAULT 1024
#define YK3_ESW_MC_QSET_BMP_BASE_DEFAULT 1024
#define YK3_ESW_MC_QSET_BMP_LIMIT_DEFAULT 1024
	// TODO init param form card user define config info
	esw_mgr->uc_key_limit = YK3_ESW_UC_MAC_LIMIT_DEFAULT;
	esw_mgr->mc_key_limit = YK3_ESW_MC_MAC_LIMIT_DEFAULT;

	esw_mgr->uc_qbmp_base = YK3_ESW_UC_QSET_BMP_BASE_DEFAULT;
	esw_mgr->uc_qbmp_limit = YK3_ESW_UC_QSET_BMP_LIMIT_DEFAULT;
	esw_mgr->mc_qbmp_base = YK3_ESW_MC_QSET_BMP_BASE_DEFAULT;
	esw_mgr->mc_qbmp_limit = YK3_ESW_MC_QSET_BMP_LIMIT_DEFAULT;
#undef YK3_ESW_UC_MAC_LIMIT_DEFAULT
#undef YK3_ESW_MC_MAC_LIMIT_DEFAULT
#undef YK3_ESW_UC_QSET_BMP_BASE_DEFAULT
#undef YK3_ESW_UC_QSET_BMP_LIMIT_DEFAULT
#undef YK3_ESW_MC_QSET_BMP_BASE_DEFAULT
#undef YK3_ESW_MC_QSET_BMP_LIMIT_DEFAULT
}

static int yk3_init_esw_mgr(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_esw_manager *esw_mgr;
	int i;

	if (!pdev_priv->lan)
		return -EINVAL;

	pdev_priv->lan->esw_mgr = kzalloc(sizeof(*pdev_priv->lan->esw_mgr), GFP_KERNEL);
	if (IS_ERR_OR_NULL(pdev_priv->lan->esw_mgr))
		return -ENOMEM;

	esw_mgr = pdev_priv->lan->esw_mgr;

	esw_mgr->hw_base = pdev_priv->lan->hw_addr;
	INIT_LIST_HEAD(&esw_mgr->l2uc_key_list);
	INIT_LIST_HEAD(&esw_mgr->l2uc_qbmp_list);
	INIT_LIST_HEAD(&esw_mgr->l2mc_key_list);
	INIT_LIST_HEAD(&esw_mgr->l2mc_qbmp_list);

	/* init esw cuckoo table */
	esw_mgr->hw_type = YK3_HW_TYPE_K3;
	if (esw_mgr->hw_type == YK3_HW_TYPE_K3) {
		esw_mgr->lan_mac_table =
			yk3_esw_cuckoo_create(YK3_CUCKOO_TYPE_K3_LAN_MAC,
					      0, 0, esw_mgr->hw_base);
	} else {
		yk3_err("yk3 lan esw init with invalid hardware type %d!\n",
			esw_mgr->hw_type);
		kfree(esw_mgr);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(esw_mgr->lan_mac_table)) {
		yk3_err("yk3 lan esw create cuckoo hash table failed!\n");
		kfree(esw_mgr);
		return -EFAULT;
	}
	yk3_init_esw_udf(esw_mgr);

	esw_mgr->vlan_qbmp_num = YK3_LAN_ESW_VLAN_QSET_BMP_NUMS;
	esw_mgr->nic_mode = 0;
	// non-bnic mode kernel vlan use first half vlan qbmp resource
	if (esw_mgr->nic_mode != 0)
		esw_mgr->vlan_qbmp_num = YK3_LAN_ESW_VLAN_QSET_BMP_NUMS / 2;
	// 8K vlan entry = 802.1Q 4K + 802.1ad 4K
	esw_mgr->vlan_entrys = kzalloc(sizeof(*esw_mgr->vlan_entrys) * 8192, GFP_KERNEL);
	if (IS_ERR_OR_NULL(esw_mgr->vlan_entrys)) {
		yk3_esw_cuckoo_destroy(esw_mgr->lan_mac_table);
		kfree(esw_mgr);
		return -ENOMEM;
	}

	for (i = 0; i < 8192; i++)
		esw_mgr->vlan_entrys[i].qbmp_idx = YK3_ESW_UNUSED_INDEX;
	INIT_LIST_HEAD(&esw_mgr->vlan_qset_bmp_list);

	esw_mgr->dbgfs_esw_dir = debugfs_create_dir("esw",
						    pdev_priv->card->dbgfs_dir);
	if (IS_ERR_OR_NULL(esw_mgr->dbgfs_esw_dir))
		yk3_err("create card %s debugfs directory for esw failed!\n",
			pdev_priv->card->name);

	esw_mgr->dbgfs_esw_mac_file = debugfs_create_file("mac", 0444,
							  esw_mgr->dbgfs_esw_dir,
							  esw_mgr,
							  &yk3_dbgfs_esw_mac_fops);
	if (IS_ERR_OR_NULL(esw_mgr->dbgfs_esw_mac_file))
		yk3_err("create card %s debugfs file for esw mac failed!\n",
			pdev_priv->card->name);

	esw_mgr->dbgfs_esw_vlan_file = debugfs_create_file("vlan", 0444,
							   esw_mgr->dbgfs_esw_dir,
							   esw_mgr,
							   &yk3_dbgfs_esw_vlan_fops);
	if (IS_ERR_OR_NULL(esw_mgr->dbgfs_esw_vlan_file))
		yk3_err("create card %s debugfs file for esw vlan failed!\n",
			pdev_priv->card->name);

	return 0;
}

static int yk3_uninit_esw_mgr(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_esw_manager *esw_mgr;
	struct yk3_esw_mac_filter *l2uc_key, *temp_uc;
	struct yk3_esw_qset_bitmap *l2uc_qbmp, *temp_uc_qbmp;
	struct yk3_esw_mac_filter *l2mc_key, *temp_mc;
	struct yk3_esw_qset_bitmap *l2mc_qbmp, *temp_mc_qbmp;
	struct yk3_esw_vlan_qbmp *vlan_qbmp, *temp_vlan_qbmp;

	esw_mgr = pdev_priv->lan->esw_mgr;
	if (!esw_mgr)
		return -EINVAL;

	debugfs_remove_recursive(esw_mgr->dbgfs_esw_dir);
	kfree(esw_mgr->vlan_entrys);

	list_for_each_entry_safe(l2uc_qbmp, temp_uc_qbmp, &esw_mgr->l2uc_qbmp_list,
				 qbmp_node) {
		list_del(&l2uc_qbmp->qbmp_node);
		kfree(l2uc_qbmp);
	}

	list_for_each_entry_safe(l2uc_key, temp_uc, &esw_mgr->l2uc_key_list,
				 key_node) {
		list_del(&l2uc_key->key_node);
		kfree(l2uc_key);
	}

	list_for_each_entry_safe(l2mc_qbmp, temp_mc_qbmp, &esw_mgr->l2mc_qbmp_list,
				 qbmp_node) {
		list_del(&l2mc_qbmp->qbmp_node);
		kfree(l2mc_qbmp);
	}

	list_for_each_entry_safe(l2mc_key, temp_mc, &esw_mgr->l2mc_key_list,
				 key_node) {
		list_del(&l2mc_key->key_node);
		kfree(l2mc_key);
	}

	list_for_each_entry_safe(vlan_qbmp, temp_vlan_qbmp, &esw_mgr->vlan_qset_bmp_list,
				 vlan_qbmp_node) {
		list_del(&vlan_qbmp->vlan_qbmp_node);
		kfree(vlan_qbmp);
	}
	yk3_esw_cuckoo_destroy(esw_mgr->lan_mac_table);
	kfree(esw_mgr);
	pdev_priv->lan->esw_mgr = NULL;

	return 0;
}

int yk3_init_esw(struct yk3_pdev_priv *pdev_priv)
{
	return yk3_init_esw_mgr(pdev_priv);
}

int yk3_uninit_esw(struct yk3_pdev_priv *pdev_priv)
{
	return yk3_uninit_esw_mgr(pdev_priv);
}
