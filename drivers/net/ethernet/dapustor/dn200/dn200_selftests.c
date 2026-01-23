// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include <linux/bitrev.h>
#include <linux/completion.h>
#include <linux/crc32.h>
#include <linux/ethtool.h>
#include <linux/ip.h>
#include <linux/phy.h>
#include <linux/udp.h>
#include <net/pkt_cls.h>
#include <net/pkt_sched.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/tc_act/tc_gact.h>
#include "dn200.h"
#include "dn200_ctrl.h"
#include <linux/if_vlan.h>
#include <net/arp.h>
struct dn200hdr {
	__be32 version;
	__be64 magic;
	u8 id;
} __packed;

#define DN200_TEST_PKT_SIZE (sizeof(struct ethhdr) + sizeof(struct iphdr) + \
			      sizeof(struct dn200hdr))
#define DN200_TEST_PKT_MAGIC	0xdeadcafecafedeadULL
#define DN200_LB_TIMEOUT	msecs_to_jiffies(200)

struct dn200_packet_attrs {
	int vlan;
	int vlan_id_in;
	int vlan_id_out;
	const unsigned char *src;
	const unsigned char *dst;
	u32 ip_src;
	u32 ip_dst;
	int tcp;
	int sport;
	int dport;
	u32 exp_hash;
	int dont_wait;
	int timeout;
	int size;
	int max_size;
	int remove_sa;
	u8 id;
	int sarc;
	u16 queue_mapping;
	u64 timestamp;
};

static u8 dn200_test_next_id;

static struct sk_buff *dn200_test_get_udp_skb(struct dn200_priv *priv,
					      struct dn200_packet_attrs *attr)
{
	struct sk_buff *skb = NULL;
	struct udphdr *uhdr = NULL;
	struct tcphdr *thdr = NULL;
	struct dn200hdr *shdr;
	struct ethhdr *ehdr;
	struct iphdr *ihdr;
	int iplen, size;

	size = attr->size + DN200_TEST_PKT_SIZE;
	if (attr->vlan) {
		size += 4;
		if (attr->vlan > 1)
			size += 4;
	}

	if (attr->tcp)
		size += sizeof(struct tcphdr);
	else
		size += sizeof(struct udphdr);

	if (attr->max_size && attr->max_size > size)
		size = attr->max_size;

	skb = netdev_alloc_skb(priv->dev, size);
	if (!skb)
		return NULL;

	prefetchw(skb->data);

	if (attr->vlan > 1)
		ehdr = skb_push(skb, ETH_HLEN + 8);
	else if (attr->vlan)
		ehdr = skb_push(skb, ETH_HLEN + 4);
	else if (attr->remove_sa)
		ehdr = skb_push(skb, ETH_HLEN - 6);
	else
		ehdr = skb_push(skb, ETH_HLEN);
	skb_reset_mac_header(skb);

	skb_set_network_header(skb, skb->len);
	ihdr = skb_put(skb, sizeof(*ihdr));

	skb_set_transport_header(skb, skb->len);
	if (attr->tcp)
		thdr = skb_put(skb, sizeof(*thdr));
	else
		uhdr = skb_put(skb, sizeof(*uhdr));

	if (!attr->remove_sa)
		eth_zero_addr(ehdr->h_source);
	eth_zero_addr(ehdr->h_dest);
	if (attr->src && !attr->remove_sa)
		ether_addr_copy(ehdr->h_source, attr->src);
	if (attr->dst)
		ether_addr_copy(ehdr->h_dest, attr->dst);

	if (!attr->remove_sa) {
		ehdr->h_proto = htons(ETH_P_IP);
	} else {
		/* HACK */
		ehdr->h_proto = htons(ETH_P_IP);
	}

	if (attr->vlan) {
		__be16 *tag, *proto;

		if (!attr->remove_sa) {
			tag = (void *)ehdr + ETH_HLEN;
			proto = (void *)ehdr + (2 * ETH_ALEN);
		} else {
			tag = (void *)ehdr + ETH_HLEN - 6;
			proto = (void *)ehdr + ETH_ALEN;
		}

		proto[0] = htons(ETH_P_8021Q);
		tag[0] = htons(attr->vlan_id_out);
		tag[1] = htons(ETH_P_IP);
		if (attr->vlan > 1) {
			proto[0] = htons(ETH_P_8021AD);
			tag[1] = htons(ETH_P_8021Q);
			tag[2] = htons(attr->vlan_id_in);
			tag[3] = htons(ETH_P_IP);
		}
	}

