// SPDX-License-Identifier: GPL-2.0
#include <asm-generic/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <asm/barrier.h>
#include <linux/etherdevice.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/uaccess.h>
#include <linux/iopoll.h>
#include <linux/llist.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

#include "yk3_doe_process.h"
/**************************************************************************************************/
static unsigned int yk3_hmc_size;
module_param_named(hmc_size, yk3_hmc_size, uint, 0444);
MODULE_PARM_DESC(yk3_hmc_size, "doe hmc size(Unit: 1MB)");

static unsigned long yk3_hmc_address;
module_param_named(hmc_address, yk3_hmc_address, ulong, 0444);
MODULE_PARM_DESC(yk3_hmc_address, "doe hmc address(reserved memory address)");

static unsigned long yk3_huge2m[512];
static int yk3_huge2m_count;
module_param_array_named(huge2m, yk3_huge2m, ulong, &yk3_huge2m_count, 0644);
MODULE_PARM_DESC(yk3_huge2m, "max 512");

static unsigned long yk3_huge1g[64];
static int yk3_huge1g_count;
module_param_array_named(huge1g, yk3_huge1g, ulong, &yk3_huge1g_count, 0644);
MODULE_PARM_DESC(yk3_huge1g, "max 64");
/**************************************************************************************************/
int yk3_doe_param_init(void)
{
	if (yk3_hmc_address && (yk3_hmc_address & YK3_DOE_PAGE_MASK_MIN)) {
		// {}
		return doe_error(-E_DOE_INVALID,
				 "hmc address[0x%lx] must be a multiple of 2M", yk3_hmc_address);
	}

	return 0;
}

static enum hrtimer_restart
doe_hmc_fast_timer(struct hrtimer *timer)
{
	struct yk3_doe_fast *fast = container_of(timer, struct yk3_doe_fast, timer);

	if (READ_ONCE(fast->stop))
		return HRTIMER_NORESTART;

	queue_delayed_work(fast->wq, &fast->work, 0);
	if (READ_ONCE(fast->count))
		fast->empty_times = 0;
	else
		fast->empty_times++;

	if (fast->empty_times < fast->empty_threshold) {
		fast->n_timer_busy++;
		hrtimer_forward_now(timer, fast->busy_interval);
	} else {
		fast->n_timer_slow++;
		hrtimer_forward_now(timer, fast->slow_interval);
	}

	return HRTIMER_RESTART;
}

static void
doe_hmc_fast_start(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;

	if (doe_priv->hmc.enable) {
		// only hmc start timer
		hrtimer_start(&fast->timer, fast->busy_interval, HRTIMER_MODE_REL);
	}
}

static int
doe_hmc_fast_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;
	char name[128];

	// list
	spin_lock_init(&fast->lock);
	INIT_LIST_HEAD(&fast->list);
	fast->threshold = YK3_DOE_FAST_CMD_BATCH_MAX;

	// worker
	snprintf(name, sizeof(name), "%s-fast-wq", doe_priv->pdev_priv->name);
	fast->wq = alloc_workqueue(name, WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (!fast->wq)
		return doe_error(-ENOMEM, "alloc fast-wq failed");
	INIT_DELAYED_WORK(&fast->work, doe_fast_worker);

	// timer
	hrtimer_init(&fast->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fast->timer.function = doe_hmc_fast_timer;
	fast->busy_interval = ktime_set(0, YK3_DOE_FAST_TIMER_BUSY_INTERVAL);
	fast->slow_interval = ktime_set(0, YK3_DOE_FAST_TIMER_SLOW_INTERVAL);
	fast->empty_threshold = 5000;

	return 0;
}

static int
doe_hmc_fast_fini(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;

	WRITE_ONCE(fast->stop, 1);
	hrtimer_cancel(&fast->timer);
	cancel_delayed_work_sync(&fast->work);
	destroy_workqueue(fast->wq);

	return 0;
}

static int
doe_hmc_mem_map(struct yk3_doe_priv *doe_priv, struct yk3_doe_page *page)
{
	page->va = memremap(page->pa, page->size, MEMREMAP_WC);
	if (!page->va) {
		// {}
		return doe_error(-E_DOE_NOMEM,
				 "memremap hmc pa[0x%llx] size[%uM] error",
				 page->pa, YK3_MB(page->size));
	}
	yk3_memzero(page->va, page->size);
	doe_debug(DOE_DEBUG_INIT, "memremap pa[0x%llx] size[%uM] => va[0x%llx]",
		  page->pa, YK3_MB(page->size), YK3_PTR(page->va));

	page->dma = dma_map_resource(doe_priv->dev, page->pa, page->size, DMA_BIDIRECTIONAL, 0);
	if (dma_mapping_error(doe_priv->dev, page->dma)) {
		// {}
		return doe_error(-E_DOE_NOMEM,
				 "dma map pa[0x%llx] size[%uM] error",
				 page->pa, YK3_MB(page->size));
	}
	doe_debug(DOE_DEBUG_INIT, "dma map pa[0x%llx] size[%uM] => dma[0x%llx]",
		  page->pa, YK3_MB(page->size), page->dma);

	return 0;
}

static void
doe_hmc_mem_unmap(struct yk3_doe_priv *doe_priv, struct yk3_doe_page *page)
{
	if (page->dma) {
		dma_unmap_resource(doe_priv->dev, page->dma, page->size, DMA_BIDIRECTIONAL, 0);
		page->dma = 0;
	}

	if (page->va) {
		memunmap(page->va);
		page->va = NULL;
	}
}

static void
doe_hmc_auto_fini(struct yk3_doe_priv *doe_priv)
{
	yk3_do_nothing();
}

static int
doe_hmc_auto_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group = &doe_priv->page_group[DOE_PAGE_GROUP_2M];
	struct yk3_doe_page *page;
	u64 page_size	= group->page_size;
	u32 page_count  = group->page_count;
	u32 i;

	for (i = 0; i < page_count; i++) {
		page = &group->pages[i];

		page->va = doe_dmam_zalloc_coherent(doe_priv, page_size, &page->dma);
		if (!page->va)
			return doe_error(-E_DOE_NOMEM, "hmc auto alloc page[%d] size[%uM] failed",
					 i, YK3_MB(page_size));
	}
	doe_debug(DOE_DEBUG_INIT, "hmc auto alloc total: %uM", YK3_MB(group->total));

	return 0;
}

static void
doe_hmc_reserved_fini(struct yk3_doe_priv *doe_priv)
{
	doe_hmc_mem_unmap(doe_priv, &doe_priv->hmc.reserved);
}

static int
doe_hmc_reserved_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_hmc *hmc = &doe_priv->hmc;
	struct yk3_doe_page_group *group = &doe_priv->page_group[DOE_PAGE_GROUP_1G];
	struct yk3_doe_page *page;
	u64 page_size = group->page_size; // yes, u64
	u32 i, page_count = group->page_count;
	int ret;

	hmc->reserved.pa = yk3_hmc_address;
	hmc->reserved.size = group->total;

	ret = doe_hmc_mem_map(doe_priv, &hmc->reserved);
	if (ret < 0)
		return ret;

	for (i = 0; i < page_count; i++) {
		page = &group->pages[i];

		page->pa	= hmc->reserved.pa  + i * page_size;
		page->va	= hmc->reserved.va  + i * page_size;
		page->dma	= hmc->reserved.dma + i * page_size;
		page->size	= page_size;

		doe_debug(DOE_DEBUG_INIT, "hmc reserved page[%d] size[%uM] pa[0x%llx] va[0x%llx] dma[0x%llx]",
			  i, YK3_MB(page->size), page->pa, YK3_PTR(page->va), page->dma);
	}

	return 0;
}

static void
doe_hmc_hugepage_fini(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group;
	struct yk3_doe_page *page;
	u32 i, j;

	for (i = 0; i < DOE_PAGE_GROUP_END; i++) {
		group = &doe_priv->page_group[i];

		for (j = 0; j < group->page_count; j++) {
			page = &group->pages[j];

			doe_hmc_mem_unmap(doe_priv, page);
		}
	}
}

