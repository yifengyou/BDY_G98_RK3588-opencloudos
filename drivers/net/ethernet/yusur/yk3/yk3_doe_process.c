// SPDX-License-Identifier: GPL-2.0
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <asm/barrier.h>
#include <linux/etherdevice.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/uaccess.h>
#include <linux/iopoll.h>
#include <linux/llist.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/sort.h>

#include "yk3_cdev_priv.h"
#include "yk3_doe_kapi.h"
#include "yk3_doe_process.h"

#if YK3_SKIP_DOE
struct yk3_doe_priv __doe_skip;
#endif

const struct yk3_doe_xie_spec  yk3_doe_xie_specs[YK3_DOE_XIE_END] = YK3_DOE_XIE_SPEC_INITER;
const struct yk3_doe_table_map yk3_doe_table_maps[DOE_TABLE_END] = YK3_DOE_TABLE_MAP_INITER;

struct yk3_doe_cfg doe_cfg = DOE_CFG_INITER;
/**************************************************************************************************/
static void
doe_cmd_wait_clean(struct yk3_doe_if *doe_if,
		   struct yk3_doe_cmd *cmd, bool is_sent, bool is_timeout)
{
	struct yk3_doe_priv *doe_priv  = doe_if->doe_priv;
	struct yk3_doe_cmd *sub, *tmp;
	bool is_debug = doe_priv->init && is_sent;

