#ifndef __ZXDH_PRIV_QUEUE_H__
#define __ZXDH_PRIV_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/list.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/netdevice.h>
#include <linux/scatterlist.h>
#include "queue.h"
#include "../en_aux.h"
#include "../../dinghai/en_np/table/include/dpp_tbl_api.h"

#define MSGQ_TEST 1

#define MSGQ_RET_OK 0
#define MSGQ_RET_ERR (-1)
#define MSGQ_RET_ERR_NULL_PTR (-2)
#define MSGQ_RET_ERR_INVALID_PARA (-3)
#define MSGQ_RET_ERR_CHANNEL_NOT_READY (-5)
#define MSGQ_RET_ERR_CHAN_BUSY (-6)
#define MSGQ_RET_ERR_VQ_BROKEN (-7)
#define MSGQ_RET_ERR_CALLBACK_OUT_OF_TIME (-8)
#define MSGQ_RET_ERR_CALLBACK_FAIL (-9)
#define MSGQ_RET_ERR_REPS_LEN_NOT_ENOUGH (-10)
#define MSGQ_RET_ERR_RX_INVALID_NUM_BUF (-11)

struct reps_info {
	uint32_t len;
	uint8_t *addr;
};

struct msgq_pkt_info {
	uint32_t timeout_us;
	uint16_t event_id;
	bool is_async;
	bool no_reps;
	uint8_t msg_priority;
	uint8_t rsv;
	uint32_t len;
	uint8_t *addr;
} __attribute__((packed));

/* msg_chan_pkt Definitions */
#define MAX_PACKET_LEN (MSGQ_MAX_ADDR_LEN - PRIV_HEADER_LEN)
#define MSGQ_MAX_ADDR_LEN 14000
#define NO_REPS_SEQUENCE_NUM 0x8000

#define TIMER_DELAY_US 100
#define MSGQ_MAX_MSG_BUFF_NUM 1024
#define BUFF_LEN 4096

#define PRIV_HEADER_LEN sizeof(struct priv_queues_net_hdr)
#define DEFAULT_PI_TYPE 0x00 /*NP*/
#define CONTROL_MSG_TYPE 0x1f
#define NEED_REPS_MSG 0x00
#define ACK_MSG 0x01
#define NO_REPS_MSG 0x02

#define RISCV_COMMON_VFID (1192)
#define RISCV_COMMON_QID (4092)

enum msgq_err_code {
	ERR_CODE_INVALID_EVENTID = 1,
	ERR_CODE_EVENT_UNREGIST,
	ERR_CODE_INVALID_ACK,
	ERR_CODE_EVENT_FAIL,
	ERR_CODE_INVALID_REPS_LEN,
	ERR_CODE_PEER_BROKEN,
};

struct pi_header {
	uint8_t pi_type;
	uint8_t pkt_type;
	uint16_t event_id;
	uint16_t vfid_dst;
	uint16_t qid_dst;
	uint16_t vfid_src;
	uint16_t qid_src;
	uint16_t sequence_num;
	uint8_t msg_priority;
	uint8_t msg_type;
	uint8_t err_code;
	uint8_t rsv[3];
};

struct msgq_pi_info {
	uint16_t event_id;
	uint16_t vfid_dst;
	uint16_t qid_dst;
	uint16_t vfid_src;
	uint16_t qid_src;
	uint16_t sequence_num;
} __attribute__((packed));

struct priv_queues_net_hdr {
	uint8_t tx_port;
	uint8_t pd_len;
	uint8_t num_buffers;
	uint8_t rsv;
	struct pi_header pi_hdr;
};

struct msg_buff {
	bool using;
	bool valid;
	bool need_free;
	uint32_t timeout_cnt;
	uint8_t **data;
	uint32_t *data_len;
} __attribute__((packed));

#define MSGQ_PRINT_HDR 1
#define MSGQ_PRINT_128B 2
#define MSGQ_PRINT_ALL 3
#define MSGQ_PRINT_STA 4

struct msgq_dev {
	bool msgq_enable;
	bool timer_in_use;
	bool loopback;
	uint8_t print_flag;
	uint16_t sequence_num;
	uint16_t free_cnt;
	uint16_t msgq_vfid;
	uint16_t msgq_rqid;
	struct send_queue *sq_priv;
	struct receive_queue *rq_priv;
	struct mutex *mlock;
	struct spinlock sn_lock;
	struct spinlock tx_lock;
	struct msg_buff msg_buff_ring[MSGQ_MAX_MSG_BUFF_NUM];
	struct timer_list poll_timer;
} __attribute__((packed));

#define CHECK_CHANNEL_USABLE(msgq, ret, err)                  \
	do {                                                  \
		if (!(msgq)->msgq_enable) {                   \
			LOG_ERR("msgq unable\n");             \
			ret = MSGQ_RET_ERR_CHANNEL_NOT_READY; \
			goto err;                             \
		}                                             \
	} while (0)

#define ZXDH_CHECK_PTR_RETURN(ptr)                    \
	do {                                          \
		if (unlikely((ptr) == NULL)) {        \
			LOG_ERR("null pointer\n");    \
			return MSGQ_RET_ERR_NULL_PTR; \
		}                                     \
	} while (0)

#define ZXDH_CHECK_PTR_GOTO_ERR(ptr, err)          \
	do {                                       \
		if (unlikely((ptr) == NULL)) {     \
			LOG_ERR("null pointer\n"); \
			goto err;                  \
		}                                  \
	} while (0)

#define ZXDH_FREE_PTR(ptr)            \
	do {                          \
		if ((ptr) != NULL) {  \
			kfree(ptr);   \
			(ptr) = NULL; \
		}                     \
	} while (0)

#define SEQUENCE_NUM_ADD(id)                   \
	do {                                   \
		(id)++;                        \
		(id) %= MSGQ_MAX_MSG_BUFF_NUM; \
	} while (0)

int32_t zxdh_msgq_init(struct zxdh_en_device *en_dev);
void zxdh_msgq_exit(struct zxdh_en_device *en_dev);
int32_t print_data(uint8_t *data, uint32_t len);
int32_t zxdh_msgq_send_cmd(struct msgq_dev *msgq_dev,
			   struct msgq_pkt_info *pkt_info,
			   struct reps_info *reps);
int zxdh_msgq_poll(struct napi_struct *napi, int budget);
int32_t msgq_privq_init(struct msgq_dev *msgq_dev, struct net_device *netdev);
void msgq_privq_uninit(struct msgq_dev *msgq_dev);

#ifdef __cplusplus
}
#endif

#endif /* __ZXDH_PRIV_QUEUE_H__  */