static int
doe_hmc_hugepage_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group;
	struct yk3_doe_page *page;
	u32 i, j;
	int ret;

	for (i = 0; i < DOE_PAGE_GROUP_END; i++) {
		group = &doe_priv->page_group[i];

		for (j = 0; j < group->page_count; j++) {
			page = &group->pages[j];

			page->size = group->page_size;
			page->pa = (i == DOE_PAGE_GROUP_2M) ? yk3_huge2m[i] : yk3_huge1g[i];

			ret = doe_hmc_mem_map(doe_priv, page);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int
doe_hmc_common_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group2M = &doe_priv->page_group[DOE_PAGE_GROUP_2M];
	struct yk3_doe_page_group *group1G = &doe_priv->page_group[DOE_PAGE_GROUP_1G];
	struct yk3_doe_hmc *hmc = &doe_priv->hmc;
	struct yk3_doe_page *pages;
	u32 page_count;
	int ret;

	group2M->page_size = YK3_2MB;
	group1G->page_size = YK3_1GB;

	// 1. hmc mode
	if (yk3_huge2m_count || yk3_huge1g_count) {
		hmc->mode = DOE_HMC_MODE_HUGEPAGE;

		if (yk3_huge2m_count) {
			group2M->total = yk3_huge2m_count * YK3_2MB;
			group2M->page_count = yk3_huge2m_count;
			hmc->size += group2M->total;
		}

		if (yk3_huge1g_count) {
			group1G->total = yk3_huge1g_count * YK3_1GB;
			group1G->page_count = yk3_huge1g_count;
			hmc->size += group1G->total;
		}
	} else if (yk3_hmc_size) {
		hmc->size = yk3_hmc_size * YK3_1MB;

		if (yk3_hmc_address) {
			hmc->mode = DOE_HMC_MODE_RESERVED;
			group1G->total = hmc->size;
			group1G->page_count = ALIGN(hmc->size, YK3_1GB) / YK3_1GB;
		} else {
			hmc->mode = DOE_HMC_MODE_AUTO;
			group2M->total = hmc->size;
			group2M->page_count = ALIGN(hmc->size, YK3_2MB) / YK3_2MB;
		}
	}
	doe_info("hmc mode: %s", doe_hmc_mode_name(hmc->mode));
	doe_info("hmc group2m total[%uM] page[%u]", YK3_MB(group2M->total), group2M->page_count);
	doe_info("hmc group1g total[%uM] page[%u]", YK3_MB(group1G->total), group1G->page_count);

	if (hmc->size > YK3_DOE_HMC_MAXSIZE)
		return doe_error(-E_DOE_NOMEM, "hmc size[%uM] > %uG",
				 YK3_MB(hmc->size), YK3_GB(YK3_DOE_HMC_MAXSIZE));

	// 2. page map
	page_count = group2M->page_count + group1G->page_count;
	if (page_count > YK3_DOE_PAGE_LIMIT)
		return doe_error(-E_DOE_INVALID, "too more page count[%u]", page_count);
	doe_priv->page_count = page_count;

	pages = devm_kzalloc(doe_priv->dev, sizeof(*pages) * page_count, GFP_KERNEL);
	if (!pages)
		return doe_error(-E_DOE_NOMEM, "no memory @ page alloc");
	doe_priv->pages = pages;

	// 3. page group
	group2M->pages = pages;
	group2M->first_page_index = 0;
	group2M->start = 0;
	group2M->end = group2M->start + group2M->total;

	group1G->pages = pages + group2M->page_count;
	group1G->first_page_index = group2M->page_count;
	group1G->start = group2M->end;
	group1G->end = group1G->start + group1G->total;

	// 4. ddr mm
	doe_priv->ddr = doe_mm_create(doe_priv, hmc->size, YK3_DOE_DDR_MASK, false, "hmc");
	// pipeline force use PTR_ERR_OR_ZERO
	ret = PTR_ERR_OR_ZERO(doe_priv->ddr);
	if (ret)
		return ret;

	// 5. index-sram mm
	doe_priv->index_sram = doe_mm_create(doe_priv, doe_priv->chip_spec->index_sram_size, 1,
					     false, "index_sram");
	// pipeline force use PTR_ERR_OR_ZERO
	ret = PTR_ERR_OR_ZERO(doe_priv->index_sram);
	if (ret)
		return ret;

	return 0;
}

static int
doe_hmc_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_hmc *hmc = &doe_priv->hmc;
	int ret;

	// 1. hmc enable ?
	hmc->enable = !!yk3_hmc_size || !!yk3_huge2m_count || !!yk3_huge1g_count;
	if (!hmc->enable) {
		doe_info("non-ddr mode");
		return 0;
	}

	doe_info("hmc mode");
#if YK3_DOE_U200
	return doe_error(-E_DOE_NOTSUPP, "u200 NOT support hmc mode");
#endif
	// TODO: driver or emp change it ?
	// doe_dma_select(doe_priv);
	doe_hmc_fast_init(doe_priv);

	// 2. hmc common init
	ret = doe_hmc_common_init(doe_priv);
	if (ret < 0)
		return ret;

	// 3. hmc init by mode
	switch (hmc->mode) {
	case DOE_HMC_MODE_RESERVED:
		ret = doe_hmc_reserved_init(doe_priv);
		break;
	case DOE_HMC_MODE_HUGEPAGE:
		ret = doe_hmc_hugepage_init(doe_priv);
		break;
	case DOE_HMC_MODE_AUTO:
	default:
		ret = doe_hmc_auto_init(doe_priv);
		break;
	}
	if (ret < 0)
		return ret;

	// 4. hmc page init
	return doe_page_init(doe_priv);
}

static void
doe_hmc_fini(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_hmc *hmc = &doe_priv->hmc;

	if (!hmc->enable)
		return;

	doe_page_fini(doe_priv);

	switch (hmc->mode) {
	case DOE_HMC_MODE_RESERVED:
		doe_hmc_reserved_fini(doe_priv);
		break;
	case DOE_HMC_MODE_HUGEPAGE:
		doe_hmc_hugepage_fini(doe_priv);
		break;
	case DOE_HMC_MODE_AUTO:
	default:
		doe_hmc_auto_fini(doe_priv);
		break;
	}

	doe_hmc_fast_fini(doe_priv);
}

int doe_fifo_init(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_fifo *fifo = &doe_if->fifo;
	u32 reg_cons = YK3_DOE_REG_FIFO_CNT_OUT(doe_if->is_read);
	u32 init_cons = doe_rd32(doe_if->doe_priv, reg_cons);

	spin_lock_init(&fifo->lock);
	fifo->pool = kcalloc(YK3_DOE_FIFO_DEPTH, sizeof(void *), GFP_KERNEL);
	if (!fifo->pool)
		return doe_error(-E_DOE_NOMEM, "fifo alloc failed");

	fifo->reg_cons	= reg_cons;
	fifo->init_cons	= init_cons;
	fifo->prod	= init_cons;
	fifo->cons	= DOE_FIFO_CONS(init_cons);
#if !YK3_DOE_FIFO_NEW
	// cb pool[0] push to fifo @ init
	fifo->pool[fifo->cons & YK3_DOE_FIFO_MASK] = doe_if->cb_pool.pool[0];
#endif

	return 0;
}

void doe_fifo_fini(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_fifo *fifo = &doe_if->fifo;

	kfree(fifo->pool);
}

static u32
doe_fifo_cons_hw(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;

	// k3
	if (is_doe_k3(doe_priv)) {
		// k3, read from hardware
		return doe_rd32(doe_priv, doe_if->fifo.reg_cons);
	}

	// k3max, read from evq's desc(dma)
	return doe_evq_fifo_cons(doe_if->evq.desc, doe_if->is_read);
}

