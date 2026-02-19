// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: Server module, used for reporting panic or reboot msg to the
 *              userspace and forward ack msg to the client
 * Author: sxt1001
 * Create: 2025-03-18
 */

#include <acpi/button.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

#include "smh_message.h"
#include "sentry_remote_reporter.h"

#undef pr_fmt
#define pr_fmt(fmt) "[sentry][remote server]: " fmt

struct sentry_remote_context sentry_remote_ctx;
DEFINE_SPINLOCK(sentry_buf_lock);

static DEFINE_MUTEX(sentry_msg_info_mutex);

/**
 * send_msg_to_userspace - Send message to userspace with proper tracking
 * @msg: Message to send
 * @comm_type: Communication type (URMA or UVB)
 * @random_id: Random identifier for message tracking
 *
 * Return: 0 on success, negative error code on failure
 *
 * This function sends a message to userspace and tracks it using node message
 * info for acknowledgment handling.
 */
int send_msg_to_userspace(struct sentry_msg_helper_msg *msg,
			  enum SENTRY_REMOTE_COMM_TYPE comm_type, uint32_t random_id)
{
	int ret;
	int node_idx = -1;
	int die_index = -1;
	union ubcore_eid dst_ubcore_eid;

	pr_info("send %s message to userspace\n",
		comm_type == COMM_TYPE_URMA ? "urma" : "uvb");

	if (comm_type == COMM_TYPE_URMA) {
		if (str_to_eid(msg->helper_msg_info.remote_info.eid, &dst_ubcore_eid) < 0) {
			pr_err("send_msg_to_userspace: invalid dst eid [%s]\n",
			       msg->helper_msg_info.remote_info.eid);
			return -EINVAL;
		}
		match_index_by_remote_ub_eid(dst_ubcore_eid, &node_idx, &die_index);
	} else if (comm_type == COMM_TYPE_UVB) {
		int i;

		for (i = 0; i < g_server_cna_valid_num; i++) {
			if (msg->helper_msg_info.remote_info.cna == g_server_cna_array[i]) {
				node_idx = i;
				break;
			}
		}
	}

	if (node_idx < 0) {
		pr_err("Invalid cna: %u or eid: %s of msg, stop to send to userspace\n",
		       msg->helper_msg_info.remote_info.cna,
		       msg->helper_msg_info.remote_info.eid);
		return -EINVAL;
	}

	mutex_lock(&sentry_msg_info_mutex);
	if (sentry_remote_ctx.node_msg_info_list[node_idx].random_id != random_id) {
		pr_info("Get new message from cna: %u, eid: %s\n",
			msg->helper_msg_info.remote_info.cna,
			msg->helper_msg_info.remote_info.eid);
		sentry_remote_ctx.node_msg_info_list[node_idx].start_send_time = ktime_get_ns();
		sentry_remote_ctx.node_msg_info_list[node_idx].msgid = smh_get_new_msg_id();
		sentry_remote_ctx.node_msg_info_list[node_idx].random_id = random_id;
	}
	msg->start_send_time = sentry_remote_ctx.node_msg_info_list[node_idx].start_send_time;
	msg->msgid = sentry_remote_ctx.node_msg_info_list[node_idx].msgid;
	mutex_unlock(&sentry_msg_info_mutex);

	ret = smh_message_send(msg, true);
	return ret;
}

/**
 * send_msg_to_userspace_and_ack - Send message to userspace and wait for acknowledgment
 * @msg: Message to send
 * @comm_type: Communication type (URMA or UVB)
 * @random_id: Random identifier for message tracking
 * @ack_type: Type of acknowledgment expected
 *
 * Return: 0 on success, negative error code on failure
 *
 * This function sends a message to userspace, waits for acknowledgment, and
 * sends acknowledgment back to the remote node.
 */
