/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config dn200 ethernet buf pool
 */

#ifndef __DN200_BUFPOOL_H__
#define __DN200_BUFPOOL_H__
#include "common.h"

#define DN200_RX_BUF_SIZE 2048
#define DN200_DEFAULT_PAGE_SIZE 4096
#define DN200_MT_32K_MAX_BUF_SIZE (256 * 1024)
#define DN200_4K_MAX_BUF_SIZE (128 * 1024)
#define DN200_DEFAULT_PAGE_SIZE_SHIFT 12
#define DN200_BUFPOOL_CACHE_MAX_SIZE 512
#define DN200_DEFAULT_CACHE_SIZE 128
#define DN200_DEFAULT_CACHE_TH 128
#define DN200_RECY_MIN_CACHE_SIZE 32

#define dn200_bufpool_align __aligned(sizeof(long))

/** prod/cons sync types */
enum dn200_ring_sync_type {
	DN200_RING_SYNC_MT, /**< multi-thread safe (default mode) */
	DN200_RING_SYNC_ST, /**< single thread only */
	DN200_RING_SYNC_MT_RTS, /**< multi-thread relaxed tail sync */
	DN200_RING_SYNC_MT_HTS, /**< multi-thread head/tail sync */
};

struct dn200_bufring_headtail {
	atomic_t head; /**< prod/consumer head. */
	atomic_t tail; /**< prod/consumer tail. */
} dn200_bufpool_align;

struct dn200_page_buf {
	struct page *page;
	int *page_ref_bias;
	dma_addr_t desc_addr;
	dma_addr_t kernel_addr;
	__u32 page_offset;
	void *buf_addr;
	u16 buf_len;
	u16 busy_cnt;
	bool low_res; /* get buf from pool with low resource state */
};

struct dn200_bufring {
	u32 ring_size; /**< Size of ring. */
	u32 ring_mask; /**< Mask (size-1) of ring. */
	struct device *device;
	u32 buf_num_per_page;

	char pad0 dn200_bufpool_align; /**< empty cache line */
	/** Ring producer status. */
	struct dn200_bufring_headtail prod;

	char pad1 dn200_bufpool_align; /**< empty cache line */
	/** Ring consumer status. */
	struct dn200_bufring_headtail cons;

	char pad2 dn200_bufpool_align; /**< empty cache line */
	u64 *ring_objs; /*Element pointer */
};

struct dn200_buf_cache_ring {
	u32 cache_size; /**< Size of the cache, pow of 2 */
	u32 cache_mask; /*cache_size - 1 */
	u32 head;
	u32 tail;
	struct device *device;
	char pad2 dn200_bufpool_align; /**< empty cache line */
	/**
	 * Cache objects
	 *
	 * Cache is allocated to this size to allow it to overflow in certain
	 * cases to avoid needless emptying of cache.
	 */
	u64 *objs[DN200_BUFPOOL_CACHE_MAX_SIZE * 4] dn200_bufpool_align;
	u64 *tmp_objs[DN200_BUFPOOL_CACHE_MAX_SIZE] dn200_bufpool_align;
} dn200_bufpool_align;

struct dn200_buf_refill_stack {
	u32 cache_size; /**< Size of the cache, pow of 2 */
	u32 current_len; /**< Current cache count */
	bool use_reserve;
	struct device *device;
	char pad2 dn200_bufpool_align; /**< empty cache line */
	/**
	 * Cache objects
	 *
	 * Cache is allocated to this size to allow it to overflow in certain
	 * cases to avoid needless emptying of cache.
	 */
	u64 *objs[DN200_BUFPOOL_CACHE_MAX_SIZE * 2] dn200_bufpool_align;
} dn200_bufpool_align;

struct dn200_bufpool_cache {
	/*buf stored at buf_cached ring firstly, and
	 * than check that the page ref count is 1 and send it to the buf_refill
	 */
	struct dn200_buf_cache_ring buf_cached;
	struct dn200_buf_refill_stack buf_refill;
};