static void
doe_fifo_dequeue(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_fifo *fifo = &doe_if->fifo;
	u32 hw_cons = doe_fifo_cons_hw(doe_if);
	u32 new_cons = DOE_FIFO_CONS(hw_cons);
	u32 old_cons = fifo->cons;
	u32 count;

	// 1. calc fifo pop(consume) count
	count = new_cons - old_cons;
	if (count) {
		// 2. fifo pop and push to cb-pool
		doe_cb_pool_put_ex(doe_if, fifo, old_cons, count);

		// 3. update fifo cons
		fifo->cons = new_cons;
		fifo->times_cons_update++;
	}
	fifo->times_dequeue++;
}

int doe_fifo_enqueue(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_fifo *fifo = &doe_if->fifo;
	struct yk3_doe_vfc *vfc = &doe_if->vfc;
	u64 ns_start = ktime_get_ns();
	s64 cmd_count = cb->cmd_count;
	bool success = false;
	bool is_fifo_enough, is_otf_check_pass;

	do {
		if (unlikely(is_doe_hw_failure(doe_priv)))
			return doe_error(-E_DOE_HWCLOSED, "hardware closed @ fifo enqueue");

		spin_lock(&fifo->lock);
		// check fifo
		is_fifo_enough = is_doe_fifo_enough(fifo);
		if (unlikely(!is_fifo_enough)) {
			// pop fifo ==> cb pool @ full
			doe_fifo_dequeue(doe_if);
			is_fifo_enough = is_doe_fifo_enough(fifo);
		}

		// check on-the-fly
		is_otf_check_pass = is_doe_otf_check_pass(doe_if, cmd_count);
		if (unlikely(!is_otf_check_pass)) {
			// sync the newest evq's event count ==> vfc @ check failed
			doe_vfc_cons_set(vfc, doe_evq_event_count(&doe_if->evq));
			is_otf_check_pass = is_doe_otf_check_pass(doe_if, cmd_count);
		}

		if (is_fifo_enough && is_otf_check_pass) {
			// doorbell
			doe_doorbell_trigger(doe_if, cb);
			// push cmd count ==> vfc
			doe_vfc_prod_add(vfc, cmd_count);
			// push 1 cb ==> fifo
			__doe_fifo_enqueue(fifo, cb);
			success = true;
		}
#if YK3_DOE_CB_POOL_SMALL
		if (doe_fifo_left(fifo) < (YK3_DOE_FIFO_DEPTH / 4))
			doe_fifo_dequeue(doe_if);
#endif
		spin_unlock(&fifo->lock);

		if (unlikely(!success && is_doe_timeout(ns_start, DOE_OTF_TIMEOUT))) {
			usleep_range(1, 2); // avoid cpu stuck
			return doe_error(-E_DOE_TIMEOUT, "timeout @ %s fifo enqueue", doe_if->name);
		}
	} while (!success);

	return 0;
}

int doe_cb_pool_init(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_cb_pool *pool = &doe_if->cb_pool;
	struct yk3_doe_if_cfg *cfg = doe_if->cfg;
	struct yk3_doe_cmd_buffer *cb;
	u32 cmd_buffer_size = cfg->cmd_buffer_size;

	bool single = (cmd_buffer_size < YK3_DOE_PAGE_SIZE) || YK3_DOE_CB_POOL_SMALL;
	u32 dma_total;
	int i;

	spin_lock_init(&pool->lock);
	pool->cmd_buffer_size = cmd_buffer_size;
	pool->prod = YK3_DOE_CB_POOL_DEPTH;
#if !YK3_DOE_FIFO_NEW
	// cons = 1 @ init
	// pool[0] push to fifo @ init
	pool->cons = 1;
#endif

	pool->pool = kcalloc(YK3_DOE_CB_POOL_DEPTH, sizeof(void *), GFP_KERNEL);
	if (!pool->pool)
		return doe_error(-E_DOE_NOMEM, "cb pool alloc failed");

	pool->cb = kcalloc(YK3_DOE_CB_POOL_DEPTH, sizeof(*pool->cb), GFP_KERNEL);
	if (!pool->cb)
		return doe_error(-E_DOE_NOMEM, "cb pool alloc failed");

	if (single) {
		dma_total = cmd_buffer_size * YK3_DOE_CB_POOL_DEPTH;
		pool->single.base = doe_dmam_zalloc_coherent(doe_priv,
							     dma_total, &pool->single.dma_base);
		if (!pool->single.base)
			return doe_error(-E_DOE_NOMEM, "tag pool dma alloc failed");
		doe_debug(DOE_DEBUG_INIT, "%s single cb dma-base[0x%llx] base[0x%llx]",
			  doe_if->name, pool->single.dma_base, YK3_PTR(pool->single.base));
	}

	for (i = 0; i < YK3_DOE_CB_POOL_DEPTH; i++) {
		cb = &pool->cb[i];
		pool->pool[i] = cb;

		doe_cb_clean(cb);
		cb->id = i;
		cb->size = cmd_buffer_size;
		if (single) {
			cb->base	= pool->single.base     + i * cmd_buffer_size;
			cb->dma_base	= pool->single.dma_base + i * cmd_buffer_size;
		} else {
			cb->base = doe_dmam_zalloc_coherent(doe_priv,
							    cmd_buffer_size, &cb->dma_base);
			if (!cb->base)
				return doe_error(-E_DOE_NOMEM, "tag pool dma alloc failed");
			doe_debug(DOE_DEBUG_INIT, "%s multi cb[%d] dma-base[0x%llx] base[0x%llx]",
				  doe_if->name, i, cb->dma_base, YK3_PTR(cb->base));
		}
	}

	return 0;
}

void doe_cb_pool_fini(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_cb_pool *pool = &doe_if->cb_pool;

	kfree(pool->cb);
	kfree(pool->pool);
}

int doe_cb_pool_get(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer **p_cb)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_cb_pool *pool = &doe_if->cb_pool;
	struct yk3_doe_cmd_buffer *cb = NULL;
	u64 ns_start = ktime_get_ns();
	bool success = false;

	do {
		if (unlikely(is_doe_hw_failure(doe_priv)))
			return doe_error(-E_DOE_HWCLOSED, "hardware closed @ cb-pool get");

		spin_lock(&pool->lock);
		if (!is_doe_cb_pool_empty(pool)) {
			// sonsume one cb
			cb = pool->pool[pool->cons++ & YK3_DOE_CB_POOL_MASK];
			success = true;
		}
		spin_unlock(&pool->lock);

		if (unlikely(!success && is_doe_timeout(ns_start, DOE_CB_POOL_TIMEOUT))) {
			usleep_range(1, 2); // avoid cpu stuck
			return doe_error(-E_DOE_TIMEOUT,
					 "timeout @ %s cb-pool get, prod[%llu] cons[%llu]",
					 doe_if->name, pool->prod, pool->cons);
		}
	} while (!success);

	if (likely(success)) {
		doe_cb_clean(cb);
		*p_cb = cb;
	}

	return 0;
}

void doe_cb_pool_put(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb)
{
	struct yk3_doe_cb_pool *pool = &doe_if->cb_pool;

	spin_lock(&pool->lock);
	pool->pool[pool->prod++ & YK3_DOE_CB_POOL_MASK] = cb;
	spin_unlock(&pool->lock);
}

void doe_cb_pool_put_ex(struct yk3_doe_if *doe_if, struct yk3_doe_fifo *fifo, u32 start, u32 count)
{
	struct yk3_doe_cb_pool *cb_pool = &doe_if->cb_pool;
	u64 prod;
	u32 i, s_idx, d_idx;

	spin_lock(&cb_pool->lock);
	prod = cb_pool->prod;
	if (unlikely(!is_doe_cb_pool_enough(cb_pool, count))) {
		doe_bug(-E_DOE_BUG, "%s put %u cb to cb-pool, NO enough space, only left %u",
			doe_if->name, count, doe_cb_pool_left(&doe_if->cb_pool));
		atomic64_inc(&doe_if->cb_pool.st.bug_cb_pool_put_with_not_enough);
		goto unlock;
	}

	for (i = 0; i < count; i++) {
		d_idx = (prod + i) & YK3_DOE_CB_POOL_MASK;
		s_idx = (start + i) & YK3_DOE_FIFO_MASK;

		cb_pool->pool[d_idx] = fifo->pool[s_idx];
	}
	cb_pool->prod += count;
unlock:
	spin_unlock(&cb_pool->lock);
}

