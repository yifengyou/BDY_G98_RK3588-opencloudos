// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_lan_regs.h"
#include "yk3_esw_cuckoo.h"

#define YK3_ESW_CUCKOO_MAX_INSERT_RETRIES      (24)
#define YK3_ESW_CUCKOO_NEW_RULE                (0xFFFFFFFF)

struct yk3_esw_cuckoo_kick_stream {
	struct yk3_esw_cuckoo_kick kicks[YK3_ESW_CUCKOO_MAX_INSERT_RETRIES];
	u32 count;
};

static inline u32 yk3_esw_cuckooo_ioread32(struct yk3_esw_cuckoo_table *table, u32 reg)
{
	return
	    yk3_rd32((void __iomem *)((uintptr_t)(table->hw.hw_addr)), reg);
}

static inline int yk3_esw_cuckoo_iowrite32(struct yk3_esw_cuckoo_table *table,
					   u32 reg, u32 val)
{
	u32 retry = 3;

	do {
		yk3_wr32((void __iomem *)((uintptr_t)(table->hw.hw_addr)), reg, val);
		if (yk3_esw_cuckooo_ioread32(table, reg) == val)
			break;
	} while (retry--);

	if (retry == 0)
		return -1;

	return 0;
}

/* k3 one mac table version */
#define YK3_ESW_CUCKOO_MAC_BUCKET_NUM           (3)
#define YK3_ESW_CUCKOO_K3_MAC_DEPTH            (2048)
#define YK3_ESW_CUCKOO_MAC_KEY_SIZE             (6)
#define YK3_ESW_CUCKOO_MAC_VALUE_SIZE           (8)
#define YK3_ESW_CUCKOO_MAC_SEED_BITS            (32)
#define YK3_ESW_CUCKOO_MAC_MUX_SEED_BITS        (5)

#define YK3_ESW_CUCKOO_MAC_WADDR                (YK3_LAN_BASE + 0x120000)
#define YK3_ESW_CUCKOO_MAC_RADDR                YK3_ESW_CUCKOO_MAC_WADDR
#define YK3_ESW_CUCKOO_MAC_SEED_ADDR(index)     (YK3_LAN_BASE + 0x100110 + 4 * (index))
#define YK3_ESW_CUCKOO_MAC_MUX_SEED_ADDR(index) (YK3_LAN_BASE + 0x100100 + 4 * (index))
#define YK3_ESW_CUCKOO_MAC_DATA_ROUND           (2)

static inline u32 yk3_esw_cuckooo_ioread32_direct(uintptr_t reg_addr)
{
	return yk3_rd32((void __iomem *)(reg_addr), 0);
}

static inline int yk3_esw_cuckoo_iowrite32_direct(uintptr_t reg_addr, u32 val)
{
	u32 retry = 3;

	do {
		yk3_wr32((void __iomem *)(reg_addr), 0, val);
		if (yk3_esw_cuckooo_ioread32_direct(reg_addr) == val)
			break;
	} while (retry--);

	if (retry == 0)
		return -1;

	return 0;
}