struct dn200_bufpool {
	int buf_num_per_page; /**< Flags supplied at creation. */
	u32 pool_size; /**< Max size of the bufpool. */
	u32 cache_size; /*Max size of cache_size per queue */
	u32 reserved_start;
	u32 buf_index;
	struct device *device;
	char pad0 dn200_bufpool_align; /**< empty cache line */
	spinlock_t lock;
	char pad1 dn200_bufpool_align; /**< empty cache line */
	char pad2 dn200_bufpool_align; /**< empty cache line */
	struct dn200_bufpool_cache *local_cache; /**< Per-queue local cache */
	struct dn200_bufring *pool_ring;
	struct dn200_bufring *cached_ring;
	struct dn200_page_buf *page_buf; /*Stores the page buf structure */
};

bool dma_can_direct_use(struct dn200_priv *priv, dma_addr_t dma_addr);
int dn200_rx_pool_setup(struct dn200_priv *priv);
void dn200_rx_pool_destory(struct dn200_priv *priv);

static __always_inline unsigned int
dn200_update_ringindex(atomic_t *v, int old_val, int new_val, bool is_single)
{
	return atomic_cmpxchg(v, old_val, new_val);
}

static __always_inline unsigned int
dn200_bufring_move_prod_head(struct dn200_bufring *r, unsigned int is_sp,
			     unsigned int n, int *old_head, int *new_head)
{
	unsigned int max = n;
	unsigned int free_entry = n;
	int cons_tail = 0;

	do {
		/* Reset n to the initial burst count */
		n = max;

		*old_head = atomic_read(&r->prod.head);
		cons_tail = atomic_read(&r->cons.tail);
		if (*old_head > cons_tail)
			free_entry = r->ring_size + cons_tail - *old_head - 1;
		else
			free_entry = cons_tail - *old_head - 1;

		if (n > free_entry)
			return 0;

		/* add rmb barrier to avoid load/load reorder in weak
		 * memory model. It is noop on x86
		 */
		smp_rmb();

		*new_head = (*old_head + n) & r->ring_mask;
	} while (unlikely(dn200_update_ringindex(&r->prod.head, *old_head,
						 *new_head,
						 is_sp) != *old_head));
	return n;
}

static __always_inline unsigned int
dn200_bufring_move_cons_head(struct dn200_bufring *r,
			     unsigned int n, int *old_head, int *new_head,
			     bool is_sc)
{
	unsigned int max = n;
	int free_entries = 0;
	int prod_tail = 0;

	/* move cons.head atomically */
	do {
		/* Restore n as it may change every loop */
		n = max;

		*old_head = atomic_read(&r->cons.head);

		/* add rmb barrier to avoid load/load reorder in weak
		 * memory model. It is noop on x86
		 */
		smp_rmb();
		prod_tail = atomic_read(&r->prod.tail);
		if (likely(prod_tail >= *old_head))
			free_entries = prod_tail - *old_head;
		else
			free_entries = r->ring_size + prod_tail - *old_head;
		/* Set the actual entries for dequeue */
		if (n > free_entries)
			n = free_entries;

		if (unlikely(n == 0 || (n % r->buf_num_per_page) != 0))
			return 0;

		*new_head = (*old_head + n) & r->ring_mask;

	} while (unlikely(dn200_update_ringindex(&r->cons.head, *old_head,
						 *new_head,
						 is_sc) != *old_head));
	return n;
}

static __always_inline void
dn200_bufring_enqueue_elems_64(struct dn200_bufring *r, u32 prod_head,
			       const void *obj_table, u32 n)
{
	unsigned int i;
	const u32 size = r->ring_size;
	u32 idx = prod_head & r->ring_mask;
	u64 *ring_objs = (u64 *)r->ring_objs;
	const u64 *obj = (const u64 *)obj_table;

	if (likely(idx + n <= size)) {
		for (i = 0; i < (n & ~0x3); i += 4, idx += 4) {
			ring_objs[idx] = obj[i];
			ring_objs[idx + 1] = obj[i + 1];
			ring_objs[idx + 2] = obj[i + 2];
			ring_objs[idx + 3] = obj[i + 3];
		}
		switch (n & 0x3) {
		case 3:
			ring_objs[idx++] = obj[i++];
			fallthrough;
		case 2:
			ring_objs[idx++] = obj[i++];
			fallthrough;
		case 1:
			ring_objs[idx++] = obj[i++];
		}
	} else {
		for (i = 0; idx < size; i++, idx++)
			ring_objs[idx] = obj[i];
		/* Start at the beginning */
		for (idx = 0; i < n; i++, idx++)
			ring_objs[idx] = obj[i];
	}

}

