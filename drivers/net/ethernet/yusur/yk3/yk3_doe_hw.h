/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_DOE_HW_H_
#define __YK3_DOE_HW_H_
#include "yk3_chip.h"
/******************************************************************************/
#ifndef YK3_DOE_U200
#define YK3_DOE_U200 0
#endif

#ifndef YK3_DOE_ONLY
#define YK3_DOE_ONLY 0
#endif

#if YK3_DOE_U200
#undef  YK3_DOE_ONLY
#define YK3_DOE_ONLY 1 // force doe only @ u200
#endif
/******************************************************************************/
/* chip spec begin */
#define YK3_DOE_VERSION_K3		0xd0e2524
#define YK3_DOE_VERSION_K3MAX		0xd0e3000

#define YK3_DOE_PROTECT_NP		0x01
#define YK3_DOE_PROTECT_HOST		0x02
#define YK3_DOE_PROTECT_ALL		(YK3_DOE_PROTECT_NP | YK3_DOE_PROTECT_HOST)

#define YK3_DOE_PROTECT_ACK_K3		0x1f	// 0b11111
#define YK3_DOE_PROTECT_ACK_K3MAX	0x3f	// 0b111111

#define YK3_DOE_PROTECT_ACK_DELAY	100
#define YK3_DOE_PROTECT_ACK_TIMEOUT	3000

#define YK3_DOE_WORK_MODE_HMC		0
#define YK3_DOE_WORK_MODE_NODDR		(BIT(YK3_DOE_AIE) \
					| BIT(YK3_DOE_CIE) \
					| BIT(YK3_DOE_MIE) \
					| BIT(YK3_DOE_HIE) \
					| BIT(YK3_DOE_LHIE))

#define YK3_DOE_PAGE_SIZE		(4 * 1024)

#define YK3_DOE_CACHELINE_COUNT_PER_UNIT	32
#define YK3_DOE_CACHELINE_UNIT_LIMIT		32
#define YK3_DOE_CACHE_BASE_MASK			(YK3_DOE_CACHELINE_COUNT_PER_UNIT - 1)

#define YK3_DOE_CMD_TAG_COUNT		BIT(16) // 64K, tag is 16bit
#define YK3_DOE_CMD_TAG_MASK		(YK3_DOE_CMD_TAG_COUNT - 1)
#define YK3_DOE_CMD_ZOMBIE_SEC		30 // 30s

#define YK3_DOE_HASH_TABLE_LIMIT_K3	8
#define YK3_DOE_HASH_TABLE_LIMIT_K3MAX	16

#define YK3_DOE_INDEX_SRAM_SPACE_K3	(8 * 1024)
#define YK3_DOE_INDEX_SRAM_SPACE_K3MAX	(16 * 1024)

#define YK3_DOE_INDEX_SRAM_SIZE_K3	(YK3_DOE_INDEX_SRAM_SPACE_K3 / sizeof(u32))
#define YK3_DOE_INDEX_SRAM_SIZE_K3MAX	(YK3_DOE_INDEX_SRAM_SPACE_K3MAX / sizeof(u32))

#define YK3_DOE_INDEX_CACHE_PER_HASH	256	// 256 index per hash table
#define YK3_DOE_INDEX_CACHE(sdepth) ({\
	typeof(sdepth) m_sdepth = (sdepth); \
	((m_sdepth) > YK3_DOE_INDEX_CACHE_PER_HASH ? YK3_DOE_INDEX_CACHE_PER_HASH : (m_sdepth)); \
})

#define YK3_DOE_HIE_CACHE_128B_COUNT_K3		(1 * 1024)
#define YK3_DOE_HIE_CACHE_128B_COUNT_K3MAX	(4 * 1024)

#define YK3_DOE_HIE_CACHE_64B_COUNT_K3		(YK3_DOE_HIE_CACHE_128B_COUNT_K3 * 2) // 2K
#define YK3_DOE_HIE_CACHE_64B_COUNT_K3MAX	(YK3_DOE_HIE_CACHE_128B_COUNT_K3MAX * 2) // 8K

#define YK3_DOE_AIE_CACHE_128B_COUNT_K3		(1 * 1024)
#define YK3_DOE_AIE_CACHE_128B_COUNT_K3MAX	(4 * 1024)

#define YK3_DOE_AIE_CACHE_64B_COUNT_K3		(YK3_DOE_AIE_CACHE_128B_COUNT_K3 * 2) // 2K
#define YK3_DOE_AIE_CACHE_64B_COUNT_K3MAX	(YK3_DOE_AIE_CACHE_128B_COUNT_K3MAX * 2) // 8K

#define YK3_DOE_LHIE_CACHE_16B_COUNT_K3		0 // only k3max support LHIE
#define YK3_DOE_LHIE_CACHE_16B_COUNT_K3MAX	(1 * 1024)

