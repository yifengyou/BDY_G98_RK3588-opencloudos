/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_DOE_PROC_H_
#define __YK3_DOE_PROC_H_
#include <linux/llist.h>
#include <linux/types.h>
#include "yk3_base.h"
#include "yk3_chip.h"
#include "yk3_doe.h"
#include "yk3_irq.h"
#include "yk3_reg.h"
#include "yk3_debug.h"
#include "yk3_emp_priv.h"
#include "yk3_cdev_priv.h"
#include "yk3_doe_hw.h"
#include "yk3_doe_kapi.h"
/******************************************************************************/
#ifndef YK3_SKIP_DOE
#define YK3_SKIP_DOE		0
#endif

#ifndef YK3_DOE_REMOVE
#define YK3_DOE_REMOVE		0
#endif

#ifndef YK3_DOE_NOT_USED
#define YK3_DOE_NOT_USED	0
#endif

#ifndef YK3_DOE_RAW_CMD
#define YK3_DOE_RAW_CMD		0
#endif

#ifndef YK3_DOE_CMD_ZOMBIE
#define YK3_DOE_CMD_ZOMBIE	0
#endif

#ifndef YK3_DOE_LAIE_MORE_TIMES
#define YK3_DOE_LAIE_MORE_TIMES 9
#endif

#ifndef YK3_DOE_TEST
#define YK3_DOE_TEST		0
#endif

#ifndef YK3_DOE_MEASURE
#define YK3_DOE_MEASURE		0
#endif

#ifndef YK3_DOE_MEASURE_N
#define YK3_DOE_MEASURE_N	128
#endif

#ifndef YK3_DOE_TODO
#define YK3_DOE_TODO		0
#endif

#define YK3_DOE_IRQ_RAW		0
#define YK3_DOE_IRQ_THREAD	1
#define YK3_DOE_IRQ_TASKLET	2
#define YK3_DOE_IRQ_THREAD_REAL	3

#ifndef YK3_DOE_IRQ
#define YK3_DOE_IRQ		YK3_DOE_IRQ_TASKLET
#endif

#ifndef YK3_DOE_IRQ_TIMES
#define YK3_DOE_IRQ_TIMES	10
#endif

#ifndef YK3_DOE_IRQ_LOSS
#define YK3_DOE_IRQ_LOSS	0
#endif

#ifndef YK3_DOE_IRQ_BH
#define YK3_DOE_IRQ_BH		0
#endif

#ifndef YK3_DOE_DOORBELL_REUSE
#define YK3_DOE_DOORBELL_REUSE	0
#endif

#define YK3_DOE_FAST_CMD_MAX	(YK3_DOE_CMD_BUFFER_MAXSIZE / YK3_DOE_CMD_MAXSIZE) // 204

#ifndef YK3_DOE_FAST_CMD_BATCH_MAX
#define YK3_DOE_FAST_CMD_BATCH_MAX	128
#endif

#if YK3_DOE_FAST_CMD_BATCH_MAX > YK3_DOE_FAST_CMD_MAX
#error "too big YK3_DOE_FAST_CMD_BATCH_MAX"
#endif

#ifndef YK3_DOE_FAST_TIMER_BUSY_INTERVAL
#define YK3_DOE_FAST_TIMER_BUSY_INTERVAL	(200 * 1000) // 200 us
#endif

#ifndef YK3_DOE_FAST_TIMER_SLOW_INTERVAL
#define YK3_DOE_FAST_TIMER_SLOW_INTERVAL	(100 * 1000 * 1000) // 100 ms
#endif

struct yk3_doe_if;

#if YK3_DOE_U200
#define is_doe_pdev_pf(pdev_priv)	false
#define is_doe_pdev_vf(pdev_priv)	false
#define is_doe_pdev_mgr(pdev_priv)	true
#define is_doe_pdev_rdma(pdev_priv)	false
#else
#define is_doe_pdev_pf(pdev_priv)	yk3_pdev_is_pf(pdev_priv)
#define is_doe_pdev_vf(pdev_priv)	yk3_pdev_is_vf(pdev_priv)
#define is_doe_pdev_mgr(pdev_priv)	yk3_pdev_is_mgr(pdev_priv)
#define is_doe_pdev_rdma(pdev_priv)	yk3_pdev_is_rdma(pdev_priv)
#endif
/******************************************************************************/
#define DOE_FMT(fmt)	"DOE " fmt

#define doe_emerg(fmt, args...)	yk3_emerg(DOE_FMT(fmt) YK3_CRLF, ##args)
#define doe_alert(fmt, args...)	yk3_alert(DOE_FMT(fmt) YK3_CRLF, ##args)
#define doe_crit(fmt, args...)  yk3_crit(DOE_FMT(fmt) YK3_CRLF, ##args)
#define doe_err(fmt, args...)   yk3_err(DOE_FMT(fmt) YK3_CRLF, ##args)
#define doe_warn(fmt, args...)  yk3_warn(DOE_FMT(fmt) YK3_CRLF, ##args)
#define doe_info(fmt, args...)  yk3_info(DOE_FMT(fmt) YK3_CRLF, ##args)

#define doe_error(err, fmt, args...) ({ \
	int m_err = (err); \
	doe_err("ERROR[%d] " fmt, m_err, ##args); \
	m_err; \
})

#define doe_bug(err, fmt, args...)	({ WARN(1, "DOE-BUG: " fmt, ##args); (err); })
#define doe_bug_once(err, fmt, args...)	({ WARN_ONCE(1, "DOE-BUG: " fmt, ##args); (err); })
/******************************************************************************/
#define DOE_LEVEL_COMMAND	0x0001
#define DOE_LEVEL_EVENT		0x0002
#define DOE_LEVEL_BUFFER	0x0004
#define DOE_LEVEL_TABLE		0x0008
#define DOE_LEVEL_RESULT	0x0010
#define DOE_LEVEL_CRIT		0x0020
#define DOE_LEVEL_TAG		0x0040
#define DOE_LEVEL_CB		0x0080

#define DOE_LEVEL_TRACE		0x0100
#define DOE_LEVEL_SYNC		0x0200
#define DOE_LEVEL_READ		0x0400
#define DOE_LEVEL_WRITE		0x0800
#define DOE_LEVEL_INIT		0x1000

#if YK3_DEBUG
#define DOE_DEBUG_RET		DOE_LEVEL_RESULT
#define DOE_DEBUG_TBL		DOE_LEVEL_TABLE
#define DOE_DEBUG_TAG		DOE_LEVEL_TAG
#define DOE_DEBUG_CRIT		DOE_LEVEL_CRIT
#define DOE_DEBUG_INIT		DOE_LEVEL_INIT

#define DOE_DEBUG_CTR		(DOE_LEVEL_COMMAND | DOE_LEVEL_TRACE | DOE_LEVEL_READ)
#define DOE_DEBUG_CTW		(DOE_LEVEL_COMMAND | DOE_LEVEL_TRACE | DOE_LEVEL_WRITE)
#define DOE_DEBUG_CSR		(DOE_LEVEL_COMMAND | DOE_LEVEL_SYNC | DOE_LEVEL_READ)
#define DOE_DEBUG_CSW		(DOE_LEVEL_COMMAND | DOE_LEVEL_SYNC | DOE_LEVEL_WRITE)
#define DOE_DEBUG_CMB		(DOE_LEVEL_COMMAND | DOE_LEVEL_BUFFER)
#define DOE_DEBUG_CBR		(DOE_LEVEL_CB | DOE_LEVEL_BUFFER | DOE_LEVEL_READ)
#define DOE_DEBUG_CBW		(DOE_LEVEL_CB | DOE_LEVEL_BUFFER | DOE_LEVEL_WRITE)

#define DOE_DEBUG_ESR		(DOE_LEVEL_EVENT | DOE_LEVEL_SYNC | DOE_LEVEL_READ)
#define DOE_DEBUG_ESW		(DOE_LEVEL_EVENT | DOE_LEVEL_SYNC | DOE_LEVEL_WRITE)
#define DOE_DEBUG_ETR		(DOE_LEVEL_EVENT | DOE_LEVEL_TRACE | DOE_LEVEL_READ)
#define DOE_DEBUG_ETW		(DOE_LEVEL_EVENT | DOE_LEVEL_TRACE | DOE_LEVEL_WRITE)
#define DOE_DEBUG_EBR		(DOE_LEVEL_EVENT | DOE_LEVEL_BUFFER | DOE_LEVEL_READ)
#define DOE_DEBUG_EBW		(DOE_LEVEL_EVENT | DOE_LEVEL_BUFFER | DOE_LEVEL_WRITE)

extern unsigned long doe_debug_level;
extern unsigned long doe_error_inject;

static inline bool
is_doe_debug_level(unsigned long level) {
	return level == (doe_debug_level & level);
}

static inline bool
is_doe_error_inject(unsigned long inject) {
	return inject == (doe_error_inject & inject);
}

#define doe_debug(level, fmt, args...) do { \
	if (is_doe_debug_level(level)) { \
		/* use doe_info */ \
		doe_info(fmt, ##args); \
	} \
} while (0)

#define doe_debug_buffer(level, buf, len, fmt, args...)  do { \
	if (is_doe_debug_level(level)) { \
		/* use doe_info */ \
		doe_info(fmt, ##args); \
		yk3_dump_buffer(buf, len); \
	} \
} while (0)
#else
#define DOE_DEBUG_RET		"RET"
#define DOE_DEBUG_TBL		"TBL"
#define DOE_DEBUG_TAG		"TAG"
#define DOE_DEBUG_CRIT		"CRT"
#define DOE_DEBUG_INIT		"INI"