static __always_inline void
dn200_bufring_dequeue_elems_64(struct dn200_bufring *r, u32 prod_head,
			       void *obj_table, u32 n)
{
	unsigned int i;
	const u32 size = r->ring_size;
	u32 idx = prod_head & r->ring_mask;
	u64 *ring_objs = (u64 *)r->ring_objs;
	u64 *obj = (u64 *)obj_table;

	if (likely(idx + n <= size)) {
		for (i = 0; i < (n & ~0x3); i += 4, idx += 4) {
			obj[i] = (ring_objs[idx]);
			obj[i + 1] = (ring_objs[idx + 1]);
			obj[i + 2] = (ring_objs[idx + 2]);
			obj[i + 3] = (ring_objs[idx + 3]);
		}
		switch (n & 0x3) {
		case 3:
			obj[i++] = (ring_objs[idx++]);
			fallthrough;
		case 2:
			obj[i++] = (ring_objs[idx++]);
			fallthrough;
		case 1:
			obj[i++] = (ring_objs[idx++]);
		}
	} else {
		for (i = 0; idx < size; i++, idx++)
			obj[i] = (ring_objs[idx]);
		/* Start at the beginning */
		for (idx = 0; i < n; i++, idx++)
			obj[i] = (ring_objs[idx]);
	}
}

static __always_inline void dn200_bufring_enqueue_elems(struct dn200_bufring *r,
							u32 prod_head,
							const void *obj_table,
							u32 num)
{
	dn200_bufring_enqueue_elems_64(r, prod_head, obj_table, num);
}

static __always_inline void dn200_bufring_dequeue_elems(struct dn200_bufring *r,
							u32 cons_head,
							void *obj_table,
							u32 num)
{
	/* 8B and 16B copies implemented individually to retain
	 * the current performance.
	 */
	dn200_bufring_dequeue_elems_64(r, cons_head, obj_table, num);
}

static __always_inline void
dn200_bufpool_ring_update_tail(struct dn200_bufring_headtail *ht, int old_val,
			       int new_val, u32 single, u32 enqueue)
{
	if (enqueue)
		/* add wmb barrier to avoid head not update*/
		smp_wmb();
	else
		/* add rmb barrier to avoid load/load reorder in weak
		 * memory model. It is noop on x86
		 */
		smp_rmb();

	/* If there are other enqueues/dequeues in progress that preceded us,
	 * we need to wait for them to complete
	 */
	if (!single) {
		while (atomic_cmpxchg(&ht->tail, old_val, new_val) != old_val)
			ndelay(1);
	} else {
		atomic_set(&ht->tail, new_val);
	}
}

static __always_inline unsigned int
dn200_bufring_do_dequeue_elem(struct dn200_bufring *r, void *obj_table,
			      unsigned int n, unsigned int is_sc)
{
	int cons_head, cons_next;

	if (unlikely(n == 0))
		return 0;

	n = dn200_bufring_move_cons_head(r, n, &cons_head, &cons_next, 0);
	if (n == 0)
		goto end;

	dn200_bufring_dequeue_elems(r, cons_head, obj_table, n);

	dn200_bufpool_ring_update_tail(&r->cons, cons_head, cons_next, is_sc,
				       0);
end:
	return n;
}

static __always_inline unsigned int
dn200_bufring_do_enqueue_elem(struct dn200_bufring *r, const void *obj_table,
			      unsigned int n, unsigned int is_sp)
{
	int prod_head, prod_next;

	if (unlikely(n == 0))
		return 0;

	n = dn200_bufring_move_prod_head(r, is_sp, n, &prod_head, &prod_next);
	if (n == 0)
		return 0;

	dn200_bufring_enqueue_elems(r, prod_head, obj_table, n);

	dn200_bufpool_ring_update_tail(&r->prod, prod_head, prod_next, is_sp,
				       1);

	return n;
}
static __always_inline unsigned int
dn200_bufring_do_init_elem(struct dn200_bufring *r, const void *obj_table,
			   unsigned int n)
{
	int prod_head, prod_next;

	if (unlikely(n == 0))
		return 0;

	n = dn200_bufring_move_prod_head(r, 0, n, &prod_head, &prod_next);
	if (n == 0)
		goto end;

	dn200_bufring_enqueue_elems(r, prod_head, obj_table, n);

	dn200_bufpool_ring_update_tail(&r->prod, prod_head, prod_next, 0, 1);
end:
	return n;
}

