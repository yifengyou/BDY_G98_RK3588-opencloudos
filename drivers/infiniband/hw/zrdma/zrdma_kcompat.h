/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZRDMA_KCOMPAT_H
#define ZRDMA_KCOMPAT_H

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif
#endif
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/mii.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/hugetlb.h>
#include <asm/io.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <net/ipv6.h>
#include <net/ip6_route.h>
#include <net/route.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include <linux/kconfig.h>
#include <net/secure_seq.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include "linux_kcompat.h"

#ifndef IB_QP_ATTR_STANDARD_BITS
#define IB_QP_ATTR_STANDARD_BITS GENMASK(20, 0)
#endif

#ifdef USE_KMAP
#define kmap_local_page kmap
#if defined(__OFED_BUILD__) && !(KERNEL_VERSION(5, 14, 0) == LINUX_VERSION_CODE)
#define kunmap_local(sq_base) kunmap(iwqp->page)
#endif
#endif

#define ZXDH_MTU_HEADER_RSV 102

// #ifdef IB_IW_PKEY
// static inline int zxdh_iw_query_pkey(struct ib_device *ibdev, u8 port,
// 				     u16 index, u16 *pkey)
// {
// 	*pkey = 0;
// 	return 0;
// }
// #endif
/*******************************************************************************/
struct zxdh_mr;
struct zxdh_cq;
struct zxdh_cq_buf;
struct zxdh_ucontext;
u32 zxdh_create_stag(struct zxdh_device *iwdev);
void zxdh_free_stag(struct zxdh_device *iwdev, u32 stag);
int zxdh_hw_alloc_mw(struct zxdh_device *iwdev, struct zxdh_mr *iwmr);
void zxdh_cq_free_rsrc(struct zxdh_pci_f *rf, struct zxdh_cq *iwcq);
int zxdh_process_resize_list(struct zxdh_cq *iwcq, struct zxdh_device *iwdev,
			     struct zxdh_cq_buf *lcqe_buf);
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE || defined(GET_ETH_SPEED_V1)
int zxdh_get_eth_speed(struct ib_device *dev, struct net_device *netdev,
		       u32 port_num, u16 *speed, u8 *width);
#elif KERNEL_VERSION(5, 4, 195) == LINUX_VERSION_CODE
#ifdef __OFED_23_10__
int zxdh_get_eth_speed(struct ib_device *dev, struct net_device *netdev,
		       u32 port_num, u16 *speed, u8 *width);
#else
int zxdh_get_eth_speed(struct ib_device *dev, struct net_device *netdev,
		       u32 port_num, u8 *speed, u8 *width);
#endif
#else
int zxdh_get_eth_speed(struct ib_device *dev, struct net_device *netdev,
		       u32 port_num, u8 *speed, u8 *width);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#define ZXDH_SET_UDP_SPORT_BYFLOW_LABLE
#endif

#ifdef Z_DH_DEBUG
#endif /* Z_DH_DEBUG */

#define kc_set_driver_id(x)
/*****************************************************************************/

/*********************************************************/
#ifndef ether_addr_copy
#define ether_addr_copy(mac_addr, new_mac_addr) \
	memcpy(mac_addr, new_mac_addr, ETH_ALEN)
#endif

#ifndef ether_addr_cmp
#define ether_addr_cmp(mac_addr, new_mac_addr) \
	memcmp(mac_addr, new_mac_addr, ETH_ALEN)
#endif

#ifndef eth_zero_addr
#define eth_zero_addr(mac_addr) memset(mac_addr, 0x00, ETH_ALEN)
#endif

#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
#define zxdh_for_each_ipv6_addr(ifp, tmp, idev) \
	list_for_each_entry_safe(ifp, tmp, &idev->addr_list, if_list)
#else
#define zxdh_for_each_ipv6_addr(ifp, tmp, idev) \
	for (ifp = idev->addr_list; ifp != NULL; ifp = ifp->if_next)
#endif /* >= 2.6.35 */

#ifdef IB_FW_VERSION_NAME_MAX
void zxdh_get_dev_fw_str(struct ib_device *dev, char *str);
#else
void zxdh_get_dev_fw_str(struct ib_device *dev, char *str, size_t str_len);
#endif /* IB_FW_VERSION_NAME_MAX */