	// wait clean
	//
	// container cmd: cmd is stack
	// single    cmd: cmd is stack
	// sub       cmd: impossible
	if (unlikely(is_doe_cmd_alloced(cmd) || is_doe_cmd_sub(cmd))) {
		doe_tag_pool_dump(&doe_if->cmd_manager.tag_pool);
		doe_bug(-E_DOE_BUG, "%s cmd: " DOE_CMD_FMT, doe_cmd_name(cmd),
			cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
		return;
	}

	// single without sub
	if (is_doe_cmd_single(cmd)) {
		if (is_debug) {
			if (doe_if->is_read)
				doe_debug(DOE_DEBUG_CSR, "Release %s %s cmd " DOE_CMD_FMT,
					  (is_timeout ? "timeout" : "complete"),
					  doe_cmd_name(cmd),
					  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
			else
				doe_debug(DOE_DEBUG_CSW, "Release %s %s cmd " DOE_CMD_FMT,
					  (is_timeout ? "timeout" : "complete"),
					  doe_cmd_name(cmd),
					  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
		}

#if YK3_DOE_RAW_CMD
		kfree(cmd->raw);
#endif
		doe_cmd_unregister(doe_if, cmd, is_timeout);
		return;
	}

	// container with sub
	list_for_each_entry_safe(sub, tmp, &cmd->list, node) {
		if (is_debug) {
			if (doe_if->is_read)
				doe_debug(DOE_DEBUG_CSR, "Release %s %s cmd " DOE_CMD_FMT,
					  (is_timeout ? "timeout" : "complete"),
					  doe_cmd_name(sub),
					  sub->seq, sub->tag, sub->flags, sub->opcode, sub->tbl_id);
			else
				doe_debug(DOE_DEBUG_CSW,
					  "Release %s %s cmd " DOE_CMD_FMT,
					  (is_timeout ? "timeout" : "complete"),
					  doe_cmd_name(sub),
					  sub->seq, sub->tag, sub->flags, sub->opcode, sub->tbl_id);
		}

		list_del(&sub->node);
		doe_cmd_free(doe_if, sub, is_timeout);
	}
}

// true : is timeout
// false: NOT timeout
static bool
doe_cmd_wait_busy(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, u64 ns_timeout)
{
	u64 ns_start = ktime_get_ns();
#if YK3_DEBUG
	if (unlikely(is_doe_inject_cmd_timeout(is_doe_cmd_read(cmd))))
		return true;
#endif

	while (!is_doe_timeout(ns_start, ns_timeout)) {
		if (unlikely(is_doe_hw_failure(doe_priv)))
			return true;

		if (READ_ONCE(cmd->wait.done)) {
			// NOT timeout
			return false;
		}

		cpu_relax();
	}

	// avoid cpu stuck
	usleep_range(1, 2);
	// timeout
	return true;
}

// true : is timeout
// false: NOT timeout
static bool
doe_cmd_wait_sleep(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, u64 ns_timeout)
{
	unsigned long left;
#if YK3_DEBUG
	if (unlikely(is_doe_inject_cmd_timeout(is_doe_cmd_read(cmd))))
		return true;
#endif

	if (is_doe_hw_failure(doe_priv))
		return true;

	left = wait_for_completion_timeout(&cmd->wait.complete, nsecs_to_jiffies(ns_timeout));
	// =0, timeout(true)
	// >0, NOT timeout(false)
	return !left;
}

// true : is timeout
// false: NOT timeout
static inline bool
doe_cmd_wait(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, u64 ns_timeout)
{
	return is_doe_cmd_wait_sleep(cmd)
		? doe_cmd_wait_sleep(doe_priv, cmd, ns_timeout)
		: doe_cmd_wait_busy(doe_priv, cmd, ns_timeout);
}

//  true: timeout
// false: NOT timeout
static bool
doe_cmd_wait_finish(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	u64 timeout = doe_opcode_timeout(cmd->opcode);
	bool is_container = is_doe_cmd_container(cmd);
	bool is_hw_ok, is_timeout;

	// wait cmd complete
	is_timeout = doe_cmd_wait(doe_priv, cmd, timeout);
#if YK3_DOE_IRQ_LOSS && (YK3_DOE_IRQ == YK3_DOE_IRQ_TASKLET)
	if (is_timeout) {
		// maybe irq miss
		tasklet_schedule(&cmd->doe_if->tasklet);

		is_timeout = doe_cmd_wait(doe_priv, cmd, timeout);
	}
#endif
	is_hw_ok = !is_doe_hw_failure(doe_priv);
	if (is_hw_ok) {
		if (is_timeout && is_container) {
			// only container report timeout error
			doe_err("%s cmd Timeout[%llums] "
				DOE_CMD_CONTAINER_FMT
				"@ irq trigger[%llu]",
				doe_cmd_name(cmd), (timeout / 1000000),
				cmd->cnt, cmd->flags, cmd->opcode, cmd->tbl_id,
				(u64)atomic64_read(&cmd->doe_if->st.irq_trigger));
		}

		// container cmd: maybe timeout
		// single    cmd: NOT timeout forever
		if (cmd->err)
			doe_err("Total:%d success:%d failed:%d hw-err:0x%x.",
				cmd->cnt, cmd->succeed, cmd->failed, cmd->err);
		else if (is_timeout && is_container)
			doe_err("Total:%d success:%d failed:%d @ irq trigger[%llu]",
				cmd->cnt, cmd->succeed, cmd->failed,
				(u64)atomic64_read(&cmd->doe_if->st.irq_trigger));
	}

	if (is_timeout) {
		if (cmd->opcode == YK3_DOE_SW_CREATE_ARRAY) {
			// rollback to disable
			doe_table_disable(doe_priv, cmd->tbl_id);
			atomic_sub(1, &doe_priv->active_table_count);
		}

		if (cmd->opcode == YK3_DOE_SW_CREATE_HASH) {
			// rollback to disable
			doe_table_disable(doe_priv, cmd->tbl_id);
			atomic_sub(1, &doe_priv->hash_table_count);
			atomic_sub(1, &doe_priv->active_table_count);
		}

		return true;
	}

	return false;
}

//申请并提交一个cmd
static void
doe_cmd_submit(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd, struct yk3_doe_cmd *parent)
{
	cmd->doe_if = doe_if;
	cmd->parent = parent;

	if (parent) {
		// push both normal and spliter
		doe_push_subcmd(parent, cmd);
		doe_cmd_wait_init(parent);
	} else {
		// cnt of single cmd must initial with 1 for wakeup condition!
		cmd->cnt = 1;
		doe_cmd_wait_init(cmd);
	}

	if (doe_if->is_read)
		doe_debug(DOE_DEBUG_CTR, "Submit %s %s cmd " DOE_CMD_FMT,
			  doe_if->name, doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	else
		doe_debug(DOE_DEBUG_CTW, "Submit %s %s cmd " DOE_CMD_FMT,
			  doe_if->name, doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
}

static int
doe_enqueue_cmdbuffer(struct yk3_doe_if *doe_if,
		      struct yk3_doe_cmd *cmd,
		      struct yk3_doe_cmd_buffer *cb)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	struct yk3_doe_hw_cmd_header *hdr;
	union yk3_doe_hw_cmd_body *body;
	u16 key_len, dov_len, cmd_len, tbl_id = cmd->tbl_id;
#if YK3_DOE_RAW_CMD
	u16 raw_datalen;
#endif

	if (unlikely(!is_doe_cb_enough(cb))) {
		// {}
		return doe_error(-E_DOE_FULL,
				 "enqueue cmd buffer full(id: %u, size: %u, count: %u, used: %u)",
				 cb->id, cb->size, cb->cmd_count, cb->cmd_space);
	}

	key_len = doe_priv->param[tbl_id].key_len;
	dov_len = doe_priv->param[tbl_id].dov_len;
	// cmd_len NOT include header
	// so, only include opcode and table id @ init
	cmd_len = sizeof(*hdr) - DOE_HW_CMD_HDRLEN;

	hdr = doe_cb_cursor(cb);
	body = (union yk3_doe_hw_cmd_body *)(hdr + 1);

	hdr->tag	= cpu_to_le16(cmd->tag);
	hdr->valid	= cpu_to_le16(1);
	hdr->status	= 0;
	hdr->opcode	= cmd->opcode;
	hdr->tbl_id	= tbl_id;

	switch (cmd->opcode) {
#if YK3_DOE_RAW_CMD
	case YK3_DOE_SW_RAW_CMD:
		raw_datalen = cmd->raw_len - 2;
		memcpy(&hdr->opcode, cmd->raw->data, raw_datalen);
		cmd_len += raw_datalen;

		/* Transter the user-define opcode for cmd-complete */
		hdr->tbl_id	= cmd->raw->tbl_id;
		cmd->tbl_id	= cmd->raw->tbl_id;
		break;
#endif
	case YK3_DOE_SW_ARRAY_LOAD:
	case YK3_DOE_SW_ARRAY_READ:
		body->item_id = cpu_to_le32(cmd->index);
		cmd_len += sizeof(body->item_id);
		break;

	case YK3_DOE_SW_ARRAY_STORE:
		hdr->status |= GEN_HEAD_PRIORITY(cmd->high_pri);
		fallthrough;
	case YK3_DOE_SW_ARRAY_WRITE: // zero clear use array write
		hdr->status |= GEN_HEAD_ENABLE(cmd->enable);

		// for zero clear
		//	cmd->index is cmd->target_table_id
		//	cmd->value is &cmd->zero_clear
		//	dov_len is sizeof(cmd->zero_clear)
		body->item_id = cpu_to_le32(cmd->index);
		memcpy(body->value, cmd->value, dov_len);
		cmd_len += sizeof(body->item_id) + dov_len;
		break;
	case YK3_DOE_SW_HASH_QUERY:
	case YK3_DOE_SW_HASH_DELETE:
		memcpy(body->body, cmd->hash.key, key_len);
		cmd_len += key_len;
		break;

	case YK3_DOE_SW_HASH_INSERT:
		hdr->status |= GEN_HEAD_PRIORITY(cmd->high_pri);
		fallthrough;
	case YK3_DOE_SW_HASH_UPDATE:
	case YK3_DOE_SW_HASH_SAVE:
		hdr->status |= GEN_HEAD_ENABLE(cmd->enable);

		memcpy(body->body + 0,	     cmd->hash.key,   key_len);
		memcpy(body->body + key_len, cmd->hash.value, dov_len);
		cmd_len += key_len + dov_len;
		break;
	case YK3_DOE_SW_COUNTER_ENABLE: // TODO: with data ?
		hdr->opcode = YK3_DOE_ARRAY_STORE;
		hdr->status |= GEN_HEAD_PRIORITY(cmd->high_pri);
		hdr->status |= GEN_HEAD_ENABLE(cmd->enable);

		body->item_id = cpu_to_le32(cmd->index);
		memcpy(body->value, cmd->value, dov_len);
		cmd_len += sizeof(body->item_id) + dov_len;
		break;
	default:
		return doe_bug(-E_DOE_BUG, "unknown cmd opcode[0x%x]", cmd->opcode);
	}

	if (doe_if->is_read)
		doe_debug(DOE_DEBUG_CTR, "%s Push cb[%u] %dth cmd[%u~%u], " DOE_CMD_FMT,
			  doe_if->name, cb->id, cb->cmd_count,
			  cb->cmd_space, cb->cmd_space + DOE_CMD_SPACE(cmd_len),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	else
		doe_debug(DOE_DEBUG_CTW, "%s Push cb[%u] %dth cmd[%u~%u], " DOE_CMD_FMT,
			  doe_if->name, cb->id, cb->cmd_count,
			  cb->cmd_space, cb->cmd_space + DOE_CMD_SPACE(cmd_len),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);

	hdr->cmd_len = cpu_to_le16(cmd_len);
	doe_cb_push(cb, cmd_len);
	doe_cmd_measure_start(cmd);

	return 0;
}

//下发cmd buffer
static int
doe_send_cmdbuffer(struct yk3_doe_if *doe_if, struct yk3_doe_cmd_buffer *cb)
{
	struct yk3_doe_priv *doe_priv = doe_if->doe_priv;
	u32 len;
	int ret;

	/* Let the cmd buffer alignment */
	len = cb->cmd_space;

	/* If there is the reserved 32 Bytes data, set bit16(CMD_VALID) to 0 */
	if (unlikely(len & YK3_DOE_CMD_ALIGN_MASK))
		yk3_memzero(doe_cb_cursor(cb), YK3_DOE_CMD_ALIGN_SIZE);

	ret = doe_fifo_enqueue(doe_if, cb);
	if (!ret && doe_priv->init) {
		if (doe_if->is_read)
			doe_debug_buffer(DOE_DEBUG_CBR,
					 cb->base, cb->cmd_space,
					 "Send %s cmd buffer 0x%llx.%d",
					 doe_if->name, YK3_PTR(cb->base), cb->cmd_space);
		else
			doe_debug_buffer(DOE_DEBUG_CBW,
					 cb->base, cb->cmd_space,
					 "Send %s cmd buffer 0x%llx.%d",
					 doe_if->name, YK3_PTR(cb->base), cb->cmd_space);
	}

	return ret;
}

static int
doe_process_cmdbuffer(struct yk3_doe_if *doe_if, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_cmd_buffer *cb = NULL;
	struct yk3_doe_cmd *sub;
	int ret;

	ret = doe_cb_pool_get(doe_if, &cb);
	if (ret < 0)
		return ret;

	if (is_doe_cmd_container(cmd)) {
		// container cmd
		list_for_each_entry(sub, &cmd->list, node) {
			ret = doe_enqueue_cmdbuffer(doe_if, sub, cb);
			if (ret < 0)
				goto error;
		}
	} else {
		// sub/single cmd
		ret = doe_enqueue_cmdbuffer(doe_if, cmd, cb);
		if (ret < 0)
			goto error;
	}

	ret = doe_send_cmdbuffer(doe_if, cb);
	if (ret < 0)
		goto error;

	return 0;
error:
	// NOT clean cb here, do it @ get
	doe_cb_pool_put(doe_if, cb);

	return ret;
}

static void
doe_cmd_wakeup(struct yk3_doe_cmd *cmd)
{
	bool is_read = is_doe_cmd_read(cmd);

	if (unlikely(is_doe_cmd_container(cmd))) {
		if (is_read)
			doe_debug(DOE_DEBUG_ESR,
				  "Wakeup container cmd: opcode[0x%x] tbl_id[0x%x] cnt[%u]",
				  cmd->opcode, cmd->tbl_id, cmd->cnt);
		else
			doe_debug(DOE_DEBUG_ESW,
				  "Wakeup container cmd: opcode[0x%x] tbl_id[0x%x] cnt[%u]",
				  cmd->opcode, cmd->tbl_id, cmd->cnt);
	} else {
		if (is_read)
			doe_debug(DOE_DEBUG_ESR,
				  "Wakeup cmd: " DOE_CMD_FMT,
				  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
		else
			doe_debug(DOE_DEBUG_ESW,
				  "Wakeup cmd: " DOE_CMD_FMT,
				  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	}

	doe_cmd_wait_done(cmd);
}

//收到event完成事件时，
//需要从event事件中拷贝指令返回的数据，以及内存资源回收
static int
doe_cmd_complete_r(struct yk3_doe_cmd *cmd, struct yk3_doe_event *event)
{
	struct yk3_doe_priv *doe_priv = cmd->doe_if->doe_priv;
	struct yk3_doe_cmd *parent = cmd->parent;
	bool is_hash_read = false;
	bool is_array_read = false;
	u16 dov_len = doe_priv->param[cmd->tbl_id].dov_len;
	u8 opcode = cmd->opcode;
	u8 hw_err = event->status;

#if YK3_DOE_RAW_CMD
	if (opcode == YK3_DOE_SW_RAW_CMD)
		opcode = cmd->raw->opcode;
#endif

	switch (opcode) {
	case YK3_DOE_SW_ARRAY_LOAD:
	case YK3_DOE_SW_ARRAY_READ:
		is_array_read = true;
		break;
	case YK3_DOE_SW_HASH_QUERY:
		is_hash_read = true;
		break;
	}

	if (unlikely(parent)) {
		doe_bug(-E_DOE_NOTSUPP, "NOT support batch read, " DOE_CMD_FMT,
			cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);

		// sub cmd
		if (!hw_err)
			doe_subcmd_successed(parent, cmd);
		else
			doe_subcmd_failed(parent, cmd, hw_err);

		if (is_doe_cmd_complete(parent))
			doe_cmd_wakeup(parent);

		return 0;
	}

	// single cmd
	if (unlikely(!is_doe_cmd_sync(cmd)))
		doe_bug(-E_DOE_BUG_RD_NOT_SYNC, "read must sync, " DOE_CMD_FMT,
			cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);

	if (!hw_err) {
#if YK3_DOE_RAW_CMD
		/* For user raw debug cmd, dov-len is uninitialized */
		if (!dov_len)
			dov_len = 128;
#endif

		if (is_hash_read) {
			memcpy(cmd->hash.value, (event + 1), dov_len);
		} else if (is_array_read) {
			memcpy(cmd->value, (event + 1), dov_len);
		} else {
			doe_bug(-E_DOE_BUG,
				"Invalid single cmd(not hash/array read), " DOE_CMD_FMT,
				cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
		}

		doe_cmd_successed(cmd);
	} else {
		// {}
		doe_cmd_failed(cmd, hw_err);
	}

	doe_cmd_wakeup(cmd);

	return 0;
}

static int
doe_cmd_complete_w(struct yk3_doe_cmd *cmd, struct yk3_doe_event *event)
{
	struct yk3_doe_cmd *parent = cmd->parent;
	u8 hw_err = event->status;

	if (unlikely(parent)) {
		// sub cmd
		if (!hw_err)
			doe_subcmd_successed(parent, cmd);
		else
			doe_subcmd_failed(parent, cmd, hw_err);

		if (is_doe_cmd_complete(parent))
			doe_cmd_wakeup(parent);

		return 0;
	}

	// single cmd
	if (!hw_err)
		doe_cmd_successed(cmd);
	else
		doe_cmd_failed(cmd, hw_err);

	if (is_doe_cmd_fast(cmd)) {
		doe_cmd_put(cmd->doe_if, cmd, true);

		if (unlikely(hw_err)) {
			doe_error(doe_hw_err(hw_err),
				  "fast cmd ack error, " DOE_CMD_FMT,
				  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
			cmd->doe_if->doe_priv->fast.n_ack_error++;
		}
	} else {
		// {}
		doe_cmd_wakeup(cmd);
	}

	return 0;
}

static int
doe_process_event_unknown(struct yk3_doe_if *doe_if, u16 tag)
{
	struct yk3_doe_cmd_manager *manager = &doe_if->cmd_manager;
	struct yk3_doe_tag_slot *slot = &manager->tag_slot[tag];

	if (slot->flags & DOE_CMD_F_TIMEOUT) {
		// slot is timeout, tag have put, cmd have clean
		// the event is just lated, NOT unknown
		slot->times_lated++; // only one writer
		slot->times_put++;
		doe_tag_slot_clean(slot);

		// put the tag(lated)
		spin_lock(&manager->lock);
		doe_tag_pool_put(&manager->tag_pool, tag);
		spin_unlock(&manager->lock);
		return 0;
	}

	slot->times_unknown++; // only one writer
	if (!doe_if->st.event_unknown++) {
		doe_tag_slot_dump(manager->tag_slot);
		return doe_bug(-E_DOE_BUG, "%s Unknown Event tag[0x%04x]", doe_if->name, tag);
	}

	return doe_error(-E_DOE_BUG, "%s Unknown Event tag[0x%04x]", doe_if->name, tag);
}

//处理一个完成事件
static int
doe_process_event_r(struct yk3_doe_if *doe_if, struct yk3_doe_event *event)
{
	struct yk3_doe_cmd *cmd;
	u16 tag = le16_to_cpu(event->tag);

	cmd = doe_cmd_take(doe_if, tag);
	if (unlikely(!cmd))
		return doe_process_event_unknown(doe_if, tag);

	doe_debug(DOE_DEBUG_ETR, "%s Recv Ack with status[%u] %s cmd: " DOE_CMD_FMT,
		  doe_if->name, event->status, doe_cmd_name(cmd),
		  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	doe_cmd_complete_r(cmd, event);

	return 0;
}

static int
doe_process_event_w(struct yk3_doe_if *doe_if, struct yk3_doe_event *event)
{
	struct yk3_doe_cmd *cmd;
	u16 tag = le16_to_cpu(event->tag);

	cmd = doe_cmd_take(doe_if, tag);
	if (unlikely(!cmd))
		return doe_process_event_unknown(doe_if, tag);

	doe_debug(DOE_DEBUG_ETW, "%s Recv Ack with status[%u] %s cmd: " DOE_CMD_FMT,
		  doe_if->name, event->status, doe_cmd_name(cmd),
		  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	doe_cmd_complete_w(cmd, event);

	return 0;
}

/*
 *接收event完成事件，应进行资源回收和完成数据拷贝及命令状态更新。
 *本函数在中断回调函数被调用
 */
int doe_event_handler_r(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_evq *evq = &doe_if->evq;
	union yk3_doe_qvq_block block;
	bool init = doe_if->doe_priv->init;
	int i;
	u64 old_cons = evq->cons;
	u64 old_event_count = evq->event_count;
	u32 cons, prod = doe_evq_prod(evq);
	u32 block_count;
	u32 times = YK3_DOE_IRQ_TIMES;

	// calc event block count(NOT consumed)
	//	doe_evq_count read the hardware prod cursor @ the DMA
	block_count = doe_evq_count(evq);
	if (!block_count)
		return 0;
try_again:
	doe_debug(DOE_DEBUG_ETR, "%s New %d Event Block @ prod[%u] Cons[%llu] event-count[%llu]",
		  doe_if->name, block_count, prod, old_cons, old_event_count);

	dma_rmb(); // before first read prod desc ?

	for (i = 0; i < block_count; i++) {
		cons = doe_evq_cons(evq);
		block.addr = doe_evq_block_address(evq, cons);
		if (init)
			doe_debug_buffer(DOE_DEBUG_EBR,
					 block.addr, 256,
					 "%s The %dth@%u Event Block[0x%llx] prod[%u] cons[%u]",
					 doe_if->name, i, block_count,
					 YK3_PTR(block.addr), prod, cons);
		else
			doe_debug(DOE_DEBUG_EBR,
				  "%s The %dth@%u Event Block[0x%llx] prod[%u] cons[%u]",
				  doe_if->name, i, block_count,
				  YK3_PTR(block.addr), prod, cons);

		// 1 event per block @ read
		doe_process_event_r(doe_if, &block.r->hdr);
		doe_evq_consume(evq, 1);
	}

	doe_debug(DOE_DEBUG_ETR,
		  "%s After %d Event Block @ prod[%u] cons[%llu==>%llu] event-count[%llu==>%llu]",
		  doe_if->name, block_count,
		  prod, old_cons, evq->cons,
		  old_event_count, evq->event_count);

	block_count = doe_evq_count(evq);
	if (unlikely(block_count)) {
		if (times--) {
			doe_if->st.event_again++;
			goto try_again;
		}

		doe_if->st.event_left++;
	}

	return block_count;
}

int doe_event_handler_w(struct yk3_doe_if *doe_if)
{
	struct yk3_doe_evq *evq = &doe_if->evq;
	union yk3_doe_qvq_block block;
	bool init = doe_if->doe_priv->init;
	int i, j;
	u64 old_cons = evq->cons;
	u64 old_event_count = evq->event_count;
	u32 cons, prod = doe_evq_prod(evq);
	u32 block_count, event_count, batch_count;
	u32 times = YK3_DOE_IRQ_TIMES;

	// calc event block count(NOT consumed)
	//	doe_evq_count read the hardware prod cursor @ the DMA
	block_count = doe_evq_count(evq);
	if (!block_count)
		return 0;
try_again:
	doe_debug(DOE_DEBUG_ETW, "%s New %d Event Block @ prod[%u] Cons[%llu] event-count[%llu]",
		  doe_if->name, block_count, prod, old_cons, old_event_count);

	dma_rmb(); // before first read prod desc ?

	batch_count = evq->batch_count;

	for (i = 0; i < block_count; i++) {
		cons = doe_evq_cons(evq);
		block.addr = doe_evq_block_address(evq, cons);
		// multi event per block @ write
		event_count = le32_to_cpu(block.w->count);
		if (unlikely(event_count > batch_count)) {
			doe_error(-E_DOE_EXCEED,
				  "%s evq recv event count[%u] over limit[%u]",
				  doe_if->name, event_count, batch_count);
			event_count = batch_count;
		}

		if (init) {
#undef  zip_fmt
#define zip_fmt	"%s The %dth@%u Event Block[0x%llx] prod[%u] cons[%u] with %u event"
			// {}
			doe_debug_buffer(DOE_DEBUG_EBW, block.addr, 256, zip_fmt,
					 doe_if->name, i, block_count,
					 YK3_PTR(block.addr), prod, cons, event_count);

		} else {
#undef  zip_fmt
#define zip_fmt "%s The %dth@%u Event Block[0x%llx] prod[%u] cons[%u] with %u event"
			// {}
			doe_debug(DOE_DEBUG_EBW, zip_fmt,
				  doe_if->name, i, block_count,
				  YK3_PTR(block.addr), prod, cons, event_count);
		}

		for (j = 0; j < event_count; j++)
			doe_process_event_w(doe_if, &block.w->events[j]);

		doe_evq_consume(evq, event_count);
	}

	doe_debug(DOE_DEBUG_ETW,
		  "%s After %d Event Block @ prod[%u] Cons[%llu==>%llu] event-count[%llu==>%llu]",
		  doe_if->name, block_count,
		  prod, old_cons, evq->cons,
		  old_event_count, evq->event_count);

	block_count = doe_evq_count(evq);
	if (unlikely(block_count)) {
		if (times--) {
			doe_if->st.event_again++;
			goto try_again;
		}

		doe_if->st.event_left++;
	}

	return block_count;
}

struct yk3_doe_mm *
doe_mm_create(struct yk3_doe_priv *doe_priv, u64 size, u32 align_mask, bool hmc, const char *name)
{
	struct yk3_doe_mm *ymm;

	// not care align, use devm
	ymm = devm_kzalloc(doe_priv->dev, sizeof(*ymm), GFP_KERNEL);
	if (!ymm)
		return ERR_PTR(-E_DOE_NOMEM);

	ymm->doe_priv = doe_priv;
	ymm->dev = doe_priv->dev;
	ymm->base_address = 0;
	ymm->total_size = size;
	ymm->align_mask = align_mask;
	ymm->name = name;
	ymm->used_list = NULL;

	/* init the head of free_list with total size */
	// not care align, use devm
	ymm->free_list = devm_kzalloc(doe_priv->dev, sizeof(*ymm->free_list), GFP_KERNEL);
	if (!ymm->free_list)
		return ERR_PTR(-E_DOE_NOMEM);

	ymm->free_list->address = 0;
	ymm->free_list->size = size;
	ymm->free_list->next = NULL;

	ymm->hmc = hmc;
	if (hmc) {
		ymm->mem_ptr = doe_dmam_zalloc_coherent(doe_priv, size, &ymm->dma_base);
		if (!ymm->mem_ptr) {
			doe_error(-E_DOE_NOMEM,
				  "mm:%s alloc dma size[0x%llx] no memory", name, size);
			return ERR_PTR(-E_DOE_NOMEM);
		}
	}

	return ymm;
}

s64 doe_mm_malloc(struct yk3_doe_mm *ymm, int tbl_id, u64 size)
{
	struct mm_region *mr, *new_mr = NULL, *prev = NULL;

	/* let the region size align */
	size = (size + ymm->align_mask) & ~ymm->align_mask;

	if (!ymm->free_list)
		goto error;

	/* traverse free_list to find enough space */
	mr = ymm->free_list;
	while (mr) {
		if (mr->size == size) {
			/* change the head node if first MR is used */
			if (!prev)
				ymm->free_list = mr->next;
			else
				prev->next = mr->next;

			new_mr = mr;
			new_mr->tbl_id = tbl_id;
			break;
		} else if (mr->size > size) {
			/* alloc new MR node for allocation */
			// not care align, use devm
			new_mr = devm_kzalloc(ymm->doe_priv->dev, sizeof(*new_mr), GFP_KERNEL);
			if (!new_mr) {
				// {}
				return doe_error(-E_DOE_NOMEM,
						 "mm: %s alloc mr failed @ table: %d size: 0x%llx",
						 ymm->name, tbl_id, size);
			}
			new_mr->size = size;
			new_mr->address = mr->address;
			new_mr->tbl_id = tbl_id;

			/* decrease the size of free MR */
			mr->size -= size;
			mr->address += size;
			break;
		}

		prev = mr;
		mr = mr->next;
	}

	if (!new_mr)
		goto error;

	/* add node to the head of used_list */
	new_mr->next = ymm->used_list;
	ymm->used_list = new_mr;

	doe_mm_dump(ymm);

	return new_mr->address;
error:
	return doe_error(-E_DOE_NOMEM, "mm: %s alloc table: %d size: 0x%llx failed",
			 ymm->name, tbl_id, size);
}

/* Insert memory region in address order */
static int
doe_mm_insert(struct yk3_doe_mm *ymm, struct mm_region *prev,
	      struct mm_region *new_mr, struct mm_region *mr)
{
	/* find the previous MR less than new_mr */
	if (prev && new_mr->address < prev->address)
		return -1;

	/* find the current MR bigger than new_mr */
	if (mr && new_mr->address > mr->address)
		return -1;

	/* now find the place to insert */
	new_mr->next = mr;

	/* maybe can merge with next_mr */
	if (mr && (new_mr->address + new_mr->size == mr->address)) {
		new_mr->size += mr->size;
		new_mr->next = mr->next;
	}

	if (prev) {
		/* maybe can merge to prev_mr */
		if (prev->address + prev->size == new_mr->address) {
			prev->size += new_mr->size;
			prev->next = new_mr->next;
		} else {
			prev->next = new_mr;
		}
	} else {
		/* insert as the head of free_list */
		ymm->free_list = new_mr;
	}

	return 0;
}

void doe_mm_free(struct yk3_doe_mm *ymm, int tbl_id, u64 address)
{
	struct mm_region *mr, *new_mr = NULL, *prev = NULL;

	if (!ymm)
		return;

	/* traverse used_list to find the target MR */
	new_mr = ymm->used_list;
	while (new_mr) {
		if (new_mr->address == address)
			break;
		prev = new_mr;
		new_mr = new_mr->next;
	}
	if (!new_mr) {
		doe_warn("mm %s free Unknowned table: %d address: 0x%llx",
			 ymm->name, tbl_id, address);
		return;
	}

	/* delete new_mr from used list */
	if (!prev)
		ymm->used_list = new_mr->next;
	else
		prev->next = new_mr->next;

	/* traverse free_list to insert the new free_MR */
	prev = NULL;
	if (!ymm->free_list) {
		ymm->free_list = new_mr;
		new_mr->next = NULL;
	} else {
		mr = ymm->free_list;
		while (doe_mm_insert(ymm, prev, new_mr, mr)) {
			prev = mr;
			mr = mr->next;
		}
	}

	doe_mm_dump(ymm);
}

static void
doe_set_page_map(struct yk3_doe_priv *doe_priv, u32 page_index, u32 page_size, u64 page_dma)
{
	u32 off = YK3_DOE_REG_PAGEMAP_ENTRY(page_index);
	u32 reg[2] = {
		[0] = off + YK3_DOE_REG_PAGEMAP_ADDR_L,
		[1] = off + YK3_DOE_REG_PAGEMAP_ADDR_H,
	};
	u32 addr[2] = {
		[0] = page_dma & 0xFFFFFFFF,
		[1] = page_dma >> 32,
	};

	doe_wr32(doe_priv, reg[0], addr[0]);
	doe_wr32(doe_priv, reg[1], addr[1]);

	if (page_size) {
		doe_debug(DOE_DEBUG_INIT,
			  "hmc pagemap[%u] reg[0x%x:0x%x] address=[0x%x:0x%x] size=0x%x",
			  page_index, reg[0], reg[1], addr[0], addr[1], page_size);
	}
}

static inline u32
doe_page_size_order(u32 page_size)
{
	u32 i = 0;

	page_size /= 4096;
	for (i = 0; (page_size >> i) > 1; i++)
		;

	return i;
}

static void
doe_set_page_info(struct yk3_doe_priv *doe_priv, u32 i_info, bool on,
		  u32 page_size, u64 start_addr, u64 end_addr, u32 first_page_index)
{
	u32 page_order = doe_page_size_order(page_size);
	union yk3_doe_pageinfo_hw info = {
		.page_valid	= on,
		.page_order	= page_order,
		.addr_begin	= start_addr >> YK3_DOE_PAGE_SHIFT,
		.addr_end	= end_addr >> YK3_DOE_PAGE_SHIFT,
		.index		= first_page_index,
	};
	u32 base = YK3_DOE_REG_PAGEINFO_ENTRY(i_info);
	int i;

	if (on)
		doe_debug(DOE_DEBUG_INIT,
			  "hmc pageinfo[%u] reg[0x%x] start=0x%x, end=0x%x, order=%u, index=%u",
			  i_info, base,
			  info.addr_begin, info.addr_end, info.page_order, info.index);

	for (i = 0; i < ARRAY_SIZE(info.field); i++) {
		// per field
		doe_wr32(doe_priv, base + i * sizeof(u32), info.field[i]);
		if (on)
			doe_debug(DOE_DEBUG_INIT,
				  "hmc pageinfo[%u.%d] reg[0x%lx] value[0x%x]",
				  i_info, i, base + i * sizeof(u32), info.field[i]);
	}
}

static void
doe_page_enable(struct yk3_doe_priv *doe_priv)
{
	u32 pf_id = doe_priv->pdev_priv->pf_id;

	doe_wr32(doe_priv, YK3_DOE_REG_ADDRMAP_PFID, YK3_DOE_ADDRMAP_PFID(pf_id));
	doe_debug(DOE_DEBUG_INIT, "hmc page pf[%u] reg[0x%x] value[%u]",
		  pf_id, YK3_DOE_REG_ADDRMAP_PFID, YK3_DOE_ADDRMAP_PFID(pf_id));

	doe_wr32(doe_priv, YK3_DOE_REG_ADDRMAP_ENABLE, 1);
	doe_debug(DOE_DEBUG_INIT, "hmc page enable reg[0x%x] value[1]", YK3_DOE_REG_ADDRMAP_ENABLE);
}

static void
doe_page_disable(struct yk3_doe_priv *doe_priv)
{
	int i;

	doe_wr32(doe_priv, YK3_DOE_REG_ADDRMAP_ENABLE, 0);

	for (i = 0; i < YK3_DOE_PAGE_LIMIT; i++)
		doe_set_page_map(doe_priv, i, 0, 0);

	for (i = 0; i < YK3_DOE_PAGE_INFO_COUNT; i++)
		doe_set_page_info(doe_priv, i, false, 0, 0, 0, 0);
}

static int
doe_page_cmp(const void *A, const void *B)
{
	const struct yk3_doe_page *a = A;
	const struct yk3_doe_page *b = B;

	if (a->dma > b->dma)
		return 1;
	else if (a->dma < b->dma)
		return -1;
	else
		return 0;
}

static void
doe_page_sort(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group2M = &doe_priv->page_group[DOE_PAGE_GROUP_2M];
	struct yk3_doe_page_group *group1G = &doe_priv->page_group[DOE_PAGE_GROUP_1G];

	if (group2M->page_count)
		sort(group2M->pages, group2M->page_count, sizeof(*group2M->pages),
		     doe_page_cmp, NULL);

	if (group1G->page_count)
		sort(group1G->pages, group1G->page_count, sizeof(*group1G->pages),
		     doe_page_cmp, NULL);
}

static int
doe_page_common_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_page_group *group;
	struct yk3_doe_page *page;
	u32 i;

	// 1. page sort by dma
	doe_page_sort(doe_priv);

	// 2. setup page map
	for (i = 0; i < doe_priv->page_count; i++) {
		page = &doe_priv->pages[i];

		doe_set_page_map(doe_priv, i, page->size, page->dma);
	}

	// 3. init page info
	for (i = 0; i < DOE_PAGE_GROUP_END; i++) {
		group = &doe_priv->page_group[i];
		if (group->page_count) {
			// {}
			doe_set_page_info(doe_priv, i, true,
					  group->page_size,
					  group->start,
					  group->end,
					  group->first_page_index);
		}
	}

	return 0;
}

int doe_page_init(struct yk3_doe_priv *doe_priv)
{
	int ret;

	// first disable ?
	// doe_page_disable(doe_priv);

	ret = doe_page_common_init(doe_priv);
	if (ret < 0)
		return ret;

	doe_page_enable(doe_priv);

	return 0;
}

void doe_page_fini(struct yk3_doe_priv *doe_priv)
{
	doe_page_disable(doe_priv);
}

static int
doe_xie_info_update(struct yk3_doe_priv *doe_priv, enum yk3_doe_xie xie, bool is_short, bool init)
{
	struct yk3_doe_xie_info *info = &doe_priv->xie_info[xie];
	struct yk3_doe_xie_info old;

	if (!init && info->cache_entry_count < info->cache_entry_limit) {
		// after init, have changed
		return doe_error(-E_DOE_USING, "%s is using, cache count[%d] < limit[%u]",
				 doe_xie_name(xie),
				 info->cache_entry_count, info->cache_entry_limit);
	}

	yk3_objcpy(&old, info);
	info->is_short = is_short;
	info->cache_entry_limit = doe_cache_entry_count(doe_priv, xie, is_short);
	info->cache_entry_count = info->cache_entry_limit;

	if (!init) {
		// log it after init
		doe_info("set %s cache_mode: %s ==> %s cache_entry_limit: %u ==> %u",
			 doe_xie_name(xie),
			 doe_cache_mode_name(old.is_short), doe_cache_mode_name(info->is_short),
			 old.cache_entry_limit, info->cache_entry_limit);
	}

	return 0;
}

static int
doe_set_xie_limit(struct yk3_doe_priv *doe_priv, enum yk3_doe_xie xie, bool is_short, bool init)
{
	struct yk3_doe_xie_info *info = &doe_priv->xie_info[xie];
	int ret;

	if (!init && info->is_short == is_short) {
		// NOT changed(skip init)
		return 0;
	}

	ret = doe_xie_info_update(doe_priv, xie, is_short, init);
	if (ret < 0)
		return ret;

	switch (xie) {
	case YK3_DOE_AIE:
		info->max_key_len = 0;
		info->max_value_len = YK3_DOE_AIE_DLEN(is_short);
		doe_wr32(doe_priv, YK3_DOE_REG_AIE_DLEN_LIMIT, (u32)is_short);
		break;
	case YK3_DOE_HIE:
		info->max_key_len = YK3_DOE_HIE_KLEN(is_short);
		info->max_value_len = YK3_DOE_HIE_VLEN(is_short);
		doe_wr32(doe_priv, YK3_DOE_REG_HIE_DLEN_LIMIT, (u32)is_short);
		break;
	default:
		yk3_do_nothing();
		break;
	}

	return 0;
}

void doe_xie_init(struct yk3_doe_priv *doe_priv)
{
	bool is_short = doe_priv->cfg.cache_mode;
	int i;

	for (i = 0; i < YK3_DOE_XIE_END; i++) {
		// init all xie
		doe_xie_info_update(doe_priv, i, is_short, true);
	}

	doe_set_xie_limit(doe_priv, YK3_DOE_AIE, is_short, true);
	doe_set_xie_limit(doe_priv, YK3_DOE_HIE, is_short, true);

	doe_priv->xie_info[YK3_DOE_LAIE].max_value_len = YK3_DOE_LAIE_DLEN;
	doe_priv->xie_info[YK3_DOE_CIE].max_value_len = YK3_DOE_CIE_DLEN;
	doe_priv->xie_info[YK3_DOE_MIE].max_value_len = YK3_DOE_MIE_DLEN;

	doe_priv->xie_info[YK3_DOE_LHIE].max_key_len = YK3_DOE_LHIE_KLEN;
	doe_priv->xie_info[YK3_DOE_LHIE].max_value_len = YK3_DOE_LHIE_VLEN;
}

//建表时的参数校验
static int
doe_check_create_param(struct yk3_doe_priv *doe_priv, u8 tbl_id,
		       struct yk3_doe_table_param *param)
{
	int tbl_type = param->is_small_array ? DOE_TABLE_SMALL_ARRAY : param->tbl_type;
	const struct yk3_doe_xie_info *info = &doe_priv->xie_info[param->xie];
	u32 active_table_count;

	/* check table id */
	if (tbl_id > YK3_DOE_USER_TABLE_LIMIT) {
		// {}
		return doe_error(-E_DOE_INVALID, "Invalid table ID %d!", tbl_id);
	}

	active_table_count = atomic_read(&doe_priv->active_table_count);
	if (active_table_count >= YK3_DOE_ACTIVE_TABLE_LIMIT) {
		// {}
		return doe_error(-E_DOE_EXCEED,
				 "too more active table count: %d", active_table_count);
	}

	if (param->key_len && param->key_len > info->max_key_len) {
		// {}
		return doe_error(-E_DOE_INVALID,
				 "Invalid tbl spec. type:%d, key_len=%d > %d",
				 param->tbl_type, param->key_len, info->max_key_len);
	}

	if (param->dov_len > info->max_value_len) {
		// {}
		return doe_error(-E_DOE_INVALID,
				 "Invalid tbl spec. type:%d, dov_len=%d > %d",
				 param->tbl_type, param->dov_len, info->max_value_len);
	}

	if (is_doe_hash(tbl_type) && !param->sdepth)
		return doe_error(-E_DOE_INVALID, "hash table sdepth zero");

	return 0;
}

//删表时的参数校验
static int
doe_check_delete_param(struct yk3_doe_priv *doe_priv, u8 tbl_id, struct yk3_doe_cmd *cmd)
{
	int tbl_type;

	if (!doe_table_try_disable(doe_priv, tbl_id)) {
		// {}
		return doe_error(-E_DOE_EXIST, "table %d NOT exist", tbl_id);
	}

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	if (cmd->opcode == YK3_DOE_SW_DELETE_ARRAY && !is_doe_array(tbl_type)) {
		// {}
		return doe_error(-E_DOE_INVALID, "delete array with table %d(NOT ARRAY)", tbl_id);
	}

	if (cmd->opcode == YK3_DOE_SW_DELETE_HASH && !is_doe_hash(tbl_type)) {
		// {}
		return doe_error(-E_DOE_INVALID, "delete hash with table %d(NOT HASH)", tbl_id);
	}

	return 0;
}

//表参数合法性校验
static int
doe_sw_cmd_valid(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	u8 tbl_id = cmd->tbl_id;
	bool is_hash = false;

	switch (cmd->opcode) {
	case YK3_DOE_SW_RAW_CMD:
	case YK3_DOE_SW_INIT_INDEX_VIEW:
	case YK3_DOE_SW_RESET_ALL_TABLE:
		return 0;
	/* create table */
	case YK3_DOE_SW_CREATE_ARRAY:
	case YK3_DOE_SW_CREATE_HASH:
		//建表时的参数校验
		return doe_check_create_param(doe_priv, tbl_id, &cmd->tbl_param);
	/* delete table */
	case YK3_DOE_SW_DELETE_ARRAY:
	case YK3_DOE_SW_DELETE_HASH:
		//删表时的参数校验
		return doe_check_delete_param(doe_priv, tbl_id, cmd);
	case YK3_DOE_SW_HASH_INSERT:
		is_hash = true;

		//维护快速返回模式的统计计数
		if (atomic_dec_return(&doe_priv->spec[tbl_id].free_entrys) < 0) {
			atomic_inc_return(&doe_priv->spec[tbl_id].free_entrys);
			return doe_error(-E_DOE_INVALID,
					 "no enough hash table[%d] free entries", tbl_id);
		}
		break;
	case YK3_DOE_SW_HASH_DELETE:
		is_hash = true;

		//维护快速返回模式的统计计数
		atomic_inc_return(&doe_priv->spec[tbl_id].free_entrys);
		break;
	case YK3_DOE_SW_HASH_QUERY:
	case YK3_DOE_SW_HASH_UPDATE:
		is_hash = true;
		break;
	case YK3_DOE_SW_HASH_SAVE:
		is_hash = true;
		break;
	case YK3_DOE_SW_ARRAY_LOAD:
	case YK3_DOE_SW_ARRAY_STORE:
	case YK3_DOE_SW_ARRAY_WRITE:
	case YK3_DOE_SW_ARRAY_READ:
		if (doe_priv->param[tbl_id].depth &&
		    cmd->index >= doe_priv->param[tbl_id].depth) {
			// ------
			return doe_error(-E_DOE_INVALID,
					 "array table[%d] index[%d] > depth[%d]", tbl_id,
					 cmd->index, doe_priv->param[tbl_id].depth);
		}

		break;
	default:
		break;
	}

	if (tbl_id >= YK3_DOE_USER_TABLE_LIMIT) {
		// {}
		return doe_error(-E_DOE_INVALID, "Too big tbl ID %d!", tbl_id);
	}

	if (!is_doe_table_enable(doe_priv, tbl_id)) {
		// {}
		return doe_error(-E_DOE_INVALID, "Disabled tbl ID %d!", tbl_id);
	}

	if (is_hash && !doe_priv->param[tbl_id].key_len) {
		// {}
		return doe_error(-E_DOE_INVALID, "Hash tbl ID %d key_len = 0", tbl_id);
	}

	if (!is_hash && doe_priv->param[tbl_id].key_len) {
		// just log
		doe_error(-E_DOE_INVALID, "Array tbl ID %d has key_len = %d",
			  tbl_id, doe_priv->param[tbl_id].key_len);
	}

	return 0;
}

//执行删表命令时，同步清cache data
static int
doe_clean_cache(struct yk3_doe_priv *doe_priv, u8 tbl_id)
{
	int tbl_type = doe_tbl_type(doe_priv, tbl_id);
	const struct yk3_doe_xie_spec *spec = doe_tbl_type_to_xie_spec(tbl_type);
	void __iomem *base = doe_priv->bar_base;
	u32 val, off = spec->reg_clean_cache;
	int ret;

	if (off == 0) {
		// laie's reg_clean_cache is 0
		// NO clean cache reg
		return 0;
	}

	doe_wr32(doe_priv, off, (tbl_id << 8) | 1);

	// 0 or -ETIMEDOUT
	ret = readl_poll_timeout_atomic(base + off, val, !(val & 0x1), 100, 100000);
	if (ret < 0) {
		int xie = doe_tbl_type_to_xie(tbl_type);

		return doe_error(-E_DOE_TIMEOUT,
				 "clean %s cache timeout @ tbl_id[%d] tbl_type[%d]",
				 doe_xie_name(xie), tbl_id, tbl_type);
	}

	return 0;
}

/*
 * Update spec table when creating user table
 *  @yk3_doe: doe device
 *  @spec_tbl_id: the special table id to be updated
 *  @parent: software cmd to creat user table
 */
static int
doe_submit_spec(struct yk3_doe_priv *doe_priv, u8 spec_tbl_id, struct yk3_doe_cmd *parent)
{
	struct yk3_doe_spec_table *spec = &doe_priv->spec[parent->tbl_id];
	struct yk3_doe_if *doe_if = doe_if_get_w(doe_priv);
	struct yk3_doe_cmd *cmd;
	void *data = doe_spec_table_data(spec, spec_tbl_id);
	u32 size = doe_spec_table_size(doe_priv, spec_tbl_id);

	cmd = doe_cmd_alloc(doe_if, YK3_DOE_SW_ARRAY_WRITE, spec_tbl_id, parent->tbl_id, 0);
	if (!cmd)
		return -E_DOE_NOMEM;

	memcpy(cmd->value, data, size);
	doe_cmd_submit(doe_if, cmd, parent);
	doe_debug_buffer(DOE_DEBUG_CMB, data, size,
			 "table[%d] spec[0x%x] %s",
			 parent->tbl_id, spec_tbl_id, doe_spec_table_name(spec_tbl_id));

	return 0;
}

//按hash表建表流程填充spec
static int
doe_create_hashtbl_spec(struct yk3_doe_priv *doe_priv, u8 tbl_id,
			struct yk3_doe_table_param *param)
{
	struct yk3_pdev_priv *pdev_priv = doe_priv->pdev_priv;
	struct yk3_doe_spec_table *spec = &doe_priv->spec[tbl_id];
	struct yk3_doe_xie_info *info = &doe_priv->xie_info[param->xie];
	struct yk3_doe_spec_table *view_spec = &doe_priv->spec[YK3_DOE_HASH_VIEW];
	struct yk3_doe_table_param *view_param = &doe_priv->param[YK3_DOE_HASH_VIEW];
	int i, ret = 0, ddr_channel = doe_ddr_channel(doe_priv);
	s64 ddr_addr = -1, ddr_len, depth_total;
	s64 index_ram_addr = -1, index_ram_len;
	s64 index_ddr_addr = -1, index_ddr_len;
	u32 item_len, item_order, index_mask, hash_seed, hash_table_count;
	u32 cache_entry_alloc = 0;

	hash_table_count = atomic_read(&doe_priv->hash_table_count);
	if (hash_table_count >= doe_priv->chip_spec->hash_table_limit) {
		// too more hash table
		return doe_error(-E_DOE_EXCEED,
				 "Hash table count[%u] exceeded the upper limit[%u]",
				 hash_table_count, doe_priv->chip_spec->hash_table_limit);
	}

	if (!doe_table_try_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_EXIST, "hash table %d exist", tbl_id);

	atomic_add(1, &doe_priv->hash_table_count);
	atomic_add(1, &doe_priv->active_table_count);

	item_len = param->key_len + param->dov_len + 32;
	item_order = doe_get_order(item_len);
	index_mask = roundup_pow_of_two(param->depth) - 1;
	depth_total = param->depth + param->sdepth;
	hash_seed = param->hash_seed ? : 0xffff;
	param->item_order = item_order;

	doe_debug(DOE_DEBUG_TBL,
		  "create table[%d] item_len[%u] depth-total[0x%llx] order[%u]",
		  tbl_id, item_len, depth_total, item_order);

	for (i = 0; i < YK3_DOE_XIE_END; i++) {
		// set all doe engine's hash seed
		if (yk3_doe_xie_specs[i].reg_hash_seed) {
			// skip laie(reg_hash_seed is 0)
			doe_wr32(doe_priv, yk3_doe_xie_specs[i].reg_hash_seed, hash_seed);
		}
	}

	// 1. miu param, just hmc
	if (doe_priv->hmc.enable) {
		ddr_len = depth_total * (1UL << item_order);
		ddr_addr = doe_mm_malloc(doe_priv->ddr, tbl_id, ddr_len);
		if (ddr_addr == -E_DOE_NOMEM) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] hmc alloc 0x%llx failed", tbl_id, ddr_len);
			goto error;
		}
		param->addr = ddr_addr;
		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] hmc alloc 0x%llx @ 0x%llx", tbl_id, ddr_len, ddr_addr);

		// non-ddr mode, NOT use miu
		spec->miu_param.ddr_channel	= ddr_channel;
		spec->miu_param.endian		= param->endian;
		spec->miu_param.ddr_mode	= YK3_DOE_DDR_MODE;
		spec->miu_param.item_size	= item_order; // u8
		spec->miu_param.item_len	= cpu_to_le16(item_len);
		spec->miu_param.ddr_base_low	= cpu_to_le32(ddr_addr);
		spec->miu_param.ddr_base_high	= (u8)(ddr_addr >> 32);
		spec->miu_param.ddr_base_low1	= 0;
		spec->miu_param.ddr_base_high1	= 0;
	} else {
		// non-ddr node
		u32 cache_entry_count = info->cache_entry_count;

		cache_entry_alloc = param->depth;
		if (cache_entry_alloc > cache_entry_count) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] cache alloc %u failed, left %u @ %s mode[%s]",
				  tbl_id, cache_entry_alloc, cache_entry_count,
				  doe_xie_name(param->xie), doe_cache_mode_name(info->is_short));
			goto error;
		}

		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] cache alloc 0x%x",
			  tbl_id, cache_entry_alloc);

		// TODO: atomic ?
		info->cache_entry_count -= cache_entry_alloc;
	}

	// hie param, both hmc/non-ddr
	spec->hie_param.mdepth		= cpu_to_le32(param->depth);
	spec->hie_param.sdepth		= cpu_to_le32(param->sdepth);
	spec->hie_param.index_mask	= cpu_to_le32(index_mask);
	spec->hie_param.key_len		= cpu_to_le16(param->key_len);
	spec->hie_param.value_len	= cpu_to_le16(param->dov_len);
	spec->hie_param.item_size	= item_order; // u8
	spec->hie_param.ddr_channel	= ddr_channel;
	spec->hie_param.endian		= param->endian;
	spec->hie_param.ddr_mode	= YK3_DOE_DDR_MODE;
	spec->hie_param.valid		= 1;
	spec->hie_param.tbl_type	= param->tbl_type;
	spec->hie_param.chain_limit	= cpu_to_le32(param->chain_limit);
	// k3max hash
	spec->hie_param.cache		= param->cache;
	spec->hie_param.cache.cache_base = (__force u16)cpu_to_le16(param->cache.cache_base);

	// cache param, both hmc/non-ddr
	spec->cache_param.valid		= 1;
	spec->cache_param.tbl_type	= param->tbl_type;
	spec->cache_param.ddr_channel	= ddr_channel;
	spec->cache_param.endian	= param->endian;
	spec->cache_param.ddr_mode	= YK3_DOE_DDR_MODE;
	spec->cache_param.value_len	= spec->hie_param.value_len;
	spec->cache_param.key_len	= spec->hie_param.key_len;
	spec->cache_param.depth		= 0;
	spec->cache_param.item_size	= item_order; // u8

	// index param, just hmc
	if (doe_priv->hmc.enable) {
		index_ram_len = (s64)param->index_cache;
		index_ram_addr = doe_mm_malloc(doe_priv->index_sram, tbl_id, index_ram_len);
		if (index_ram_addr == -E_DOE_NOMEM) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] index ram alloc 0x%llx failed",
				  tbl_id, index_ram_len);
			goto error;
		}
		spec->index_param.ram_base = cpu_to_le16(index_ram_addr);
		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] index ram alloc 0x%llx at 0x%llx",
			  tbl_id, index_ram_len, index_ram_addr);

		/* alloc index ddr resources in array_ddr1 */
		index_ddr_len = (s64)param->sdepth * sizeof(u32);
		index_ddr_addr = doe_mm_malloc(doe_priv->ddr, tbl_id, index_ddr_len);
		if (index_ddr_addr == -E_DOE_NOMEM) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] index alloc 0x%llx failed",
				  tbl_id, index_ddr_len);
			goto error;
		}
		// ddr_base表示哈希表index子表基址对应的索引块下标
		// 即：ddr_base = index子表基址 / 256
		spec->index_param.ddr_base = cpu_to_le32(index_ddr_addr >> 8);
		param->index_addr = index_ddr_addr;
		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] index alloc 0x%llx at 0x%llx",
			  tbl_id, index_ddr_len, index_ddr_addr);

		/* Init index_param special table */
		spec->index_param.ram_point = 0;
		spec->index_param.ddr_point = 0;
		spec->index_param.ddr_state = 0;
	}

	if (doe_priv->hmc.enable) {
		// non-ddr: hash NOT need zero clear
		//	hmc: hash zero clear directley
		doe_zero_clear_init(&spec->zero_clear, depth_total);
	}

	// hash view init, just for hmc
	if (doe_priv->hmc.enable) {
		//view_param->depth = (param->index_mask << 1) + 1;
		view_param->dov_len = item_len;

		/* Resetting table parameters for each initialization */
		/* TODO: add lock to protect flush table */
		view_spec->miu_param.ddr_channel = ddr_channel;
		view_spec->miu_param.endian = param->endian;
		view_spec->miu_param.ddr_mode = YK3_DOE_DDR_MODE;
		view_spec->miu_param.item_size = item_order;
		view_spec->miu_param.item_len = spec->miu_param.item_len;
		view_spec->miu_param.ddr_base_low = spec->miu_param.ddr_base_low;
		view_spec->miu_param.ddr_base_high = spec->miu_param.ddr_base_high;
		view_spec->miu_param.ddr_base_low1 = 0;
		view_spec->miu_param.ddr_base_high1 = 0;

		view_spec->aie_param.depth = cpu_to_le32(depth_total);
		view_spec->aie_param.item_size = item_order;
		view_spec->aie_param.item_len = spec->miu_param.item_len;
		view_spec->aie_param.ddr_channel = ddr_channel;
		view_spec->aie_param.endian = param->endian;
		view_spec->aie_param.ddr_mode = YK3_DOE_DDR_MODE;
		view_spec->aie_param.valid = 1;
		view_spec->aie_param.tbl_type = 0;

		view_spec->cache_param.valid = 1;
		view_spec->cache_param.tbl_type = 0;
		view_spec->cache_param.ddr_channel = ddr_channel;
		view_spec->cache_param.endian = param->endian;
		view_spec->cache_param.ddr_mode = YK3_DOE_DDR_MODE;
		view_spec->cache_param.value_len = spec->miu_param.item_len;
		view_spec->cache_param.key_len = 0;
		view_spec->cache_param.depth = cpu_to_le32(depth_total);
		view_spec->cache_param.item_size = item_order;
	}

	yk3_objcpy(&doe_priv->param[tbl_id], param);
	atomic_set(&spec->free_entrys, param->depth);
	yk3_dev_debug("Create hash table %d: %d+%d(2^%d) * 0x%x\n", tbl_id,
		      param->key_len, param->dov_len, spec->hie_param.item_size, param->depth);

	return 0;