	if (attr->tcp) {
		thdr->source = htons(attr->sport);
		thdr->dest = htons(attr->dport);
		thdr->doff = sizeof(struct tcphdr) / 4;
		thdr->check = 0;
	} else {
		uhdr->source = htons(attr->sport);
		uhdr->dest = htons(attr->dport);
		uhdr->len = htons(sizeof(*shdr) + sizeof(*uhdr) + attr->size);
		if (attr->max_size)
			uhdr->len = htons(attr->max_size -
					  (sizeof(*ihdr) + sizeof(*ehdr)));
		uhdr->check = 0;
	}

	ihdr->ihl = 5;
	ihdr->ttl = 32;
	ihdr->version = 4;
	if (attr->tcp)
		ihdr->protocol = IPPROTO_TCP;
	else
		ihdr->protocol = IPPROTO_UDP;
	iplen = sizeof(*ihdr) + sizeof(*shdr) + attr->size;
	if (attr->tcp)
		iplen += sizeof(*thdr);
	else
		iplen += sizeof(*uhdr);

	if (attr->max_size)
		iplen = attr->max_size - sizeof(*ehdr);

	ihdr->tot_len = htons(iplen);
	ihdr->frag_off = 0;
	ihdr->saddr = htonl(attr->ip_src);
	ihdr->daddr = htonl(attr->ip_dst);
	ihdr->tos = 0;
	ihdr->id = 0;
	ip_send_check(ihdr);

	shdr = skb_put(skb, sizeof(*shdr));
	shdr->version = 0;
	shdr->magic = cpu_to_be64(DN200_TEST_PKT_MAGIC);
	attr->id = dn200_test_next_id;
	shdr->id = dn200_test_next_id++;

	if (attr->size)
		skb_put(skb, attr->size);
	if (attr->max_size && (attr->max_size > skb->len))
		skb_put(skb, attr->max_size - skb->len);

	skb->csum = 0;
	skb->ip_summed = CHECKSUM_PARTIAL;
	if (attr->tcp) {
		thdr->check =
		    ~tcp_v4_check(skb->len, ihdr->saddr, ihdr->daddr, 0);
		skb->csum_start = skb_transport_header(skb) - skb->head;
		skb->csum_offset = offsetof(struct tcphdr, check);
	} else {
		udp4_hwcsum(skb, ihdr->saddr, ihdr->daddr);
	}

	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->dev = priv->dev;

	if (attr->timestamp)
		skb->tstamp = ns_to_ktime(attr->timestamp);

	return skb;
}

static struct sk_buff *dn200_test_get_arp_skb(struct dn200_priv *priv,
					      struct dn200_packet_attrs *attr)
{
	__be32 ip_src = htonl(attr->ip_src);
	__be32 ip_dst = htonl(attr->ip_dst);
	struct sk_buff *skb = NULL;

	skb = arp_create(ARPOP_REQUEST, ETH_P_ARP, ip_dst, priv->dev, ip_src,
			 NULL, attr->src, attr->dst);
	if (!skb)
		return NULL;

	skb->pkt_type = PACKET_HOST;
	skb->dev = priv->dev;

	return skb;
}

struct dn200_test_priv {
	struct dn200_packet_attrs *packet;
	struct packet_type pt;
	struct completion comp;
	int double_vlan;
	int vlan_id;
	int ok;
};

static int dn200_test_loopback_validate(struct sk_buff *skb,
					struct net_device *ndev,
					struct packet_type *pt,
					struct net_device *orig_ndev)
{
	struct dn200_test_priv *tpriv = pt->af_packet_priv;
	const unsigned char *src = tpriv->packet->src;
	const unsigned char *dst = tpriv->packet->dst;
	struct dn200hdr *shdr;
	struct ethhdr *ehdr;
	struct udphdr *uhdr;
	struct tcphdr *thdr;
	struct iphdr *ihdr;

