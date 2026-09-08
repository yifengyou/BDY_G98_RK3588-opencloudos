// SPDX-License-Identifier: GPL-2.0
#include "yk3.h"
#include "yk3_dmamap.h"

struct yk3_dmamap_node {
	struct rb_node rb;
	u64 start;	/* Start of interval */
	u64 end;	/* end location _in_ interval */
	u64 pa;
	u64 __subtree_last;
	struct page **pages;
	int npages;
	refcount_t refcnt; /* reference count */
};

struct yk3_dmamap_table {
#ifdef RB_ROOT_CACHED
	struct rb_root_cached root;
#else
	struct rb_root root;
#endif
	struct mutex mlock;;       //protect root
	struct list_head node;
	atomic_t refcnt;
	struct iommu_domain *domain;
	struct device *dev;
	struct iommu_group *group;
};

struct yk3_dmamap_table_head {
	struct list_head head;
	struct mutex mlock;	//protect list
};

typedef void (*yk3_dmamap_cb)(struct yk3_dmamap_table *tbl,
			      struct yk3_dmamap_node *node, bool force);

static struct yk3_dmamap_table_head yk3_dmamap_tlb_head = {
	.head = LIST_HEAD_INIT(yk3_dmamap_tlb_head.head),
	.mlock = __MUTEX_INITIALIZER(yk3_dmamap_tlb_head.mlock),
};

#define START(node) ((node)->start)
#define LAST(node) ((node)->end)
INTERVAL_TREE_DEFINE(struct yk3_dmamap_node,
		     rb, u64, __subtree_last,
		     START, LAST, static inline, yk3_dmamap_node);

static void yk3_dmamap_flush(struct yk3_dmamap_table *tbl);

struct yk3_dmamap_table *yk3_dmamap_table_create(struct device *dev)
{
	struct yk3_dmamap_table_head *head = &yk3_dmamap_tlb_head;
	struct yk3_dmamap_table *tbl;
	struct iommu_group *group;

	group = iommu_group_get(dev);
	if (!group)
		return NULL;

	mutex_lock(&head->mlock);
	list_for_each_entry(tbl, &head->head, node) {
		if (tbl->dev == dev || tbl->group == group) {
			iommu_group_put(group);
			atomic_inc(&tbl->refcnt);
			mutex_unlock(&head->mlock);
			return tbl;
		}
	}
	mutex_unlock(&head->mlock);

	tbl = kzalloc(sizeof(*tbl), GFP_KERNEL);
	if (!tbl) {
		iommu_group_put(group);
		return NULL;
	}
#ifdef RB_ROOT_CACHED
	tbl->root = RB_ROOT_CACHED;
#else
	tbl->root = RB_ROOT;
#endif
	INIT_LIST_HEAD(&tbl->node);
	atomic_set(&tbl->refcnt, 1);
	tbl->dev = dev;

	tbl->domain = iommu_get_domain_for_dev(dev);
	if (!tbl->domain)
		goto out_free_domain;

	tbl->group = group;
	mutex_init(&tbl->mlock);

	list_add_tail(&tbl->node, &head->head);

	return tbl;

out_free_domain:
	kfree(tbl);
	iommu_group_put(group);
	return NULL;
}

void yk3_dmamap_table_destroy(struct yk3_dmamap_table *tbl)
{
	struct yk3_dmamap_table_head *head = &yk3_dmamap_tlb_head;

	if (!atomic_dec_and_test(&tbl->refcnt))
		return;

	mutex_lock(&head->mlock);
	list_del(&tbl->node);
	mutex_unlock(&head->mlock);

	yk3_dmamap_flush(tbl);

	iommu_group_put(tbl->group);

	kfree(tbl);
}

static int yk3_dmamap_add(struct yk3_dmamap_table *tbl, u64 start, u64 end,
			  u64 pa, void *opaque)
{
	struct yk3_dmamap_node *node;
	struct page **pages = (struct page **)opaque;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->start = start;
	node->end = end;
	node->pa = pa;
	node->npages = (end - start + PAGE_SIZE) >> PAGE_SHIFT;
	refcount_set(&node->refcnt, 1);

	node->pages = vmalloc(node->npages * sizeof(struct page *));
	if (!node->pages)
		return -ENOMEM;
	memcpy(node->pages, pages, node->npages * sizeof(struct page *));

	yk3_dmamap_node_insert(node, &tbl->root);

	return 0;
}

