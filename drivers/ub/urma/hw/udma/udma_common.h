/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#ifndef __UDMA_COMM_H__
#define __UDMA_COMM_H__

#include <linux/jhash.h>
#include <linux/vmalloc.h>
#include <ub/urma/ubcore_api.h>
#include "udma_ctx.h"
#include "udma_dev.h"

#define TP_ACK_UDP_SPORT_H_OFFSET 8
#define UDMA_TPHANDLE_TPID_SHIFT 0xFFFFFF

struct udma_jetty_grp {
	struct ubcore_jetty_group ubcore_jetty_grp;
	uint32_t start_jetty_id;
	uint32_t next_jetty_id;
	uint32_t jetty_grp_id;
	uint32_t valid;
	struct mutex valid_lock;
	refcount_t ae_refcount;
	struct completion ae_comp;
};

struct udma_jetty_queue {
	struct udma_buf buf;
	void *kva_curr;
	uint32_t id;
	void __iomem *db_addr;
	void __iomem *dwqe_addr;
	uint32_t pi;
	uint32_t ci;
	uintptr_t *wrid;
	spinlock_t lock;
	uint32_t max_inline_size;
	uint32_t max_sge_num;
	uint32_t tid;
	bool flush_flag;
	uint32_t old_entry_idx;
	enum ubcore_transport_mode trans_mode;
	struct ubcore_tjetty *rc_tjetty;
	bool is_jetty;
	uint32_t sqe_bb_cnt;
	uint32_t lock_free; /* Support kernel mode lock-free mode */
	uint32_t ta_timeout; /* ms */
	enum ubcore_jetty_state state;
	struct udma_context *udma_ctx;
	bool non_pin;
	struct udma_jetty_grp *jetty_grp;
	enum udma_jetty_type jetty_type;
};

enum tp_state {
	TP_INVALID = 0x0,
	TP_VALID = 0x1,
	TP_RTS = 0x3,
	TP_ERROR = 0x6,
};

int pin_queue_addr(struct udma_dev *dev, uint64_t addr,
		   uint32_t len, struct udma_buf *buf);
void unpin_queue_addr(struct ubcore_umem *umem);

struct udma_umem_param {
	struct ubcore_device *ub_dev;
	uint64_t va;
	uint64_t len;
	union ubcore_umem_flag flag;
	bool is_kernel;
};

struct udma_ue_index_cmd {
	uint16_t ue_idx;
	uint8_t rsv[2];
	uint8_t guid[16];
};

