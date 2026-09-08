// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_edma_priv.h"

static inline bool page_no_user(struct yk3_page *page)
{
	return page_count(page->page) == 1;
}

static inline bool page_no_free(struct yk3_page *page)
{
	return !page->freenum;
}

static inline bool page_all_free(struct yk3_page *page)
{
	return page->freenum == page->maxnum;
}

static inline bool group_no_free(struct yk3_pagegroup *group)
{
	return !group->freenum;
}

static inline bool group_all_free(struct yk3_pagegroup *group)
{
	return group->freefragnum == YK3_N_PP_GROUPSIZE * group->pages[0].maxnum;
}

void yk3_pp_debugfs_show(struct seq_file *seq, struct yk3_pagepool *pp)
{
	int i;
	struct yk3_pagegroup *group;

	seq_printf(seq, "%-16s :\n", "pagepool");
	seq_printf(seq, "\t%-32s : %-4d\n", "fragpower", pp->fragpower);
	seq_printf(seq, "\t%-32s : %-4d\n", "fragsize", (1 << pp->fragpower));
	seq_printf(seq, "\t%-32s : %-4d\n", "pagefragmax", pp->pagefragmax);
	seq_printf(seq, "\t%-32s : %-4d\n", "groupnum", pp->groupnum);
	seq_printf(seq, "\t%-32s : %-4lu\n", "grouppagenum", YK3_N_PP_GROUPSIZE);

	seq_printf(seq, "\t%-32s : %-4lld\n", "recycle", pp->stat.num_recycle);
	seq_printf(seq, "\t%-32s : %-4lld\n", "page_recycle", pp->stat.num_page_recycle);
	seq_printf(seq, "\t%-32s : %-4lld\n", "create_group", pp->stat.num_create_group);
	seq_printf(seq, "\t%-32s : %-4lld\n", "destroy_group", pp->stat.num_destroy_group);

	seq_printf(seq, "\t%-32s : %-4d\n", "total_page", pp->stat.total_page);
	seq_printf(seq, "\t%-32s : %-4d\n", "total_frag", pp->stat.total_frag);
	seq_printf(seq, "\t%-32s : %-4d\n", "used_frag", pp->stat.used_frag);
	seq_printf(seq, "\t%-32s : %-4d\n", "used_page", pp->stat.used_page);
	seq_printf(seq, "\t%-32s : %-4lld\n", "err_page_init", pp->stat.err_page_init);

	for_each_set_bit(i, pp->emptybits, YK3_N_PP_PAGEGROUP_MAX) {
		group = pp->groups[i];

		seq_printf(seq, "\t%-6s %d:\n", "group", i);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "freenum", group->freenum);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "freefragnum", group->freefragnum);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "head", group->rb.head);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "tail", group->rb.tail);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "size", group->rb.size);
		seq_printf(seq, "\t\t%-16s : %-4d\n", "freebit",
			   test_bit(group->gidx, pp->freebits));
	}
}

static int yk3_page_init(struct yk3_pagepool *pp, struct yk3_pagegroup *group, u16 pidx)
{
	int ret;
	struct yk3_page *page = &group->pages[pidx];

	page->page = alloc_pages_node(dev_to_node(pp->dev), GFP_ATOMIC | __GFP_ZERO, 0);
	if (!page->page) {
		ret = -ENOMEM;
		goto err_alloc_page;
	}

	page->dma_addr = dma_map_page(pp->dev, page->page, 0, PAGE_SIZE, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(pp->dev, page->dma_addr))) {
		ret = -EIO;
		goto err_dma_map;
	}

	page->gidx = group->gidx;
	page->pidx = pidx;
	page->power = pp->fragpower;
	page->maxnum = pp->pagefragmax;
	page->freenum = pp->pagefragmax;
	bitmap_set(page->freebits, 0, page->maxnum);

	return 0;

err_dma_map:
	__free_pages(page->page, 0);
err_alloc_page:
	return ret;
}

static void yk3_page_uninit(struct yk3_pagepool *pp, struct yk3_page *page)
{
	if (!page_all_free(page)) {
		pp->stat.used_page--;
		pp->stat.used_frag -= (page->maxnum - page->freenum);
	}
	dma_unmap_page(pp->dev, page->dma_addr, PAGE_SIZE, DMA_FROM_DEVICE);
	if (page_no_user(page))
		__free_pages(page->page, 0);
	else
		put_page(page->page);
}