static u32 yk3_esw_cuckoo_mac_crc32_48bit(const u8 *key, u32 seed)
{
	u32 crc = 0;
	u8 c[32] = { 0 };
	u8 q[32] = { 0 };
	u8 in[48] = { 0 };
	int i, j;

	for (i = 0; i < 32; i++)
		q[i] = (seed >> i) & 0x1;

	for (i = 0; i < 6; i++)
		for (j = 0; j < 8; j++)
			in[i * 8 + j] = (key[i] >> j) & 0x1;

	c[0] =
	    q[0] ^ q[8] ^ q[9] ^ q[10] ^ q[12] ^ q[13] ^ q[14] ^ q[15] ^ q[16] ^
	    q[18] ^ q[21] ^ q[28] ^ q[29] ^ q[31] ^ in[0] ^ in[6] ^ in[9] ^
	    in[10] ^ in[12] ^ in[16] ^ in[24] ^ in[25] ^ in[26] ^ in[28] ^
	    in[29] ^ in[30] ^ in[31] ^ in[32] ^ in[34] ^ in[37] ^ in[44] ^
	    in[45] ^ in[47];
	c[1] =
	    q[0] ^ q[1] ^ q[8] ^ q[11] ^ q[12] ^ q[17] ^ q[18] ^ q[19] ^ q[21] ^
	    q[22] ^ q[28] ^ q[30] ^ q[31] ^ in[0] ^ in[1] ^ in[6] ^ in[7] ^
	    in[9] ^ in[11] ^ in[12] ^ in[13] ^ in[16] ^ in[17] ^ in[24] ^ in[27]
	    ^ in[28] ^ in[33] ^ in[34] ^ in[35] ^ in[37] ^ in[38] ^ in[44] ^
	    in[46] ^ in[47];
	c[2] =
	    q[0] ^ q[1] ^ q[2] ^ q[8] ^ q[10] ^ q[14] ^ q[15] ^ q[16] ^ q[19] ^
	    q[20] ^ q[21] ^ q[22] ^ q[23] ^ q[28] ^ in[0] ^ in[1] ^ in[2] ^
	    in[6] ^ in[7] ^ in[8] ^ in[9] ^ in[13] ^ in[14] ^ in[16] ^ in[17] ^
	    in[18] ^ in[24] ^ in[26] ^ in[30] ^ in[31] ^ in[32] ^ in[35] ^
	    in[36] ^ in[37] ^ in[38] ^ in[39] ^ in[44];
	c[3] =
	    q[1] ^ q[2] ^ q[3] ^ q[9] ^ q[11] ^ q[15] ^ q[16] ^ q[17] ^ q[20] ^
	    q[21] ^ q[22] ^ q[23] ^ q[24] ^ q[29] ^ in[1] ^ in[2] ^ in[3] ^
	    in[7] ^ in[8] ^ in[9] ^ in[10] ^ in[14] ^ in[15] ^ in[17] ^ in[18] ^
	    in[19] ^ in[25] ^ in[27] ^ in[31] ^ in[32] ^ in[33] ^ in[36] ^
	    in[37] ^ in[38] ^ in[39] ^ in[40] ^ in[45];
	c[4] =
	    q[2] ^ q[3] ^ q[4] ^ q[8] ^ q[9] ^ q[13] ^ q[14] ^ q[15] ^ q[17] ^
	    q[22] ^ q[23] ^ q[24] ^ q[25] ^ q[28] ^ q[29] ^ q[30] ^ q[31] ^
	    in[0] ^ in[2] ^ in[3] ^ in[4] ^ in[6] ^ in[8] ^ in[11] ^ in[12] ^
	    in[15] ^ in[18] ^ in[19] ^ in[20] ^ in[24] ^ in[25] ^ in[29] ^
	    in[30] ^ in[31] ^ in[33] ^ in[38] ^ in[39] ^ in[40] ^ in[41] ^
	    in[44] ^ in[45] ^ in[46] ^ in[47];
	c[5] =
	    q[3] ^ q[4] ^ q[5] ^ q[8] ^ q[12] ^ q[13] ^ q[21] ^ q[23] ^ q[24] ^
	    q[25] ^ q[26] ^ q[28] ^ q[30] ^ in[0] ^ in[1] ^ in[3] ^ in[4] ^
	    in[5] ^ in[6] ^ in[7] ^ in[10] ^ in[13] ^ in[19] ^ in[20] ^ in[21] ^
	    in[24] ^ in[28] ^ in[29] ^ in[37] ^ in[39] ^ in[40] ^ in[41] ^
	    in[42] ^ in[44] ^ in[46];
	c[6] =
	    q[4] ^ q[5] ^ q[6] ^ q[9] ^ q[13] ^ q[14] ^ q[22] ^ q[24] ^ q[25] ^
	    q[26] ^ q[27] ^ q[29] ^ q[31] ^ in[1] ^ in[2] ^ in[4] ^ in[5] ^
	    in[6] ^ in[7] ^ in[8] ^ in[11] ^ in[14] ^ in[20] ^ in[21] ^ in[22] ^
	    in[25] ^ in[29] ^ in[30] ^ in[38] ^ in[40] ^ in[41] ^ in[42] ^
	    in[43] ^ in[45] ^ in[47];
	c[7] =
	    q[0] ^ q[5] ^ q[6] ^ q[7] ^ q[8] ^ q[9] ^ q[12] ^ q[13] ^ q[16] ^
	    q[18] ^ q[21] ^ q[23] ^ q[25] ^ q[26] ^ q[27] ^ q[29] ^ q[30] ^
	    q[31] ^ in[0] ^ in[2] ^ in[3] ^ in[5] ^ in[7] ^ in[8] ^ in[10] ^
	    in[15] ^ in[16] ^ in[21] ^ in[22] ^ in[23] ^ in[24] ^ in[25] ^
	    in[28] ^ in[29] ^ in[32] ^ in[34] ^ in[37] ^ in[39] ^ in[41] ^
	    in[42] ^ in[43] ^ in[45] ^ in[46] ^ in[47];
	c[8] =
	    q[1] ^ q[6] ^ q[7] ^ q[12] ^ q[15] ^ q[16] ^ q[17] ^ q[18] ^ q[19] ^
	    q[21] ^ q[22] ^ q[24] ^ q[26] ^ q[27] ^ q[29] ^ q[30] ^ in[0] ^
	    in[1] ^ in[3] ^ in[4] ^ in[8] ^ in[10] ^ in[11] ^ in[12] ^ in[17] ^
	    in[22] ^ in[23] ^ in[28] ^ in[31] ^ in[32] ^ in[33] ^ in[34] ^
	    in[35] ^ in[37] ^ in[38] ^ in[40] ^ in[42] ^ in[43] ^ in[45] ^
	    in[46];
	c[9] =
	    q[2] ^ q[7] ^ q[8] ^ q[13] ^ q[16] ^ q[17] ^ q[18] ^ q[19] ^ q[20] ^
	    q[22] ^ q[23] ^ q[25] ^ q[27] ^ q[28] ^ q[30] ^ q[31] ^ in[1] ^
	    in[2] ^ in[4] ^ in[5] ^ in[9] ^ in[11] ^ in[12] ^ in[13] ^ in[18] ^
	    in[23] ^ in[24] ^ in[29] ^ in[32] ^ in[33] ^ in[34] ^ in[35] ^
	    in[36] ^ in[38] ^ in[39] ^ in[41] ^ in[43] ^ in[44] ^ in[46] ^
	    in[47];
	c[10] =
	    q[0] ^ q[3] ^ q[10] ^ q[12] ^ q[13] ^ q[15] ^ q[16] ^ q[17] ^ q[19]
	    ^ q[20] ^ q[23] ^ q[24] ^ q[26] ^ in[0] ^ in[2] ^ in[3] ^ in[5] ^
	    in[9] ^ in[13] ^ in[14] ^ in[16] ^ in[19] ^ in[26] ^ in[28] ^ in[29]
	    ^ in[31] ^ in[32] ^ in[33] ^ in[35] ^ in[36] ^ in[39] ^ in[40] ^
	    in[42];
	c[11] =
	    q[0] ^ q[1] ^ q[4] ^ q[8] ^ q[9] ^ q[10] ^ q[11] ^ q[12] ^ q[15] ^
	    q[17] ^ q[20] ^ q[24] ^ q[25] ^ q[27] ^ q[28] ^ q[29] ^ q[31] ^
	    in[0] ^ in[1] ^ in[3] ^ in[4] ^ in[9] ^ in[12] ^ in[14] ^ in[15] ^
	    in[16] ^ in[17] ^ in[20] ^ in[24] ^ in[25] ^ in[26] ^ in[27] ^
	    in[28] ^ in[31] ^ in[33] ^ in[36] ^ in[40] ^ in[41] ^ in[43] ^
	    in[44] ^ in[45] ^ in[47];
	c[12] =
	    q[1] ^ q[2] ^ q[5] ^ q[8] ^ q[11] ^ q[14] ^ q[15] ^ q[25] ^ q[26] ^
	    q[30] ^ q[31] ^ in[0] ^ in[1] ^ in[2] ^ in[4] ^ in[5] ^ in[6] ^
	    in[9] ^ in[12] ^ in[13] ^ in[15] ^ in[17] ^ in[18] ^ in[21] ^ in[24]
	    ^ in[27] ^ in[30] ^ in[31] ^ in[41] ^ in[42] ^ in[46] ^ in[47];
	c[13] =
	    q[0] ^ q[2] ^ q[3] ^ q[6] ^ q[9] ^ q[12] ^ q[15] ^ q[16] ^ q[26] ^
	    q[27] ^ q[31] ^ in[1] ^ in[2] ^ in[3] ^ in[5] ^ in[6] ^ in[7] ^
	    in[10] ^ in[13] ^ in[14] ^ in[16] ^ in[18] ^ in[19] ^ in[22] ^
	    in[25] ^ in[28] ^ in[31] ^ in[32] ^ in[42] ^ in[43] ^ in[47];
	c[14] =
	    q[1] ^ q[3] ^ q[4] ^ q[7] ^ q[10] ^ q[13] ^ q[16] ^ q[17] ^ q[27] ^
	    q[28] ^ in[2] ^ in[3] ^ in[4] ^ in[6] ^ in[7] ^ in[8] ^ in[11] ^
	    in[14] ^ in[15] ^ in[17] ^ in[19] ^ in[20] ^ in[23] ^ in[26] ^
	    in[29] ^ in[32] ^ in[33] ^ in[43] ^ in[44];
	c[15] =
	    q[0] ^ q[2] ^ q[4] ^ q[5] ^ q[8] ^ q[11] ^ q[14] ^ q[17] ^ q[18] ^
	    q[28] ^ q[29] ^ in[3] ^ in[4] ^ in[5] ^ in[7] ^ in[8] ^ in[9] ^
	    in[12] ^ in[15] ^ in[16] ^ in[18] ^ in[20] ^ in[21] ^ in[24] ^
	    in[27] ^ in[30] ^ in[33] ^ in[34] ^ in[44] ^ in[45];
	c[16] =
	    q[1] ^ q[3] ^ q[5] ^ q[6] ^ q[8] ^ q[10] ^ q[13] ^ q[14] ^ q[16] ^
	    q[19] ^ q[21] ^ q[28] ^ q[30] ^ q[31] ^ in[0] ^ in[4] ^ in[5] ^
	    in[8] ^ in[12] ^ in[13] ^ in[17] ^ in[19] ^ in[21] ^ in[22] ^ in[24]
	    ^ in[26] ^ in[29] ^ in[30] ^ in[32] ^ in[35] ^ in[37] ^ in[44] ^
	    in[46] ^ in[47];
	c[17] =
	    q[2] ^ q[4] ^ q[6] ^ q[7] ^ q[9] ^ q[11] ^ q[14] ^ q[15] ^ q[17] ^
	    q[20] ^ q[22] ^ q[29] ^ q[31] ^ in[1] ^ in[5] ^ in[6] ^ in[9] ^
	    in[13] ^ in[14] ^ in[18] ^ in[20] ^ in[22] ^ in[23] ^ in[25] ^
	    in[27] ^ in[30] ^ in[31] ^ in[33] ^ in[36] ^ in[38] ^ in[45] ^
	    in[47];
	c[18] =
	    q[3] ^ q[5] ^ q[7] ^ q[8] ^ q[10] ^ q[12] ^ q[15] ^ q[16] ^ q[18] ^
	    q[21] ^ q[23] ^ q[30] ^ in[2] ^ in[6] ^ in[7] ^ in[10] ^ in[14] ^
	    in[15] ^ in[19] ^ in[21] ^ in[23] ^ in[24] ^ in[26] ^ in[28] ^
	    in[31] ^ in[32] ^ in[34] ^ in[37] ^ in[39] ^ in[46];
	c[19] =
	    q[0] ^ q[4] ^ q[6] ^ q[8] ^ q[9] ^ q[11] ^ q[13] ^ q[16] ^ q[17] ^
	    q[19] ^ q[22] ^ q[24] ^ q[31] ^ in[3] ^ in[7] ^ in[8] ^ in[11] ^
	    in[15] ^ in[16] ^ in[20] ^ in[22] ^ in[24] ^ in[25] ^ in[27] ^
	    in[29] ^ in[32] ^ in[33] ^ in[35] ^ in[38] ^ in[40] ^ in[47];
	c[20] =
	    q[0] ^ q[1] ^ q[5] ^ q[7] ^ q[9] ^ q[10] ^ q[12] ^ q[14] ^ q[17] ^
	    q[18] ^ q[20] ^ q[23] ^ q[25] ^ in[4] ^ in[8] ^ in[9] ^ in[12] ^
	    in[16] ^ in[17] ^ in[21] ^ in[23] ^ in[25] ^ in[26] ^ in[28] ^
	    in[30] ^ in[33] ^ in[34] ^ in[36] ^ in[39] ^ in[41];
	c[21] =
	    q[1] ^ q[2] ^ q[6] ^ q[8] ^ q[10] ^ q[11] ^ q[13] ^ q[15] ^ q[18] ^
	    q[19] ^ q[21] ^ q[24] ^ q[26] ^ in[5] ^ in[9] ^ in[10] ^ in[13] ^
	    in[17] ^ in[18] ^ in[22] ^ in[24] ^ in[26] ^ in[27] ^ in[29] ^
	    in[31] ^ in[34] ^ in[35] ^ in[37] ^ in[40] ^ in[42];
	c[22] =
	    q[0] ^ q[2] ^ q[3] ^ q[7] ^ q[8] ^ q[10] ^ q[11] ^ q[13] ^ q[15] ^
	    q[18] ^ q[19] ^ q[20] ^ q[21] ^ q[22] ^ q[25] ^ q[27] ^ q[28] ^
	    q[29] ^ q[31] ^ in[0] ^ in[9] ^ in[11] ^ in[12] ^ in[14] ^ in[16] ^
	    in[18] ^ in[19] ^ in[23] ^ in[24] ^ in[26] ^ in[27] ^ in[29] ^
	    in[31] ^ in[34] ^ in[35] ^ in[36] ^ in[37] ^ in[38] ^ in[41] ^
	    in[43] ^ in[44] ^ in[45] ^ in[47];
	c[23] =
	    q[0] ^ q[1] ^ q[3] ^ q[4] ^ q[10] ^ q[11] ^ q[13] ^ q[15] ^ q[18] ^
	    q[19] ^ q[20] ^ q[22] ^ q[23] ^ q[26] ^ q[30] ^ q[31] ^ in[0] ^
	    in[1] ^ in[6] ^ in[9] ^ in[13] ^ in[15] ^ in[16] ^ in[17] ^ in[19] ^
	    in[20] ^ in[26] ^ in[27] ^ in[29] ^ in[31] ^ in[34] ^ in[35] ^
	    in[36] ^ in[38] ^ in[39] ^ in[42] ^ in[46] ^ in[47];
	c[24] =
	    q[0] ^ q[1] ^ q[2] ^ q[4] ^ q[5] ^ q[11] ^ q[12] ^ q[14] ^ q[16] ^
	    q[19] ^ q[20] ^ q[21] ^ q[23] ^ q[24] ^ q[27] ^ q[31] ^ in[1] ^
	    in[2] ^ in[7] ^ in[10] ^ in[14] ^ in[16] ^ in[17] ^ in[18] ^ in[20]
	    ^ in[21] ^ in[27] ^ in[28] ^ in[30] ^ in[32] ^ in[35] ^ in[36] ^
	    in[37] ^ in[39] ^ in[40] ^ in[43] ^ in[47];
	c[25] =
	    q[1] ^ q[2] ^ q[3] ^ q[5] ^ q[6] ^ q[12] ^ q[13] ^ q[15] ^ q[17] ^
	    q[20] ^ q[21] ^ q[22] ^ q[24] ^ q[25] ^ q[28] ^ in[2] ^ in[3] ^
	    in[8] ^ in[11] ^ in[15] ^ in[17] ^ in[18] ^ in[19] ^ in[21] ^ in[22]
	    ^ in[28] ^ in[29] ^ in[31] ^ in[33] ^ in[36] ^ in[37] ^ in[38] ^
	    in[40] ^ in[41] ^ in[44];
	c[26] =
	    q[2] ^ q[3] ^ q[4] ^ q[6] ^ q[7] ^ q[8] ^ q[9] ^ q[10] ^ q[12] ^
	    q[15] ^ q[22] ^ q[23] ^ q[25] ^ q[26] ^ q[28] ^ q[31] ^ in[0] ^
	    in[3] ^ in[4] ^ in[6] ^ in[10] ^ in[18] ^ in[19] ^ in[20] ^ in[22] ^
	    in[23] ^ in[24] ^ in[25] ^ in[26] ^ in[28] ^ in[31] ^ in[38] ^
	    in[39] ^ in[41] ^ in[42] ^ in[44] ^ in[47];
	c[27] =
	    q[3] ^ q[4] ^ q[5] ^ q[7] ^ q[8] ^ q[9] ^ q[10] ^ q[11] ^ q[13] ^
	    q[16] ^ q[23] ^ q[24] ^ q[26] ^ q[27] ^ q[29] ^ in[1] ^ in[4] ^
	    in[5] ^ in[7] ^ in[11] ^ in[19] ^ in[20] ^ in[21] ^ in[23] ^ in[24]
	    ^ in[25] ^ in[26] ^ in[27] ^ in[29] ^ in[32] ^ in[39] ^ in[40] ^
	    in[42] ^ in[43] ^ in[45];
	c[28] =
	    q[4] ^ q[5] ^ q[6] ^ q[8] ^ q[9] ^ q[10] ^ q[11] ^ q[12] ^ q[14] ^
	    q[17] ^ q[24] ^ q[25] ^ q[27] ^ q[28] ^ q[30] ^ in[2] ^ in[5] ^
	    in[6] ^ in[8] ^ in[12] ^ in[20] ^ in[21] ^ in[22] ^ in[24] ^ in[25]
	    ^ in[26] ^ in[27] ^ in[28] ^ in[30] ^ in[33] ^ in[40] ^ in[41] ^
	    in[43] ^ in[44] ^ in[46];
	c[29] =
	    q[5] ^ q[6] ^ q[7] ^ q[9] ^ q[10] ^ q[11] ^ q[12] ^ q[13] ^ q[15] ^
	    q[18] ^ q[25] ^ q[26] ^ q[28] ^ q[29] ^ q[31] ^ in[3] ^ in[6] ^
	    in[7] ^ in[9] ^ in[13] ^ in[21] ^ in[22] ^ in[23] ^ in[25] ^ in[26]
	    ^ in[27] ^ in[28] ^ in[29] ^ in[31] ^ in[34] ^ in[41] ^ in[42] ^
	    in[44] ^ in[45] ^ in[47];
	c[30] =
	    q[6] ^ q[7] ^ q[8] ^ q[10] ^ q[11] ^ q[12] ^ q[13] ^ q[14] ^ q[16] ^
	    q[19] ^ q[26] ^ q[27] ^ q[29] ^ q[30] ^ in[4] ^ in[7] ^ in[8] ^
	    in[10] ^ in[14] ^ in[22] ^ in[23] ^ in[24] ^ in[26] ^ in[27] ^
	    in[28] ^ in[29] ^ in[30] ^ in[32] ^ in[35] ^ in[42] ^ in[43] ^
	    in[45] ^ in[46];
	c[31] =
	    q[7] ^ q[8] ^ q[9] ^ q[11] ^ q[12] ^ q[13] ^ q[14] ^ q[15] ^ q[17] ^
	    q[20] ^ q[27] ^ q[28] ^ q[30] ^ q[31] ^ in[5] ^ in[8] ^ in[9] ^
	    in[11] ^ in[15] ^ in[23] ^ in[24] ^ in[25] ^ in[27] ^ in[28] ^
	    in[29] ^ in[30] ^ in[31] ^ in[33] ^ in[36] ^ in[43] ^ in[44] ^
	    in[46] ^ in[47];

	for (i = 0; i < 32; i++)
		crc |= c[i] << i;

	return crc;
}