static inline unsigned int
dn200_bufring_dequeue_elem(struct dn200_bufpool *rx_pool, void *obj_table,
			   unsigned int n, unsigned int is_sc)
{
	int i = 0;
	int max = n;
	int recy_count = 0;
	u64 *tmp_objs;
	int stride = rx_pool->buf_num_per_page;
	u64 *list_objs;

	n = dn200_bufring_do_dequeue_elem(rx_pool->pool_ring, obj_table, max,
					  is_sc);
	if (likely(n))
		return n;

	tmp_objs =
	    kzalloc(sizeof(u64) * DN200_BUFPOOL_CACHE_MAX_SIZE, GFP_ATOMIC);
	if (!tmp_objs)
		return 0;

	list_objs = kcalloc(stride, sizeof(u64), GFP_ATOMIC);
	if (!list_objs) {
		kfree(tmp_objs);
		return 0;
	}
recy:
	recy_count =
	    dn200_bufring_do_dequeue_elem(rx_pool->cached_ring,
					  (void *)tmp_objs,
					  DN200_BUFPOOL_CACHE_MAX_SIZE, is_sc);
	if (unlikely(!recy_count))
		goto free;

	for (i = 0; i < recy_count; i += stride) {
		struct dn200_page_buf *buf =
		    (struct dn200_page_buf *)tmp_objs[i];

		if (((page_ref_count(buf->page) - *(buf->page_ref_bias))) != 0)
			break;
	}
	if (likely(i))
		dn200_bufring_do_enqueue_elem(rx_pool->pool_ring,
					      (void *)tmp_objs, i, is_sc);

	if (i < recy_count)
		dn200_bufring_do_enqueue_elem(rx_pool->cached_ring,
					      (void *)(tmp_objs + i),
					      recy_count - i, is_sc);
	else
		goto recy;

free:
	n = dn200_bufring_do_dequeue_elem(rx_pool->pool_ring, obj_table, max,
					  is_sc);
	kfree(tmp_objs);
	kfree(list_objs);
	return n;
}

#define DN200_RX_CACHE_MIN_SIZE	512

static __always_inline bool
dn200_rx_pool_buf_put(struct dn200_bufpool *rx_pool,
		      struct dn200_page_buf *page_buf)
{
	struct dn200_bufring *pool_ring = rx_pool->pool_ring;

	page_buf->busy_cnt = 0;
	dn200_bufring_do_enqueue_elem(pool_ring, (void *)page_buf, 1,
				      (int)DN200_RING_SYNC_MT);
	return true;
}

static __always_inline unsigned int
dn200_page_buf_free(struct dn200_buf_cache_ring *buf_cached)
{
	if (buf_cached->tail > buf_cached->head) {
		return buf_cached->tail - buf_cached->head - 1;
	} else {
		return buf_cached->cache_size + buf_cached->tail -
		    buf_cached->head - 1;
	}
}

static __always_inline u32
dn200_page_buf_avail(struct dn200_buf_cache_ring *buf_cached)
{
	if (buf_cached->tail <= buf_cached->head) {
		return buf_cached->head - buf_cached->tail;
	} else {
		return buf_cached->cache_size - (buf_cached->tail -
						 buf_cached->head);
	}
}