	skb = skb_unshare(skb, GFP_ATOMIC);
	if (!skb)
		goto out;

	if (skb_linearize(skb))
		goto out;
	if (skb_headlen(skb) < (DN200_TEST_PKT_SIZE - ETH_HLEN))
		goto out;

	ehdr = (struct ethhdr *)skb_mac_header(skb);
	if (dst) {
		if (!ether_addr_equal_unaligned(ehdr->h_dest, dst))
			goto out;
	}
	if (tpriv->packet->sarc) {
		if (!ether_addr_equal_unaligned(ehdr->h_source, ehdr->h_dest))
			goto out;
	} else if (src) {
		if (!ether_addr_equal_unaligned(ehdr->h_source, src))
			goto out;
	}

	ihdr = ip_hdr(skb);
	if (tpriv->double_vlan)
		ihdr = (struct iphdr *)(skb_network_header(skb) + 4);

	if (tpriv->packet->tcp) {
		if (ihdr->protocol != IPPROTO_TCP)
			goto out;

		thdr = (struct tcphdr *)((u8 *) ihdr + 4 * ihdr->ihl);
		if (thdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct dn200hdr *)((u8 *) thdr + sizeof(*thdr));
	} else {
		if (ihdr->protocol != IPPROTO_UDP)
			goto out;

		uhdr = (struct udphdr *)((u8 *) ihdr + 4 * ihdr->ihl);
		if (uhdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct dn200hdr *)((u8 *) uhdr + sizeof(*uhdr));
	}

	if (shdr->magic != cpu_to_be64(DN200_TEST_PKT_MAGIC))
		goto out;
	if (tpriv->packet->exp_hash && !skb->hash)
		goto out;
	if (tpriv->packet->id != shdr->id)
		goto out;

	tpriv->ok = true;
	complete(&tpriv->comp);
out:
	kfree_skb(skb);
	return 0;
}

static int __dn200_test_loopback(struct dn200_priv *priv,
				 struct dn200_packet_attrs *attr)
{
	struct dn200_test_priv *tpriv;
	struct sk_buff *skb = NULL;
	int ret = 0;

	tpriv = kzalloc(sizeof(*tpriv), GFP_KERNEL);
	if (!tpriv)
		return -ENOMEM;

	tpriv->ok = false;
	init_completion(&tpriv->comp);

	tpriv->pt.type = htons(ETH_P_IP);
	tpriv->pt.func = dn200_test_loopback_validate;
	tpriv->pt.dev = priv->dev;
	tpriv->pt.af_packet_priv = tpriv;
	tpriv->packet = attr;

	if (!attr->dont_wait)
		dev_add_pack(&tpriv->pt);

	skb = dn200_test_get_udp_skb(priv, attr);
	if (!skb) {
		ret = -ENOMEM;
		goto cleanup;
	}
	ret = dev_direct_xmit(skb, attr->queue_mapping);
	if (ret)
		goto cleanup;

	if (attr->dont_wait)
		goto cleanup;

	if (!attr->timeout)
		attr->timeout = DN200_LB_TIMEOUT;

	wait_for_completion_timeout(&tpriv->comp, attr->timeout);
	ret = tpriv->ok ? 0 : -ETIMEDOUT;
cleanup:
	if (!attr->dont_wait)
		dev_remove_pack(&tpriv->pt);
	kfree(tpriv);

	return ret;
}

static int dn200_test_mac_loopback(struct dn200_priv *priv)
{
	struct dn200_packet_attrs attr = { };

	attr.dst = priv->dev->dev_addr;
	return __dn200_test_loopback(priv, &attr);
}

static int dn200_test_phy_loopback(struct dn200_priv *priv)
{
	struct dn200_packet_attrs attr = { };
	int ret;

	if (!priv->dev->phydev)
		return -EOPNOTSUPP;

	attr.dst = priv->dev->dev_addr;
	ret = __dn200_test_loopback(priv, &attr);

	return ret;
}