int send_msg_to_userspace_and_ack(struct sentry_msg_helper_msg *msg,
				  enum SENTRY_REMOTE_COMM_TYPE comm_type,
				  uint32_t random_id, enum sentry_msg_helper_msg_type ack_type)
{
	int ret;
	int times = msg->timeout_time / MILLISECONDS_OF_EACH_MDELAY;
	int i, j;

	ret = send_msg_to_userspace(msg, comm_type, random_id);
	if (ret) {
		pr_err("Failed to send remote message to userspace\n");
		return ret;
	}

	/* Wait for acknowledgment from userspace */
	for (i = 0; i < times; i++) {
		uint64_t cur_time = ktime_get_ns();

		ret = smh_message_get_ack(msg);
		if (!ret) {
			int sleep_time = MILLISECONDS_OF_EACH_MDELAY -
					(int)((ktime_get_ns() - cur_time) / NSEC_PER_MSEC);
			if (sleep_time > 0)
				msleep_interruptible(sleep_time);
			continue;
		}

		/* Get acknowledgment success, send acknowledgment message */
		char send_ack[URMA_SEND_DATA_MAX_LEN];

		ret = snprintf(send_ack, URMA_SEND_DATA_MAX_LEN, "%d_%u_%s_%lu",
			       ack_type,
			       msg->helper_msg_info.remote_info.cna,
			       msg->helper_msg_info.remote_info.eid,
			       msg->res);
		if ((size_t)ret >= URMA_SEND_DATA_MAX_LEN) {
			pr_err("msg str size exceeds the max value\n");
			return -EINVAL;
		}

		pr_info("Start to send %s ack msg to %u\n",
			comm_type == COMM_TYPE_URMA ? "urma" : "uvb",
			msg->helper_msg_info.remote_info.cna);

		if (comm_type == COMM_TYPE_URMA) {
			/* Retry URMA acknowledgment sending */
			for (j = 0; j < URMA_ACK_RETRY_NUM; j++) {
				ret = urma_send(send_ack, sizeof(send_ack),
						msg->helper_msg_info.remote_info.eid, -1);
				if (ret == COMM_PARM_NOT_SET)
					break;
				msleep_interruptible(MILLISECONDS_OF_EACH_MDELAY);
			}
		} else {
			/* UVB is a reliable protocol, no need to resend */
			ret = uvb_send(send_ack, msg->helper_msg_info.remote_info.cna, false);
		}

		if (ret <= 0) {
			pr_warn("Failed to send %s ack message to client (cna:%u, eid:%s)\n",
				comm_type == COMM_TYPE_URMA ? "urma" : "uvb",
				msg->helper_msg_info.remote_info.cna,
				msg->helper_msg_info.remote_info.eid);
			return -EFAULT;
		}
		return 0;
	}

	return -ETIMEDOUT;
}

/**
 * get_ack_type - Get acknowledgment type for given event type
 * @event_type: Event type to get acknowledgment for
 *
 * Return: Corresponding acknowledgment type
 */
enum sentry_msg_helper_msg_type get_ack_type(enum sentry_msg_helper_msg_type event_type)
{
	enum sentry_msg_helper_msg_type ack_type;

	switch (event_type) {
	case SMH_MESSAGE_PANIC:
		ack_type = SMH_MESSAGE_PANIC_ACK;
		break;
	case SMH_MESSAGE_KERNEL_REBOOT:
		ack_type = SMH_MESSAGE_KERNEL_REBOOT_ACK;
		break;
	default:
		pr_warn("Invalid event type!\n");
		ack_type = SMH_MESSAGE_UNKNOWN;
	}

	return ack_type;
}

/**
 * process_remote_event_msg - Process remote event message in kernel thread context
 * @data: Pointer to child_thread_process_data structure containing message info
 *
 * Return: 0 on success, negative error code on failure
 */
static int process_remote_event_msg(void *data)
{
	int ret;
	enum sentry_msg_helper_msg_type ack_type;
	struct child_thread_process_data *child_data = data;

	try_module_get(THIS_MODULE);

	ack_type = get_ack_type(child_data->msg->type);
	if (ack_type == SMH_MESSAGE_UNKNOWN) {
		ret = -EINVAL;
		goto cleanup_child;
	}

	ret = send_msg_to_userspace_and_ack(child_data->msg, child_data->comm_type,
					    child_data->random_id, ack_type);

cleanup_child:
	kfree(child_data->msg);
	kfree(child_data);
	module_put(THIS_MODULE);
	return ret;
}