error:
	if (ddr_addr >= 0)
		doe_mm_free(doe_priv->ddr, tbl_id, ddr_addr);
	if (index_ddr_addr >= 0)
		doe_mm_free(doe_priv->ddr, tbl_id, index_ddr_addr);
	if (index_ram_addr >= 0)
		doe_mm_free(doe_priv->index_sram, tbl_id, index_ram_addr);
	if (cache_entry_alloc)
		info->cache_entry_count += cache_entry_alloc;

	atomic_sub(1, &doe_priv->hash_table_count);
	atomic_sub(1, &doe_priv->active_table_count);

	doe_table_disable(doe_priv, tbl_id);
	return ret;
}

//按hash表删表流程填充spec
static int
doe_delete_hashtbl_spec(struct yk3_doe_priv *doe_priv, u8 tbl_id,
			struct yk3_doe_table_param *param)
{
	struct yk3_doe_spec_table *spec;
	s64 addr;

	spec = &doe_priv->spec[tbl_id];

	atomic_sub(1, &doe_priv->hash_table_count);
	atomic_sub(1, &doe_priv->active_table_count);

	spec->hie_param.valid = 0;
	spec->aie_param.valid = 0;
	spec->cache_param.valid = 0;

	if (doe_priv->hmc.enable) {
		/* free index ddr */
		addr = (s64)le32_to_cpu(spec->index_param.ddr_base) << 8;
		doe_mm_free(doe_priv->ddr, tbl_id, (u64)addr);

		/* free index sram */
		addr = le16_to_cpu(spec->index_param.ram_base);
		doe_mm_free(doe_priv->index_sram, tbl_id, (u64)addr);

		addr = (s64)spec->miu_param.ddr_base_high << 32;
		addr |= le32_to_cpu(spec->miu_param.ddr_base_low);
		doe_mm_free(doe_priv->ddr, tbl_id, (u64)addr);
	} else {
		struct yk3_doe_xie_info *info = &doe_priv->xie_info[param->xie];

		info->cache_entry_count += param->depth;
	}

