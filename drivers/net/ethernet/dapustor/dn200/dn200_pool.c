// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Guo Feng <guofeng@dapustor.com>
 *
 * Config dn200 ethernet buf pool
 */

#include "dn200.h"
#include "common.h"
#include "dn200_pool.h"
#include "dn200_iatu.h"
#include "dn200_reg.h"

#define DN200_PAGE_ORDER (MAX_ORDER - 1)
#define DN200_RING_SIZE_512 512

bool dma_can_direct_use(struct dn200_priv *priv, dma_addr_t dma_addr)
{
	int addr_bits_limit = priv->plat_ex->addr_bits_limit;
	u64 addr_forbid_bits = priv->plat_ex->addr_forbid_bits;

	/* vf must use iatu and can't direct use the dma */
	if (PRIV_IS_VF(priv))
		return false;

	/* if raid not exist:
	 * 1. for pf: we can use the dma when it is within the addr limit and not be forbidden
	 * 2. for vf: just can support dma32
	 */
	if (!priv->plat_ex->raid_supported) {
		/* dma address longer than addr bits limit (e.g. 40bits for pf or 32bits for vf) */
		if ((dma_addr >> addr_bits_limit) > 0)
			return false;

		/* dma address include all forbidden bits */
		if (addr_forbid_bits != 0 &&
		    ((dma_addr & addr_forbid_bits) == addr_forbid_bits)) {
			return false;
		}
		return true;
	}

	/* if raid exist, all dma address can't be directly used */
	return false;
}

static unsigned int dn200_page_pool_size_get(struct dn200_priv *priv)
{
	unsigned int pool_size = 0;
	unsigned int ring_size = priv->dma_rx_size;
	int muiltple_num = 4;
	unsigned int buf_ring_sz = 0;

	if ((PAGE_SIZE >> DN200_DEFAULT_PAGE_SIZE_SHIFT) > 8)
		muiltple_num *= 4;
	else if ((PAGE_SIZE >> DN200_DEFAULT_PAGE_SIZE_SHIFT) > 4)
		muiltple_num *= 3;
	else if ((PAGE_SIZE >> DN200_DEFAULT_PAGE_SIZE_SHIFT) > 2)
		muiltple_num *= 2;
	if (ring_size <= DN200_RING_SIZE_512)
		ring_size = DN200_RING_SIZE_512;

	buf_ring_sz = priv->plat->rx_queues_to_use * ring_size * muiltple_num;
	if (buf_ring_sz > DN200_MT_32K_MAX_BUF_SIZE)
		buf_ring_sz = DN200_MT_32K_MAX_BUF_SIZE;
	pool_size = buf_ring_sz * DN200_RX_BUF_SIZE;

	return pool_size;
}

