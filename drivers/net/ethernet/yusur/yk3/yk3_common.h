/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_COMMON_H
#define _YK3_COMMON_H
/******************************************************************************/
#ifndef K2U
#define K2U 0
#endif

#ifndef K3_IN_K2U
#define K3_IN_K2U 0
#endif

// temp for k3max
//	K3MAX_PREVIEW should delete @ driver support k3max
#ifndef K3MAX_PREVIEW
#define K3MAX_PREVIEW 0
#endif
/******************************************************************************/
#ifdef __KERNEL__
#include <asm/byteorder.h>

// kernel defined:
//	__LITTLE_ENDIAN/__LITTLE_ENDIAN_BITFIELD
//	or
//	__BIG_ENDIAN/__BIG_ENDIAN_BITFIELD
#if defined(__LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN_BITFIELD)
#define YK3_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN) || defined(__BIG_ENDIAN_BITFIELD)
#define YK3_BIG_ENDIAN
#else
#error "invalid BYTE ORDER"
#endif
#else /* __KERNEL__ */
#include <endian.h>

// user defined: __BYTE_ORDER/__LITTLE_ENDIAN/__BIG_ENDIAN
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define YK3_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define YK3_BIG_ENDIAN
#endif

#ifndef BIT
#define BIT(nr) ((unsigned long)(1) << (nr))
#endif
#endif /* __KERNEL__ */

#define __in
#define __out
#define __inout
#define __deprecated
#define __noused
#define __todo

#ifndef YK3_CACHELINE
#define YK3_CACHELINE 64
#endif

#ifndef __cacheline
#define __cacheline __aligned(YK3_CACHELINE)
#endif

#ifndef fallthrough
#ifdef __has_attribute
#	if __has_attribute(__fallthrough__)
#		define fallthrough __attribute__((__fallthrough__))
#	else
#		define fallthrough do {} while (0)
#	endif
#else
#	define fallthrough do {} while (0)
#endif
#endif

#define YK3_CRLF	"\n"

#define YK3_TAB		"\t"
#define YK3_TAB2	"\t\t"
#define YK3_TAB3	"\t\t\t"
#define YK3_TAB4	"\t\t\t\t"
#define YK3_UNKNOWN	"unknown"

#define YK3_1MB		BIT(20)
#define YK3_2MB		BIT(21)
#define YK3_1GB		BIT(30)

#define YK3_MB(size)	((u32)((size) / YK3_1MB))
#define YK3_GB(size)	((u32)((size) / YK3_1MB))

#define YK3_PTR(ptr)	((u64)(uintptr_t)(ptr))

#define yk3_do_nothing()	do {} while (0)
#define yk3_memzero(ptr, size)	memset(ptr, 0, size)
#define yk3_streq(a, b)		(!strcmp(a, b))

// use a's size
#define yk3_objcmp(a, b)	({	\
	typeof(a) m_a = (a);		\
	memcmp(m_a, b, sizeof(*m_a));	\
})
// use a's size
#define yk3_objeq(a, b)		(!yk3_objcmp(a, b))

// use s's size
#define yk3_objcpy_s(d, s)	({	\
	typeof(s) m_s = (s);		\
	memcpy(d, m_s, sizeof(*m_s));	\
})
// use d's size
#define yk3_objcpy_d(d, s)	({	\
	typeof(d) m_d = (d);		\
	memcpy(m_d, s, sizeof(*m_d));	\
})
// use d's size
#define yk3_objcpy(d, s)    yk3_objcpy_d(d, s)
#define yk3_objzero(obj)    ({	\
	typeof(obj) m_obj = (obj);		\
	yk3_memzero(m_obj, sizeof(*m_obj));	\
})

#define yk3_soprintf(buf, offset, fmt, args...) ({	\
	typeof(offset) m_off = (offset);		\
	snprintf((buf) + m_off, sizeof(buf) - m_off, fmt, ##args); \
})

#define is_good_zone(x, begin, end) ({ \
	typeof(x) m_x = (x); \
	(m_x >= (begin) && m_x < (end)); \
})
#define is_good_enum(x, end) is_good_zone(x, 0, end)
/******************************************************************************/
struct yk3_pdev_priv;

