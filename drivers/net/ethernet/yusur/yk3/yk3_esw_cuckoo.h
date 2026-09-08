/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_ESW_CUCKOO_H
#define _YK3_ESW_CUCKOO_H

#include "yk3.h"

enum {
	YK3_CUCKOO_TYPE_K3_LAN_MAC,
	YK3_CUCKOO_TYPE_END,
};

#define YK3_ESW_CUCKOO_MAX_BUCKETS        (3)
#define YK3_ESW_CUCKOO_MAX_KEY_SIZE       (16)
#define YK3_ESW_CUCKOO_MAX_VALUE_SIZE     (16)
#define YK3_ESW_CUCKOO_MAX_DATA_ROUND     ((YK3_ESW_CUCKOO_MAX_KEY_SIZE + \
					   YK3_ESW_CUCKOO_MAX_VALUE_SIZE) / sizeof(u32))

struct yk3_esw_cuckoo_hw {
	void *ctx;
	void __iomem *hw_addr;
	u8 pf_id;
	u32 init_done_addr;
	u32 seed_addr[YK3_ESW_CUCKOO_MAX_BUCKETS];
	u32 mux_seed_addr[YK3_ESW_CUCKOO_MAX_BUCKETS];
	u32 waddr;
	u32 raddr;
	u32 data_addr;
	u32 data_round;
};

struct yk3_esw_cuckoo_table {
	u32 type;
	u32 flag;
	u32 bucket_count;
	u32 depth;
	u32 key_size;		/* bytes */
	u32 value_size;		/* bytes */
	u32 seed_bits;		/* bits */
	u32 mux_seed_bits;	/* bits */
	u32 init_done;
	u32 seed[YK3_ESW_CUCKOO_MAX_BUCKETS];
	u32 mux_seed[YK3_ESW_CUCKOO_MAX_BUCKETS];
	u32 buckets_entry_num[YK3_ESW_CUCKOO_MAX_BUCKETS];
	struct yk3_esw_cuckoo_hw hw;
	u32 (*yk3_esw_cuckoo_table_init)(struct yk3_esw_cuckoo_table *table);
	u32 (*yk3_esw_cuckoo_table_uninit)(struct yk3_esw_cuckoo_table *table);
};

struct yk3_esw_cuckoo_entry {
	u8 is_occupied;
	u8 key[YK3_ESW_CUCKOO_MAX_KEY_SIZE];
	u8 value[YK3_ESW_CUCKOO_MAX_VALUE_SIZE];
};

struct yk3_esw_cuckoo_kick {
	struct yk3_esw_cuckoo_entry entry;
	u32 from_bucket;
	u32 to_bucket;
	u32 from_pos;
	u32 to_pos;
};

struct yk3_esw_cuckoo_table_uncached {
	struct yk3_esw_cuckoo_table table_base;
	const struct yk3_esw_cuckoo_ops_uncached *ops;
	struct yk3_esw_cuckoo_entry entry_swap;
};

struct yk3_esw_cuckoo_ops_uncached {
	void (*get_hw_info)(struct yk3_esw_cuckoo_table_uncached *table);
	u32 (*hash)(const u8 *key, u32 seed, u32 mux_seed);
	u32 (*get_ram_addr)(u32 bucket, u32 pos);
	void (*generate_rule_data)(const u8 *key, const u8 *value,
				   u32 *data);
	void (*parse_rule_data)(const u32 *data, u8 *key, u8 *value);
	int (*store_rule_data)(struct yk3_esw_cuckoo_table_uncached *table,
			       u8 bucket, u32 pos);
	int (*store_rule_data2file)(struct yk3_esw_cuckoo_table_uncached *table,
				    u8 bucket, u32 pos);
	int (*backup_entry)(struct yk3_esw_cuckoo_table_uncached *table,
			    struct yk3_esw_cuckoo_entry entry);
};

struct yk3_esw_cuckoo_table *yk3_esw_cuckoo_create(u32 type, u32 flag, u32 pf_id,
						   void __iomem *bar_addr);
void yk3_esw_cuckoo_destroy(struct yk3_esw_cuckoo_table *table);
int yk3_esw_cuckoo_insert(struct yk3_esw_cuckoo_table *table, const u8 *key, const u8 *value);
int yk3_esw_cuckoo_delete(struct yk3_esw_cuckoo_table *table, const u8 *key);
int yk3_esw_cuckoo_change(struct yk3_esw_cuckoo_table *table, const u8 *key, const u8 *value);
int yk3_esw_cuckoo_search(struct yk3_esw_cuckoo_table *table, const u8 *key, u8 *value,
			  u32 *bucket, u32 *pos);

#endif /* _YK3_ESW_CUCKOO_H */