#define DOE_DEBUG_CTR		"CTR"
#define DOE_DEBUG_CTW		"CTW"
#define DOE_DEBUG_CSR		"CSR"
#define DOE_DEBUG_CSW		"CSW"
#define DOE_DEBUG_CMB		"CMB"
#define DOE_DEBUG_CBR		"CBR"
#define DOE_DEBUG_CBW		"CBW"

#define DOE_DEBUG_ESR		"ESR"
#define DOE_DEBUG_ESW		"ESW"
#define DOE_DEBUG_ETR		"ESR"
#define DOE_DEBUG_ETW		"ESW"
#define DOE_DEBUG_EBR		"EBR"
#define DOE_DEBUG_EBW		"EBW"

#define doe_debug(level, fmt, args...)	yk3_debug("DOE [" level "]" fmt, ##args)
#define doe_debug_buffer(level, buf, len, fmt, args...)  yk3_do_nothing()
#endif

#define doe_debug_tmp(fmt, args...)	doe_info(fmt, ##args)
/******************************************************************************/
#define DOE_INJECT_READ_TIMEOUT		0x0001
#define DOE_INJECT_WRITE_TIMEOUT	0x0001

#define DOE_INJECT_RD_IRQ_SKIP		0x0010
#define DOE_INJECT_WR_IRQ_SKIP		0x0020

#define is_doe_inject(flag)		is_doe_error_inject(flag)
#define is_doe_inject_cmd_timeout_r()	is_doe_inject(DOE_INJECT_READ_TIMEOUT)
#define is_doe_inject_cmd_timeout_w()	is_doe_inject(DOE_INJECT_WRITE_TIMEOUT)
#define is_doe_inject_cmd_timeout(is_read) \
	((is_read) ? is_doe_inject_cmd_timeout_r() : is_doe_inject_cmd_timeout_w())

#define is_doe_inject_irq_skip_r()	is_doe_inject(DOE_INJECT_RD_IRQ_SKIP)
#define is_doe_inject_irq_skip_w()	is_doe_inject(DOE_INJECT_WR_IRQ_SKIP)
#define is_doe_inject_irq_skip(is_read) \
	((is_read) ? is_doe_inject_irq_skip_r() : is_doe_inject_irq_skip_w())
/******************************************************************************/
#if YK3_DOE_IRQ_BH && (YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET)
#define spin_lock_by(lock, in_irq, flags) do { \
	typeof(lock) m_lock = (lock); \
	(void)(flags); \
	if (in_irq) \
		spin_lock(m_lock); \
	else \
		spin_lock_bh(m_lock); \
} while (0)

#define spin_unlock_by(lock, in_irq, flags) do { \
	typeof(lock) m_lock = (lock); \
	(void)(flags); \
	if (in_irq) \
		spin_unlock(m_lock); \
	else \
		spin_unlock_bh(m_lock); \
} while (0)
#else
#define spin_lock_by(lock, in_irq, flags) do { \
	typeof(lock) m_lock = (lock); \
	if (in_irq) \
		spin_lock(m_lock); \
	else \
		spin_lock_irqsave(m_lock, flags); \
} while (0)

#define spin_unlock_by(lock, in_irq, flags) do { \
	typeof(lock) m_lock = (lock); \
	if (in_irq) \
		spin_unlock(m_lock); \
	else \
		spin_unlock_irqrestore(m_lock, flags); \
} while (0)
#endif
/******************************************************************************/
#define DOE_CMD_MULTI_TIMEOUT		(3 * NSEC_PER_SEC)	// 3s
#define DOE_CMD_SINGLE_TIMEOUT		(500 * NSEC_PER_MSEC)	// 500ms

#define DOE_TAG_TIMEOUT			(50 * NSEC_PER_MSEC)	// 50ms
#define DOE_OTF_TIMEOUT			(50 * NSEC_PER_MSEC)	// 50ms
#define DOE_CB_POOL_TIMEOUT		(50 * NSEC_PER_MSEC)	// 50ms

static inline bool
__is_doe_timeout(u64 ns_now, u64 ns_start, u64 ns_timeout) {
	return (ns_now - ns_start) > ns_timeout;
}

static inline bool
is_doe_timeout(u64 ns_start, u64 ns_timeout) {
	return __is_doe_timeout(ktime_get_ns(), ns_start, ns_timeout);
}

static inline bool
is_doe_hash(int tbl_type) {
	return tbl_type == DOE_TABLE_BIG_HASH || tbl_type == DOE_TABLE_SMALL_HASH;
}

#define is_doe_array(tbl_type)	(!is_doe_hash(tbl_type))
/******************************************************************************/
struct yk3_doe_table_map {
	const char *name;
	enum yk3_doe_xie xie;
};

#define YK3_DOE_TABLE_MAP(_name, _xie) { \
	.name = _name, \
	.xie = _xie, \
}

#define YK3_DOE_TABLE_MAP_INITER { \
	[DOE_TABLE_NORMAL_ARRAY] = YK3_DOE_TABLE_MAP("array", YK3_DOE_AIE), \
	[DOE_TABLE_BIG_HASH]	 = YK3_DOE_TABLE_MAP("hash", YK3_DOE_HIE), \
	[DOE_TABLE_COUNTER]	 = YK3_DOE_TABLE_MAP("counter", YK3_DOE_CIE), \
	[DOE_TABLE_METER]	 = YK3_DOE_TABLE_MAP("meter", YK3_DOE_MIE), \
	[DOE_TABLE_LOCK]	 = YK3_DOE_TABLE_MAP("lock", YK3_DOE_AIE), \
	[DOE_TABLE_SMALL_HASH]	 = YK3_DOE_TABLE_MAP("small-hash", YK3_DOE_LHIE), \
	[DOE_TABLE_SMALL_ARRAY]	 = YK3_DOE_TABLE_MAP("small-array", YK3_DOE_LAIE), \
} /* end */

extern const struct yk3_doe_table_map yk3_doe_table_maps[];

static inline enum yk3_doe_xie
doe_tbl_type_to_xie(enum yk3_doe_tbl tbl_type) {
	return yk3_doe_table_maps[tbl_type].xie;
}

static inline const char *
doe_tbl_name(enum yk3_doe_tbl tbl_type) {
	return yk3_doe_table_maps[tbl_type].name;
}

static inline const struct yk3_doe_xie_spec *
doe_tbl_type_to_xie_spec(enum yk3_doe_tbl tbl_type) {
	enum yk3_doe_xie xie = doe_tbl_type_to_xie(tbl_type);

	return &yk3_doe_xie_specs[xie];
}

/******************************************************************************/
#define POLLING_TIMEDOUT		100000000ULL	/* 100ms */
#define POLLING_WORK_SCHEDULE_TIME	10000000000ULL	/* 10s */

// Virtual Flow Control
//
// input : push cmd  into hardware, unit: command
// output: pop event from hareware, unit: event block
struct yk3_doe_vfc {
	u64 depth; // event block count
	u64 mask;

	// total cmd count after enqueue fifo
	u64 prod;

	// total event count
	//
	// init: 0
	// get @ enqueue fifo
	// set @ enqueue fifo (when NOT enough)
	u64 cons;

	u64 times_cons_update;
} __cacheline;

static inline void
doe_vfc_init(struct yk3_doe_vfc *vfc, u64 depth) {
	vfc->depth = depth;
	vfc->mask = depth - 1;
	vfc->prod = 0;
	vfc->cons = 0;
}

// cmd count on-the-fly
static inline u64
doe_vfc_count(struct yk3_doe_vfc *vfc) {
	return vfc->prod - vfc->cons;
}

// like ++x
// call @ doe_fifo_enqueue
//	in fifo lock, NOT need WRITE_ONCE
static inline void
doe_vfc_prod_add(struct yk3_doe_vfc *vfc, u64 n) {
	vfc->prod += n;
}

// call @ doe_fifo_enqueue (when NOT enough)
//	in fifo lock, NOT need WRITE_ONCE
//
// cons: get by doe_evq_event_count(evq), when NOT enough
static inline void
doe_vfc_cons_set(struct yk3_doe_vfc *vfc, u64 cons) {
	vfc->cons = cons;
	vfc->times_cons_update++;
}

/******************************************************************************/
// depth is YK3_DOE_CMD_TAG_COUNT
struct doe_tag_pool {
	u16 *tags;

	u64 prod;
	u64 cons;
} __cacheline;

static inline void
doe_tag_pool_dump(struct doe_tag_pool *pool) {
	doe_debug(DOE_DEBUG_TAG, "tag pool: prod[%llu] cons[%llu]", pool->prod, pool->cons);
}

static inline void
doe_tag_pool_fini(struct doe_tag_pool *pool) {
	kfree(pool->tags);
}

static inline int
doe_tag_pool_init(struct doe_tag_pool *pool) {
	u16 *tags;
	int tag;

	tags = kcalloc(YK3_DOE_CMD_TAG_COUNT, sizeof(u16), GFP_KERNEL);
	if (!tags)
		return doe_error(-E_DOE_NOMEM, "tag pool alloc failed");

	for (tag = 0; tag < YK3_DOE_CMD_TAG_COUNT; tag++)
		tags[tag] = tag;

	pool->prod	= YK3_DOE_CMD_TAG_COUNT;
	pool->cons	= 0;
	pool->tags	= tags;

	return 0;
}