static int
doe_tag_slot_init(struct yk3_doe_cmd_manager *manager)
{
	struct yk3_doe_tag_slot *slot;
	struct yk3_doe_cmd *cmd;
	int tag, total = YK3_DOE_CMD_TAG_COUNT * sizeof(*slot);

	// kzalloc maybe failed, use kvzalloc
	slot = kvzalloc(total, GFP_KERNEL);
	if (!slot)
		return doe_error(-E_DOE_NOMEM, "tag slot alloc failed");
	manager->tag_slot = slot;

	if (manager->pool_enable) {
		for (tag = 0; tag < YK3_DOE_CMD_TAG_COUNT; tag++) {
			cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
			if (!cmd)
				return doe_error(-E_DOE_NOMEM, "tag slot cmd alloc failed");

			cmd->tag = tag;
			slot[tag].pool = cmd; // pre alloc use pool
		}
	}

	return 0;
}

static void
doe_tag_slot_fini(struct yk3_doe_cmd_manager *manager)
{
	struct yk3_doe_tag_slot *slot = manager->tag_slot;
	int tag;

	if (!slot)
		return;

	if (manager->pool_enable) {
		for (tag = 0; tag < YK3_DOE_CMD_TAG_COUNT; tag++) {
			// {}
			kfree(slot[tag].pool);
		}
	}

	kvfree(slot); // alloc by kvzalloc
}

int doe_cmd_manager_init(struct yk3_doe_if *doe_if, bool pool_enable)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	int ret;

	spin_lock_init(&manager->lock);
	manager->pool_enable = pool_enable;

	ret = doe_tag_pool_init(&manager->tag_pool);
	if (ret < 0)
		return ret;

	ret = doe_tag_slot_init(manager);
	if (ret < 0)
		return ret;

	return 0;
}

void doe_cmd_manager_fini(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;

	spin_lock(&manager->lock);
	doe_tag_slot_fini(manager);
	doe_tag_pool_fini(&manager->tag_pool);
	spin_unlock(&manager->lock);
}

// call after unbind
static void
doe_cmd_measure_stop(struct yk3_doe_cmd *cmd)
{
#if YK3_DOE_MEASURE
	struct yk3_doe_if *doe_if = cmd->doe_if;
	u64 ns_total = ktime_get_ns() - cmd->ns_start;
	int idx = cmd->seq & (YK3_DOE_MEASURE_N - 1);

	doe_if->measure.avg[idx].times++;
	doe_if->measure.avg[idx].total += ns_total;

	if (unlikely(!ns_total))
		return;

	if (ns_total < doe_if->measure.level[DOE_MEASURE_LEVEL_MIN].value)
		doe_if->measure.level[DOE_MEASURE_LEVEL_MIN].value = ns_total;

	if (ns_total > doe_if->measure.level[DOE_MEASURE_LEVEL_MAX].value)
		doe_if->measure.level[DOE_MEASURE_LEVEL_MAX].value = ns_total;

#define __ge(us) (ns_total >= ((us) * NSEC_PER_USEC))
#define __add(op, us) do { \
	doe_if->measure.level[DOE_MEASURE_LEVEL_##op##us].times++; \
	doe_if->measure.level[DOE_MEASURE_LEVEL_##op##us].total += ns_total; \
} while (0)
#define __add_ge(us) __add(GE, us)
#define __add_lt(us) __add(LT, us)

	if (__ge(2000))
		__add_ge(2000);
	else if (__ge(1000))
		__add_ge(1000);
	else if (__ge(900))
		__add_ge(900);
	else if (__ge(800))
		__add_ge(800);
	else if (__ge(700))
		__add_ge(700);
	else if (__ge(600))
		__add_ge(600);
	else if (__ge(500))
		__add_ge(500);
	else if (__ge(400))
		__add_ge(400);
	else if (__ge(300))
		__add_ge(300);
	else if (__ge(200))
		__add_ge(200);
	else if (__ge(100))
		__add_ge(100);
	else if (__ge(90))
		__add_ge(90);
	else if (__ge(80))
		__add_ge(80);
	else if (__ge(70))
		__add_ge(70);
	else if (__ge(60))
		__add_ge(60);
	else if (__ge(50))
		__add_ge(50);
	else if (__ge(40))
		__add_ge(40);
	else if (__ge(30))
		__add_ge(30);
	else if (__ge(20))
		__add_ge(20);
	else if (__ge(10))
		__add_ge(10);
	else if (__ge(4))
		__add_ge(4);
	else
		__add_lt(4);

#undef __add_lt
#undef __add_ge
#undef __add
#undef __ge
#endif
}

struct yk3_doe_cmd *
doe_cmd_get(struct yk3_doe_if *doe_if, int init_flags)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_cmd *cmd = NULL;
	u64 ns_start = ktime_get_ns();
	unsigned long flags;
	bool success = false;
	u16 tag = 0;

	if (unlikely(!manager->pool_enable)) {
		atomic64_inc(&manager->st.bug_get_with_pool_disable);
		doe_tag_pool_dump(&manager->tag_pool);
		doe_bug(-E_DOE_BUG, "cmd manager get with pool disabled");
		return NULL;
	}

	do {
		if (unlikely(is_doe_hw_failure(doe_priv))) {
			doe_error(-E_DOE_HWCLOSED, "hardware closed @ cmd get");
			return NULL;
		}

		spin_lock_irqsave(&manager->lock, flags);
		if (unlikely(is_doe_tag_pool_empty(&manager->tag_pool))) {
			// {}
			atomic64_inc(&manager->st.get_with_pool_empty);
		} else {
			tag = doe_tag_pool_get(&manager->tag_pool);
			cmd = manager->tag_slot[tag].pool; // get cmd from pool
			success = true;
		}
		spin_unlock_irqrestore(&manager->lock, flags);

		if (unlikely(!success && is_doe_timeout(ns_start, DOE_TAG_TIMEOUT))) {
			usleep_range(1, 2); // avoid cpu stuck
			doe_error(-E_DOE_TIMEOUT, "timeout @ %s cmd get", doe_if->name);
			return NULL;
		}
	} while (!success);

	// ----------------------------------
	// out of lock
	// ----------------------------------
	yk3_memzero(cmd, offsetof(struct yk3_doe_cmd, body));
	cmd->flags = init_flags;
	doe_cmd_bind(manager, cmd, tag);

	return cmd;
}

int doe_cmd_put(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool in_irq)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	unsigned long flags;

	if (unlikely(!manager->pool_enable)) {
		atomic64_inc(&manager->st.bug_put_with_pool_disable);
		doe_tag_pool_dump(&manager->tag_pool);
		return doe_bug(-E_DOE_BUG, "cmd manager put with pool disabled");
	} else if (unlikely(is_doe_tag_pool_full(&manager->tag_pool))) {
		atomic64_inc(&manager->st.bug_put_with_pool_full);
		doe_tag_pool_dump(&manager->tag_pool);
		return doe_bug(-E_DOE_BUG, "cmd manager put with pool full");
	}

	spin_lock_by(&manager->lock, in_irq, flags);
	// unbind the cmd with lock, both timeout and irq handle it
	doe_cmd_unbind(manager, cmd, false);
	doe_tag_pool_put(&manager->tag_pool, cmd->tag);
	spin_unlock_by(&manager->lock, in_irq, flags);
	doe_cmd_measure_stop(cmd);

	if (doe_if->is_read)
		doe_debug(DOE_DEBUG_CTR, "put cmd: " DOE_CMD_FMT,
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	else
		doe_debug(DOE_DEBUG_CTW, "put cmd: " DOE_CMD_FMT,
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);

	return 0;
}

