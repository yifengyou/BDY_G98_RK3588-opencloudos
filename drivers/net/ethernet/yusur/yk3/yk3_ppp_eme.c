// SPDX-License-Identifier: GPL-2.0

#include <linux/random.h>
#include <linux/sort.h>
#include "yk3.h"
#include "yk3_mbox.h"

static struct yk3_ppp_eme_mgr g_eme_mgr = {
	.opt_lock = __SPIN_LOCK_INITIALIZER(g_eme_mgr.opt_lock)
};

static int __yk3_ppp_eme_del_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
				  u32 table_id, u8 *key);

static int __yk3_ppp_eme_create_table(struct yk3_pdev_priv *pdev_priv, int hw_id,
				      u32 table_id, u32 key_size, u32 value_size);

static u32 yk3_ppp_eme_ioread32(struct yk3_ppp_eme *eme, u32 reg)
{
	return ioread32((void __iomem *)((uintptr_t)(eme->hw.bar_addr) + (reg)));
}

static void yk3_ppp_eme_iowrite32(struct yk3_ppp_eme *eme, u32 reg, u32 val)
{
	iowrite32(val, (void __iomem *)((uintptr_t)(eme->hw.bar_addr) + (reg)));
}

static int yk3_ppp_eme_compare(const void *a, const void *b)
{
	return (*(u32 *)a - *(u32 *)b);
}

static int yk3_ppp_eme_check_duplicates(u32 *array, u32 size)
{
	u32 *arr;
	int i;

	arr = kcalloc(size, sizeof(u32), GFP_ATOMIC);
	if (!arr)
		return -ENOMEM;

	memcpy(arr, array, size * sizeof(u32));

	sort(arr, size, sizeof(u32), yk3_ppp_eme_compare, NULL);

	for (i = 1; i < size; i++) {
		if (arr[i] == arr[i - 1]) {
			kfree(arr);
			return -EINVAL;
		}
	}

	kfree(arr);

	return 0;
}

static int yk3_ppp_eme_generate_seed(struct yk3_ppp_eme *eme)
{
	u32 mux_seed[YK3_PPP_EME_BUCKET_NUM];
	u32 seed[YK3_PPP_EME_BUCKET_NUM];
	u32 mux_seed_mask;
	u32 seed_mask;
	int no_duplicate = 0;
	int i, j;
	u32 random_val = 0;

	mux_seed_mask = (1ULL << eme->mux_seed_bits) - 1;
	seed_mask = (1ULL << eme->seed_bits) - 1;

	for (j = 0; j < YK3_PPP_EME_MAX_CREATE_SEED_RETRIES; j++) {
		for (i = 0; i < eme->bucket_count; i++) {
			get_random_bytes(&random_val, sizeof(random_val));
			seed[i] = random_val & seed_mask;
		}

		for (i = 0; i < eme->bucket_count; i++) {
			get_random_bytes(&random_val, sizeof(random_val));
			mux_seed[i] = random_val & mux_seed_mask;
		}

		if (!yk3_ppp_eme_check_duplicates(seed, eme->bucket_count) &&
		    !yk3_ppp_eme_check_duplicates(mux_seed, eme->bucket_count)) {
			no_duplicate = 1;
			break;
		}
	}

	if (!no_duplicate)
		return -EAGAIN;

	for (i = 0; i < eme->bucket_count; i++) {
		yk3_ppp_eme_iowrite32(eme, eme->hw.seed_addr[i], seed[i]);
		yk3_ppp_eme_iowrite32(eme, eme->hw.mux_seed_addr[i], mux_seed[i]);
		yk3_debug("eme-%d: bucket %d seed 0x%x mux_seed 0x%x.\n",
			  eme->hw_id, i, seed[i], mux_seed[i]);
	}

	memcpy(eme->seed, seed, sizeof(u32) * eme->bucket_count);
	memcpy(eme->mux_seed, mux_seed, sizeof(u32) * eme->bucket_count);

	return 0;
}

static void yk3_ppp_eme_get_hw_offset(struct yk3_ppp_eme *eme)
{
	int i;
	u32 ppp_id;
	u32 tmp;

	tmp = yk3_ppp_eme_ioread32(eme, 0x2400020);
	yk3_ppp_eme_iowrite32(eme, 0x2400020, 0x9);

	for (i = 0; i < 1024; i++) {
		ppp_id = yk3_ppp_eme_ioread32(eme, 0x400000 + 4 + i * 0x10);
		yk3_debug("eme-%d: addr %08x value 0x%08x",
			  eme->hw_id, 0x400000 + 4 + i * 0x10, ppp_id);

		if ((ppp_id & 0xFFFFFF00) == 0x00700000) {
			eme->hw_offset[0] = yk3_ppp_eme_ioread32(eme, 0x400000 + 8 + i * 0x10);
		} else if ((ppp_id & 0xFFFFFF00) == 0x00700100) {
			eme->hw_offset[1] = yk3_ppp_eme_ioread32(eme, 0x400000 + 8 + i * 0x10);
		} else if ((ppp_id & 0xFFFFFF00) == 0x00700200) {
			eme->hw_offset[2] = yk3_ppp_eme_ioread32(eme, 0x400000 + 8 + i * 0x10);
		} else if ((ppp_id & 0xFFFFFF00) == 0x00700300) {
			eme->hw_offset[3] = yk3_ppp_eme_ioread32(eme, 0x400000 + 8 + i * 0x10);
			break;
		}
	}

	eme->hw_offset[7] = 0;
	yk3_debug("eme-%d: mac0 offset 0x%08x", eme->hw_id, eme->hw_offset[0]);
	yk3_debug("eme-%d: mac1 offset 0x%08x", eme->hw_id, eme->hw_offset[1]);
	yk3_debug("eme-%d: host offset 0x%08x", eme->hw_id, eme->hw_offset[2]);
	yk3_debug("eme-%d: soc offset 0x%08x", eme->hw_id, eme->hw_offset[3]);
	yk3_debug("eme-%d: test offset 0x%08x", eme->hw_id, eme->hw_offset[7]);
	yk3_ppp_eme_iowrite32(eme, 0x2400020, tmp);
}

static void yk3_ppp_eme_get_hw_info(struct yk3_ppp_eme *eme)
{
	int i;

	eme->bucket_count = YK3_PPP_EME_BUCKET_NUM;
	eme->depth = YK3_PPP_EME_DEPTH;
	eme->seed_bits = YK3_PPP_EME_SEED_BITS;
	eme->mux_seed_bits = YK3_PPP_EME_MUX_SEED_BITS;

	for (i = 0; i < eme->bucket_count; i++) {
		eme->hw.seed_addr[i] = eme->hw_offset[eme->hw_id] +
				       YK3_PPP_EME_SEED_ADDR(i);
		eme->hw.mux_seed_addr[i] = eme->hw_offset[eme->hw_id] +
					   YK3_PPP_EME_MUX_SEED_ADDR(i);
	}

	eme->hw.init_done_addr = eme->hw_offset[eme->hw_id] + YK3_PPP_EME_INIT_DONE;
	eme->hw.waddr = eme->hw_offset[eme->hw_id] + YK3_PPP_EME_WADDR;
	eme->hw.raddr = eme->hw_offset[eme->hw_id] + YK3_PPP_EME_RADDR;
	eme->hw.data_addr = eme->hw_offset[eme->hw_id] + YK3_PPP_EME_DATA_ADDR;
	eme->hw.data_round = YK3_PPP_EME_DATA_ROUND;
}

static noinline u8 yk3_ppp_eme_crc32_0(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[2] ^ q[3] ^ q[4] ^ q[6] ^ q[10] ^ q[13] ^ q[19] ^ q[24] ^ q[28] ^
		q[31] ^ in[0] ^ in[6] ^ in[9] ^ in[10] ^ in[12] ^ in[16] ^ in[24] ^ in[25] ^
		in[26] ^ in[28] ^ in[29] ^ in[30] ^ in[31] ^ in[32] ^ in[34] ^ in[37] ^
		in[44] ^ in[45] ^ in[47] ^ in[48] ^ in[50] ^ in[53] ^ in[54] ^ in[55] ^
		in[58] ^ in[60] ^ in[61] ^ in[63] ^ in[65] ^ in[66] ^ in[67] ^ in[68] ^
		in[72] ^ in[73] ^ in[79] ^ in[81] ^ in[82] ^ in[83] ^ in[84] ^ in[85] ^
		in[87] ^ in[94] ^ in[95] ^ in[96] ^ in[97] ^ in[98] ^ in[99] ^ in[101] ^
		in[103] ^ in[104] ^ in[106] ^ in[110] ^ in[111] ^ in[113] ^ in[114] ^
		in[116] ^ in[117] ^ in[118] ^ in[119] ^ in[123] ^ in[125] ^ in[126] ^
		in[127] ^ in[128] ^ in[132] ^ in[134] ^ in[135] ^ in[136] ^ in[137] ^
		in[143] ^ in[144] ^ in[149] ^ in[151] ^ in[155] ^ in[156] ^ in[158] ^
		in[161] ^ in[162] ^ in[166] ^ in[167] ^ in[169] ^ in[170] ^ in[171] ^
		in[172] ^ in[182] ^ in[183] ^ in[186] ^ in[188] ^ in[190] ^ in[191] ^
		in[192] ^ in[193] ^ in[194] ^ in[197] ^ in[198] ^ in[199] ^ in[201] ^
		in[202] ^ in[203] ^ in[207] ^ in[208] ^ in[209] ^ in[210] ^ in[212] ^
		in[214] ^ in[216] ^ in[224] ^ in[226] ^ in[227] ^ in[228] ^ in[230] ^
		in[234] ^ in[237] ^ in[243] ^ in[248] ^ in[252] ^ in[255];
	return val;
}