#define YK3_DOE_CIE_CACHE_16B_COUNT_K3		(2 * 1024)
#define YK3_DOE_CIE_CACHE_16B_COUNT_K3MAX	(8 * 1024)

#define YK3_DOE_MIE_CACHE_32B_COUNT_K3		(1 * 1024)
#define YK3_DOE_MIE_CACHE_32B_COUNT_K3MAX	(8 * 1024)

// 0: single ddr
// 1: double ddr(deprecated)
#define YK3_DOE_DDR_MODE		0

#define YK3_DOE_SDEPTH_ALIGN		64

#define YK3_DOE_RAM_ITEM_ORDER		3
#define YK3_DOE_RAM_ALIGN_SIZE		BIT(YK3_DOE_RAM_ITEM_ORDER) // 8
#define YK3_DOE_RAM_BLOCK_SIZE		32

#define YK3_DOE_DDR_ITEM_ORDER		6
#define YK3_DOE_DDR_ALIGN_SIZE		BIT(YK3_DOE_DDR_ITEM_ORDER) // 64

#define YK3_DOE_DLEN_LONG		128
#define YK3_DOE_DLEN_SHORT		64

#define YK3_DOE_HIE_KLEN_LONG		96
#define YK3_DOE_HIE_KLEN_SHORT		YK3_DOE_DLEN_SHORT
#define YK3_DOE_HIE_KLEN(is_short)	\
	((is_short) ? YK3_DOE_HIE_KLEN_SHORT : YK3_DOE_HIE_KLEN_LONG)

#define YK3_DOE_HIE_VLEN_LONG		YK3_DOE_DLEN_LONG
#define YK3_DOE_HIE_VLEN_SHORT		YK3_DOE_DLEN_SHORT
#define YK3_DOE_HIE_VLEN(is_short)	\
	((is_short) ? YK3_DOE_HIE_VLEN_SHORT : YK3_DOE_HIE_VLEN_LONG)

#define YK3_DOE_LHIE_KLEN		16
#define YK3_DOE_LHIE_VLEN		16

#define YK3_DOE_AIE_DLEN_LONG		YK3_DOE_DLEN_LONG
#define YK3_DOE_AIE_DLEN_SHORT		YK3_DOE_DLEN_SHORT
#define YK3_DOE_AIE_DLEN(is_short)	\
	((is_short) ? YK3_DOE_AIE_DLEN_SHORT : YK3_DOE_AIE_DLEN_LONG)

#define YK3_DOE_LAIE_DLEN		YK3_DOE_DLEN_LONG
#define YK3_DOE_CIE_DLEN		16
#define YK3_DOE_MIE_DLEN		13

#define YK3_DOE_CIE_DSPACE_ZIP		16
#define YK3_DOE_CIE_DSPACE		64

#define YK3_DOE_TABLE_LIMIT		256
#define YK3_DOE_USER_TABLE_LIMIT	DOE_TABLE_ID_END
#define YK3_DOE_ACTIVE_TABLE_LIMIT	32
#define YK3_DOE_SPEC_TBL_BASE		240
#define YK3_DOE_CMD_MAXSIZE		320 // 32 + 32 + 256(raw) = 32
#define YK3_DOE_CMD_ALIGN_SIZE		32
#define YK3_DOE_CMD_ALIGN_MASK		(YK3_DOE_CMD_ALIGN_SIZE - 1)
#define YK3_DOE_CMD_ALIGN(x)		ALIGN(x, YK3_DOE_CMD_ALIGN_SIZE)
#define YK3_DOE_CACHE_NUM		24

// 0: just 1 slot, NOT use 0
// 1: support fifo(depth: 64)
#define YK3_DMA_MODE_SLOT		0 // __deprecated
#define YK3_DMA_MODE_FIFO		1

/* Logical address offset for index ddr resources */
#define YK3_DOE_INDEX_BLOCK_SIZE		256
#define YK3_DOE_INDEX_COUNT_PER_BLOCK		(YK3_DOE_INDEX_BLOCK_SIZE / sizeof(u32))

union yk3_doe_index_block {
	u8 raw[YK3_DOE_INDEX_BLOCK_SIZE];

	u32 index[YK3_DOE_INDEX_COUNT_PER_BLOCK];
};

#define YK3_DOE_DDR_ALIGN		0x1000		/* 4K */
#define YK3_DOE_DDR_MASK		(YK3_DOE_DDR_ALIGN - 1)
#define YK3_DOE_DDR_SLICE		0x200000	/* 2M */

struct yk3_doe_ddr_block {
	u8 raw[YK3_DOE_DDR_ALIGN];
};

// small-array
#define YK3_DOE_RAM_SIZE		0x8000		/* 32k */
#define YK3_DOE_RAM_MASK		(YK3_DOE_RAM_ALIGN_SIZE - 1)