static inline struct page *dn200_alloc_page(struct dn200_priv *priv,
					    dma_addr_t *dma_addr,
					    dma_addr_t *base_addr)
{
	struct page *page = NULL;
	dma_addr_t addr = 0;
	gfp_t gfp_mask;

	enum dma_data_direction dma_dir = DMA_FROM_DEVICE;

	/*1. pure & sriov pf: use normal memory to alloc page first, and then check the dma addr
	 *2. vf: don't use normal memory to alloc, use dma32 directly
	 */
	gfp_mask = GFP_ATOMIC | __GFP_NOWARN | __GFP_COMP  |
			     __GFP_MEMALLOC;
	if (priv->plat->addr64 == 32)
		gfp_mask |= GFP_DMA32;

	page =
	    alloc_pages_node(priv->numa_node,
			     gfp_mask, dn200_rx_pg_order_get(priv));
	if (!page) {
		dev_err(priv->device, "no page memory: %s, %d.\n",
			__func__, __LINE__);
		return NULL;
	}

	addr =
	    dma_map_page_attrs(priv->device, page, 0,
			       dn200_rx_pg_size_get(priv), dma_dir,
			       (DMA_ATTR_SKIP_CPU_SYNC |
				DMA_ATTR_WEAK_ORDERING));
	if (dma_mapping_error(priv->device, addr)) {
		dev_err(priv->device, "dma map error: %s, %d.\n", __func__,
			__LINE__);
		__free_pages(page, dn200_rx_pg_order_get(priv));
		return NULL;
	}
	if (dn200_rx_iatu_find(addr, priv, base_addr) < 0) {
		dev_dbg(priv->device,
			"%s, %d, addr iatu find failed, dma_addr:%#llx, page phys:%#llx\n",
			__func__, __LINE__, addr, page_to_phys(page));

		if (page) {
			__free_pages(page, dn200_rx_pg_order_get(priv));
			dma_unmap_page_attrs(priv->device, addr,
					     dn200_rx_pg_size_get(priv),
					     dma_dir,
					     (DMA_ATTR_SKIP_CPU_SYNC |
					      DMA_ATTR_WEAK_ORDERING));
		}

		page = __dev_alloc_pages(GFP_ATOMIC | __GFP_NOWARN | GFP_DMA32,
					 dn200_rx_pg_order_get(priv));
		if (!page) {
			dev_err(priv->device,
				"no page memory: %s, %d, page order:%d\n",
				__func__, __LINE__,
				dn200_rx_pg_order_get(priv));

			return NULL;
		}
		addr =
		    dma_map_page_attrs(priv->device, page, 0,
				       dn200_rx_pg_size_get(priv), dma_dir,
				       (DMA_ATTR_SKIP_CPU_SYNC |
					DMA_ATTR_WEAK_ORDERING));
		if (dma_mapping_error(priv->device, addr)) {
			dev_err(priv->device, "dma map error: %s, %d.\n",
				__func__, __LINE__);
			__free_pages(page, dn200_rx_pg_order_get(priv));
			return NULL;
		}
		/* dma address maybe exceed 32bit for DMA32,
		 * 1. if not exceed, just use DMA32 iATU
		 * 2. if exceed, use iATU mapping same as normal memory
		 */
		if (dn200_rx_iatu_find(addr, priv, base_addr) < 0) {
			dev_err(priv->device,
				"%s, %d, dma32 addr iatu find failed, dma_addr:%#llx, page phys:%#llx\n",
				__func__, __LINE__, addr, page_to_phys(page));
			dma_unmap_page_attrs(priv->device, addr,
					     dn200_rx_pg_size_get(priv),
					     dma_dir,
					     (DMA_ATTR_SKIP_CPU_SYNC |
					      DMA_ATTR_WEAK_ORDERING));
			__free_pages(page, dn200_rx_pg_order_get(priv));
			return NULL;
		}
	}
	*dma_addr = addr;
	return page;
}

static inline void dn200_free_all_pages(struct dn200_priv *priv)
{
	int pg_idx = 0;
	struct dn200_page_pool *page_pool = &priv->page_pool;
	enum dma_data_direction dma_dir = DMA_FROM_DEVICE;

	for (pg_idx = 0; pg_idx < page_pool->alloced_pages; ++pg_idx) {
		if (page_pool->mem_info && page_pool->mem_info[pg_idx].page) {
			dma_unmap_page_attrs(priv->device,
					     page_pool->mem_info[pg_idx].dma_addr,
					     dn200_rx_pg_size_get(priv),
					     dma_dir,
					     (DMA_ATTR_SKIP_CPU_SYNC |
					      DMA_ATTR_WEAK_ORDERING));
			__page_frag_cache_drain(page_pool->mem_info[pg_idx].page, page_pool->mem_info[pg_idx].page_ref_bias);
		}
	}
	page_pool->alloced_pages = 0;
}

static int dn200_page_pool_setup(struct dn200_priv *priv)
{
	int pg_idx = 0;
	struct page *page = NULL;
	struct dn200_page_pool *page_pool = &priv->page_pool;
	struct dn200_mem_info *mem_info;
	int pg_num = 0;

	memset(page_pool, 0, sizeof(*page_pool));
	pg_num = DIV_ROUND_UP(dn200_page_pool_size_get(priv), PAGE_SIZE);
	dev_dbg(priv->device, "pg num: %s, %d, pg_num:%d.\n", __func__,
		__LINE__, pg_num);

	mem_info =
	    vzalloc_node(pg_num * sizeof(struct dn200_mem_info),
			 priv->numa_node);
	if (!mem_info)
		return -ENOMEM;

	page_pool->device = priv->device;
	page_pool->mem_info = mem_info;
	page_pool->page_order = dn200_rx_pg_order_get(priv);
	page_pool->total_pages = pg_num;
	page_pool->alloced_pages = 0;

	for (pg_idx = 0; pg_idx < pg_num; ++pg_idx) {
		dma_addr_t dma_addr;
		dma_addr_t base_addr;

		page = dn200_alloc_page(priv, &dma_addr, &base_addr);
		if (!page) {
			dev_err(priv->device,
				"no page memory: %s, %d, pg_idx:%d.\n",
				__func__, __LINE__, pg_idx);
			goto err_no_mem;
		}
		page_pool->mem_info[pg_idx].page = page;
		page_pool->mem_info[pg_idx].dma_addr = dma_addr;
		page_pool->mem_info[pg_idx].base_addr = base_addr;
		page_ref_add(page, USHRT_MAX - 1);
		page_pool->mem_info[pg_idx].page_ref_bias = USHRT_MAX;
	}
	page_pool->alloced_pages = pg_num;
	dev_dbg(priv->device,
		"page_pool: %s, %d, start pg:0x%p, ord:%d, tot_pgs:%d, tot_size:%ld. alloced_pages %d\n",
		__func__, __LINE__, page_pool->mem_info[0].page,
		page_pool->page_order, page_pool->total_pages,
		page_pool->total_pages * PAGE_SIZE, page_pool->alloced_pages);
	return 0;

err_no_mem:
	page_pool->alloced_pages = pg_idx;
	dev_err(priv->device, "no page memory: %s, %d, request pages:%d.\n",
		__func__, __LINE__, pg_num);
	dn200_free_all_pages(priv);
	return -ENOMEM;
}