static inline u64
doe_tag_pool_count(struct doe_tag_pool *pool) {
	return pool->prod - pool->cons;
}

static inline u64
doe_tag_pool_left(struct doe_tag_pool *pool) {
	return YK3_DOE_CMD_TAG_COUNT - doe_tag_pool_count(pool);
}

static inline bool
is_doe_tag_pool_empty(struct doe_tag_pool *pool) {
	return doe_tag_pool_count(pool) == 0;
}

static inline bool
is_doe_tag_pool_full(struct doe_tag_pool *pool) {
	return doe_tag_pool_count(pool) == YK3_DOE_CMD_TAG_COUNT;
}

static inline u16
doe_tag_pool_get(struct doe_tag_pool *pool) {
	return pool->tags[pool->cons++ & YK3_DOE_CMD_TAG_MASK];
}

static inline void
doe_tag_pool_put(struct doe_tag_pool *pool, u16 tag) {
	pool->tags[pool->prod++ & YK3_DOE_CMD_TAG_MASK] = tag;
}

/******************************************************************************/
struct yk3_doe_cmd_manager_st {
	atomic64_t bug_get_with_pool_disable;
	atomic64_t bug_put_with_pool_disable;
	atomic64_t bug_put_with_pool_full;
	atomic64_t bug_unregister_with_pool_full;
	atomic64_t get_with_pool_empty;
	atomic64_t register_with_pool_empty;
	atomic64_t err_alloc_nil;
} __cacheline;

struct yk3_doe_cmd;

struct yk3_doe_tag_slot {
	struct yk3_doe_cmd *pool;
	struct yk3_doe_cmd *bind;
	u32 times_put;
	u32 times_get;
	u32 times_unknown;
	u32 times_timeout;
	u32 times_lated;

	// cmd info
	u64 seq;
	union {
		u64 raw;

		struct {
			u8 opcode;
			u8 tbl_id;
			u16 flags;
			u32 index;
		};
	};
} __cacheline;

static inline void
doe_tag_slot_clean(struct yk3_doe_tag_slot *slot) {
	slot->seq = 0;
	slot->raw = 0;
}

static inline void
doe_tag_slot_dump(struct yk3_doe_tag_slot *slots) {
	struct yk3_doe_tag_slot *slot;
	int tag;

	for (tag = 0; tag < YK3_DOE_CMD_TAG_COUNT; tag++) {
		slot = &slots[tag];

		if (slot->times_put != slot->times_get ||
		    slot->times_unknown ||
		    slot->times_timeout ||
		    slot->times_lated) {
			// {}
			doe_info("tag[0x%04x] get[%u] put[%u] unknown[%u] timeout[%u] late[%u]",
				 tag, slot->times_get, slot->times_put,
				 slot->times_unknown, slot->times_timeout, slot->times_lated);
		}
	}
}

struct yk3_doe_cmd_manager {
	struct doe_tag_pool		tag_pool;
	struct yk3_doe_cmd_manager_st	st;

	struct {
		atomic64_t		seq;
		struct yk3_doe_tag_slot *tag_slot;  // index is tag
		bool			pool_enable;
		spinlock_t		lock; // for tag and trace
	} __cacheline;
};

/******************************************************************************/
struct yk3_doe_evq_desc {
	__le64 dma_address;	// k3/k3max used

	__le32 dma_wr_cnt_out;	// k3max used as fifo cons, k3 ignore
	__le32 dma_rd_cnt_out;	// k3max used as fifo cons, k3 ignore

	__le16 dma_wr_buf_space;// k3/k3max ignore
	__le16 dma_rd_buf_space;// k3/k3max ignore
} __cacheline;

// only k3max
static inline u32
doe_evq_fifo_cons(struct yk3_doe_evq_desc *desc, bool is_read) {
	__le32 prod;

	if (is_read)
		prod = READ_ONCE(desc->dma_rd_cnt_out);
	else
		prod = READ_ONCE(desc->dma_wr_cnt_out);

	return le32_to_cpu(prod);
}

// hw product, unit: event block
// sw consume, unit: event block
struct yk3_doe_evq {
	// readonly {
	dma_addr_t dma_base;	// buffer
	void *base;

	dma_addr_t dma_desc;
	struct yk3_doe_evq_desc *desc;

	u32 depth;
	u32 mask;
	u32 block_size;
	u16 block_order;
	u16 batch_count; // batch event count @ event block
	// readonly  }

	// consumed/handled event count
	//
	// read/write @ irq
	// read @ api
	u64 event_count;

	// consumed/handled event block count
	//
	// read/write @ irq
	// read @ api
	u64 cons;
} __cacheline;

// producted event block count
//
// update by haredware
// read @ irq and api
static inline u32
doe_evq_prod(struct yk3_doe_evq *evq) {
	__le64	dma_prod = READ_ONCE(evq->desc->dma_address);
	u64	dma_base = (u64)evq->dma_base;

	return (le64_to_cpu(dma_prod) - dma_base) >> evq->block_order;
}

// unit: event block
static inline u32
doe_evq_cons(struct yk3_doe_evq *evq) {
	return READ_ONCE(evq->cons) & evq->mask;
}

// used event block count
//
// maybe call by event handler
// maybe call by command handler
static inline u32
doe_evq_count(struct yk3_doe_evq *evq) {
	u32 prod = doe_evq_prod(evq);
	u32 cons = doe_evq_cons(evq);

	// (prod + depth - cons) & mask
	//
	// prod >= cons
	//	==> (prod + depth - cons) & mask
	//	==> (prod - cons + depth) & mask
	//	==> (prod - cons) & mask
	//
	// prod < cons
	//	==> (prod + depth - cons) & mask
	return (prod + evq->depth - cons) & evq->mask;
}

// left/free event block count
//	prod is auto rollback, should keep one slot !!!
static inline u32
doe_evq_left(struct yk3_doe_evq *evq) {
	return evq->depth - doe_evq_count(evq);
}

// consumed/handled event count
static inline u64
doe_evq_event_count(struct yk3_doe_evq *evq) {
	return READ_ONCE(evq->event_count);
}

// consume n event and 1 block
static inline void
doe_evq_consume(struct yk3_doe_evq *evq, u32 n_event) {
	WRITE_ONCE(evq->cons, evq->cons + 1);
	WRITE_ONCE(evq->event_count, evq->event_count + n_event);
}

/******************************************************************************/
union yk3_doe_evq_block_r {
	u8 _[YK3_DOE_EVQ_BLOCK_SIZE_R];

	struct yk3_doe_event hdr;
};

struct yk3_doe_evq_block_w {
	__le32 count;

	struct yk3_doe_event events[];
};

union yk3_doe_qvq_block {
	void *addr;
	union yk3_doe_evq_block_r *r;
	struct yk3_doe_evq_block_w *w;
};

static inline void *
doe_evq_block_address(struct yk3_doe_evq *evq, u32 pos) {
	return (char *)evq->base + (pos * evq->block_size);
}

/******************************************************************************/
#define YK3_CB_RESV	YK3_DOE_CMD_MAXSIZE

struct yk3_doe_cmd_buffer {
	// read only
	dma_addr_t dma_base;
	void *base;
	u32 size; // buffer size
	u32 id;

	// read & write
	union {
		u64 cmd_info;

		struct {
			u32 cmd_space;
			u32 cmd_count;
		};
	};
};

static inline void
doe_cb_clean(struct yk3_doe_cmd_buffer *cb) {
	// cb->cmd_space = 0;
	// cb->cmd_count = 0;
	cb->cmd_info = 0;
}

static inline u32
doe_cb_left(struct yk3_doe_cmd_buffer *cb) {
	return cb->size - cb->cmd_space;
}

static inline bool
is_doe_cb_enough(struct yk3_doe_cmd_buffer *cb) {
	return doe_cb_left(cb) >= YK3_CB_RESV;
}

static inline void *
doe_cb_cursor(struct yk3_doe_cmd_buffer *cb) {
	return cb->base + cb->cmd_space;
}

static inline void
doe_cb_push(struct yk3_doe_cmd_buffer *cb, u32 data_len) {
	cb->cmd_count++;

	cb->cmd_space += DOE_CMD_SPACE(data_len);
}

/******************************************************************************/
// hw_cons: read from reg_cons
#define DOE_FIFO_CONS(hw_cons)	((hw_cons) - YK3_DOE_FIFO_RESV)
#if YK3_DOE_FIFO_NEW
static inline u32
DOE_FIFO_DIFF(u32 new_cons, u32 old_cons) {
	if (new_cons >= old_cons)
		return (new_cons - old_cons);

	return (new_cons + YK3_DOE_FIFO_DEPTH - old_cons);
}
#else
#define DOE_FIFO_DIFF(new, old)	(((new) + YK3_DOE_FIFO_DEPTH - (old)) & YK3_DOE_FIFO_MASK)
#endif

struct yk3_doe_fifo {
	struct yk3_doe_cmd_buffer **pool; // depth is YK3_DOE_FIFO_DEPTH

	u64 times_dequeue;
	u64 times_cons_update;
	u32 prod; // sw prod.
	u32 cons; // hw cons.
	u32 init_cons;	// init value(read from reg_cons)
	u32 reg_cons;	// only k3: dma_rd_cnt_out or dma_wr_cnt_out

	spinlock_t lock; // fifo lock, before/after kick doorbel
} __cacheline;