	yk3_objzero(&spec->miu_param);
	yk3_objzero(&spec->hie_param);
	yk3_objzero(&spec->aie_param);
	yk3_objzero(&spec->cache_param);
	doe_table_disable(doe_priv, tbl_id);

	return 0;
}

//按数组表建表流程填充spec
static int
doe_create_arraytbl_spec(struct yk3_doe_priv *doe_priv, u8 tbl_id,
			 struct yk3_doe_table_param *param)
{
	int channel = param->ddr_channel;
	bool is_small_array = param->is_small_array;
	bool is_counter = (param->xie == YK3_DOE_CIE);
	struct yk3_doe_spec_table *spec = &doe_priv->spec[tbl_id];
	struct yk3_doe_xie_info *info = &doe_priv->xie_info[param->xie];
	struct yk3_pdev_priv *pdev_priv = doe_priv->pdev_priv;
	struct yk3_doe_mm *array_mm = doe_get_mm(doe_priv, channel);
	int ret = 0;
	s64 tbl_len, addr = -1;
	u32 item_len, item_space, item_order = 0;
	u32 cache_entry_alloc = 0;

	if (!doe_table_try_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_EXIST, "array table %d exist", tbl_id);

	atomic_add(1, &doe_priv->active_table_count);

	item_len = param->dov_len; // always raw length
	item_order = doe_get_order(item_len);
	if (is_counter) {
		if (is_doe_k3(doe_priv) || !doe_priv->is_cie_zip) {
			// k3 counter space is 64B
			// k3max counter space is 64B @ not zip mode
			item_order = doe_get_order(YK3_DOE_CIE_DSPACE);
		} else {
			// k3max counter space is 16B @ zip mode
			item_order = doe_get_order(YK3_DOE_CIE_DSPACE_ZIP);
		}
	}