static bool yk3_page_recycle(struct yk3_pagepool *pp, struct yk3_page *page)
{
	bool recycle = false;

	if (page_no_free(page) && page_no_user(page)) {
		recycle = true;

		bitmap_set(page->freebits, 0, page->maxnum);
		page->freenum = page->maxnum;

		pp->stat.used_page--;
		pp->stat.used_frag -= page->maxnum;

		pp->stat.num_page_recycle++;
	}

	return recycle;
}

static int yk3_page_alloc(struct yk3_pagepool *pp, struct yk3_page *page)
{
	int idx;

	idx = find_first_bit(page->freebits, page->maxnum);
	if (idx >= page->maxnum)
		return -ENOMEM;

	get_page(page->page);
	clear_bit(idx, page->freebits);

	if (page_all_free(page))
		pp->stat.used_page++;
	page->freenum--;
	pp->stat.used_frag++;

	return idx;
}

static void yk3_page_free(struct yk3_pagepool *pp, struct yk3_page *page, u16 idx)
{
	page->freenum++;
	if (page_all_free(page))
		pp->stat.used_page--;
	pp->stat.used_frag--;

	put_page(page->page);
	set_bit(idx, page->freebits);
}

static struct yk3_pagegroup *
yk3_pagegroup_create(struct yk3_pagepool *pp, u16 gidx)
{
	int i, ret;
	struct yk3_pagegroup *group;

	group = kzalloc(sizeof(*group), GFP_ATOMIC);
	if (!group)
		return NULL;

	group->gidx = gidx;
	group->freenum = YK3_N_PP_GROUPSIZE;
	group->freefragnum = YK3_N_PP_GROUPSIZE * pp->pagefragmax;
	yk3_ringb_init(&group->rb, YK3_N_PP_GROUPSIZE);

	for (i = 0; i < YK3_N_PP_GROUPSIZE; i++) {
		ret = yk3_page_init(pp, group, i);
		if (ret) {
			pp->stat.err_page_init++;
			goto err_page_init;
		}
	}

	pp->stat.total_page += YK3_N_PP_GROUPSIZE;
	pp->stat.total_frag += YK3_N_PP_GROUPSIZE * pp->pagefragmax;

	return group;

err_page_init:
	for (i--; i >= 0; i--)
		yk3_page_uninit(pp, &group->pages[i]);

	kfree(group);
	return NULL;
}

static void
yk3_pagegroup_destroy(struct yk3_pagepool *pp, struct yk3_pagegroup *group)
{
	int i;

	pp->stat.total_page -= YK3_N_PP_GROUPSIZE;
	pp->stat.total_frag -= YK3_N_PP_GROUPSIZE * pp->pagefragmax;

	for (i = 0; i < YK3_N_PP_GROUPSIZE; i++)
		yk3_page_uninit(pp, &group->pages[i]);

	kfree(group);
}

static struct yk3_page *
yk3_pagegroup_get_page(struct yk3_pagepool *pp, u16 idx)
{
	struct yk3_pagegroup *group;
	struct yk3_page *page;

	group = pp->groups[idx];

	if (yk3_ringb_full(&group->rb))
		return NULL;

	page = &group->pages[yk3_ringb_head(&group->rb)];

	return page;
}

static int yk3_pagegroup_recycle(struct yk3_pagepool *pp, u16 idx)
{
	struct yk3_pagegroup *group;
	struct yk3_page *page;
	int recycle = 0;

	group = pp->groups[idx];

	while (!yk3_ringb_empty(&group->rb)) {
		page = &group->pages[yk3_ringb_tail(&group->rb)];

		if (!yk3_page_recycle(pp, page))
			break;

		group->freenum++;
		group->freefragnum += page->maxnum;

		yk3_ringb_pop(&group->rb);
		recycle++;
	}

	return recycle;
}

static int yk3_pagegroup_alloc(struct yk3_pagepool *pp, struct yk3_pagegroup *group,
			       struct yk3_page *page)
{
	int fidx;

	fidx = yk3_page_alloc(pp, page);
	if (fidx < 0)
		return fidx;

	if (page_no_free(page)) {
		yk3_ringb_push(&group->rb);
		group->freenum--;
	}

	group->freefragnum--;

	return fidx;
}

static void yk3_pagegroup_free(struct yk3_pagepool *pp, struct yk3_pagegroup *group,
			       struct yk3_page *page, int fidx)
{
	yk3_page_free(pp, page, fidx);
}

struct yk3_pagepool *yk3_pp_create(struct yk3_pdev_priv *pdev_priv, u16 datasize)
{
	u16 fragsize;
	u16 pagefragmax;
	u16 fragpower;
	struct yk3_pagepool *pp;