enum yk3_doe_tbl {
#if K3_IN_K2U
	DOE_TABLE_END = 7,
#else
	DOE_TABLE_NORMAL_ARRAY,
	DOE_TABLE_BIG_HASH,
	DOE_TABLE_COUNTER,
	DOE_TABLE_METER,
	DOE_TABLE_LOCK,

	// 小数组表, 是小在容量上, 只能使用32KB的片上ram
	//	key/value无特殊限制
	// 小哈希表, 是小在长度上, key/value都是16B
	//	容量无特殊限制
	//	叫短哈希表更合适
	DOE_TABLE_SMALL_HASH,  // k3max only
	DOE_TABLE_SMALL_ARRAY,

	DOE_TABLE_END
#endif
};

static inline bool
is_good_doe_table_type(int type) {
	return is_good_enum(type, DOE_TABLE_END);
}

enum { DOE_TABLE_ID_END = 238 };

static inline bool
is_good_doe_table_id(int id) {
	return is_good_enum(id, DOE_TABLE_ID_END);
}

enum { DOE_TABLE_ID_LIMIT = 254 };  // hardware table id

static inline bool
is_valid_doe_table_id(int id) {
	return is_good_enum(id, DOE_TABLE_ID_LIMIT);
}

// only for k3max hash table:
//	blacklist: both support non-ddr and hmc
//
//	cache isolation: both support non-ddr and hmc
//		non-ddr: ignore table depth, use cache_unit_num * 32 as depth
struct yk3_doe_hie_cache {
	// 必须是 32 的整数倍，用于确定该表在 Cache 中的起始位置
	//	单位: cacheline
	u16 cache_base;
	// Cache 资源按 32 行 的固定粒度划分为存储单元
	//	定义该表可使用的存储单元数量。如果下发为0，则认为是32
	//	硬件根据上述参数，将哈希值映射到指定的存储区域内进行寻址
	//	最终cache隔离占用的cacheline区间为:
	//		[cache_base, cache_base + cache_unit_num * 32)
	u8 cache_unit_num;
	// 0: blacklist disable
	// 1: blacklist enable
	u8 blacklist;
};

// hash表建表参数
//
// k3max 黑名单表项指令执行策略
//	insert:	对无效的表项会进行插入，并同时更新到DDR上
//	query :	如果命中cache中无效的表项，会直接返回结果，返回状态为0x09
//	delete:	只要是无效表项就会返回失败，状态为0x09
//	save  :	对无效的表项会进行插入，并同时更新到DDR上
//	update:	对无效的表项不会进行更新，返回失败，状态位0x09
struct yk3_hashtbl_cfg {
	u8  key_len;
	u8  value_len;
	// 桶宽不一定等于容量, 应该允许桶宽小于hash容量
	// 暂时未使用, 先加上这个参数, 可以先设置为等于hash容量
	u32 __todo bucket_width;
	// 副表容量如果小于hash容量, 极端情况下会溢出冲突链, 下发失败
	// 如果是快速同步(快速返回)模式, 副表容量应等于hash容量
	// 如果是严格同步模式, 副表容量可小于hash容量
	u32 sdepth;
	u32 chain_limit;
	u32 hash_seed;

	// k3max hash only
	struct yk3_doe_hie_cache cache;
};

enum yk3_doe_endian {
	YK3_DOE_ENDIAN_LITTLE   = 0, // default
	YK3_DOE_ENDIAN_BIG      = 1,

	YK3_DOE_ENDIAN_DEFT     = YK3_DOE_ENDIAN_LITTLE,
	YK3_DOE_ENDIAN_END
};

#define doe_endian_name(endian) (((endian) == YK3_DOE_ENDIAN_BIG) ? "big" : "little")

struct yk3_tbl_cfg {
	u8              tbl_type;  // yk3_doe_tbl
	u8              tbl_id;
	u8 __deprecated location;
	u8              endian;  // enum yk3_doe_endian, only for counter/meter
	u32             depth;   // 表项个数

	union {
		// 数组表建表参数
		struct {
			u8 value_len;
		} array;

		// counter表建表参数
		// meter表建表参数
		// resv;

		// hash表建表参数
		struct yk3_hashtbl_cfg hash;
	};
};