static inline u32
doe_fifo_count(struct yk3_doe_fifo *fifo) {
	return fifo->prod - fifo->cons;
}

static inline u32
doe_fifo_left(struct yk3_doe_fifo *fifo) {
	return YK3_DOE_FIFO_DEPTH - doe_fifo_count(fifo);
}

static inline bool
is_doe_fifo_empty(struct yk3_doe_fifo *fifo) {
	return doe_fifo_left(fifo) == YK3_DOE_FIFO_EMPTY;
}

// is full, then read reg(dma_rd_cnt_out/dma_wr_cnt_out) and update cons
static inline bool
is_doe_fifo_full(struct yk3_doe_fifo *fifo) {
	return doe_fifo_count(fifo) == YK3_DOE_FIFO_FULL;
}

static inline bool
is_doe_fifo_enough(struct yk3_doe_fifo *fifo) {
	return doe_fifo_count(fifo) < YK3_DOE_FIFO_FULL;
}

static inline void
__doe_fifo_enqueue(struct yk3_doe_fifo *fifo, struct yk3_doe_cmd_buffer *cb) {
	fifo->pool[fifo->prod++ & YK3_DOE_FIFO_MASK] = cb;
}

int doe_fifo_enqueue(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb);

int doe_fifo_init(struct yk3_doe_if *doe_if);
void doe_fifo_fini(struct yk3_doe_if *doe_if);
/******************************************************************************/
struct yk3_doe_cb_pool {
	struct yk3_doe_cmd_buffer **pool;
	struct yk3_doe_cmd_buffer *cb;

	struct {
		dma_addr_t dma_base;
		void *base;
	} single;

	u64 prod;
	u64 cons;
	u32 cmd_buffer_size;
	spinlock_t lock; // lock

	struct {
		atomic64_t bug_cb_pool_put_with_not_enough;
	} st;
} __cacheline;

static inline u32
doe_cb_pool_count(struct yk3_doe_cb_pool *pool) {
	return pool->prod - pool->cons;
}

static inline u32
doe_cb_pool_left(struct yk3_doe_cb_pool *pool) {
	return YK3_DOE_CB_POOL_DEPTH - doe_cb_pool_count(pool);
}

static inline bool
is_doe_cb_pool_empty(struct yk3_doe_cb_pool *pool) {
	return doe_cb_pool_count(pool) == 0;
}

static inline bool
is_doe_cb_pool_full(struct yk3_doe_cb_pool *pool) {
	return doe_cb_pool_count(pool) == YK3_DOE_CB_POOL_DEPTH;
}

static inline bool
is_doe_cb_pool_enough(struct yk3_doe_cb_pool *pool, u32 n) {
	return doe_cb_pool_left(pool) >= n;
}

int doe_cb_pool_get(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer **p_cb);
void doe_cb_pool_put(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb);
void doe_cb_pool_put_ex(struct yk3_doe_if *doe_if, struct yk3_doe_fifo *fifo, u32 start, u32 count);

int doe_cb_pool_init(struct yk3_doe_if *doe_if);
void doe_cb_pool_fini(struct yk3_doe_if *doe_if);
/******************************************************************************/
struct yk3_doe_priv;

struct yk3_doe_if_st {
	atomic64_t irq_trigger;
	u64 event_unknown;
	u64 event_again;
	u64 event_left;
} __cacheline;

union yk3_doe_doorbell {
	u64 raw[2];

	struct {
		u32 addr_L;
		u32 addr_H;
		u32 len;
		u32 control;
	};
};

static inline void
doe_doorbell_init(union yk3_doe_doorbell *db, bool is_read) {
	if (is_read) {
		db->addr_L	= YK3_DOE_REG_RD_CMD_ADDR_L;
		db->addr_H	= YK3_DOE_REG_RD_CMD_ADDR_H;
		db->len		= YK3_DOE_REG_RD_CMD_LEN;
		db->control	= YK3_DOE_REG_RD_CMD_CONTROL;
	} else {
		db->addr_L	= YK3_DOE_REG_WR_CMD_ADDR_L;
		db->addr_H	= YK3_DOE_REG_WR_CMD_ADDR_H;
		db->len		= YK3_DOE_REG_WR_CMD_LEN;
		db->control	= YK3_DOE_REG_WR_CMD_CONTROL;
	}
}

static inline void
doe_doorbell_copy(union yk3_doe_doorbell *dst, union yk3_doe_doorbell *src) {
	dst->raw[0] = src->raw[0];
	dst->raw[1] = src->raw[1];
}

/******************************************************************************/
static inline u32
doe_get_order(u32 len) {
	return len ? ilog2(roundup_pow_of_two(len)) : 0;
}

struct yk3_doe_if_cfg {
	u32 cmd_buffer_size;
	u32 evq_depth_min;
	u32 evq_depth;
	u32 evq_block_size;
	u16 evq_block_order;
	u16 evq_batch_count; // event batch count per block

	union yk3_doe_irq_coalesce irq_coalesce;
};

struct yk3_doe_cfg {
	struct yk3_doe_if_cfg if_cfg[2];

	u32 sec_cmd_zombie;
	enum yk3_doe_cache_mode cache_mode;
};

static inline void
doe_cfg_repair(struct yk3_doe_cfg *cfg) {
	struct yk3_doe_if_cfg *if_cfg;
	int i;

	for (i = 0; i < 2; i++) {
		if_cfg = &cfg->if_cfg[i];

		if_cfg->evq_block_order = doe_get_order(if_cfg->evq_block_size);
	}
}

#define DOE_IF_CFG_INITER_R { \
	.cmd_buffer_size = YK3_DOE_CMD_BUFFER_SIZE_R, \
	.evq_depth_min = YK3_DOE_EVQ_DEPTH_MIN_R, \
	.evq_depth = YK3_DOE_EVQ_DEPTH_R, \
	.evq_block_size = YK3_DOE_EVQ_BLOCK_SIZE_R, \
	.evq_batch_count = YK3_DOE_EVQ_BATCH_COUNT_R, \
	.irq_coalesce = { \
		.timeout = YK3_DOE_IRQ_COALESCE_TIMEOUT_R, \
		.count = YK3_DOE_IRQ_COALESCE_COUNT_R, \
	}, \
}

#define DOE_IF_CFG_INITER_W { \
	.cmd_buffer_size = YK3_DOE_CMD_BUFFER_SIZE_W, \
	.evq_depth_min = YK3_DOE_EVQ_DEPTH_MIN_W, \
	.evq_depth = YK3_DOE_EVQ_DEPTH_W, \
	.evq_block_size = YK3_DOE_EVQ_BLOCK_SIZE_W, \
	.evq_batch_count = YK3_DOE_EVQ_BATCH_COUNT_W, \
	.irq_coalesce = { \
		.timeout = YK3_DOE_IRQ_COALESCE_TIMEOUT_W, \
		.count = YK3_DOE_IRQ_COALESCE_COUNT_W, \
	}, \
}

#define DOE_CFG_INITER { \
	.if_cfg = { \
		[YK3_DOE_RD] = DOE_IF_CFG_INITER_R, \
		[YK3_DOE_WR] = DOE_IF_CFG_INITER_W, \
	}, \
	.sec_cmd_zombie = YK3_DOE_CMD_ZOMBIE_SEC, \
	.cache_mode = YK3_DOE_CACHE_MODE_DEFT, \
}

extern struct yk3_doe_cfg doe_cfg;
/******************************************************************************/
// DOE intf size: 768
struct yk3_doe_if {
	// read only
	const char		*name;
	struct yk3_doe_priv	*doe_priv;
	bool			is_read;
	int			irq_vector;
#if YK3_DOE_ONLY
	int			irq_number;
#endif
#if YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
	struct tasklet_struct	tasklet;
#endif
#if K2U
	struct notifier_block	irq_param;
#else
	struct yk3_irq_param	irq_param;
#endif
	union yk3_doe_doorbell	db_reg;
#if YK3_DOE_DOORBELL_REUSE
	union yk3_doe_doorbell	db_val;
#endif
	struct yk3_doe_if_cfg	*cfg;
	struct yk3_doe_cmd_manager cmd_manager;
	struct yk3_doe_cb_pool	cb_pool;
	struct yk3_doe_evq	evq;
	struct yk3_doe_vfc	vfc;
	struct yk3_doe_fifo	fifo;
	struct yk3_doe_if_st	st;
#if YK3_DOE_MEASURE
	struct {
		struct yk3_doe_measure avg[YK3_DOE_MEASURE_N];
		struct yk3_doe_measure level[DOE_MEASURE_LEVEL_END];
	} measure;
#endif
};

static inline void
doe_if_init(struct yk3_doe_if *doe_if) {
	doe_if->irq_vector = -1;
}

static inline bool
is_doe_irq_ok(struct yk3_doe_if *doe_if) {
	return doe_if->irq_vector >= 0;
}

// otf: on-the-fly command
//
// command count(on-the-fly) + n_cmd(want push fifo) < free event block
static inline bool
is_doe_otf_check_pass(struct yk3_doe_if *doe_if, u64 n_cmd) {
	// is <, NOT <=
	//	evq keep one slot !!!
	return doe_vfc_count(&doe_if->vfc) + n_cmd < doe_evq_left(&doe_if->evq);
}

static inline struct yk3_doe_cmd *
doe_cmd_take(const struct yk3_doe_if *doe_if, u16 tag) {
	return doe_if->cmd_manager.tag_slot[tag].bind;
}