static void yk3_dmamap_del_cb(struct yk3_dmamap_table *tbl,
			      struct yk3_dmamap_node *node,
			      bool force)
{
	u64 iova;
	u64 size;
	size_t npages;
	struct page **pages;
#ifndef YK3_HAVE_UNPIN_USER_PAGES
	unsigned int i;
#endif

	if (!force && !refcount_dec_and_test(&node->refcnt))
		return;

	iova = node->start;
	size = node->end - node->start + 1;
	npages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	pages = node->pages;

#ifdef YK3_HAVE_UNPIN_USER_PAGES
	iommu_unmap(tbl->domain, iova, size);
	unpin_user_pages(pages, npages);
#else
	iommu_unmap(tbl->domain, iova, size);

	for (i = 0; i < npages; i++) {
		if (pages[i])
			put_page(pages[i]);
	}
#endif /* YK3_HAVE_UNPIN_USER_PAGES */

	yk3_dmamap_node_remove(node, &tbl->root);

	vfree(node->pages);
	kfree(node);
}

static void yk3_dmamap_iter(struct yk3_dmamap_table *tbl, u64 start, u64 end,
			    yk3_dmamap_cb cb, bool force)
{
	struct yk3_dmamap_node *node;

	while ((node = yk3_dmamap_node_iter_first(&tbl->root, start, end)) != NULL)
		cb(tbl, node, force);
}

static void yk3_dmamap_del(struct yk3_dmamap_table *tbl, u64 start, u64 end)
{
	yk3_dmamap_iter(tbl, start, end, yk3_dmamap_del_cb, true);
}

static void yk3_dmamap_flush(struct yk3_dmamap_table *tbl)
{
	yk3_dmamap_del(tbl, 0ULL, 0ULL - 1);
}

bool yk3_dmamap_exist(struct yk3_dmamap_table *tbl, u64 start, u64 end)
{
	struct yk3_dmamap_node *node;

	mutex_lock(&tbl->mlock);
	node = yk3_dmamap_node_iter_first(&tbl->root, start, end);
	mutex_unlock(&tbl->mlock);

	return !!node;
}

void yk3_dmamap_update_refcnt(struct yk3_dmamap_table *tbl, u64 start, u64 end)
{
	struct yk3_dmamap_node *node;

	node = yk3_dmamap_node_iter_first(&tbl->root, start, end);
	while (node) {
		refcount_inc(&node->refcnt);
		node = yk3_dmamap_node_iter_next(node, start, end);
	}
}

int yk3_dmamap_map(struct yk3_dmamap_table *tbl, u64 iova, u64 size,
		   u64 pa, void *opaque)
{
	int ret;
	int prot =  (IOMMU_WRITE | IOMMU_READ);

	if (yk3_dmamap_exist(tbl, iova, iova + size - 1))
		return -EEXIST;

	mutex_lock(&tbl->mlock);
#ifdef YK3_HAVE_IOMMU_MAP_GFP
	ret = iommu_map(tbl->domain, iova, pa, size, prot, GFP_KERNEL);
#else
	ret = iommu_map(tbl->domain, iova, pa, size, prot);
#endif
	if (ret) {
		mutex_unlock(&tbl->mlock);
		return ret;
	}

	ret = yk3_dmamap_add(tbl, iova, iova + size - 1, pa, opaque);
	mutex_unlock(&tbl->mlock);

	return ret;
}

void yk3_dmamap_unmap(struct yk3_dmamap_table *tbl, u64 iova, u64 size)
{
	mutex_lock(&tbl->mlock);
	yk3_dmamap_iter(tbl, iova, iova + size - 1, yk3_dmamap_del_cb, false);
	mutex_unlock(&tbl->mlock);
}