static u32 yk3_esw_cuckoo_k3_mac_hash(const u8 *key, u32 seed, u32 mux_seed)
{
	const u32 out_width = 11;	/* 11bits */
	u64 crc_total;
	u64 crc_high;
	u64 crc_low;
	u64 crc_out;
	u64 crc32;
	u32 pos;

	crc32 = yk3_esw_cuckoo_mac_crc32_48bit(key, seed);

	crc_total = (crc32 << 32) >> mux_seed;
	crc_high = (crc_total & 0xFFFFFFFF00000000) >> 32;
	crc_low = crc_total & 0xFFFFFFFF;

	crc_out = crc_high | crc_low;
	pos = (int)(crc_out % (1 << out_width));

	return pos;
}

static void yk3_esw_cuckoo_mac_parse_rule_data(const u32 *data,
					       u8 *key, u8 *value)
{
	u8 *p = (u8 *)data;

	/* [47:0] MAC ADDR */
	memcpy(key, &p[0], YK3_ESW_CUCKOO_MAC_KEY_SIZE);

	/* [58:48] MAC VALUE */
	value[0] = p[6];
	value[1] = p[7] & 0x7;

	/* [59] enable flag */
	value[1] |= p[7] & 0x8;
}

static int yk3_esw_cuckoo_mac_store_rule_data(struct yk3_esw_cuckoo_table_uncached *table,
					      u8 bucket, u32 pos)
{
	u32 data[YK3_ESW_CUCKOO_MAX_DATA_ROUND] = { 0 };
	u32 index;
	uintptr_t entry_addr;
	struct yk3_esw_cuckoo_table *base_info = &table->table_base;
	int i, ret;

	table->ops->generate_rule_data(table->entry_swap.key,
				       table->entry_swap.value, data);

	if (bucket == YK3_ESW_CUCKOO_MAC_BUCKET_NUM) {
		entry_addr = (uintptr_t)(base_info->hw.hw_addr +
					 base_info->hw.waddr + 0x18000 + 0x8 * pos);
	} else {
		index = table->ops->get_ram_addr(bucket, pos);
		entry_addr = (uintptr_t)(base_info->hw.hw_addr +
					 base_info->hw.waddr + base_info->value_size * index);
	}
	for (i = 0; i < base_info->hw.data_round; i++) {
		ret = yk3_esw_cuckoo_iowrite32_direct(entry_addr + 4 * i, data[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static void yk3_esw_cuckoo_mac_generate_rule_data(const u8 *key, const u8 *value,
						  u32 *data)
{
	memcpy(data, value, YK3_ESW_CUCKOO_MAC_VALUE_SIZE);
}

static u32 yk3_esw_cuckoo_mac_get_ram_addr(u32 bucket, u32 pos)
{
	return (((bucket & 0x3) << 12) | (pos & 0xFFF));
}

static void yk3_esw_cuckoo_mac_get_hw_info(struct yk3_esw_cuckoo_table_uncached *table)
{
	struct yk3_esw_cuckoo_table *base_info = &table->table_base;
	int i;

	base_info->bucket_count = YK3_ESW_CUCKOO_MAC_BUCKET_NUM;
	base_info->depth = YK3_ESW_CUCKOO_K3_MAC_DEPTH;
	base_info->key_size = YK3_ESW_CUCKOO_MAC_KEY_SIZE;
	base_info->value_size = YK3_ESW_CUCKOO_MAC_VALUE_SIZE;
	base_info->seed_bits = YK3_ESW_CUCKOO_MAC_SEED_BITS;
	base_info->mux_seed_bits = YK3_ESW_CUCKOO_MAC_MUX_SEED_BITS;
	for (i = 0; i < base_info->bucket_count; i++) {
		base_info->hw.seed_addr[i] = YK3_ESW_CUCKOO_MAC_SEED_ADDR(i);
		base_info->hw.mux_seed_addr[i] = YK3_ESW_CUCKOO_MAC_MUX_SEED_ADDR(i);
	}
	base_info->hw.waddr = YK3_ESW_CUCKOO_MAC_WADDR;
	base_info->hw.raddr = YK3_ESW_CUCKOO_MAC_RADDR;
	base_info->hw.data_addr = 0;
	base_info->hw.data_round = YK3_ESW_CUCKOO_MAC_DATA_ROUND;
}

static const struct yk3_esw_cuckoo_ops_uncached k3_mac_ops = {
	.get_hw_info = yk3_esw_cuckoo_mac_get_hw_info,
	.hash = yk3_esw_cuckoo_k3_mac_hash,
	.get_ram_addr = yk3_esw_cuckoo_mac_get_ram_addr,
	.generate_rule_data = yk3_esw_cuckoo_mac_generate_rule_data,
	.parse_rule_data = yk3_esw_cuckoo_mac_parse_rule_data,
	.store_rule_data = yk3_esw_cuckoo_mac_store_rule_data,
};

static int yk3_esw_cuckoo_set_table_default_params(struct yk3_esw_cuckoo_table *table)
{
	struct yk3_esw_cuckoo_table_uncached *uncached_table = NULL;

	switch (table->type) {
	case YK3_CUCKOO_TYPE_K3_LAN_MAC:
		uncached_table = (struct yk3_esw_cuckoo_table_uncached *)table;
		uncached_table->ops = &k3_mac_ops;
		if (uncached_table->ops->get_hw_info)
			uncached_table->ops->get_hw_info(uncached_table);
		break;
	default:
		yk3_err("Invalid cuckoo hash type");
		return -EINVAL;
	}

	return 0;
}

static int yk3_esw_cuckoo_generate_seed(struct yk3_esw_cuckoo_table *table)
{
	u32 mux_seed[YK3_ESW_CUCKOO_MAX_BUCKETS];
	u32 seed[YK3_ESW_CUCKOO_MAX_BUCKETS];
	int i;

	if (table->type == YK3_CUCKOO_TYPE_K3_LAN_MAC) {
		/* MurmurHash constant */
		seed[0] = 0xcc9e2d51;
		/* Pi first 32bits */
		seed[1] = 0x243f6a88;
		/* MurmurHash constant */
		seed[2] = 0x1b873593;
		mux_seed[0] = 0x1a;
		mux_seed[1] = 0x12;
		mux_seed[2] = 0x1c;
	}

	for (i = 0; i < table->bucket_count; i++) {
		yk3_esw_cuckoo_iowrite32(table, table->hw.seed_addr[i],
					 seed[i]);
		yk3_esw_cuckoo_iowrite32(table, table->hw.mux_seed_addr[i],
					 mux_seed[i]);
		yk3_debug("bucket %d seed 0x%x mux_seed 0x%x", i,
			  seed[i], mux_seed[i]);
	}
	memcpy(table->seed, seed, sizeof(u32) * table->bucket_count);
	memcpy(table->mux_seed, mux_seed, sizeof(u32) * table->bucket_count);

	return 0;
}

struct yk3_esw_cuckoo_table *yk3_esw_cuckoo_create(u32 type, u32 flag, u32 pf_id,
						   void __iomem *hw_addr)
{
	struct yk3_esw_cuckoo_table *table;
	size_t table_size = 0;
	int ret;

	if (type != YK3_CUCKOO_TYPE_K3_LAN_MAC) {
		yk3_err("Invalid cuckoo hash type %d", type);
		return NULL;
	}

	table_size = sizeof(struct yk3_esw_cuckoo_table_uncached);
	table = kzalloc(table_size, GFP_KERNEL);
	if (!table) {
		yk3_err("Alloc memory for cuckoo hash table failed!");
		return NULL;
	}

	table->type = type;
	table->flag = flag;
	table->hw.pf_id = pf_id;
	if (yk3_esw_cuckoo_set_table_default_params((struct yk3_esw_cuckoo_table *)table) != 0) {
		kfree(table);
		return NULL;
	}
	table->hw.hw_addr = hw_addr;

	if (yk3_esw_cuckoo_generate_seed((struct yk3_esw_cuckoo_table *)table) != 0) {
		kfree(table);
		return NULL;
	}

	if (table->yk3_esw_cuckoo_table_init) {
		ret = table->yk3_esw_cuckoo_table_init(table);
		if (ret != 0) {
			kfree(table);
			return NULL;
		}
	}

	return table;
}

void yk3_esw_cuckoo_destroy(struct yk3_esw_cuckoo_table *table)
{
	int ret;

	if (table->yk3_esw_cuckoo_table_uninit) {
		ret = table->yk3_esw_cuckoo_table_uninit(table);
		if (ret != 0)
			yk3_err("Cuckoo hash type %d uninit failed!", table->type);
	}

	kfree(table);
}

static void yk3_esw_cuckoo_kick_push(struct yk3_esw_cuckoo_kick_stream *stream,
				     struct yk3_esw_cuckoo_kick kick)
{
	if (stream->count < YK3_ESW_CUCKOO_MAX_INSERT_RETRIES) {
		yk3_debug("push kick entry key: %pM", kick.entry.key);
		yk3_debug("form pos %d, bucket %d, to pos %d, bucket %d",
			  kick.from_pos, kick.from_bucket, kick.to_pos, kick.to_bucket);
		stream->kicks[stream->count] = kick;
		stream->count++;
	}
}

static void yk3_esw_cuckoo_kick_pop(struct yk3_esw_cuckoo_kick_stream *stream,
				    struct yk3_esw_cuckoo_kick *kick)
{
	if (stream->count > 0) {
		*kick = stream->kicks[--stream->count];
		yk3_debug("kick pop entry key: %pM", kick->entry.key);
		yk3_debug("kick pop entry form pos %d, bucket %d, to pos %d, bucket %d",
			  kick->from_pos, kick->from_bucket, kick->to_pos, kick->to_bucket);
		yk3_debug("kick pop stream info count %d", stream->count);
	}
}

static int yk3_esw_cuckoo_check_rule_empty_uncached(struct yk3_esw_cuckoo_table_uncached *table,
						    u8 bucket, u32 pos)
{
	u32 data[YK3_ESW_CUCKOO_MAX_DATA_ROUND] = {0};
	u8 key_empty[YK3_ESW_CUCKOO_MAX_KEY_SIZE] = {0};
	uintptr_t entry_addr;
	struct yk3_esw_cuckoo_table *base_info = &table->table_base;
	u32 index;
	int i;

	index = table->ops->get_ram_addr(bucket, pos);
	entry_addr = (uintptr_t)(base_info->hw.hw_addr + base_info->hw.raddr +
				 (index * base_info->value_size));
	for (i = 0; i < base_info->hw.data_round; i++)
		data[i] = yk3_esw_cuckooo_ioread32_direct(entry_addr + (i * 4));

	table->ops->parse_rule_data(data, table->entry_swap.key, table->entry_swap.value);

	if (!memcmp(key_empty, table->entry_swap.key, base_info->key_size)) {
		memset(table->entry_swap.value, 0, YK3_ESW_CUCKOO_MAX_VALUE_SIZE);
		return 0;
	}

	return -EBUSY;
}

static int yk3_esw_cuckoo_load_hw_entry_uncached(struct yk3_esw_cuckoo_table_uncached *table,
						 u8 bucket, u32 pos, u8 *key, u32 *data)
{
	u8 value[YK3_ESW_CUCKOO_MAX_VALUE_SIZE] = {0};
	u32 index;
	uintptr_t entry_addr;
	struct yk3_esw_cuckoo_table *base_info = &table->table_base;
	int i;

	if (!table || !key) {
		yk3_err("Input parameter table: %p, key %p, pf %d", table, key,
			table ? table->table_base.hw.pf_id : -1);
		return -EINVAL;
	}

	index = table->ops->get_ram_addr(bucket, pos);
	entry_addr = (uintptr_t)(base_info->hw.hw_addr +
			base_info->hw.raddr + (index * base_info->value_size));
	for (i = 0; i < base_info->hw.data_round; i++) {
		data[i] = yk3_esw_cuckooo_ioread32_direct(entry_addr + (i * sizeof(u32)));
		yk3_debug("read rule into var: addr %08lx, data round %d, data 0x%08x",
			  entry_addr, i, data[i]);
	}
	table->ops->parse_rule_data(data, key, value);

	return 0;
}

static int yk3_esw_cuckoo_update_kicks(struct yk3_esw_cuckoo_table_uncached *table,
				       struct yk3_esw_cuckoo_kick_stream *stream)
{
	struct yk3_esw_cuckoo_kick kick;
	struct yk3_esw_cuckoo_table *base_info;
	u32 index;
	u32 entry_addr;
	int i, j;

	yk3_debug("%s start kick entry %d", __func__, stream->count);

	base_info = &table->table_base;
	for (i = stream->count - 1; i >= 0; i--) {
		kick = stream->kicks[i];
		yk3_debug("write kick entry key: %pM", kick.entry.key);
		yk3_debug("form pos %d, bucket %d, to pos %d, bucket %d",
			  kick.from_pos, kick.from_bucket, kick.to_pos, kick.to_bucket);
		index = table->ops->get_ram_addr(kick.to_bucket, kick.to_pos);
		entry_addr = base_info->hw.waddr + base_info->value_size * index;
		for (j = 0; j < base_info->hw.data_round; j++)
			if (yk3_esw_cuckoo_iowrite32(base_info, entry_addr + 4 * j,
						     *(((u32 *)kick.entry.value) + j)) != 0)
				return -EFAULT;
	}
	base_info->buckets_entry_num[stream->kicks[stream->count - 1].to_bucket]++;

	return 0;
}

static int yk3_esw_cuckoo_kick_uncached(struct yk3_esw_cuckoo_table_uncached *table,
					struct yk3_esw_cuckoo_entry *entry, u32 to_bucket,
					struct yk3_esw_cuckoo_kick_stream *stream)
{
	struct yk3_esw_cuckoo_table *base_info;
	struct yk3_esw_cuckoo_entry kicked_entry;
	u8 key_empty[YK3_ESW_CUCKOO_MAX_KEY_SIZE] = {0};
	struct yk3_esw_cuckoo_kick kick;
	u32 from_bucket;
	u32 from_pos;
	u32 to_pos;
	int pos_occupied = 0;
	int ret = 0;
	int i;

	if (stream->count >= YK3_ESW_CUCKOO_MAX_INSERT_RETRIES) {
		yk3_err("stream %p over max retries!!!", stream);
		return -EINVAL;
	}
	yk3_debug("stream %p count %d", stream, stream->count);

	base_info = &table->table_base;
	to_pos = table->ops->hash(entry->key, base_info->seed[to_bucket],
				  base_info->mux_seed[to_bucket]);
	/* Avoid reverse movement issues */
	for (i = 0; i < stream->count; i++) {
		if ((to_bucket == stream->kicks[i].from_bucket &&
		     to_pos == stream->kicks[i].from_pos) ||
		    (to_bucket == stream->kicks[i].to_bucket &&
		     to_pos == stream->kicks[i].to_pos)) {
			yk3_debug("stream %p to bucket %d pos %d conflict with bucket %d pos %d",
				  stream, to_bucket, to_pos, stream->kicks[i].to_bucket,
				  stream->kicks[i].to_pos);
			return -EINVAL;
		}
	}

	if (stream->count == 0) {
		from_bucket = YK3_ESW_CUCKOO_NEW_RULE;
		from_pos = 0;
	} else {
		from_bucket = stream->kicks[stream->count - 1].to_bucket;
		from_pos = stream->kicks[stream->count - 1].to_pos;
	}

	kick.entry = *entry;
	kick.from_bucket = from_bucket;
	kick.from_pos = from_pos;
	kick.to_bucket = to_bucket;
	kick.to_pos = to_pos;
	yk3_esw_cuckoo_kick_push(stream, kick);

	yk3_esw_cuckoo_load_hw_entry_uncached(table, to_bucket, to_pos,
					      kicked_entry.key, (u32 *)kicked_entry.value);
	pos_occupied = memcmp(key_empty, kicked_entry.key, base_info->key_size);
	if (pos_occupied != 0) {
		/* Need more kick */
		i = ++to_bucket == base_info->bucket_count ? 0 : to_bucket;
		ret = yk3_esw_cuckoo_kick_uncached(table, &kicked_entry, i, stream);
		if (ret != 0) {
			/* If failed exit recursive process */
			yk3_esw_cuckoo_kick_pop(stream, &kick);
			return ret;
		}
	}

	return 0;
}

static int yk3_esw_cuckoo_insert_uncached(struct yk3_esw_cuckoo_table_uncached *table,
					  const u8 *key, const u32 *value)
{
	u32 to_pos = 0;
	u32 index;
	u32 entry_addr;
	struct yk3_esw_cuckoo_entry *entry;
	struct yk3_esw_cuckoo_table *base_info;
	struct yk3_esw_cuckoo_kick_stream *stream;
	int ret = 0;
	int i;
	u32 bucket_min = 0;
	u32 bucket_min_num = 0;
	bool need_kick = false;

	if (!table || !key || !value) {
		yk3_err("Input parameter error table: %p, key: %p, value: %p, pf %d",
			table, key, value, table ? table->table_base.hw.pf_id : -1);
		return -EINVAL;
	}
	base_info = &table->table_base;

	bucket_min_num = base_info->buckets_entry_num[0];
	bucket_min = 0;
	for (i = 0; i < base_info->bucket_count; i++) {
		if (base_info->buckets_entry_num[i] == 0) {
			bucket_min = i;
			break;
		}
		if (base_info->buckets_entry_num[i] < bucket_min_num) {
			bucket_min_num = base_info->buckets_entry_num[i];
			bucket_min = i;
		}
	}

	to_pos = table->ops->hash(key, base_info->seed[bucket_min],
				  base_info->mux_seed[bucket_min]);
	ret = yk3_esw_cuckoo_check_rule_empty_uncached(table, bucket_min, to_pos);
	if (ret != 0)
		need_kick = true;

	if (need_kick) {
		for (i = 0; i < base_info->bucket_count; i++) {
			if (i == bucket_min)
				continue;
			to_pos = table->ops->hash(key, base_info->seed[i], base_info->mux_seed[i]);
			ret = yk3_esw_cuckoo_check_rule_empty_uncached(table, i, to_pos);
			if (ret == 0) {
				need_kick = false;
				bucket_min = i;
				break;
			}
		}
	}

	if (need_kick) {
		stream = kzalloc(sizeof(*stream), GFP_ATOMIC);
		entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
		if (!stream || !entry) {
			kfree(stream);
			kfree(entry);
			yk3_err("yk3 cuckoo uncached table insert alloc memory failed!");
			return -ENOMEM;
		}

		memcpy(entry->key, key, base_info->key_size);
		memcpy(entry->value, (u8 *)value, base_info->value_size);
		for (i = 0; i < base_info->bucket_count; i++) {
			ret = yk3_esw_cuckoo_kick_uncached(table, entry, i, stream);
			if (ret == 0) {
				if (yk3_esw_cuckoo_update_kicks(table, stream)) {
					yk3_err("yk3 cuckoo uncached table kick update failed!");
					ret = -EFAULT;
				}
				break;
			}
			memset(stream, 0, sizeof(*stream));
		}
		kfree(entry);
		kfree(stream);
		if (ret == 0) {
			yk3_debug("yk3 cuckoo uncached table kick insert ok!");
			return ret;
		}
		yk3_err("yk3 cuckoo uncached table insert entry failed!");
		return ret;
	}

	index = table->ops->get_ram_addr(bucket_min, to_pos);
	entry_addr = base_info->hw.waddr + base_info->value_size * index;
	for (i = 0; i < base_info->hw.data_round; i++) {
		if (yk3_esw_cuckoo_iowrite32(base_info, entry_addr + 4 * i, value[i]) != 0)
			return -EFAULT;
	}
	yk3_debug("yk3 cuckoo uncached table insert direct bucket %d pos %d",
		  bucket_min, to_pos);
	base_info->buckets_entry_num[bucket_min]++;

	return 0;
}

static int yk3_esw_cuckoo_update_uncached(struct yk3_esw_cuckoo_table_uncached *table,
					  const u8 *key, const u32 *value)
{
	u32 to_pos = 0;
	u32 index;
	u32 entry_addr;
	u8 hw_key[YK3_ESW_CUCKOO_MAX_KEY_SIZE] = {0};
	u32 hw_value[YK3_ESW_CUCKOO_MAX_VALUE_SIZE / 4] = {0};
	struct yk3_esw_cuckoo_table *base_info;
	int ret;
	int i;

	if (!table || !key || !value) {
		yk3_err("Input parameter error table: %p, key: %p, value: %p, pf %d",
			table, key, value, table ? table->table_base.hw.pf_id : -1);
		return -EINVAL;
	}
	base_info = &table->table_base;

	for (i = 0; i < base_info->bucket_count; i++) {
		to_pos = table->ops->hash(key, base_info->seed[i], base_info->mux_seed[i]);
		ret = yk3_esw_cuckoo_load_hw_entry_uncached(table, i, to_pos, hw_key, hw_value);
		if (ret != 0) {
			yk3_err("%s get bucket %d pos %d entry failed for update!",
				__func__, i, to_pos);
			return -EFAULT;
		}
		if (memcmp(key, hw_key, base_info->key_size) == 0) {
			yk3_debug("%s match bucket %d pos %d entry !", __func__, i, to_pos);
			break;
		}
	}

	if (i == base_info->bucket_count) {
		yk3_err("yk3 cuckoo uncached table update entry failed!");
		return -EAGAIN;
	}

	index = table->ops->get_ram_addr(i, to_pos);
	entry_addr = base_info->hw.waddr + base_info->value_size * index;
	for (i = 0; i < base_info->hw.data_round; i++) {
		if (yk3_esw_cuckoo_iowrite32(base_info, entry_addr + 4 * i,
					     value[i]) != 0)
			return -EFAULT;
	}

	return 0;
}

static int yk3_esw_cuckoo_search_uncached(struct yk3_esw_cuckoo_table_uncached *table,
					  const u8 *key, u32 *value)
{
	u32 to_pos = 0;
	u8 hw_key[YK3_ESW_CUCKOO_MAX_KEY_SIZE] = {0};
	u32 hw_value[YK3_ESW_CUCKOO_MAX_VALUE_SIZE / 4] = {0};
	struct yk3_esw_cuckoo_table *base_info;
	int ret;
	int i;

	if (!table || !key || !value) {
		yk3_err("Input parameter error table: %p, key: %p, value: %p, pf %d",
			table, key, value, table ? table->table_base.hw.pf_id : -1);
		return -EINVAL;
	}
	base_info = &table->table_base;

	for (i = 0; i < base_info->bucket_count; i++) {
		to_pos = table->ops->hash(key, base_info->seed[i], base_info->mux_seed[i]);
		ret = yk3_esw_cuckoo_load_hw_entry_uncached(table, i, to_pos, hw_key, hw_value);
		if (ret != 0) {
			yk3_err("%s get bucket %d pos %d entry failed for update!",
				__func__, i, to_pos);
			return -EFAULT;
		}
		if (memcmp(key, hw_key, base_info->key_size) == 0) {
			yk3_debug("%s match bucket %d pos %d entry !", __func__, i, to_pos);
			break;
		}
	}

	if (i == base_info->bucket_count) {
		yk3_err("yk3 cuckoo uncached table update entry failed!");
		return -EAGAIN;
	}
	memcpy(value, hw_value, base_info->value_size);

	return 0;
}

static int yk3_esw_cuckoo_delete_uncached(struct yk3_esw_cuckoo_table_uncached *table,
					  const u8 *key)
{
	char value_empty[YK3_ESW_CUCKOO_MAX_VALUE_SIZE] = {0};
	u8 hw_key[YK3_ESW_CUCKOO_MAX_KEY_SIZE] = {0};
	u32 hw_value[YK3_ESW_CUCKOO_MAX_VALUE_SIZE / 4] = {0};
	u32 to_pos;
	u32 index;
	u32 entry_addr;
	struct yk3_esw_cuckoo_table *base_info;
	int i;
	int ret;

	if (!table || !key) {
		yk3_err("Input parameter error");
		return -EINVAL;
	}
	base_info = &table->table_base;

	for (i = 0; i < base_info->bucket_count; i++) {
		to_pos = table->ops->hash(key, base_info->seed[i], base_info->mux_seed[i]);
		ret = yk3_esw_cuckoo_load_hw_entry_uncached(table, i, to_pos, hw_key, hw_value);
		if (ret) {
			yk3_err("yk3 cuckoo read uncached table failed!");
			return ret;
		}
		yk3_debug("del bucket %d, pos %d, hw mac: %pM", i, to_pos, hw_key);
		if (!memcmp(hw_key, key, base_info->key_size)) {
			index = table->ops->get_ram_addr(i, to_pos);
			entry_addr = base_info->hw.waddr + base_info->value_size * index;
			for (i = 0; i < base_info->hw.data_round; i++) {
				if (yk3_esw_cuckoo_iowrite32(base_info, entry_addr + 4 * i,
							     value_empty[i]) != 0)
					return -EFAULT;
			}
			base_info->buckets_entry_num[i]--;
			break;
		}
	}
	if (i == base_info->bucket_count)
		yk3_err("yk3 cuckoo uncached table delete entry no exist!");

	return 0;
}

int yk3_esw_cuckoo_insert(struct yk3_esw_cuckoo_table *table, const u8 *key, const u8 *value)
{
	int ret = -1;

	if (!table || !key || !value) {
		yk3_err("Input parameter error");
		return -EINVAL;
	}

	ret = yk3_esw_cuckoo_insert_uncached((struct yk3_esw_cuckoo_table_uncached *)table,
					     key, (u32 *)value);
	if (ret) {
		yk3_err("Failed to insert uncached table");
		return ret;
	}

	return ret;
}

int yk3_esw_cuckoo_delete(struct yk3_esw_cuckoo_table *table, const u8 *key)
{
	if (!table || !key) {
		yk3_err("%s get invalid parameter!", __func__);
		return -EINVAL;
	}

	return yk3_esw_cuckoo_delete_uncached((struct yk3_esw_cuckoo_table_uncached *)table, key);
}

int yk3_esw_cuckoo_change(struct yk3_esw_cuckoo_table *table, const u8 *key, const u8 *value)
{
	if (!table || !key || !value) {
		yk3_err("%s get invalid parameter!", __func__);
		return -EINVAL;
	}

	return yk3_esw_cuckoo_update_uncached((struct yk3_esw_cuckoo_table_uncached *)table,
					      key, (u32 *)value);
}

int yk3_esw_cuckoo_search(struct yk3_esw_cuckoo_table *table, const u8 *key, u8 *value,
			  u32 *bucket, u32 *pos)
{
	if (!table || !key || !value || !bucket || !pos) {
		yk3_err("%s invalid parameter, table:%p, key:%p, value:%p, bucket:%p, pos:%p",
			__func__, table, key, value, bucket, pos);
		return -EINVAL;
	}

	return yk3_esw_cuckoo_search_uncached((struct yk3_esw_cuckoo_table_uncached *)table,
					      key, (u32 *)value);
}
