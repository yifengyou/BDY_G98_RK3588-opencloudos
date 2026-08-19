// SPDX-License-Identifier: GPL-2.0

#include "yk3_scatter.h"

struct yk3_scatter_additer {
	/* property */
	u16 hdrlen;
	u16 mss;
	u16 maxfragsize;
	u16 maxtxdnum;
	u16 maxmssnum;

	/* outer */
	u16 i;
	u16 cnt;

	/* inner */
	u16 fragidx;
	u16 segidx;
	u16 frag_copied;

	struct yk3_scatter *scatter;
	dma_addr_t header_addr;
	u32 headlen;
};

static int
yk3_scatter_add(struct yk3_scatter_additer *additer, dma_addr_t addr, u32 len)
{
	struct yk3_sctfrag *frag;
	struct yk3_sctseg *seg;
	bool head_copied = false;
	bool head_first = true;
	u16 fragsize;
	u16 mssnum;

again:
	fragsize = additer->maxfragsize;
	mssnum = additer->maxmssnum;

	if (additer->fragidx >= ARRAY_SIZE(additer->scatter->frags) ||
	    additer->segidx >= ARRAY_SIZE(additer->scatter->segs))
		return -ENOBUFS;

	frag = &additer->scatter->frags[additer->fragidx];
	seg = &additer->scatter->segs[additer->segidx];

	if (frag->seg_num > (additer->maxtxdnum >> 1)) {
		mssnum = (additer->frag_copied - additer->hdrlen + additer->mss - 1);
		mssnum = mssnum / additer->mss;
		mssnum = mssnum ?: 1;
		fragsize = mssnum * additer->mss + additer->hdrlen;
	}

	/* 1. add header */
	if (!additer->frag_copied) {
		seg->addr = additer->header_addr;
		if (!additer->i && !head_copied) {
			seg->len = min_t(u32, len, fragsize);
			head_copied = true;
			len -= seg->len;
			addr += seg->len;
		} else {
			seg->len = additer->hdrlen;
		}
		seg->fp = true;
		if (seg->len == fragsize || (additer->cnt == 1 && !len))
			seg->lp = true;
		else
			seg->lp = false;

		additer->scatter->sctlist[0].freeid = additer->segidx;

		frag->seg_start = additer->segidx;
		frag->seg_num++;
		if (!additer->i && head_first)
			frag->tso_fp = true;
		if (additer->cnt == 1 && !len)
			frag->tso_lp = true;
		else
			frag->tso_lp = false;
		frag->tso_valid = true;

		additer->frag_copied += seg->len;
		additer->segidx++;
		head_first = false;
		if (seg->len == fragsize) {
			frag->mss_num = mssnum;
			additer->fragidx++;
			additer->frag_copied = 0;
			goto again;
		}
	}

	if (additer->segidx >= ARRAY_SIZE(additer->scatter->segs))
		return -ENOBUFS;

	/* 2. add payload */
	if (len == 0)
		return 0;

	seg = &additer->scatter->segs[additer->segidx];

	if (len < (fragsize - additer->frag_copied)) {
		seg->addr = addr;
		seg->len = len;

		seg->fp = false;
		if (additer->i == (additer->cnt - 1))
			seg->lp = true;

		frag->seg_num++;
		if (additer->i == (additer->cnt - 1))
			frag->tso_lp = true;

		addr += len;
		len -= len;

		if (!len)
			additer->scatter->sctlist[additer->i].freeid = additer->segidx;
		if (!additer->i)
			additer->scatter->sctlist[0].freeid = additer->segidx;

		additer->frag_copied += seg->len;
		additer->segidx++;
	} else {
		seg->addr = addr;
		seg->len = fragsize - additer->frag_copied;
		seg->fp = false;
		seg->lp = true;

		frag->seg_num++;
		if (additer->i == (additer->cnt - 1) && !len)
			frag->tso_lp = true;

		addr += fragsize - additer->frag_copied;
		len -= fragsize - additer->frag_copied;

		if (!len)
			additer->scatter->sctlist[additer->i].freeid = additer->segidx;
		if (!additer->i)
			additer->scatter->sctlist[0].freeid = additer->segidx;

		additer->frag_copied = 0;
		additer->segidx++;
		additer->fragidx++;
		frag->mss_num = mssnum;
	}

	if (additer->fragidx >= ARRAY_SIZE(additer->scatter->frags) ||
	    additer->segidx >= ARRAY_SIZE(additer->scatter->segs))
		return -ENOBUFS;

	if (len > 0)
		goto again;

	return 0;
}