int doe_cmd_manager_init(struct yk3_doe_if *doe_if, bool pool_enable);
void doe_cmd_manager_fini(struct yk3_doe_if *doe_if);

// NOT call in irq
struct yk3_doe_cmd *
doe_cmd_get(struct yk3_doe_if *doe_if, int init_flags);

// maybe call in irq
int doe_cmd_put(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool in_irq);

// NOT call in irq
int doe_cmd_register(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd);
int doe_cmd_unregister(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool is_timeout);

struct yk3_doe_cmd *
doe_cmd_alloc(struct yk3_doe_if *doe_if, int opcode, int tbl_id, u32 index, int flags);
int doe_cmd_free(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool is_timeout);

#if YK3_DOE_CMD_ZOMBIE
void doe_cmd_zombie_check(struct yk3_doe_if *doe_if);
#endif
/******************************************************************************/
struct yk3_doe_spec_table {
	atomic_t			free_entrys;
	struct yk3_doe_zero_clear	zero_clear;
	struct yk3_doe_cache_param	cache_param;
	struct yk3_doe_index_param	index_param;
	struct yk3_doe_aie_param	aie_param;
	struct yk3_doe_hie_param	hie_param;
	struct yk3_doe_miu_param	miu_param;
};

static inline void *
doe_spec_table_data(struct yk3_doe_spec_table *spec, enum yk3_doe_special_table spec_tbl_id) {
	switch (spec_tbl_id) {
	case YK3_DOE_CACHE_PARAM_TABLE:	return &spec->cache_param;
	case YK3_DOE_INDEX_PARAM_TABLE:	return &spec->index_param;
	case YK3_DOE_AIE_PARAM_TABLE:	return &spec->aie_param;
	case YK3_DOE_HIE_PARAM_TABLE:	return &spec->hie_param;
	case YK3_DOE_MIU_PARAM_TABLE:	return &spec->miu_param;

	case YK3_DOE_ZERO_CLEAR_TABLE:
	default: return &spec->zero_clear;
	}
}

/* parameters for table creating */
struct yk3_doe_table_param {
	// logic address
	// only for hmc array load/store
	u64 addr;
	u64 index_addr;

	u32 depth;
	u32 sdepth;		/* depth of hash second table */

	u32 chain_limit;	/* hash表冲突链最大长度 */
	u32 hash_seed;		/* hash表冲突链最大长度 */

	u16 dov_len;		// array/hash value length
	u16 key_len;		// for hash, 0 with array
	u16 item_space;		// only for hmc array load/store
	u16 item_order;		// small-array ignore it

	u16 index_cache;	/* default 256 */
	u8 tbl_type;
	u8 xie;
	u8 endian;
	u8 ddr_channel;
	u8 is_hash;
	u8 is_small_array;
	u8 is_small_hash;

	struct yk3_doe_hie_cache cache;
};

static inline u64
doe_table_index_block_address(struct yk3_doe_table_param *param, int index) {
	return param->index_addr + (u64)index * sizeof(union yk3_doe_index_block);
}

static inline u64
doe_table_ddr_block_address(struct yk3_doe_table_param *param, int index) {
	return param->addr + (u64)index * sizeof(struct yk3_doe_ddr_block);
}

enum yk3_doe_rw {
	YK3_DOE_WR = 0, // is_read = false
	YK3_DOE_RD = 1, // is_read = true
};

#define doe_rw_name(_rw)		(((_rw) == YK3_DOE_WR) ? "write" : "read")
#define doe_cache_mode_name(_mode)	(!!(_mode) ? "short" : "long")

struct yk3_doe_xie_info {
	union {
		bool is_short;
		enum yk3_doe_cache_mode cache_mode;
	};
	u32 max_key_len;	// 0 is NOT used
	u32 max_value_len;
	u32 cache_entry_count;
	u32 cache_entry_limit;
};

enum yk3_doe_hmc_mode {
	DOE_HMC_MODE_AUTO	= 0,
	DOE_HMC_MODE_RESERVED	= 1,
	DOE_HMC_MODE_HUGEPAGE	= 2,

	DOE_HMC_MODE_END
};

static inline const char *
doe_hmc_mode_name(enum yk3_doe_hmc_mode mode) {
	switch (mode) {
	case DOE_HMC_MODE_RESERVED:
		return "reserved";
	case DOE_HMC_MODE_HUGEPAGE:
		return "hugepage";
	case DOE_HMC_MODE_AUTO:
	default:
		return "auto";
	}
}

enum {
	DOE_PAGE_GROUP_2M = 0,
	DOE_PAGE_GROUP_1G = 1,

	DOE_PAGE_GROUP_END
};

static inline const char *
doe_page_group_name(int group) {
	switch (group) {
	case DOE_PAGE_GROUP_2M:
		return "2m";
	case DOE_PAGE_GROUP_1G:
	default:
		return "1g";
	}
}

struct yk3_doe_page {
	u64 pa;
	void *va;
	dma_addr_t dma;
	u64 size; // yes, 64bit. hmc reserved use big size @ doe_hmc_mem_map
};

struct yk3_doe_page_group {
	u64 total;
	u64 start;
	u64 end;
	u32 page_size;
	u32 page_count;
	u32 first_page_index;

	struct yk3_doe_page *pages;
};

struct yk3_doe_hmc {
	u64	size;
	bool	enable;
	bool	soc; // host(default, false), soc(true)
	enum yk3_doe_hmc_mode mode;

	struct yk3_doe_page reserved;
};

struct yk3_doe_fast {
	// timer
	struct {
		struct hrtimer	timer;
		ktime_t		busy_interval;
		ktime_t		slow_interval;
		u64		n_timer_busy;
		u64		n_timer_slow;
		u32		empty_times;
		u32		empty_threshold;
		int		stop;
	} __cacheline;

	// work
	struct {
		struct delayed_work	work;
		struct workqueue_struct *wq;
		u64		n_work_empty;
		u64		n_work_batch;
		u64		n_work_pop;
		atomic64_t	n_work_cmd_error;
	} __cacheline;

	struct list_head list;
	spinlock_t	lock; // list lock
	u32		count;
	u32		threshold;
	atomic64_t	n_user_batch;
	atomic64_t	n_user_nil;
	atomic64_t	n_user_cmd_error;
	u64		n_user_pop;
	u64		n_user_push;
	u64		n_ack_error;
} __cacheline;

void doe_fast_worker(struct work_struct *work);

struct yk3_doe_st {
	atomic64_t times_reg_wr_when_hw_failure;
	atomic64_t times_reg_rd_when_hw_failure;
} __cacheline;

// use pdev_priv
//	pdev_priv->doe_priv
//	pdev_priv->dbgfs_doe_hst_file
//	pdev_priv->dbgfs_doe_sst_file
//	pdev_priv->dbgfs_doe_gst_file
//	pdev_priv->dbgfs_dir
//	pdev_priv->name
//	pdev_priv->dev
//	pdev_priv->pdev
//	pdev_priv->card
//	pdev_priv->vendor
//	pdev_priv->device
//	pdev_priv->pf_id
//	pdev_priv->bar_pa
//	pdev_priv->bar_size
//	pdev_priv->bar_addr

// DOE priv size:
struct yk3_doe_priv {
	struct rw_semaphore		tbl_lock;
	struct device			*dev;
	struct pci_dev			*pdev;
	struct yk3_card			*card;
	struct yk3_pdev_priv		*pdev_priv;
	const struct yk3_chip_doe_spec	*chip_spec;
	void __iomem			*bar_base;	/* doe reg */
	struct yk3_doe_mm		*ddr;		// for hmc
	struct yk3_doe_mm		*ram;		// for little-array
	struct yk3_doe_mm		*index_sram;	// for hash's index cache
	struct yk3_doe_hmc		hmc;		/* hmc info */
	struct yk3_doe_cfg		cfg;
	struct yk3_doe_if		doe_if[2];	/* YK3_DOE_WR and YK3_DOE_RD */
	struct yk3_doe_xie_info		xie_info[YK3_DOE_XIE_END];
	struct yk3_doe_table_param	param[YK3_DOE_TABLE_LIMIT];
	struct yk3_doe_spec_table	spec[YK3_DOE_USER_TABLE_LIMIT + 2];
	atomic_t			tbl_enable[YK3_DOE_TABLE_LIMIT];
	struct yk3_doe_page_group	page_group[DOE_PAGE_GROUP_END];
	struct yk3_doe_page		*pages;
	struct yk3_doe_fast		fast;

	struct dentry *dbgfs_doe_hst_file;
	struct dentry *dbgfs_doe_sst_file;
	struct dentry *dbgfs_doe_gst_file;

	u32		page_count;
	u32		clk_gate_en;
	atomic_t	hash_table_count;
	atomic_t	active_table_count;
	enum yk3_chip	chip;
	enum yk3_arch	arch;
	bool		init;
	bool		is_cie_zip; // only for k3max
	bool		is_low_power;
	bool		is_cache_isolation;
	struct yk3_doe_st st;
};

#if YK3_SKIP_DOE
extern struct yk3_doe_priv __doe_skip;
#endif

static inline bool
is_doe_k3(struct yk3_doe_priv *doe_priv) {
	return doe_priv->chip == YK3_K3;
}

static inline bool
is_doe_k3max(struct yk3_doe_priv *doe_priv) {
	return doe_priv->chip == YK3_K3MAX;
}