/* cmd buffer */
#define YK3_DOE_INST_HDRSIZE		32
#define YK3_DOE_INST_ALIGN		32
#define YK3_DOE_INST_SIZE(body_len)	(YK3_DOE_INST_HDRSIZE + 2 + (body_len))

// w: 224, hash insert
// r: 96,  hash query
#define YK3_DOE_INST_MAXBODYSIZE_W	(YK3_DOE_HIE_KLEN_LONG + YK3_DOE_DLEN_LONG)
#define YK3_DOE_INST_MAXBODYSIZE_R	YK3_DOE_HIE_KLEN_LONG
// w: 258, hash insert
// r: 130, hash query
#define _YK3_DOE_INST_MAXSIZE_W		YK3_DOE_INST_SIZE(YK3_DOE_INST_MAXBODYSIZE_W)
#define _YK3_DOE_INST_MAXSIZE_R		YK3_DOE_INST_SIZE(YK3_DOE_INST_MAXBODYSIZE_R)
// w: 288, hash insert
// r: 160, hash query
#define YK3_DOE_INST_MAXSIZE_W		ALIGN(_YK3_DOE_INST_MAXSIZE_W, YK3_DOE_INST_ALIGN)
#define YK3_DOE_INST_MAXSIZE_R		ALIGN(_YK3_DOE_INST_MAXSIZE_R, YK3_DOE_INST_ALIGN)

#define YK3_DOE_EVQ_BATCH_COUNT(block_size) \
	(((block_size) - sizeof(u32)) / sizeof(struct yk3_doe_event))

// write event queue: default
//
// depth: 64K, same as tag count
// block: 64
// batch: 15
// total: 64K*64=4M

//#define YK3_DOE_EVQ_DEPTH_W		YK3_DOE_CMD_TAG_COUNT // 64K
//#define YK3_DOE_EVQ_BLOCK_SIZE_W	64
//#define YK3_DOE_EVQ_BATCH_COUNT_W	YK3_DOE_EVQ_BATCH_COUNT(YK3_DOE_EVQ_BLOCK_SIZE_W) // 15
//#define YK3_DOE_EVQ_TOTAL_SIZE_W	// 64 * 64K = 4M

#define YK3_DOE_EVQ_DEPTH_MIN_W		1024
#define YK3_DOE_EVQ_DEPTH_W		(16 * 1024)
#define YK3_DOE_EVQ_BLOCK_SIZE_W	256
#define YK3_DOE_EVQ_BATCH_COUNT_W	YK3_DOE_EVQ_BATCH_COUNT(YK3_DOE_EVQ_BLOCK_SIZE_W) // 15
#define YK3_DOE_EVQ_TOTAL_SIZE_W	// 64 * 64K = 4M

// read event queue: default
//
// depth: 1K
// block: 4K
// batch: 1
// total: 1K*4K=4M
#define YK3_DOE_EVQ_DEPTH_MIN_R		64
#define YK3_DOE_EVQ_DEPTH_R		(1  * 1024) // 1K
#define YK3_DOE_EVQ_BLOCK_SIZE_R	4096
#define YK3_DOE_EVQ_BATCH_COUNT_R	1
#define YK3_DOE_EVQ_TOTAL_SIZE_R	// 4K * 1K  = 4M

#define YK3_DOE_IRQ_COALESCE_TIMEOUT_R	500
#define YK3_DOE_IRQ_COALESCE_COUNT_R	1

#define YK3_DOE_IRQ_COALESCE_TIMEOUT_W	100
#define YK3_DOE_IRQ_COALESCE_COUNT_W	15

struct yk3_doe_event {
	u8 status;
	u8 nb; /* number of counter table item while loading */
	__le16 tag;
} __packed;

// B0 --------------------------- B15
// count(7B) + bytes(8B) + valid(1B)
struct yk3_doe_hw_counter {
	union {
		struct {
			__le64 count;
			u8 r1[7];
		} __packed;

		struct {
			u8 r2[7];
			__le64 bytes;
		} __packed;
	};

	u8 r3	: 1;
	u8 valid: 1;
	u8 r4	: 6;
} __packed;

static inline u64
doe_hw_counter_count(struct yk3_doe_hw_counter *counter) {
	u64 count = le64_to_cpu(counter->count);

	return count & BIT(56);
}

static inline u64
doe_hw_counter_bytes(struct yk3_doe_hw_counter *counter) {
	return le64_to_cpu(counter->bytes);
}

#ifndef YK3_DOE_FIFO_NEW
#define YK3_DOE_FIFO_NEW	1
#endif

#define YK3_DOE_FIFO_DEPTH		64
#define YK3_DOE_FIFO_MASK		(YK3_DOE_FIFO_DEPTH - 1)
#define YK3_DOE_FIFO_RESV		(!YK3_DOE_FIFO_NEW)
#define YK3_DOE_FIFO_FULL		(YK3_DOE_FIFO_DEPTH - YK3_DOE_FIFO_RESV)
#define YK3_DOE_FIFO_EMPTY		YK3_DOE_FIFO_RESV

