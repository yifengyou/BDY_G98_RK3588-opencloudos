/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _YK3_KCOMPAT_H_
#define _YK3_KCOMPAT_H_

#include <linux/device.h>
#include <linux/idr.h>
#include <linux/etherdevice.h>
#include "kcompat_config.h"
#define YK3_HAVE_GUP_RCLONG_NOTASK_COMBINEDFLAGS yes
#ifndef YK3_HAVE_ARRAY_INDEX_NOSPEC
#ifndef array_index_mask_nospec
static inline unsigned long array_index_mask_nospec(unsigned long index,
						    unsigned long size)
{
	OPTIMIZER_HIDE_VAR(index);
	return ~(long)(index | (size - 1UL - index)) >> (BITS_PER_LONG - 1);
}
#endif
#define array_index_nospec(index, size)					\
({									\
	typeof(index) _i = (index);					\
	typeof(size) _s = (size);					\
	unsigned long _mask = array_index_mask_nospec(_i, _s);		\
									\
	BUILD_BUG_ON(sizeof(_i) > sizeof(long));			\
	BUILD_BUG_ON(sizeof(_s) > sizeof(long));			\
									\
	(typeof(_i))(_i & _mask);					\
})
#else
#include <linux/nospec.h>
#endif /* YK3_HAVE_ARRAY_INDEX_NOSPEC */

#ifndef YK3_HAVE_TIMER_SETUP
#define TIMER_DATA_TYPE         unsigned long

#define timer_setup(__timer, callback, flags)                             \
{ \
	void (*function)(unsigned long data);\
	struct timer_list *__temp_timer1 = (__timer);\
	TIMER_DATA_TYPE __temp_timer2 = (TIMER_DATA_TYPE)(__temp_timer1); \
	function = (void (*)(unsigned long))callback;\
	setup_timer(__temp_timer1, function, __temp_timer2); \
}
#endif /* YK3_HAVE_TIMER_SETUP */

#ifndef YK3_HAVE_TIMER_DELETE_SYNC
#define timer_delete_sync(t) del_timer_sync(t)
#endif

#ifndef YK3_HAVE_BEFOR_JIFFIES66
#define time_is_after_jiffies64(a) time_before64(get_jiffies_64(), a)
#define time_is_before_jiffies64(a) time_after64(get_jiffies_64(), a)
#endif /* YK3_HAVE_BEFOR_JIFFIES66 */

/* If NETIF_F_GSO_UDP_L4 is not supported in the kernel,
 * do not configure NETIF_F_GSO_UDP_L4 feature
 */
#ifndef YK3_HAVE_NETIF_F_GSO_UDP_L4
#define NETIF_F_GSO_UDP_L4 0
#endif /* YK3_HAVE_NETIF_F_GSO_UDP_L4 */

#ifndef YK3_HAVE_ETH_TLEN
#define ETH_TLEN 2
#endif /* YK3_HAVE_ETH_TLEN */

#ifndef YK3_HAVE_ETH_GSTRINT_LEN
#define ETH_GSTRING_LEN 32
#endif /* YK3_HAVE_ETH_GSTRINT_LEN */

#ifndef YK3_HAVE_NETDEV_XMIT_MORE
#define netdev_xmit_more() (skb->xmit_more)
#endif /* YK3_HAVE_NETDEV_XMIT_MORE */

#ifndef YK3_HAVE_STRSCPY
#define strscpy strncpy
#endif /* YK3_HAVE_STRSCPY */

#ifndef YK3_HAVE_ETHTOOL_RXNFC
struct ethtool_rxnfc {
	__u32				cmd;
	__u32				flow_type;
	__u64				data;
	struct ethtool_rx_flow_spec	fs;
	union {
		__u32			rule_cnt;
		__u32			rss_context;
	};
	DECLARE_FLEX_ARRAY(__u32, rule_locs);
};
#endif /* YK3_HAVE_ETHTOOL_RXNFC */

#ifndef YK3_HAVE_AUXILIARY_BUS
typedef int (*match_non_const)(struct device *dev, void *data);

static inline struct device *
call_bus_find_device(struct bus_type *bus, struct device *start,
		     const void *data,
		     int (*match)(struct device *dev, const void *data))
{
#ifndef YK3_HAVE_BUS_FIND_DEVICE_TAKES_CONST
	/* Necessary for older distributions like rhel7, centos7 and
	 * unpatched kernels. (e.g. kernel 3.10, 4.15, 4.18)
	 */
	match_non_const match_ptr = (match_non_const)match;

	return bus_find_device(bus, start, (void *)data, match_ptr);
#else
	/* Path taken for newer kernels. (e.g. kernel >= 5.3) */
	return bus_find_device(bus, start, data, match);
#endif /* YK3_HAVE_BUS_FIND_DEVICE_TAKES_CONST */
}
#endif /* YK3_HAVE_AUXILIARY_BUS */

#ifndef YK3_HAVE_DEFINE_SEQ_ATTRIBUTE
#include <linux/fs.h>
#define DEFINE_SEQ_ATTRIBUTE(__name)					\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	int ret = seq_open(file, &__name ## _sops);			\
	if (!ret && inode->i_private) {					\
		struct seq_file *seq_f = file->private_data;		\
		seq_f->private = inode->i_private;			\
	}								\
	return ret;							\
}									\
									\