/*****************************************************************************/
#ifdef CREATE_AH_VER_5
int zxdh_create_ah_v2(struct ib_ah *ib_ah, struct rdma_ah_attr *attr, u32 flags,
		      struct ib_udata *udata);
int zxdh_create_ah(struct ib_ah *ibah, struct rdma_ah_init_attr *attr,
		   struct ib_udata *udata);
#endif

#ifdef DESTROY_AH_VER_4
int zxdh_destroy_ah(struct ib_ah *ibah, u32 ah_flags);
#endif

#ifdef CREATE_CQ_VER_4
int zxdh_create_cq(struct ib_cq *ibcq, const struct ib_cq_init_attr *attr,
		   struct uverbs_attr_bundle *attrs);
#elif defined(CREATE_CQ_VER_3)
int zxdh_create_cq(struct ib_cq *ibcq, const struct ib_cq_init_attr *attr,
		   struct ib_udata *udata);
#endif

/* functions called by zxdh_create_qp and zxdh_free_qp_rsrc */
int zxdh_validate_qp_attrs(struct ib_qp_init_attr *init_attr,
			   struct zxdh_device *iwdev);

void zxdh_setup_virt_qp(struct zxdh_device *iwdev, struct zxdh_qp *iwqp,
			struct zxdh_qp_init_info *init_info);

int zxdh_setup_kmode_qp(struct zxdh_device *iwdev, struct zxdh_qp *iwqp,
			struct zxdh_qp_init_info *info,
			struct ib_qp_init_attr *init_attr);

void zxdh_roce_fill_and_set_qpctx_info(struct zxdh_qp *iwqp,
				       struct zxdh_qp_host_ctx_info *ctx_info);

int zxdh_cqp_create_qp_cmd(struct zxdh_qp *iwqp);

void zxdh_free_qp_rsrc(struct zxdh_qp *iwqp);

#ifdef ZXDH_ALLOC_MW_VER_2
int zxdh_alloc_mw(struct ib_mw *ibmw, struct ib_udata *udata);
#endif

#ifdef CREATE_QP_VER_2
int zxdh_create_qp(struct ib_qp *ibqp, struct ib_qp_init_attr *init_attr,
		   struct ib_udata *udata);
#endif

int zxdh_hw_alloc_stag(struct zxdh_device *iwdev, struct zxdh_mr *iwmr);

#ifdef ZXDH_ALLOC_MR_VER_0
struct ib_mr *zxdh_alloc_mr(struct ib_pd *pd, enum ib_mr_type mr_type,
			    u32 max_num_sg);
#endif

#ifdef ALLOC_UCONTEXT_VER_2
int zxdh_alloc_ucontext(struct ib_ucontext *uctx, struct ib_udata *udata);
#endif

#ifdef DEALLOC_UCONTEXT_VER_2
void zxdh_dealloc_ucontext(struct ib_ucontext *context);
#endif

#if defined(ETHER_COPY_VER_1)
void zxdh_ether_copy(u8 *dmac, struct ib_ah_attr *attr);
#endif

#ifdef ALLOC_PD_VER_3
int zxdh_alloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#endif

#ifdef DEALLOC_PD_VER_4
int zxdh_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
#endif

#ifdef ZXDH_DESTROY_CQ_VER_4
int zxdh_destroy_cq(struct ib_cq *ib_cq, struct ib_udata *udata);
#endif

#ifdef DESTROY_QP_VER_2
int zxdh_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata);
#define kc_zxdh_destroy_qp(ibqp, udata) zxdh_destroy_qp(ibqp, udata)
#endif

#ifdef DEREG_MR_VER_2
int zxdh_dereg_mr(struct ib_mr *ib_mr, struct ib_udata *udata);
#endif

int zxdh_hwdereg_mr(struct ib_mr *ib_mr);

#ifdef REREG_MR_VER_2
struct ib_mr *zxdh_rereg_user_mr(struct ib_mr *ib_mr, int flags, u64 start,
				 u64 len, u64 virt, int new_access,
				 struct ib_pd *new_pd, struct ib_udata *udata);
#endif

int zxdh_hwreg_mr(struct zxdh_device *iwdev, struct zxdh_mr *iwmr, u16 access);

