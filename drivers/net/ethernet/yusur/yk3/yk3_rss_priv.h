/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_RSS_PRIV_H_
#define _YK3_RSS_PRIV_H_

#define YK3_RSS_BAR_BASE	0x00000000

/* rss indirect sacle & bias & sw_fr */
#define YK3_RSS_INDIRECT_SCALE_BIAS_ADDR	0x80
#define YK3_RSS_INDIRECT_SCALE		GENMASK(7, 6)
#define YK3_RSS_INDIRECT_BIAS		GENMASK(11, 8)
#define YK3_RSS_INDIRECT_SW_FR		GENMASK(12, 12)
/* default value */
#define YK3_RSS_INDIRECT_SCALE_POWER_VALUE	2
#define YK3_RSS_INDIRECT_BIAS_VALUE	0
#define YK3_RSS_INDIRECT_SW_FR_VALUE	1

/* rss indirect table */
#define YK3_RSS_INDIRECT_BASE	0x9000
/* rss key */
#define YK3_RSS_KEY_ADDR		0xd00140
#define YK3_RSS_HASH_KEY_SIZE	40

enum {
	YK3_CMD_RSS_INDIRECT_TABLE_GET = 1,
	YK3_CMD_RSS_INDIRECT_TABLE_INIT,
	YK3_CMD_RSS_INDIRECT_TABLE_SET,
	YK3_CMD_RSS_INDIRECT_KEY_SET,
	YK3_CMD_RSS_INDIRECT_KEY_GET,
};

struct yk3_mbox_rss_indir_cmd {
	u8 cmd_type;
	s8 cmd_status;
	u16 qstart;
	u16 qnb;
	u8 cmd_data[];
};

#endif /*_YK3_RSS_PRIV_H_*/