static bool dn200_rx_pool_buf_init(struct dn200_bufpool *rx_pool,
				   struct dn200_page_buf *page_buf)
{
	struct dn200_bufring *pool_ring = rx_pool->pool_ring;
	u64 buf_addr = (u64) page_buf;

	page_buf->busy_cnt = 0;
	dn200_bufring_do_init_elem(pool_ring, (void *)&buf_addr, 1);
	if (page_ref_count(page_buf->page) - *(page_buf->page_ref_bias) != 0) {
		dev_err(rx_pool->device,
			" %s dma:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d page_ref_bias %d\n",
			__func__, page_buf->kernel_addr, page_buf->page_offset,
			page_to_phys(page_buf->page), page_buf->buf_len,
			page_buf->busy_cnt, page_buf->page,
			page_ref_count(page_buf->page),
			*(page_buf->page_ref_bias));
	}

	return true;
}

static struct dn200_page_buf *dn200_rx_reserve_buf_init(struct dn200_bufpool *rx_pool,
							struct dn200_page_buf *page_buf)
{
	struct dn200_page_buf *buf;

	page_buf->busy_cnt = 0;

	rx_pool->page_buf[rx_pool->buf_index] = *page_buf;
	buf = &rx_pool->page_buf[rx_pool->buf_index];

	rx_pool->buf_index++;
	return buf;
}

static int dn200_rx_pool_page_add(struct dn200_priv *priv, struct page *page,
				  dma_addr_t *kernel_dma_addr,
				  dma_addr_t *desc_dma_addr,
				  int *page_ref_bias, bool is_reserve)
{
	struct dn200_bufpool *rx_pool = &priv->buf_pool;
	int buf_num_per_pg = PAGE_SIZE / DN200_RX_BUF_SIZE;
	int buf_idx = 0;
	struct dn200_page_buf pg_buf;
	struct dn200_page_buf *buf;
	int rx_buf_size = DN200_RX_BUF_SIZE;

	for (buf_idx = 0; buf_idx < buf_num_per_pg; ++buf_idx) {
		memset(&pg_buf, 0, sizeof(pg_buf));
		pg_buf.buf_addr = page_address(page) + buf_idx * rx_buf_size;
		pg_buf.buf_len = rx_buf_size;
		pg_buf.page = page;
		pg_buf.desc_addr = *desc_dma_addr;
		pg_buf.kernel_addr = *kernel_dma_addr;
		pg_buf.page_offset =
		    buf_idx * rx_buf_size + dn200_rx_offset(priv);
		pg_buf.busy_cnt = 0;
		pg_buf.low_res = is_reserve;
		pg_buf.page_ref_bias = page_ref_bias;
		buf = dn200_rx_reserve_buf_init(rx_pool, &pg_buf);
		if (!is_reserve)
			dn200_rx_pool_buf_init(rx_pool, buf);
	}
	return 0;
}