int doe_cmd_register(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	unsigned long flags;
	u64 ns_start = ktime_get_ns();
	bool success = false;
	u16 tag = 0;

	do {
		if (unlikely(is_doe_hw_failure(doe_priv)))
			return doe_error(-E_DOE_HWCLOSED, "hardware closed @ cmd register");

		spin_lock_irqsave(&manager->lock, flags);
		if (unlikely(is_doe_tag_pool_empty(&manager->tag_pool))) {
			// {}
			atomic64_inc(&manager->st.register_with_pool_empty);
		} else {
			tag = doe_tag_pool_get(&manager->tag_pool);
			success = true;
		}
		spin_unlock_irqrestore(&manager->lock, flags);

		if (unlikely(!success && is_doe_timeout(ns_start, DOE_TAG_TIMEOUT))) {
			usleep_range(1, 2); // avoid cpu stuck
			return doe_error(-E_DOE_TIMEOUT, "timeout @ %s cmd register", doe_if->name);
		}
	} while (!success);

	// NOT zero cmd(cmd is stack var, have declared as zero)
	doe_cmd_bind(manager, cmd, tag);

	return 0;
}

int doe_cmd_unregister(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool is_timeout)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	unsigned long flags;

	if (unlikely(is_doe_tag_pool_full(&manager->tag_pool))) {
		atomic64_inc(&manager->st.bug_unregister_with_pool_full);
		doe_tag_pool_dump(&manager->tag_pool);
		return doe_bug(-E_DOE_BUG, "tag pool full @ cmd unregister");
	}

	spin_lock_irqsave(&manager->lock, flags);
	// unbind the cmd with lock, both timeout and irq handle it
	doe_cmd_unbind(manager, cmd, is_timeout);
	//     timeout: keep the tag, maybe event late
	// NOT timeout: put  the tag
	if (likely(!is_timeout))
		doe_tag_pool_put(&manager->tag_pool, cmd->tag);
	spin_unlock_irqrestore(&manager->lock, flags);
	doe_cmd_measure_stop(cmd);

	return 0;
}

struct yk3_doe_cmd *
doe_cmd_alloc(struct yk3_doe_if *doe_if, int opcode, int tbl_id, u32 index, int flags)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	struct yk3_doe_cmd *cmd = NULL;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		atomic64_inc(&manager->st.err_alloc_nil);
		doe_error(-E_DOE_NOMEM,
			  "cmd alloc no memory, flags[0x%x] opcode[0x%x] tbl_id[0x%x]",
			  flags, opcode, tbl_id);
		return NULL;
	}

	cmd->opcode	= opcode;
	cmd->tbl_id	= tbl_id;
	cmd->index	= index;
	cmd->flags	= flags;

	ret = doe_cmd_register(doe_if, cmd);
	if (ret < 0)
		goto error;

	return cmd;
error:
	kfree(cmd);
	return NULL;
}

int doe_cmd_free(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, bool is_timeout)
{
	int ret;

	ret = doe_cmd_unregister(doe_if, cmd, is_timeout);
	kfree(cmd);

	return ret;
}

#if YK3_DOE_CMD_ZOMBIE

void doe_cmd_zombie_check(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	struct yk3_doe_cmd *cmd, *tmp;
	u64 ns_now = ktime_get_ns();
	u64 ns_zombie = doe_if->doe_priv->cfg.sec_cmd_zombie * NSEC_PER_SEC;

	spin_lock(&manager->lock);

	spin_unlock(&manager->lock);

	return found;
}
#endif

int doe_evq_init(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_evq *evq = &doe_if->evq;
	struct yk3_doe_if_cfg *cfg = doe_if->cfg;
	u32 min_depth = cfg->evq_depth_min;
	u32 old_depth = cfg->evq_depth;
	u32 new_depth = old_depth;
	u32 block_size = cfg->evq_block_size;
	u32 block_order = cfg->evq_block_order;
	u32 batch_count = cfg->evq_batch_count;
	u32 total_size = new_depth * block_size;

	do {
		total_size = new_depth * block_size;
		evq->base = doe_dmam_zalloc_coherent(doe_priv, total_size, &evq->dma_base);
		if (!evq->base) {
			new_depth >>= 1; // try half
			if (new_depth < min_depth) {
				// {}
				return doe_error(-E_DOE_NOMEM,
						 "%s min evq depth[%u] failed",
						 doe_if->name, min_depth);
			}
		}
	} while (!evq->base);

	if (old_depth != new_depth) {
		doe_info("%s evq dma size downgrade %uK ==> %uK", doe_if->name,
			 (old_depth * block_size) / 1024,
			 (new_depth * block_size) / 1024);

		cfg->evq_depth = new_depth;
	}

	evq->desc = doe_dmam_zalloc_coherent(doe_priv, sizeof(*evq->desc), &evq->dma_desc);
	if (!evq->desc)
		return doe_error(-E_DOE_NOMEM, "evq dma alloc failed");
	evq->desc->dma_address = cpu_to_le64(evq->dma_base);
	doe_debug(DOE_DEBUG_INIT, "%s evq: 0x%llx(dma 0x%llx) desc: 0x%llx(dma 0x%llx)",
		  doe_if->name, YK3_PTR(evq->base), evq->dma_base,
		  YK3_PTR(evq->desc), evq->dma_desc);

	evq->depth = new_depth;
	evq->mask = new_depth - 1;
	evq->block_size = block_size;
	evq->block_order = block_order;
	evq->batch_count = batch_count;

	return 0;
}

#if YK3_DOE_IRQ == YK3_DOE_IRQ_THREAD
irqreturn_t doe_event_thread_r(int irq, void *dev_id)
{
	struct yk3_doe_if *doe_if = dev_id;

	doe_event_handler_r(doe_if);

	return IRQ_HANDLED;
}

irqreturn_t doe_event_thread_w(int irq, void *dev_id)
{
	struct yk3_doe_if *doe_if = dev_id;

	doe_event_handler_w(doe_if);

	return IRQ_HANDLED;
}
#endif

#if YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
static void
doe_event_tasklet_r(unsigned long data)
{
	struct yk3_doe_if *doe_if = (struct yk3_doe_if *)data;

	if (doe_event_handler_r(doe_if))
		tasklet_schedule(&doe_if->tasklet);
}

static void
doe_event_tasklet_w(unsigned long data)
{
	struct yk3_doe_if *doe_if = (struct yk3_doe_if *)data;

	if (doe_event_handler_w(doe_if))
		tasklet_schedule(&doe_if->tasklet);
}
#endif