#ifndef YK3_DOE_CB_POOL_SMALL
#define YK3_DOE_CB_POOL_SMALL		0
#endif

#if YK3_DOE_CB_POOL_SMALL
#define YK3_DOE_CB_POOL_DEPTH		YK3_DOE_FIFO_DEPTH // 64
#else
#define YK3_DOE_CB_POOL_DEPTH		(2 * YK3_DOE_FIFO_DEPTH) // 128
#endif

#define YK3_DOE_CB_POOL_MASK		(YK3_DOE_CB_POOL_DEPTH - 1)

#define YK3_DOE_CMD_BUFFER_MAXSIZE	(64 * 1024)
// max write cmd:
//	hash insert: 32 + 32 + 96 + 128 = 288
//	index res  : 32 + 32 + 256 = 320, only @ create hash, TODO: NOT use index res
// fifo(max cmd): (64 * 1024) * 64 / 288  = 14563 ==> 14K cmd
// fifo(min cmd): (64 * 1024) * 64 / 64   = 65536 ==> 64K cmd
// evq : (4 * 1024 * 1024) / 256 = 16K  event
#define YK3_DOE_CMD_BUFFER_SIZE_W	(64 * 1024)
// max read cmd:
//	hash query: 32 + 32 + 96 = 160
// fifo: 64 cmd(1 cmd per command buffer, NOT batch)
// evq : 64 event
#define YK3_DOE_CMD_BUFFER_SIZE_R	(4 * 1024) // 256

// 64K * 64 = 4M
#define YK3_DOE_CMD_BUFFER_TOTAL_W	(YK3_DOE_FIFO_DEPTH * YK3_DOE_CMD_BUFFER_SIZE_W)
// 4K * 64 = 256K
#define YK3_DOE_CMD_BUFFER_TOTAL_R	(YK3_DOE_FIFO_DEPTH * YK3_DOE_CMD_BUFFER_SIZE_R)

// LA: logic address
#define YK3_DOE_PAGE_SHIFT		12

#define YK3_DOE_PAGE_ORDER_4K		12 // 4K
#define YK3_DOE_PAGE_ORDER_2M		21 // 2M
#define YK3_DOE_PAGE_ORDER_1G		30 // 1G
// pageinfo table(max 8 enatry)
// --------------------------------------
// order	page-size
// 12		4K
// 21		2M
// 22		4M
// 23		8M
// 24		16M
// 25		32M
// 26		64M
// 27		128M
// 28		256M
// 29		512M
// 30		1G
#define YK3_DOE_PAGE_INFO_COUNT		8

#define YK3_DOE_PAGE_MASK_MIN		(YK3_2MB - 1)
#define YK3_DOE_PAGE_LIMIT		1024
#define YK3_DOE_HMC_MAXSIZE		(64 * YK3_1GB) // 64G
/* chip spec end */
/******************************************************************************/
/* chip reg begin */
#define YK3_DOE_REG_BAR			0

#if YK3_DOE_U200
#define YK3_DOE_REG_BASE		0x800000
#else
#define YK3_DOE_REG_BASE		0x1800000
#endif
#define YK3_BAR_BASE(pdev_priv)		((pdev_priv)->bar_addr[0])
#define YK3_DOE_BAR_BASE(pdev_priv)	(YK3_BAR_BASE(pdev_priv) + YK3_DOE_REG_BASE)

#define YK3_DOE_REG_VERSION		0x00
#define YK3_DOE_REG_EVQ_RESET		0x04
#define YK3_DOE_REG_PF_NUM		0x08
#define YK3_DOE_REG_WORK_MODE		0x0c

#define YK3_DOE_REG_RD_EV_SIZE		0x20
#define YK3_DOE_REG_RD_EV_TOTAL_SIZE	0x24
#define YK3_DOE_REG_RD_EV_BASE_L	0x28
#define YK3_DOE_REG_RD_EV_BASE_H	0x2c
#define YK3_DOE_REG_RD_EV_PTR_L		0x30
#define YK3_DOE_REG_RD_EV_PTR_H		0x34

#define YK3_DOE_REG_WR_EV_SIZE		0x50
#define YK3_DOE_REG_WR_EV_TOTAL_SIZE	0x54
#define YK3_DOE_REG_WR_EV_BASE_L	0x58
#define YK3_DOE_REG_WR_EV_BASE_H	0x5c
#define YK3_DOE_REG_WR_EV_PTR_L		0x60
#define YK3_DOE_REG_WR_EV_PTR_H		0x64