static inline int dn200_cache_enqueue2pool(struct dn200_bufpool *rx_pool,
					   u8 queue_id, u32 max)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	int stride = rx_pool->buf_num_per_page;
	struct dn200_buf_cache_ring *buf_cached = &local_cache->buf_cached;
	u64 *objs = (u64 *)buf_cached->objs;
	int index = 0, i = 0;
	u32 avail = dn200_page_buf_avail(buf_cached);

	max = min(avail, max);
	max = ALIGN(max, stride);
	if (max < stride)
		return 0;

	for (index = buf_cached->tail, i = 0;
	     i < max && index < buf_cached->cache_size;
	     index += stride, i += stride) {
		struct dn200_page_buf *buf =
		    (struct dn200_page_buf *)objs[index];

		if ((page_ref_count(buf->page) - *(buf->page_ref_bias)) != 0)
			goto ref_unavail;
	}

ref_unavail:
	if (i) {
		objs = (u64 *)&buf_cached->objs[buf_cached->tail];
		dn200_bufring_do_enqueue_elem(rx_pool->pool_ring, (void *)objs,
					      i, 0);
		buf_cached->tail = index & buf_cached->cache_mask;
	}
	dev_dbg(rx_pool->device,
		"%s recycle count %d to refill bufstack now recycle tail %d buf_cached->head %d\n",
		__func__, i, index & buf_cached->cache_mask, buf_cached->head);
	return i;
}

static int dn200_cache_buf_recycle_em(struct dn200_bufpool *rx_pool,
				      u8 queue_id, u32 max)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	int stride = rx_pool->buf_num_per_page;
	struct dn200_buf_cache_ring *buf_cached = &local_cache->buf_cached;
	struct dn200_buf_refill_stack *buf_refill = &local_cache->buf_refill;
	u64 *objs = (u64 *)buf_cached->objs;
	u64 *tmp_objs = (u64 *)buf_cached->tmp_objs;
	u64 *refill_objs = (u64 *)&buf_refill->objs[0];
	int index = 0, i = 0, j;
	int unavail_count = 0, refill_count = 0;
	u32 avail = dn200_page_buf_avail(buf_cached);

	avail = (avail >= stride) ? (avail - stride) : 0;
	max = min_t(u32, avail, max);
	max = ALIGN(max, stride);

	max = min_t(u32, max, (u32)DN200_BUFPOOL_CACHE_MAX_SIZE);
	if (max < stride)
		return 0;

	for (index = buf_cached->tail, i = 0;
	     i < max && index < buf_cached->cache_size; index += stride) {
		struct dn200_page_buf *buf =
		    (struct dn200_page_buf *)objs[index];

		if (unlikely((page_ref_count(buf->page) - *(buf->page_ref_bias)) != 0)) {
			for (j = 0; j < stride; j++)
				tmp_objs[unavail_count++] = objs[index + j];
		} else {
			for (j = 0; j < stride; j++)
				refill_objs[refill_count++] = objs[index + j];
		}
		i += stride;
	}
	dev_dbg(rx_pool->device,
		"%s recycle count %d to refill bufstack %d to cache ring .now recycle tail %d, head %d cache mask %d\n",
		__func__, refill_count, unavail_count,
		index & buf_cached->cache_mask, buf_cached->head,
		buf_cached->cache_mask);

	buf_refill->current_len = refill_count;
	buf_cached->tail = index & buf_cached->cache_mask;
	if (unavail_count)
		dn200_bufring_do_enqueue_elem(rx_pool->cached_ring,
					      (void *)tmp_objs, unavail_count,
					      0);

	if (unlikely(dn200_page_buf_avail(buf_cached) >
		     DN200_BUFPOOL_CACHE_MAX_SIZE))
		dn200_cache_enqueue2pool(rx_pool, queue_id,
					 dn200_page_buf_avail(buf_cached) -
					 DN200_BUFPOOL_CACHE_MAX_SIZE);


	return buf_refill->current_len;
}