static int
doe_pre_init(struct yk3_doe_priv *doe_priv)
{
	u32 reg, val;

	val = doe_rd32(doe_priv, YK3_DOE_REG_VERSION);
	if (val != doe_priv->chip_spec->version) {
		doe_error(-EDOM, "hardware version:%x not match software version:%x.",
			  val, doe_priv->chip_spec->version);
		// just error
		// return -EDOM;
	}

	// k3 T-card/R-card, emp init YK3_DOE_REG_CLK_GATE_EN
	// k3 hmc or k3max-preview, reset YK3_DOE_REG_CLK_GATE_EN to 0x7f
	if (doe_priv->hmc.enable || K3MAX_PREVIEW) {
		reg = doe_rd32(doe_priv, YK3_DOE_REG_CLK_GATE_EN);
		doe_wr32(doe_priv, YK3_DOE_REG_CLK_GATE_EN, 0x7f);
		doe_debug(DOE_DEBUG_INIT, "reset gate[0x%x] 0x%x ==> 0x7f",
			  YK3_DOE_REG_CLK_GATE_EN, reg);
	}
	doe_gate_update(doe_priv);
	doe_debug(DOE_DEBUG_INIT, "read gate[0x%x] = 0x%x",
		  YK3_DOE_REG_CLK_GATE_EN, doe_priv->clk_gate_en);

	val = doe_priv->hmc.enable ? YK3_DOE_WORK_MODE_HMC : YK3_DOE_WORK_MODE_NODDR;
	reg = doe_rd32(doe_priv, YK3_DOE_REG_WORK_MODE);
	doe_wr32(doe_priv, YK3_DOE_REG_WORK_MODE, val);
	doe_debug(DOE_DEBUG_INIT, "reset work_mode[0x%x] 0x%x ==> 0x%x",
		  YK3_DOE_REG_WORK_MODE, reg, val);

	reg = doe_rd32(doe_priv, YK3_DOE_REG_DMA_MODE);
	if (reg != YK3_DMA_MODE_FIFO) {
		doe_wr32(doe_priv, YK3_DOE_REG_DMA_MODE, YK3_DMA_MODE_FIFO);
		doe_debug(DOE_DEBUG_INIT, "reset dma_mode[0x%x] 0x%x ==> 0x%x",
			  YK3_DOE_REG_DMA_MODE, reg, YK3_DMA_MODE_FIFO);
	}

	reg = doe_rd32(doe_priv, YK3_DOE_REG_AIE_DLEN_LIMIT);
	if (reg) {
		doe_wr32(doe_priv, YK3_DOE_REG_AIE_DLEN_LIMIT, 0);
		doe_debug(DOE_DEBUG_INIT, "reset aie_dlen_limit[0x%x] 0x%x ==> 0",
			  YK3_DOE_REG_AIE_DLEN_LIMIT, reg);
	}

	reg = doe_rd32(doe_priv, YK3_DOE_REG_HIE_DLEN_LIMIT);
	if (reg) {
		doe_wr32(doe_priv, YK3_DOE_REG_HIE_DLEN_LIMIT, 0);
		doe_debug(DOE_DEBUG_INIT, "reset hie_dlen_limit[0x%x] 0x%x ==> 0",
			  YK3_DOE_REG_HIE_DLEN_LIMIT, reg);
	}

	if (is_doe_k3max(doe_priv)) {
		// force counter_zip 0
		val = doe_rd32(doe_priv, YK3_DOE_REG_COUNTER_ZIP);
		if (val) {
			doe_wr32(doe_priv, YK3_DOE_REG_COUNTER_ZIP, 0);
			doe_debug(DOE_DEBUG_INIT, "reset counter_zip[0x%x] 0x%x ==> 0",
				  YK3_DOE_REG_COUNTER_ZIP, val);
		}

		// force cache_isolation 0
		val = !!doe_rd32(doe_priv, YK3_DOE_REG_CACHE_ISO_HIE);
		if (val) {
			doe_wr32(doe_priv, YK3_DOE_REG_CACHE_ISO_HIE, 0);
			doe_debug(DOE_DEBUG_INIT, "reset cache_isolation[0x%x] 0x%x ==> 0",
				  YK3_DOE_REG_CACHE_ISO_HIE, val);
		}
	}

	doe_debug(DOE_DEBUG_INIT, "cmd size: %u",  (u32)sizeof(struct yk3_doe_cmd));
	doe_debug(DOE_DEBUG_INIT, "intf size: %u", (u32)sizeof(struct yk3_doe_if));
	doe_debug(DOE_DEBUG_INIT, "priv size: %u", (u32)sizeof(struct yk3_doe_priv));

	return 0;
}

//申请并初始化doe_interface
static int
doe_intf_init(struct yk3_doe_priv *doe_priv, struct yk3_doe_if *doe_if)
{
	bool is_read = (doe_if == doe_if_get_r(doe_priv));
	bool pool_enable = is_read ? false : doe_priv->hmc.enable;
	struct yk3_doe_if_cfg *cfg = &doe_priv->cfg.if_cfg[is_read];
	int ret;

	doe_if->name = is_read ? "r-if" : "w-if";
	doe_if->doe_priv = doe_priv;
	doe_if->is_read = is_read;
	doe_if->cfg = cfg;

	doe_debug(DOE_DEBUG_INIT, "%s cfg cmd buffer size     : %u",
		  doe_if->name, cfg->cmd_buffer_size);
	doe_debug(DOE_DEBUG_INIT, "%s cfg evq depth           : %u",
		  doe_if->name, cfg->evq_depth);
	doe_debug(DOE_DEBUG_INIT, "%s cfg evq block size      : %u",
		  doe_if->name, cfg->evq_block_size);
	doe_debug(DOE_DEBUG_INIT, "%s cfg evq block order     : %u",
		  doe_if->name, cfg->evq_block_order);
	doe_debug(DOE_DEBUG_INIT, "%s cfg evq batch count     : %u",
		  doe_if->name, cfg->evq_batch_count);
	doe_debug(DOE_DEBUG_INIT, "%s cfg irq coalesce timeout: %u",
		  doe_if->name, cfg->irq_coalesce.timeout);
	doe_debug(DOE_DEBUG_INIT, "%s cfg irq coalesce count  : %u",
		  doe_if->name, cfg->irq_coalesce.count);

	doe_doorbell_init(&doe_if->db_reg, is_read);
	doe_vfc_init(&doe_if->vfc, cfg->evq_depth);

	ret = doe_cb_pool_init(doe_if);
	if (ret < 0)
		return ret;

	// MUST after cb pool
	ret = doe_fifo_init(doe_if);
	if (ret < 0)
		return ret;

	ret = doe_evq_init(doe_if);
	if (ret < 0)
		return ret;

	ret = doe_cmd_manager_init(doe_if, pool_enable);
	if (ret < 0)
		return ret;

	doe_measure_init(doe_if);

#if YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
	tasklet_init(&doe_if->tasklet,
		     is_read ? doe_event_tasklet_r : doe_event_tasklet_w,
		     (unsigned long)doe_if);
#endif
	doe_debug(DOE_DEBUG_INIT, "Init %s", doe_if->name);

	return 0;
}

//释放doe_interface
static void
doe_intf_fini(struct yk3_doe_if *doe_if)
{
	if (!doe_if)
		return;

#if YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
	tasklet_kill(&doe_if->tasklet);
#endif
	doe_cmd_manager_fini(doe_if);
	// no evq exit
	doe_fifo_fini(doe_if);
	doe_cb_pool_fini(doe_if);

	doe_debug(DOE_DEBUG_INIT, "Exit %s", doe_if->name);
}

static int
doe_hw_resources_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_table_param *param;
	struct yk3_doe_spec_table *spec;
	int ddr_channel = doe_ddr_channel(doe_priv);
	int ret, i;

	for (i = YK3_DOE_ZERO_CLEAR_TABLE; i <= YK3_DOE_MIU_PARAM_TABLE; i++) {
		param = &doe_priv->param[i];
		param->depth = doe_spec_table_depth(i);
		param->dov_len = doe_spec_table_size(doe_priv, i);
	}

	/* init param of special table in ddr */
	param = &doe_priv->param[YK3_DOE_INDEX_VIEW];
	param->depth = doe_priv->hmc.size / YK3_DOE_INDEX_BLOCK_SIZE - 1;
	param->dov_len = YK3_DOE_INDEX_BLOCK_SIZE;

	// hash view spec
	spec = &doe_priv->spec[YK3_DOE_INDEX_VIEW];
	spec->miu_param.item_len = cpu_to_le16(YK3_DOE_INDEX_BLOCK_SIZE);
	spec->miu_param.item_size = doe_get_order(YK3_DOE_INDEX_BLOCK_SIZE);
	spec->miu_param.ddr_channel = ddr_channel;
	spec->miu_param.endian = YK3_DOE_ENDIAN_DEFT;
	spec->miu_param.ddr_mode = YK3_DOE_DDR_MODE;
	doe_debug_buffer(DOE_DEBUG_INIT, &spec->miu_param, sizeof(spec->miu_param),
			 "init index view miu");

	spec->aie_param.item_size = spec->miu_param.item_size;
	spec->aie_param.item_len = spec->miu_param.item_len;
	spec->aie_param.depth = cpu_to_le32(param->depth);
	spec->aie_param.tbl_type = DOE_TABLE_NORMAL_ARRAY;
	spec->aie_param.valid = 1;
	spec->aie_param.ddr_mode = YK3_DOE_DDR_MODE;
	spec->aie_param.endian = YK3_DOE_ENDIAN_DEFT;
	spec->aie_param.ddr_channel = ddr_channel;
	doe_debug_buffer(DOE_DEBUG_INIT, &spec->aie_param, sizeof(spec->aie_param),
			 "init index view aie");

	spec->cache_param.ddr_mode = YK3_DOE_DDR_MODE;
	spec->cache_param.endian = YK3_DOE_ENDIAN_DEFT;
	spec->cache_param.ddr_channel = ddr_channel;
	spec->cache_param.tbl_type = DOE_TABLE_NORMAL_ARRAY;
	spec->cache_param.valid = 1;
	spec->cache_param.value_len = spec->miu_param.item_len;
	spec->cache_param.key_len = 0;
	spec->cache_param.depth = spec->aie_param.depth;
	spec->cache_param.item_size = spec->miu_param.item_size;
	doe_debug_buffer(DOE_DEBUG_INIT, &spec->cache_param, sizeof(spec->cache_param),
			 "init index view cache");

	// TODO: 32B align ?
	doe_priv->ram = doe_mm_create(doe_priv, YK3_DOE_RAM_SIZE, YK3_DOE_RAM_MASK, false, "ram");
	// pipeline force use PTR_ERR_OR_ZERO
	ret = PTR_ERR_OR_ZERO(doe_priv->ram);
	if (ret)
		return ret;

	return 0;
}