static inline bool
is_doe_fpga(struct yk3_doe_priv *doe_priv) {
	return doe_priv->arch == YK3_FPGA;
}

static inline bool
is_doe_asic(struct yk3_doe_priv *doe_priv) {
	return doe_priv->arch == YK3_ASIC;
}

static inline enum yk3_doe_channel
doe_ddr_channel(struct yk3_doe_priv *doe_priv) {
	return doe_priv->chip_spec->ddr_channel;
}

static inline enum yk3_doe_channel
doe_array_channel(struct yk3_doe_priv *doe_priv, bool is_small_array) {
	return is_small_array ? YK3_DOE_CHANNEL_RAM : doe_ddr_channel(doe_priv);
}

static inline int
doe_cache_entry_count(struct yk3_doe_priv *doe_priv, enum yk3_doe_xie xie, bool is_short) {
	int cache_entry_count = doe_priv->chip_spec->cache_entry_count[xie][is_short];

	return doe_priv->is_low_power ? (cache_entry_count / 2) : cache_entry_count;
}

// only for hmc
static inline u64
doe_table_entry(struct yk3_doe_priv *doe_priv, int tbl_id, int index) {
	struct yk3_doe_table_param *param = &doe_priv->param[tbl_id];

	return param->addr + (s64)index * param->item_space;
}

static inline void *
doe_hmc_group_obj(const struct yk3_doe_page_group *group,
		  const u64 address,
		  const u32 mask,
		  const u32 order) {
	u32 idx = address >> order;
	u32 off = address & mask;

	return (group->pages[idx].va + off);
}

static inline void *
doe_hmc_object(struct yk3_doe_priv *doe_priv, u64 logic_address) {
	struct yk3_doe_page_group *group2M = &doe_priv->page_group[DOE_PAGE_GROUP_2M];
	struct yk3_doe_page_group *group1G = &doe_priv->page_group[DOE_PAGE_GROUP_1G];

	if (logic_address < group2M->total) {
		// {}
		return doe_hmc_group_obj(group2M, logic_address, YK3_2MB - 1, 21);
	}

	return doe_hmc_group_obj(group1G, logic_address - group2M->total, YK3_1GB - 1, 30);
}

static inline void *
doe_hmc_array_obj(struct yk3_doe_priv *doe_priv, int tbl_id, int index) {
	u64 address = doe_table_entry(doe_priv, tbl_id, index);

	return doe_hmc_object(doe_priv, address);
}

int doe_hmc_array_op(struct yk3_doe_priv *doe_priv, int tbl_id, int index,
		     void *buf, int len, enum yk3_doe_rw op);

int doe_hmc_counter_load(struct yk3_doe_priv *doe_priv,
			 int tbl_id, int index, struct yk3_doe_counter *counter);

static inline void
doe_tbl_lock_init(struct yk3_doe_priv *doe_priv) {
	init_rwsem(&doe_priv->tbl_lock);
}

static inline void
doe_tbl_lock_w(struct yk3_doe_priv *doe_priv) {
	down_write_nested(&doe_priv->tbl_lock, SINGLE_DEPTH_NESTING);
}

static inline void
doe_tbl_unlock_w(struct yk3_doe_priv *doe_priv) {
	up_write(&doe_priv->tbl_lock);
}

static inline void
doe_tbl_lock_r(struct yk3_doe_priv *doe_priv) {
	down_read_nested(&doe_priv->tbl_lock, SINGLE_DEPTH_NESTING);
}

static inline void
doe_tbl_unlock_r(struct yk3_doe_priv *doe_priv) {
	up_read(&doe_priv->tbl_lock);
}

static inline void *
doe_dmam_zalloc_coherent(struct yk3_doe_priv *doe_priv, size_t size, dma_addr_t *dma_handle) {
	void *obj;

	obj = dmam_alloc_coherent(doe_priv->dev, size, dma_handle, GFP_KERNEL);
	if (obj)
		yk3_memzero(obj, size);

	return obj;
}

static inline struct yk3_doe_priv *
doe_priv_get(struct yk3_pdev_priv *pdev_priv) {
	if (unlikely(!pdev_priv)) {
		doe_error(-E_DOE_INVALID, "pdev is nil");
		return NULL;
	}

#if YK3_SKIP_DOE
	return &__doe_skip;
#else
	if (unlikely(!pdev_priv->doe_priv)) {
		doe_error(-E_DOE_INVALID, "pdev[0x%llx]'s doe_priv is nil", YK3_PTR(pdev_priv));
		return NULL;
	}

	return pdev_priv->doe_priv;
#endif
}

static inline struct yk3_doe_if *
doe_if_get(struct yk3_doe_priv *doe_priv, bool is_read) {
	return &doe_priv->doe_if[!!is_read];
}

static inline struct yk3_doe_if *
doe_if_get_w(struct yk3_doe_priv *doe_priv) {
	return doe_if_get(doe_priv, false);
}

static inline struct yk3_doe_if *
doe_if_get_r(struct yk3_doe_priv *doe_priv) {
	return doe_if_get(doe_priv, true);
}

static inline bool
doe_table_try_enable(struct yk3_doe_priv *doe_priv, int tbl_id) {
	// 0==>1 @ old is 0
	return atomic_cmpxchg(&doe_priv->tbl_enable[tbl_id], 0, 1) == 0;
}

static inline bool
doe_table_try_disable(struct yk3_doe_priv *doe_priv, int tbl_id) {
	// 1==>0 @ old is 1
	return atomic_cmpxchg(&doe_priv->tbl_enable[tbl_id], 1, 0) == 1;
}

static inline void
__doe_table_change(struct yk3_doe_priv *doe_priv, int tbl_id, int state) {
	atomic_set(&doe_priv->tbl_enable[tbl_id], state);
}

static inline void
doe_table_enable(struct yk3_doe_priv *doe_priv, int tbl_id) {
	__doe_table_change(doe_priv, tbl_id, 1);
}

static inline void
doe_table_disable(struct yk3_doe_priv *doe_priv, int tbl_id) {
	__doe_table_change(doe_priv, tbl_id, 0);
}

static inline int
__doe_table_state(const struct yk3_doe_priv *doe_priv, int tbl_id) {
	return atomic_read(&doe_priv->tbl_enable[tbl_id]);
}

static inline bool
is_doe_table_enable(const struct yk3_doe_priv *doe_priv, int tbl_id) {
	return __doe_table_state(doe_priv, tbl_id) == 1;
}

static inline enum yk3_doe_tbl
doe_tbl_type(const struct yk3_doe_priv *doe_priv, int tbl_id) {
	const struct yk3_doe_table_param *param = &doe_priv->param[tbl_id];

	return param->is_small_array ? DOE_TABLE_SMALL_ARRAY : param->tbl_type;
}

static inline int
doe_xie_gate_bit(const struct yk3_doe_priv *doe_priv, enum yk3_doe_xie xie) {
	// lhie: use bit 6 (!= xie)
	// other xie: use bit == xie
	return (xie == YK3_DOE_LHIE) ? 6 : xie;
}

static inline bool
is_doe_gate_enable(const struct yk3_doe_priv *doe_priv, int bit) {
	return !!(BIT(bit) & doe_priv->clk_gate_en);
}

static inline bool
is_doe_xie_enable(const struct yk3_doe_priv *doe_priv, enum yk3_doe_xie xie) {
	int bit = doe_xie_gate_bit(doe_priv, xie);

	return is_doe_gate_enable(doe_priv, bit);
}

#define is_doe_hw_failure(doe_priv)	((doe_priv)->card->emp_close_hw)

int doe_table_type_check(const struct yk3_doe_priv *doe_priv, enum yk3_doe_tbl tbl_type);

static inline u32
doe_rd32(struct yk3_doe_priv *doe_priv, u32 off) {
	if (unlikely(is_doe_hw_failure(doe_priv))) {
		atomic64_inc(&doe_priv->st.times_reg_rd_when_hw_failure);
		return 0;
	}

	return yk3_rd32(doe_priv->bar_base, off);
}

static inline void
doe_wr32(struct yk3_doe_priv *doe_priv, u32 off, u32 val) {
	if (unlikely(off == YK3_DOE_REG_VERSION))
		doe_bug(-E_DOE_BUG, "overwirte doe version ==> 0x%x", val);

	if (!is_doe_hw_failure(doe_priv)) {
		// {}
		yk3_wr32(doe_priv->bar_base, off, val);
	} else {
		// {}
		atomic64_inc(&doe_priv->st.times_reg_wr_when_hw_failure);
	}
}

static inline u32
bar_rd32(struct yk3_doe_priv *doe_priv, u32 off) {
	return yk3_rd32(YK3_BAR_BASE(doe_priv->pdev_priv), off);
}

static inline void
bar_wr32(struct yk3_doe_priv *doe_priv, u32 off, u32 val) {
	yk3_wr32(YK3_BAR_BASE(doe_priv->pdev_priv), off, val);
}

static inline void
doe_gate_update(struct yk3_doe_priv *doe_priv) {
	doe_priv->clk_gate_en = doe_rd32(doe_priv, YK3_DOE_REG_CLK_GATE_EN);
}