/**
 * write_ack_msg_buf - Write acknowledgment message to shared buffer
 * @msg: Acknowledgment message
 * @comm_type: Communication type (URMA or UVB)
 *
 * This function writes an acknowledgment message to a shared buffer for
 * inter-process communication, ensuring thread-safe access.
 */
void write_ack_msg_buf(const struct sentry_msg_helper_msg *msg,
		       enum SENTRY_REMOTE_COMM_TYPE comm_type)
{
	if (atomic_inc_return(&sentry_remote_ctx.remote_event_ack_received) == 1) {
		pr_info("Receive ack message from %s: [%d_%u_%s_%lu]. Start to update buf\n",
			comm_type == COMM_TYPE_URMA ? "URMA" : "UVB",
			msg->type,
			msg->helper_msg_info.remote_info.cna,
			msg->helper_msg_info.remote_info.eid,
			msg->res);

		spin_lock(&sentry_buf_lock);
		memcpy(&sentry_remote_ctx.remote_event_ack_msg_buf, msg,
		       sizeof(sentry_remote_ctx.remote_event_ack_msg_buf));
		spin_unlock(&sentry_buf_lock);
		atomic_set(&sentry_remote_ctx.remote_event_ack_done, 1);
	}
}

/**
 * create_kthread_to_process_msg - Create kernel thread to process incoming message
 * @event_msg: Raw event message string
 * @comm_type: Communication type (URMA or UVB)
 *
 * Return: 0 on success, negative error code on failure
 *
 * This function creates a kernel thread to process incoming remote messages,
 * handling both panic/reboot events and acknowledgment messages.
 */
int create_kthread_to_process_msg(const char *event_msg,
				  enum SENTRY_REMOTE_COMM_TYPE comm_type)
{
	int ret;
	struct sentry_msg_helper_msg msg;
	uint32_t random_id;
	struct child_thread_process_data *child_data;
	struct task_struct *child_thread;

	ret = convert_str_to_smh_msg(event_msg, &msg, &random_id);
	if (ret) {
		pr_err("convert %s data to smh msg failed. msg [%s]\n",
		       comm_type == COMM_TYPE_URMA ? "urma" : "uvb", event_msg);
		return -EINVAL;
	}

	if (msg.type != SMH_MESSAGE_PANIC && msg.type != SMH_MESSAGE_KERNEL_REBOOT) {
		/* Write acknowledgment message to shared memory */
		write_ack_msg_buf(&msg, comm_type);
		return 0;
	}

	child_data = kzalloc(sizeof(*child_data), GFP_KERNEL);
	if (!child_data) {
		pr_err("Failed to allocate memory for child_data\n");
		return -ENOMEM;
	}

	child_data->msg = kzalloc(sizeof(*child_data->msg), GFP_KERNEL);
	if (!child_data->msg) {
		kfree(child_data);
		pr_err("Failed to allocate memory for child_data->msg\n");
		return -ENOMEM;
	}

	/* Update child thread data */
	memcpy(child_data->msg, &msg, sizeof(*child_data->msg));
	child_data->random_id = random_id;
	child_data->comm_type = comm_type;

	child_thread = kthread_run(process_remote_event_msg, child_data,
				   "sentry_msg_thread_%s_%u",
				   comm_type == COMM_TYPE_URMA ? "urma" : "uvb",
				   random_id);
	if (IS_ERR(child_thread)) {
		kfree(child_data->msg);
		kfree(child_data);
		pr_err("Failed to create child thread\n");
		return PTR_ERR(child_thread);
	}

	return 0;
}

/**
 * process_urma_data - Process URMA data in kernel thread
 * @data: Thread data (unused)
 *
 * Return: 0 on success, negative error code on failure
 *
 * This function runs in a kernel thread to receive and process URMA messages,
 * creating separate threads for message processing.
 */