static void
doe_hw_resources_fini(struct yk3_doe_priv *doe_priv)
{
	yk3_do_nothing();
}

static int
doe_evq_reset(struct yk3_doe_priv *doe_priv)
{
	u32 val;
	int ret;

	doe_wr32(doe_priv, YK3_DOE_REG_EVQ_RESET, 1);

	// wait reset clear
	ret = readl_poll_timeout_atomic(doe_priv->bar_base + YK3_DOE_REG_EVQ_RESET,
					val, val == 0, 100, 3000);
	if (ret < 0)
		return doe_error(-E_DOE_TIMEOUT, "evq reset timeout");

	return 0;
}

#define DOE_IRQ_BASE_VAL(pf_id) ((pf_id) << 21)
//设置DMA，使能中断
static int
doe_reg_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_pdev_priv *pdev_priv = doe_priv->pdev_priv;
	struct yk3_doe_if *doe_if;
	struct yk3_doe_evq *evq;
	u32 pf_id, irq_vector;
	int ret;

	// pf id
	pf_id = pdev_priv->pf_id;
	doe_wr32(doe_priv, YK3_DOE_REG_PF_NUM, (pf_id << 9));

	// rd event & irq
	doe_if = doe_if_get_r(doe_priv);
	evq = &doe_if->evq;
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_SIZE, evq->block_size);
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_TOTAL_SIZE, evq->depth * evq->block_size);
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_BASE_L, evq->dma_base);
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_BASE_H, (evq->dma_base >> 32));
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_PTR_L, evq->dma_desc);
	doe_wr32(doe_priv, YK3_DOE_REG_RD_EV_PTR_H, (evq->dma_desc >> 32));
	// doe_wr32(doe_priv, YK3_DOE_REG_RD_IRQ_COALESCE, doe_if->cfg->irq_coalesce.raw);
	irq_vector = DOE_IRQ_BASE_VAL(pf_id) + doe_if->irq_vector;
	doe_wr32(doe_priv, YK3_DOE_REG_RD_INT_VECTOR, irq_vector);
	doe_debug(DOE_DEBUG_INIT, "rd pf[%u] irq vector[%u]", pf_id, doe_if->irq_vector);

	// wr event & irq
	doe_if = doe_if_get_w(doe_priv);
	evq = &doe_if->evq;
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_SIZE, evq->block_size);
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_TOTAL_SIZE, evq->depth * evq->block_size);
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_BASE_L, evq->dma_base);
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_BASE_H, (evq->dma_base >> 32));
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_PTR_L, evq->dma_desc);
	doe_wr32(doe_priv, YK3_DOE_REG_WR_EV_PTR_H, (evq->dma_desc >> 32));
	// doe_wr32(doe_priv, YK3_DOE_REG_WR_IRQ_COALESCE, doe_if->cfg->irq_coalesce.raw);
	irq_vector = DOE_IRQ_BASE_VAL(pf_id) + doe_if->irq_vector;
	doe_wr32(doe_priv, YK3_DOE_REG_WR_INT_VECTOR, irq_vector);
	doe_debug(DOE_DEBUG_INIT, "wr pf[%u] irq vector[%u]", pf_id, doe_if->irq_vector);

	// DOE reset evq must be asserted after event initial
	ret = doe_evq_reset(doe_priv);
	if (ret < 0)
		return ret;

	return 0;
}

// TODO: check reg int_rd_lost_cnt/int_wr_lost_cnt
irqreturn_t doe_irq_handler_r(int action, void *data)
{
	struct yk3_doe_if *doe_if = (struct yk3_doe_if *)data;

	atomic64_inc(&doe_if->st.irq_trigger);

#if YK3_DEBUG
	if (unlikely(is_doe_inject_irq_skip(doe_if->is_read)))
		return IRQ_HANDLED;
#endif

#if YK3_DOE_IRQ == YK3_DOE_IRQ_THREAD
	return IRQ_WAKE_THREAD;
#elif YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
	tasklet_schedule(&doe_if->tasklet);
#else
	doe_event_handler_r(doe_if);
#endif

	return IRQ_HANDLED;
}

irqreturn_t doe_irq_handler_w(int action, void *data)
{
	struct yk3_doe_if *doe_if = (struct yk3_doe_if *)data;

	atomic64_inc(&doe_if->st.irq_trigger);

#if YK3_DOE_IRQ == YK3_DOE_IRQ_THREAD
	return IRQ_WAKE_THREAD;
#elif YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET
	tasklet_schedule(&doe_if->tasklet);
#else
	doe_event_handler_w(doe_if);
#endif

	return IRQ_HANDLED;
}

static int
doe_register_irqs(struct yk3_doe_priv *doe_priv)
{
#if K2U
	return k2u_doe_register_irqs(doe_priv);
#else
	struct yk3_irq_param *param;
	struct yk3_doe_if *doe_if;
	int ret, i;

	/* write irq register */
	for (i = 0; i < 2; ++i) {
		doe_if	= &doe_priv->doe_if[i];
		param	= &doe_if->irq_param;

		param->flags	= YK3_F_IRQ_AFFINITY;
		param->data	= doe_if;
		param->vector	= (i == YK3_DOE_RD) ? YK3_DOE_IRQ_EVQ_R : YK3_DOE_IRQ_EVQ_W;
		param->handler	= (i == YK3_DOE_RD) ? doe_irq_handler_r : doe_irq_handler_w;
		strscpy(param->name,
			(i == YK3_DOE_RD) ? "yk3_doe_irq_read" : "yk3_doe_irq_write",
			sizeof(param->name));
#if YK3_DOE_ONLY
		doe_if->irq_number = pci_irq_vector(doe_priv->pdev, param->vector);
		doe_info("irq vector[%d] ==> number[%d]", param->vector, doe_if->irq_number);

		ret = request_irq(doe_if->irq_number, param->handler, 0, param->name, doe_if);
		if (ret < 0)
			return doe_error(ret, "doe-only irq vector[%u] request failed",
					 param->vector);
#else
		ret = yk3_irq_request(doe_priv->pdev_priv, param);
		if (ret < 0)
			return doe_error(ret, "irq vector[%u] request failed", param->vector);
#endif
		// irq_vector >= 0 is irq ok
		doe_if->irq_vector = param->vector;
	}

	return 0;
#endif
}