	if (datasize > YK3_N_PP_FRAGSIZE_MAX)
		datasize = YK3_N_PP_FRAGSIZE_MAX;

	fragsize = roundup_pow_of_two(datasize);
	pagefragmax = PAGE_SIZE / fragsize;
	pagefragmax = min_t(u16, pagefragmax, YK3_N_PP_PAGEFRAG_MAX);
	fragsize = PAGE_SIZE / pagefragmax;
	fragpower = ilog2(fragsize);

	pp = kzalloc(sizeof(*pp), GFP_KERNEL);
	if (!pp)
		return NULL;

	pp->pdev_priv = pdev_priv;
	pp->dev = pdev_priv->dev;

	pp->groupnum = 0;
	pp->fragpower = fragpower;
	pp->pagefragmax = pagefragmax;

	return pp;
}

void yk3_pp_destroy(struct yk3_pagepool *pp)
{
	int i;
	struct yk3_pagegroup *group;

	for_each_set_bit(i, pp->emptybits, YK3_N_PP_PAGEGROUP_MAX) {
		group = pp->groups[i];

		yk3_pagegroup_destroy(pp, group);
	}

	kfree(pp);
}

static struct yk3_page *yk3_pp_get_page(struct yk3_pagepool *pp)
{
	int idx;
	struct yk3_pagegroup *group;

	idx = find_first_bit(pp->freebits, YK3_N_PP_PAGEGROUP_MAX);
	if (idx < YK3_N_PP_PAGEGROUP_MAX)
		goto find_group;

	idx = find_first_zero_bit(pp->emptybits, YK3_N_PP_PAGEGROUP_MAX);
	if (idx >= YK3_N_PP_PAGEGROUP_MAX)
		return NULL;

	group = yk3_pagegroup_create(pp, idx);
	if (!group)
		return NULL;

	pp->stat.num_create_group++;
	set_bit(idx, pp->freebits);
	set_bit(idx, pp->emptybits);
	pp->groups[idx] = group;
	pp->groupnum++;

find_group:
	return yk3_pagegroup_get_page(pp, idx);
}

static int yk3_pp_recycle_group(struct yk3_pagepool *pp, struct yk3_pagegroup *group)
{
	int ret;
	bool freezero = false;

	if (group_no_free(group))
		freezero = true;

	ret = yk3_pagegroup_recycle(pp, group->gidx);

	if (freezero && !group_no_free(group))
		set_bit(group->gidx, pp->freebits);

	return ret;
}

static void yk3_pp_destroy_group(struct yk3_pagepool *pp, struct yk3_pagegroup *group)
{
	clear_bit(group->gidx, pp->emptybits);
	clear_bit(group->gidx, pp->freebits);
	pp->groupnum--;
	pp->stat.num_destroy_group++;
	yk3_pagegroup_destroy(pp, group);
}

void yk3_pp_recycle(struct yk3_pagepool *pp)
{
	int i;
	int ret;
	int recycle = 0;
	struct yk3_pagegroup *group;
	int freegroupcnt = 0;
	struct yk3_pagegroup *freegroup = NULL;

	pp->stat.num_recycle++;
	for_each_set_bit(i, pp->emptybits, YK3_N_PP_PAGEGROUP_MAX) {
		group = pp->groups[i];

		ret = yk3_pp_recycle_group(pp, group);
		recycle += ret;

		if (group_all_free(group)) {
			freegroupcnt++;
			freegroup = group;
		}
	}

	if (freegroupcnt > 1)
		yk3_pp_destroy_group(pp, freegroup);
}

int yk3_pp_alloc(struct yk3_pagepool *pp, struct yk3_page **page_ret)
{
	int ret;
	struct yk3_page *page;
	struct yk3_pagegroup *group;

	page = yk3_pp_get_page(pp);
	if (!page)
		return -ENOMEM;

	group = pp->groups[page->gidx];

	ret = yk3_pagegroup_alloc(pp, group, page);
	if (ret < 0)
		return ret;

	if (yk3_ringb_full(&group->rb))
		clear_bit(page->gidx, pp->freebits);

	*page_ret = page;

	return ret;
}

void yk3_pp_free(struct yk3_pagepool *pp, struct yk3_page *page, u16 fidx)
{
	struct yk3_pagegroup *group = pp->groups[page->gidx];

	yk3_pagegroup_free(pp, group, page, fidx);
}