	item_space = 1 << item_order;
	if (is_small_array) {
		// laie
		item_space = ALIGN(item_len, YK3_DOE_RAM_ALIGN_SIZE);

		// 片上数组表内存空间 = {[item_len] * 表深度}
		//	[]表示8字节对齐
		//	{}表示32字节对齐
		tbl_len = (s64)param->depth * item_space;
		tbl_len = ALIGN(tbl_len, YK3_DOE_RAM_BLOCK_SIZE);
	} else {
		// aie/cie/mie
		tbl_len = (s64)param->depth * item_space;
	}

	param->item_space = item_space;
	param->item_order = item_order;

	doe_debug(DOE_DEBUG_TBL,
		  "create array[%d] item_len[%u] depth[%u] order[%u]",
		  tbl_id, item_len, param->depth, item_order);

	// miu param
	// (1) for hmc
	// (2) for small-array @ non-ddr
	if (doe_priv->hmc.enable || is_small_array) {
		addr = doe_mm_malloc(array_mm, tbl_id, tbl_len);
		if (addr == -E_DOE_NOMEM) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] hmc alloc 0x%llx failed", tbl_id, tbl_len);
			goto error;
		}
		param->addr = addr;
		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] hmc alloc 0x%llx @ 0x%llx", tbl_id, tbl_len, addr);

		spec->miu_param.ddr_channel = channel;
		spec->miu_param.endian = param->endian;
		spec->miu_param.ddr_mode = YK3_DOE_DDR_MODE;
		spec->miu_param.item_size = item_order; // u8, small-array ignore it
		spec->miu_param.item_len = cpu_to_le16(item_len);
		spec->miu_param.ddr_base_low = cpu_to_le32(addr);
		spec->miu_param.ddr_base_high = (u8)(addr >> 32);
		spec->miu_param.ddr_base_low1 = 0;
		spec->miu_param.ddr_base_high1 = 0;
	} else {
		// non-ddr node
		u32 cache_entry_count = info->cache_entry_count;

		cache_entry_alloc = param->depth;
		if (cache_entry_alloc > cache_entry_count) {
			ret = -E_DOE_NOMEM;
			doe_error(ret, "table[%d] cache alloc %u failed, left %u @ %s mode[%s]",
				  tbl_id, cache_entry_alloc, cache_entry_count,
				  doe_xie_name(param->xie), doe_cache_mode_name(info->is_short));
			goto error;
		}

		doe_debug(DOE_DEBUG_TBL,
			  "table[%d] cache alloc 0x%x",
			  tbl_id, cache_entry_alloc);

		// TODO: atomic ?
		info->cache_entry_count -= cache_entry_alloc;
	}

	// aie param, both hmc/non-ddr
	spec->aie_param.item_size = item_order; // u8
	spec->aie_param.item_len = cpu_to_le16(item_len);
	spec->aie_param.ddr_channel = channel;
	spec->aie_param.valid = 1;
	spec->aie_param.depth = cpu_to_le32(param->depth);
	spec->aie_param.tbl_type = param->tbl_type;
	spec->aie_param.endian = param->endian;
	spec->aie_param.ddr_mode = YK3_DOE_DDR_MODE;

	// cache param, both hmc/non-ddr
	spec->cache_param.ddr_channel = channel;
	spec->cache_param.endian = param->endian;
	spec->cache_param.tbl_type = param->tbl_type;
	spec->cache_param.valid = 1;
	spec->cache_param.ddr_mode = YK3_DOE_DDR_MODE;
	spec->cache_param.value_len = cpu_to_le16(item_len);
	spec->cache_param.key_len = 0;
	spec->cache_param.depth = cpu_to_le32(param->depth);
	spec->cache_param.item_size = item_order; // u8

	// zero clear
	// (1) for hmc
	// (2) for small-array @ non-ddr
	if (is_small_array) {
		// non-ddr: only small-array need zero clear, others NOT need
		//     hmc: only small-array need zero clear, others zero clear directly
		doe_zero_clear_init(&spec->zero_clear, param->depth);
	}

	yk3_objcpy(&doe_priv->param[tbl_id], param);
	yk3_dev_debug("Create array table %d: %d(2^%d) * 0x%x\n", tbl_id,
		      param->dov_len, spec->aie_param.item_size,
		      param->depth + 1);

	return 0;