// 数组表读写参数
struct yk3_array_entry {
	// update: not used
	// query : return value_len @ create table
	u8 __out value_len;
	u8       high_pri;  // 数组写操作时可用
	u32      index;
	union {
		void __inout *_value;      // kernel call use it
		u8 __inout value[128];  // user ioctl call use it
	};
};

enum {
	YK3_SMALL_HASH_KEY_MAXSIZE	= 16,
	YK3_SMALL_HASH_VALUE_MAXSIZE	= 16,
};

// hash表增删改查参数
struct yk3_hash_entry {
	u8 __noused key_len;
	// add/delete/update: not used
	// query : rewrite value_len(the value_len @ create table)
	u8 __out value_len;
	u8       hash_save;  // hash_save标志，仅update操作时可用
	u8       high_pri;   // 仅insert操作可用
	union {
		struct {
			void __inout *_key;    // kernel call use it
			void __inout *_value;  // kernel call use it
		};
		struct {
			u8 __inout key[96];     // user ioctl call use it
			u8 __inout value[128];  // user ioctl call use it
		};
	};
};

// TODO: need counter value ?
//
// counter表写
struct yk3_counter_update {
	u8  enable;
	u8  high_pri;  // 仅update操作可用
	u8  clear_data;
	u32 index;
};

// hardware data layout
//  count is low  8B
//  bytes is high 8B
struct yk3_doe_counter {
	u64 count;
	u64 bytes;
};

#define YK3_DOE_COUNTER_QUERY_PAD       12
#define YK3_DOE_COUNTER_QUERY_BUFSIZE   4096

// hardware data layout
// 1. index is little-endian
// 2. counter's byte order follows the byte order specified when the table was created.
struct yk3_counter_query_entry {
	u32                     index;
	struct yk3_doe_counter	counter;
	u8 __noused pad[YK3_DOE_COUNTER_QUERY_PAD];
} __packed;

#define YK3_DOE_COUNTER_QUERY_VALID_SIZE  offsetof(struct yk3_counter_query_entry, pad)  // 20

struct yk3_counter_query {
	u32 index;
	struct yk3_doe_counter entry;
};

struct yk3_counter_entry {
	struct yk3_counter_update update;
	struct yk3_counter_query  query;
};

// The byte order follows the byte order specified when the table was created.
struct yk3_meter_config {
	u16 cir;
	u32 cbs;
	u16 pir;
	u32 pbs;
	u8  att_factor;
} __packed;

// meter表写
struct yk3_meter_entry {
	u8                  high_pri;  // 仅update操作可用
	u32                 index;
	struct yk3_meter_config config;
};

enum yk3_doe_call_mode {
	YK3_DOE_SYNC_CALL	= 0,
	YK3_DOE_FAST_CALL	= 1,
	YK3_DOE_ASYNC_CALL	= 2,
};

enum yk3_doe_flag {
	YK3_DOE_F_WAIT_SLEEP	= 0x01, // only for sync call
};

struct yk3_tbl_entry {
	u8 tbl_type; /* yk3_doe_tbl */
	u8 tbl_id;

	// yk3_doe_call_mode
	//	only for ioctl, kernel call ignore it
	u8 call_mode;

	// yk3_doe_flag
	u8 flags;

	union {
		// 数组表读写参数
		struct yk3_array_entry array;

		// hash表读增删改查参数
		struct yk3_hash_entry hash;

		// counter表读写
		struct yk3_counter_entry counter;

		// meter表写
		struct yk3_meter_entry meter;
	};
};

enum yk3_doe_action {
	YK3_DOE_TLB_EXISTED,
	YK3_DOE_GET_PROTECT,
	YK3_DOE_SET_PROTECT,
	YK3_DOE_GET_CACHE_INFO,
	YK3_DOE_SET_CACHE_MODE,
	YK3_DOE_GET_HASH_TABLE_MAX,
	YK3_DOE_GET_HASH_ENTRY_COUNT,
	YK3_DOE_GET_COUNTER_ZIP,
	YK3_DOE_SET_COUNTER_ZIP,
	YK3_DOE_GET_HCODE_MODE,
	YK3_DOE_SET_HCODE_MODE,
	YK3_DOE_GET_CACHE_ISOLATION,
	YK3_DOE_SET_CACHE_ISOLATION,

