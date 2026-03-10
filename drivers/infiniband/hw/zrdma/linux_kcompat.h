/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef LINUX_KCOMPAT_H
#define LINUX_KCOMPAT_H

#define IB_DEV_OPS_FILL_ENTRY
#define IB_DEV_CAPS_VER_2
#define CREATE_AH_VER_5
#define PROCESS_MAD_VER_3
#define ZRDMA_CREATE_SRQ_VER_2
#define ZRDMA_DESTROY_SRQ_VER_3
#define DESTROY_AH_VER_4
#define CREATE_QP_VER_2
#define GLOBAL_QP_MEM
#define DESTROY_QP_VER_2
#define kc_zxdh_destroy_qp(ibqp, udata) zxdh_destroy_qp(ibqp, udata)
#define CREATE_CQ_VER_3
#define COPY_USER_PGADDR_VER_4
#define ALLOC_UCONTEXT_VER_2
#define DEALLOC_UCONTEXT_VER_2
#define DEALLOC_PD_VER_4
#define ALLOC_PD_VER_3
#define ALLOC_HW_STATS_STRUCT_V2
#define ALLOC_HW_STATS_V3
#define QUERY_GID_ROCE_V2
#define MODIFY_PORT_V2
#define QUERY_PKEY_V2
#define ROCE_PORT_IMMUTABLE_V2
#define GET_HW_STATS_V2
#define GET_LINK_LAYER_V2
#define IW_PORT_IMMUTABLE_V2
#define QUERY_GID_V2
#define QUERY_PORT_V2
#define GET_ETH_SPEED_AND_WIDTH_V2
#define RDMA_MMAP_DB_SUPPORT
#define RDMA_COPY_TO_STRUCT_OR_ZERO_SUPPORT
#define ZXDH_ALLOC_MW_VER_2
#define ZXDH_ALLOC_MR_VER_0
#define ZXDH_DESTROY_CQ_VER_4
#define set_max_sge(props, rf)                                            \
	do {                                                              \
		((props)->max_send_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
		((props)->max_recv_sge =                                  \
			 (rf)->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags); \
	} while (0)
#define kc_set_props_ip_gid_caps(props) ((props)->ip_gids = true)
#define kc_rdma_gid_attr_network_type(sgid_attr, gid_type, gid) \
	rdma_gid_attr_network_type(sgid_attr)
#define kc_deref_sgid_attr(sgid_attr) ((sgid_attr)->ndev)

#define kc_typeq_ib_wr const

#define kc_ib_register_device(device, name, dev) \
	ib_register_device(device, name, dev)
#define HAS_IB_SET_DEVICE_OP
#define kc_set_ibdev_add_del_gid(ibdev)
#define kc_ib_modify_qp_is_ok(cur_state, next_state, type, mask, ll) \
	ib_modify_qp_is_ok(cur_state, next_state, type, mask)
#define SET_BEST_PAGE_SZ_V2
#define kc_rdma_udata_to_drv_context(ibpd, udata) \
	rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#define USE_QP_ATTRS_STANDARD
#define NETDEV_TO_IBDEV_SUPPORT
#define IB_DEALLOC_DRIVER_SUPPORT
#define kc_get_ucontext(udata) \
	rdma_udata_to_drv_context(udata, struct zxdh_ucontext, ibucontext)
#define IN_IFADDR
int zxdh_alloc_ucontext(struct ib_ucontext *uctx, struct ib_udata *udata);
void zxdh_dealloc_ucontext(struct ib_ucontext *context);
int zxdh_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
int zxdh_alloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#define set_ibdev_dma_device(ibdev, dev)
#define ah_attr_to_dmac(attr) ((attr).roce.dmac)
#define SET_ROCE_CM_INFO_VER_3
#define IB_UMEM_GET_V3
#define DEREG_MR_VER_2
#define REREG_MR_VER_2

#endif /* LINUX_KCOMPAT_H */
