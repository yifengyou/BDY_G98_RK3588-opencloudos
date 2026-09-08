/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_PPP_EME_H
#define __YK3_PPP_EME_H

#include "yk3_base.h"

#define YK3_PPP_EME_BAR				(0)

#define YK3_PPP_EME_BUCKET_NUM			(4)
#define YK3_PPP_EME_DEPTH			(256)
#define YK3_PPP_EME_KEY_SIZE			(64)
#define YK3_PPP_EME_VALUE_SIZE			(2)
#define YK3_PPP_EME_SEED_BITS			(32)
#define YK3_PPP_EME_MUX_SEED_BITS		(5)

#define YK3_PPP_EME_KEY_SIZE_SHARE		(14)
#define YK3_PPP_EME_KEY_SIZE_EXCLUSIVE		(30)
#define YK3_PPP_EME_KEY_SIZE_MULTI		(60)

#define YK3_PPP_EME_WADDR			(0x5000)
#define YK3_PPP_EME_DATA_ADDR			(0x5004)
#define YK3_PPP_EME_RADDR			(0x5008)
#define YK3_PPP_EME_SEED_ADDR(index)		(0x500C + 4 * (index))
#define YK3_PPP_EME_MUX_SEED_ADDR(index)	(0x502C + 4 * (index))
#define YK3_PPP_EME_INIT_DONE			(0x5100)
#define YK3_PPP_EME_DATA_ROUND			(8)

#define YK3_PPP_EME_MAX_INSERT_RETRIES		(10)
#define YK3_PPP_EME_MAX_CREATE_SEED_RETRIES	(16)

#define YK3_PPP_EME_BITMASK0_WADDR		(0x4078)
#define YK3_PPP_EME_BITMASK0_DATA_ADDR		(0x407C)
#define YK3_PPP_EME_BITMASK0_RADDR		(0x4080)
#define YK3_PPP_EME_BITMASK1_WADDR		(0x4084)
#define YK3_PPP_EME_BITMASK1_DATA_ADDR		(0x4088)
#define YK3_PPP_EME_BITMASK1_RADDR		(0x408C)

#define YK3_PPP_EME_NEW_RULE			(0xFFFFFFFF)
#define YK3_PPP_EME_DISCARD_RULE		(0xFFFFFFFE)
#define YK3_PPP_EME_ADDED_RULE			(0xFFFFFFFD)

#define YK3_PPP_EME_KEY_TYPE_SHARE		(0x00)
#define YK3_PPP_EME_KEY_TYPE_EXCLUSIVE		(0x40)
#define YK3_PPP_EME_KEY_TYPE_MULTI_HEAD		(0x80)
#define YK3_PPP_EME_KEY_TYPE_MULTI_TAIL		(0xa0)

#define YK3_PPP_EME_MAX_TABLE_NUM		(16)

#define YK3_PPP_EME_KEY_TID_MASK		GENMASK(5, 0)
#define YK3_PPP_EME_KEY_TYPE_MASK		GENMASK(7, 6)
#define YK3_PPP_EME_VALUE_TYPE_MASK		GENMASK(7, 5)
#define YK3_PPP_EME_NXTINDEX_MASK		GENMASK(12, 0)

enum YK3_PPP_EME_HW_ID {
	YK3_PPP_EME_HW_ID_MAC0,
	YK3_PPP_EME_HW_ID_MAC1,
	YK3_PPP_EME_HW_ID_HOST,
	YK3_PPP_EME_HW_ID_SOC,
	YK3_PPP_EME_HW_ID_MAX,
};

struct yk3_ppp_eme_entry {
	u8 is_occupied;
	u8 key[YK3_PPP_EME_KEY_SIZE];
	u8 value[YK3_PPP_EME_VALUE_SIZE];
	u8 type;
	union {
		struct {
			u8 secondary_key[YK3_PPP_EME_KEY_SIZE];
			u8 secondary_value[YK3_PPP_EME_VALUE_SIZE];
		};
		struct {
			int pre_index;
			int next_index;
		};
	};
};

struct yk3_ppp_eme_hw {
	void __iomem *bar_addr;
	u32 init_done_addr;
	u32 seed_addr[YK3_PPP_EME_BUCKET_NUM];
	u32 mux_seed_addr[YK3_PPP_EME_BUCKET_NUM];
	u32 waddr;
	u32 raddr;
	u32 data_addr;
	u32 data_round;
};

struct yk3_ppp_eme_kick {
	struct yk3_ppp_eme_entry entry;
	u32 from_bucket;
	u32 to_bucket;
	u32 from_pos;
	u32 to_pos;
};

struct yk3_ppp_eme_kick_stream {
	struct yk3_ppp_eme_kick kicks[YK3_PPP_EME_MAX_INSERT_RETRIES];
	u32 count;
};

struct yk3_ppp_eme_table {
	u32 table_id;
	u32 key_size;
	u32 key_payload_size;
	u32 value_size;
	u8 key_type;
	u8 key_hdr;
	u32 key_hdr_size;
	u8 is_created;
};

struct yk3_ppp_eme {
	u32 bucket_count;
	u32 depth;
	u32 seed_bits;          /* bits */
	u32 mux_seed_bits;      /* bits */
	u32 seed[YK3_PPP_EME_BUCKET_NUM];
	u32 mux_seed[YK3_PPP_EME_BUCKET_NUM];
	u32 buckets_entry_num[YK3_PPP_EME_BUCKET_NUM];
	struct yk3_ppp_eme_entry buckets[YK3_PPP_EME_BUCKET_NUM][YK3_PPP_EME_DEPTH];
	struct yk3_ppp_eme_table tables[YK3_PPP_EME_MAX_TABLE_NUM];
	struct yk3_ppp_eme_hw hw;
	u32 hw_offset[8];
	int hw_id;
	/* kick operation workspace to reduce stack usage */
	struct {
		struct yk3_ppp_eme_entry kicked_entry;
		struct yk3_ppp_eme_kick kick;
		struct yk3_ppp_eme_kick ori_kicks[3];
		int ori_kcnt;
	} kick_ws;
};

struct yk3_ppp_eme_mgr {
	u32 init_done;
	struct yk3_ppp_eme *emes[YK3_PPP_EME_HW_ID_MAX];
	spinlock_t opt_lock;	// eme opt lock
};

int yk3_ppp_eme_init(struct yk3_pdev_priv *pdev_priv);
void yk3_ppp_eme_uninit(struct yk3_pdev_priv *pdev_priv);

int yk3_ppp_eme_create_table(struct yk3_pdev_priv *pdev_priv, int hw_id,
			     u32 table_id, u32 key_size, u32 value_size);
int yk3_ppp_eme_delete_table(struct yk3_pdev_priv *pdev_priv, int hw_id,
			     u32 table_id);
int yk3_ppp_eme_add_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
			 u32 table_id, u8 *key, u8 *value);
int yk3_ppp_eme_del_rule(struct yk3_pdev_priv *pdev_priv,
			 int hw_id, u32 table_id, u8 *key);
int yk3_ppp_eme_mod_rule(struct yk3_pdev_priv *pdev_priv,
			 int hw_id, u32 table_id, u8 *key, u8 *value);
int yk3_ppp_eme_get_rule(struct yk3_pdev_priv *pdev_priv,
			 int hw_id, u32 table_id, u8 *key, u8 *value);

#endif