static int
yk3_scatter_complex(struct yk3_scatter *scatter, u16 mss, u16 maxtxdnum,
		    u16 fragsize, u16 hdrlen)
{
	int i, ret;
	struct yk3_scatterlist *sctlist;
	struct yk3_scatter_additer additer = {0};
	dma_addr_t dma_addr;
	u32 dma_len;

	if (fragsize < mss + hdrlen)
		return -EINVAL;

	additer.hdrlen = hdrlen;
	additer.mss = mss;
	additer.maxtxdnum = maxtxdnum;
	additer.maxfragsize = (fragsize - hdrlen) / mss * mss + hdrlen;
	additer.maxmssnum = (fragsize - hdrlen) / mss;
	additer.scatter = scatter;
	additer.header_addr = scatter->sctlist[0].addr;
	additer.headlen = scatter->sctlist[0].len;

	for (i = 0; i < scatter->sctlist_num; i++) {
		sctlist = &scatter->sctlist[i];

		dma_addr = sctlist->addr;
		dma_len = sctlist->len;

		additer.i = i;
		additer.cnt = scatter->sctlist_num;

		ret = yk3_scatter_add(&additer, dma_addr, dma_len);
		if (ret < 0)
			goto failed;
	}

	for (i = 0; i < scatter->sctlist_num; i++) {
		sctlist = &scatter->sctlist[i];
		scatter->segs[sctlist->freeid].unmap = true;
		scatter->segs[sctlist->freeid].sctlidx = i;
	}

	for (i = 0; i < ARRAY_SIZE(scatter->frags) && scatter->frags[i].seg_num; i++) {
		if (scatter->frags[i].seg_num > maxtxdnum)
			goto failed;
	}

	return 0;

failed:
	return -ENOBUFS;
}

static int
yk3_scatter_map(struct yk3_scatter *scatter, struct device *dev, struct sk_buff *skb)
{
	int i;
	int sctlist_idx = 0;
	struct yk3_scatterlist *sctlist;
	skb_frag_t *frag;
	dma_addr_t dma_addr;
	size_t dma_len;

	if (skb_headlen(skb) > 0) {
		sctlist = &scatter->sctlist[sctlist_idx];
		sctlist->page_map = 0;
		dma_len = skb_headlen(skb);
		dma_addr = dma_map_single(dev, skb->data, dma_len, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(dev, dma_addr)))
			goto err_unmap;
		dma_unmap_addr_set(sctlist, addr, dma_addr);
		dma_unmap_len_set(sctlist, len, dma_len);
		sctlist_idx++;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++, sctlist_idx++) {
		sctlist = &scatter->sctlist[sctlist_idx];
		sctlist->page_map = 1;
		frag = &skb_shinfo(skb)->frags[i];
		dma_len = skb_frag_size(&skb_shinfo(skb)->frags[i]);
		dma_addr = skb_frag_dma_map(dev, frag, 0, dma_len, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(dev, dma_addr)))
			goto err_unmap;
		dma_unmap_addr_set(sctlist, addr, dma_addr);
		dma_unmap_len_set(sctlist, len, dma_len);
	}

	scatter->sctlist_num = sctlist_idx;

	return 0;