error:
	atomic_sub(1, &doe_priv->active_table_count);
	doe_table_disable(doe_priv, tbl_id);

	return ret;
}

//按数组表删表流程填充spec
static int
doe_delete_arraytbl_spec(struct yk3_doe_priv *doe_priv, u8 tbl_id,
			 struct yk3_doe_table_param *param)
{
	struct yk3_doe_spec_table *spec;
	s64 addr;
	struct yk3_doe_mm *array_mm;
	bool is_small_array = param->is_small_array;

	array_mm = doe_get_mm(doe_priv, param->ddr_channel);

	atomic_sub(1, &doe_priv->active_table_count);
	spec = &doe_priv->spec[tbl_id];
	spec->hie_param.valid = 0;
	spec->aie_param.valid = 0;
	spec->cache_param.valid = 0;

	if (doe_priv->hmc.enable || is_small_array) {
		addr = (s64)spec->miu_param.ddr_base_high << 32;
		addr |= le32_to_cpu(spec->miu_param.ddr_base_low);
		doe_mm_free(array_mm, tbl_id, (u64)addr);
	} else {
		struct yk3_doe_xie_info *info = &doe_priv->xie_info[param->xie];

		info->cache_entry_count += param->depth;
	}

	yk3_objzero(&spec->miu_param);
	yk3_objzero(&spec->hie_param);
	yk3_objzero(&spec->aie_param);
	yk3_objzero(&spec->cache_param);

	doe_table_disable(doe_priv, tbl_id);
	return 0;
}

static void
doe_hash_index_init_hmc(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_table_param *param = &cmd->tbl_param;
	union yk3_doe_index_block *block;
	u64 block_address;
	u32 block_count = (u64)param->sdepth * sizeof(u32) / sizeof(*block);
	u32 start = param->depth, index_count;
	u32 i, j, k = 0;

	for (i = 0; i < block_count; i++) {
		// recalc block address, maybe use new page
		block_address = doe_table_index_block_address(param, i);
		block = doe_hmc_object(doe_priv, block_address);

		index_count = ARRAY_SIZE(block->index);
		for (j = 0; j < index_count; j++, k++) {
			// {}
			block->index[j] = start + k;
		}
	}

	doe_debug(DOE_DEBUG_TBL,
		  "init hash[%d] index with block-count[0x%x]", cmd->tbl_id, block_count);
}

// 1. non-ddr:
//	small-array zero clear by doe command
//	others use cache, NOT need
// 2. hmc:
//	small-array zero clear by doe command
//	others use host ddr, zero clear by memzero
static int
doe_table_zero_clear_hw(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_spec_table *spec = &doe_priv->spec[cmd->tbl_id];
	struct yk3_doe_if *doe_if = doe_if_get_w(doe_priv);
	struct yk3_doe_cmd *sub;

	sub = doe_cmd_alloc(doe_if, YK3_DOE_SW_ARRAY_WRITE, YK3_DOE_ZERO_CLEAR_TABLE,
			    cmd->tbl_id, 0);
	if (!sub)
		return -E_DOE_NOMEM;

	yk3_objcpy(&sub->zero_clear, &spec->zero_clear);
	doe_cmd_submit(doe_if, sub, cmd);
	doe_debug_buffer(DOE_DEBUG_CMB, &spec->zero_clear, sizeof(spec->zero_clear),
			 "table[%d] spec[0x%x] %s",
			 cmd->tbl_id,
			 YK3_DOE_ZERO_CLEAR_TABLE,
			 doe_spec_table_name(YK3_DOE_ZERO_CLEAR_TABLE));

	return 0;
}

static int
doe_table_zero_clear_hmc(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_table_param *param = &cmd->tbl_param;
	struct yk3_doe_ddr_block *block;
	u64 block_address, total_space;
	u32 block_count, item_count, i;

	item_count = param->is_hash ? (param->depth + param->sdepth) : param->depth;
	total_space = item_count * (1UL << param->item_order);
	total_space = ALIGN(total_space, YK3_DOE_DDR_ALIGN);
	block_count = total_space / sizeof(*block);

	for (i = 0; i < block_count; i++) {
		block_address = doe_table_ddr_block_address(param, i);
		block = doe_hmc_object(doe_priv, block_address);

		yk3_objzero(block);
	}

	return 0;
}

//按hash表建表流程提交spec
static void
doe_submit_create_hashtbl(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	// 1. hmc memory init before hw cmd
	if (doe_priv->hmc.enable) {
		// 1.1 Main/Secondary table zero clear
		doe_table_zero_clear_hmc(doe_priv, cmd);
		// 1.2 Index table init
		doe_hash_index_init_hmc(doe_priv, cmd);
	}

	// 2. hash miu param
	if (doe_priv->hmc.enable) {
		// hmc mode    : need config miu param table
		// non-ddr mode: NOT  config miu param table
		doe_submit_spec(doe_priv, YK3_DOE_MIU_PARAM_TABLE, cmd);
	}

	// 3. hash hie param
	// non-ddr/hmc mode, both config hie param table
	doe_submit_spec(doe_priv, YK3_DOE_HIE_PARAM_TABLE, cmd);

	// 4. hash index param
	if (doe_priv->hmc.enable) {
		// create hash
		//
		// hmc mode    : need config index param table
		// non-ddr mode: NOT  config index param table
		doe_submit_spec(doe_priv, YK3_DOE_INDEX_PARAM_TABLE, cmd);
	}

	// 5. hash cache param
	doe_submit_spec(doe_priv, YK3_DOE_CACHE_PARAM_TABLE, cmd);
}

//按数组表建表流程提交spec
static void
doe_submit_create_arraytbl(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	bool is_small_array = cmd->tbl_param.is_small_array;
	int i, times = 1 + (is_small_array ? YK3_DOE_LAIE_MORE_TIMES : 0);

	// 1. hmc memory init before hw cmd
	if (doe_priv->hmc.enable && !is_small_array) {
		// hmc + NOT u200 + NOT small-array, zero clear directly
		doe_table_zero_clear_hmc(doe_priv, cmd);
	}

	// 2. array miu param
	if (doe_priv->hmc.enable || is_small_array) {
		// need config miu param table
		//
		// 1. all table @ hmc mode
		// 2. small array @ non-ddr mode
		doe_submit_spec(doe_priv, YK3_DOE_MIU_PARAM_TABLE, cmd);
	}

	// 3. array aie param
	for (i = 0; i < times; i++) {
		// HW-BUG: "zero clear" after "aie param", need some delay
		//
		// small  array table:
		// (1) need zero clear
		// (2) submit "aie param table" multi times, the MORE TIMES is the delay
		//
		// normal array table:
		// (1) NOT need zero clear
		// (2) submit "aie param table" 1 times
		doe_submit_spec(doe_priv, YK3_DOE_AIE_PARAM_TABLE, cmd);
	}

	// 4. zero clear
	// (1) small array need zero clear
	// (2) hmc + u200, need zero clear, not support
	if (is_small_array)
		doe_table_zero_clear_hw(doe_priv, cmd);

	// 5. array cache param
	doe_submit_spec(doe_priv, YK3_DOE_CACHE_PARAM_TABLE, cmd);
}

// hash/array is same
static void
doe_submit_delete_tbl(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	u32 old = doe_get_protect(doe_priv);

	// TODO: begin/end create/delete
	doe_set_protect(doe_priv, YK3_DOE_PROTECT_ALL);

	//执行删表命令时，同步清cache data
	doe_clean_cache(doe_priv, cmd->tbl_id);

	doe_submit_spec(doe_priv, YK3_DOE_CACHE_PARAM_TABLE, cmd);
	doe_submit_spec(doe_priv, YK3_DOE_AIE_PARAM_TABLE, cmd);
	doe_submit_spec(doe_priv, YK3_DOE_HIE_PARAM_TABLE, cmd);
	doe_submit_spec(doe_priv, YK3_DOE_MIU_PARAM_TABLE, cmd);

	doe_set_protect(doe_priv, old);
}

