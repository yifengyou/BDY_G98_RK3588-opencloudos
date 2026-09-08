/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_SCATTER_H
#define _YK3_SCATTER_H

#include "yk3_base.h"
#include "yk3_edma.h"

struct yk3_scatterlist {
	dma_addr_t addr;
	u32 len;
	u16 freeid;
	u16 page_map:1;
};

struct yk3_sctfrag {
	u16 seg_start;
	u16 seg_num;
	bool tso_fp;
	bool tso_lp;
	bool tso_valid;
	u8 mss_num;
};

struct yk3_sctseg {
	dma_addr_t addr;
	u32 len;
	bool fp;
	bool lp;
	bool unmap;
	u16 sctlidx;
};

struct yk3_scatter {
	u16 sctlist_num;
	struct yk3_scatterlist sctlist[YK3_N_MAX_SCTLIST];
	struct yk3_sctfrag frags[YK3_N_MAX_SCTFRAGS];
	struct yk3_sctseg segs[YK3_N_MAX_SCTSEGS];
	struct sk_buff *skb;
	u8 hdrlen;
};

struct yk3_scatter_iter {
	u8 frag_idx;
	u8 seg_idx;
	u8 seg_num;
	bool unmap;
	struct yk3_sctfrag *frag;
	struct yk3_sctseg *seg;
	struct yk3_scatterlist *sctl;
};

static inline void
yk3_scatter_iter_start(struct yk3_scatter *scatter, struct yk3_scatter_iter *iter)
{
	iter->frag_idx = 0;
	iter->seg_idx = 0;
	iter->seg_num = scatter->frags[0].seg_num;
	iter->frag = scatter->frags;
	iter->seg = scatter->segs;
	iter->unmap = iter->seg->unmap;
	if (iter->unmap)
		iter->sctl = &scatter->sctlist[iter->seg->sctlidx];
	else
		iter->sctl = NULL;
}

static inline bool
yk3_scatter_iter_cond(struct yk3_scatter *scatter,
		      struct yk3_scatter_iter *iter)
{
	u8 seg_start;
	u8 seg_num;

	seg_start = scatter->frags[iter->frag_idx].seg_start;
	seg_num = scatter->frags[iter->frag_idx].seg_num;
	if (!seg_num)
		return false;
	if (iter->seg_idx >= seg_start + seg_num)
		return false;
	return true;
}

static inline void
yk3_scatter_iter_next(struct yk3_scatter *scatter,
		      struct yk3_scatter_iter *iter)
{
	u8 seg_start;
	u8 seg_num;

	seg_start = scatter->frags[iter->frag_idx].seg_start;
	seg_num = scatter->frags[iter->frag_idx].seg_num;
	iter->seg_idx++;
	if (iter->seg_idx >= seg_start + seg_num)
		iter->frag_idx++;
	iter->seg_num = scatter->frags[iter->frag_idx].seg_num;
	iter->frag = &scatter->frags[iter->frag_idx];
	iter->seg = &scatter->segs[iter->seg_idx];
	iter->unmap = iter->seg->unmap;
	if (iter->unmap)
		iter->sctl = &scatter->sctlist[iter->seg->sctlidx];
	else
		iter->sctl = NULL;
}

int yk3_scatter_construct(struct yk3_scatter *scatter, struct device *dev,
			  struct sk_buff *skb, u16 fragsize);
void yk3_scatter_destruct(struct yk3_scatter *scatter, struct device *dev);

#endif /* _YK3_SCATTER_H */