struct ib_mr *zxdh_rereg_mr_trans(struct zxdh_mr *iwmr, u64 start, u64 len,
				  u64 virt, struct ib_udata *udata);

struct zxdh_pbl *zxdh_get_pbl(unsigned long va, struct list_head *pbl_list);

void zxdh_copy_user_pgaddrs(struct zxdh_mr *iwmr, u64 *pblpar,
			    struct zxdh_pble_info **pbleinfo,
			    enum zxdh_pble_level level, bool use_pbles,
			    bool pble_type);

void zxdh_del_memlist(struct zxdh_mr *iwmr, struct zxdh_ucontext *ucontext);

void zxdh_unregister_rdma_device(struct ib_device *ibdev);
#ifndef RDMA_MMAP_DB_SUPPORT
int rdma_user_mmap_io(struct ib_ucontext *ucontext, struct vm_area_struct *vma,
		      unsigned long pfn, unsigned long size, pgprot_t prot);
#endif
void zxdh_disassociate_ucontext(struct ib_ucontext *context);
int kc_zxdh_set_roce_cm_info(struct zxdh_qp *iwqp, struct ib_qp_attr *attr,
			     u16 *vlan_id);
int kc_zxdh_create_sysfs_file(struct ib_device *ibdev);
struct zxdh_device *kc_zxdh_get_device(struct net_device *netdev);
void kc_zxdh_put_device(struct zxdh_device *iwdev);

#ifdef QUERY_GID_ROCE_V2
int zxdh_query_gid_roce(struct ib_device *ibdev, u32 port, int index,
			union ib_gid *gid);
#endif

#ifdef MODIFY_PORT_V2
int zxdh_modify_port(struct ib_device *ibdev, u32 port, int mask,
		     struct ib_port_modify *props);
#endif

#ifdef QUERY_PKEY_V2
int zxdh_query_pkey(struct ib_device *ibdev, u32 port, u16 index, u16 *pkey);
#endif

#ifdef ROCE_PORT_IMMUTABLE_V2
int zxdh_roce_port_immutable(struct ib_device *ibdev, u32 port_num,
			     struct ib_port_immutable *immutable);
#endif

#ifdef IW_PORT_IMMUTABLE_V2
int zxdh_iw_port_immutable(struct ib_device *ibdev, u32 port_num,
			   struct ib_port_immutable *immutable);
#endif

#ifdef ALLOC_HW_STATS_V3
struct rdma_hw_stats *zxdh_alloc_hw_port_stats(struct ib_device *ibdev,
					       u32 port_num);
#endif

#ifdef GET_HW_STATS_V2
int zxdh_get_hw_stats(struct ib_device *ibdev, struct rdma_hw_stats *stats,
		      u32 port_num, int index);
#endif

#ifdef PROCESS_MAD_VER_4
int zxdh_process_mad(struct ib_device *ibdev, int mad_flags, u8 port_num,
		     const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		     const struct ib_mad *in_mad, struct ib_mad *out_mad,
		     size_t *out_mad_size, u16 *out_mad_pkey_index);
#endif
#ifdef PROCESS_MAD_VER_3
int zxdh_process_mad(struct ib_device *ibdev, int mad_flags, u32 port_num,
		     const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		     const struct ib_mad *in_mad, struct ib_mad *out_mad,
		     size_t *out_mad_size, u16 *out_mad_pkey_index);
#endif

#ifdef QUERY_GID_V2
int zxdh_query_gid(struct ib_device *ibdev, u32 port, int index,
		   union ib_gid *gid);
#endif

int zxdh_query_qpc(struct zxdh_sc_qp *qp, struct zxdh_dma_mem *qpc_buf);
void zxdh_print_hw_qpc(__le64 *qp_ctx);

#ifdef GET_LINK_LAYER_V2
enum rdma_link_layer zxdh_get_link_layer(struct ib_device *ibdev, u32 port_num);
#endif

#ifdef QUERY_PORT_V2
int zxdh_query_port(struct ib_device *ibdev, u32 port,
		    struct ib_port_attr *props);
#endif

void zxdh_clean_cqes(struct zxdh_qp *iwqp, struct zxdh_cq *iwcq);
#ifndef NETDEV_TO_IBDEV_SUPPORT
struct ib_device *ib_device_get_by_netdev(struct net_device *ndev,
					  int driver_id);