static int dn200_test_mmc(struct dn200_priv *priv)
{
	struct dn200_counters *initial, *final;
	int ret;

	if (!priv->dma_cap.rmon)
		return -EOPNOTSUPP;

	initial = kzalloc(sizeof(*initial), GFP_KERNEL);
	if (!initial)
		return -ENOMEM;

	final = kzalloc(sizeof(*final), GFP_KERNEL);
	if (!final) {
		ret = -ENOMEM;
		goto out_free_initial;
	}
	memset(initial, 0, sizeof(*initial));
	memset(final, 0, sizeof(*final));
	/* Save previous results into internal struct */
	dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);
	memcpy(initial, &priv->mmc, sizeof(*initial));

	ret = dn200_test_mac_loopback(priv);
	if (ret)
		goto out_free_final;

	/* These will be loopback results so no need to save them */
	dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);
	memcpy(final, &priv->mmc, sizeof(*final));
	/* The number of MMC counters available depends on HW configuration
	 * so we just use this one to validate the feature. I hope there is
	 * not a version without this counter.
	 */
	if (final->mmc_tx_framecount_g <= initial->mmc_tx_framecount_g) {
		ret = -EINVAL;
		goto out_free_final;
	}

out_free_final:
	kfree(final);
out_free_initial:
	kfree(initial);
	return ret;
}

static int dn200_filter_check(struct dn200_priv *priv)
{
	if (!(priv->dev->flags & IFF_PROMISC))
		return 0;

	netdev_warn(priv->dev, "Test can't be run in promiscuous mode!\n");
	return -EOPNOTSUPP;
}

static bool dn200_hash_check(struct dn200_priv *priv, unsigned char *addr)
{
	int mc_offset = 32 - priv->hw->mcast_bits_log2;
	struct netdev_hw_addr *ha;
	u32 hash, hash_nr;

	/* First compute the hash for desired addr */
	hash = bitrev32(~crc32_le(~0, addr, 6)) >> mc_offset;
	hash_nr = hash >> 5;
	hash = 1 << (hash & 0x1f);

	/* Now, check if it collides with any existing one */
	netdev_for_each_mc_addr(ha, priv->dev) {
		u32 nr =
		    bitrev32(~crc32_le(~0, ha->addr, ETH_ALEN)) >> mc_offset;
		if (((nr >> 5) == hash_nr) && ((1 << (nr & 0x1f)) == hash))
			return false;
	}

	/* No collisions, address is good to go */
	return true;
}

static bool dn200_perfect_check(struct dn200_priv *priv, unsigned char *addr)
{
	struct netdev_hw_addr *ha;

	/* Check if it collides with any existing one */
	netdev_for_each_uc_addr(ha, priv->dev) {
		if (!memcmp(ha->addr, addr, ETH_ALEN))
			return false;
	}

	/* No collisions, address is good to go */
	return true;
}

