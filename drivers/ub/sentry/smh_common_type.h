/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * Description: Common Header File for Sentry Module
 * Author: Luckky
 * Create: 2025-02-17
 */

#ifndef SMH_COMMON_TYPE_H
#define SMH_COMMON_TYPE_H

#include <ub/ubus/ub-mem-decoder.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/inet.h>
#include <linux/proc_fs.h>

#define SMH_TYPE ('}')
#define OOM_EVENT_MAX_NUMA_NODES 8
#define REPORT_COMM_TIME  5000
#define MILLISECONDS_OF_EACH_MDELAY 1000
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