void ib_unregister_device_put(struct ib_device *device);
#endif
struct zxdh_device *zxdh_device_get_by_source_netdev(struct net_device *netdev);
#if defined(DEREG_MR_VER_2) && defined(HAS_IB_SET_DEVICE_OP)
#define kc_free_lsmm_dereg_mr(iwdev, iwqp) \
	((iwdev)->ibdev.ops.dereg_mr((iwqp)->lsmm_mr, NULL))
#elif defined(DEREG_MR_VER_2) && !defined(HAS_IB_SET_DEVICE_OP)
#define kc_free_lsmm_dereg_mr(iwdev, iwqp) \
	((iwdev)->ibdev.dereg_mr((iwqp)->lsmm_mr, NULL))
#elif !defined(DEREG_MR_VER_2) && defined(HAS_IB_SET_DEVICE_OP)
#define kc_free_lsmm_dereg_mr(iwdev, iwqp) \
	((iwdev)->ibdev.ops.dereg_mr((iwqp)->lsmm_mr))
#else
#define kc_free_lsmm_dereg_mr(iwdev, iwqp) \
	((iwdev)->ibdev.dereg_mr((iwqp)->lsmm_mr))
#endif

static inline int cq_validate_flags(u32 flags, u8 hw_rev)
{
	/* GEN1 does not support CQ create flags */
	if (hw_rev == ZXDH_GEN_1)
		return flags ? -EOPNOTSUPP : 0;

	return flags & ~IB_UVERBS_CQ_FLAGS_TIMESTAMP_COMPLETION ? -EOPNOTSUPP :
									0;
}

static inline u64 *zxdh_next_pbl_addr(u64 *pbl, struct zxdh_pble_info **pinfo,
				      u32 *idx, u32 *l2_pinfo_cnt)
{
	*idx += 1;
	if (!(*pinfo) || *idx != (*pinfo)->cnt)
		return ++pbl;
	*idx = 0;
	(*pinfo)++;
	*l2_pinfo_cnt += 1;
	return (*pinfo)->addr;
}

/* Introduced in this series https://lore.kernel.org/linux-rdma/0-v2-270386b7e60b+28f4-umem_1_jgg@nvidia.com/
 * An zrdma version helper doing same for older functions with difference that iova is passed in
 * as opposed to derived from umem->iova.
 */
static inline size_t zxdh_ib_umem_num_dma_blocks(struct ib_umem *umem,
						 unsigned long pgsz, u64 iova)
{
	/* some older OFED distros do not have ALIGN_DOWN */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) __ALIGN_KERNEL((x) - ((a)-1), (a))
#endif

	return (size_t)((ALIGN(iova + umem->length, pgsz) -
			 ALIGN_DOWN(iova, pgsz))) /
	       pgsz;
}

int zxdh_fill_qpc(struct zxdh_sc_dev *dev, u32 qpn, struct zxdh_dma_mem *qpc_buf);
int zxdh_fill_cqc(struct zxdh_sc_dev *dev, u32 cqn, struct zxdh_dma_mem *cqc_buf);
int zxdh_fill_ceqc(struct zxdh_sc_dev *dev, u32 ceqn, struct zxdh_dma_mem *ceqc_buf);
int zxdh_fill_aeqc(struct zxdh_sc_dev *dev, struct zxdh_dma_mem *aeqc_buf);
int zxdh_fill_srqc(struct zxdh_sc_dev *dev, u32 srqn, struct zxdh_dma_mem *srqc_buf);

enum ib_mtu zxdh_mtu_int_to_enum(int mtu);

/* UAPI */
#if KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE || defined(KYLIN_V10_4)
#define ZXDH_UAPI_DEF
#endif

#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define ZXDH_UAPI_DEF
#endif

/* ZXDH_SW_RDMA_DEVICE */
#if (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE)
#define zxdh_rdma_device_to_drv_device(device, ibdev) \
	rdma_device_to_drv_device(device, struct zxdh_device, ibdev)
#else
#define zxdh_rdma_device_to_drv_device(device, ibdev) \
	container_of(device, struct zxdh_device, ibdev.dev)
#endif

#if (KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE)
#define IB_READ_GID_ATTRIBUTE_NETDEVICE_NOT_DEFINE
#endif

#endif /* ZRDMA_KCOMPAT_H_ */