static inline int dn200_cache_buf_recycle(struct dn200_bufpool *rx_pool,
					  u8 queue_id, u32 max)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	int stride = rx_pool->buf_num_per_page;
	struct dn200_buf_refill_stack *buf_refill = &local_cache->buf_refill;
	struct dn200_buf_cache_ring *buf_cached = &local_cache->buf_cached;
	u64 *refill_objs = (u64 *)&buf_refill->objs[0];
	u64 *objs = (u64 *)buf_cached->objs;
	int index = 0, i = 0, j;
	u32 avail = dn200_page_buf_avail(buf_cached);

	if (unlikely(avail <= DN200_RECY_MIN_CACHE_SIZE))
		return 0;

	if (unlikely(avail > DN200_BUFPOOL_CACHE_MAX_SIZE))
		return dn200_cache_buf_recycle_em(rx_pool, queue_id, max);

	avail = (avail >= stride) ? (avail - stride) : 0;
	max = min(avail, max);
	max = ALIGN(max, stride);
	if (max < stride)
		return 0;

	if (likely((max + buf_cached->tail) < buf_cached->cache_size)) {
		for (index = buf_cached->tail, i = 0; i < max;
		     index += stride, i += stride) {
			struct dn200_page_buf *buf =
			    (struct dn200_page_buf *)objs[index];

			if ((page_ref_count(buf->page) - *(buf->page_ref_bias)) != 0)
				goto ref_unavail;

			for (j = 0; j < stride; j++)
				refill_objs[i + j] = objs[index + j];

		}
	} else {
		for (index = buf_cached->tail, i = 0;
		     index < buf_cached->cache_size;
		     index += stride, i += stride) {
			struct dn200_page_buf *buf =
			    (struct dn200_page_buf *)objs[index];

			if ((page_ref_count(buf->page) - *(buf->page_ref_bias)) != 0)
				goto ref_unavail;

			for (j = 0; j < stride; j++)
				refill_objs[i + j] = objs[index + j];
		}
		for (index = 0; i < max; index += stride, i += stride) {
			struct dn200_page_buf *buf =
			    (struct dn200_page_buf *)objs[index];

			if ((page_ref_count(buf->page) - *(buf->page_ref_bias)) != 0)
				goto ref_unavail;

			for (j = 0; j < stride; j++)
				refill_objs[i + j] = objs[index + j];
		}
	}

ref_unavail:
	dev_dbg(rx_pool->device,
		"%s recycle count %d to refill bufstack now recycle tail %d head %d cache_mask %d cache_size %d\n",
		__func__, i, index & buf_cached->cache_mask, buf_cached->head,
		buf_cached->cache_mask, buf_cached->cache_size);
	buf_cached->tail = index & buf_cached->cache_mask;
	buf_refill->current_len = i;
	return i;
}

#define DN200_USE_RESVERED_BUF 1
static inline int dn200_rx_pool_buf_alloc_n(struct dn200_bufpool *rx_pool,
					    u8 queue_id, u8 count, void **buf)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	struct dn200_buf_refill_stack *buf_refill = &local_cache->buf_refill;
	u64 *cache_objs;
	u64 *buf_objs = (u64 *)buf;
	int ret, recy;
	u32 index, remaining = 0;
	u32 max = count, n;

	if (unlikely(max != 1)) {
		dev_warn(rx_pool->device, "%s only one buf can be alloced for at a time! queue %d\n",
			__func__, queue_id);
		return 0;
	}
	n = min(buf_refill->current_len, max);
	remaining = max - n;

	if (n == 0)
		goto get_remaining;

	cache_objs = (u64 *)&buf_refill->objs[buf_refill->current_len - 1];
	for (index = 0; index < n; index++)
		*buf_objs = *cache_objs;

	buf_refill->current_len -= n;
	if (remaining == 0)
		return 0;


get_remaining:
	recy = dn200_cache_buf_recycle(rx_pool, queue_id,
				       buf_refill->cache_size);
	if (recy)
		goto put_remaining;

	/* Fill the cache from the backend; fetch size + remaining objects. */
	ret = dn200_bufring_dequeue_elem(rx_pool, buf_refill->objs,
					 buf_refill->cache_size, 0);
	if (unlikely(ret <= 0)) {
		dev_dbg(rx_pool->device, "%s get buf from buf_ring failed\n",
			__func__);
		return DN200_USE_RESVERED_BUF;
	}
	buf_refill->current_len = ret;

put_remaining:
	/* Satisfy the remaining part of the request from the filled cache. */
	cache_objs = (u64 *)&buf_refill->objs[buf_refill->current_len - 1];
	n = min(buf_refill->current_len, remaining);
	remaining -= n;
	for (index = 0; index < n; index++)
		*buf_objs = *cache_objs;

	buf_refill->current_len -= n;
	if (unlikely(remaining))
		goto get_remaining;

	return 0;
}