static int process_urma_data(void *data)
{
	int ret = 0;
	int recv_msg_nodes = 0;
	char **msg_str;
	int i;

	msg_str = kcalloc(MAX_NODE_NUM * MAX_DIE_NUM, sizeof(*msg_str), GFP_KERNEL);
	if (!msg_str) {
		pr_err("Failed to allocate memory for msg_str!\n");
		return -ENOMEM;
	}

	for (i = 0; i < MAX_NODE_NUM * MAX_DIE_NUM; i++) {
		msg_str[i] = kzalloc(URMA_SEND_DATA_MAX_LEN, GFP_KERNEL);
		if (!msg_str[i]) {
			pr_err("Failed to allocate memory for msg_str[%d]!\n", i);
			ret = -ENOMEM;
			goto free_msg;
		}
	}

	while (!kthread_should_stop()) {
		/* Listen for URMA messages */
		recv_msg_nodes = urma_recv(msg_str, URMA_SEND_DATA_MAX_LEN);
		if (recv_msg_nodes <= 0) {
			/*
			 * Prevent processes from entering the D state if reboot event
			 * occurs on the current node
			 */
			msleep_interruptible(MILLISECONDS_OF_EACH_MDELAY);
			continue;
		}

		pr_info("urma messages are received, the number of nodes that are successfully received is %d\n",
			recv_msg_nodes);

		for (i = 0; i < recv_msg_nodes; i++) {
			if (strcmp(HEARTBEAT, msg_str[i]) == 0 ||
			    strcmp(HEARTBEAT_ACK, msg_str[i]) == 0)
				continue;

			ret = create_kthread_to_process_msg(msg_str[i], COMM_TYPE_URMA);
			if (ret == -ENOMEM)
				goto free_msg;
		}

		/*
		 * Prevent processes from entering the D state if reboot event
		 * occurs on the current node
		 */
		msleep_interruptible(MILLISECONDS_OF_EACH_MDELAY);
	}

free_msg:
	free_char_array(msg_str, MAX_NODE_NUM);

	pr_info("Urma receiver thread stopped!\n");
	return ret;
}

/**
 * cis_ubios_remote_msg_cb - UVB remote message callback
 * @cis_msg: CIS message from UVB
 *
 * Return: 0 on success, negative error code on failure
 *
 * This function serves as the callback for UVB remote messages,
 * processing incoming messages through the appropriate mechanism.
 */
int cis_ubios_remote_msg_cb(struct cis_message *cis_msg)
{
	int ret;

	pr_info("uvb get msg: [%s]\n", (char *)cis_msg->input);
	ret = create_kthread_to_process_msg((char *)cis_msg->input, COMM_TYPE_UVB);
	return ret;
}

/**
 * sentry_panic_reporter_init - Initialize sentry panic reporter module
 *
 * Return: 0 on success, negative error code on failure
 */
int sentry_panic_reporter_init(void)
{
	atomic_set(&sentry_remote_ctx.remote_event_ack_received, 0);
	atomic_set(&sentry_remote_ctx.remote_event_ack_done, 0);

	sentry_remote_ctx.urma_receiver_thread = kthread_run(process_urma_data, NULL, "sentry_urma_kthread");
	if (IS_ERR(sentry_remote_ctx.urma_receiver_thread)) {
		pr_err("Failed to create kernel urma receiver thread.\n");
		return PTR_ERR(sentry_remote_ctx.urma_receiver_thread);
	}

	pr_info("Create kernel urma receiver thread success.\n");
	return 0;
}

/**
 * sentry_panic_reporter_exit - Cleanup sentry panic reporter module
 */
void sentry_panic_reporter_exit(void)
{
	if (sentry_remote_ctx.urma_receiver_thread) {
		kthread_stop(sentry_remote_ctx.urma_receiver_thread);
		sentry_remote_ctx.urma_receiver_thread = NULL;
		pr_info("Kernel urma receiver thread stopped\n");
	}
}