static int dn200_test_hfilt(struct dn200_priv *priv)
{
	unsigned char gd_addr[ETH_ALEN] = { 0xf1, 0xee, 0xdd, 0xcc, 0xbb, 0xaa };
	unsigned char bd_addr[ETH_ALEN] = { 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct dn200_packet_attrs attr = { };
	int ret, tries = 256;

	ret = dn200_filter_check(priv);
	if (ret)
		return ret;

	if (netdev_mc_count(priv->dev) >= priv->hw->multicast_filter_bins)
		return -EOPNOTSUPP;

	while (--tries) {
		/* We only need to check the bd_addr for collisions */
		bd_addr[ETH_ALEN - 1] = tries;
		if (dn200_hash_check(priv, bd_addr))
			break;
	}
	if (!tries)
		return -EOPNOTSUPP;

	ret = dev_mc_add(priv->dev, gd_addr);
	if (ret)
		return ret;
	usleep_range(10000, 20000);
	attr.dst = gd_addr;

	/* Shall receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	if (ret)
		goto cleanup;

	attr.dst = bd_addr;

	/* Shall NOT receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	ret = ret ? 0 : -EINVAL;

cleanup:
	dev_mc_del(priv->dev, gd_addr);
	usleep_range(10000, 20000);
	return ret;
}

static int dn200_test_pfilt(struct dn200_priv *priv)
{
	unsigned char gd_addr[ETH_ALEN] = { 0xf0, 0x01, 0x44, 0x55, 0x66, 0x77 };
	unsigned char bd_addr[ETH_ALEN] = { 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct dn200_packet_attrs attr = { };
	int ret, tries = 256;

	if (dn200_filter_check(priv))
		return -EOPNOTSUPP;
	if (netdev_uc_count(priv->dev) >= priv->hw->unicast_filter_entries)
		return -EOPNOTSUPP;

	while (--tries) {
		/* We only need to check the bd_addr for collisions */
		bd_addr[ETH_ALEN - 1] = tries;
		if (dn200_perfect_check(priv, bd_addr))
			break;
	}

	if (!tries)
		return -EOPNOTSUPP;

	ret = dev_uc_add(priv->dev, gd_addr);
	if (ret)
		return ret;
	usleep_range(10000, 20000);
	attr.dst = gd_addr;

	/* Shall receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	if (ret)
		goto cleanup;

	attr.dst = bd_addr;

	/* Shall NOT receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	ret = ret ? 0 : -EINVAL;

cleanup:
	dev_uc_del(priv->dev, gd_addr);
	usleep_range(10000, 20000);
	return ret;
}

static int dn200_test_mcfilt(struct dn200_priv *priv)
{
	unsigned char uc_addr[ETH_ALEN] = { 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff };
	unsigned char mc_addr[ETH_ALEN] = { 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct dn200_packet_attrs attr = { };
	int ret, tries = 256;

	if (dn200_filter_check(priv))
		return -EOPNOTSUPP;
	if (netdev_uc_count(priv->dev) >= priv->hw->unicast_filter_entries)
		return -EOPNOTSUPP;
	if (netdev_mc_count(priv->dev) >= priv->hw->multicast_filter_bins)
		return -EOPNOTSUPP;
	while (--tries) {
		/* We only need to check the mc_addr for collisions */
		mc_addr[ETH_ALEN - 1] = tries;
		if (dn200_hash_check(priv, mc_addr))
			break;
	}
	if (!tries)
		return -EOPNOTSUPP;

	ret = dev_uc_add(priv->dev, uc_addr);
	if (ret)
		return ret;
	usleep_range(10000, 20000);
	attr.dst = uc_addr;

	/* Shall receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	if (ret)
		goto cleanup;

	attr.dst = mc_addr;

	/* Shall NOT receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	ret = ret ? 0 : -EINVAL;

cleanup:
	dev_uc_del(priv->dev, uc_addr);
	usleep_range(10000, 20000);
	return ret;
}

static int dn200_test_ucfilt(struct dn200_priv *priv)
{
	unsigned char uc_addr[ETH_ALEN] = { 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff };
	unsigned char mc_addr[ETH_ALEN] = { 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct dn200_packet_attrs attr = { };
	int ret, tries = 256;

	if (dn200_filter_check(priv))
		return -EOPNOTSUPP;
	if (netdev_uc_count(priv->dev) >= priv->hw->unicast_filter_entries)
		return -EOPNOTSUPP;
	if (netdev_mc_count(priv->dev) >= priv->hw->multicast_filter_bins)
		return -EOPNOTSUPP;

	while (--tries) {
		/* We only need to check the uc_addr for collisions */
		uc_addr[ETH_ALEN - 1] = tries;
		if (dn200_perfect_check(priv, uc_addr))
			break;
	}

	if (!tries)
		return -EOPNOTSUPP;

	ret = dev_mc_add(priv->dev, mc_addr);
	if (ret)
		return ret;
	usleep_range(10000, 20000);
	attr.dst = mc_addr;

	/* Shall receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	if (ret)
		goto cleanup;

	attr.dst = uc_addr;

	/* Shall NOT receive packet */
	ret = __dn200_test_loopback(priv, &attr);
	ret = ret ? 0 : -EINVAL;

cleanup:
	dev_mc_del(priv->dev, mc_addr);
	usleep_range(10000, 20000);
	return ret;
}
static inline bool dn200_napi_reschedule(struct napi_struct *napi)
{
	if (napi_schedule_prep(napi)) {
		__napi_schedule(napi);
		return true;
	}
	return false;
}