static const struct file_operations __name ## _fops = {			\
	.owner		= THIS_MODULE,					\
	.open		= __name ## _open,				\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= seq_release,					\
}

#define DEFINE_SHOW_ATTRIBUTE(__name)					\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, __name ## _show, inode->i_private);	\
}									\
									\
static const struct file_operations __name ## _fops = {			\
	.owner		= THIS_MODULE,					\
	.open		= __name ## _open,				\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
}

#define DEFINE_PROC_SHOW_ATTRIBUTE(__name)				\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, __name ## _show, pde_data(inode));	\
}									\
									\
static const struct proc_ops __name ## _proc_ops = {			\
	.proc_open	= __name ## _open,				\
	.proc_read	= seq_read,					\
	.proc_lseek	= seq_lseek,					\
	.proc_release	= single_release,				\
}
#endif

#if (!defined(YK3_HAVE_FLOW_OFFLOAD) || !defined(YK3_HAVE_FLOW_CLS_OFFLOAD))
#define YK3_TC_DISABLE
#endif
#if !defined(YK3_HAVE_FLOW_INDR_DEV_REGISTER)
#define YK3_TC_DISABLE
#endif

#ifdef YK3_HAVE_NDO_SETUP_TC
#if (defined(YK3_HAVE_TC_MQPRIO_OFFLOAD) || defined(YK3_HAVE_TC_MQPRIO_OFFLOAD_KYLIN))
#define YK3_MQPRIO_OFFLOAD
#endif
#endif

#include <linux/mm.h>
#include <linux/sched.h>

#ifndef FOLL_WRITE
#define FOLL_WRITE	0x01
#endif

#ifndef FOLL_FORCE
#define FOLL_FORCE	0x10
#endif

/* linux-5.6 have got pin_user_pages() */

#ifndef YK3_HAVE_GUP_HAS_PIN
static inline long
pin_user_pages(unsigned long start, unsigned long nr_pages,
	       unsigned int gup_flags, struct page **pages,
	       struct vm_area_struct **vmas)
{
  /* We support four get_user_pages() function prototypes here,
   * including an intermediate one that has one of the changes but not
   * the other, and we assume that intermediate case if the main three
   * are not defined:
   *
   * Pre-3.9: YK3_HAVE_GUP_RCINT_TASK_SEPARATEFLAGS
   * int get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
   *                    unsigned long start, int nr_pages, int write, int force,
   *                    struct page **pages, struct vm_area_struct **vmas);
   *
   * Pre-4.6.0: YK3_HAVE_GUP_RCLONG_TASK_SEPARATEFLAGS
   * long get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
   *                     unsigned long start, unsigned long nr_pages,
   *                     int write, int force, struct page **pages,
   *                     struct vm_area_struct **vmas);
   *
   * 4.4.(>=168): YK3_HAVE_GUP_RCLONG_TASK_COMBINEDFLAGS
   * long get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
   *                     unsigned long start, unsigned long nr_pages,
   *                     unsigned int gup_flags, struct page **pages,
   *                     struct vm_area_struct **vmas)
   *
   * Intermediate (up to 4.9.0): (would be YK3_HAVE_GUP_RCLONG_NOTASK_SEPARATEFLAGS)
   * long get_user_pages(unsigned long start, unsigned long nr_pages,
   *                     int write, int force, struct page **pages,
   *                     struct vm_area_struct **vmas);
   *
   * Post-4.9.0: YK3_HAVE_GUP_RCLONG_NOTASK_COMBINEDFLAGS
   * long get_user_pages(unsigned long start, unsigned long nr_pages,
   *                     unsigned int gup_flags, struct page **pages,
   *                     struct vm_area_struct **vmas);
   */
#ifdef YK3_HAVE_GUP_RCINT_TASK_SEPARATEFLAGS
	return (long)get_user_pages(current, current->mm, start, (int)nr_pages,
				    gup_flags & FOLL_WRITE, gup_flags & FOLL_FORCE, pages, vmas);
#elif defined YK3_HAVE_GUP_RCLONG_TASK_SEPARATEFLAGS
	return get_user_pages(current, current->mm, start, nr_pages, gup_flags & FOLL_WRITE,
			      gup_flags & FOLL_FORCE, pages, vmas);
#elif defined YK3_HAVE_GUP_RCLONG_TASK_COMBINEDFLAGS
	return get_user_pages(current, current->mm, start, nr_pages, gup_flags, pages, vmas);

#elif defined YK3_HAVE_GUP_RCLONG_NOTASK_COMBINEDFLAGS
	return get_user_pages(start, nr_pages, gup_flags, pages, vmas);
#else
	return get_user_pages(start, nr_pages, gup_flags & FOLL_WRITE,
			      gup_flags & FOLL_FORCE, pages, vmas);
#endif
}

static inline void unpin_user_page(struct page *page)
{
	put_page(page);
}
#endif /* YK3_HAVE_GUP_HAS_PIN */