struct udma_tp_ctx {
	/* Byte4 */
	uint32_t version : 1;
	uint32_t tp_mode : 1;
	uint32_t trt : 1;
	uint32_t wqe_bb_shift : 4;
	uint32_t oor_en : 1;
	uint32_t tempid : 6;
	uint32_t portn : 6;
	uint32_t rsvd1 : 12;
	/* Byte8 */
	uint32_t wqe_ba_l;
	/* Byte12 */
	uint32_t wqe_ba_h : 20;
	uint32_t udp_srcport_range : 4;
	uint32_t cng_alg_sel : 3;
	uint32_t lbi : 1;
	uint32_t rsvd4 : 1;
	uint32_t vlan_en : 1;
	uint32_t mtu : 2;
	/* Byte16 */
	uint32_t route_addr_idx : 20;
	uint32_t rsvd6 : 12;
	/* Byte20 */
	u32 tpn_vtpn : 24;
	u32 rsvd7 : 8;
	/* Byte24 to Byte28 */
	u32 rsvd8[2];
	/* Byte 32 */
	u32 seid_idx : 16;
	u32 sjetty_l : 16;
	/* Byte 36 */
	u32 sjetty_h : 4;
	u32 tp_wqe_token_id : 20;
	u32 tp_wqe_position : 1;
	u32 rsv9_l : 7;
	/* Byte 40 */
	u32 rsvd9_h : 6;
	u32 taack_tpn : 24;
	u32 rsvd10 : 2;
	/* Byte 44 */
	u32 spray_en : 1;
	u32 sr_en : 1;
	u32 ack_freq_mode : 1;
	u32 route_type : 2;
	u32 vl : 4;
	u32 dscp : 6;
	u32 switch_mp_en : 1;
	u32 at_times : 5;
	u32 retry_num_init : 3;
	u32 at : 5;
	u32 rsvd13 : 3;
	/* Byte 48 */
	u32 on_flight_size : 16;
	u32 hpln : 8;
	u32 fl_l : 8;
	/* Byte 52 */
	u32 fl_h : 12;
	u32 dtpn : 20;
	/* Byte 56 */
	u32 rc_tpn : 24;
	u32 rc_vl : 4;
	u32 tpg_vld : 1;
	u32 reorder_cap : 3;
	/* Byte 60 */
	u32 reorder_q_shift : 4;
	u32 reorder_q_addr_l : 28;
	/* Byte 64 */
	u32 reorder_q_addr_h : 24;
	u32 tpg_l : 8;
	/* Byte 68 */
	u32 tpg_h : 12;
	u32 jettyn : 20;
	/* Byte 72 */
	u32 dyn_timeout_mode : 1;
	u32 base_time : 23;
	u32 rsvd15 : 8;
	/* Byte 76 */
	u32 tpack_psn : 24;
	u32 tpack_rspst : 3;
	u32 tpack_rspinfo : 5;
	/* Byte 80 */
	u32 tpack_msn : 24;
	u32 ack_udp_srcport_l : 8;
	/* Byte 84 */
	u32 ack_udp_srcport_h : 8;
	u32 max_rcv_psn : 24;
	/* Byte 88 */
	u32 scc_token : 19;
	u32 poll_db_wait_do : 1;
	u32 msg_rty_lp_flg : 1;
	u32 retry_cnt : 3;
	u32 sq_invld_flg : 1;
	u32 wait_ack_timeout : 1;
	u32 tx_rtt_caling : 1;
	u32 cnp_tx_flag : 1;
	u32 sq_db_doing : 1;
	u32 tpack_doing : 1;
	u32 sack_wait_do : 1;
	u32 tpack_wait_do : 1;
	/* Byte 92 */
	u16 post_max_idx;
	u16 wqe_max_bb_idx;
	/* Byte 96 */
	u16 wqe_bb_pi;
	u16 wqe_bb_ci;
	/* Byte 100 */
	u16 data_udp_srcport;
	u16 wqe_msn;
	/* Byte 104 */
	u32 cur_req_psn : 24;
	u32 tx_ack_psn_err : 1;
	u32 poll_db_type : 2;
	u32 tx_ack_flg : 1;
	u32 tx_sq_err_flg : 1;
	u32 scc_retry_type : 2;
	u32 flush_cqe_wait_do : 1;
	/* Byte 108 */
	u32 wqe_max_psn : 24;
	u32 ssc_token_l : 4;
	u32 rsvd16 : 4;
	/* Byte 112 */
	u32 tx_sq_timer;
	/* Byte 116 */
	u32 rtt_timestamp_psn : 24;
	u32 rsvd17 : 8;
	/* Byte 120 */
	u32 rtt_timestamp : 24;
	u32 cnp_timer_l : 8;
	/* Byte 124 */
	u32 cnp_timer_h : 16;
	u32 max_reorder_id : 16;
	/* Byte 128 */
	u16 cur_reorder_id;
	u16 wqe_max_msn;
	/* Byte 132 */
	u16 post_bb_pi;
	u16 post_bb_ci;
	/* Byte 136 */
	u32 lr_ae_ind : 1;
	u32 rx_cqe_cnt : 16;
	u32 reorder_q_si : 13;
	u32 rq_err_type_l : 2;
	/* Byte 140 */
	u32 rq_err_type_h : 3;
	u32 rsvd18 : 2;
	u32 rsvd19 : 27;
	/* Byte 144 */
	u32 req_seq;
	/* Byte 148 */
	uint32_t req_ce_seq;
	/* Byte 152 */
	u32 req_cmp_lrb_indx : 12;
	u32 req_lrb_indx : 12;
	u32 req_lrb_indx_vld : 1;
	u32 rx_req_psn_err : 1;
	u32 rx_req_last_optype : 3;
	u32 rx_req_fake_flg : 1;
	u32 rsvd20 : 2;
	/* Byte 156 */
	uint16_t jfr_wqe_idx;
	uint16_t rx_req_epsn_l;
	/* Byte 160 */
	uint32_t rx_req_epsn_h : 8;
	uint32_t rx_req_reduce_code : 8;
	uint32_t rx_req_msn_l : 16;
	/* Byte 164 */
	uint32_t rx_req_msn_h : 8;
	uint32_t jfr_wqe_rnr : 1;
	uint32_t jfr_wqe_rnr_timer : 5;
	uint32_t rsvd21 : 2;
	uint32_t jfr_wqe_cnt : 16;
	/* Byte 168 */
	uint32_t max_reorder_q_idx : 13;
	uint32_t rsvd22 : 3;
	uint32_t reorder_q_ei : 13;
	uint32_t rx_req_last_elr_flg : 1;
	uint32_t rx_req_last_elr_err_type_l : 2;
	/* Byte172 */
	uint32_t rx_req_last_elr_err_type_h : 3;
	uint32_t rx_req_last_op : 1;
	uint32_t jfrx_jetty : 1;
	uint32_t jfrx_jfcn_l : 16;
	uint32_t jfrx_jfcn_h : 4;
	uint32_t jfrx_jfrn_l : 7;
	/* Byte176 */
	u32 jfrx_jfrn_h1 : 9;
	u32 jfrx_jfrn_h2 : 4;
	u32 rq_timer_l : 19;
	/* Byte180 */
	u32 rq_timer_h : 13;
	u32 rq_at : 5;
	u32 wait_cqe_timeout : 1;
	u32 rsvd23 : 13;
	/* Byte184 */
	u32 rx_sq_timer;
	/* Byte188 */
	u32 tp_st : 3;
	u32 rsvd24 : 4;
	u32 ls_ae_ind : 1;
	u32 retry_msg_psn : 24;
	/* Byte192 */
	u32 retry_msg_fpsn : 24;
	u32 rsvd25 : 8;
	/* Byte196 */
	u16 retry_wqebb_idx;
	u16 retry_msg_msn;
	/* Byte200 */
	u32 ack_rcv_seq;
	/* Byte204 */
	u32 rtt : 24;
	u32 dup_sack_cnt : 8;
	/* Byte208 */
	u32 sack_max_rcv_psn : 24;
	u32 rsvd26 : 7;
	u32 rx_ack_flg : 1;
	/* Byte212 */
	u32 rx_ack_msn : 16;
	u32 sack_lrb_indx : 12;
	u32 rx_fake_flg : 1;
	u32 rx_rtt_caling : 1;
	u32 rx_ack_psn_err : 1;
	u32 sack_lrb_indx_vld : 1;
	/* Byte216 */
	u32 rx_ack_epsn : 24;
	u32 rsvd27 : 8;
	/* Byte220 */
	u32 max_retry_psn : 24;
	u32 retry_reorder_id_l : 8;
	/* Byte224 */
	u32 retry_reorder_id_h : 8;
	u32 rsvd28 : 8;
	u32 rsvd29 : 16;
	/* Byte228 to Byte256 */
	u32 scc_data[8];
};