// TODO: init by config
#define YK3_DOE_REG_RD_IRQ_COALESCE	0x74
#define YK3_DOE_REG_WR_IRQ_COALESCE	0x78
union yk3_doe_irq_coalesce {
	u32 raw;

	struct {
		u16 timeout; // ticks
		u16 count;
	};
};

#define YK3_DOE_REG_RD_INT_VECTOR	0x88
#define YK3_DOE_REG_WR_INT_VECTOR	0x8c

#define YK3_DOE_REG_IIU_ALARM		0xc0
#define YK3_DOE_REG_IIU_ALARM_MASK	(BIT(10) - 1)

#define YK3_DOE_REG_PROTECT_CFG		0xb0
#define YK3_DOE_REG_PROTECT_ACK		0xb4

#define YK3_DOE_REG_COUNTER_ZIP		0xb8

// dma_rd_cnt_in : hardware push doorbell into hw-fifo
// dma_rd_cnt_out: hardware pop  doorbell from hw-fifo
// dma_rd_cnt_all: hardware recived doorbel
//
// hw-sort: dma_rd_cnt_all ==> dma_rd_cnt_in ==> dma_rd_cnt_out
#define YK3_DOE_REG_RD_FIFO_CNT_OUT	0x274
#define YK3_DOE_REG_WR_FIFO_CNT_OUT	0x284
#define YK3_DOE_REG_FIFO_CNT_OUT(is_read) \
	((is_read) ? YK3_DOE_REG_RD_FIFO_CNT_OUT : YK3_DOE_REG_WR_FIFO_CNT_OUT)

#define YK3_DOE_REG_DMA_MODE		0x300
#define YK3_DOE_REG_RD_FIFO_LEFT	0x304
#define YK3_DOE_REG_WR_FIFO_LEFT	0x308

#define YK3_DOE_REG_RD_CMD_ADDR_L	0x310
#define YK3_DOE_REG_RD_CMD_ADDR_H	0x314
#define YK3_DOE_REG_RD_CMD_LEN		0x318
#define YK3_DOE_REG_RD_CMD_CONTROL	0x31c

#define YK3_DOE_REG_WR_CMD_ADDR_L	0x320
#define YK3_DOE_REG_WR_CMD_ADDR_H	0x324
#define YK3_DOE_REG_WR_CMD_LEN		0x328
#define YK3_DOE_REG_WR_CMD_CONTROL	0x32c

#define YK3_DOE_REG_CLK_GATE_EN		0x330

// NO small array's reset reg
#define YK3_DOE_REG_INIT_CACHE_AIE	0x140010
#define YK3_DOE_REG_INIT_CACHE_HIE	0x180010
#define YK3_DOE_REG_INIT_CACHE_LHIE	0x1c0010
#define YK3_DOE_REG_INIT_CACHE_CIE	0x200010
#define YK3_DOE_REG_INIT_CACHE_MIE	0x340010

// NO small array's clean cache reg
#define YK3_DOE_REG_CLEAN_CACHE_AIE	0x140014
#define YK3_DOE_REG_CLEAN_CACHE_HIE	0x180014
#define YK3_DOE_REG_CLEAN_CACHE_LHIE	0x1c0014
#define YK3_DOE_REG_CLEAN_CACHE_CIE	0x200014
#define YK3_DOE_REG_CLEAN_CACHE_MIE	0x340014

// NO small array's hash seed reg
#define YK3_DOE_REG_HASH_SEED_AIE	0x140038
#define YK3_DOE_REG_HASH_SEED_HIE	0x180038
#define YK3_DOE_REG_HASH_SEED_LHIE	0x1c0038
#define YK3_DOE_REG_HASH_SEED_CIE	0x200038
#define YK3_DOE_REG_HASH_SEED_MIE	0x340038

#define YK3_DOE_REG_HCODE_MODE_AIE	0x140058
#define YK3_DOE_REG_HCODE_MODE_HIE	0x180058
#define YK3_DOE_REG_HCODE_MODE_LHIE	0x1c0058
#define YK3_DOE_REG_HCODE_MODE_CIE	0x200058
#define YK3_DOE_REG_HCODE_MODE_MIE	0x340058

#define YK3_DOE_REG_CACHE_ISO_HIE	0x18007c

// 1: long  mode, 128B
// 0: short mode, 64B
#define YK3_DOE_REG_AIE_DLEN_LIMIT	0x14003c
#define YK3_DOE_REG_HIE_DLEN_LIMIT	0x18003c

#define YK3_DOE_REG_HIE_ENTRY_COUNT	0x180220
#define YK3_DOE_REG_LHIE_ENTRY_COUNT	0x1c0220

#define ADDITION_CMD_NUM		300
#define YK3_DOE_COUNTER_LOAD_STRIDE	0x20

#define GEN_HEAD_ENABLE(report) (((report) & 0x1) << 1)
#define GEN_HEAD_PRIORITY(priority) ((priority) & (0x4 | 0x1))