static int
doe_unregister_irqs(struct yk3_doe_priv *doe_priv)
{
#if K2U
	return k2u_doe_unregister_irqs(doe_priv);
#else
	struct yk3_irq_param *param;
	struct yk3_doe_if *doe_if;
	int i;

	for (i = 0; i < 2; i++) {
		doe_if	= &doe_priv->doe_if[i];
		param	= &doe_if->irq_param;

		if (is_doe_irq_ok(doe_if)) {
#if YK3_DOE_ONLY
			synchronize_irq(doe_if->irq_number);
			free_irq(doe_if->irq_number, doe_if);

			doe_info("%s irq %d/%d trigger : 0x%llx",
				 doe_if->name, doe_if->irq_vector, doe_if->irq_number,
				 (u64)atomic64_read(&doe_if->st.irq_trigger));
#else
			yk3_irq_free(doe_priv->pdev_priv, param);
#endif
			doe_if_init(doe_if);
		}
	}

	return 0;
#endif
}

static void
doe_priv_init(struct yk3_pdev_priv *pdev_priv, struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_if *doe_if;

	pdev_priv->doe_priv = doe_priv;

	doe_tbl_lock_init(doe_priv);
	doe_priv->dev = &pdev_priv->pdev->dev;
	doe_priv->pdev = pdev_priv->pdev;
	doe_priv->card = pdev_priv->card;
	doe_priv->pdev_priv = pdev_priv;

	// must before doe_rd32/doe_wr32
	doe_priv->bar_base = YK3_DOE_BAR_BASE(pdev_priv);
	doe_debug(DOE_DEBUG_INIT, "doe base: 0x%x", YK3_DOE_REG_BASE);
	// must before doe chip used
#if K3MAX_PREVIEW || K2U
	doe_priv->chip = YK3_K3MAX;
	doe_priv->arch = YK3_FPGA;
#else
	doe_priv->chip = doe_priv->card->chip;
	doe_priv->arch = YK3_ASIC;
#endif
	doe_priv->chip_spec = &yk3_chip_specs[doe_priv->chip].doe;
	doe_info("chip[%s] arch[%s] ddr-channel[%d]",
		 yk3_chip_name(doe_priv->chip),
		 yk3_arch_name(doe_priv->arch),
		 doe_ddr_channel(doe_priv));

	// cfg init
	yk3_objcpy(&doe_priv->cfg, &doe_cfg);
	// xie info init
	doe_xie_init(doe_priv);

	// pre-init w-if
	doe_if = doe_if_get_w(doe_priv);
	doe_if->name = "w-if";
	doe_if_init(doe_if);

	// pre-init r-if
	doe_if = doe_if_get_r(doe_priv);
	doe_if->name = "r-if";
	doe_if_init(doe_if);
}

#if YK3_SKIP_DOE
static int
doe_sim_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_doe_priv *doe_priv;
	int ret;

	doe_priv = &__doe_skip;
	doe_priv_init(pdev_priv, doe_priv);
	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	doe_pre_init(doe_priv);
	doe_xie_init(doe_priv);

	doe_info("Sim Install successfully, version %08x", doe_rd32(doe_priv, YK3_DOE_REG_VERSION));
	return 0;
}
#endif

enum {
					// <0: error
	DOE_INIT_SKIP		= 0,	//  0: ok, but skip init
	DOE_INIT_CONTINUE	= 1,	// >0: ok, and continue init
};

static int
doe_init_check(struct yk3_pdev_priv *pdev_priv)
{
	if (is_doe_pdev_pf(pdev_priv)) {
		struct yk3_card *card = pdev_priv->card;

		if (!card) {
			// {}
			return doe_error(-E_DOE_INVALID, "%s card is nil.", pdev_priv->name);
		} else if (!card->mgr_pdev_priv) {
			// VM without mgr, NOT support doe, skip init
			doe_debug(DOE_DEBUG_INIT, "VM skip doe");
			return DOE_INIT_SKIP;
		} else if (!card->mgr_pdev_priv->doe_priv) {
			// {}
			return doe_error(-E_DOE_INVALID, "%s doe_priv is nil.", pdev_priv->name);
		}

		// pf: just save mgr's doe_priv, skip init
		pdev_priv->doe_priv = card->mgr_pdev_priv->doe_priv;

		return DOE_INIT_SKIP;
	}

	//  vf: NOT support doe, skip init
	// mgr: continue init
	return is_doe_pdev_mgr(pdev_priv) ? DOE_INIT_CONTINUE : DOE_INIT_SKIP;
}

void yk3_doe_exit(struct yk3_pdev_priv *pdev_priv)
{
#if !YK3_SKIP_DOE
	struct yk3_doe_priv *doe_priv;

	if (!is_doe_pdev_mgr(pdev_priv))
		return;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return;

	// default(without yk3 driver), np & host protect enable
	doe_wr32(doe_priv, YK3_DOE_REG_PROTECT_CFG, YK3_DOE_PROTECT_ALL);

	doe_hmc_fini(doe_priv);
	doe_unregister_irqs(doe_priv);
	doe_hw_resources_fini(doe_priv);
	doe_intf_fini(doe_if_get_r(doe_priv));
	doe_intf_fini(doe_if_get_w(doe_priv));
	doe_debugfs_fini(doe_priv);
	kfree(doe_priv); pdev_priv->doe_priv = NULL;

	doe_debug(DOE_DEBUG_INIT, "Exit");
#endif
}

int yk3_doe_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_doe_priv *doe_priv;
	int ret;

	doe_cfg_repair(&doe_cfg);
#if YK3_SKIP_DOE
	return doe_sim_init(pdev_priv);
#endif
	ret = doe_init_check(pdev_priv);
	if (ret != DOE_INIT_CONTINUE)
		return ret; // error or skip

	// is mgr, do init
	doe_priv = kzalloc(sizeof(*doe_priv), GFP_KERNEL);
	if (!doe_priv)
		return doe_error(-E_DOE_NOMEM, "doe priv alloc failed");

	doe_priv_init(pdev_priv, doe_priv);
	ret = doe_hw_check(doe_priv);
	if (ret < 0) {
		doe_error(ret, "hw check failed");
		goto error;
	}

	ret = doe_hmc_init(doe_priv);
	if (ret) {
		doe_error(ret, "hmc init failed");
		goto error;
	}

	ret = doe_pre_init(doe_priv);
	if (ret) {
		doe_error(ret, "pre init failed");
		goto error;
	}

	ret = doe_debugfs_init(doe_priv);
	if (ret) {
		doe_error(ret, "debug init failed");
		goto error;
	}

	/* Interface buffer init, include cmd buffer and event queue buffer */
	ret = doe_intf_init(doe_priv, doe_if_get_w(doe_priv));
	if (ret) {
		doe_error(ret, "w-interface init failed");
		goto error;
	}

	ret = doe_intf_init(doe_priv, doe_if_get_r(doe_priv));
	if (ret) {
		doe_error(ret, "r-interface init failed");
		goto error;
	}

	/* DOE hardware resources init */
	ret = doe_hw_resources_init(doe_priv);
	if (ret) {
		doe_error(ret, "resource init failed");
		goto error;
	}

	/* 注册中断 */
	ret = doe_register_irqs(doe_priv);
	if (ret) {
		doe_error(ret, "irq register failed");
		goto error;
	}

	/*
	 *应在中断回调函数注册完成后再设置使能
	 *设置DMA，event buffer，使能中断
	 */
	ret = doe_reg_init(doe_priv);
	if (ret) {
		doe_error(ret, "register init failed");
		goto error;
	}

	//触发YK3_DOE_SW_HW_INIT命令，对DOE初始化
	ret = doe_hw_init(doe_priv);
	if (ret) {
		doe_error(ret, "hw init failed");
		goto error;
	}

	ret = doe_hw_check(doe_priv);
	if (ret < 0) {
		doe_error(ret, "hw check failed");
		goto error;
	}

	doe_hmc_fast_start(doe_priv);

	doe_info("version %08x", doe_rd32(doe_priv, YK3_DOE_REG_VERSION));
	return 0;
error:
	yk3_doe_exit(pdev_priv);

	return ret;
}