static noinline u8 yk3_ppp_eme_crc32_1(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[5] ^ q[6] ^ q[7] ^ q[10] ^ q[11] ^ q[13] ^ q[14] ^
		q[19] ^ q[20] ^ q[24] ^ q[25] ^ q[28] ^ q[29] ^ q[31] ^ in[0] ^ in[1] ^
		in[6] ^ in[7] ^ in[9] ^ in[11] ^ in[12] ^ in[13] ^ in[16] ^ in[17] ^
		in[24] ^ in[27] ^ in[28] ^ in[33] ^ in[34] ^ in[35] ^ in[37] ^ in[38] ^
		in[44] ^ in[46] ^ in[47] ^ in[49] ^ in[50] ^ in[51] ^ in[53] ^ in[56] ^
		in[58] ^ in[59] ^ in[60] ^ in[62] ^ in[63] ^ in[64] ^ in[65] ^ in[69] ^
		in[72] ^ in[74] ^ in[79] ^ in[80] ^ in[81] ^ in[86] ^ in[87] ^ in[88] ^
		in[94] ^ in[100] ^ in[101] ^ in[102] ^ in[103] ^ in[105] ^ in[106] ^
		in[107] ^ in[110] ^ in[112] ^ in[113] ^ in[115] ^ in[116] ^ in[120] ^
		in[123] ^ in[124] ^ in[125] ^ in[129] ^ in[132] ^ in[133] ^ in[134] ^
		in[138] ^ in[143] ^ in[145] ^ in[149] ^ in[150] ^ in[151] ^ in[152] ^
		in[155] ^ in[157] ^ in[158] ^ in[159] ^ in[161] ^ in[163] ^ in[166] ^
		in[168] ^ in[169] ^ in[173] ^ in[182] ^ in[184] ^ in[186] ^ in[187] ^
		in[188] ^ in[189] ^ in[190] ^ in[195] ^ in[197] ^ in[200] ^ in[201] ^
		in[204] ^ in[207] ^ in[211] ^ in[212] ^ in[213] ^ in[214] ^ in[215] ^
		in[216] ^ in[217] ^ in[224] ^ in[225] ^ in[226] ^ in[229] ^ in[230] ^
		in[231] ^ in[234] ^ in[235] ^ in[237] ^ in[238] ^ in[243] ^ in[244] ^
		in[248] ^ in[249] ^ in[252] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_2(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[4] ^ q[7] ^ q[8] ^ q[10] ^ q[11] ^ q[12] ^ q[13] ^ q[14] ^
		q[15] ^ q[19] ^ q[20] ^ q[21] ^ q[24] ^ q[25] ^ q[26] ^ q[28] ^ q[29] ^
		q[30] ^ q[31] ^ in[0] ^ in[1] ^ in[2] ^ in[6] ^ in[7] ^ in[8] ^ in[9] ^
		in[13] ^ in[14] ^ in[16] ^ in[17] ^ in[18] ^ in[24] ^ in[26] ^ in[30] ^
		in[31] ^ in[32] ^ in[35] ^ in[36] ^ in[37] ^ in[38] ^ in[39] ^ in[44] ^
		in[51] ^ in[52] ^ in[53] ^ in[55] ^ in[57] ^ in[58] ^ in[59] ^ in[64] ^
		in[67] ^ in[68] ^ in[70] ^ in[72] ^ in[75] ^ in[79] ^ in[80] ^ in[83] ^
		in[84] ^ in[85] ^ in[88] ^ in[89] ^ in[94] ^ in[96] ^ in[97] ^ in[98] ^
		in[99] ^ in[102] ^ in[107] ^ in[108] ^ in[110] ^ in[118] ^ in[119] ^
		in[121] ^ in[123] ^ in[124] ^ in[127] ^ in[128] ^ in[130] ^ in[132] ^
		in[133] ^ in[136] ^ in[137] ^ in[139] ^ in[143] ^ in[146] ^ in[149] ^
		in[150] ^ in[152] ^ in[153] ^ in[155] ^ in[159] ^ in[160] ^ in[161] ^
		in[164] ^ in[166] ^ in[171] ^ in[172] ^ in[174] ^ in[182] ^ in[185] ^
		in[186] ^ in[187] ^ in[189] ^ in[192] ^ in[193] ^ in[194] ^ in[196] ^
		in[197] ^ in[199] ^ in[203] ^ in[205] ^ in[207] ^ in[209] ^ in[210] ^
		in[213] ^ in[215] ^ in[217] ^ in[218] ^ in[224] ^ in[225] ^ in[228] ^
		in[231] ^ in[232] ^ in[234] ^ in[235] ^ in[236] ^ in[237] ^ in[238] ^
		in[239] ^ in[243] ^ in[244] ^ in[245] ^ in[248] ^ in[249] ^ in[250] ^
		in[252] ^ in[253] ^ in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_3(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[2] ^ q[5] ^ q[8] ^ q[9] ^ q[11] ^ q[12] ^ q[13] ^ q[14] ^ q[15] ^
		q[16] ^ q[20] ^ q[21] ^ q[22] ^ q[25] ^ q[26] ^ q[27] ^ q[29] ^ q[30] ^
		q[31] ^ in[1] ^ in[2] ^ in[3] ^ in[7] ^ in[8] ^ in[9] ^ in[10] ^ in[14] ^
		in[15] ^ in[17] ^ in[18] ^ in[19] ^ in[25] ^ in[27] ^ in[31] ^ in[32] ^
		in[33] ^ in[36] ^ in[37] ^ in[38] ^ in[39] ^ in[40] ^ in[45] ^ in[52] ^
		in[53] ^ in[54] ^ in[56] ^ in[58] ^ in[59] ^ in[60] ^ in[65] ^ in[68] ^
		in[69] ^ in[71] ^ in[73] ^ in[76] ^ in[80] ^ in[81] ^ in[84] ^ in[85] ^
		in[86] ^ in[89] ^ in[90] ^ in[95] ^ in[97] ^ in[98] ^ in[99] ^ in[100] ^
		in[103] ^ in[108] ^ in[109] ^ in[111] ^ in[119] ^ in[120] ^ in[122] ^
		in[124] ^ in[125] ^ in[128] ^ in[129] ^ in[131] ^ in[133] ^ in[134] ^
		in[137] ^ in[138] ^ in[140] ^ in[144] ^ in[147] ^ in[150] ^ in[151] ^
		in[153] ^ in[154] ^ in[156] ^ in[160] ^ in[161] ^ in[162] ^ in[165] ^
		in[167] ^ in[172] ^ in[173] ^ in[175] ^ in[183] ^ in[186] ^ in[187] ^
		in[188] ^ in[190] ^ in[193] ^ in[194] ^ in[195] ^ in[197] ^ in[198] ^
		in[200] ^ in[204] ^ in[206] ^ in[208] ^ in[210] ^ in[211] ^ in[214] ^
		in[216] ^ in[218] ^ in[219] ^ in[225] ^ in[226] ^ in[229] ^ in[232] ^
		in[233] ^ in[235] ^ in[236] ^ in[237] ^ in[238] ^ in[239] ^ in[240] ^
		in[244] ^ in[245] ^ in[246] ^ in[249] ^ in[250] ^ in[251] ^ in[253] ^
		in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_4(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[4] ^ q[9] ^ q[12] ^ q[14] ^ q[15] ^ q[16] ^ q[17] ^ q[19] ^ q[21] ^
		q[22] ^ q[23] ^ q[24] ^ q[26] ^ q[27] ^ q[30] ^ in[0] ^ in[2] ^ in[3] ^
		in[4] ^ in[6] ^ in[8] ^ in[11] ^ in[12] ^ in[15] ^ in[18] ^ in[19] ^
		in[20] ^ in[24] ^ in[25] ^ in[29] ^ in[30] ^ in[31] ^ in[33] ^ in[38] ^
		in[39] ^ in[40] ^ in[41] ^ in[44] ^ in[45] ^ in[46] ^ in[47] ^ in[48] ^
		in[50] ^ in[57] ^ in[58] ^ in[59] ^ in[63] ^ in[65] ^ in[67] ^ in[68] ^
		in[69] ^ in[70] ^ in[73] ^ in[74] ^ in[77] ^ in[79] ^ in[83] ^ in[84] ^
		in[86] ^ in[90] ^ in[91] ^ in[94] ^ in[95] ^ in[97] ^ in[100] ^ in[103] ^
		in[106] ^ in[109] ^ in[111] ^ in[112] ^ in[113] ^ in[114] ^ in[116] ^
		in[117] ^ in[118] ^ in[119] ^ in[120] ^ in[121] ^ in[127] ^ in[128] ^
		in[129] ^ in[130] ^ in[136] ^ in[137] ^ in[138] ^ in[139] ^ in[141] ^
		in[143] ^ in[144] ^ in[145] ^ in[148] ^ in[149] ^ in[152] ^ in[154] ^
		in[156] ^ in[157] ^ in[158] ^ in[163] ^ in[167] ^ in[168] ^ in[169] ^
		in[170] ^ in[171] ^ in[172] ^ in[173] ^ in[174] ^ in[176] ^ in[182] ^
		in[183] ^ in[184] ^ in[186] ^ in[187] ^ in[189] ^ in[190] ^ in[192] ^
		in[193] ^ in[195] ^ in[196] ^ in[197] ^ in[202] ^ in[203] ^ in[205] ^
		in[208] ^ in[210] ^ in[211] ^ in[214] ^ in[215] ^ in[216] ^ in[217] ^
		in[219] ^ in[220] ^ in[224] ^ in[228] ^ in[233] ^ in[236] ^ in[238] ^
		in[239] ^ in[240] ^ in[241] ^ in[243] ^ in[245] ^ in[246] ^ in[247] ^
		in[248] ^ in[250] ^ in[251] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_5(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[3] ^ q[4] ^ q[5] ^ q[6] ^ q[15] ^ q[16] ^ q[17] ^
		q[18] ^ q[19] ^ q[20] ^ q[22] ^ q[23] ^ q[25] ^ q[27] ^ in[0] ^ in[1] ^
		in[3] ^ in[4] ^ in[5] ^ in[6] ^ in[7] ^ in[10] ^ in[13] ^ in[19] ^ in[20] ^
		in[21] ^ in[24] ^ in[28] ^ in[29] ^ in[37] ^ in[39] ^ in[40] ^ in[41] ^
		in[42] ^ in[44] ^ in[46] ^ in[49] ^ in[50] ^ in[51] ^ in[53] ^ in[54] ^
		in[55] ^ in[59] ^ in[61] ^ in[63] ^ in[64] ^ in[65] ^ in[67] ^ in[69] ^
		in[70] ^ in[71] ^ in[72] ^ in[73] ^ in[74] ^ in[75] ^ in[78] ^ in[79] ^
		in[80] ^ in[81] ^ in[82] ^ in[83] ^ in[91] ^ in[92] ^ in[94] ^ in[97] ^
		in[99] ^ in[103] ^ in[106] ^ in[107] ^ in[111] ^ in[112] ^ in[115] ^
		in[116] ^ in[120] ^ in[121] ^ in[122] ^ in[123] ^ in[125] ^ in[126] ^
		in[127] ^ in[129] ^ in[130] ^ in[131] ^ in[132] ^ in[134] ^ in[135] ^
		in[136] ^ in[138] ^ in[139] ^ in[140] ^ in[142] ^ in[143] ^ in[145] ^
		in[146] ^ in[150] ^ in[151] ^ in[153] ^ in[156] ^ in[157] ^ in[159] ^
		in[161] ^ in[162] ^ in[164] ^ in[166] ^ in[167] ^ in[168] ^ in[173] ^
		in[174] ^ in[175] ^ in[177] ^ in[182] ^ in[184] ^ in[185] ^ in[186] ^
		in[187] ^ in[192] ^ in[196] ^ in[199] ^ in[201] ^ in[202] ^ in[204] ^
		in[206] ^ in[207] ^ in[208] ^ in[210] ^ in[211] ^ in[214] ^ in[215] ^
		in[217] ^ in[218] ^ in[220] ^ in[221] ^ in[224] ^ in[225] ^ in[226] ^
		in[227] ^ in[228] ^ in[229] ^ in[230] ^ in[239] ^ in[240] ^ in[241] ^
		in[242] ^ in[243] ^ in[244] ^ in[246] ^ in[247] ^ in[249] ^ in[251];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_6(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[2] ^ q[3] ^ q[4] ^ q[5] ^ q[6] ^ q[7] ^ q[16] ^ q[17] ^ q[18] ^
		q[19] ^ q[20] ^ q[21] ^ q[23] ^ q[24] ^ q[26] ^ q[28] ^ in[1] ^ in[2] ^
		in[4] ^ in[5] ^ in[6] ^ in[7] ^ in[8] ^ in[11] ^ in[14] ^ in[20] ^ in[21] ^
		in[22] ^ in[25] ^ in[29] ^ in[30] ^ in[38] ^ in[40] ^ in[41] ^ in[42] ^
		in[43] ^ in[45] ^ in[47] ^ in[50] ^ in[51] ^ in[52] ^ in[54] ^ in[55] ^
		in[56] ^ in[60] ^ in[62] ^ in[64] ^ in[65] ^ in[66] ^ in[68] ^ in[70] ^
		in[71] ^ in[72] ^ in[73] ^ in[74] ^ in[75] ^ in[76] ^ in[79] ^ in[80] ^
		in[81] ^ in[82] ^ in[83] ^ in[84] ^ in[92] ^ in[93] ^ in[95] ^ in[98] ^
		in[100] ^ in[104] ^ in[107] ^ in[108] ^ in[112] ^ in[113] ^ in[116] ^
		in[117] ^ in[121] ^ in[122] ^ in[123] ^ in[124] ^ in[126] ^ in[127] ^
		in[128] ^ in[130] ^ in[131] ^ in[132] ^ in[133] ^ in[135] ^ in[136] ^
		in[137] ^ in[139] ^ in[140] ^ in[141] ^ in[143] ^ in[144] ^ in[146] ^
		in[147] ^ in[151] ^ in[152] ^ in[154] ^ in[157] ^ in[158] ^ in[160] ^
		in[162] ^ in[163] ^ in[165] ^ in[167] ^ in[168] ^ in[169] ^ in[174] ^
		in[175] ^ in[176] ^ in[178] ^ in[183] ^ in[185] ^ in[186] ^ in[187] ^
		in[188] ^ in[193] ^ in[197] ^ in[200] ^ in[202] ^ in[203] ^ in[205] ^
		in[207] ^ in[208] ^ in[209] ^ in[211] ^ in[212] ^ in[215] ^ in[216] ^
		in[218] ^ in[219] ^ in[221] ^ in[222] ^ in[225] ^ in[226] ^ in[227] ^
		in[228] ^ in[229] ^ in[230] ^ in[231] ^ in[240] ^ in[241] ^ in[242] ^
		in[243] ^ in[244] ^ in[245] ^ in[247] ^ in[248] ^ in[250] ^ in[252];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_7(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[5] ^ q[7] ^ q[8] ^ q[10] ^ q[13] ^ q[17] ^ q[18] ^ q[20] ^ q[21] ^
		q[22] ^ q[25] ^ q[27] ^ q[28] ^ q[29] ^ q[31] ^ in[0] ^ in[2] ^ in[3] ^
		in[5] ^ in[7] ^ in[8] ^ in[10] ^ in[15] ^ in[16] ^ in[21] ^ in[22] ^
		in[23] ^ in[24] ^ in[25] ^ in[28] ^ in[29] ^ in[32] ^ in[34] ^ in[37] ^
		in[39] ^ in[41] ^ in[42] ^ in[43] ^ in[45] ^ in[46] ^ in[47] ^ in[50] ^
		in[51] ^ in[52] ^ in[54] ^ in[56] ^ in[57] ^ in[58] ^ in[60] ^ in[68] ^
		in[69] ^ in[71] ^ in[74] ^ in[75] ^ in[76] ^ in[77] ^ in[79] ^ in[80] ^
		in[87] ^ in[93] ^ in[95] ^ in[97] ^ in[98] ^ in[103] ^ in[104] ^ in[105] ^
		in[106] ^ in[108] ^ in[109] ^ in[110] ^ in[111] ^ in[116] ^ in[119] ^
		in[122] ^ in[124] ^ in[126] ^ in[129] ^ in[131] ^ in[133] ^ in[135] ^
		in[138] ^ in[140] ^ in[141] ^ in[142] ^ in[143] ^ in[145] ^ in[147] ^
		in[148] ^ in[149] ^ in[151] ^ in[152] ^ in[153] ^ in[156] ^ in[159] ^
		in[162] ^ in[163] ^ in[164] ^ in[167] ^ in[168] ^ in[171] ^ in[172] ^
		in[175] ^ in[176] ^ in[177] ^ in[179] ^ in[182] ^ in[183] ^ in[184] ^
		in[187] ^ in[189] ^ in[190] ^ in[191] ^ in[192] ^ in[193] ^ in[197] ^
		in[199] ^ in[202] ^ in[204] ^ in[206] ^ in[207] ^ in[213] ^ in[214] ^
		in[217] ^ in[219] ^ in[220] ^ in[222] ^ in[223] ^ in[224] ^ in[229] ^
		in[231] ^ in[232] ^ in[234] ^ in[237] ^ in[241] ^ in[242] ^ in[244] ^
		in[245] ^ in[246] ^ in[249] ^ in[251] ^ in[252] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_8(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[2] ^ q[3] ^ q[4] ^ q[8] ^ q[9] ^ q[10] ^ q[11] ^ q[13] ^ q[14] ^
		q[18] ^ q[21] ^ q[22] ^ q[23] ^ q[24] ^ q[26] ^ q[29] ^ q[30] ^ q[31] ^
		in[0] ^ in[1] ^ in[3] ^ in[4] ^ in[8] ^ in[10] ^ in[11] ^ in[12] ^ in[17] ^
		in[22] ^ in[23] ^ in[28] ^ in[31] ^ in[32] ^ in[33] ^ in[34] ^ in[35] ^
		in[37] ^ in[38] ^ in[40] ^ in[42] ^ in[43] ^ in[45] ^ in[46] ^ in[50] ^
		in[51] ^ in[52] ^ in[54] ^ in[57] ^ in[59] ^ in[60] ^ in[63] ^ in[65] ^
		in[66] ^ in[67] ^ in[68] ^ in[69] ^ in[70] ^ in[73] ^ in[75] ^ in[76] ^
		in[77] ^ in[78] ^ in[79] ^ in[80] ^ in[82] ^ in[83] ^ in[84] ^ in[85] ^
		in[87] ^ in[88] ^ in[95] ^ in[97] ^ in[101] ^ in[103] ^ in[105] ^ in[107] ^
		in[109] ^ in[112] ^ in[113] ^ in[114] ^ in[116] ^ in[118] ^ in[119] ^
		in[120] ^ in[126] ^ in[128] ^ in[130] ^ in[135] ^ in[137] ^ in[139] ^
		in[141] ^ in[142] ^ in[146] ^ in[148] ^ in[150] ^ in[151] ^ in[152] ^
		in[153] ^ in[154] ^ in[155] ^ in[156] ^ in[157] ^ in[158] ^ in[160] ^
		in[161] ^ in[162] ^ in[163] ^ in[164] ^ in[165] ^ in[166] ^ in[167] ^
		in[168] ^ in[170] ^ in[171] ^ in[173] ^ in[176] ^ in[177] ^ in[178] ^
		in[180] ^ in[182] ^ in[184] ^ in[185] ^ in[186] ^ in[197] ^ in[199] ^
		in[200] ^ in[201] ^ in[202] ^ in[205] ^ in[209] ^ in[210] ^ in[212] ^
		in[215] ^ in[216] ^ in[218] ^ in[220] ^ in[221] ^ in[223] ^ in[225] ^
		in[226] ^ in[227] ^ in[228] ^ in[232] ^ in[233] ^ in[234] ^ in[235] ^
		in[237] ^ in[238] ^ in[242] ^ in[245] ^ in[246] ^ in[247] ^ in[248] ^
		in[250] ^ in[253] ^ in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_9(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[2] ^ q[3] ^ q[4] ^ q[5] ^ q[9] ^ q[10] ^ q[11] ^ q[12] ^ q[14] ^
		q[15] ^ q[19] ^ q[22] ^ q[23] ^ q[24] ^ q[25] ^ q[27] ^ q[30] ^ q[31] ^
		in[1] ^ in[2] ^ in[4] ^ in[5] ^ in[9] ^ in[11] ^ in[12] ^ in[13] ^ in[18] ^
		in[23] ^ in[24] ^ in[29] ^ in[32] ^ in[33] ^ in[34] ^ in[35] ^ in[36] ^
		in[38] ^ in[39] ^ in[41] ^ in[43] ^ in[44] ^ in[46] ^ in[47] ^ in[51] ^
		in[52] ^ in[53] ^ in[55] ^ in[58] ^ in[60] ^ in[61] ^ in[64] ^ in[66] ^
		in[67] ^ in[68] ^ in[69] ^ in[70] ^ in[71] ^ in[74] ^ in[76] ^ in[77] ^
		in[78] ^ in[79] ^ in[80] ^ in[81] ^ in[83] ^ in[84] ^ in[85] ^ in[86] ^
		in[88] ^ in[89] ^ in[96] ^ in[98] ^ in[102] ^ in[104] ^ in[106] ^ in[108] ^
		in[110] ^ in[113] ^ in[114] ^ in[115] ^ in[117] ^ in[119] ^ in[120] ^
		in[121] ^ in[127] ^ in[129] ^ in[131] ^ in[136] ^ in[138] ^ in[140] ^
		in[142] ^ in[143] ^ in[147] ^ in[149] ^ in[151] ^ in[152] ^ in[153] ^
		in[154] ^ in[155] ^ in[156] ^ in[157] ^ in[158] ^ in[159] ^ in[161] ^
		in[162] ^ in[163] ^ in[164] ^ in[165] ^ in[166] ^ in[167] ^ in[168] ^
		in[169] ^ in[171] ^ in[172] ^ in[174] ^ in[177] ^ in[178] ^ in[179] ^
		in[181] ^ in[183] ^ in[185] ^ in[186] ^ in[187] ^ in[198] ^ in[200] ^
		in[201] ^ in[202] ^ in[203] ^ in[206] ^ in[210] ^ in[211] ^ in[213] ^
		in[216] ^ in[217] ^ in[219] ^ in[221] ^ in[222] ^ in[224] ^ in[226] ^
		in[227] ^ in[228] ^ in[229] ^ in[233] ^ in[234] ^ in[235] ^ in[236] ^
		in[238] ^ in[239] ^ in[243] ^ in[246] ^ in[247] ^ in[248] ^ in[249] ^
		in[251] ^ in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_10(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[5] ^ q[11] ^ q[12] ^ q[15] ^ q[16] ^ q[19] ^ q[20] ^
		q[23] ^ q[25] ^ q[26] ^ in[0] ^ in[2] ^ in[3] ^ in[5] ^ in[9] ^ in[13] ^
		in[14] ^ in[16] ^ in[19] ^ in[26] ^ in[28] ^ in[29] ^ in[31] ^ in[32] ^
		in[33] ^ in[35] ^ in[36] ^ in[39] ^ in[40] ^ in[42] ^ in[50] ^ in[52] ^
		in[55] ^ in[56] ^ in[58] ^ in[59] ^ in[60] ^ in[62] ^ in[63] ^ in[66] ^
		in[69] ^ in[70] ^ in[71] ^ in[73] ^ in[75] ^ in[77] ^ in[78] ^ in[80] ^
		in[83] ^ in[86] ^ in[89] ^ in[90] ^ in[94] ^ in[95] ^ in[96] ^ in[98] ^
		in[101] ^ in[104] ^ in[105] ^ in[106] ^ in[107] ^ in[109] ^ in[110] ^
		in[113] ^ in[115] ^ in[117] ^ in[119] ^ in[120] ^ in[121] ^ in[122] ^
		in[123] ^ in[125] ^ in[126] ^ in[127] ^ in[130] ^ in[134] ^ in[135] ^
		in[136] ^ in[139] ^ in[141] ^ in[148] ^ in[149] ^ in[150] ^ in[151] ^
		in[152] ^ in[153] ^ in[154] ^ in[157] ^ in[159] ^ in[160] ^ in[161] ^
		in[163] ^ in[164] ^ in[165] ^ in[168] ^ in[171] ^ in[173] ^ in[175] ^
		in[178] ^ in[179] ^ in[180] ^ in[183] ^ in[184] ^ in[187] ^ in[190] ^
		in[191] ^ in[192] ^ in[193] ^ in[194] ^ in[197] ^ in[198] ^ in[204] ^
		in[208] ^ in[209] ^ in[210] ^ in[211] ^ in[216] ^ in[217] ^ in[218] ^
		in[220] ^ in[222] ^ in[223] ^ in[224] ^ in[225] ^ in[226] ^ in[229] ^
		in[235] ^ in[236] ^ in[239] ^ in[240] ^ in[243] ^ in[244] ^ in[247] ^
		in[249] ^ in[250];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_11(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[4] ^ q[10] ^ q[12] ^ q[16] ^ q[17] ^ q[19] ^ q[20] ^ q[21] ^
		q[26] ^ q[27] ^ q[28] ^ q[31] ^ in[0] ^ in[1] ^ in[3] ^ in[4] ^ in[9] ^
		in[12] ^ in[14] ^ in[15] ^ in[16] ^ in[17] ^ in[20] ^ in[24] ^ in[25] ^
		in[26] ^ in[27] ^ in[28] ^ in[31] ^ in[33] ^ in[36] ^ in[40] ^ in[41] ^
		in[43] ^ in[44] ^ in[45] ^ in[47] ^ in[48] ^ in[50] ^ in[51] ^ in[54] ^
		in[55] ^ in[56] ^ in[57] ^ in[58] ^ in[59] ^ in[64] ^ in[65] ^ in[66] ^
		in[68] ^ in[70] ^ in[71] ^ in[73] ^ in[74] ^ in[76] ^ in[78] ^ in[82] ^
		in[83] ^ in[85] ^ in[90] ^ in[91] ^ in[94] ^ in[98] ^ in[101] ^ in[102] ^
		in[103] ^ in[104] ^ in[105] ^ in[107] ^ in[108] ^ in[113] ^ in[117] ^
		in[119] ^ in[120] ^ in[121] ^ in[122] ^ in[124] ^ in[125] ^ in[131] ^
		in[132] ^ in[134] ^ in[140] ^ in[142] ^ in[143] ^ in[144] ^ in[150] ^
		in[152] ^ in[153] ^ in[154] ^ in[156] ^ in[160] ^ in[164] ^ in[165] ^
		in[167] ^ in[170] ^ in[171] ^ in[174] ^ in[176] ^ in[179] ^ in[180] ^
		in[181] ^ in[182] ^ in[183] ^ in[184] ^ in[185] ^ in[186] ^ in[190] ^
		in[195] ^ in[197] ^ in[201] ^ in[202] ^ in[203] ^ in[205] ^ in[207] ^
		in[208] ^ in[211] ^ in[214] ^ in[216] ^ in[217] ^ in[218] ^ in[219] ^
		in[221] ^ in[223] ^ in[225] ^ in[228] ^ in[234] ^ in[236] ^ in[240] ^
		in[241] ^ in[243] ^ in[244] ^ in[245] ^ in[250] ^ in[251] ^ in[252] ^
		in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_12(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[3] ^ q[4] ^ q[5] ^ q[6] ^ q[10] ^ q[11] ^ q[17] ^ q[18] ^ q[19] ^ q[20] ^
		q[21] ^ q[22] ^ q[24] ^ q[27] ^ q[29] ^ q[31] ^ in[0] ^ in[1] ^ in[2] ^
		in[4] ^ in[5] ^ in[6] ^ in[9] ^ in[12] ^ in[13] ^ in[15] ^ in[17] ^ in[18] ^
		in[21] ^ in[24] ^ in[27] ^ in[30] ^ in[31] ^ in[41] ^ in[42] ^ in[46] ^
		in[47] ^ in[49] ^ in[50] ^ in[51] ^ in[52] ^ in[53] ^ in[54] ^ in[56] ^
		in[57] ^ in[59] ^ in[61] ^ in[63] ^ in[68] ^ in[69] ^ in[71] ^ in[73] ^
		in[74] ^ in[75] ^ in[77] ^ in[81] ^ in[82] ^ in[85] ^ in[86] ^ in[87] ^
		in[91] ^ in[92] ^ in[94] ^ in[96] ^ in[97] ^ in[98] ^ in[101] ^ in[102] ^
		in[105] ^ in[108] ^ in[109] ^ in[110] ^ in[111] ^ in[113] ^ in[116] ^
		in[117] ^ in[119] ^ in[120] ^ in[121] ^ in[122] ^ in[127] ^ in[128] ^
		in[133] ^ in[134] ^ in[136] ^ in[137] ^ in[141] ^ in[145] ^ in[149] ^
		in[153] ^ in[154] ^ in[156] ^ in[157] ^ in[158] ^ in[162] ^ in[165] ^
		in[167] ^ in[168] ^ in[169] ^ in[170] ^ in[175] ^ in[177] ^ in[180] ^
		in[181] ^ in[184] ^ in[185] ^ in[187] ^ in[188] ^ in[190] ^ in[192] ^
		in[193] ^ in[194] ^ in[196] ^ in[197] ^ in[199] ^ in[201] ^ in[204] ^
		in[206] ^ in[207] ^ in[210] ^ in[214] ^ in[215] ^ in[216] ^ in[217] ^
		in[218] ^ in[219] ^ in[220] ^ in[222] ^ in[227] ^ in[228] ^ in[229] ^
		in[230] ^ in[234] ^ in[235] ^ in[241] ^ in[242] ^ in[243] ^ in[244] ^
		in[245] ^ in[246] ^ in[248] ^ in[251] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_13(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[4] ^ q[5] ^ q[6] ^ q[7] ^ q[11] ^ q[12] ^ q[18] ^ q[19] ^ q[20] ^ q[21] ^
		q[22] ^ q[23] ^ q[25] ^ q[28] ^ q[30] ^ in[1] ^ in[2] ^ in[3] ^ in[5] ^
		in[6] ^ in[7] ^ in[10] ^ in[13] ^ in[14] ^ in[16] ^ in[18] ^ in[19] ^
		in[22] ^ in[25] ^ in[28] ^ in[31] ^ in[32] ^ in[42] ^ in[43] ^ in[47] ^
		in[48] ^ in[50] ^ in[51] ^ in[52] ^ in[53] ^ in[54] ^ in[55] ^ in[57] ^
		in[58] ^ in[60] ^ in[62] ^ in[64] ^ in[69] ^ in[70] ^ in[72] ^ in[74] ^
		in[75] ^ in[76] ^ in[78] ^ in[82] ^ in[83] ^ in[86] ^ in[87] ^ in[88] ^
		in[92] ^ in[93] ^ in[95] ^ in[97] ^ in[98] ^ in[99] ^ in[102] ^ in[103] ^
		in[106] ^ in[109] ^ in[110] ^ in[111] ^ in[112] ^ in[114] ^ in[117] ^
		in[118] ^ in[120] ^ in[121] ^ in[122] ^ in[123] ^ in[128] ^ in[129] ^
		in[134] ^ in[135] ^ in[137] ^ in[138] ^ in[142] ^ in[146] ^ in[150] ^
		in[154] ^ in[155] ^ in[157] ^ in[158] ^ in[159] ^ in[163] ^ in[166] ^
		in[168] ^ in[169] ^ in[170] ^ in[171] ^ in[176] ^ in[178] ^ in[181] ^
		in[182] ^ in[185] ^ in[186] ^ in[188] ^ in[189] ^ in[191] ^ in[193] ^
		in[194] ^ in[195] ^ in[197] ^ in[198] ^ in[200] ^ in[202] ^ in[205] ^
		in[207] ^ in[208] ^ in[211] ^ in[215] ^ in[216] ^ in[217] ^ in[218] ^
		in[219] ^ in[220] ^ in[221] ^ in[223] ^ in[228] ^ in[229] ^ in[230] ^
		in[231] ^ in[235] ^ in[236] ^ in[242] ^ in[243] ^ in[244] ^ in[245] ^
		in[246] ^ in[247] ^ in[249] ^ in[252] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_14(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[5] ^ q[6] ^ q[7] ^ q[8] ^ q[12] ^ q[13] ^ q[19] ^ q[20] ^ q[21] ^
		q[22] ^ q[23] ^ q[24] ^ q[26] ^ q[29] ^ q[31] ^ in[2] ^ in[3] ^ in[4] ^
		in[6] ^ in[7] ^ in[8] ^ in[11] ^ in[14] ^ in[15] ^ in[17] ^ in[19] ^
		in[20] ^ in[23] ^ in[26] ^ in[29] ^ in[32] ^ in[33] ^ in[43] ^ in[44] ^
		in[48] ^ in[49] ^ in[51] ^ in[52] ^ in[53] ^ in[54] ^ in[55] ^ in[56] ^
		in[58] ^ in[59] ^ in[61] ^ in[63] ^ in[65] ^ in[70] ^ in[71] ^ in[73] ^
		in[75] ^ in[76] ^ in[77] ^ in[79] ^ in[83] ^ in[84] ^ in[87] ^ in[88] ^
		in[89] ^ in[93] ^ in[94] ^ in[96] ^ in[98] ^ in[99] ^ in[100] ^ in[103] ^
		in[104] ^ in[107] ^ in[110] ^ in[111] ^ in[112] ^ in[113] ^ in[115] ^
		in[118] ^ in[119] ^ in[121] ^ in[122] ^ in[123] ^ in[124] ^ in[129] ^
		in[130] ^ in[135] ^ in[136] ^ in[138] ^ in[139] ^ in[143] ^ in[147] ^
		in[151] ^ in[155] ^ in[156] ^ in[158] ^ in[159] ^ in[160] ^ in[164] ^
		in[167] ^ in[169] ^ in[170] ^ in[171] ^ in[172] ^ in[177] ^ in[179] ^
		in[182] ^ in[183] ^ in[186] ^ in[187] ^ in[189] ^ in[190] ^ in[192] ^
		in[194] ^ in[195] ^ in[196] ^ in[198] ^ in[199] ^ in[201] ^ in[203] ^
		in[206] ^ in[208] ^ in[209] ^ in[212] ^ in[216] ^ in[217] ^ in[218] ^
		in[219] ^ in[220] ^ in[221] ^ in[222] ^ in[224] ^ in[229] ^ in[230] ^
		in[231] ^ in[232] ^ in[236] ^ in[237] ^ in[243] ^ in[244] ^ in[245] ^
		in[246] ^ in[247] ^ in[248] ^ in[250] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_15(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[6] ^ q[7] ^ q[8] ^ q[9] ^ q[13] ^ q[14] ^ q[20] ^ q[21] ^ q[22] ^
		q[23] ^ q[24] ^ q[25] ^ q[27] ^ q[30] ^ in[3] ^ in[4] ^ in[5] ^ in[7] ^
		in[8] ^ in[9] ^ in[12] ^ in[15] ^ in[16] ^ in[18] ^ in[20] ^ in[21] ^
		in[24] ^ in[27] ^ in[30] ^ in[33] ^ in[34] ^ in[44] ^ in[45] ^ in[49] ^
		in[50] ^ in[52] ^ in[53] ^ in[54] ^ in[55] ^ in[56] ^ in[57] ^ in[59] ^
		in[60] ^ in[62] ^ in[64] ^ in[66] ^ in[71] ^ in[72] ^ in[74] ^ in[76] ^
		in[77] ^ in[78] ^ in[80] ^ in[84] ^ in[85] ^ in[88] ^ in[89] ^ in[90] ^
		in[94] ^ in[95] ^ in[97] ^ in[99] ^ in[100] ^ in[101] ^ in[104] ^ in[105] ^
		in[108] ^ in[111] ^ in[112] ^ in[113] ^ in[114] ^ in[116] ^ in[119] ^
		in[120] ^ in[122] ^ in[123] ^ in[124] ^ in[125] ^ in[130] ^ in[131] ^
		in[136] ^ in[137] ^ in[139] ^ in[140] ^ in[144] ^ in[148] ^ in[152] ^
		in[156] ^ in[157] ^ in[159] ^ in[160] ^ in[161] ^ in[165] ^ in[168] ^
		in[170] ^ in[171] ^ in[172] ^ in[173] ^ in[178] ^ in[180] ^ in[183] ^
		in[184] ^ in[187] ^ in[188] ^ in[190] ^ in[191] ^ in[193] ^ in[195] ^
		in[196] ^ in[197] ^ in[199] ^ in[200] ^ in[202] ^ in[204] ^ in[207] ^
		in[209] ^ in[210] ^ in[213] ^ in[217] ^ in[218] ^ in[219] ^ in[220] ^
		in[221] ^ in[222] ^ in[223] ^ in[225] ^ in[230] ^ in[231] ^ in[232] ^
		in[233] ^ in[237] ^ in[238] ^ in[244] ^ in[245] ^ in[246] ^ in[247] ^
		in[248] ^ in[249] ^ in[251] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_16(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[3] ^ q[4] ^ q[6] ^ q[7] ^ q[8] ^ q[9] ^ q[13] ^ q[14] ^ q[15] ^ q[19] ^
		q[21] ^ q[22] ^ q[23] ^ q[25] ^ q[26] ^ in[0] ^ in[4] ^ in[5] ^ in[8] ^
		in[12] ^ in[13] ^ in[17] ^ in[19] ^ in[21] ^ in[22] ^ in[24] ^ in[26] ^
		in[29] ^ in[30] ^ in[32] ^ in[35] ^ in[37] ^ in[44] ^ in[46] ^ in[47] ^
		in[48] ^ in[51] ^ in[56] ^ in[57] ^ in[66] ^ in[68] ^ in[75] ^ in[77] ^
		in[78] ^ in[82] ^ in[83] ^ in[84] ^ in[86] ^ in[87] ^ in[89] ^ in[90] ^
		in[91] ^ in[94] ^ in[97] ^ in[99] ^ in[100] ^ in[102] ^ in[103] ^ in[104] ^
		in[105] ^ in[109] ^ in[110] ^ in[111] ^ in[112] ^ in[115] ^ in[116] ^
		in[118] ^ in[119] ^ in[120] ^ in[121] ^ in[124] ^ in[127] ^ in[128] ^
		in[131] ^ in[134] ^ in[135] ^ in[136] ^ in[138] ^ in[140] ^ in[141] ^
		in[143] ^ in[144] ^ in[145] ^ in[151] ^ in[153] ^ in[155] ^ in[156] ^
		in[157] ^ in[160] ^ in[167] ^ in[170] ^ in[173] ^ in[174] ^ in[179] ^
		in[181] ^ in[182] ^ in[183] ^ in[184] ^ in[185] ^ in[186] ^ in[189] ^
		in[190] ^ in[193] ^ in[196] ^ in[199] ^ in[200] ^ in[202] ^ in[205] ^
		in[207] ^ in[209] ^ in[211] ^ in[212] ^ in[216] ^ in[218] ^ in[219] ^
		in[220] ^ in[221] ^ in[222] ^ in[223] ^ in[227] ^ in[228] ^ in[230] ^
		in[231] ^ in[232] ^ in[233] ^ in[237] ^ in[238] ^ in[239] ^ in[243] ^
		in[245] ^ in[246] ^ in[247] ^ in[249] ^ in[250];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_17(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[4] ^ q[5] ^ q[7] ^ q[8] ^ q[9] ^ q[10] ^ q[14] ^ q[15] ^ q[16] ^
		q[20] ^ q[22] ^ q[23] ^ q[24] ^ q[26] ^ q[27] ^ in[1] ^ in[5] ^ in[6] ^
		in[9] ^ in[13] ^ in[14] ^ in[18] ^ in[20] ^ in[22] ^ in[23] ^ in[25] ^
		in[27] ^ in[30] ^ in[31] ^ in[33] ^ in[36] ^ in[38] ^ in[45] ^ in[47] ^
		in[48] ^ in[49] ^ in[52] ^ in[57] ^ in[58] ^ in[67] ^ in[69] ^ in[76] ^
		in[78] ^ in[79] ^ in[83] ^ in[84] ^ in[85] ^ in[87] ^ in[88] ^ in[90] ^
		in[91] ^ in[92] ^ in[95] ^ in[98] ^ in[100] ^ in[101] ^ in[103] ^ in[104] ^
		in[105] ^ in[106] ^ in[110] ^ in[111] ^ in[112] ^ in[113] ^ in[116] ^
		in[117] ^ in[119] ^ in[120] ^ in[121] ^ in[122] ^ in[125] ^ in[128] ^
		in[129] ^ in[132] ^ in[135] ^ in[136] ^ in[137] ^ in[139] ^ in[141] ^
		in[142] ^ in[144] ^ in[145] ^ in[146] ^ in[152] ^ in[154] ^ in[156] ^
		in[157] ^ in[158] ^ in[161] ^ in[168] ^ in[171] ^ in[174] ^ in[175] ^
		in[180] ^ in[182] ^ in[183] ^ in[184] ^ in[185] ^ in[186] ^ in[187] ^
		in[190] ^ in[191] ^ in[194] ^ in[197] ^ in[200] ^ in[201] ^ in[203] ^
		in[206] ^ in[208] ^ in[210] ^ in[212] ^ in[213] ^ in[217] ^ in[219] ^
		in[220] ^ in[221] ^ in[222] ^ in[223] ^ in[224] ^ in[228] ^ in[229] ^
		in[231] ^ in[232] ^ in[233] ^ in[234] ^ in[238] ^ in[239] ^ in[240] ^
		in[244] ^ in[246] ^ in[247] ^ in[248] ^ in[250] ^ in[251];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_18(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[5] ^ q[6] ^ q[8] ^ q[9] ^ q[10] ^ q[11] ^ q[15] ^ q[16] ^
		q[17] ^ q[21] ^ q[23] ^ q[24] ^ q[25] ^ q[27] ^ q[28] ^ in[2] ^ in[6] ^
		in[7] ^ in[10] ^ in[14] ^ in[15] ^ in[19] ^ in[21] ^ in[23] ^ in[24] ^
		in[26] ^ in[28] ^ in[31] ^ in[32] ^ in[34] ^ in[37] ^ in[39] ^ in[46] ^
		in[48] ^ in[49] ^ in[50] ^ in[53] ^ in[58] ^ in[59] ^ in[68] ^ in[70] ^
		in[77] ^ in[79] ^ in[80] ^ in[84] ^ in[85] ^ in[86] ^ in[88] ^ in[89] ^
		in[91] ^ in[92] ^ in[93] ^ in[96] ^ in[99] ^ in[101] ^ in[102] ^ in[104] ^
		in[105] ^ in[106] ^ in[107] ^ in[111] ^ in[112] ^ in[113] ^ in[114] ^
		in[117] ^ in[118] ^ in[120] ^ in[121] ^ in[122] ^ in[123] ^ in[126] ^
		in[129] ^ in[130] ^ in[133] ^ in[136] ^ in[137] ^ in[138] ^ in[140] ^
		in[142] ^ in[143] ^ in[145] ^ in[146] ^ in[147] ^ in[153] ^ in[155] ^
		in[157] ^ in[158] ^ in[159] ^ in[162] ^ in[169] ^ in[172] ^ in[175] ^
		in[176] ^ in[181] ^ in[183] ^ in[184] ^ in[185] ^ in[186] ^ in[187] ^
		in[188] ^ in[191] ^ in[192] ^ in[195] ^ in[198] ^ in[201] ^ in[202] ^
		in[204] ^ in[207] ^ in[209] ^ in[211] ^ in[213] ^ in[214] ^ in[218] ^
		in[220] ^ in[221] ^ in[222] ^ in[223] ^ in[224] ^ in[225] ^ in[229] ^
		in[230] ^ in[232] ^ in[233] ^ in[234] ^ in[235] ^ in[239] ^ in[240] ^
		in[241] ^ in[245] ^ in[247] ^ in[248] ^ in[249] ^ in[251] ^ in[252];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_19(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[6] ^ q[7] ^ q[9] ^ q[10] ^ q[11] ^ q[12] ^ q[16] ^
		q[17] ^ q[18] ^ q[22] ^ q[24] ^ q[25] ^ q[26] ^ q[28] ^ q[29] ^ in[3] ^
		in[7] ^ in[8] ^ in[11] ^ in[15] ^ in[16] ^ in[20] ^ in[22] ^ in[24] ^
		in[25] ^ in[27] ^ in[29] ^ in[32] ^ in[33] ^ in[35] ^ in[38] ^ in[40] ^
		in[47] ^ in[49] ^ in[50] ^ in[51] ^ in[54] ^ in[59] ^ in[60] ^ in[69] ^
		in[71] ^ in[78] ^ in[80] ^ in[81] ^ in[85] ^ in[86] ^ in[87] ^ in[89] ^
		in[90] ^ in[92] ^ in[93] ^ in[94] ^ in[97] ^ in[100] ^ in[102] ^ in[103] ^
		in[105] ^ in[106] ^ in[107] ^ in[108] ^ in[112] ^ in[113] ^ in[114] ^
		in[115] ^ in[118] ^ in[119] ^ in[121] ^ in[122] ^ in[123] ^ in[124] ^
		in[127] ^ in[130] ^ in[131] ^ in[134] ^ in[137] ^ in[138] ^ in[139] ^
		in[141] ^ in[143] ^ in[144] ^ in[146] ^ in[147] ^ in[148] ^ in[154] ^
		in[156] ^ in[158] ^ in[159] ^ in[160] ^ in[163] ^ in[170] ^ in[173] ^
		in[176] ^ in[177] ^ in[182] ^ in[184] ^ in[185] ^ in[186] ^ in[187] ^
		in[188] ^ in[189] ^ in[192] ^ in[193] ^ in[196] ^ in[199] ^ in[202] ^
		in[203] ^ in[205] ^ in[208] ^ in[210] ^ in[212] ^ in[214] ^ in[215] ^
		in[219] ^ in[221] ^ in[222] ^ in[223] ^ in[224] ^ in[225] ^ in[226] ^
		in[230] ^ in[231] ^ in[233] ^ in[234] ^ in[235] ^ in[236] ^ in[240] ^
		in[241] ^ in[242] ^ in[246] ^ in[248] ^ in[249] ^ in[250] ^ in[252] ^
		in[253];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_20(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[3] ^ q[7] ^ q[8] ^ q[10] ^ q[11] ^ q[12] ^ q[13] ^
		q[17] ^ q[18] ^ q[19] ^ q[23] ^ q[25] ^ q[26] ^ q[27] ^ q[29] ^ q[30] ^
		in[4] ^ in[8] ^ in[9] ^ in[12] ^ in[16] ^ in[17] ^ in[21] ^ in[23] ^
		in[25] ^ in[26] ^ in[28] ^ in[30] ^ in[33] ^ in[34] ^ in[36] ^ in[39] ^
		in[41] ^ in[48] ^ in[50] ^ in[51] ^ in[52] ^ in[55] ^ in[60] ^ in[61] ^
		in[70] ^ in[72] ^ in[79] ^ in[81] ^ in[82] ^ in[86] ^ in[87] ^ in[88] ^
		in[90] ^ in[91] ^ in[93] ^ in[94] ^ in[95] ^ in[98] ^ in[101] ^ in[103] ^
		in[104] ^ in[106] ^ in[107] ^ in[108] ^ in[109] ^ in[113] ^ in[114] ^
		in[115] ^ in[116] ^ in[119] ^ in[120] ^ in[122] ^ in[123] ^ in[124] ^
		in[125] ^ in[128] ^ in[131] ^ in[132] ^ in[135] ^ in[138] ^ in[139] ^
		in[140] ^ in[142] ^ in[144] ^ in[145] ^ in[147] ^ in[148] ^ in[149] ^
		in[155] ^ in[157] ^ in[159] ^ in[160] ^ in[161] ^ in[164] ^ in[171] ^
		in[174] ^ in[177] ^ in[178] ^ in[183] ^ in[185] ^ in[186] ^ in[187] ^
		in[188] ^ in[189] ^ in[190] ^ in[193] ^ in[194] ^ in[197] ^ in[200] ^
		in[203] ^ in[204] ^ in[206] ^ in[209] ^ in[211] ^ in[213] ^ in[215] ^
		in[216] ^ in[220] ^ in[222] ^ in[223] ^ in[224] ^ in[225] ^ in[226] ^
		in[227] ^ in[231] ^ in[232] ^ in[234] ^ in[235] ^ in[236] ^ in[237] ^
		in[241] ^ in[242] ^ in[243] ^ in[247] ^ in[249] ^ in[250] ^ in[251] ^
		in[253] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_21(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[3] ^ q[4] ^ q[8] ^ q[9] ^ q[11] ^ q[12] ^ q[13] ^
		q[14] ^ q[18] ^ q[19] ^ q[20] ^ q[24] ^ q[26] ^ q[27] ^ q[28] ^ q[30] ^
		q[31] ^ in[5] ^ in[9] ^ in[10] ^ in[13] ^ in[17] ^ in[18] ^ in[22] ^
		in[24] ^ in[26] ^ in[27] ^ in[29] ^ in[31] ^ in[34] ^ in[35] ^ in[37] ^
		in[40] ^ in[42] ^ in[49] ^ in[51] ^ in[52] ^ in[53] ^ in[56] ^ in[61] ^
		in[62] ^ in[71] ^ in[73] ^ in[80] ^ in[82] ^ in[83] ^ in[87] ^ in[88] ^
		in[89] ^ in[91] ^ in[92] ^ in[94] ^ in[95] ^ in[96] ^ in[99] ^ in[102] ^
		in[104] ^ in[105] ^ in[107] ^ in[108] ^ in[109] ^ in[110] ^ in[114] ^
		in[115] ^ in[116] ^ in[117] ^ in[120] ^ in[121] ^ in[123] ^ in[124] ^
		in[125] ^ in[126] ^ in[129] ^ in[132] ^ in[133] ^ in[136] ^ in[139] ^
		in[140] ^ in[141] ^ in[143] ^ in[145] ^ in[146] ^ in[148] ^ in[149] ^
		in[150] ^ in[156] ^ in[158] ^ in[160] ^ in[161] ^ in[162] ^ in[165] ^
		in[172] ^ in[175] ^ in[178] ^ in[179] ^ in[184] ^ in[186] ^ in[187] ^
		in[188] ^ in[189] ^ in[190] ^ in[191] ^ in[194] ^ in[195] ^ in[198] ^
		in[201] ^ in[204] ^ in[205] ^ in[207] ^ in[210] ^ in[212] ^ in[214] ^
		in[216] ^ in[217] ^ in[221] ^ in[223] ^ in[224] ^ in[225] ^ in[226] ^
		in[227] ^ in[228] ^ in[232] ^ in[233] ^ in[235] ^ in[236] ^ in[237] ^
		in[238] ^ in[242] ^ in[243] ^ in[244] ^ in[248] ^ in[250] ^ in[251] ^
		in[252] ^ in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_22(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[5] ^ q[6] ^ q[9] ^ q[12] ^ q[14] ^ q[15] ^ q[20] ^ q[21] ^ q[24] ^
		q[25] ^ q[27] ^ q[29] ^ in[0] ^ in[9] ^ in[11] ^ in[12] ^ in[14] ^ in[16] ^
		in[18] ^ in[19] ^ in[23] ^ in[24] ^ in[26] ^ in[27] ^ in[29] ^ in[31] ^
		in[34] ^ in[35] ^ in[36] ^ in[37] ^ in[38] ^ in[41] ^ in[43] ^ in[44] ^
		in[45] ^ in[47] ^ in[48] ^ in[52] ^ in[55] ^ in[57] ^ in[58] ^ in[60] ^
		in[61] ^ in[62] ^ in[65] ^ in[66] ^ in[67] ^ in[68] ^ in[73] ^ in[74] ^
		in[79] ^ in[82] ^ in[85] ^ in[87] ^ in[88] ^ in[89] ^ in[90] ^ in[92] ^
		in[93] ^ in[94] ^ in[98] ^ in[99] ^ in[100] ^ in[101] ^ in[104] ^ in[105] ^
		in[108] ^ in[109] ^ in[113] ^ in[114] ^ in[115] ^ in[119] ^ in[121] ^
		in[122] ^ in[123] ^ in[124] ^ in[128] ^ in[130] ^ in[132] ^ in[133] ^
		in[135] ^ in[136] ^ in[140] ^ in[141] ^ in[142] ^ in[143] ^ in[146] ^
		in[147] ^ in[150] ^ in[155] ^ in[156] ^ in[157] ^ in[158] ^ in[159] ^
		in[163] ^ in[167] ^ in[169] ^ in[170] ^ in[171] ^ in[172] ^ in[173] ^
		in[176] ^ in[179] ^ in[180] ^ in[182] ^ in[183] ^ in[185] ^ in[186] ^
		in[187] ^ in[189] ^ in[193] ^ in[194] ^ in[195] ^ in[196] ^ in[197] ^
		in[198] ^ in[201] ^ in[203] ^ in[205] ^ in[206] ^ in[207] ^ in[209] ^
		in[210] ^ in[211] ^ in[212] ^ in[213] ^ in[214] ^ in[215] ^ in[216] ^
		in[217] ^ in[218] ^ in[222] ^ in[225] ^ in[229] ^ in[230] ^ in[233] ^
		in[236] ^ in[238] ^ in[239] ^ in[244] ^ in[245] ^ in[248] ^ in[249] ^
		in[251] ^ in[253];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_23(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[3] ^ q[4] ^ q[7] ^ q[15] ^ q[16] ^ q[19] ^ q[21] ^ q[22] ^ q[24] ^
		q[25] ^ q[26] ^ q[30] ^ q[31] ^ in[0] ^ in[1] ^ in[6] ^ in[9] ^ in[13] ^
		in[15] ^ in[16] ^ in[17] ^ in[19] ^ in[20] ^ in[26] ^ in[27] ^ in[29] ^
		in[31] ^ in[34] ^ in[35] ^ in[36] ^ in[38] ^ in[39] ^ in[42] ^ in[46] ^
		in[47] ^ in[49] ^ in[50] ^ in[54] ^ in[55] ^ in[56] ^ in[59] ^ in[60] ^
		in[62] ^ in[65] ^ in[69] ^ in[72] ^ in[73] ^ in[74] ^ in[75] ^ in[79] ^
		in[80] ^ in[81] ^ in[82] ^ in[84] ^ in[85] ^ in[86] ^ in[87] ^ in[88] ^
		in[89] ^ in[90] ^ in[91] ^ in[93] ^ in[96] ^ in[97] ^ in[98] ^ in[100] ^
		in[102] ^ in[103] ^ in[104] ^ in[105] ^ in[109] ^ in[111] ^ in[113] ^
		in[115] ^ in[117] ^ in[118] ^ in[119] ^ in[120] ^ in[122] ^ in[124] ^
		in[126] ^ in[127] ^ in[128] ^ in[129] ^ in[131] ^ in[132] ^ in[133] ^
		in[135] ^ in[141] ^ in[142] ^ in[147] ^ in[148] ^ in[149] ^ in[155] ^
		in[157] ^ in[159] ^ in[160] ^ in[161] ^ in[162] ^ in[164] ^ in[166] ^
		in[167] ^ in[168] ^ in[169] ^ in[173] ^ in[174] ^ in[177] ^ in[180] ^
		in[181] ^ in[182] ^ in[184] ^ in[187] ^ in[191] ^ in[192] ^ in[193] ^
		in[195] ^ in[196] ^ in[201] ^ in[203] ^ in[204] ^ in[206] ^ in[209] ^
		in[211] ^ in[213] ^ in[215] ^ in[217] ^ in[218] ^ in[219] ^ in[223] ^
		in[224] ^ in[227] ^ in[228] ^ in[231] ^ in[239] ^ in[240] ^ in[243] ^
		in[245] ^ in[246] ^ in[248] ^ in[249] ^ in[250] ^ in[254] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_24(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[4] ^ q[5] ^ q[8] ^ q[16] ^ q[17] ^ q[20] ^ q[22] ^ q[23] ^
		q[25] ^ q[26] ^ q[27] ^ q[31] ^ in[1] ^ in[2] ^ in[7] ^ in[10] ^ in[14] ^
		in[16] ^ in[17] ^ in[18] ^ in[20] ^ in[21] ^ in[27] ^ in[28] ^ in[30] ^
		in[32] ^ in[35] ^ in[36] ^ in[37] ^ in[39] ^ in[40] ^ in[43] ^ in[47] ^
		in[48] ^ in[50] ^ in[51] ^ in[55] ^ in[56] ^ in[57] ^ in[60] ^ in[61] ^
		in[63] ^ in[66] ^ in[70] ^ in[73] ^ in[74] ^ in[75] ^ in[76] ^ in[80] ^
		in[81] ^ in[82] ^ in[83] ^ in[85] ^ in[86] ^ in[87] ^ in[88] ^ in[89] ^
		in[90] ^ in[91] ^ in[92] ^ in[94] ^ in[97] ^ in[98] ^ in[99] ^ in[101] ^
		in[103] ^ in[104] ^ in[105] ^ in[106] ^ in[110] ^ in[112] ^ in[114] ^
		in[116] ^ in[118] ^ in[119] ^ in[120] ^ in[121] ^ in[123] ^ in[125] ^
		in[127] ^ in[128] ^ in[129] ^ in[130] ^ in[132] ^ in[133] ^ in[134] ^
		in[136] ^ in[142] ^ in[143] ^ in[148] ^ in[149] ^ in[150] ^ in[156] ^
		in[158] ^ in[160] ^ in[161] ^ in[162] ^ in[163] ^ in[165] ^ in[167] ^
		in[168] ^ in[169] ^ in[170] ^ in[174] ^ in[175] ^ in[178] ^ in[181] ^
		in[182] ^ in[183] ^ in[185] ^ in[188] ^ in[192] ^ in[193] ^ in[194] ^
		in[196] ^ in[197] ^ in[202] ^ in[204] ^ in[205] ^ in[207] ^ in[210] ^
		in[212] ^ in[214] ^ in[216] ^ in[218] ^ in[219] ^ in[220] ^ in[224] ^
		in[225] ^ in[228] ^ in[229] ^ in[232] ^ in[240] ^ in[241] ^ in[244] ^
		in[246] ^ in[247] ^ in[249] ^ in[250] ^ in[251] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_25(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[2] ^ q[5] ^ q[6] ^ q[9] ^ q[17] ^ q[18] ^ q[21] ^ q[23] ^ q[24] ^
		q[26] ^ q[27] ^ q[28] ^ in[2] ^ in[3] ^ in[8] ^ in[11] ^ in[15] ^ in[17] ^
		in[18] ^ in[19] ^ in[21] ^ in[22] ^ in[28] ^ in[29] ^ in[31] ^ in[33] ^
		in[36] ^ in[37] ^ in[38] ^ in[40] ^ in[41] ^ in[44] ^ in[48] ^ in[49] ^
		in[51] ^ in[52] ^ in[56] ^ in[57] ^ in[58] ^ in[61] ^ in[62] ^ in[64] ^
		in[67] ^ in[71] ^ in[74] ^ in[75] ^ in[76] ^ in[77] ^ in[81] ^ in[82] ^
		in[83] ^ in[84] ^ in[86] ^ in[87] ^ in[88] ^ in[89] ^ in[90] ^ in[91] ^
		in[92] ^ in[93] ^ in[95] ^ in[98] ^ in[99] ^ in[100] ^ in[102] ^ in[104] ^
		in[105] ^ in[106] ^ in[107] ^ in[111] ^ in[113] ^ in[115] ^ in[117] ^
		in[119] ^ in[120] ^ in[121] ^ in[122] ^ in[124] ^ in[126] ^ in[128] ^
		in[129] ^ in[130] ^ in[131] ^ in[133] ^ in[134] ^ in[135] ^ in[137] ^
		in[143] ^ in[144] ^ in[149] ^ in[150] ^ in[151] ^ in[157] ^ in[159] ^
		in[161] ^ in[162] ^ in[163] ^ in[164] ^ in[166] ^ in[168] ^ in[169] ^
		in[170] ^ in[171] ^ in[175] ^ in[176] ^ in[179] ^ in[182] ^ in[183] ^
		in[184] ^ in[186] ^ in[189] ^ in[193] ^ in[194] ^ in[195] ^ in[197] ^
		in[198] ^ in[203] ^ in[205] ^ in[206] ^ in[208] ^ in[211] ^ in[213] ^
		in[215] ^ in[217] ^ in[219] ^ in[220] ^ in[221] ^ in[225] ^ in[226] ^
		in[229] ^ in[230] ^ in[233] ^ in[241] ^ in[242] ^ in[245] ^ in[247] ^
		in[248] ^ in[250] ^ in[251] ^ in[252];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_26(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[4] ^ q[7] ^ q[13] ^ q[18] ^ q[22] ^ q[25] ^ q[27] ^ q[29] ^
		q[31] ^ in[0] ^ in[3] ^ in[4] ^ in[6] ^ in[10] ^ in[18] ^ in[19] ^ in[20] ^
		in[22] ^ in[23] ^ in[24] ^ in[25] ^ in[26] ^ in[28] ^ in[31] ^ in[38] ^
		in[39] ^ in[41] ^ in[42] ^ in[44] ^ in[47] ^ in[48] ^ in[49] ^ in[52] ^
		in[54] ^ in[55] ^ in[57] ^ in[59] ^ in[60] ^ in[61] ^ in[62] ^ in[66] ^
		in[67] ^ in[73] ^ in[75] ^ in[76] ^ in[77] ^ in[78] ^ in[79] ^ in[81] ^
		in[88] ^ in[89] ^ in[90] ^ in[91] ^ in[92] ^ in[93] ^ in[95] ^ in[97] ^
		in[98] ^ in[100] ^ in[104] ^ in[105] ^ in[107] ^ in[108] ^ in[110] ^
		in[111] ^ in[112] ^ in[113] ^ in[117] ^ in[119] ^ in[120] ^ in[121] ^
		in[122] ^ in[126] ^ in[128] ^ in[129] ^ in[130] ^ in[131] ^ in[137] ^
		in[138] ^ in[143] ^ in[145] ^ in[149] ^ in[150] ^ in[152] ^ in[155] ^
		in[156] ^ in[160] ^ in[161] ^ in[163] ^ in[164] ^ in[165] ^ in[166] ^
		in[176] ^ in[177] ^ in[180] ^ in[182] ^ in[184] ^ in[185] ^ in[186] ^
		in[187] ^ in[188] ^ in[191] ^ in[192] ^ in[193] ^ in[195] ^ in[196] ^
		in[197] ^ in[201] ^ in[202] ^ in[203] ^ in[204] ^ in[206] ^ in[208] ^
		in[210] ^ in[218] ^ in[220] ^ in[221] ^ in[222] ^ in[224] ^ in[228] ^
		in[231] ^ in[237] ^ in[242] ^ in[246] ^ in[249] ^ in[251] ^ in[253] ^
		in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_27(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[5] ^ q[8] ^ q[14] ^ q[19] ^ q[23] ^ q[26] ^ q[28] ^ q[30] ^ in[1] ^
		in[4] ^ in[5] ^ in[7] ^ in[11] ^ in[19] ^ in[20] ^ in[21] ^ in[23] ^
		in[24] ^ in[25] ^ in[26] ^ in[27] ^ in[29] ^ in[32] ^ in[39] ^ in[40] ^
		in[42] ^ in[43] ^ in[45] ^ in[48] ^ in[49] ^ in[50] ^ in[53] ^ in[55] ^
		in[56] ^ in[58] ^ in[60] ^ in[61] ^ in[62] ^ in[63] ^ in[67] ^ in[68] ^
		in[74] ^ in[76] ^ in[77] ^ in[78] ^ in[79] ^ in[80] ^ in[82] ^ in[89] ^
		in[90] ^ in[91] ^ in[92] ^ in[93] ^ in[94] ^ in[96] ^ in[98] ^ in[99] ^
		in[101] ^ in[105] ^ in[106] ^ in[108] ^ in[109] ^ in[111] ^ in[112] ^
		in[113] ^ in[114] ^ in[118] ^ in[120] ^ in[121] ^ in[122] ^ in[123] ^
		in[127] ^ in[129] ^ in[130] ^ in[131] ^ in[132] ^ in[138] ^ in[139] ^
		in[144] ^ in[146] ^ in[150] ^ in[151] ^ in[153] ^ in[156] ^ in[157] ^
		in[161] ^ in[162] ^ in[164] ^ in[165] ^ in[166] ^ in[167] ^ in[177] ^
		in[178] ^ in[181] ^ in[183] ^ in[185] ^ in[186] ^ in[187] ^ in[188] ^
		in[189] ^ in[192] ^ in[193] ^ in[194] ^ in[196] ^ in[197] ^ in[198] ^
		in[202] ^ in[203] ^ in[204] ^ in[205] ^ in[207] ^ in[209] ^ in[211] ^
		in[219] ^ in[221] ^ in[222] ^ in[223] ^ in[225] ^ in[229] ^ in[232] ^
		in[238] ^ in[243] ^ in[247] ^ in[250] ^ in[252] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_28(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[2] ^ q[6] ^ q[9] ^ q[15] ^ q[20] ^ q[24] ^ q[27] ^ q[29] ^ q[31] ^
		in[2] ^ in[5] ^ in[6] ^ in[8] ^ in[12] ^ in[20] ^ in[21] ^ in[22] ^ in[24] ^
		in[25] ^ in[26] ^ in[27] ^ in[28] ^ in[30] ^ in[33] ^ in[40] ^ in[41] ^
		in[43] ^ in[44] ^ in[46] ^ in[49] ^ in[50] ^ in[51] ^ in[54] ^ in[56] ^
		in[57] ^ in[59] ^ in[61] ^ in[62] ^ in[63] ^ in[64] ^ in[68] ^ in[69] ^
		in[75] ^ in[77] ^ in[78] ^ in[79] ^ in[80] ^ in[81] ^ in[83] ^ in[90] ^
		in[91] ^ in[92] ^ in[93] ^ in[94] ^ in[95] ^ in[97] ^ in[99] ^ in[100] ^
		in[102] ^ in[106] ^ in[107] ^ in[109] ^ in[110] ^ in[112] ^ in[113] ^
		in[114] ^ in[115] ^ in[119] ^ in[121] ^ in[122] ^ in[123] ^ in[124] ^
		in[128] ^ in[130] ^ in[131] ^ in[132] ^ in[133] ^ in[139] ^ in[140] ^
		in[145] ^ in[147] ^ in[151] ^ in[152] ^ in[154] ^ in[157] ^ in[158] ^
		in[162] ^ in[163] ^ in[165] ^ in[166] ^ in[167] ^ in[168] ^ in[178] ^
		in[179] ^ in[182] ^ in[184] ^ in[186] ^ in[187] ^ in[188] ^ in[189] ^
		in[190] ^ in[193] ^ in[194] ^ in[195] ^ in[197] ^ in[198] ^ in[199] ^
		in[203] ^ in[204] ^ in[205] ^ in[206] ^ in[208] ^ in[210] ^ in[212] ^
		in[220] ^ in[222] ^ in[223] ^ in[224] ^ in[226] ^ in[230] ^ in[233] ^
		in[239] ^ in[244] ^ in[248] ^ in[251] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_29(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[3] ^ q[7] ^ q[10] ^ q[16] ^ q[21] ^ q[25] ^ q[28] ^ q[30] ^
		in[3] ^ in[6] ^ in[7] ^ in[9] ^ in[13] ^ in[21] ^ in[22] ^ in[23] ^ in[25] ^
		in[26] ^ in[27] ^ in[28] ^ in[29] ^ in[31] ^ in[34] ^ in[41] ^ in[42] ^
		in[44] ^ in[45] ^ in[47] ^ in[50] ^ in[51] ^ in[52] ^ in[55] ^ in[57] ^
		in[58] ^ in[60] ^ in[62] ^ in[63] ^ in[64] ^ in[65] ^ in[69] ^ in[70] ^
		in[76] ^ in[78] ^ in[79] ^ in[80] ^ in[81] ^ in[82] ^ in[84] ^ in[91] ^
		in[92] ^ in[93] ^ in[94] ^ in[95] ^ in[96] ^ in[98] ^ in[100] ^ in[101] ^
		in[103] ^ in[107] ^ in[108] ^ in[110] ^ in[111] ^ in[113] ^ in[114] ^
		in[115] ^ in[116] ^ in[120] ^ in[122] ^ in[123] ^ in[124] ^ in[125] ^
		in[129] ^ in[131] ^ in[132] ^ in[133] ^ in[134] ^ in[140] ^ in[141] ^
		in[146] ^ in[148] ^ in[152] ^ in[153] ^ in[155] ^ in[158] ^ in[159] ^
		in[163] ^ in[164] ^ in[166] ^ in[167] ^ in[168] ^ in[169] ^ in[179] ^
		in[180] ^ in[183] ^ in[185] ^ in[187] ^ in[188] ^ in[189] ^ in[190] ^
		in[191] ^ in[194] ^ in[195] ^ in[196] ^ in[198] ^ in[199] ^ in[200] ^
		in[204] ^ in[205] ^ in[206] ^ in[207] ^ in[209] ^ in[211] ^ in[213] ^
		in[221] ^ in[223] ^ in[224] ^ in[225] ^ in[227] ^ in[231] ^ in[234] ^
		in[240] ^ in[245] ^ in[249] ^ in[252] ^ in[254];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_30(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[0] ^ q[1] ^ q[2] ^ q[4] ^ q[8] ^ q[11] ^ q[17] ^ q[22] ^ q[26] ^ q[29] ^
		q[31] ^ in[4] ^ in[7] ^ in[8] ^ in[10] ^ in[14] ^ in[22] ^ in[23] ^ in[24] ^
		in[26] ^ in[27] ^ in[28] ^ in[29] ^ in[30] ^ in[32] ^ in[35] ^ in[42] ^
		in[43] ^ in[45] ^ in[46] ^ in[48] ^ in[51] ^ in[52] ^ in[53] ^ in[56] ^
		in[58] ^ in[59] ^ in[61] ^ in[63] ^ in[64] ^ in[65] ^ in[66] ^ in[70] ^
		in[71] ^ in[77] ^ in[79] ^ in[80] ^ in[81] ^ in[82] ^ in[83] ^ in[85] ^
		in[92] ^ in[93] ^ in[94] ^ in[95] ^ in[96] ^ in[97] ^ in[99] ^ in[101] ^
		in[102] ^ in[104] ^ in[108] ^ in[109] ^ in[111] ^ in[112] ^ in[114] ^
		in[115] ^ in[116] ^ in[117] ^ in[121] ^ in[123] ^ in[124] ^ in[125] ^
		in[126] ^ in[130] ^ in[132] ^ in[133] ^ in[134] ^ in[135] ^ in[141] ^
		in[142] ^ in[147] ^ in[149] ^ in[153] ^ in[154] ^ in[156] ^ in[159] ^
		in[160] ^ in[164] ^ in[165] ^ in[167] ^ in[168] ^ in[169] ^ in[170] ^
		in[180] ^ in[181] ^ in[184] ^ in[186] ^ in[188] ^ in[189] ^ in[190] ^
		in[191] ^ in[192] ^ in[195] ^ in[196] ^ in[197] ^ in[199] ^ in[200] ^
		in[201] ^ in[205] ^ in[206] ^ in[207] ^ in[208] ^ in[210] ^ in[212] ^
		in[214] ^ in[222] ^ in[224] ^ in[225] ^ in[226] ^ in[228] ^ in[232] ^
		in[235] ^ in[241] ^ in[246] ^ in[250] ^ in[253] ^ in[255];

	return val;
}

static noinline u8 yk3_ppp_eme_crc32_31(u8 *q, u8 *in)
{
	u8 val = 0;

	val = q[1] ^ q[2] ^ q[3] ^ q[5] ^ q[9] ^ q[12] ^ q[18] ^ q[23] ^ q[27] ^ q[30] ^
		in[5] ^ in[8] ^ in[9] ^ in[11] ^ in[15] ^ in[23] ^ in[24] ^ in[25] ^
		in[27] ^ in[28] ^ in[29] ^ in[30] ^ in[31] ^ in[33] ^ in[36] ^ in[43] ^
		in[44] ^ in[46] ^ in[47] ^ in[49] ^ in[52] ^ in[53] ^ in[54] ^ in[57] ^
		in[59] ^ in[60] ^ in[62] ^ in[64] ^ in[65] ^ in[66] ^ in[67] ^ in[71] ^
		in[72] ^ in[78] ^ in[80] ^ in[81] ^ in[82] ^ in[83] ^ in[84] ^ in[86] ^
		in[93] ^ in[94] ^ in[95] ^ in[96] ^ in[97] ^ in[98] ^ in[100] ^ in[102] ^
		in[103] ^ in[105] ^ in[109] ^ in[110] ^ in[112] ^ in[113] ^ in[115] ^
		in[116] ^ in[117] ^ in[118] ^ in[122] ^ in[124] ^ in[125] ^ in[126] ^
		in[127] ^ in[131] ^ in[133] ^ in[134] ^ in[135] ^ in[136] ^ in[142] ^
		in[143] ^ in[148] ^ in[150] ^ in[154] ^ in[155] ^ in[157] ^ in[160] ^
		in[161] ^ in[165] ^ in[166] ^ in[168] ^ in[169] ^ in[170] ^ in[171] ^
		in[181] ^ in[182] ^ in[185] ^ in[187] ^ in[189] ^ in[190] ^ in[191] ^
		in[192] ^ in[193] ^ in[196] ^ in[197] ^ in[198] ^ in[200] ^ in[201] ^
		in[202] ^ in[206] ^ in[207] ^ in[208] ^ in[209] ^ in[211] ^ in[213] ^
		in[215] ^ in[223] ^ in[225] ^ in[226] ^ in[227] ^ in[229] ^ in[233] ^
		in[236] ^ in[242] ^ in[247] ^ in[251] ^ in[254];

	return val;
}

static u32 yk3_ppp_eme_crc32_256bit(const u8 *key, u32 seed)
{
	u32 crc = 0;
	u8 c[32] = { 0 };
	u8 q[32] = { 0 };
	u8 in[256] = { 0 };
	int i, j;

	for (i = 0; i < 32; i++)
		q[i] = (seed >> i) & 0x1;

	for (i = 0; i < 32; i++)
		for (j = 0; j < 8; j++)
			in[i * 8 + j] = (key[i] >> j) & 0x1;

	c[0] = yk3_ppp_eme_crc32_0(q, in);
	c[1] = yk3_ppp_eme_crc32_1(q, in);
	c[2] = yk3_ppp_eme_crc32_2(q, in);
	c[3] = yk3_ppp_eme_crc32_3(q, in);
	c[4] = yk3_ppp_eme_crc32_4(q, in);
	c[5] = yk3_ppp_eme_crc32_5(q, in);
	c[6] = yk3_ppp_eme_crc32_6(q, in);
	c[7] = yk3_ppp_eme_crc32_7(q, in);
	c[8] = yk3_ppp_eme_crc32_8(q, in);
	c[9] = yk3_ppp_eme_crc32_9(q, in);
	c[10] = yk3_ppp_eme_crc32_10(q, in);
	c[11] = yk3_ppp_eme_crc32_11(q, in);
	c[12] = yk3_ppp_eme_crc32_12(q, in);
	c[13] = yk3_ppp_eme_crc32_13(q, in);
	c[14] = yk3_ppp_eme_crc32_14(q, in);
	c[15] = yk3_ppp_eme_crc32_15(q, in);
	c[16] = yk3_ppp_eme_crc32_16(q, in);
	c[17] = yk3_ppp_eme_crc32_17(q, in);
	c[18] = yk3_ppp_eme_crc32_18(q, in);
	c[19] = yk3_ppp_eme_crc32_19(q, in);
	c[20] = yk3_ppp_eme_crc32_20(q, in);
	c[21] = yk3_ppp_eme_crc32_21(q, in);
	c[22] = yk3_ppp_eme_crc32_22(q, in);
	c[23] = yk3_ppp_eme_crc32_23(q, in);
	c[24] = yk3_ppp_eme_crc32_24(q, in);
	c[25] = yk3_ppp_eme_crc32_25(q, in);
	c[26] = yk3_ppp_eme_crc32_26(q, in);
	c[27] = yk3_ppp_eme_crc32_27(q, in);
	c[28] = yk3_ppp_eme_crc32_28(q, in);
	c[29] = yk3_ppp_eme_crc32_29(q, in);
	c[30] = yk3_ppp_eme_crc32_30(q, in);
	c[31] = yk3_ppp_eme_crc32_31(q, in);

	for (i = 0; i < 32; i++)
		crc |= c[i] << i;

	return crc;
}

static u32 yk3_ppp_eme_hash(const u8 *key, u8 key_type, u32 seed, u32 mux_seed)
{
	u64 crc_total;
	u64 crc_high;
	u64 crc_low;
	u64 crc_out;
	u64 crc32;
	u32 pos;
	u8 hash_key[YK3_PPP_EME_KEY_SIZE] = { 0 };
	int i;

	switch (key_type) {
	case YK3_PPP_EME_KEY_TYPE_SHARE:
		for (i = 0; i < (32 - YK3_PPP_EME_KEY_SIZE_SHARE); i++)
			hash_key[i] = 0;
		for (i = 0; i < YK3_PPP_EME_KEY_SIZE_SHARE; i++)
			hash_key[i + 32 - YK3_PPP_EME_KEY_SIZE_SHARE] = *(key + i);
		break;
	case YK3_PPP_EME_KEY_TYPE_EXCLUSIVE:
		for (i = 0; i < (32 - YK3_PPP_EME_KEY_SIZE_EXCLUSIVE); i++)
			hash_key[i] = 0;
		for (i = 0; i < YK3_PPP_EME_KEY_SIZE_EXCLUSIVE; i++)
			hash_key[i + 32 - YK3_PPP_EME_KEY_SIZE_EXCLUSIVE] = (*(key + i));
		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
		for (i = 0; i < 32; i++)
			hash_key[i] = (*(key + i + YK3_PPP_EME_KEY_SIZE_MULTI - 32));
		for (i = 0; i < (YK3_PPP_EME_KEY_SIZE - YK3_PPP_EME_KEY_SIZE_MULTI); i++)
			hash_key[i + 32] = 0;
		for (i = 0; i < (YK3_PPP_EME_KEY_SIZE_MULTI - 32); i++)
			hash_key[i + 32 + YK3_PPP_EME_KEY_SIZE - YK3_PPP_EME_KEY_SIZE_MULTI] =
			(*(key + i));
		break;
	default:
		return -1;
	}

	crc32 = yk3_ppp_eme_crc32_256bit(hash_key, seed);
	if (key_type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD)
		crc32 = yk3_ppp_eme_crc32_256bit(hash_key + 32, crc32);

	crc_total = (crc32 << 32) >> mux_seed;
	crc_high = (crc_total & 0xFFFFFFFF00000000) >> 32;
	crc_low = crc_total & 0xFFFFFFFF;

	crc_out = crc_high | crc_low;
	pos = (int)(crc_out % YK3_PPP_EME_DEPTH);

	return pos;
}

static u8 yk3_ppp_eme_get_key_type(u32 key_len)
{
	u8 type;

	switch (key_len) {
	case YK3_PPP_EME_KEY_SIZE_SHARE:
		type = YK3_PPP_EME_KEY_TYPE_SHARE;
		break;
	case YK3_PPP_EME_KEY_SIZE_EXCLUSIVE:
		type = YK3_PPP_EME_KEY_TYPE_EXCLUSIVE;
		break;
	case YK3_PPP_EME_KEY_SIZE_MULTI:
		type = YK3_PPP_EME_KEY_TYPE_MULTI_HEAD;
		break;
	default:
		type = YK3_PPP_EME_KEY_TYPE_MULTI_TAIL;
	}

	return type;
}

static void yk3_ppp_eme_array_to_hexstr(const u8 *arr, u32 len, char *hex_str)
{
	int i;

	for (i = 0; i < len; i++)
		sprintf(hex_str + i * 2, "%02x", arr[len - 1 - i]);

	hex_str[len * 2] = '\0';
}

// key: with hdr
static int yk3_ppp_eme_search(struct yk3_ppp_eme *eme, u32 table_id, const u8 *key,
			      u8 *value, u32 *bucket, u32 *pos)
{
	int found = 0;
	int i;
	int j;
	struct yk3_ppp_eme_table *table = &eme->tables[table_id];
	u8 key_type = table->key_type;
	u32 key_size = table->key_size;
	u32 value_size = table->value_size;
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1] = {0};

	if (!eme || !key || !value)
		return found;

	yk3_ppp_eme_array_to_hexstr(key, key_size, key_str);
	for (i = 0; i < eme->bucket_count; i++) {
		j = yk3_ppp_eme_hash(key, key_type, eme->seed[i], eme->mux_seed[i]);
		yk3_debug("eme-%d: Position of key %s in bucket %d is %d.\n",
			  eme->hw_id, key_str, i, j);
		if (eme->buckets[i][j].is_occupied &&
		    key_type == eme->buckets[i][j].type &&
		    !memcmp(key, eme->buckets[i][j].key, key_size)) {
			if (found == 1) {
				yk3_err("eme-%d: Found duplicate key %s in buckets.\n",
					eme->hw_id, key_str);
				yk3_err("eme-%d: Last pos in bucket %d pos %d.\n",
					eme->hw_id, *bucket, *pos);
				yk3_err("eme-%d: Duplicate %s in bucket %d pos %d.\n",
					eme->hw_id, key_str, i, j);
			}
			memcpy(value, eme->buckets[i][j].value, value_size);
			*bucket = i;
			*pos = j;
			found = 1;
		} else if ((eme->buckets[i][j].is_occupied == 2) &&
			   (key_type == eme->buckets[i][j].type) &&
			   !memcmp(key, eme->buckets[i][j].secondary_key, key_size)) {
			if (found == 1) {
				yk3_err("eme-%d: Found duplicate key %s in buckets.\n",
					eme->hw_id, key_str);
				yk3_err("eme-%d: Last pos in bucket %d pos %d.\n",
					eme->hw_id, *bucket, *pos);
				yk3_err("eme-%d: Duplicate %s in bucket %d pos %d.\n",
					eme->hw_id, key_str, i, j);
			}
			memcpy(value, eme->buckets[i][j].secondary_value, table->value_size);
			*bucket = i;
			*pos = j;
			found = 1;
		}
	}

	return found;
}

static int yk3_ppp_eme_kick_push(struct yk3_ppp_eme_kick_stream *stream,
				 struct yk3_ppp_eme_kick kick)
{
	int i;

	for (i = 0; i < stream->count; i++) {
		if (stream->kicks[i].to_bucket == kick.to_bucket &&
		    stream->kicks[i].to_pos == kick.to_pos) {
			yk3_err("eme: dup dst in kick stream, bucket %u, pos %u",
				kick.to_bucket, kick.to_pos);
			return -EEXIST;
		}
	}

	if (stream->count < YK3_PPP_EME_MAX_INSERT_RETRIES) {
		stream->kicks[stream->count] = kick;
		stream->count++;
	}

	return 0;
}

static int yk3_ppp_eme_find_free_position(struct yk3_ppp_eme *eme, u32 bucket, int current_pos)
{
	int free_pos = -1;
	int j;

	for (j = 1; j <= 4; j++) {
		if (eme->buckets[bucket][(j + current_pos) % eme->depth].is_occupied == 0) {
			free_pos = ((j + current_pos) % (eme->depth));
			break;
		}
	}

	return free_pos;
}

/* Helper function: save original entry for rollback */
static inline void yk3_ppp_eme_save_original(struct yk3_ppp_eme *eme, u32 bucket, u32 pos)
{
	eme->kick_ws.ori_kicks[eme->kick_ws.ori_kcnt].entry = eme->buckets[bucket][pos];
	eme->kick_ws.ori_kicks[eme->kick_ws.ori_kcnt].to_bucket = bucket;
	eme->kick_ws.ori_kicks[eme->kick_ws.ori_kcnt].to_pos = pos;
	eme->kick_ws.ori_kcnt++;
}

/* Helper function: rollback all kicks on error */
static void yk3_ppp_eme_rollback_kicks(struct yk3_ppp_eme *eme, u32 to_bucket, u32 entry_num)
{
	int i;

	for (i = 0; i < eme->kick_ws.ori_kcnt; i++)
		eme->buckets[to_bucket][eme->kick_ws.ori_kicks[i].to_pos] =
			eme->kick_ws.ori_kicks[i].entry;
	eme->buckets_entry_num[to_bucket] = entry_num;
}

/* Helper function: handle SHARE type with occupied=1 */
static int yk3_ppp_eme_handle_share_occupied1(struct yk3_ppp_eme *eme,
					      struct yk3_ppp_eme_entry entry,
					      u32 to_bucket, u32 to_pos,
					      u32 from_bucket, u32 from_pos,
					      struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;

	eme->kick_ws.kick.entry = entry;
	memcpy(eme->kick_ws.kick.entry.secondary_key, eme->kick_ws.kicked_entry.key,
	       YK3_PPP_EME_KEY_SIZE);
	memcpy(eme->kick_ws.kick.entry.secondary_value, eme->kick_ws.kicked_entry.value,
	       YK3_PPP_EME_VALUE_SIZE);
	eme->kick_ws.kick.entry.is_occupied = 2;
	eme->kick_ws.kick.from_bucket = YK3_PPP_EME_ADDED_RULE;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	return 0;
}

/* Helper function: handle SHARE type with occupied=2 */
static int yk3_ppp_eme_handle_share_occupied2(struct yk3_ppp_eme *eme,
					      struct yk3_ppp_eme_entry entry,
					      u32 to_bucket, u32 to_pos,
					      u32 from_bucket, u32 from_pos,
					      struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;

	eme->kick_ws.kick.entry = entry;
	memcpy(eme->kick_ws.kick.entry.secondary_key, eme->kick_ws.kicked_entry.key,
	       YK3_PPP_EME_KEY_SIZE);
	memcpy(eme->kick_ws.kick.entry.secondary_value, eme->kick_ws.kicked_entry.value,
	       YK3_PPP_EME_VALUE_SIZE);
	eme->kick_ws.kick.entry.is_occupied = 2;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	memcpy(eme->kick_ws.kicked_entry.key, eme->kick_ws.kicked_entry.secondary_key,
	       YK3_PPP_EME_KEY_SIZE);
	memcpy(eme->kick_ws.kicked_entry.value, eme->kick_ws.kicked_entry.secondary_value,
	       YK3_PPP_EME_VALUE_SIZE);
	memset(eme->kick_ws.kicked_entry.secondary_key, 0, YK3_PPP_EME_KEY_SIZE);
	memset(eme->kick_ws.kicked_entry.secondary_value, 0, YK3_PPP_EME_VALUE_SIZE);
	eme->kick_ws.kicked_entry.is_occupied = 1;

	return 0;
}

/* Helper function: handle SHARE type with EXCLUSIVE kicked entry */
static int yk3_ppp_eme_handle_share_exclusive(struct yk3_ppp_eme *eme,
					      struct yk3_ppp_eme_entry entry,
					      u32 to_bucket, u32 to_pos,
					      u32 from_bucket, u32 from_pos,
					      struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;

	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	return 0;
}

/* Helper function: handle SHARE type with MULTI_HEAD kicked entry */
static int yk3_ppp_eme_handle_share_multi_head(struct yk3_ppp_eme *eme,
					       struct yk3_ppp_eme_entry entry,
					       u32 to_bucket, u32 to_pos,
					       u32 from_bucket, u32 from_pos,
					       struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;

	/* Discard next entry */
	eme->kick_ws.kick.entry =
		eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index];
	eme->kick_ws.kick.entry.is_occupied = 0;
	memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
	memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
	eme->kick_ws.kick.entry.type = 0;
	eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = eme->kick_ws.kicked_entry.next_index;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, eme->kick_ws.kicked_entry.next_index);
	eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index] = eme->kick_ws.kick.entry;
	eme->buckets_entry_num[to_bucket]--;

	/* Insert new entry */
	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	return 0;
}

/* Helper function: handle SHARE type with MULTI_TAIL kicked entry */
static int yk3_ppp_eme_handle_share_multi_tail(struct yk3_ppp_eme *eme,
					       struct yk3_ppp_eme_entry entry,
					       u32 to_bucket, u32 to_pos,
					       u32 from_bucket, u32 from_pos,
					       struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;
	int pre_idx;

	eme->kick_ws.kicked_entry =
		eme->buckets[to_bucket][eme->kick_ws.kicked_entry.pre_index];

	/* Insert new entry */
	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	pre_idx = eme->buckets[to_bucket][to_pos].pre_index;
	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	/* Discard pre entry */
	eme->kick_ws.kick.entry = eme->buckets[to_bucket][pre_idx];
	eme->kick_ws.kick.entry.is_occupied = 0;
	memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
	memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
	eme->kick_ws.kick.entry.type = 0;
	eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = pre_idx;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, eme->kick_ws.kick.to_pos);
	eme->buckets[to_bucket][eme->kick_ws.kick.to_pos] = eme->kick_ws.kick.entry;
	eme->buckets_entry_num[to_bucket]--;

	return 0;
}

/* Handle all SHARE type cases */
static int yk3_ppp_eme_handle_share_type(struct yk3_ppp_eme *eme,
					 struct yk3_ppp_eme_entry entry,
					 u32 to_bucket, u32 to_pos,
					 u32 from_bucket, u32 from_pos,
					 struct yk3_ppp_eme_kick_stream *stream)
{
	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_SHARE &&
	    eme->kick_ws.kicked_entry.is_occupied == 1) {
		return yk3_ppp_eme_handle_share_occupied1(eme, entry, to_bucket, to_pos,
							  from_bucket, from_pos, stream);
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_SHARE &&
	    eme->kick_ws.kicked_entry.is_occupied == 2) {
		return yk3_ppp_eme_handle_share_occupied2(eme, entry, to_bucket, to_pos,
							  from_bucket, from_pos, stream);
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_EXCLUSIVE) {
		return yk3_ppp_eme_handle_share_exclusive(eme, entry, to_bucket, to_pos,
							  from_bucket, from_pos, stream);
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		return yk3_ppp_eme_handle_share_multi_head(eme, entry, to_bucket, to_pos,
							   from_bucket, from_pos, stream);
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_TAIL) {
		return yk3_ppp_eme_handle_share_multi_tail(eme, entry, to_bucket, to_pos,
							   from_bucket, from_pos, stream);
	}

	return 0;
}

/* Handle EXCLUSIVE type cases */
static int yk3_ppp_eme_handle_exclusive_type(struct yk3_ppp_eme *eme,
					     struct yk3_ppp_eme_entry entry,
					     u32 to_bucket, u32 to_pos,
					     u32 from_bucket, u32 from_pos,
					     struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		/* Discard next entry */
		eme->kick_ws.kick.entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index];
		eme->kick_ws.kick.entry.is_occupied = 0;
		memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.entry.type = 0;
		eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = eme->kick_ws.kicked_entry.next_index;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket,
					  eme->kick_ws.kicked_entry.next_index);
		eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index] =
			eme->kick_ws.kick.entry;
		eme->buckets_entry_num[to_bucket]--;

		/* Insert new entry */
		eme->kick_ws.kick.entry = entry;
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = to_pos;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
		eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

		return 0;
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_TAIL) {
		int pre_idx;

		eme->kick_ws.kicked_entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.pre_index];

		/* Insert new entry */
		eme->kick_ws.kick.entry = entry;
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = to_pos;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		pre_idx = eme->buckets[to_bucket][to_pos].pre_index;
		yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
		eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

		/* Discard pre entry */
		eme->kick_ws.kick.entry = eme->buckets[to_bucket][pre_idx];
		eme->kick_ws.kick.entry.is_occupied = 0;
		memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.entry.type = 0;
		eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = pre_idx;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket, eme->kick_ws.kick.to_pos);
		eme->buckets[to_bucket][eme->kick_ws.kick.to_pos] = eme->kick_ws.kick.entry;
		eme->buckets_entry_num[to_bucket]--;

		return 0;
	}

	/* Default case */
	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	return 0;
}

/* Handle MULTI_HEAD type cases */
static int yk3_ppp_eme_handle_multi_head_type(struct yk3_ppp_eme *eme,
					      struct yk3_ppp_eme_entry entry,
					      u32 to_bucket, u32 to_pos,
					      u32 from_bucket, u32 from_pos,
					      struct yk3_ppp_eme_kick_stream *stream)
{
	int ret;
	int free_pos;
	int j;

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		/* Update next entry */
		eme->kick_ws.kick.entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index];
		memcpy(eme->kick_ws.kick.entry.key, entry.key, YK3_PPP_EME_KEY_SIZE);
		memcpy(eme->kick_ws.kick.entry.value, entry.value, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = eme->kick_ws.kicked_entry.next_index;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket,
					  eme->kick_ws.kicked_entry.next_index);
		eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index] =
			eme->kick_ws.kick.entry;

		/* Update head entry */
		eme->kick_ws.kick.entry = eme->buckets[to_bucket][to_pos];
		memcpy(eme->kick_ws.kick.entry.key, entry.key, YK3_PPP_EME_KEY_SIZE);
		memcpy(eme->kick_ws.kick.entry.value, entry.value, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = to_pos;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
		eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

		return 0;
	}

	free_pos = yk3_ppp_eme_find_free_position(eme, to_bucket, to_pos);

	if (!eme->kick_ws.kicked_entry.is_occupied) {
		if (free_pos != -1)
			return -1;
		for (j = 1; j <= 4; j++) {
			if (eme->buckets[to_bucket][to_pos + j].type !=
			    YK3_PPP_EME_KEY_TYPE_MULTI_HEAD &&
			    eme->buckets[to_bucket][to_pos + j].type !=
			    YK3_PPP_EME_KEY_TYPE_MULTI_TAIL) {
				free_pos = to_pos + j;
				eme->kick_ws.kicked_entry =
					eme->buckets[to_bucket][free_pos];
				break;
			}
		}
		if (free_pos == -1) {
			free_pos = to_pos + 1;
			eme->kick_ws.kicked_entry = eme->buckets[to_bucket][free_pos];
		}
	}

	if (free_pos == -1)
		return -1;

	/* Insert head entry */
	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.entry.pre_index = -1;
	eme->kick_ws.kick.entry.next_index = free_pos;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = to_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, to_pos);
	eme->buckets[to_bucket][to_pos] = eme->kick_ws.kick.entry;

	/* Insert tail entry */
	eme->kick_ws.kick.entry = entry;
	eme->kick_ws.kick.entry.pre_index = to_pos;
	eme->kick_ws.kick.entry.next_index = -1;
	eme->kick_ws.kick.entry.type = YK3_PPP_EME_KEY_TYPE_MULTI_TAIL;
	eme->kick_ws.kick.from_bucket = from_bucket;
	eme->kick_ws.kick.from_pos = from_pos;
	eme->kick_ws.kick.to_bucket = to_bucket;
	eme->kick_ws.kick.to_pos = free_pos;

	ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	if (ret)
		return ret;

	yk3_ppp_eme_save_original(eme, to_bucket, free_pos);
	eme->buckets[to_bucket][free_pos] = eme->kick_ws.kick.entry;
	eme->buckets_entry_num[to_bucket]++;

	/* Handle kicked entry */
	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		eme->kick_ws.kick.entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index];
		eme->kick_ws.kick.entry.is_occupied = 0;
		memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.entry.type = 0;
		eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = eme->kick_ws.kicked_entry.next_index;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket,
					  eme->kick_ws.kicked_entry.next_index);
		eme->buckets[to_bucket][eme->kick_ws.kicked_entry.next_index] =
			eme->kick_ws.kicked_entry;
		eme->buckets_entry_num[to_bucket]--;
	}

	if (eme->kick_ws.kicked_entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_TAIL) {
		eme->kick_ws.kick.entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.pre_index];
		eme->kick_ws.kick.entry.is_occupied = 0;
		memset(eme->kick_ws.kick.entry.key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->kick_ws.kick.entry.value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->kick_ws.kick.entry.type = 0;
		eme->kick_ws.kick.from_bucket = YK3_PPP_EME_DISCARD_RULE;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = eme->kick_ws.kicked_entry.pre_index;

		ret = yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
		if (ret)
			return ret;

		yk3_ppp_eme_save_original(eme, to_bucket,
					  eme->kick_ws.kicked_entry.pre_index);
		eme->kick_ws.kicked_entry =
			eme->buckets[to_bucket][eme->kick_ws.kicked_entry.pre_index];
		eme->buckets[to_bucket][eme->kick_ws.kick.to_pos] = eme->kick_ws.kick.entry;
		eme->buckets_entry_num[to_bucket]--;
	}

	return 0;
}

static int yk3_ppp_eme_kick(struct yk3_ppp_eme *eme, struct yk3_ppp_eme_entry entry,
			    u32 to_bucket, struct yk3_ppp_eme_kick_stream *stream)
{
	u32 to_pos;
	u32 from_bucket;
	u32 from_pos;
	int ret = 0;
	int free_pos;
	int entry_num;
	int need_kick = 0;
	int i;

	if (stream->count >= YK3_PPP_EME_MAX_INSERT_RETRIES) {
		yk3_warn("eme-%d: stream kick count overhead.\n", eme->hw_id);
		return -ENOSPC;
	}

	if (entry.is_occupied == 2) {
		yk3_warn("eme-%d: can not kick two entry at once.\n", eme->hw_id);
		return -EOPNOTSUPP;
	}

	to_pos = yk3_ppp_eme_hash(entry.key, entry.type, eme->seed[to_bucket],
				  eme->mux_seed[to_bucket]);
	if (stream->count == 0) {
		from_bucket = YK3_PPP_EME_NEW_RULE;
		from_pos = 0;
	} else {
		from_bucket = stream->kicks[stream->count - 1].to_bucket;
		from_pos = stream->kicks[stream->count - 1].to_pos;
	}

	/* Initialize workspace */
	eme->kick_ws.ori_kcnt = 0;

	if (!eme->buckets[to_bucket][to_pos].is_occupied &&
	    entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		free_pos = yk3_ppp_eme_find_free_position(eme, to_bucket, to_pos);
		if (free_pos == -1)
			need_kick = 1;
	} else if (eme->buckets[to_bucket][to_pos].is_occupied) {
		need_kick = 1;
	}

	if (need_kick) {
		eme->kick_ws.kicked_entry = eme->buckets[to_bucket][to_pos];
		if (eme->kick_ws.kicked_entry.is_occupied == 2 &&
		    entry.type != YK3_PPP_EME_KEY_TYPE_SHARE) {
			yk3_warn("eme-%d: can not kick two entry at one", eme->hw_id);
			return -EOPNOTSUPP;
		}
		entry_num = eme->buckets_entry_num[to_bucket];

		switch (entry.type) {
		case YK3_PPP_EME_KEY_TYPE_SHARE:
			ret = yk3_ppp_eme_handle_share_type(eme, entry, to_bucket,
							    to_pos, from_bucket,
							    from_pos, stream);
			break;
		case YK3_PPP_EME_KEY_TYPE_EXCLUSIVE:
			ret = yk3_ppp_eme_handle_exclusive_type(eme, entry, to_bucket,
								to_pos, from_bucket,
								from_pos, stream);
			break;
		case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
			ret = yk3_ppp_eme_handle_multi_head_type(eme, entry, to_bucket,
								 to_pos, from_bucket,
								 from_pos, stream);
			break;
		default:
			yk3_err("eme-%d: unrecognized key type %d.\n", eme->hw_id, entry.type);
			return -EINVAL;
		}

		if (!ret) {
			for (i = 1; i < eme->bucket_count; i++) {
				ret = yk3_ppp_eme_kick(eme, eme->kick_ws.kicked_entry,
						       ((i + to_bucket) % eme->bucket_count),
						       stream);
				if (ret == 0)
					break;
			}
		}

		if (ret)
			yk3_ppp_eme_rollback_kicks(eme, to_bucket, entry_num);

		return ret;
	}

	/* Insert new rule */
	if (entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
		free_pos = yk3_ppp_eme_find_free_position(eme, to_bucket, to_pos);
		if (free_pos == -1)
			return -ENOSPC;

		entry.pre_index = -1;
		entry.next_index = free_pos;
		eme->kick_ws.kick.entry = entry;
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = to_pos;
		yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);

		eme->buckets[to_bucket][free_pos] = entry;
		eme->buckets[to_bucket][free_pos].pre_index = to_pos;
		eme->buckets[to_bucket][free_pos].next_index = -1;
		eme->buckets[to_bucket][free_pos].type = YK3_PPP_EME_KEY_TYPE_MULTI_TAIL;
		eme->buckets_entry_num[to_bucket]++;

		eme->kick_ws.kick.entry = eme->buckets[to_bucket][free_pos];
		eme->kick_ws.kick.from_bucket = YK3_PPP_EME_NEW_RULE;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = free_pos;
		yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	} else {
		eme->kick_ws.kick.entry = entry;
		eme->kick_ws.kick.from_bucket = from_bucket;
		eme->kick_ws.kick.from_pos = from_pos;
		eme->kick_ws.kick.to_bucket = to_bucket;
		eme->kick_ws.kick.to_pos = to_pos;
		yk3_ppp_eme_kick_push(stream, eme->kick_ws.kick);
	}

	eme->buckets[to_bucket][to_pos] = entry;
	eme->buckets[to_bucket][to_pos].is_occupied = 1;
	eme->buckets_entry_num[to_bucket]++;

	return 0;
}

static int yk3_ppp_eme_try_insert(struct yk3_ppp_eme *eme, u32 table_id, u8 *key, u8 *value,
				  struct yk3_ppp_eme_kick_stream *stream)
{
	struct yk3_ppp_eme_table *table = &eme->tables[table_id];
	u32 key_size = table->key_size;
	u32 value_size = table->value_size;
	u8 key_type = table->key_type;
	u8 value_tmp[YK3_PPP_EME_VALUE_SIZE];
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1];
	char val_str[YK3_PPP_EME_VALUE_SIZE * 2 + 1];
	u32 bucket;
	u32 pos;
	u32 to_pos;
	int ret = -1;
	struct yk3_ppp_eme_entry entry;
	struct yk3_ppp_eme_kick kick;
	int free_pos;
	int i;

	memset(&entry, 0, sizeof(entry));
	entry.is_occupied = 1;
	memcpy(entry.key, key, key_size);
	memcpy(entry.value, value, value_size);
	entry.type = key_type;

	yk3_ppp_eme_array_to_hexstr(key, key_size, key_str);
	yk3_ppp_eme_array_to_hexstr(value, value_size, val_str);
	yk3_info("eme-%d: table %u prepare to intert key %s with val %s.\n",
		 eme->hw_id, table_id, key_str, val_str);
	if (yk3_ppp_eme_search(eme, table_id, key, value_tmp, &bucket, &pos)) {
		yk3_err("eme-%d: key %s has already been inserted.\n", eme->hw_id, key_str);
		return -EEXIST;
	}

	stream->count = 0;

	for (i = 0; i < eme->bucket_count; i++) {
		to_pos = yk3_ppp_eme_hash(key, key_type, eme->seed[i], eme->mux_seed[i]);
		if (eme->buckets[i][to_pos].is_occupied == 0) {
			kick.entry = entry;
			kick.entry.is_occupied = 1;

			if (entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
				free_pos = yk3_ppp_eme_find_free_position(eme, i, to_pos);
				if (free_pos == -1)
					continue;
				kick.entry.pre_index = -1;
				kick.entry.next_index = free_pos;
			}

			kick.from_bucket = YK3_PPP_EME_NEW_RULE;
			kick.from_pos = 0;
			kick.to_bucket = i;
			kick.to_pos = to_pos;
			yk3_ppp_eme_kick_push(stream, kick);

			eme->buckets[i][to_pos] = kick.entry;
			eme->buckets_entry_num[i]++;

			if (entry.type == YK3_PPP_EME_KEY_TYPE_MULTI_HEAD) {
				kick.entry = entry;
				kick.entry.is_occupied = 1;
				kick.entry.type = YK3_PPP_EME_KEY_TYPE_MULTI_TAIL;
				kick.entry.pre_index = to_pos;
				kick.entry.next_index = -1;
				kick.from_bucket = YK3_PPP_EME_NEW_RULE;
				kick.from_pos = 0;
				kick.to_bucket = i;
				kick.to_pos = free_pos;
				ret = yk3_ppp_eme_kick_push(stream, kick);
				eme->buckets[i][free_pos] = kick.entry;
				eme->buckets_entry_num[i]++;
			}

			return 0;
		}
	}

	for (i = 0; i < eme->bucket_count; i++) {
		stream->count = 0;
		ret = yk3_ppp_eme_kick(eme, entry, i, stream);
		if (ret == 0)
			break;
	}

	if (ret) {
		yk3_err("eme-%d: cannot insert key %s, kick failed.\n", eme->hw_id, key_str);
		return ret;
	}

	yk3_debug("eme-%d: get pos for key %s success.\n", eme->hw_id, key_str);

	return 0;
}

static void yk3_ppp_eme_generate_rule(const u8 *key, const u8 *value, u32 *data, int len)
{
	u16 high;
	u16 low;
	int i;

	for (i = 0; i < len; i++) {
		if (i == 0) {
			high = *(u16 *)&key[0];
			low = *(u16 *)&value[0];
			data[i] = ((u32)high << 16) | low;
		} else {
			data[i] = *(u32 *)(key + 4 * i - 2);
		}
	}
}

static u32 yk3_ppp_eme_get_ram_addr(u32 bucket, u32 pos)
{
	return (((bucket & 0x3) << 8) | (pos & 0xFF));
}

static int yk3_ppp_eme_write_rule(struct yk3_ppp_eme *eme, u8 bucket, u32 pos)
{
	u32 data[YK3_PPP_EME_DATA_ROUND] = {0};
	u8 value[YK3_PPP_EME_VALUE_SIZE] = {0};
	u32 ram_addr;
	int i;
	int pointer = 0;

	ram_addr = yk3_ppp_eme_get_ram_addr(bucket, pos);

	switch (eme->buckets[bucket][pos].type) {
	case YK3_PPP_EME_KEY_TYPE_SHARE:
		memcpy(value, eme->buckets[bucket][pos].value, YK3_PPP_EME_VALUE_SIZE);
		(*(value + 1)) = (((*(value + 1)) & ~YK3_PPP_EME_VALUE_TYPE_MASK) |
				  eme->buckets[bucket][pos].type);
		yk3_ppp_eme_generate_rule(eme->buckets[bucket][pos].key, value,
					  &data[4], 4);

		if (eme->buckets[bucket][pos].is_occupied == 2) {
			memcpy(value, eme->buckets[bucket][pos].secondary_value,
			       YK3_PPP_EME_VALUE_SIZE);
			(*(value + 1)) = (((*(value + 1)) & ~YK3_PPP_EME_VALUE_TYPE_MASK) |
					  eme->buckets[bucket][pos].type);
			yk3_ppp_eme_generate_rule(eme->buckets[bucket][pos].secondary_key,
						  value, data, 4);
		}
		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
		if (eme->buckets[bucket][pos].next_index > pos)
			pointer = eme->buckets[bucket][pos].next_index - pos - 1;
		if (eme->buckets[bucket][pos].next_index < pos)
			pointer = eme->buckets[bucket][pos].next_index +
				  YK3_PPP_EME_DEPTH - 1 - pos;
		memcpy(value, (u8 *)&(pointer), YK3_PPP_EME_VALUE_SIZE);
		(*(value + 1)) = (((*(value + 1)) & ~YK3_PPP_EME_VALUE_TYPE_MASK) |
				  eme->buckets[bucket][pos].type);
		yk3_ppp_eme_generate_rule(&eme->buckets[bucket][pos].key[30], value,
					  data, YK3_PPP_EME_DATA_ROUND);
		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_TAIL:
		memcpy(value, eme->buckets[bucket][pos].value, YK3_PPP_EME_VALUE_SIZE);
		(*(value + 1)) = (((*(value + 1)) & ~YK3_PPP_EME_VALUE_TYPE_MASK) |
				  eme->buckets[bucket][pos].type);
		yk3_ppp_eme_generate_rule(eme->buckets[bucket][pos].key, value, data,
					  YK3_PPP_EME_DATA_ROUND);
		break;
	default:
		memcpy(value, eme->buckets[bucket][pos].value, YK3_PPP_EME_VALUE_SIZE);
		(*(value + 1)) = (((*(value + 1)) & ~YK3_PPP_EME_VALUE_TYPE_MASK) |
				  eme->buckets[bucket][pos].type);
		yk3_ppp_eme_generate_rule(eme->buckets[bucket][pos].key, value, data,
					  YK3_PPP_EME_DATA_ROUND);
	}

	yk3_ppp_eme_iowrite32(eme, eme->hw.waddr, ram_addr);
	for (i = 0; i < eme->hw.data_round; i++) {
		yk3_ppp_eme_iowrite32(eme, eme->hw.data_addr, data[i]);
		yk3_debug("eme-%d: write ram addr %08x data: 0x%08x.\n",
			  eme->hw_id, ram_addr, data[i]);
	}

	return 0;
}

static void yk3_ppp_eme_parse_rule_data(const u32 *data, u8 *key, u8 *value)
{
	int i;

	for (i = 0; i < YK3_PPP_EME_DATA_ROUND; i++) {
		if (i == 0) {
			memcpy(value, &data[0], 2);
			memcpy(key, (u8 *)&data[0] + 2, 2);
		} else {
			memcpy(key + 4 * i - 2, &data[i], 4);
		}
	}
}

static int yk3_ppp_eme_read_rule(struct yk3_ppp_eme *eme, u8 bucket, u32 pos)
{
	u32 hw_data[YK3_PPP_EME_DATA_ROUND] = {0};
	u32 data[YK3_PPP_EME_DATA_ROUND] = {0};
	static const u32 empty_data[YK3_PPP_EME_DATA_ROUND] = {0};
	u8 key[YK3_PPP_EME_KEY_SIZE] = {0};
	u8 value[YK3_PPP_EME_VALUE_SIZE] = {0};
	u8 key_high[YK3_PPP_EME_KEY_SIZE] = {0};
	u8 value_high[YK3_PPP_EME_VALUE_SIZE] = {0};
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1] = {0};
	u32 ram_addr;
	int i;
	u16 pointer;
	u8 type;
	u32 cal_pos;
	int next_index = 0;
	u32 tbl_id = 0;
	u32 key_size = 0;
	u32 value_size = YK3_PPP_EME_VALUE_SIZE;

	ram_addr = yk3_ppp_eme_get_ram_addr(bucket, pos);

	yk3_ppp_eme_iowrite32(eme, eme->hw.raddr, ram_addr);

	for (i = 0; i < eme->hw.data_round; i++)
		hw_data[i] = yk3_ppp_eme_ioread32(eme, eme->hw.data_addr);

	if (!memcmp(hw_data, empty_data, YK3_PPP_EME_DATA_ROUND * sizeof(u32)))
		return -EINVAL;

	yk3_ppp_eme_parse_rule_data(hw_data, key, value);
	type = value[YK3_PPP_EME_VALUE_SIZE - 1] & YK3_PPP_EME_VALUE_TYPE_MASK;
	memset(key, 0, YK3_PPP_EME_KEY_SIZE);

	switch (type) {
	case YK3_PPP_EME_KEY_TYPE_SHARE:
		key_size = YK3_PPP_EME_KEY_SIZE_SHARE;
		memcpy(data, (u8 *)hw_data + key_size + value_size, key_size + value_size);
		yk3_ppp_eme_parse_rule_data(data, key, value);
		tbl_id = key[key_size - 1] & YK3_PPP_EME_KEY_TID_MASK;
		if (__yk3_ppp_eme_create_table(NULL, eme->hw_id, tbl_id, key_size, value_size))
			goto err_table;
		// process first_key
		cal_pos = yk3_ppp_eme_hash(key, type, eme->seed[bucket], eme->mux_seed[bucket]);
		if (pos != cal_pos) {
			yk3_ppp_eme_array_to_hexstr(key, key_size, key_str);
			goto err_pos;
		}

		memcpy(eme->buckets[bucket][pos].key, key, key_size);
		value[value_size - 1] = value[value_size - 1] & ~YK3_PPP_EME_VALUE_TYPE_MASK;
		memcpy(eme->buckets[bucket][pos].value, value, value_size);
		eme->buckets_entry_num[bucket]++;
		eme->buckets[bucket][pos].is_occupied++;

		// process second key
		memset(data, 0, YK3_PPP_EME_DATA_ROUND * sizeof(u32));
		memset(key, 0, YK3_PPP_EME_KEY_SIZE);
		memcpy(data, hw_data, key_size + value_size);
		/**
		 * if first key is empty, the whole data is empty.
		 * so only judge the second key.
		 */
		if (!memcmp(data, empty_data, key_size + value_size))
			break;
		yk3_ppp_eme_parse_rule_data(data, key, value);
		tbl_id = key[key_size - 1] & YK3_PPP_EME_KEY_TID_MASK;
		if (__yk3_ppp_eme_create_table(NULL, eme->hw_id, tbl_id, key_size, value_size))
			goto err_table;
		cal_pos = yk3_ppp_eme_hash(key, type, eme->seed[bucket], eme->mux_seed[bucket]);
		if (pos != cal_pos) {
			yk3_ppp_eme_array_to_hexstr(key, key_size, key_str);
			goto err_pos;
		}

		memcpy(eme->buckets[bucket][pos].secondary_key, key, key_size);
		value[value_size - 1] = value[value_size - 1] & ~YK3_PPP_EME_VALUE_TYPE_MASK;
		memcpy(eme->buckets[bucket][pos].secondary_value, value, value_size);
		eme->buckets_entry_num[bucket]++;
		eme->buckets[bucket][pos].is_occupied++;

		break;
	case YK3_PPP_EME_KEY_TYPE_EXCLUSIVE:
		key_size = YK3_PPP_EME_KEY_SIZE_EXCLUSIVE;
		memcpy(data, hw_data, key_size + value_size);
		yk3_ppp_eme_parse_rule_data(data, key, value);
		tbl_id = key[key_size - 1] & YK3_PPP_EME_KEY_TID_MASK;
		if (__yk3_ppp_eme_create_table(NULL, eme->hw_id, tbl_id, key_size, value_size))
			goto err_table;
		cal_pos = yk3_ppp_eme_hash(key, type, eme->seed[bucket], eme->mux_seed[bucket]);
		if (pos != cal_pos) {
			yk3_ppp_eme_array_to_hexstr(key, key_size, key_str);
			goto err_pos;
		}

		memcpy(eme->buckets[bucket][pos].key, key, key_size);
		value[value_size - 1] = value[value_size - 1] & ~YK3_PPP_EME_VALUE_TYPE_MASK;
		memcpy(eme->buckets[bucket][pos].value, value, value_size);
		eme->buckets_entry_num[bucket]++;
		eme->buckets[bucket][pos].is_occupied++;

		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
		key_size = YK3_PPP_EME_KEY_SIZE_MULTI;
		memcpy(data, hw_data, (key_size >> 1) + value_size);
		yk3_ppp_eme_parse_rule_data(data, key_high, value_high);
		tbl_id = key_high[(key_size >> 1) - 1] & YK3_PPP_EME_KEY_TID_MASK;
		if (__yk3_ppp_eme_create_table(NULL, eme->hw_id, tbl_id, key_size, value_size))
			goto err_table;
		pointer = (value_high[0] | value_high[1] << 8);
		next_index = pointer & YK3_PPP_EME_NXTINDEX_MASK;
		next_index += (pos + 1);
		ram_addr = yk3_ppp_eme_get_ram_addr(bucket, next_index);
		yk3_ppp_eme_iowrite32(eme, eme->hw.raddr, ram_addr);
		for (i = 0; i < eme->hw.data_round; i++)
			hw_data[i] = yk3_ppp_eme_ioread32(eme, eme->hw.data_addr);
		memset(data, 0, YK3_PPP_EME_DATA_ROUND * sizeof(u32));
		memcpy(data, hw_data, (key_size >> 1) + value_size);
		yk3_ppp_eme_parse_rule_data(data, key, value);
		memcpy(key + (key_size >> 1), key_high, key_size >> 1);
		cal_pos = yk3_ppp_eme_hash(key, type, eme->seed[bucket], eme->mux_seed[bucket]);
		if (pos != cal_pos) {
			yk3_ppp_eme_array_to_hexstr((u8 *)data, key_size, key_str);
			goto err_pos;
		}

		if (eme->buckets[bucket][next_index].is_occupied) {
			yk3_ppp_eme_array_to_hexstr((u8 *)data, key_size, key_str);
			yk3_err("eme-%d: next index pos already occupied.\n", eme->hw_id);
			goto err_pos;
		}

		memcpy(eme->buckets[bucket][pos].key, key, key_size);
		value[value_size - 1] = value[value_size - 1] & ~YK3_PPP_EME_VALUE_TYPE_MASK;
		memcpy(eme->buckets[bucket][pos].value, value, value_size);
		eme->buckets[bucket][pos].pre_index = -1;
		eme->buckets[bucket][pos].next_index = next_index;
		eme->buckets[bucket][pos].type = type;
		eme->buckets[bucket][pos].is_occupied = 1;

		memcpy(eme->buckets[bucket][next_index].key, key, key_size);
		memcpy(eme->buckets[bucket][next_index].value, value, value_size);
		eme->buckets[bucket][pos].pre_index = pos;
		eme->buckets[bucket][pos].next_index = -1;
		eme->buckets[bucket][next_index].type = YK3_PPP_EME_KEY_TYPE_MULTI_TAIL;
		eme->buckets[bucket][next_index].is_occupied = 1;
		break;
	}

	return 0;
err_table:
	yk3_err("eme-%d: failed to create table for loaded rule.\n", eme->hw_id);
	return -1;
err_pos:
	yk3_err("eme-%d: bucket %u, pos %u, key %s pos error.\n",
		eme->hw_id, bucket, pos, key_str);
	return -ENOENT;
}

static int __maybe_unused yk3_ppp_eme_load_from_hw(struct yk3_pdev_priv *pdev_priv, int hw_id)
{
	int ret;
	int i;
	int j;
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme = eme_mgr->emes[hw_id];

	for (i = 0; i < eme->bucket_count; i++) {
		eme->seed[i] = yk3_ppp_eme_ioread32(eme, eme->hw.seed_addr[i]);
		eme->mux_seed[i] = yk3_ppp_eme_ioread32(eme, eme->hw.mux_seed_addr[i]);
	}

	for (i = 0; i < eme->bucket_count; i++) {
		eme->buckets_entry_num[i] = 0;
		for (j = 0; j < YK3_PPP_EME_DEPTH; j++) {
			ret = yk3_ppp_eme_read_rule(eme, i, j);
			if (ret == 0)
				eme->buckets_entry_num[i]++;
		}
	}

	return 0;
}

static int yk3_ppp_eme_reverse_update(struct yk3_ppp_eme *eme,
				      struct yk3_ppp_eme_kick_stream *stream)
{
	struct yk3_ppp_eme_kick kick;
	int i;

	for (i = stream->count - 1; i >= 0; i--) {
		kick = stream->kicks[i];
		eme->buckets[kick.to_bucket][kick.to_pos] = kick.entry;
		if (kick.from_bucket == YK3_PPP_EME_DISCARD_RULE)
			eme->buckets[kick.to_bucket][kick.to_pos].is_occupied = 0;

		yk3_ppp_eme_write_rule(eme, kick.to_bucket, kick.to_pos);
	}

	return 0;
}

static int yk3_ppp_eme_init_simple(struct yk3_pdev_priv *pdev_priv, int hw_id)
{
	int i;
	int j;
	int ret;
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme;

	eme = kzalloc(sizeof(*eme), GFP_ATOMIC);
	if (!eme)
		return -ENOMEM;

	eme->hw_id = hw_id;
	eme->hw.bar_addr = pdev_priv->bar_addr[YK3_PPP_EME_BAR];

	yk3_ppp_eme_get_hw_offset(eme);
	yk3_ppp_eme_get_hw_info(eme);
	yk3_debug("eme-%d: disabled on hw before init.\n", hw_id);
	yk3_ppp_eme_iowrite32(eme, eme->hw.init_done_addr, 0);
	memset(&eme->buckets_entry_num, 0, sizeof(u32) * YK3_PPP_EME_BUCKET_NUM);
	for (i = 0; i < YK3_PPP_EME_BUCKET_NUM; i++)
		memset(&eme->buckets[i][0], 0,
		       sizeof(struct yk3_ppp_eme_entry) * YK3_PPP_EME_DEPTH);
	memset(&eme->tables, 0, sizeof(struct yk3_ppp_eme_table) * YK3_PPP_EME_MAX_TABLE_NUM);

	for (i = 0; i < YK3_PPP_EME_BUCKET_NUM; i++)
		for (j = 0; j < YK3_PPP_EME_DEPTH; j++)
			yk3_ppp_eme_write_rule(eme, i, j);

	ret = yk3_ppp_eme_generate_seed(eme);
	if (ret) {
		yk3_err("eme-%d: failed to generate hash seeds.\n", hw_id);
		return ret;
	}

	yk3_ppp_eme_iowrite32(eme, eme->hw.init_done_addr, 1);
	yk3_debug("eme-%d: enabled on hw after init.\n", hw_id);
	eme_mgr->emes[hw_id] = eme;

	return 0;
}

static void yk3_ppp_eme_uninit_simple(struct yk3_pdev_priv *pdev_priv, int hw_id)
{
	int i;
	int j;
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme = eme_mgr->emes[hw_id];

	if (!eme)
		return;

	yk3_ppp_eme_iowrite32(eme, eme->hw.init_done_addr, 0);
	yk3_debug("eme-%d: disabled on hw to uninit.\n", hw_id);
	memset(&eme->buckets_entry_num, 0, sizeof(u32) * YK3_PPP_EME_BUCKET_NUM);
	for (i = 0; i < YK3_PPP_EME_BUCKET_NUM; i++)
		memset(&eme->buckets[i][0], 0,
		       sizeof(struct yk3_ppp_eme_entry) * YK3_PPP_EME_DEPTH);

	// clear hw
	for (i = 0; i < YK3_PPP_EME_BUCKET_NUM; i++)
		for (j = 0; j < YK3_PPP_EME_DEPTH; j++)
			yk3_ppp_eme_write_rule(eme, i, j);

	kfree(eme);
	eme_mgr->emes[hw_id] = NULL;
}

static int __yk3_ppp_eme_create_table(struct yk3_pdev_priv *pdev_priv, int hw_id,
				      u32 table_id, u32 key_size, u32 value_size)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme;
	struct yk3_ppp_eme_table *table;
	u32 key_payload_size = key_size;

	if (!eme_mgr->init_done) {
		yk3_err("eme: eme not init yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid eme table id: %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	// skip key hdr
	key_size++;
	if (key_size > YK3_PPP_EME_KEY_SIZE_MULTI) {
		yk3_err("eme-%d: key size overflow %u.\n", hw_id, key_payload_size);
		return -EINVAL;
	}

	if (key_size <= YK3_PPP_EME_KEY_SIZE_SHARE)
		key_size = YK3_PPP_EME_KEY_SIZE_SHARE;
	else if (key_size <= YK3_PPP_EME_KEY_SIZE_EXCLUSIVE)
		key_size = YK3_PPP_EME_KEY_SIZE_EXCLUSIVE;
	else
		key_size = YK3_PPP_EME_KEY_SIZE_MULTI;

	if (value_size != YK3_PPP_EME_VALUE_SIZE) {
		yk3_err("eme-%d: invalid value size %u to create table %u.\n",
			hw_id, value_size, table_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (table->is_created) {
		yk3_warn("eme-%d: table %u already created, key size %u, value size %u.\n",
			 hw_id, table_id, table->key_size, table->value_size);

		if (table->key_size != key_size || table->value_size != value_size) {
			yk3_warn("eme-%d: key size %u or value size %u conflicts.\n",
				 hw_id, key_size, value_size);

			return -1;
		}

		return 0;
	}

	table->key_size = key_size;
	table->key_payload_size = key_payload_size;
	table->value_size = value_size;
	table->key_type = yk3_ppp_eme_get_key_type(key_size);
	table->key_hdr = (table->key_type & YK3_PPP_EME_KEY_TYPE_MASK) |
			 (table_id & YK3_PPP_EME_KEY_TID_MASK);
	table->key_hdr_size = sizeof(table->key_hdr);
	table->is_created = 1;
	yk3_info("eme-%d: create table %u: key size %u, value size %u.\n",
		 hw_id, table_id, key_size, value_size);

	return 0;
}

static int yk3_ppp_eme_create_table_locked(struct yk3_pdev_priv *pdev_priv, int hw_id,
					   u32 table_id, u32 key_size, u32 value_size)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_create_table(pdev_priv, hw_id, table_id,
					 key_size, value_size);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

static int __yk3_ppp_eme_delete_table(struct yk3_pdev_priv *pdev_priv,
				      int hw_id, u32 table_id)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme;
	struct yk3_ppp_eme_table *table;
	int bucket, pos;
	u8 key_hdr;
	u8 *key;
	u32 hdr_off;
	u32 payload_off;
	u8 del_key[YK3_PPP_EME_KEY_SIZE] = {0};

	if (!eme_mgr->init_done) {
		yk3_err("eme: eme not initialized yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid table id %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (!table->is_created) {
		yk3_err("eme-%d: table %u not created.\n", hw_id, table_id);
		return -ENOENT;
	}

	hdr_off = table->key_size - table->key_hdr_size;
	payload_off = hdr_off - table->key_payload_size;
	for (bucket = 0; bucket < YK3_PPP_EME_BUCKET_NUM; bucket++) {
		for (pos = 0; pos < YK3_PPP_EME_DEPTH; pos++) {
			if (!eme->buckets[bucket][pos].is_occupied)
				continue;
			if (eme->buckets[bucket][pos].type != table->key_type)
				continue;
			if (eme->buckets[bucket][pos].is_occupied == 2) {
				key = eme->buckets[bucket][pos].secondary_key;
				memcpy(&key_hdr, key + hdr_off, table->key_hdr_size);
				if (key_hdr == table->key_hdr) {
					memcpy(del_key, key + payload_off, table->key_payload_size);
					__yk3_ppp_eme_del_rule(pdev_priv, hw_id, table_id, del_key);
					memset(del_key, 0, YK3_PPP_EME_KEY_SIZE);
				}
			}
			key = eme->buckets[bucket][pos].key;
			memcpy(&key_hdr, key + hdr_off, table->key_hdr_size);
			if (key_hdr != table->key_hdr)
				continue;
			memcpy(del_key, key + payload_off, table->key_payload_size);
			__yk3_ppp_eme_del_rule(pdev_priv, hw_id, table_id, del_key);
			memset(del_key, 0, YK3_PPP_EME_KEY_SIZE);
		}
	}

	table->is_created = 0;
	yk3_info("eme-%d: table %u is deleted.\n", hw_id, table_id);

	return 0;
}

static int yk3_ppp_eme_delete_table_locked(struct yk3_pdev_priv *pdev_priv,
					   int hw_id, u32 table_id)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_delete_table(pdev_priv, hw_id, table_id);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

static inline void yk3_ppp_eme_gen_hkey(char *hkey, struct yk3_ppp_eme_table *table, char *key)
{
	memcpy(hkey + table->key_size - table->key_hdr_size - table->key_payload_size,
	       key, table->key_payload_size);
	memcpy(hkey + table->key_size - table->key_hdr_size, &table->key_hdr, table->key_hdr_size);
}

static int __yk3_ppp_eme_add_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
				  u32 table_id, u8 *key, u8 *value)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme_tmp = NULL;
	struct yk3_ppp_eme *eme;
	struct yk3_ppp_eme_table *table;
	struct yk3_ppp_eme_kick_stream *stream = NULL;
	u8 hkey[YK3_PPP_EME_KEY_SIZE] = {0};
	int ret = 0;
	int i;

	if (!eme_mgr->init_done) {
		yk3_err("eme: not initialized yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid eme table id: %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	if (!key || !value) {
		yk3_err("eme-%d: key or value NULL.\n", hw_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (!table->is_created) {
		yk3_err("eme-%d: table %u to insert not created yet.\n", hw_id, table_id);
		ret = -ENOENT;
		goto inset_out;
	}

	eme_tmp = kzalloc(sizeof(*eme_tmp), GFP_ATOMIC);
	if (!eme_tmp) {
		ret = -ENOMEM;
		goto inset_out;
	}
	stream = kzalloc(sizeof(*stream), GFP_ATOMIC);
	if (!stream) {
		ret = -ENOMEM;
		goto inset_out;
	}

	*eme_tmp = *eme;
	yk3_ppp_eme_gen_hkey(hkey, table, key);

	ret = yk3_ppp_eme_try_insert(eme_tmp, table_id, hkey, value, stream);
	if (ret) {
		yk3_err("eme-%d: failed to insert rule.\n", hw_id);
		goto inset_out;
	}

	ret = yk3_ppp_eme_reverse_update(eme, stream);
	if (ret) {
		yk3_err("eme-%d: failed to reverse update.\n", hw_id);
		goto inset_out;
	}

	for (i = 0; i < eme->bucket_count; i++)
		eme->buckets_entry_num[i] = eme_tmp->buckets_entry_num[i];

inset_out:
	kfree(eme_tmp);
	kfree(stream);

	return ret;
}

static int yk3_ppp_eme_add_rule_locked(struct yk3_pdev_priv *pdev_priv, int hw_id,
				       u32 table_id, u8 *key, u8 *value)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_add_rule(pdev_priv, hw_id, table_id, key, value);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

static int __yk3_ppp_eme_del_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
				  u32 table_id, u8 *key)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme;
	struct yk3_ppp_eme_table *table;
	u8 hkey[YK3_PPP_EME_KEY_SIZE] = {0};
	u8 value_tmp[YK3_PPP_EME_VALUE_SIZE] = {0};
	u32 bucket = 0;
	u32 pos = 0;
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1] = {0};
	char val_str[YK3_PPP_EME_VALUE_SIZE * 2 + 1] = {0};

	if (!eme_mgr->init_done) {
		yk3_err("eme: eme not initialized yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid eme table id: %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	if (!key) {
		yk3_err("eme-%d: key is NULL.\n", hw_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (!table->is_created) {
		yk3_err("eme-%d: table %u to delete rule not created yet.\n", hw_id, table_id);
		return -EINVAL;
	}

	yk3_ppp_eme_gen_hkey(hkey, table, key);
	yk3_ppp_eme_array_to_hexstr(hkey, table->key_size, key_str);

	if (!yk3_ppp_eme_search(eme, table_id, hkey, value_tmp, &bucket, &pos)) {
		yk3_debug("eme-%d: failed to find key: %s.\n", hw_id, key_str);
		return -ENOENT;
	}

	yk3_ppp_eme_array_to_hexstr(value_tmp, table->value_size, val_str);

	switch (table->key_type) {
	case YK3_PPP_EME_KEY_TYPE_SHARE:
		if (eme->buckets[bucket][pos].is_occupied == 1) {
			eme->buckets[bucket][pos].is_occupied--;
			eme->buckets_entry_num[bucket]--;
			memset(eme->buckets[bucket][pos].key, 0, YK3_PPP_EME_KEY_SIZE);
			memset(eme->buckets[bucket][pos].value, 0, YK3_PPP_EME_VALUE_SIZE);
		} else if (eme->buckets[bucket][pos].is_occupied == 2) {
			eme->buckets[bucket][pos].is_occupied--;
			if (!memcmp(hkey, eme->buckets[bucket][pos].key, table->key_size)) {
				memcpy(eme->buckets[bucket][pos].key,
				       eme->buckets[bucket][pos].secondary_key,
				       table->key_size);
				memcpy(eme->buckets[bucket][pos].value,
				       eme->buckets[bucket][pos].secondary_value,
				       table->value_size);
			}
		}
		memset(eme->buckets[bucket][pos].secondary_key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->buckets[bucket][pos].secondary_value, 0,
		       YK3_PPP_EME_VALUE_SIZE);
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
		eme->buckets[bucket][pos].is_occupied = 0;
		eme->buckets_entry_num[bucket]--;
		memset(eme->buckets[bucket][pos].key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->buckets[bucket][pos].value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->buckets[bucket][pos].type = 0;
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		pos = eme->buckets[bucket][pos].next_index;
		eme->buckets[bucket][pos].is_occupied = 0;
		eme->buckets_entry_num[bucket]--;
		memset(eme->buckets[bucket][pos].key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->buckets[bucket][pos].value, 0, YK3_PPP_EME_VALUE_SIZE);
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		break;
	default:
		eme->buckets[bucket][pos].is_occupied = 0;
		eme->buckets_entry_num[bucket]--;
		memset(eme->buckets[bucket][pos].key, 0, YK3_PPP_EME_KEY_SIZE);
		memset(eme->buckets[bucket][pos].value, 0, YK3_PPP_EME_VALUE_SIZE);
		eme->buckets[bucket][pos].type = 0;
		yk3_ppp_eme_write_rule(eme, bucket, pos);
	}

	yk3_info("eme-%d: key %s with value %s deleted.\n", hw_id, key_str, val_str);

	return 0;
}

static int yk3_ppp_eme_del_rule_locked(struct yk3_pdev_priv *pdev_priv, int hw_id,
				       u32 table_id, u8 *key)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_del_rule(pdev_priv, hw_id, table_id, key);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

static int __yk3_ppp_eme_mod_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
				  u32 table_id, u8 *key, u8 *value)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme_table *table;
	u8 hkey[YK3_PPP_EME_KEY_SIZE] = {0};
	struct yk3_ppp_eme *eme;
	u32 bucket = 0;
	u32 pos = 0;
	u8 value_tmp[YK3_PPP_EME_VALUE_SIZE] = {0};
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1] = {0};
	char val_str[YK3_PPP_EME_VALUE_SIZE * 2 + 1] = {0};

	if (!eme_mgr->init_done) {
		yk3_err("eme: eme not initialized yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid table id %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	if (!key || !value) {
		yk3_err("eme-%d: null key or value.\n", hw_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (!table->is_created) {
		yk3_err("eme-%d: table %u to mod rule not created yet.\n", hw_id, table_id);
		return -EINVAL;
	}

	yk3_ppp_eme_gen_hkey(hkey, table, key);
	yk3_ppp_eme_array_to_hexstr(hkey, table->key_size, key_str);
	yk3_ppp_eme_array_to_hexstr(value, table->value_size, val_str);

	if (!yk3_ppp_eme_search(eme, table_id, hkey, value_tmp, &bucket, &pos)) {
		yk3_err("eme-%d: failed to find key: %s.\n", hw_id, key_str);
		return -ENOENT;
	}

	switch (table->key_type) {
	case YK3_PPP_EME_KEY_TYPE_SHARE:
		if (eme->buckets[bucket][pos].is_occupied == 1) {
			memcpy(eme->buckets[bucket][pos].key, hkey, table->key_size);
			memcpy(eme->buckets[bucket][pos].value, value, table->value_size);
		} else if (eme->buckets[bucket][pos].is_occupied == 2) {
			if (!memcmp(hkey, eme->buckets[bucket][pos].key, table->key_size)) {
				memcpy(eme->buckets[bucket][pos].key, hkey,
				       table->key_size);
				memcpy(eme->buckets[bucket][pos].value, value,
				       table->value_size);
			} else if (!memcmp(hkey, eme->buckets[bucket][pos].secondary_key,
				   table->key_size)){
				memcpy(eme->buckets[bucket][pos].secondary_key, hkey,
				       table->key_size);
				memcpy(eme->buckets[bucket][pos].secondary_value, value,
				       table->value_size);
			} else {
				yk3_ppp_eme_array_to_hexstr(hkey, table->key_size, key_str);
				yk3_err("eme-%d: key %s Not found.\n", hw_id, key_str);
				return -ENOENT;
			}
		}
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		break;
	case YK3_PPP_EME_KEY_TYPE_MULTI_HEAD:
		memcpy(eme->buckets[bucket][pos].key, hkey, table->key_size);
		memcpy(eme->buckets[bucket][pos].value, value, table->value_size);
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		pos = eme->buckets[bucket][pos].next_index;
		memcpy(eme->buckets[bucket][pos].key, hkey, table->key_size);
		memcpy(eme->buckets[bucket][pos].value, value, table->value_size);
		yk3_ppp_eme_write_rule(eme, bucket, pos);
		break;
	default:
		memcpy(eme->buckets[bucket][pos].key, hkey, table->key_size);
		memcpy(eme->buckets[bucket][pos].value, value, table->value_size);
		yk3_ppp_eme_write_rule(eme, bucket, pos);
	}

	yk3_info("eme-%d: key %s modify value with %s.\n", hw_id, key_str, val_str);

	return 0;
}

static int yk3_ppp_eme_mod_rule_locked(struct yk3_pdev_priv *pdev_priv, int hw_id,
				       u32 table_id, u8 *key, u8 *value)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_mod_rule(pdev_priv, hw_id, table_id, key, value);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

static int __yk3_ppp_eme_get_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
				  u32 table_id, u8 *key, u8 *value)
{
	u8 hkey[YK3_PPP_EME_KEY_SIZE] = {0};
	struct yk3_ppp_eme_table *table;
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	struct yk3_ppp_eme *eme;
	u32 bucket;
	u32 pos;
	char key_str[YK3_PPP_EME_KEY_SIZE * 2 + 1] = {0};
	char val_str[YK3_PPP_EME_VALUE_SIZE * 2 + 1] = {0};

	if (!eme_mgr->init_done) {
		yk3_err("eme: eme not initialized yet.\n");
		return -EINVAL;
	}

	if (hw_id >= YK3_PPP_EME_HW_ID_MAX) {
		yk3_err("eme: invalid eme id %d.\n", hw_id);
		return -EINVAL;
	}

	if (table_id >= YK3_PPP_EME_MAX_TABLE_NUM) {
		yk3_err("eme-%d: invalid table id %u.\n", hw_id, table_id);
		return -EINVAL;
	}

	if (!key || !value) {
		yk3_err("eme-%d: null key or value.\n", hw_id);
		return -EINVAL;
	}

	eme = eme_mgr->emes[hw_id];
	table = &eme->tables[table_id];

	if (!table->is_created) {
		yk3_err("eme-%d: table %u to get rule not created yet.\n", hw_id, table_id);
		return -EINVAL;
	}

	yk3_ppp_eme_gen_hkey(hkey, table, key);
	yk3_ppp_eme_array_to_hexstr(hkey, table->key_size, key_str);

	if (!yk3_ppp_eme_search(eme, table_id, hkey, value, &bucket, &pos)) {
		yk3_err("eme-%d: key %s not found.\n", hw_id, key_str);
		return -ENOENT;
	}

	yk3_ppp_eme_array_to_hexstr(value, table->value_size, val_str);
	yk3_info("eme-%d: get key %s with value %s.\n", hw_id, key_str, val_str);

	return 0;
}

static int yk3_ppp_eme_get_rule_locked(struct yk3_pdev_priv *pdev_priv, int hw_id,
				       u32 table_id, u8 *key, u8 *value)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	spin_lock(&eme_mgr->opt_lock);
	ret = __yk3_ppp_eme_get_rule(pdev_priv, hw_id, table_id, key, value);
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

// mbox
enum {
	YK3_EME_MBOX_TABLE_ADD = 0,
	YK3_EME_MBOX_TABLE_DEL = 1,
};

struct yk3_eme_mbox_table_msg {
	int hw_id;
	u32 table_id;
	u32 key_size;
	u32 val_size;
	u16 table_op;
};

enum {
	YK3_EME_MBOX_RULE_ADD = 0,
	YK3_EME_MBOX_RULE_DEL = 1,
	YK3_EME_MBOX_RULE_MOD = 2,
	YK3_EME_MBOX_RULE_GET = 3,
};

struct yk3_eme_mbox_rule_msg {
	int hw_id;
	u32 table_id;
	u8 key[YK3_PPP_EME_KEY_SIZE];
	u8 val[YK3_PPP_EME_VALUE_SIZE];
	u16 rule_op;
};

struct yk3_eme_mbox_rsp_msg {
	u8 ret_val[YK3_PPP_EME_VALUE_SIZE];
	int ret_code;
};

struct yk3_eme_mbox_opcode_cb {
	u16 opcode;
	void (*cb)(struct yk3_mbox_msg *msg, void *param);
};

static void yk3_eme_mbox_set_table(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_eme_mbox_table_msg *req;
	struct yk3_eme_mbox_rsp_msg *rsp;
	struct yk3_mbox_msg mbox_rsp = {0};
	struct yk3_mbox_option opt = {0};
	int ret = -EOPNOTSUPP;

	req = (struct yk3_eme_mbox_table_msg *)msg->data;
	rsp = (struct yk3_eme_mbox_rsp_msg *)mbox_rsp.data;

	switch (req->table_op) {
	case YK3_EME_MBOX_TABLE_ADD:
		ret = yk3_ppp_eme_create_table_locked(pdev_priv, req->hw_id, req->table_id,
						      req->key_size, req->val_size);
		break;
	case YK3_EME_MBOX_TABLE_DEL:
		ret = yk3_ppp_eme_delete_table_locked(pdev_priv, req->hw_id, req->table_id);
		break;
	default:
		yk3_dev_err("eme-%d: unknown table_op %u from mbox.\n",
			    req->hw_id, req->table_op);
		break;
	}

	if (ret)
		yk3_dev_err("eme-%d: table %d opcode %u return %d failed.\n",
			    req->hw_id, req->table_id, req->table_op, ret);

	mbox_rsp.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_rsp.seqno = msg->seqno;
	mbox_rsp.dst_id = msg->src_id;
	rsp->ret_code = ret;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_rsp, &opt, NULL);
	if (ret)
		yk3_dev_err("eme-%d: failed to send table op %u rsp mbox msg.\n",
			    req->hw_id, req->table_op);
}

static void yk3_eme_mbox_set_rule(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_eme_mbox_rule_msg *req;
	struct yk3_eme_mbox_rsp_msg *rsp;
	struct yk3_mbox_msg mbox_rsp = {0};
	struct yk3_mbox_option opt = {0};
	int ret = -EOPNOTSUPP;

	req = (struct yk3_eme_mbox_rule_msg *)msg->data;
	rsp = (struct yk3_eme_mbox_rsp_msg *)mbox_rsp.data;

	// TODO: print rule info
	switch (req->rule_op) {
	case YK3_EME_MBOX_RULE_ADD:
		ret = yk3_ppp_eme_add_rule_locked(pdev_priv, req->hw_id, req->table_id,
						  req->key, req->val);
		break;
	case YK3_EME_MBOX_RULE_DEL:
		ret = yk3_ppp_eme_del_rule_locked(pdev_priv, req->hw_id, req->table_id,
						  req->key);
		break;
	case YK3_EME_MBOX_RULE_MOD:
		ret = yk3_ppp_eme_mod_rule_locked(pdev_priv, req->hw_id, req->table_id,
						  req->key, req->val);
		break;
	case YK3_EME_MBOX_RULE_GET:
		ret = yk3_ppp_eme_get_rule_locked(pdev_priv, req->hw_id, req->table_id,
						  req->key, rsp->ret_val);
		break;
	default:
		yk3_dev_err("eme-%d: table %u unknown rule opcode %u from mbox.\n",
			    req->hw_id, req->table_id, req->rule_op);
		break;
	}

	if (ret)
		yk3_dev_err("eme-%d: table %u, rule opcode %u failed with %d!\n",
			    req->hw_id, req->table_id, req->rule_op, ret);

	mbox_rsp.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_rsp.seqno = msg->seqno;
	mbox_rsp.dst_id = msg->src_id;
	rsp->ret_code = ret;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_rsp, &opt, NULL);
	if (ret)
		yk3_dev_err("eme-%d: table %u, rule opcode %u failed to send rsp mbox msg.\n",
			    req->hw_id, req->table_id, req->rule_op);
}

static struct yk3_eme_mbox_opcode_cb yk3_eme_mgr_pf_opcodes[] = {
	{YK3_MBOX_OPCODE_EME_SET_RULE, yk3_eme_mbox_set_rule},
	{YK3_MBOX_OPCODE_EME_SET_TABLE, yk3_eme_mbox_set_table},
};

static int yk3_ppp_eme_init_mbox(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	int i;
	int opcode_num;
	int ret = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	if (IS_ERR_OR_NULL(mbox))
		return -EINVAL;

	opcode_num = ARRAY_SIZE(yk3_eme_mgr_pf_opcodes);
	for (i = 0; i < opcode_num; i++) {
		ret = yk3_mbox_register_callback(pdev_priv, yk3_eme_mgr_pf_opcodes[i].opcode,
						 yk3_eme_mgr_pf_opcodes[i].cb, (void *)pdev_priv);
		if (ret) {
			yk3_dev_err("eme: %s register mbox opcode 0x%04x failed errno %d.\n",
				    __func__, yk3_eme_mgr_pf_opcodes[i].opcode, ret);
			return -EFAULT;
		}
	}

	return 0;
}

static int yk3_ppp_eme_uninit_mbox(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	int i = 0;
	int opcode_num = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	if (IS_ERR_OR_NULL(mbox))
		return -EINVAL;

	opcode_num = ARRAY_SIZE(yk3_eme_mgr_pf_opcodes);

	for (i = 0; i < opcode_num; i++)
		yk3_mbox_unregister_callback(pdev_priv, yk3_eme_mgr_pf_opcodes[i].opcode);

	return 0;
}

static int yk3_ppp_eme_mbox_table_ops(struct yk3_pdev_priv *pdev_priv, int hw_id,
				      u32 table_id, u32 key_size, u32 val_size, int op)
{
	struct yk3_mbox_msg mbox_req = {0};
	struct yk3_mbox_msg mbox_rsp = {0};
	struct yk3_mbox_option opt = {0};
	struct yk3_eme_mbox_table_msg *req;
	struct yk3_eme_mbox_rsp_msg *rsp;
	int ret = 0;

	req = (struct yk3_eme_mbox_table_msg *)mbox_req.data;
	req->hw_id = hw_id;
	req->table_id = table_id;
	req->key_size = key_size;
	req->val_size = val_size;
	req->table_op = op;

	mbox_req.dst_id = yk3_mbox_master_id();
	mbox_req.opcode = YK3_MBOX_OPCODE_EME_SET_TABLE;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 100;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_req, &opt, &mbox_rsp);
	if (ret) {
		yk3_dev_err("eme: mbox send eme table opt %u msg failed.\n", req->table_op);
		return ret;
	}

	rsp = (struct yk3_eme_mbox_rsp_msg *)mbox_rsp.data;
	ret = rsp->ret_code;

	return ret;
}

static int yk3_ppp_eme_mbox_rule_ops(struct yk3_pdev_priv *pdev_priv, int hw_id,
				     u32 table_id, u8 *key, u8 *value, int op)
{
	struct yk3_mbox_msg mbox_req = {0};
	struct yk3_mbox_msg mbox_rsp = {0};
	struct yk3_mbox_option opt = {0};
	struct yk3_eme_mbox_rule_msg *req;
	struct yk3_eme_mbox_rsp_msg *rsp;
	int ret = 0;

	u32 key_size = sizeof(key);

	if (key_size > YK3_PPP_EME_KEY_SIZE) {
		yk3_err("eme: key size too long %u.\n", key_size);
		return -EINVAL;
	}

	req = (struct yk3_eme_mbox_rule_msg *)mbox_req.data;
	req->hw_id = hw_id;
	req->table_id = table_id;
	req->rule_op = op;
	memcpy(req->key, key, key_size);

	switch (op) {
	case YK3_EME_MBOX_RULE_ADD:
	case YK3_EME_MBOX_RULE_MOD:
		memcpy(req->val, value, YK3_PPP_EME_VALUE_SIZE);
		break;
	case YK3_EME_MBOX_RULE_GET:
	case YK3_EME_MBOX_RULE_DEL:
		break;
	default:
		yk3_dev_err("eme: unknown eme rule op %u.\n", op);
	}

	mbox_req.dst_id = yk3_mbox_master_id();
	mbox_req.opcode = YK3_MBOX_OPCODE_EME_SET_RULE;
	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 100;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_req, &opt, &mbox_rsp);
	if (ret) {
		yk3_dev_err("eme: mbox send eme rule opt %u msg failed.\n", req->rule_op);
		return ret;
	}

	rsp = (struct yk3_eme_mbox_rsp_msg *)mbox_rsp.data;
	ret = rsp->ret_code;
	if (op == YK3_EME_MBOX_RULE_GET)
		memcpy(value, rsp->ret_val, YK3_PPP_EME_VALUE_SIZE);

	return ret;
}

int yk3_ppp_eme_create_table(struct yk3_pdev_priv *pdev_priv, int hw_id,
			     u32 table_id, u32 key_size, u32 val_size)
{
	return yk3_ppp_eme_mbox_table_ops(pdev_priv, hw_id, table_id, key_size,
					  val_size, YK3_EME_MBOX_TABLE_ADD);
}

int yk3_ppp_eme_delete_table(struct yk3_pdev_priv *pdev_priv, int hw_id, u32 table_id)
{
	return yk3_ppp_eme_mbox_table_ops(pdev_priv, hw_id, table_id, 0,
					  0, YK3_EME_MBOX_TABLE_DEL);
}

int yk3_ppp_eme_add_rule(struct yk3_pdev_priv *pdev_priv, int hw_id, u32 table_id,
			 u8 *key, u8 *value)
{
	return yk3_ppp_eme_mbox_rule_ops(pdev_priv, hw_id, table_id, key,
					 value, YK3_EME_MBOX_RULE_ADD);
}

int yk3_ppp_eme_del_rule(struct yk3_pdev_priv *pdev_priv, int hw_id, u32 table_id, u8 *key)
{
	return yk3_ppp_eme_mbox_rule_ops(pdev_priv, hw_id, table_id, key,
					 NULL, YK3_EME_MBOX_RULE_DEL);
}

int yk3_ppp_eme_mod_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
			 u32 table_id, u8 *key, u8 *value)
{
	return yk3_ppp_eme_mbox_rule_ops(pdev_priv, hw_id, table_id, key,
					 value, YK3_EME_MBOX_RULE_MOD);
}

int yk3_ppp_eme_get_rule(struct yk3_pdev_priv *pdev_priv, int hw_id,
			 u32 table_id, u8 *key, u8 *value)
{
	return yk3_ppp_eme_mbox_rule_ops(pdev_priv, hw_id, table_id, key,
					 value, YK3_EME_MBOX_RULE_GET);
}

int yk3_ppp_eme_init(struct yk3_pdev_priv *pdev_priv)
{
	int i;
	int ret = 0;
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	spin_lock(&eme_mgr->opt_lock);
	if (eme_mgr->init_done) {
		spin_unlock(&eme_mgr->opt_lock);
		yk3_warn("eme: exact match engine already initialized.\n");

		return ret;
	}

	ret = yk3_ppp_eme_init_mbox(pdev_priv);
	if (ret) {
		yk3_err("eme: init eme mbox failed with %d.\n", ret);
		goto err;
	}
	for (i = 0; i < YK3_PPP_EME_HW_ID_MAX; i++) {
		g_eme_mgr.emes[i] = NULL;
		ret = yk3_ppp_eme_init_simple(pdev_priv, i);
		if (ret)
			goto err_mbox;
	}
	eme_mgr->init_done = 1;

	spin_unlock(&eme_mgr->opt_lock);

	return 0;
err_mbox:
	if (yk3_ppp_eme_uninit_mbox(pdev_priv))
		yk3_err("eme: uninit mbox failed with %d.\n", ret);
err:
	for (i = 0; i < YK3_PPP_EME_HW_ID_MAX; i++) {
		kfree(g_eme_mgr.emes[i]);
		g_eme_mgr.emes[i] = NULL;
	}
	spin_unlock(&eme_mgr->opt_lock);

	return ret;
}

void yk3_ppp_eme_uninit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ppp_eme_mgr *eme_mgr = &g_eme_mgr;
	int i;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	spin_lock(&eme_mgr->opt_lock);
	if (!eme_mgr->init_done) {
		spin_unlock(&eme_mgr->opt_lock);
		yk3_warn("eme: not initialized yet.\n");
		return;
	}

	ret = yk3_ppp_eme_uninit_mbox(pdev_priv);
	if (ret)
		yk3_err("eme: uninit mbox failed with %d.\n", ret);

	for (i = 0; i < YK3_PPP_EME_HW_ID_MAX; i++)
		yk3_ppp_eme_uninit_simple(pdev_priv, i);

	eme_mgr->init_done = 0;
	spin_unlock(&eme_mgr->opt_lock);
}