#define YK3_DOE_REG_ADDRMAP_BASE	0xB00000
#define YK3_DOE_REG_ADDRMAP_ENABLE	(YK3_DOE_REG_ADDRMAP_BASE + 0x2a00)
#define YK3_DOE_REG_ADDRMAP_PFID	(YK3_DOE_REG_ADDRMAP_BASE + 0x2800)

#define YK3_DOE_ADDRMAP_PFID_SHIFT	9
#define YK3_DOE_ADDRMAP_PFID(pf_id)	((pf_id) << YK3_DOE_ADDRMAP_PFID_SHIFT)

#define YK3_DOE_REG_PAGEMAP_BASE	(YK3_DOE_REG_ADDRMAP_BASE + 0x0)
#define YK3_DOE_REG_PAGEMAP_ENTRY(idx)	(YK3_DOE_REG_PAGEMAP_BASE + (sizeof(u64) * (idx)))
#define YK3_DOE_REG_PAGEMAP_ADDR_L	0x0
#define YK3_DOE_REG_PAGEMAP_ADDR_H	0x4

#define YK3_DOE_REG_PAGEINFO_BASE	(YK3_DOE_REG_ADDRMAP_BASE + 0x2000)
#define YK3_DOE_PAGEINFO_ENTRY_SIZE	0x100
#define YK3_DOE_REG_PAGEINFO_ENTRY(idx) \
	(YK3_DOE_REG_PAGEINFO_BASE + (YK3_DOE_PAGEINFO_ENTRY_SIZE * (idx)))

// page_order: 物理页面大小 = 1<<(page_order + 12)
// addr_begin: 逻辑地址区间起始地址（切掉低12bit）
// addr_end  : 逻辑地址区间结束地址（切掉低12bit）
// index     : 页面映射表的索引，指向逻辑地址区间对应页面组的第1个页面
union yk3_doe_pageinfo_hw {
	u8 raw[YK3_DOE_PAGEINFO_ENTRY_SIZE];
	u32 field[5]; // same as below struct

	// hardware entry
	struct {
		u32 page_valid;
		u32 page_order;
		u32 addr_begin;
		u32 addr_end;
		u32 index;
	};
};

/******************************************************************************/
#define YK3_COMMON_REG_BASE		0x2200000
#define YK3_COMMON_REG(off)		(YK3_COMMON_REG_BASE + (off))

// doe dma channel
// np  int channel
#define YK3_COMMON_REG_PUSH_SELECT	YK3_COMMON_REG(0x74)

#define YK3_DOE_PUSH_SELECT_SOC		0
#define YK3_DOE_PUSH_SELECT_HOST	1
#define YK3_DOE_PUSH_SELECT_NOCV2	2
#define YK3_DOE_PUSH_SELECT(hmc_enable, hmc_soc) ((hmc_enable) \
		? ((hmc_soc) ? YK3_DOE_PUSH_SELECT_SOC : YK3_DOE_PUSH_SELECT_HOST) \
		: YK3_DOE_PUSH_SELECT_NOCV2)

#define YK3_NP_INT_SELECT_HOST		0
#define YK3_NP_INT_SELECT_SOC		1

union yk3_doe_push_select {
	u32 raw;

	struct {
#if defined(YK3_LITTLE_ENDIAN)
		u32 doe_push_sel: 2;
		u32 np_int_sel: 1;
		u32 r: 29;
#elif defined(YK3_BIG_ENDIAN)
		u32 r: 29;
		u32 np_int_sel: 1;
		u32 doe_push_sel: 2;
#endif
	};
};

/* chip reg end */
/******************************************************************************/
/* chip data define */

// k3/k3max is same, use yk3_doe_xie_spec
// k3/k3max is diff, use yk3_chip_spec
struct yk3_doe_xie_spec {
	const char *name;
	u32 reg_init_cache;	// 0 is NOT use
	u32 reg_clean_cache;	// 0 is NOT use
	u32 reg_hash_seed;	// 0 is NOT use
};

#define YK3_DOE_XIE_SPEC(_name, \
			 _reg_init_cache, \
			 _reg_clean_cache, \
			 _reg_hash_seed) { \
	.name = (_name), \
	.reg_init_cache = (_reg_init_cache), \
	.reg_clean_cache = (_reg_clean_cache), \
	.reg_hash_seed = (_reg_hash_seed), \
} /* end */