static int dn200_test_rss(struct dn200_priv *priv)
{
	struct dn200_packet_attrs attr = { };

	if (!priv->dma_cap.rssen || !priv->rss.enable)
		return -EOPNOTSUPP;

	attr.src = priv->dev->dev_addr;
	attr.dst = priv->dev->dev_addr;
	attr.ip_dst = 0x728A86D7;
	attr.ip_src = 0x728A86D8;
	attr.exp_hash = true;
	return __dn200_test_loopback(priv, &attr);
}


static int dn200_test_desc_sar(struct dn200_priv *priv)
{
	unsigned char src[ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	struct dn200_packet_attrs attr = { };
	int ret;

	if (!priv->dma_cap.vlins)
		return -EOPNOTSUPP;

	attr.sarc = true;
	attr.src = src;
	attr.dst = priv->dev->dev_addr;

	priv->sarc_type = 0x2;

	ret = __dn200_test_loopback(priv, &attr);

	priv->sarc_type = 0x0;
	return ret;
}

static int dn200_test_reg_sar(struct dn200_priv *priv)
{
	unsigned char src[ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	struct dn200_packet_attrs attr = { };
	int ret;

	if (!priv->dma_cap.vlins)
		return -EOPNOTSUPP;

	attr.sarc = true;
	attr.src = src;
	attr.dst = priv->dev->dev_addr;

	if (dn200_sarc_configure(priv, priv->ioaddr, 0x3))
		return -EOPNOTSUPP;

	ret = __dn200_test_loopback(priv, &attr);

	dn200_sarc_configure(priv, priv->ioaddr, 0x0);
	return ret;
}


static int dn200_test_arp_validate(struct sk_buff *skb,
				   struct net_device *ndev,
				   struct packet_type *pt,
				   struct net_device *orig_ndev)
{
	struct dn200_test_priv *tpriv = pt->af_packet_priv;
	struct ethhdr *ehdr;
	struct arphdr *ahdr;

	ehdr = (struct ethhdr *)skb_mac_header(skb);
	if (!ether_addr_equal_unaligned(ehdr->h_dest, tpriv->packet->src))
		goto out;

	ahdr = arp_hdr(skb);
	if (ahdr->ar_op != htons(ARPOP_REPLY))
		goto out;

	tpriv->ok = true;
	complete(&tpriv->comp);
out:
	kfree_skb(skb);
	return 0;
}

static int dn200_test_arpoffload(struct dn200_priv *priv)
{
	unsigned char src[ETH_ALEN] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
	unsigned char dst[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct dn200_packet_attrs attr = { };
	struct dn200_test_priv *tpriv;
	struct sk_buff *skb = NULL;
	u32 ip_addr = 0xdeadcafe;
	u32 ip_src = 0xdeadbeef;
	int ret;

	if (!priv->dma_cap.arpoffsel)
		return -EOPNOTSUPP;

	tpriv = kzalloc(sizeof(*tpriv), GFP_KERNEL);
	if (!tpriv)
		return -ENOMEM;

	tpriv->ok = false;
	init_completion(&tpriv->comp);

	tpriv->pt.type = htons(ETH_P_ARP);
	tpriv->pt.func = dn200_test_arp_validate;
	tpriv->pt.dev = priv->dev;
	tpriv->pt.af_packet_priv = tpriv;
	tpriv->packet = &attr;
	dev_add_pack(&tpriv->pt);

	attr.src = src;
	attr.ip_src = ip_src;
	attr.dst = dst;
	attr.ip_dst = ip_addr;

	skb = dn200_test_get_arp_skb(priv, &attr);
	if (!skb) {
		ret = -ENOMEM;
		goto cleanup;
	}

	ret = dn200_set_arp_offload(priv, priv->hw, true, ip_addr);
	if (ret)
		goto cleanup;

	ret = dev_set_promiscuity(priv->dev, 1);
	if (ret)
		goto cleanup;
	usleep_range(10000, 20000);
	ret = dev_direct_xmit(skb, 0);
	if (ret)
		goto cleanup_promisc;

	wait_for_completion_timeout(&tpriv->comp, DN200_LB_TIMEOUT);
	ret = tpriv->ok ? 0 : -ETIMEDOUT;

cleanup_promisc:
	dev_set_promiscuity(priv->dev, -1);
cleanup:
	dn200_set_arp_offload(priv, priv->hw, false, 0x0);
	dev_remove_pack(&tpriv->pt);
	usleep_range(10000, 20000);
	kfree(tpriv);
	return ret;
}

static int __dn200_test_jumbo(struct dn200_priv *priv, u16 queue)
{
	struct dn200_packet_attrs attr = { };
	int size = priv->dma_buf_sz;

	attr.dst = priv->dev->dev_addr;
	attr.max_size = size;
	attr.queue_mapping = queue;

	return __dn200_test_loopback(priv, &attr);
}

static int dn200_test_jumbo(struct dn200_priv *priv)
{
	return __dn200_test_jumbo(priv, 0);
}

static int dn200_test_mjumbo(struct dn200_priv *priv)
{
	u32 chan, tx_cnt = priv->plat->tx_queues_to_use;
	int ret;

	for (chan = 0; chan < tx_cnt; chan++) {
		ret = __dn200_test_jumbo(priv, chan);
		if (ret)
			return ret;
	}

	return 0;
}

#define DN200_LOOPBACK_NONE 0
#define DN200_LOOPBACK_MAC 1
#define DN200_LOOPBACK_PHY	2

#define PUREPF_XPCS BIT(0)
#define PUREPF_PHY BIT(1)
#define SRIOVPF_XPCS BIT(2)
#define SRIOVPF_PHY BIT(3)
#define SRIOVVF_XPCS BIT(4)
#define SRIOVVF_PHY BIT(5)

static const struct dn200_test {
	char name[ETH_GSTRING_LEN];
	int lb;
	int (*fn)(struct dn200_priv *priv);
	u16 type;
} dn200_selftests[] = {
	{
		.name = "MAC Loopback               ",
		.lb = DN200_LOOPBACK_MAC,
		.fn = dn200_test_mac_loopback,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
	{
		.name = "PHY Loopback               ",
		.lb = DN200_LOOPBACK_PHY, /* Test will handle it */
		.fn = dn200_test_phy_loopback,
		.type = PUREPF_PHY | SRIOVPF_PHY,
	},
	{
		.name = "MMC Counters               ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_mmc,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
	{
		.name = "Hash Filter MC             ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_hfilt,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
	{
		.name = "Perfect Filter UC          ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_pfilt,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
	{
		.name = "MC Filter                  ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_mcfilt,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
	{
		.name = "UC Filter                  ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_ucfilt,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	}, {
		.name = "RSS                        ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_rss,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_PHY | SRIOVPF_XPCS,
	},
	{
		.name = "SA Replacement (desc)      ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_desc_sar,
		.type = PUREPF_PHY | SRIOVPF_PHY,
	}, {
		.name = "SA Replacement (reg)       ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_reg_sar,
		.type = PUREPF_PHY | SRIOVPF_PHY,
	},
	{
		.name = "ARP Offload                ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_arpoffload,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	}, {
		.name = "Jumbo Frame                ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_jumbo,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	}, {
		.name = "Multichannel Jumbo         ",
		.lb = DN200_LOOPBACK_PHY,
		.fn = dn200_test_mjumbo,
		.type = PUREPF_XPCS | PUREPF_PHY | SRIOVPF_XPCS | SRIOVPF_PHY,
	},
};

static u16 dn200_seltftest_get_type(struct dn200_priv *priv)
{
	u16 type = 0;

	if (priv->dev->phydev) {
		if (PRIV_IS_VF(priv))
			type = SRIOVVF_PHY;
		else if (PRIV_SRIOV_SUPPORT(priv))
			type = SRIOVPF_PHY;
		else
			type = PUREPF_PHY;
	} else {
		if (PRIV_IS_VF(priv))
			type = SRIOVVF_XPCS;
		else if (PRIV_SRIOV_SUPPORT(priv))
			type = SRIOVPF_XPCS;
		else
			type = PUREPF_XPCS;
	}
	return type;
}

void dn200_selftest_run(struct net_device *dev,
			struct ethtool_test *etest, u64 *buf)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int count = dn200_selftest_get_count(priv);
	int i, ret, j = 0;
	u16 type = dn200_seltftest_get_type(priv);
	u16 val = 0;

	memset(buf, 0, sizeof(*buf) * count);
	dn200_test_next_id = 0;
	if (PRIV_IS_VF(priv)) {
		netdev_err(priv->dev, "VF self tests are not supported\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	if (etest->flags != ETH_TEST_FL_OFFLINE) {
		/*phyloopback needs on line */
		if (type & dn200_selftests[1].type) {
			netdev_err(priv->dev,
				   "Only offline tests are supported\n");
			etest->flags |= ETH_TEST_FL_FAILED;
			return;
		}

	} else if (!netif_carrier_ok(dev)) {
		netdev_err(priv->dev, "You need valid Link to execute tests\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	/* Wait for queues drain */
	msleep(200);
	/*mac selftest first*/
	ret = 0;
	ret = dn200_set_mac_loopback(priv, priv->ioaddr, true);
	if (ret) {
		netdev_err(priv->dev, "Loopback is not supported\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}
	for (i = 0; i < ARRAY_SIZE(dn200_selftests); i++) {
		if (dn200_selftests[i].lb != DN200_LOOPBACK_MAC)
			continue;
		if (type & dn200_selftests[i].type) {
			ret = dn200_selftests[i].fn(priv);
			if (ret && (ret != -EOPNOTSUPP))
				etest->flags |= ETH_TEST_FL_FAILED;
			buf[j] = ret;
			j++;
		}
	}
	ret = dn200_set_mac_loopback(priv, priv->ioaddr, false);
	if (ret) {
		netdev_err(priv->dev, "Loopback is not supported\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}
	/*phy selftest*/

	if (dev->phydev)
		priv->plat_ex->phy_info->phy_loopback_flag = true;

	if (dev->phydev) {
		val = phy_read(dev->phydev, MII_BMCR);
		ret = phy_write(dev->phydev, MII_BMCR, 0x4140);
		msleep(2000);
		if (ret)
			ret = dn200_set_mac_loopback(priv, priv->ioaddr, true);
	} else {
		ret = dn200_set_mac_loopback(priv, priv->ioaddr, true);
	}
	if (ret) {
		netdev_err(priv->dev, "Loopback is not supported\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	for (i = 0; i < ARRAY_SIZE(dn200_selftests); i++) {
		ret = 0;
		if (dn200_selftests[i].lb != DN200_LOOPBACK_PHY)
			continue;
		/*
		 * First tests will always be MAC / PHY loobpack. If any of
		 * them is not supported we abort earlier.
		 */
		if (type & dn200_selftests[i].type) {
			ret = dn200_selftests[i].fn(priv);
			if (ret && (ret != -EOPNOTSUPP))
				etest->flags |= ETH_TEST_FL_FAILED;
			buf[j] = ret;
			j++;
		}
	}

	if (dev->phydev) {
		ret = phy_write(dev->phydev, MII_BMCR, val | BMCR_RESET);
		if (ret)
			ret = dn200_set_mac_loopback(priv, priv->ioaddr, false);
	} else {
		ret = dn200_set_mac_loopback(priv, priv->ioaddr, false);
	}

	if (ret) {
		netdev_err(priv->dev, "Loopback is not supported\n");
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	if (dev->phydev) {
		usleep_range(3000000, 4000000);
		priv->plat_ex->phy_info->phy_loopback_flag = false;
	}
}

void dn200_selftest_get_strings(struct dn200_priv *priv, u8 *data)
{
	u8 *p = data;
	int i;
	int j = 0;
	u16 type = dn200_seltftest_get_type(priv);

	for (i = 0; i < ARRAY_SIZE(dn200_selftests); i++) {
		if (type & dn200_selftests[i].type) {
			snprintf(p, ETH_GSTRING_LEN, "%2d. %s", j + 1,
				 dn200_selftests[i].name);
			p += ETH_GSTRING_LEN;
			j++;
		}
	}
}

int dn200_selftest_get_count(struct dn200_priv *priv)
{
	int i, j = 0;
	u16 type = dn200_seltftest_get_type(priv);

	for (i = 0; i < ARRAY_SIZE(dn200_selftests); i++) {
		if (type & dn200_selftests[i].type)
			j++;
	}
	return j;
}