static inline void
doe_doorbell_trigger(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb) {
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	union yk3_doe_doorbell *reg = &doe_if->db_reg;
	union yk3_doe_doorbell new = {
		.addr_H = (cb->dma_base >> 32),
		.addr_L = (cb->dma_base & 0xffffffff),
		.len = cb->cmd_space,
	};

#if YK3_DOE_DOORBELL_REUSE
	union yk3_doe_doorbell *old = &doe_if->db_val;

	if (new.addr_H != old->addr_H)
		doe_wr32(doe_priv, reg->addr_H, new.addr_H);

	if (new.addr_L != old->addr_L)
		doe_wr32(doe_priv, reg->addr_L, new.addr_L);

	if (new.len != old->len)
		doe_wr32(doe_priv, reg->len, new.len);

	doe_wr32(doe_priv, reg->control, 1);
	doe_doorbell_copy(old, &new);
#else
	doe_wr32(doe_priv, reg->addr_H, new.addr_H);
	doe_wr32(doe_priv, reg->addr_L, new.addr_L);
	doe_wr32(doe_priv, reg->len, new.len);
	doe_wr32(doe_priv, reg->control, 1);
#endif
}

static inline void
doe_dma_select(struct yk3_doe_priv *doe_priv) {
	union yk3_doe_push_select old, new;

	// read old
	old.raw = bar_rd32(doe_priv, YK3_COMMON_REG_PUSH_SELECT);
	// update doe_push_sel, keep other
	new.raw = old.raw;
	new.doe_push_sel = YK3_DOE_PUSH_SELECT(doe_priv->hmc.enable, doe_priv->hmc.soc);

	if (old.raw != new.raw) {
		bar_wr32(doe_priv, YK3_COMMON_REG_PUSH_SELECT, new.raw);

		doe_info("reset dma-select[0x%x] 0x%x ==> 0x%x",
			 YK3_COMMON_REG_PUSH_SELECT, old.raw, new.raw);
	}
}

static inline int
doe_spec_table_size(struct yk3_doe_priv *doe_priv, enum yk3_doe_special_table spec_tbl_id) {
	switch (spec_tbl_id) {
	case YK3_DOE_CACHE_PARAM_TABLE:
		return sizeof(struct yk3_doe_cache_param);
	case YK3_DOE_INDEX_PARAM_TABLE:
		return sizeof(struct yk3_doe_index_param);
	case YK3_DOE_AIE_PARAM_TABLE:
		return sizeof(struct yk3_doe_aie_param);
	case YK3_DOE_MIU_PARAM_TABLE:
		return sizeof(struct yk3_doe_miu_param);
	case YK3_DOE_HIE_PARAM_TABLE: {
		struct yk3_doe_hie_param *param;

		if (is_doe_k3(doe_priv))
			return sizeof(*param) - sizeof(param->cache);
		else
			return sizeof(*param);
	}
	case YK3_DOE_ZERO_CLEAR_TABLE:
	default:
		return sizeof(struct yk3_doe_zero_clear);
	}
}

/******************************************************************************/
enum yk3_doe_sw_opcode {
	YK3_DOE_SW_RAW_CMD = 0x01,
	YK3_DOE_SW_CREATE_ARRAY = 0x10,
	YK3_DOE_SW_DELETE_ARRAY = 0x11,
	YK3_DOE_SW_CREATE_HASH = 0x12,
	YK3_DOE_SW_DELETE_HASH = 0x13,

	// hardware opcode begin
	YK3_DOE_SW_ARRAY_LOAD	= YK3_DOE_ARRAY_LOAD,
	YK3_DOE_SW_ARRAY_STORE	= YK3_DOE_ARRAY_STORE,
	YK3_DOE_SW_ARRAY_WRITE	= YK3_DOE_ARRAY_WRITE,
	YK3_DOE_SW_ARRAY_READ	= YK3_DOE_ARRAY_READ,
	YK3_DOE_SW_HASH_INSERT	= YK3_DOE_HASH_INSERT,
	YK3_DOE_SW_HASH_DELETE	= YK3_DOE_HASH_DELETE,
	YK3_DOE_SW_HASH_QUERY	= YK3_DOE_HASH_QUERY,
	YK3_DOE_SW_HASH_UPDATE	= YK3_DOE_HASH_UPDATE,
	YK3_DOE_SW_HASH_SAVE	= YK3_DOE_HASH_SAVE,
	// hardware opcode end

	YK3_DOE_SW_COUNTER_ENABLE	= 0x60,
	YK3_DOE_SW_COUNTER_LOAD		= 0x61,
	YK3_DOE_SW_INIT_INDEX_VIEW = 0x80,
	YK3_DOE_SW_RESET_ALL_TABLE = 0x81,

	YK3_DOE_SW_END
};

static inline bool
is_doe_init_opcode(enum yk3_doe_sw_opcode opcode) {
	return opcode == YK3_DOE_SW_INIT_INDEX_VIEW || opcode == YK3_DOE_SW_RESET_ALL_TABLE;
}

static inline u64
doe_opcode_timeout(enum yk3_doe_sw_opcode opcode)
{
	switch (opcode) {
	case YK3_DOE_SW_CREATE_ARRAY:
	case YK3_DOE_SW_DELETE_ARRAY:
	case YK3_DOE_SW_CREATE_HASH:
	case YK3_DOE_SW_DELETE_HASH:
	case YK3_DOE_SW_RESET_ALL_TABLE:
	case YK3_DOE_SW_INIT_INDEX_VIEW:
		return DOE_CMD_MULTI_TIMEOUT;
	default:
		return DOE_CMD_SINGLE_TIMEOUT;
	}
}

enum yk3_doe_cmd_flag {
	DOE_CMD_F_STACK		= 0x01,
	DOE_CMD_F_CONTAINER	= 0x02,
	DOE_CMD_F_SUB		= 0x04,
	DOE_CMD_F_TIMEOUT	= 0x08,

	DOE_CMD_F_READ		= 0x10,
	DOE_CMD_F_ASYNC		= 0x20,
	DOE_CMD_F_FAST		= 0x40,
	DOE_CMD_F_SYNC		= 0x80,

	DOE_CMD_F_WAIT_SLEEP	= 0x100,

	// DOE_CMD_F_HASH	= 0x100,
	// DOE_CMD_F_ARRAY	= 0x200,
	// DOE_CMD_F_COUNTER	= 0x400,
};

struct yk3_doe_cmd_raw {
	u32 len;

	union {
		u8 raw[YK3_DOE_CMD_MAXSIZE];

		struct {
			u8 opcode;
			u8 tbl_id;

			u8 data[YK3_DOE_CMD_MAXSIZE - 2];
		};
	};
};

// DOE cmd size: 288
struct yk3_doe_cmd {
	u8 opcode;
	u8 tbl_id;
	// For Counter table enable item
	//	and reused by protect status.
	u8 enable;
	// High priority. Only be used in `store/insert` command.
	// If the table item is set to high priority, it will be always in cache.
	u8 high_pri;
	u16 tag;
	u16 flags;

	u16 subidx; // cmd is subcmd, subidx is the index of parent cmd's subcmd
	// cmd is container: cnt is the count of sub cmd
	// cmd is single   : cnt is 1
	u16 cnt;
	/* The number of secceed/failed command */
	u16 succeed;
	u16 failed;

	u8 err; // hw error
	u8 r[3];

	union {
		int done;			// busy wait
		struct completion complete;	// sleep wait
	} wait;

#if YK3_DOE_MEASURE
	u64 ns_start;
#endif
	u64 seq;
	struct yk3_doe_cmd	*parent;
	struct yk3_doe_if	*doe_if;

	union {
		// only for single sync cmd
		struct yk3_doe_cmd_raw		*raw;

		// only for container cmd
		struct list_head		list;

		// only for sub cmd for single fast/async cmd
		struct list_head		node;
	};

	// keep last
	union {
		u8 body[128 + 96];

		// single array load/store
		// counter enable
		// meter config
		// zero table
		struct {
			union {
				u32 index;
				u32 target_table_id;
			};

			union {
				struct yk3_meter_config config;
				struct yk3_doe_zero_clear zero_clear;
				struct yk3_doe_counter counter;

				// array/counter
				u8 value[128];
			};
		};

		/* single hash instruction */
		struct {
			u8 key[96];
			u8 value[128];
		} hash;

		/* create table */
		struct yk3_doe_table_param tbl_param;
	};
};

//				cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id
#define DOE_CMD_FMT		"seq[%llu] tag[0x%04x] flags[0x%x] opcode[0x%x] tbl_id[0x%x]"

//				cmd->cnt, cmd->flags, cmd->opcode, cmd->tbl_id
#define DOE_CMD_CONTAINER_FMT	"count[%d] flags[0x%x] opcode[0x%x] tbl_id[0x%x]"

#define DOE_CMD_STACK(flag)	{ .flags = (DOE_CMD_F_STACK | DOE_CMD_F_SYNC | (flag)) }
#define DOE_CMD_STACK_R		DOE_CMD_STACK(DOE_CMD_F_READ)
#define DOE_CMD_STACK_W		DOE_CMD_STACK(0)
#define DOE_CMD_CONTAINER(cmd)	{ \
	.flags = (DOE_CMD_F_STACK | DOE_CMD_F_SYNC | DOE_CMD_F_CONTAINER), \
	.list  = LIST_HEAD_INIT(cmd.list), \
}

static inline bool
is_doe_cmd_use_index_res(const struct yk3_doe_cmd *cmd) {
	return  cmd->opcode == YK3_DOE_SW_ARRAY_WRITE &&
		cmd->tbl_id == YK3_DOE_INDEX_VIEW;
}

static inline bool
is_doe_cmd_complete(const struct yk3_doe_cmd *cmd) {
	return cmd->cnt == (cmd->succeed + cmd->failed);
}