#define YK3_DOE_XIE_SPEC_INITER { \
	[YK3_DOE_AIE]	= YK3_DOE_XIE_SPEC("aie", \
				YK3_DOE_REG_INIT_CACHE_AIE, \
				YK3_DOE_REG_CLEAN_CACHE_AIE, \
				YK3_DOE_REG_HASH_SEED_AIE), \
	[YK3_DOE_LAIE]	= YK3_DOE_XIE_SPEC("laie", \
				0, /* laie without cache */ \
				0, /* laie without cache */ \
				0  /* laie without hash seed */), \
	[YK3_DOE_CIE]	= YK3_DOE_XIE_SPEC("cie", \
				YK3_DOE_REG_INIT_CACHE_CIE, \
				YK3_DOE_REG_CLEAN_CACHE_CIE, \
				YK3_DOE_REG_HASH_SEED_CIE), \
	[YK3_DOE_MIE]	= YK3_DOE_XIE_SPEC("mie", \
				YK3_DOE_REG_INIT_CACHE_MIE, \
				YK3_DOE_REG_CLEAN_CACHE_MIE, \
				YK3_DOE_REG_HASH_SEED_MIE), \
	[YK3_DOE_HIE]	= YK3_DOE_XIE_SPEC("hie", \
				YK3_DOE_REG_INIT_CACHE_HIE, \
				YK3_DOE_REG_CLEAN_CACHE_HIE, \
				YK3_DOE_REG_HASH_SEED_HIE), \
	[YK3_DOE_LHIE]	= YK3_DOE_XIE_SPEC("lhie", \
				YK3_DOE_REG_INIT_CACHE_LHIE, \
				YK3_DOE_REG_CLEAN_CACHE_LHIE, \
				YK3_DOE_REG_HASH_SEED_LHIE), \
} /* end */

extern const struct yk3_doe_xie_spec yk3_doe_xie_specs[];

static inline const char *
doe_xie_name(enum yk3_doe_xie xie) {
	return is_good_doe_xie(xie) ? yk3_doe_xie_specs[xie].name : YK3_UNKNOWN;
}

enum yk3_doe_hw_opcode {
	YK3_DOE_ARRAY_LOAD	= 0x20,
	YK3_DOE_ARRAY_STORE	= 0x21,

	YK3_DOE_ARRAY_WRITE	= 0x27,
	YK3_DOE_ARRAY_READ	= 0x28,

	YK3_DOE_HASH_INSERT	= 0x30,
	YK3_DOE_HASH_DELETE	= 0x31,
	YK3_DOE_HASH_QUERY	= 0x32,
	YK3_DOE_HASH_UPDATE	= 0x33,
	YK3_DOE_HASH_SAVE	= 0x37,
};

enum yk3_doe_special_table {
	YK3_DOE_HASH_VIEW		= 0xee,	// 238
	YK3_DOE_INDEX_VIEW		= 0xef,	// 239
	YK3_DOE_ZERO_CLEAR_TABLE	= 0xf0,	// 240
	YK3_DOE_CACHE_PARAM_TABLE	= 0xf8,	// 248
	YK3_DOE_INDEX_PARAM_TABLE	= 0xf9,	// 249
	YK3_DOE_AIE_PARAM_TABLE		= 0xfb,	// 251
	YK3_DOE_HIE_PARAM_TABLE		= 0xfc,	// 252
	YK3_DOE_MIU_PARAM_TABLE		= 0xfd,	// 253

	// 240
	YK3_DOE_SPEC_PARAM_BASE		= YK3_DOE_ZERO_CLEAR_TABLE,
	// 1+253-240=14
	YK3_DOE_SPEC_PARAM_COUNT	= 1 + YK3_DOE_MIU_PARAM_TABLE - YK3_DOE_ZERO_CLEAR_TABLE,
};

static inline const char *
doe_spec_table_name(enum yk3_doe_special_table tbl_id) {
	switch (tbl_id) {
	case YK3_DOE_HASH_VIEW:
		return "hash-view";
	case YK3_DOE_INDEX_VIEW:
		return "index-view";
	case YK3_DOE_ZERO_CLEAR_TABLE:
		return "zero-clear";
	case YK3_DOE_CACHE_PARAM_TABLE:
		return "cache-param";
	case YK3_DOE_INDEX_PARAM_TABLE:
		return "index-param";
	case YK3_DOE_AIE_PARAM_TABLE:
		return "aie-param";
	case YK3_DOE_HIE_PARAM_TABLE:
		return "hie-param";
	case YK3_DOE_MIU_PARAM_TABLE:
		return "miu-param";
	default:
		return "unknown-spec";
	}
}

static inline int
doe_spec_table_depth(enum yk3_doe_special_table spec_tbl_id) {
	switch (spec_tbl_id) {
	case YK3_DOE_INDEX_PARAM_TABLE:
	case YK3_DOE_HIE_PARAM_TABLE:
	case YK3_DOE_ZERO_CLEAR_TABLE:
		// 238, only user table, NOT include hash view and index view
		return YK3_DOE_USER_TABLE_LIMIT;
	case YK3_DOE_AIE_PARAM_TABLE:
	case YK3_DOE_MIU_PARAM_TABLE:
	case YK3_DOE_CACHE_PARAM_TABLE:
	default:
		// 240, user table + hash view + index view
		return YK3_DOE_USER_TABLE_LIMIT + 2;
	}
}