	YK3_DOE_SET_MEASURE, // doe tool private command
	YK3_DOE_ACTION_MAX,
};

// k3/k3max cache spec:
//--------------------------------------------------------------------------------
//               cache-entry-length  k3-cache-entry-count  k3max-cache-entry-count
// array(nomal)  64B(short mode)     2K                    8K
// hash(nomal)   64B(short mode)     2K                    8K
//                |
//                |-max key   len: 64B
//                |-max value len: 64B
//
// array(nomal)  128B(long mode)     1K                    4K
// hash(nomal)   128B(long mode)     1K                    4K
//                |
//                |-max key   len: 96B
//                |-max value len: 128B
//
// hash(small)   16B                 not-support           1K
// counter       16B                 2K                    8K
// meter         32B                 1K                    8K
//--------------------------------------------------------------------------------

// set cache mode: YK3_DOE_SET_CACHE_MODE
// only for hash(normal) and array(nomal)
struct yk3_tbl_ctl_cache_mode {
	u8 tbl_type;
	u8 mode; // 0: long mode(default), 1: short mode
};

// get cache info: YK3_DOE_GET_CACHE_INFO
// exclude small array
struct yk3_tbl_ctl_cache_info {
	u8 tbl_type;
	// 0: long mode(default), 1: short mode
	// just for hash(normal) and array(nomal), ignore it @ other table
	u8  __out mode;
	// only for hash(nomal/small)
	// ignore it @ other table
	u32 __out max_key_len;
	// for all table
	u32 __out max_value_len;
	// max  cache entry count
	u32 __out cache_entry_limit;
	// left cache entry count, just for non-ddr
	u32 __out cache_entry_count;
};

enum { K3MAX_HASH_TABLE_LIMIT = 16 };

struct yk3_tbl_ctl_hash_entry_count {
	bool is_small;

	union {
		u32 raw;

		struct {
#if defined(YK3_LITTLE_ENDIAN)
			u32 tbl_id: 8;
			u32 count: 24;
#elif defined(YK3_BIG_ENDIAN)
			u32 count: 24;
			u32 tbl_id: 8;
#endif
		};
	} stat[K3MAX_HASH_TABLE_LIMIT];
};

enum yk3_tbl_hcode_mode {
	// 使用全部key计算hcode
	YK3_HCODE_KEY_ALL,

	// 根据表容量, 选择key/index的第n位作为hcode
	// non-ddr模式下能确保不冲突
	YK3_HCODE_KEY_16B,

	YK3_HCODE_KEY_END
};

struct yk3_tbl_ctl {
	u8 action_type; /* yk3_doe_action */

	union {
		// YK3_DOE_TLB_EXISTED
		struct {
			u8 tbl_id;
		} tbl_existed;

		// YK3_DOE_GET_PROTECT/YK3_DOE_SET_PROTECT
		struct {
			// 0: np protect disable/off
			// 1: np protect enable/on,
			u8 status;
		} protect;

		// YK3_DOE_GET_COUNTER_ZIP/YK3_DOE_SET_COUNTER_ZIP
		// counter zip mode, only support k3max
		struct {
			// 0: unzip mode(default)
			// 1: zip mode
			u8 zip;
		} counter;

		// YK3_DOE_GET_HCODE_MODE/YK3_DOE_SET_HCODE_MODE
		struct {
			// only support
			//	array
			//	small-array
			//	hash
			//	counter
			//	meter
			u8 tbl_type;
			u8 mode;	// enum yk3_tbl_hcode_mode
		} hcode_mode;

		// YK3_DOE_GET_CACHE_ISOLATION/YK3_DOE_SET_CACHE_ISOLATION
		//	only support k3max hash table
		u8 cache_isolation;
		struct yk3_tbl_ctl_cache_mode cache_mode; // YK3_DOE_SET_CACHE_MODE
		struct yk3_tbl_ctl_cache_info cache_info; // YK3_DOE_GET_CACHE_INFO

		// YK3_DOE_GET_HASH_ENTRY_COUNT
		struct yk3_tbl_ctl_hash_entry_count hash_entry_count;
	};
};

struct yk3_doe_batch {};
/******************************************************************************/
#endif /* _YK3_COMMON_H */
