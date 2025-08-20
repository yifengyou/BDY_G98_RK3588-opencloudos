/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#ifndef __UDMA_JETTY_H__
#define __UDMA_JETTY_H__

#include "udma_common.h"

#define JETTY_CTX_JFRN_H_OFFSET 12

enum jetty_state {
	JETTY_RESET,
	JETTY_READY,
	JETTY_ERROR,
	JETTY_SUSPEND,
	STATE_NUM,
};

struct udma_jetty {
	struct ubcore_jetty ubcore_jetty;
	struct udma_jfr *jfr;
	struct udma_jetty_queue sq;
	uint64_t jetty_addr;
	refcount_t ae_refcount;
	struct completion ae_comp;
	bool pi_type;
	bool ue_rx_closed;
};

enum jetty_type {
	JETTY_RAW_OR_NIC,
	JETTY_UM,
	JETTY_RC,
	JETTY_RM,
	JETTY_TYPE_RESERVED,
};

struct udma_jetty_ctx {
	/* DW0 */
	uint32_t ta_timeout : 2;
	uint32_t rnr_retry_num : 3;
	uint32_t type : 3;
	uint32_t sqe_bb_shift : 4;
	uint32_t sl : 4;
	uint32_t state : 3;
	uint32_t jfs_mode : 1;
	uint32_t sqe_token_id_l : 12;
	/* DW1 */
	uint32_t sqe_token_id_h : 8;
	uint32_t err_mode : 1;
	uint32_t rsv : 1;
	uint32_t cmp_odr : 1;
	uint32_t rsv1 : 1;
	uint32_t sqe_base_addr_l : 20;
	/* DW2 */
	uint32_t sqe_base_addr_h;
	/* DW3 */
	uint32_t rsv2;
	/* DW4 */
	uint32_t tx_jfcn : 20;
	uint32_t jfrn_l : 12;
	/* DW5 */
	uint32_t jfrn_h : 8;
	uint32_t rsv3 : 4;
	uint32_t rx_jfcn : 20;
	/* DW6 */
	uint32_t seid_idx : 10;
	uint32_t pi_type : 1;
	uint32_t rsv4 : 21;
	/* DW7 */
	uint32_t user_data_l;
	/* DW8 */
	uint32_t user_data_h;
	/* DW9 */
	uint32_t sqe_position : 1;
	uint32_t sqe_pld_position : 1;
	uint32_t sqe_pld_tokenid : 20;
	uint32_t rsv5 : 10;
	/* DW10 */
	uint32_t tpn : 24;
	uint32_t rsv6 : 8;
	/* DW11 */
	uint32_t rmt_eid : 20;
	uint32_t rsv7 : 12;
	/* DW12 */
	uint32_t rmt_tokenid : 20;
	uint32_t rsv8 : 12;
	/* DW13 - DW15 */
	uint32_t rsv8_1[3];
	/* DW16 */
	uint32_t next_send_ssn : 16;
	uint32_t src_order_wqe : 16;
	/* DW17 */
	uint32_t src_order_ssn : 16;
	uint32_t src_order_sgme_cnt : 16;
	/* DW18 */
	uint32_t src_order_sgme_send_cnt : 16;
	uint32_t CI : 16;
	/* DW19 */
	uint32_t wqe_sgmt_send_cnt : 20;
	uint32_t src_order_wqebb_num : 4;
	uint32_t src_order_wqe_vld : 1;
	uint32_t no_wqe_send_cnt : 4;
	uint32_t so_lp_vld : 1;
	uint32_t fence_lp_vld : 1;
	uint32_t strong_fence_lp_vld : 1;
	/* DW20 */
	uint32_t PI : 16;
	uint32_t sq_db_doing : 1;
	uint32_t ost_rce_credit : 15;
	/* DW21 */
	uint32_t sq_db_retrying : 1;
	uint32_t wmtp_rsv0 : 31;
	/* DW22 */
	uint32_t wait_ack_timeout : 1;
	uint32_t wait_rnr_timeout : 1;
	uint32_t cqe_ie : 1;
	uint32_t cqe_sz : 1;
	uint32_t wml_rsv0 : 28;
	/* DW23 */
	uint32_t wml_rsv1 : 32;
	/* DW24 */
	uint32_t next_rcv_ssn : 16;
	uint32_t next_cpl_bb_idx : 16;
	/* DW25 */
	uint32_t next_cpl_sgmt_num : 20;
	uint32_t we_rsv0 : 12;
	/* DW26 */
	uint32_t next_cpl_bb_num : 4;
	uint32_t next_cpl_cqe_en : 1;
	uint32_t next_cpl_info_vld : 1;
	uint32_t rpting_cqe : 1;
	uint32_t not_rpt_cqe : 1;
	uint32_t flush_ssn : 16;
	uint32_t flush_ssn_vld : 1;
	uint32_t flush_vld : 1;
	uint32_t flush_cqe_done : 1;
	uint32_t we_rsv1 : 5;
	/* DW27 */
	uint32_t rcved_cont_ssn_num : 20;
	uint32_t we_rsv2 : 12;
	/* DW28 */
	uint32_t sq_timer;
	/* DW29 */
	uint32_t rnr_cnt : 3;
	uint32_t abt_ssn : 16;
	uint32_t abt_ssn_vld : 1;
	uint32_t taack_timeout_flag : 1;
	uint32_t we_rsv3 : 9;
	uint32_t err_type_l : 2;
	/* DW30 */
	uint32_t err_type_h : 7;
	uint32_t sq_flush_ssn : 16;
	uint32_t we_rsv4 : 9;
	/* DW31 */
	uint32_t avail_sgmt_ost : 10;
	uint32_t read_op_cnt : 10;
	uint32_t we_rsv5 : 12;
	/* DW32 - DW63 */
	uint32_t taack_nack_bm[32];
};

static inline struct udma_jetty *to_udma_jetty(struct ubcore_jetty *jetty)
{
	return container_of(jetty, struct udma_jetty, ubcore_jetty);
}

static inline struct udma_jetty_grp *to_udma_jetty_grp(struct ubcore_jetty_group *jetty_grp)
{
	return container_of(jetty_grp, struct udma_jetty_grp, ubcore_jetty_grp);
}

static inline struct udma_jetty *to_udma_jetty_from_queue(struct udma_jetty_queue *queue)
{
	return container_of(queue, struct udma_jetty, sq);
}

#endif /* __UDMA_JETTY_H__ */