/* Linux < 5.8 does not have mmap_write_lock() */
#ifndef YK3_HAVE_MMAP_LOCK_WRAPPERS
static inline void mmap_write_lock(struct mm_struct *mm)
{
	down_write(&mm->mmap_sem);
}

static inline void mmap_write_unlock(struct mm_struct *mm)
{
	up_write(&mm->mmap_sem);
}

static inline void mmap_read_lock(struct mm_struct *mm)
{
	down_read(&mm->mmap_sem);
}

static inline void mmap_read_unlock(struct mm_struct *mm)
{
	up_read(&mm->mmap_sem);
}
#endif

#ifndef	FLOW_RSS
#define FLOW_RSS		0x20000000
#endif

#ifndef YK3_HAVE_ETH_HW_ADDR_SET
static inline void eth_hw_addr_set(struct net_device *dev, const u8 *addr)
{
	ether_addr_copy(dev->dev_addr, addr);
}
#endif /* YK3_HAVE_ETH_HW_ADDR_SET */

#ifndef YK3_HAVE_ETHTOOL_MAC_STATS
struct ethtool_eth_mac_stats {
	__u64 frames_transmitted_ok;
	__u64 single_collision_frames;
	__u64 multiple_collision_frames;
	__u64 frames_received_ok;
	__u64 frame_check_sequence_errors;
	__u64 alignment_errors;
	__u64 octets_transmitted_ok;
	__u64 frames_with_deferred_xmissions;
	__u64 late_collisions;
	__u64 frames_aborted_due_to_xs_colls;
	__u64 frames_lost_due_to_int_mac_xmit_error;
	__u64 carrier_sense_errors;
	__u64 octets_received_ok;
	__u64 frames_lost_due_to_int_mac_rcv_error;
	__u64 multicast_frames_xmitted_ok;
	__u64 broadcast_frames_xmitted_ok;
	__u64 frames_with_excessive_deferral;
	__u64 multicast_frames_received_ok;
	__u64 broadcast_frames_received_ok;
	__u64 in_range_length_errors;
	__u64 out_of_range_length_field;
	__u64 frame_too_long_errors;
};
#endif /* YK3_HAVE_ETHTOOL_MAC_STATS */

#ifndef YK3_HAVE_BITOPS_FNS
static inline unsigned long fns(unsigned long word, unsigned int n)
{
	unsigned int bit;

	while (word) {
		bit = __ffs(word);
		if (n-- == 0)
			return bit;
		__clear_bit(bit, &word);
	}

	return BITS_PER_LONG;
}
#endif /* YK3_HAVE_BITOPS_FNS */

#ifndef YK3_HAVE_FIND_NTH_BIT
static inline unsigned long
__find_nth_bit(const unsigned long *addr, unsigned long size, unsigned long n)
{
	unsigned long sz = (size), nr = (n), idx = 0, w = 0, tmp = 0;

	for (idx = 0; (idx + 1) * BITS_PER_LONG <= sz; idx++) {
		if (idx * BITS_PER_LONG + nr >= sz)
			goto out;

		tmp = addr[idx];
		w = hweight_long(tmp);
		if (w > nr)
			goto found;

		nr -= w;
	}

	if (sz % BITS_PER_LONG)
		tmp = addr[idx] & BITMAP_LAST_WORD_MASK(sz);
found:
	sz = min(idx * BITS_PER_LONG + fns(tmp, nr), sz);
out:
	return sz;
}

static inline
unsigned long find_nth_bit(const unsigned long *addr, unsigned long size, unsigned long n)
{
	if (n >= size)
		return size;

	return __find_nth_bit(addr, size, n);
}
#endif /* YK3_HAVE_FIND_NTH_BIT */

#ifndef REFCOUNT_MAX
#define REFCOUNT_MAX		INT_MAX
#endif

#ifndef ETH_MODULE_SFF_8079
#define ETH_MODULE_SFF_8079		0x1
#endif
#ifndef ETH_MODULE_SFF_8079_LEN
#define ETH_MODULE_SFF_8079_LEN		256
#endif
#ifndef ETH_MODULE_SFF_8472
#define ETH_MODULE_SFF_8472		0x2
#endif
#ifndef ETH_MODULE_SFF_8472_LEN
#define ETH_MODULE_SFF_8472_LEN		512
#endif
#ifndef ETH_MODULE_SFF_8636
#define ETH_MODULE_SFF_8636		0x3
#endif
#ifndef ETH_MODULE_SFF_8636_LEN
#define ETH_MODULE_SFF_8636_LEN		256
#endif
#ifndef ETH_MODULE_SFF_8436
#define ETH_MODULE_SFF_8436		0x4
#endif
#ifndef ETH_MODULE_SFF_8436_LEN
#define ETH_MODULE_SFF_8436_LEN		256
#endif
#ifndef ETH_MODULE_SFF_8636_MAX_LEN
#define ETH_MODULE_SFF_8636_MAX_LEN     640
#endif
#ifndef ETH_MODULE_SFF_8436_MAX_LEN
#define ETH_MODULE_SFF_8436_MAX_LEN     640
#endif

#endif