//YK3_DOE_SW_HW_INIT命令时，为DOE引擎建立一张索引资源表
static int
doe_init_index_view(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *parent)
{
	struct yk3_doe_cmd *cmd;
	struct yk3_doe_spec_table *spec = &doe_priv->spec[YK3_DOE_INDEX_VIEW];
	struct yk3_doe_if *doe_if = doe_if_get_w(doe_priv);

	// 1. index-view's miu-param-table
	cmd = doe_cmd_alloc(doe_if, YK3_DOE_SW_ARRAY_WRITE, YK3_DOE_MIU_PARAM_TABLE,
			    YK3_DOE_INDEX_VIEW, 0);
	if (!cmd)
		return -E_DOE_NOMEM;
	memcpy(cmd->value, &spec->miu_param, sizeof(spec->miu_param));
	doe_cmd_submit(doe_if, cmd, parent);
	doe_debug_buffer(DOE_DEBUG_CMB, &spec->miu_param, sizeof(spec->miu_param),
			 "table[%d] spec[0x%x] %s",
			 YK3_DOE_INDEX_VIEW,
			 YK3_DOE_MIU_PARAM_TABLE,
			 doe_spec_table_name(YK3_DOE_MIU_PARAM_TABLE));

	// 2. index-view's aie-param-table
	cmd = doe_cmd_alloc(doe_if, YK3_DOE_SW_ARRAY_WRITE, YK3_DOE_AIE_PARAM_TABLE,
			    YK3_DOE_INDEX_VIEW, 0);
	if (!cmd)
		return -E_DOE_NOMEM;
	memcpy(cmd->value, &spec->aie_param, sizeof(spec->aie_param));
	doe_cmd_submit(doe_if, cmd, parent);
	doe_debug_buffer(DOE_DEBUG_CMB, &spec->aie_param, sizeof(spec->aie_param),
			 "table[%d] spec[0x%x] %s",
			 YK3_DOE_INDEX_VIEW,
			 YK3_DOE_AIE_PARAM_TABLE,
			 doe_spec_table_name(YK3_DOE_AIE_PARAM_TABLE));

	// 3. index-view's cache-param-table
	cmd = doe_cmd_alloc(doe_if, YK3_DOE_SW_ARRAY_WRITE, YK3_DOE_CACHE_PARAM_TABLE,
			    YK3_DOE_INDEX_VIEW, 0);
	if (!cmd)
		return -E_DOE_NOMEM;
	memcpy(cmd->value, &spec->cache_param, sizeof(spec->cache_param));
	doe_cmd_submit(doe_if, cmd, parent);
	doe_debug_buffer(DOE_DEBUG_CMB, &spec->cache_param, sizeof(spec->cache_param),
			 "table[%d] spec[0x%x] %s",
			 YK3_DOE_INDEX_VIEW,
			 YK3_DOE_CACHE_PARAM_TABLE,
			 doe_spec_table_name(YK3_DOE_CACHE_PARAM_TABLE));

	return 0;
}

int doe_hmc_array_op(struct yk3_doe_priv *doe_priv,
		     int tbl_id, int index, void *buf, int len, enum yk3_doe_rw op)
{
	struct yk3_doe_table_param *param = &doe_priv->param[tbl_id];
	void *item;
	int item_len = param->dov_len;

	if (unlikely(len < item_len))
		return -E_DOE_INVALID;

	if (unlikely(index >= param->depth))
		return -E_DOE_INVALID;

	item = doe_hmc_array_obj(doe_priv, tbl_id, index);
	if (op == YK3_DOE_RD) {
		// load/read
		memcpy(buf, item, item_len);
	} else {
		// store/write
		memcpy(item, buf, item_len);
	}

	// TODO: big/little endian

	return 0;
}

int doe_hmc_counter_load(struct yk3_doe_priv *doe_priv,
			 int tbl_id, int index, struct yk3_doe_counter *counter)
{
	struct yk3_doe_table_param *param = &doe_priv->param[tbl_id];
	struct yk3_doe_hw_counter hw_counter;
	int ret;

	ret = doe_hmc_array_op(doe_priv, tbl_id, index,
			       &hw_counter, sizeof(hw_counter), YK3_DOE_RD);
	if (ret < 0)
		return ret;
	else if (!hw_counter.valid)
		return -E_DOE_NOTEXIST;

	counter->count = doe_hw_counter_count(&hw_counter);
	counter->bytes = doe_hw_counter_bytes(&hw_counter);

	// hw is little, table create is big, do swap
	if (param->endian == YK3_DOE_ENDIAN_BIG) {
		counter->count = swab64(counter->count);
		counter->bytes = swab64(counter->bytes);
	}

	return 0;
}

//表是否已存在(表是否已创建)
int doe_table_existed(struct yk3_doe_priv *doe_priv, u8 tbl_id)
{
	if (!is_good_doe_table_id(tbl_id)) {
		// {}
		return doe_error(-E_DOE_INVALID, "Error user tbl ID %d!", tbl_id);
	}

	return is_doe_table_enable(doe_priv, tbl_id);
}

int doe_set_protect(struct yk3_doe_priv *doe_priv, u32 protect)
{
	u32 ack, ack_mask = doe_priv->chip_spec->protect_ack;
	int ret = 0;

	doe_wr32(doe_priv, YK3_DOE_REG_PROTECT_CFG, protect);
	if (protect) {
		ret = readl_poll_timeout_atomic(doe_priv->bar_base + YK3_DOE_REG_PROTECT_ACK,
						protect, (protect & ack_mask) == ack_mask,
						YK3_DOE_PROTECT_ACK_DELAY,
						YK3_DOE_PROTECT_ACK_TIMEOUT);
		if (ret < 0) {
			ack = doe_rd32(doe_priv, YK3_DOE_REG_PROTECT_ACK);
			return doe_error(-E_DOE_TIMEOUT,
					 "set protect[0x%x] failed with ack[0x%x]", protect, ack);
		}
	}

	return 0;
}

int doe_set_np_protect(struct yk3_doe_priv *doe_priv, int enable)
{
	u32 new, old = doe_get_protect(doe_priv);

	if (enable) {
		// enable np protect
		new = old | YK3_DOE_PROTECT_NP;
	} else {
		// disable np protect
		new = old & (~YK3_DOE_PROTECT_NP);
	}

	return doe_set_protect(doe_priv, new);
}

int doe_get_np_protect(struct yk3_doe_priv *doe_priv)
{
	int ret;

	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	return doe_get_protect(doe_priv) & YK3_DOE_PROTECT_NP;
}

int doe_get_cache_info(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl_cache_info *cache)
{
	struct yk3_doe_xie_info *info;
	enum yk3_doe_xie xie;
	int ret;

	ret = doe_table_type_check(doe_priv, cache->tbl_type);
	if (ret < 0)
		return ret;

	xie = doe_tbl_type_to_xie(cache->tbl_type);
	info = &doe_priv->xie_info[xie];
	switch (xie) {
	case YK3_DOE_AIE:
	case YK3_DOE_HIE:
	case YK3_DOE_LHIE:
	case YK3_DOE_CIE:
	case YK3_DOE_MIE:
		cache->mode = info->cache_mode;
		cache->max_key_len = info->max_key_len;
		cache->max_value_len = info->max_value_len;
		cache->cache_entry_count = info->cache_entry_count;
		cache->cache_entry_limit = info->cache_entry_limit;
		break;
	default:
		yk3_do_nothing();
		break;
	}

	return 0;
}

int doe_set_cache_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl_cache_mode *cache)
{
	enum yk3_doe_xie xie = doe_tbl_type_to_xie(cache->tbl_type);
	int ret;

	ret = doe_table_type_check(doe_priv, cache->tbl_type);
	if (ret < 0)
		return ret;

	switch (xie) {
	case YK3_DOE_AIE:
	case YK3_DOE_HIE:
		return doe_set_xie_limit(doe_priv, xie, !!cache->mode, false);
	default:
		yk3_do_nothing();
		return 0;
	}
}

int doe_get_counter_zip(struct yk3_doe_priv *doe_priv)
{
	return is_doe_k3(doe_priv) ? (-E_DOE_NOTSUPP) : doe_priv->is_cie_zip;
}

int doe_set_counter_zip(struct yk3_doe_priv *doe_priv, int zip)
{
	if (is_doe_k3(doe_priv))
		return doe_error(-E_DOE_NOTSUPP, "k3 not support counter zip");

	doe_wr32(doe_priv, YK3_DOE_REG_COUNTER_ZIP, !!zip);

	return 0;
}

static u32
doe_hcode_mode_reg(int tbl_type)
{
	enum yk3_doe_xie xie = doe_tbl_type_to_xie(tbl_type);

	switch (xie) {
	case YK3_DOE_AIE:	return YK3_DOE_REG_HCODE_MODE_AIE;
	case YK3_DOE_HIE:	return YK3_DOE_REG_HCODE_MODE_HIE;
	case YK3_DOE_LHIE:	return YK3_DOE_REG_HCODE_MODE_LHIE;
	case YK3_DOE_CIE:	return YK3_DOE_REG_HCODE_MODE_CIE;
	case YK3_DOE_MIE:	return YK3_DOE_REG_HCODE_MODE_MIE;
	default:		return 0;
	}
}

int doe_get_hcode_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param)
{
	int tbl_type = param->hcode_mode.tbl_type;
	u32 reg = doe_hcode_mode_reg(tbl_type);
	u32 val;
	int ret;

	ret = doe_table_type_check(doe_priv, tbl_type);
	if (ret < 0)
		return ret;

	if (!reg)
		return -E_DOE_NOTSUPP;

	val = doe_rd32(doe_priv, reg);
	if (val & BIT(31))
		param->hcode_mode.mode = YK3_HCODE_KEY_16B;
	else
		param->hcode_mode.mode = YK3_HCODE_KEY_ALL;

	return 0;
}

int doe_set_hcode_mode(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param)
{
	int tbl_type = param->hcode_mode.tbl_type;
	u32 reg = doe_hcode_mode_reg(tbl_type);
	u32 val = (param->hcode_mode.mode == YK3_HCODE_KEY_16B) ? BIT(31) : 0;
	int ret;

	ret = doe_table_type_check(doe_priv, tbl_type);
	if (ret < 0)
		return ret;

	if (!reg)
		return -E_DOE_NOTSUPP;

	doe_wr32(doe_priv, reg, val);

	return 0;
}

int doe_get_cache_ioslation(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param)
{
	param->cache_isolation = doe_priv->is_cache_isolation;

	return 0;
}

int doe_set_cache_ioslation(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param)
{
	if (atomic_read(&doe_priv->hash_table_count))
		return doe_error(-E_DOE_NOTSUPP,
				 "can not change cache ioslation @ hash table using");

	doe_priv->is_cache_isolation = param->cache_isolation;
	doe_wr32(doe_priv, YK3_DOE_REG_CACHE_ISO_HIE, doe_priv->is_cache_isolation);

	return 0;
}

int doe_get_hash_entry_count(struct yk3_doe_priv *doe_priv, struct yk3_tbl_ctl *param)
{
	u32 i, reg;

	if (!is_doe_k3max(doe_priv))
		return -E_DOE_NOTSUPP;

	reg = param->hash_entry_count.is_small
		? YK3_DOE_REG_LHIE_ENTRY_COUNT
		: YK3_DOE_REG_HIE_ENTRY_COUNT;

	for (i = 0; i < K3MAX_HASH_TABLE_LIMIT; i++) {
		// {}
		param->hash_entry_count.stat[i].raw = doe_rd32(doe_priv, reg + i * sizeof(u32));
	}

	return 0;
}

int doe_table_type_check(const struct yk3_doe_priv *doe_priv, enum yk3_doe_tbl tbl_type)
{
	enum yk3_doe_xie xie;

	if (unlikely(!is_good_doe_table_type(tbl_type)))
		return doe_error(-E_DOE_INVALID, "invalid table type: %d", tbl_type);

	xie = doe_tbl_type_to_xie(tbl_type);

	if (unlikely(!is_doe_xie_enable(doe_priv, xie))) {
		doe_err("%s is closed, can't operate %s table. clk_gate = 0x%x/%d",
			doe_xie_name(xie),
			doe_tbl_name(tbl_type),
			doe_priv->clk_gate_en,
			doe_xie_gate_bit(doe_priv, xie));
		return -E_DOE_NOTSUPP;
	}

	return 0;
}

//进行DOE表复位（所有表初始化）
static int
doe_reset(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	int i;

	atomic_set(&doe_priv->hash_table_count, 0);
	atomic_set(&doe_priv->active_table_count, 0);

	for (i = 0; i < YK3_DOE_USER_TABLE_LIMIT; ++i) {
		cmd->tbl_id = i;

		doe_submit_spec(doe_priv, YK3_DOE_CACHE_PARAM_TABLE, cmd);
		doe_submit_spec(doe_priv, YK3_DOE_AIE_PARAM_TABLE, cmd);
		doe_submit_spec(doe_priv, YK3_DOE_HIE_PARAM_TABLE, cmd);
		doe_submit_spec(doe_priv, YK3_DOE_MIU_PARAM_TABLE, cmd);
	}

	return 0;
}