static int dn200_rx_pool_init(struct dn200_priv *priv)
{
	struct dn200_page_pool *page_pool = &priv->page_pool;
	struct dn200_bufpool *buf_pool = &priv->buf_pool;
	struct dn200_bufring *buf_ring;
	struct dn200_bufring *cached_ring = NULL;
	struct dn200_page_buf *page_buf_arr;
	int queue;
	int buf_num_per_pg = PAGE_SIZE / DN200_RX_BUF_SIZE;
	int buf_num = page_pool->total_pages * buf_num_per_pg;	//rx_buf_pgs;
	int ring_size;
	int reserved = priv->plat->rx_queues_to_use * priv->dma_rx_size;

	memset(buf_pool, 0, sizeof(*buf_pool));
	page_pool->reserve_page = reserved / buf_num_per_pg;
	dev_dbg(priv->device,
		"%s reserve page size %d reserved buf %d total buf_num %d\n",
		__func__, page_pool->reserve_page, reserved, buf_num);
	if (buf_num < reserved) {
		dev_err(priv->device,
			"%s no enough pagebuf! now has %d, %d required at least!\n",
			__func__, buf_num, reserved);
		return -ENOMEM;
	}
	ring_size = roundup_pow_of_two((buf_num - reserved + 1));
	buf_ring = vzalloc_node(sizeof(*buf_ring), priv->numa_node);
	if (!buf_ring) {
		dev_err(priv->device, "%s failed to alloc buf_ring\n",
			__func__);
		return -ENOMEM;
	}
	buf_ring->ring_objs =
	    vzalloc_node(sizeof(u64) * ring_size, priv->numa_node);
	if (!buf_ring->ring_objs) {
		dev_err(priv->device, "%s failed to alloc buf_ring ring_objs\n",
			__func__);
		vfree(buf_ring);
		return -ENOMEM;
	}

	cached_ring = vzalloc_node(sizeof(*cached_ring), priv->numa_node);
	if (!cached_ring)
		goto free_buf_ring;
	cached_ring->ring_objs =
	    vzalloc_node(sizeof(u64) * ring_size, priv->numa_node);
	if (!cached_ring->ring_objs) {
		dev_err(priv->device,
			"%s failed to alloc cached_ring ring_objs\n", __func__);
		vfree(cached_ring);
		goto free_buf_ring;
	}

	buf_ring->ring_size = ring_size;
	buf_ring->ring_mask = ring_size - 1;
	buf_ring->device = priv->device;
	buf_ring->buf_num_per_page = buf_num_per_pg;
	atomic_set(&buf_ring->cons.head, 0);
	atomic_set(&buf_ring->cons.tail, 0);
	atomic_set(&buf_ring->prod.head, 0);
	atomic_set(&buf_ring->prod.tail, 0);

	cached_ring->ring_size = ring_size;
	cached_ring->ring_mask = ring_size - 1;
	cached_ring->device = priv->device;
	cached_ring->buf_num_per_page = buf_num_per_pg;
	atomic_set(&cached_ring->cons.head, 0);
	atomic_set(&cached_ring->cons.tail, 0);
	atomic_set(&cached_ring->prod.head, 0);
	atomic_set(&cached_ring->prod.tail, 0);
	dev_dbg(priv->device, "%s bufring size %d mask %#x\n", __func__,
		ring_size, ring_size - 1);
	page_buf_arr = vzalloc_node(sizeof(*page_buf_arr) * buf_num,
				    priv->numa_node);
	if (!page_buf_arr)
		goto err_out;

	buf_pool->pool_ring = buf_ring;
	buf_pool->cached_ring = cached_ring;
	buf_pool->buf_num_per_page = buf_num_per_pg;
	buf_pool->pool_size = buf_num - reserved;
	buf_pool->reserved_start = buf_pool->pool_size;
	buf_pool->buf_index = 0;
	buf_pool->device = priv->device;
	buf_pool->page_buf = page_buf_arr;
	buf_pool->cache_size =
	    min_t(unsigned int, DN200_BUFPOOL_CACHE_MAX_SIZE, priv->dma_rx_size);

	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		rx_q->rx_pool = buf_pool;
	}
	return 0;

err_out:
	vfree(cached_ring->ring_objs);
	vfree(cached_ring);
free_buf_ring:
	vfree(buf_ring->ring_objs);
	vfree(buf_ring);
	return -ENOMEM;
}