static inline struct dn200_page_buf *
dn200_rx_pool_buf_alloc(struct dn200_bufpool *rx_pool, u8 queue_id, int offset,
			int dma_rx_size)
{
	struct dn200_page_buf *buf = NULL;
	int ret = 0;
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	struct dn200_buf_refill_stack *buf_refill = &local_cache->buf_refill;

	if (unlikely(buf_refill->use_reserve)) {
		if (offset & (rx_pool->buf_num_per_page - 1)) {
			return &rx_pool->page_buf[rx_pool->reserved_start +
						  queue_id * dma_rx_size +
						  offset];
		} else {
			buf_refill->use_reserve = false;
		}
	}

	ret = dn200_rx_pool_buf_alloc_n(rx_pool, queue_id, 1, (void **)&buf);
	if (unlikely(ret == DN200_USE_RESVERED_BUF || !buf)) {
		dev_dbg(rx_pool->device, "%s get buf failed! queue %d\n",
			__func__, queue_id);
		buf = &rx_pool->page_buf[rx_pool->reserved_start +
					 queue_id * dma_rx_size + offset];
		buf_refill->use_reserve = true;
	}
	return buf;
}

#define DN200_BUFCACHED_NEXT_HEAD(h, c) ((h + 1) & c->cache_mask)

#define DN200_BUFCACHED_AVAILD(cache)	((((cache)->tail >= (cache)->head) ? 0 : (cache)->size) + \
	(ring)->tail - (ring)->head)

static inline int dn200_cache_clean(struct dn200_bufpool *rx_pool, u8 queue_id,
				    u32 max)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	int stride = rx_pool->buf_num_per_page;
	struct dn200_buf_cache_ring *buf_cached = &local_cache->buf_cached;
	u64 *objs;
	u32 avail = dn200_page_buf_avail(buf_cached);

	max = ALIGN(max, stride);
	max = min(avail, max);
	if (max < stride)
		return 0;

	max = min(max, (buf_cached->cache_size - buf_cached->tail));
	objs = (u64 *)&buf_cached->objs[buf_cached->tail];
	dn200_bufring_do_enqueue_elem(rx_pool->cached_ring, (void *)objs, max,
				      0);
	buf_cached->tail = (max + buf_cached->tail) & buf_cached->cache_mask;

	return max;
}

static inline void dn200_rx_pool_buf_free(struct dn200_bufpool *rx_pool,
					  u8 queue_id,
					  struct dn200_page_buf *buf)
{
	struct dn200_bufpool_cache *local_cache =
	    &rx_pool->local_cache[queue_id];
	struct dn200_buf_cache_ring *buf_cached = &local_cache->buf_cached;
	u64 *cache_objs;
	u32 head = buf_cached->head;
	u32 free = 0;

	/*reserved page_buf, do not need to be recycled */
	if (unlikely(buf->low_res))
		return;

	(*(buf->page_ref_bias))--;
	if (unlikely(*(buf->page_ref_bias) == (rx_pool->buf_num_per_page - 1))) {
		page_ref_add(buf->page, USHRT_MAX - (rx_pool->buf_num_per_page - 1));
		*(buf->page_ref_bias) = USHRT_MAX;
	}
	free = dn200_page_buf_free(buf_cached);
	if (unlikely(!free)) {
		if (!dn200_cache_clean(rx_pool, queue_id,
				       DN200_BUFPOOL_CACHE_MAX_SIZE)) {
			dev_err(rx_pool->device,
				"%s free to bufpool cached failed!, cache buf full\n",
				__func__);
			return;
		}
	}

	cache_objs = (u64 *)&buf_cached->objs[head];
	head = DN200_BUFCACHED_NEXT_HEAD(head, buf_cached);
	*cache_objs = (u64)buf;
	buf_cached->head = head;
}

struct page *dn200_alloc_dma_page_dir(struct dn200_priv *priv,
				    dma_addr_t *dma_addr, enum dma_data_direction dma_dir);

void dn200_free_dma_page_dir(struct dn200_priv *priv,
					struct page *page, dma_addr_t dma_addr,
					enum dma_data_direction dma_dir);
#endif