//进行DOE表复位（表引擎cache初始化）
static int
doe_cache_reset(struct yk3_doe_priv *doe_priv)
{
	void __iomem *base = doe_priv->bar_base;
	u32 val, off;
	int i, ret;

	for (i = 0; i < YK3_DOE_XIE_END; ++i) {
		off = yk3_doe_xie_specs[i].reg_init_cache;
		if (off == 0 || !is_doe_xie_enable(doe_priv, i)) {
			// 1. off is 0, needn't reset
			// 2. xie is closed, needn't reset
			continue;
		}

		doe_wr32(doe_priv, off, 1);
		doe_debug(DOE_DEBUG_CRIT,
			  "reset %s cache reg[0x%x] = %u", doe_xie_name(i), off, 1);

		ret = readl_poll_timeout_atomic(base + off, val, !(val & 0x1), 100, 500000);
		if (ret < 0)
			return doe_error(-E_DOE_TIMEOUT, "reset %s cache timeout", doe_xie_name(i));
	}

	return 0;
}

//命令分析和处理，将命令提交到list
static int
doe_process_cmd(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_if *doe_if;
	bool is_read = is_doe_cmd_read(cmd);
	int ret;

	if (is_read)
		doe_debug(DOE_DEBUG_CTR, "Process %s cmd " DOE_CMD_FMT,
			  doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);
	else
		doe_debug(DOE_DEBUG_CTW, "Process %s cmd " DOE_CMD_FMT,
			  doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id);

	switch (cmd->opcode) {
	case YK3_DOE_SW_CREATE_ARRAY:
		//按数组表建表流程填充spec
		ret = doe_create_arraytbl_spec(doe_priv, cmd->tbl_id, &cmd->tbl_param);
		if (ret < 0)
			return ret;
		//按数组表建表流程提交spec
		doe_submit_create_arraytbl(doe_priv, cmd);
		break;
	case YK3_DOE_SW_DELETE_ARRAY:
		//按数组表删表流程填充spec
		doe_delete_arraytbl_spec(doe_priv, cmd->tbl_id, &doe_priv->param[cmd->tbl_id]);
		//按数组表删表流程提交spec
		doe_submit_delete_tbl(doe_priv, cmd);
		break;
	case YK3_DOE_SW_CREATE_HASH:
		//按hash表建表流程填充spec
		ret = doe_create_hashtbl_spec(doe_priv, cmd->tbl_id, &cmd->tbl_param);
		if (ret < 0)
			return ret;
		//按hash表建表流程提交spec
		doe_submit_create_hashtbl(doe_priv, cmd);
		break;
	case YK3_DOE_SW_DELETE_HASH:
		//按hash表删表流程填充spec
		doe_delete_hashtbl_spec(doe_priv, cmd->tbl_id, &doe_priv->param[cmd->tbl_id]);
		//按hash表删表流程提交spec
		doe_submit_delete_tbl(doe_priv, cmd);
		break;
	/* alloc the desc for single cmd */
	case YK3_DOE_SW_ARRAY_LOAD:
	case YK3_DOE_SW_ARRAY_STORE:
	case YK3_DOE_SW_HASH_INSERT:
	case YK3_DOE_SW_HASH_DELETE:
	case YK3_DOE_SW_HASH_QUERY:
	case YK3_DOE_SW_HASH_UPDATE:
	case YK3_DOE_SW_HASH_SAVE:
	case YK3_DOE_SW_RAW_CMD:
	case YK3_DOE_SW_COUNTER_ENABLE:
		//普通的访表指令，构建cmd并提交
		doe_if = doe_if_get(doe_priv, is_read);
		doe_cmd_submit(doe_if, cmd, NULL);
		break;
	case YK3_DOE_SW_INIT_INDEX_VIEW:
		if (doe_priv->hmc.enable) {
			ret = doe_init_index_view(doe_priv, cmd);
			if (ret < 0)
				return ret;
		}
		break;
	case YK3_DOE_SW_RESET_ALL_TABLE:
		//进行DOE表复位（所有表初始化）
		doe_reset(doe_priv, cmd);
		break;
	default:
		return doe_error(-E_DOE_INVALID, "invalid process opcode: %d", cmd->opcode);
	}

	return 0;
}

static int
doe_fast_process(struct yk3_doe_priv *doe_priv, struct list_head *list, int n_cmd)
{
	struct yk3_doe_if *doe_if = doe_if_get_w(doe_priv);
	struct yk3_doe_cmd_buffer *cb = NULL;
	struct yk3_doe_cmd *cmd;
	int ret;

	// 1. get cb
	ret = doe_cb_pool_get(doe_if, &cb);
	if (ret < 0)
		goto release;

	// 2. cmd process and push cmd to cb
	list_for_each_entry(cmd, list, node) {
		ret = doe_process_cmd(doe_priv, cmd);
		if (ret < 0)
			goto error;

		ret = doe_enqueue_cmdbuffer(doe_if, cmd, cb);
		if (ret < 0)
			goto error;
	}

	// 3. send cb
	ret = doe_send_cmdbuffer(doe_if, cb);
	if (ret < 0)
		goto error;

	return 0;
error:
	// NOT clean cb here, do it @ get
	doe_cb_pool_put(doe_if, cb);
release:
	list_for_each_entry(cmd, list, node) {
		// {}
		doe_cmd_put(doe_if, cmd, false);
	}

	return ret;
}

// >0: push success and pop all, return n_pop
// =0: push success
static int
doe_fast_push(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, struct list_head *empty_list)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;
	int n_pop, count;

	spin_lock(&fast->lock);
	// 1. push cmd
	count = 1 + READ_ONCE(fast->count);
	list_add_tail(&cmd->node, &fast->list);
	fast->n_user_push++;

	if (count < fast->threshold) {
		// 2.1 NOT need pop
		n_pop = 0;
		WRITE_ONCE(fast->count, count);
	} else {
		// 2.2 pop
		n_pop = count;
		list_splice_tail_init(&fast->list, empty_list);
		fast->n_user_pop += n_pop;
		WRITE_ONCE(fast->count, 0);
	}
	spin_unlock(&fast->lock);

	return n_pop;
}

// >0: pop success, return n_pop
// =0: pop nothing
static int
doe_fast_pop(struct yk3_doe_priv *doe_priv, struct list_head *empty_list)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;
	int n_pop;

	spin_lock(&fast->lock);
	n_pop = READ_ONCE(fast->count);
	if (n_pop) {
		list_splice_tail_init(&fast->list, empty_list);
		fast->n_work_pop += n_pop;
		WRITE_ONCE(fast->count, 0);
	}
	spin_unlock(&fast->lock);

	return n_pop;
}

void doe_fast_worker(struct work_struct *work)
{
	struct yk3_doe_fast *fast = container_of(work, struct yk3_doe_fast, work.work);
	struct yk3_doe_priv *doe_priv = container_of(fast, struct yk3_doe_priv, fast);
	struct list_head list = LIST_HEAD_INIT(list);
	int ret, n_cmd;

	n_cmd = doe_fast_pop(doe_priv, &list);
	if (!n_cmd) {
		fast->n_work_empty++;
		return;
	}

	// pop
	ret = doe_fast_process(doe_priv, &list, n_cmd);
	if (ret) {
		// {}
		atomic64_add(n_cmd, &fast->n_work_cmd_error);
	} else {
		// {}
		fast->n_work_batch++;
	}
}

static int
doe_fast_call(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_fast *fast = &doe_priv->fast;
	struct list_head list = LIST_HEAD_INIT(list);
	int ret, n_cmd;

	n_cmd = doe_fast_push(doe_priv, cmd, &list);
	if (!n_cmd) {
		// push ok
		return 0;
	}

	ret = doe_fast_process(doe_priv, &list, n_cmd);
	if (ret) {
		// {}
		atomic64_add(n_cmd, &fast->n_user_cmd_error);
	} else {
		// {}
		atomic64_add(n_cmd, &fast->n_user_batch);
	}

	return ret;
}

static int
doe_sync_call(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd)
{
	struct yk3_doe_if *doe_if = doe_if_get(doe_priv, is_doe_cmd_read(cmd));
	bool is_timeout = false;
	bool is_sent = false;
	int ret;

	if (is_doe_cmd_container(cmd)) {
		// constainer
		cmd->doe_if = doe_if;
	} else if (is_doe_cmd_stack(cmd)) {
		// sub/single cmd
		//
		// register stack cmd
		ret = doe_cmd_register(doe_if, cmd);
		if (ret < 0)
			return ret;
	}

	// 命令分析和处理，将命令提交到list
	ret = doe_process_cmd(doe_priv, cmd);
	if (ret < 0)
		goto clean;

	// 构建成cmd buffer下发给DOE引擎
	ret = doe_process_cmdbuffer(doe_if, cmd);
	if (ret < 0)
		goto clean;

	is_sent = true;
	is_timeout = doe_cmd_wait_finish(doe_priv, cmd);
	if (is_timeout) {
		ret = -E_DOE_TIMEOUT;
		goto clean;
	}

clean:
	doe_cmd_wait_clean(doe_if, cmd, is_sent, is_timeout);

	if (unlikely(!ret && cmd->err)) {
		// cmd->err is hardware error(unsigned)
		ret = doe_hw_err(cmd->err);
	}

	return ret;
}

int doe_kernel_call(struct yk3_doe_priv *doe_priv, struct yk3_doe_cmd *cmd, u8 call_mode)
{
	bool is_read = is_doe_cmd_read(cmd);
	bool is_fast_call = IS_YK3_DOE_FAST_CALL(call_mode);
	int ret = 0;

	if (unlikely(doe_priv->init && is_doe_init_opcode(cmd->opcode))) {
		//  {}
		return 0;
	}

	if (is_read)
		doe_debug(DOE_DEBUG_CTR, "Recive %s cmd: " DOE_CMD_FMT " call-mode: 0x%x",
			  doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id, call_mode);
	else
		doe_debug(DOE_DEBUG_CTW, "Recive %s cmd: " DOE_CMD_FMT " call-mode: 0x%x",
			  doe_cmd_name(cmd),
			  cmd->seq, cmd->tag, cmd->flags, cmd->opcode, cmd->tbl_id, call_mode);

	ret = doe_sw_cmd_valid(doe_priv, cmd);
	if (unlikely(ret < 0)) {
		if (is_fast_call) {
			doe_cmd_put(doe_if_get_w(doe_priv), cmd, false);
			atomic64_inc(&doe_priv->fast.n_user_cmd_error);
		}

		return ret;
	}

	if (is_fast_call)
		ret = doe_fast_call(doe_priv, cmd);
	else
		ret = doe_sync_call(doe_priv, cmd);

	return ret;
}

#ifndef DOE_HW_CHECK_JIFFIES
#define DOE_HW_CHECK_JIFFIES	(60 * HZ)
#endif

static int
doe_iiu_alarm_check(struct yk3_doe_priv *doe_priv)
{
	u32 alarm;
	int i, ret = 0;

	alarm = doe_rd32(doe_priv, YK3_DOE_REG_IIU_ALARM);
	alarm &= YK3_DOE_REG_IIU_ALARM_MASK;

	if (alarm) {
		ret = -ENODEV;
		doe_error(ret, "hardware error(iiu alarm: 0x%x).", alarm);

		for (i = 0; i < 32; i++) {
			if (alarm & BIT(i)) {
				// {}
				doe_error(ret, "hardware error(iiu alarm bit %d).", i);
			}
		}
	}

	return ret;
}

int doe_hw_check(struct yk3_doe_priv *doe_priv)
{
	int ret;

	if (unlikely(is_doe_hw_failure(doe_priv)))
		return doe_error(-E_DOE_HWCLOSED, "hardware closed");

	ret = doe_iiu_alarm_check(doe_priv);
	if (ret < 0)
		return ret;

	return 0;
}

//触发YK3_DOE_SW_HW_INIT命令，对DOE初始化
int doe_hw_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	int ret;

	// 1. protect np
	// default(without yk3 driver), np & host protect enable
	// init(with yk3 driver), np protect enable, remove host protect
	doe_set_protect(doe_priv, YK3_DOE_PROTECT_NP);

	// 2. reset all user table
	cmd.opcode = YK3_DOE_SW_RESET_ALL_TABLE;
	ret = doe_sync_call(doe_priv, &cmd);
	if (ret < 0)
		return ret;

	doe_priv->init = true;

	// 3. 进行DOE表复位(表引擎cache初始化)
	doe_cache_reset(doe_priv);

	if (doe_priv->hmc.enable) {
		struct yk3_doe_cmd cmd_init = DOE_CMD_CONTAINER(cmd_init);

		// 4. init index view
		cmd_init.opcode = YK3_DOE_SW_INIT_INDEX_VIEW;
		ret = doe_sync_call(doe_priv, &cmd_init);
		if (ret < 0)
			return ret;
	}

	doe_priv->init = true;

	return 0;
}