static int dn200_local_cache_init(struct dn200_priv *priv)
{
	struct dn200_bufpool_cache *local_cache;
	struct dn200_buf_refill_stack *buf_refill;
	struct dn200_buf_cache_ring *buf_cached;
	int queue = 0;

	local_cache = vzalloc_node(sizeof(*local_cache) *
					   priv->plat->rx_queues_to_use,
				   priv->numa_node);
	if (!local_cache)
		return -ENOMEM;
	priv->buf_pool.local_cache = local_cache;

	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++) {
		/*INIT refill cache, stack model */
		buf_refill = &local_cache->buf_refill;
		buf_refill->cache_size =
		    roundup_pow_of_two(priv->buf_pool.cache_size);
		buf_refill->current_len = 0;
		/*INIT cache ring, sp_sc ring model */
		buf_cached = &local_cache->buf_cached;
		buf_cached->cache_size = DN200_BUFPOOL_CACHE_MAX_SIZE * 4;
		buf_cached->head = 0;
		buf_cached->tail = 0;
		buf_cached->cache_mask = buf_cached->cache_size - 1;

		buf_refill->device = priv->device;
		buf_cached->device = priv->device;
		local_cache++;
	}

	return 0;
}

static int dn200_rx_pool_alloc(struct dn200_priv *priv)
{
	int pg_idx = 0;
	struct dn200_page_pool *page_pool = NULL;
	dma_addr_t kernel_dma_addr;
	dma_addr_t desc_dma_addr;
	struct page *page = NULL;

	page_pool = &priv->page_pool;
	for (pg_idx = 0;
	     pg_idx < page_pool->total_pages - page_pool->reserve_page;
	     pg_idx++) {
		page = page_pool->mem_info[pg_idx].page;
		desc_dma_addr = page_pool->mem_info[pg_idx].base_addr;
		kernel_dma_addr = page_pool->mem_info[pg_idx].dma_addr;
		if (!page) {
			dev_err(priv->device,
				"page is null: %s, %d, pg_idx:%d.\n", __func__,
				__LINE__, pg_idx);
			return -ENOMEM;
		}
		dn200_rx_pool_page_add(priv, page, &kernel_dma_addr,
				       &desc_dma_addr,
					   &page_pool->mem_info[pg_idx].page_ref_bias,
					   false);
	}

	for (; pg_idx < page_pool->total_pages; pg_idx++) {
		page = page_pool->mem_info[pg_idx].page;
		desc_dma_addr = page_pool->mem_info[pg_idx].base_addr;
		kernel_dma_addr = page_pool->mem_info[pg_idx].dma_addr;
		if (!page) {
			dev_err(priv->device,
				"page is null: %s, %d, pg_idx:%d.\n", __func__,
				__LINE__, pg_idx);
			return -ENOMEM;
		}
		dn200_rx_pool_page_add(priv, page, &kernel_dma_addr,
				       &desc_dma_addr,
					   &page_pool->mem_info[pg_idx].page_ref_bias,
					   true);
	}
	dev_dbg(priv->device, "%s put %d page_buf to buf_pool buf_ring %d\n",
		__func__, priv->buf_pool.buf_index,
		atomic_read(&priv->buf_pool.pool_ring->prod.head));

	return 0;
}

int dn200_rx_pool_setup(struct dn200_priv *priv)
{
	if (dn200_page_pool_setup(priv))
		return -ENOMEM;

	if (dn200_rx_pool_init(priv))
		return -ENOMEM;

	if (dn200_rx_pool_alloc(priv))
		return -ENOMEM;

	if (dn200_local_cache_init(priv))
		return -ENOMEM;
	return 0;
}

void dn200_rx_pool_destory(struct dn200_priv *priv)
{
	struct dn200_page_pool *page_pool = &priv->page_pool;
	struct dn200_bufpool *buf_pool = &priv->buf_pool;
	struct dn200_bufring *buf_ring = buf_pool->pool_ring;
	struct dn200_bufring *cached_ring = buf_pool->cached_ring;
	int queue;

	if (priv->buf_pool.local_cache) {
		vfree(priv->buf_pool.local_cache);
		priv->buf_pool.local_cache = NULL;
	}
	if (buf_pool->page_buf) {
		vfree(buf_pool->page_buf);
		buf_pool->page_buf = NULL;
	}
	if (buf_ring) {
		vfree(buf_ring->ring_objs);
		vfree(buf_ring);
		buf_pool->pool_ring = NULL;
	}
	if (cached_ring) {
		vfree(cached_ring->ring_objs);
		vfree(cached_ring);
		buf_pool->cached_ring = NULL;
	}
	dn200_free_all_pages(priv);

	for (queue = 0; queue < priv->plat->rx_queues_to_use; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		rx_q->rx_pool = NULL;
	}

	if (page_pool->mem_info) {
		vfree(page_pool->mem_info);
		page_pool->mem_info = NULL;
	}

	memset(&priv->page_pool, 0, sizeof(priv->page_pool));
	memset(&priv->buf_pool, 0, sizeof(priv->buf_pool));
}