err_unmap:
	for (sctlist_idx--; sctlist_idx >= 0; sctlist_idx--) {
		sctlist = &scatter->sctlist[sctlist_idx];
		if (!sctlist->page_map)
			dma_unmap_single(dev, dma_unmap_addr(sctlist, addr),
					 dma_unmap_len(sctlist, len), DMA_TO_DEVICE);
		else
			dma_unmap_page(dev, dma_unmap_addr(sctlist, addr),
				       dma_unmap_len(sctlist, len), DMA_TO_DEVICE);
	}
	return -ENOMEM;
}

static void
yk3_scatter_unmap(struct yk3_scatter *scatter, struct device *dev)
{
	int i;
	struct yk3_scatterlist *sctlist;

	for (i = 0; i < scatter->sctlist_num; i++) {
		sctlist = &scatter->sctlist[i];
		if (!sctlist->page_map)
			dma_unmap_single(dev, dma_unmap_addr(sctlist, addr),
					 dma_unmap_len(sctlist, len), DMA_TO_DEVICE);
		else
			dma_unmap_page(dev, dma_unmap_addr(sctlist, addr),
				       dma_unmap_len(sctlist, len), DMA_TO_DEVICE);
	}
}

int
yk3_scatter_construct(struct yk3_scatter *scatter, struct device *dev,
		      struct sk_buff *skb, u16 fragsize)
{
	int i;
	int hdrlen = 0;
	int ret;

	/* 1. sgvec init */
	memset(scatter, 0, sizeof(*scatter));
	scatter->skb = skb;

	/* 2. skb_to_sgvec */
	ret = yk3_scatter_map(scatter, dev, skb);
	if (ret < 0)
		return ret;

	if (skb_is_gso(skb)) {
		if (skb_shinfo(skb)->gso_type & (SKB_GSO_TCPV6 | SKB_GSO_TCPV4))
			hdrlen = skb->encapsulation ? (skb_inner_transport_offset(skb) +
				 inner_tcp_hdrlen(skb)) : (skb_transport_offset(skb) +
				 tcp_hdrlen(skb));
#ifdef YK3_HAVE_NETIF_F_GSO_UDP_L4
		else if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_L4)
			hdrlen = skb->encapsulation ? (skb_inner_transport_offset(skb) +
				 sizeof(struct udphdr)) : (skb_transport_offset(skb) +
				 sizeof(struct udphdr));
#endif
		else
			return -EINVAL;

		scatter->hdrlen = (u8)hdrlen;
	}

	/* 3. to tx_scatter */
	if (skb->len <= fragsize || (!skb_is_gso(skb) && skb->data_len <= fragsize)) {
		/* 3.1 normal */
		if (!skb_is_gso(skb)) {
			scatter->frags[0].tso_fp = false;
			scatter->frags[0].tso_lp = false;
			scatter->frags[0].tso_valid = false;
		/* 3.2 gso and <= fragsize */
		} else {
			scatter->frags[0].tso_fp = true;
			scatter->frags[0].tso_lp = true;
			scatter->frags[0].tso_valid = true;
		}
		scatter->frags[0].seg_start = 0;
		scatter->frags[0].seg_num = scatter->sctlist_num;

		for (i = 0; i < scatter->sctlist_num; i++) {
			scatter->segs[i].fp = (i == 0) ? 1 : 0;
			scatter->segs[i].lp = (i ==  scatter->sctlist_num - 1) ? 1 : 0;
			scatter->segs[i].addr = dma_unmap_addr(&scatter->sctlist[i], addr);
			scatter->segs[i].len = dma_unmap_len(&scatter->sctlist[i], len);
			scatter->segs[i].unmap = true;
			scatter->segs[i].sctlidx = i;
		}
	/* 3.3 gso and > fragsize */
	} else {
		ret = yk3_scatter_complex(scatter, skb_shinfo(skb)->gso_size, YK3_N_MAX_TXD,
					  fragsize, hdrlen);
		if (ret < 0)
			goto tx_scatter_failed;
	}

	return 0;

tx_scatter_failed:
	yk3_scatter_unmap(scatter, dev);
	return ret;
}

void yk3_scatter_destruct(struct yk3_scatter *scatter, struct device *dev)
{
	yk3_scatter_unmap(scatter, dev);
}