struct ubcore_umem *udma_umem_get(struct udma_umem_param *param);
void udma_umem_release(struct ubcore_umem *umem, bool is_kernel);
void udma_init_udma_table(struct udma_table *table, uint32_t max, uint32_t min, bool irq_lock);
void udma_init_udma_table_mutex(struct xarray *table, struct mutex *udma_mutex, bool irq_lock);
void udma_destroy_npu_cb_table(struct udma_dev *dev);
void udma_destroy_udma_table(struct udma_dev *dev, struct udma_table *table,
			     const char *table_name);
void udma_destroy_eid_table(struct udma_dev *udma_dev);
void udma_dfx_store_id(struct udma_dev *udma_dev, struct udma_dfx_entity *entity,
		       uint32_t id, const char *name);
void udma_dfx_delete_id(struct udma_dev *udma_dev, struct udma_dfx_entity *entity,
			uint32_t id);
int udma_alloc_normal_buf(struct udma_dev *udma_dev, size_t memory_size, struct udma_buf *buf);
void udma_free_normal_buf(struct udma_dev *udma_dev, size_t memory_size, struct udma_buf *buf);
int udma_k_alloc_buf(struct udma_dev *dev, struct udma_buf *buf);
void udma_k_free_buf(struct udma_dev *dev, struct udma_buf *buf);
void *udma_alloc_iova(struct udma_dev *udma_dev, size_t memory_size, dma_addr_t *addr);
void udma_free_iova(struct udma_dev *udma_dev, size_t memory_size, void *kva_or_slot,
		    dma_addr_t addr);

static inline void udma_write64(struct udma_dev *udma_dev,
				uint64_t *val, void __iomem *dest)
{
	writeq(*val, dest);
}

static inline void udma_alloc_kernel_db(struct udma_dev *dev,
					struct udma_jetty_queue *queue)
{
	queue->dwqe_addr = dev->k_db_base + JETTY_DSQE_OFFSET +
			   UDMA_HW_PAGE_SIZE * queue->id;
	queue->db_addr = queue->dwqe_addr + UDMA_DOORBELL_OFFSET;
}

static inline void *get_buf_entry(struct udma_buf *buf, uint32_t n)
{
	uint32_t entry_index = n & (buf->entry_cnt - 1);

	return (char *)buf->kva + (entry_index * buf->entry_size);
}

static inline uint8_t to_ta_timeout(uint32_t err_timeout)
{
#define TA_TIMEOUT_DIVISOR 8
	return err_timeout / TA_TIMEOUT_DIVISOR;
}

static inline uint64_t udma_cal_npages(uint64_t va, uint64_t len)
{
	return (ALIGN(va + len, PAGE_SIZE) - ALIGN_DOWN(va, PAGE_SIZE)) / PAGE_SIZE;
}

int udma_query_ue_idx(struct ubcore_device *ub_dev, struct ubcore_devid *devid,
		      uint16_t *ue_idx);
void udma_dfx_ctx_print(struct udma_dev *udev, const char *name, uint32_t id, uint32_t len,
			uint32_t *ctx);
void udma_swap_endian(uint8_t arr[], uint8_t res[], uint32_t res_size);

void udma_init_hugepage(struct udma_dev *dev);
void udma_destroy_hugepage(struct udma_dev *dev);

#endif /* __UDMA_COMM_H__ */