struct page *dn200_alloc_dma_page_dir(struct dn200_priv *priv,
				    dma_addr_t *dma_addr, enum dma_data_direction dma_dir)
{
	struct page *page = NULL;
	dma_addr_t base_addr;
	dma_addr_t addr = 0;


	page = alloc_pages_node(priv->numa_node,
		GFP_ATOMIC | __GFP_NOWARN | __GFP_ZERO | __GFP_COMP | __GFP_MEMALLOC, 0);
	if (!page) {
		dev_err(priv->device, "no page memory: %s, %d.\n", __func__, __LINE__);
		return NULL;
	}

	addr = dma_map_page_attrs(priv->device, page, 0, PAGE_SIZE, dma_dir,
			       (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
	if (dma_mapping_error(priv->device, addr)) {
		dev_err(priv->device, "dma map error: %s, %d.\n", __func__, __LINE__);
		__free_pages(page, 0);
		return NULL;
	}

	if (priv->plat_ex->raid_supported)
		goto result;

	/*judge addr exceed 40 bit or 32-40bit should not equal 0xE0 ~ 0xFF*/
	if ((addr >> 40) || (!(addr & GENMASK(39, 37))))
		goto result;

	if (test_bit(DN200_IATU_INIT, &priv->state)) {
		if (dn200_rx_iatu_find(addr, priv, &base_addr) < 0) {
			dev_dbg(priv->device,
				"%s, %d, addr iatu find failed, dma_addr:%#llx, page phys:%#llx\n",
				__func__, __LINE__, addr, page_to_phys(page));

			if (page) {
				__free_pages(page, 0);
				dma_unmap_page_attrs(priv->device, addr, PAGE_SIZE, dma_dir,
							(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
		}

			page = __dev_alloc_pages(GFP_ATOMIC | __GFP_NOWARN | GFP_DMA32, 0);
			if (!page) {
				dev_err(priv->device, "no page memory: %s, %d, page order:%d\n",
					__func__, __LINE__, 0);

				return NULL;
			}
			addr = dma_map_page_attrs(priv->device, page, 0, PAGE_SIZE, dma_dir,
						(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
			if (dma_mapping_error(priv->device, addr)) {
				dev_err(priv->device, "dma map error: %s, %d.\n",
					__func__, __LINE__);
				__free_pages(page, 0);
				return NULL;
			}
			/* dma address maybe exceed 32bit for DMA32,
			 * 1. if not exceed, just use DMA32 iATU
			 * 2. if exceed, use iATU mapping same as normal memory
			 */
			if (dn200_rx_iatu_find(addr, priv, &base_addr) < 0) {
				dev_err(priv->device,
					"%s, %d, dma32 addr iatu find failed, dma_addr:%#llx, page phys:%#llx\n",
					__func__, __LINE__, addr, page_to_phys(page));
				dma_unmap_page_attrs(priv->device, addr, PAGE_SIZE, dma_dir,
							(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
				__free_pages(page, 0);
				return NULL;

			}
		}
	} else {
		if (!(addr >> MAX_LIMIT_RANGE_SHIFT))
			goto result;

		if (page) {
			__free_pages(page, 0);
			dma_unmap_page_attrs(priv->device, addr, PAGE_SIZE, dma_dir,
							(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
		}
		page = __dev_alloc_pages(GFP_ATOMIC | __GFP_NOWARN | GFP_DMA32, 0);
		if (!page) {
			dev_err(priv->device, "no page memory: %s, %d, page order:%d\n",
				__func__, __LINE__, 0);

			return NULL;
		}
		addr = dma_map_page_attrs(priv->device, page, 0, PAGE_SIZE, dma_dir,
						(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
		if (dma_mapping_error(priv->device, addr)) {
			dev_err(priv->device, "dma map error: %s, %d.\n",
				__func__, __LINE__);
			__free_pages(page, 0);
			return NULL;
		}
	}

result:
	*dma_addr = addr;
	return page;
}

void dn200_free_dma_page_dir(struct dn200_priv *priv,
					struct page *page, dma_addr_t dma_addr,
					enum dma_data_direction dma_dir)
{

	dma_unmap_page_attrs(priv->device, dma_addr, PAGE_SIZE, dma_dir,
			     (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING));
	__free_pages(page, 0);
}

