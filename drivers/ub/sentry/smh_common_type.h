/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * Description: Common Header File for Sentry Module
 * Author: Luckky
 * Create: 2025-02-17
 */

#ifndef SMH_COMMON_TYPE_H
#define SMH_COMMON_TYPE_H

#include <linux/firmware/ubios/cis.h>
#include <ub/ubus/ub-mem-decoder.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/inet.h>
#include <linux/proc_fs.h>

#define SMH_TYPE ('}')
#define MAX_DIE_NUM 2
#define OOM_EVENT_MAX_NUMA_NODES 8
#define MAX_NODE_NUM 32
#define EID_MAX_LEN 40 // eid str len 39 + '\0'
#define REPORT_COMM_TIME  5000
#define URMA_SEND_DATA_MAX_LEN (2 + EID_MAX_LEN * 2 + 10 + 1 + 4) // type_cna_eid_randomID_res
#define MILLISECONDS_OF_EACH_MDELAY 1000
#define CNA_MAX_VALUE 0xffffff
#define INTEGER_TO_STR_MAX_LEN 22
#define COMM_PARM_NOT_SET (-2)
#define HEARTBEAT "heartbeat"
#define HEARTBEAT_ACK "heartbeat_ack"
#define ENABLE_VALUE_MAX_LEN 4 // 'off' + '\0'

#define URMA_REBUILD_THRESHOLD 3
#define URMA_ACK_RETRY_NUM     10

#define PROC_FILE_PERMISSION 0600
#define PROC_DIR_PERMISSION 0550

enum {
    SMH_CMD_MSG_ACK = 0x10,
};

#define SMH_MSG_ACK _IO(SMH_TYPE, SMH_CMD_MSG_ACK)

enum sentry_msg_helper_msg_type {
    SMH_MESSAGE_POWER_OFF,
    SMH_MESSAGE_OOM,
    SMH_MESSAGE_UB_MEM_ERR,
    SMH_MESSAGE_UNKNOWN,
};

struct sentry_msg_helper_msg {
    enum sentry_msg_helper_msg_type type;
    uint64_t msgid;
    uint64_t start_send_time;
    uint64_t timeout_time;
    // reboot_info is empty
    union {
		struct {
			int nr_nid;
			int nid[OOM_EVENT_MAX_NUMA_NODES];
			int sync;
			int timeout;
			int reason;
		} oom_info;
		struct {
			uint64_t pa;
			int mem_type;
			int fault_with_kill;
			enum ras_err_type raw_ubus_mem_err_type;
		} ub_mem_info;
    } helper_msg_info;
    unsigned long res;
};

extern uint32_t g_local_cna;
#define UVB_SENDER_ID_SYSSENTRY_INDEX (g_local_cna)
#define UVB_SENDER_ID_SYSSENTRY (UBIOS_USER_ID_RICH_OS | UVB_SENDER_ID_SYSSENTRY_INDEX)
#define UVB_RECEIVER_ID_SYSSENTRY(cna) (UBIOS_USER_ID_UB_DEVICE | (cna))

static inline int sentry_create_proc_file(const char *name, struct proc_dir_entry *parent,
					  const struct proc_ops *proc_ops)
{
    int ret = 0;

    if (!proc_create(name, PROC_FILE_PERMISSION, parent, proc_ops)) {
		pr_err("create proc file %s failed.\n", name);
		ret = -ENOMEM;
    }
    return ret;
}
#endif