static inline bool
is_doe_cmd_flag(const struct yk3_doe_cmd *cmd, enum yk3_doe_cmd_flag flag) {
	return !!(cmd->flags & flag);
}

static inline bool
is_doe_cmd_read(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_READ);
}

static inline bool
is_doe_cmd_container(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_CONTAINER);
}

static inline bool
is_doe_cmd_sub(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_SUB);
}

static inline bool
is_doe_cmd_single(const struct yk3_doe_cmd *cmd) {
	// single: NOT container and NOT sub
	return !(cmd->flags & (DOE_CMD_F_CONTAINER | DOE_CMD_F_SUB));
}

// sync call
static inline bool
is_doe_cmd_sync(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_SYNC);
}

// fast call
static inline bool
is_doe_cmd_fast(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_FAST);
}

// async call
static inline bool
is_doe_cmd_async(const struct yk3_doe_cmd *cmd) {
	// async: NOT sync and NOT fast
	return !(cmd->flags & (DOE_CMD_F_SYNC | DOE_CMD_F_FAST));
}

static inline bool
is_doe_cmd_stack(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_STACK);
}

static inline bool
is_doe_cmd_timeout(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_TIMEOUT);
}

static inline bool
is_doe_cmd_wait_sleep(const struct yk3_doe_cmd *cmd) {
	return is_doe_cmd_flag(cmd, DOE_CMD_F_WAIT_SLEEP);
}

#define is_doe_cmd_alloced(cmd) (!is_doe_cmd_stack(cmd))

static inline const char *
doe_cmd_name(const struct yk3_doe_cmd *cmd) {
	if (is_doe_cmd_single(cmd))
		return "single";
	else if (is_doe_cmd_container(cmd))
		return "container";
	else if (is_doe_cmd_sub(cmd))
		return "sub";
	else
		return "unknown";
}

static inline void
doe_push_subcmd(struct yk3_doe_cmd *cmd, struct yk3_doe_cmd *sub) {
	int idx = cmd->cnt++;

	sub->subidx = idx;
	sub->flags |= (DOE_CMD_F_SUB | DOE_CMD_F_SYNC);

	list_add_tail(&sub->node, &cmd->list);
}

static inline void
doe_cmd_wait_init(struct yk3_doe_cmd *cmd) {
	if (is_doe_cmd_wait_sleep(cmd)) {
		// sleep wait
		init_completion(&cmd->wait.complete);
	} else {
		// busy wait
		WRITE_ONCE(cmd->wait.done, 0);
	}
}

static inline void
doe_cmd_wait_done(struct yk3_doe_cmd *cmd) {
	if (is_doe_cmd_wait_sleep(cmd)) {
		// sleep wait
		complete(&cmd->wait.complete);
	} else {
		// busy wait
		WRITE_ONCE(cmd->wait.done, 1);
	}
}

static inline void
doe_subcmd_successed(struct yk3_doe_cmd *cmd, struct yk3_doe_cmd *sub) {
	cmd->succeed++;
	sub->succeed++;
}

static inline void
doe_subcmd_failed(struct yk3_doe_cmd *cmd, struct yk3_doe_cmd *sub, u8 hw_err) {
	sub->failed++;
	cmd->failed++;

	sub->err = hw_err;		// sub cmd: save error
	cmd->err = cmd->err ? : hw_err;	// container : save first error
}

static inline void
doe_cmd_successed(struct yk3_doe_cmd *cmd) {
	cmd->succeed++;
}

static inline void
doe_cmd_failed(struct yk3_doe_cmd *cmd, u8 hw_err) {
	cmd->failed++;
	cmd->err = hw_err;
}

static inline void
doe_cmd_measure_start(struct yk3_doe_cmd *cmd) {
#if YK3_DOE_MEASURE
	cmd->ns_start = ktime_get_ns();
#endif
}

static inline void
doe_measure_init(struct yk3_doe_if *doe_if) {
#if YK3_DOE_MEASURE
	yk3_objzero(&doe_if->measure);

	doe_if->measure.level[DOE_MEASURE_LEVEL_MIN].value = 0xffffffff;
#endif
}

static inline void
doe_cmd_bind(struct yk3_doe_cmd_manager *manager, struct yk3_doe_cmd *cmd, u16 tag) {
	struct yk3_doe_tag_slot *slot = &manager->tag_slot[tag];

	cmd->tag = tag;
	cmd->seq = (u64)atomic64_fetch_add(1, &manager->seq);

	slot->bind	= cmd;
	slot->opcode	= cmd->opcode;
	slot->tbl_id	= cmd->tbl_id;
	slot->flags	= cmd->flags;
	slot->index	= cmd->index;
	slot->seq	= cmd->seq;
	slot->times_get++;
}

static inline void
doe_cmd_unbind(struct yk3_doe_cmd_manager *manager, struct yk3_doe_cmd *cmd, bool is_timeout) {
	u16 tag = cmd->tag;
	struct yk3_doe_tag_slot *slot = &manager->tag_slot[tag];

	slot->bind = NULL;
	if (unlikely(is_timeout)) {
		// keep slot info
		slot->flags |= DOE_CMD_F_TIMEOUT;
		slot->times_timeout++;
	} else {
		slot->times_put++;
		doe_tag_slot_clean(slot);
	}
}

/******************************************************************************/
struct mm_region {
	struct mm_region *next;
	u64 address;
	u64 size;
	int tbl_id;
};

struct yk3_doe_mm {
	struct yk3_doe_priv *doe_priv;
	struct device *dev;
	const char *name;
	u64 base_address;
	u64 total_size;
	u64 align_mask;
	dma_addr_t dma_base;
	void *mem_ptr;
	bool hmc;
	struct mm_region *used_list;
	struct mm_region *free_list;
};

static inline struct yk3_doe_mm *
doe_get_mm(struct yk3_doe_priv *doe_priv, u8 channel) {
	return (channel == YK3_DOE_CHANNEL_RAM) ? doe_priv->ram : doe_priv->ddr;
}

#ifdef MM_VERBOSE_DEBUG
static inline void
doe_mm_dump(struct yk3_doe_mm *ymm)
{
	struct mm_region *mr;
	int i;

	pr_debug("Memory Manage %s used list:\n", ymm->name);

	for (i = 1, mr = ymm->used_list; mr; i++, mr = mr->next)
		pr_debug("[%d] 0x%llx+0x%llx ->\n", i, mr->address, mr->size);

	pr_debug("Memory Manage %s freed list:\n", ymm->name);

	for (i = 1, mr = ymm->free_list; mr; i++, mr = mr->next)
		pr_debug("[%d] 0x%llx+0x%llx ->\n", i, mr->address, mr->size);
}
#else
static inline void doe_mm_dump(struct yk3_doe_mm *ymm) {}
#endif /* MM_VERBOSE_DEBUG */

struct yk3_doe_mm *
doe_mm_create(struct yk3_doe_priv *doe_priv, u64 size, u32 align_mask, bool hmc, const char *name);
s64 doe_mm_malloc(struct yk3_doe_mm *ymm, int tbl_id, u64 size);
void doe_mm_free(struct yk3_doe_mm *ymm, int tbl_id, u64 address);
/******************************************************************************/
long yk3_doe_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

irqreturn_t doe_irq_handler_r(int action, void *data);
irqreturn_t doe_irq_handler_w(int action, void *data);

#if K2U
int k2u_doe_register_irqs(struct yk3_doe_priv *doe_priv);
int k2u_doe_unregister_irqs(struct yk3_doe_priv *doe_priv);
#endif

int  doe_debugfs_init(struct yk3_doe_priv *doe_priv);
void doe_debugfs_fini(struct yk3_doe_priv *doe_priv);

//yk3_doe_kapi.c API
int doe_table_existed(struct yk3_doe_priv *doe_priv, u8 tbl_id);

static inline int
doe_get_protect(struct yk3_doe_priv *doe_priv) {
	return doe_rd32(doe_priv, YK3_DOE_REG_PROTECT_CFG);
}

int doe_set_protect(struct yk3_doe_priv *doe_priv, u32 protect);

// 0: protect disable
// 1: protect enable
int doe_get_np_protect(struct yk3_doe_priv *doe_priv);
int doe_set_np_protect(struct yk3_doe_priv *doe_priv, int enable);

int doe_get_cache_info(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl_cache_info *cache);
int doe_set_cache_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl_cache_mode *cache);

int doe_get_counter_zip(struct yk3_doe_priv *doe_priv);
int doe_set_counter_zip(struct yk3_doe_priv *doe_priv, int zip);

int doe_get_hcode_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param);
int doe_set_hcode_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param);

int doe_get_cache_ioslation(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param);
int doe_set_cache_ioslation(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param);

int doe_get_hash_entry_count(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param);

int doe_evq_init(struct yk3_doe_if *doe_if);

//yk3_doe_process.c API
void doe_xie_init(struct yk3_doe_priv *doe_priv);
int doe_hw_init(struct yk3_doe_priv *doe_priv);

int doe_event_handler_r(struct yk3_doe_if *doe_if);
int doe_event_handler_w(struct yk3_doe_if *doe_if);

int doe_hw_check(struct yk3_doe_priv *doe_priv);

int doe_page_init(struct yk3_doe_priv *doe_priv);
void doe_page_fini(struct yk3_doe_priv *doe_priv);

int doe_kernel_call(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, u8 call_mode);
/******************************************************************************/
#endif /* __YK3_DOE_PROC_H_ */