/* Memory Interface Unit */
struct yk3_doe_miu_param {
#if defined(YK3_LITTLE_ENDIAN)
	u8 ddr_channel:2;
	u8 endian:1;
	u8 ddr_mode:1;
	u8 rsvd:4;
#elif defined(YK3_BIG_ENDIAN)
	u8 rsvd:4;
	u8 ddr_mode:1;
	u8 endian:1;
	u8 ddr_channel:2;
#endif

	u8 item_size;
	__le16 item_len;
	__le32 ddr_base_low;
	u8 ddr_base_high;
	__le32 __deprecated ddr_base_low1;
	u8 __deprecated ddr_base_high1;
} __packed;

struct yk3_doe_aie_param {
	__le32 depth;
	__le16 item_len;
	u8 item_size;

#if defined(YK3_LITTLE_ENDIAN)
	u8 ddr_channel:2;
	u8 endian:1;
	u8 ddr_mode:1;
	u8 valid:1;
	u8 tbl_type:3;
#elif defined(YK3_BIG_ENDIAN)
	u8 tbl_type:3;
	u8 valid:1;
	u8 ddr_mode:1;
	u8 endian:1;
	u8 ddr_channel:2;
#endif
} __packed;

struct yk3_doe_hie_param {
	__le32 mdepth;		/* main table depth */
	__le32 sdepth;		/* second table depth */
	__le32 index_mask;	/* mask of main table */
	__le16 key_len;
	__le16 value_len;
	u8 item_size;

#if defined(YK3_LITTLE_ENDIAN)
	u8 ddr_channel:2;
	u8 endian:1;
	u8 ddr_mode:1;
	u8 valid:1;
	u8 tbl_type:3;
#elif defined(YK3_BIG_ENDIAN)
	u8 tbl_type:3;
	u8 valid:1;
	u8 ddr_mode:1;
	u8 endian:1;
	u8 ddr_channel:2;
#endif
	__le32 chain_limit;

	struct yk3_doe_hie_cache cache;
} __packed;

struct yk3_doe_cache_param {
#if defined(YK3_LITTLE_ENDIAN)
	u8 valid:1;
	u8 tbl_type:3;
	u8 ddr_channel:2;
	u8 endian:1;
	u8 ddr_mode:1;
#elif defined(YK3_BIG_ENDIAN)
	u8 ddr_mode:1;
	u8 endian:1;
	u8 ddr_channel:2;
	u8 tbl_type:3;
	u8 valid:1;
#endif

	__le16 value_len;
	__le16 key_len;
	__le32 depth;
	u8 item_size;
} __packed;

struct yk3_doe_zero_clear {
	__le32 start;
	__le32 total;
	u8 debug;
} __packed;

static inline void
doe_zero_clear_init(struct yk3_doe_zero_clear *zero_clear, u32 depth_total) {
	zero_clear->start = 0;
	zero_clear->total = cpu_to_le32(depth_total);
	zero_clear->debug = 0;
}

struct yk3_doe_index_param {
	__le16 ram_base;
	/* ddr_base is the index of resource table, base 256Bytes */
	__le32 ddr_base;
	/* point and state: SW init to 0, HW maintain */
	__le16 ram_point;
	__le32 ddr_point;
	u8 ddr_state;
} __packed;

struct yk3_doe_hw_cmd_array {
	__le32 item_id;
};

union yk3_doe_hw_cmd_body {
	// hash op
	u8 body[256 + sizeof(__le32)];

	struct {
		union {
			__le32 item_id; // array load/store/write/read
			__le32 target_table_id; // counter enable
		};

		union {
			struct yk3_doe_zero_clear zero_clear;
			struct yk3_doe_zero_clear counter_load; // same as zero_clear
			u8 value[256];
		};
	};
};

enum { DOE_HW_CMD_HDRLEN = 32};
#define DOE_CMD_SPACE(data_len)	(DOE_HW_CMD_HDRLEN + YK3_DOE_CMD_ALIGN(data_len))

/*
 * AIE: header + 32'B index + data
 * HIE: header + key + value
 */
struct yk3_doe_hw_cmd_header {
	__le16 tag;
	__le16 valid;
	__le16 cmd_len;
	u8 status;
	u8 resv[25];

	u8 opcode;
	u8 tbl_id;
} __packed;

enum yk3_doe_channel {
	YK3_DOE_CHANNEL_DDR0	= 0, // deprecated
	YK3_DOE_CHANNEL_DDR1	= 1, // hmc used
	YK3_DOE_CHANNEL_RAM	= 2, // small-array used

	YK3_DOE_CHANNEL_END,
};

/******************************************************************************/
#endif /* __YK3_DOE_HW_H_ */
