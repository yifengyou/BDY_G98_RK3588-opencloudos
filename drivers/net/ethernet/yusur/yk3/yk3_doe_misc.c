// SPDX-License-Identifier: GPL-2.0
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/log2.h>
#include <linux/module.h>

#include "yk3_doe_process.h"
/**************************************************************************************************/
#if YK3_DEBUG
unsigned long doe_debug_level;
module_param_named(debug_level, doe_debug_level, ulong, 0644);
MODULE_PARM_DESC(doe_debug_level, "debug level mask");

unsigned long doe_error_inject;
module_param_named(error_inject, doe_error_inject, ulong, 0644);
MODULE_PARM_DESC(doe_error_inject, "error inject mask");
#endif

#if YK3_DOE_TEST
struct doe_test_param {
	char name[32];
	struct task_struct *task;
	struct yk3_doe_priv *doe_priv;
	struct yk3_doe_priv_test_perf param;
	int idx;
	u64 pps;
};

static atomic_t doe_test_ref;
static int doe_test_start;
static struct doe_test_param doe_test_param[32];

#define DOE_TEST_TIMEOUT	(10UL * 1000 * 1000 * 1000)

static int
doe_ioctl_test(struct yk3_pdev_priv *pdev_priv, struct yk3_doe_priv_test_perf *param);
#endif

#if YK3_DOE_MEASURE
static int
doe_ioctl_measure(struct yk3_pdev_priv *pdev_priv, struct yk3_doe_priv_measure *param);
#endif
/**************************************************************************************************/
void yk3_dump_line_helper(int line, const void *raw, int len, yk3_dump_line_f *dump_line, void *ctx)
{
	char buf[YK3_DUMP_LINE_LIMIT];
	int  i, offset = 0;
	const u8 *bin = raw;

	// line as: "xxxxH :"
	offset += yk3_soprintf(buf, offset, "%.4XH :", YK3_DUMP_LINE_BYTES * line);

	/* Hexadecimal Content as: " 00112233 44556677 8899aabb ccddeeff ; " */
	for (i = 0; i < YK3_DUMP_LINE_BYTES; i++) {
		if (0 == (i % YK3_DUMP_LINE_BLOCK_BYTES))
			offset += yk3_soprintf(buf, offset, " ");

		if (i < len)
			offset += yk3_soprintf(buf, offset, "%.2X", bin[i]);
		else
			offset += yk3_soprintf(buf, offset, "  ");
	}
	offset += yk3_soprintf(buf, offset, " ; ");

	// Raw Content as: "cccccccccccccccc"
	for (i = 0; i < YK3_DUMP_LINE_BYTES; i++) {
		int c = (int)bin[i];

		offset += yk3_soprintf(buf, offset, "%c", (c >= 0x20 && c <= 0x7e) ? c : '.');
	}
	offset += yk3_soprintf(buf, offset, YK3_CRLF);
	buf[offset] = 0;

	if (dump_line)
		(*dump_line)(ctx, buf);
	else
		yk3_info("%s", buf);
}

void yk3_dump_buffer_helper(const void *buffer, int len, yk3_dump_line_f *dump_line, void *ctx)
{
	int i, line, tail;
	const u8 *raw = buffer;

	if (len < 0)
		return;

	line = ALIGN(len, YK3_DUMP_LINE_BYTES) / YK3_DUMP_LINE_BYTES;
	tail = len % YK3_DUMP_LINE_BYTES;
	tail = tail ? tail : YK3_DUMP_LINE_BYTES;

	// header
	if (dump_line) {
		(*dump_line)(ctx, YK3_DUMP_LINE_HEADER0);
		(*dump_line)(ctx, YK3_DUMP_LINE_HEADER1);
		(*dump_line)(ctx, YK3_DUMP_LINE_HEADER2);
		(*dump_line)(ctx, YK3_DUMP_LINE_HEADER3);
	} else {
		yk3_info(YK3_DUMP_LINE_HEADER0);
		yk3_info(YK3_DUMP_LINE_HEADER1);
		yk3_info(YK3_DUMP_LINE_HEADER2);
		yk3_info(YK3_DUMP_LINE_HEADER3);
	}

	// body
	for (i = 0; i < (line - 1); i++)
		yk3_dump_line_helper(i, raw + i * YK3_DUMP_LINE_BYTES, YK3_DUMP_LINE_BYTES,
				     dump_line, ctx);

	yk3_dump_line_helper(line - 1, raw + i * YK3_DUMP_LINE_BYTES, tail, dump_line, ctx);
}

void yk3_seq_dump_line(struct seq_file *seq, const char *line)
{
	seq_printf(seq, "%s", line);
}

/**************************************************************************************************/
#define doe_seq_echo(fmt, args...) seq_printf(seq, fmt YK3_CRLF, ##args)
#define doe_reg_echo(name, reg) ({ \
	u32 m_reg = (reg); \
	doe_seq_echo("[0x%04x] %-22s : 0x%08x", m_reg, name, doe_rd32(doe_priv, m_reg)); \
})
#define doe_seq_split()	doe_seq_echo("#--------------")
#define doe_seq_blank()	doe_seq_echo("")

#define doe_seq_dump_buffer(buf, len) \
	yk3_dump_buffer_helper(buf, len, (yk3_dump_line_f *)yk3_seq_dump_line, seq)
/**************************************************************************************************/
static int
yk3_doe_hst_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_doe_priv *doe_priv = pdev_priv->doe_priv;

	if (!is_doe_pdev_mgr(pdev_priv))
		return 0;

	doe_seq_echo("# ------ IIU ------");

	doe_reg_echo("doe_version",		0x0000);
	doe_reg_echo("pf_num",			0x0008);
	doe_reg_echo("doe_work_mode",		0x000c);
	doe_reg_echo("rd_int_vector_reg",	0x0088);
	doe_reg_echo("wr_int_vector_reg",	0x008c);
	doe_reg_echo("doe_protect_cfg",		0x00b0);
	doe_reg_echo("doe_iiu_alarm_reg",	0x00c0);
	doe_reg_echo("axi_ar_cnt",		0x0100);
	doe_reg_echo("axi_rlast_cnt",		0x0104);
	doe_reg_echo("axi_rdata_cnt",		0x0108);
	doe_reg_echo("axi_rrsp_err_cnt",	0x010c);
	doe_reg_echo("axi_aw_cnt",		0x0110);
	doe_reg_echo("axi_wdata_last_cnt",	0x0114);
	doe_reg_echo("axi_bid_cnt",		0x0118);
	doe_reg_echo("axi_brsp_err_cnt",	0x011c);
	doe_reg_echo("int_rd_all_cnt",		0x0120);
	doe_reg_echo("int_rd_out_cnt",		0x0124);
	doe_reg_echo("int_rd_lost_cnt",		0x0128);
	doe_reg_echo("int_wr_all_cnt",		0x0130);
	doe_reg_echo("int_wr_out_cnt",		0x0134);
	doe_reg_echo("int_wr_lost_cnt",		0x0138);
	doe_reg_echo("laie_inst_sop_cnt",	0x0140);
	doe_reg_echo("laie_inst_eop_cnt",	0x0144);
	doe_reg_echo("laie_res_sop_cnt",	0x0148);
	doe_reg_echo("laie_res_eop_cnt",	0x014c);
	doe_reg_echo("lock_aie_inst_sop_cnt",	0x0150);
	doe_reg_echo("lock_aie_inst_eop_cnt",	0x0154);
	doe_reg_echo("lock_aie_res_sop_cnt",	0x0158);
	doe_reg_echo("lock_aie_res_eop_cnt",	0x015c);
	doe_reg_echo("aie_inst_sop_cnt",	0x0160);
	doe_reg_echo("aie_inst_eop_cnt",	0x0164);
	doe_reg_echo("aie_res_sop_cnt",		0x0168);
	doe_reg_echo("aie_res_eop_cnt",		0x016c);
	doe_reg_echo("mie_inst_sop_cnt",	0x0170);
	doe_reg_echo("mie_inst_eop_cnt",	0x0174);
	doe_reg_echo("mie_res_sop_cnt",		0x0178);
	doe_reg_echo("mie_res_eop_cnt",		0x017c);
	doe_reg_echo("cie_inst_sop_cnt",	0x0180);
	doe_reg_echo("cie_inst_eop_cnt",	0x0184);
	doe_reg_echo("cie_res_sop_cnt",		0x0188);
	doe_reg_echo("cie_res_eop_cnt",		0x018c);
	doe_reg_echo("hie_inst_sop_cnt",	0x0190);
	doe_reg_echo("hie_inst_eop_cnt",	0x0194);
	doe_reg_echo("hie_res_sop_cnt",		0x0198);
	doe_reg_echo("hie_res_eop_cnt",		0x019c);
	doe_reg_echo("lhie_inst_sop_cnt",	0x01a0);
	doe_reg_echo("lhie_inst_eop_cnt",	0x01a4);
	doe_reg_echo("lhie_res_sop_cnt",	0x01a8);
	doe_reg_echo("lhie_res_eop_cnt",	0x01ac);
	doe_reg_echo("spe_miu_aw_cnt",		0x01b0);
	doe_reg_echo("spe_miu_wsop_cnt",	0x01b4);
	doe_reg_echo("spe_miu_weop_cnt",	0x01b8);
	doe_reg_echo("spe_miu_b_cnt",		0x01bc);
	doe_reg_echo("spe_miu_ar_cnt",		0x01c0);
	doe_reg_echo("spe_miu_rsop_cnt",	0x01c4);
	doe_reg_echo("spe_miu_reop_cnt",	0x01c8);
	doe_reg_echo("rd_res_sop_cnt",		0x0200);
	doe_reg_echo("rd_res_eop_cnt",		0x0204);
	doe_reg_echo("wr_res_sop_cnt",		0x0208);
	doe_reg_echo("wr_res_eop_cnt",		0x020c);
	doe_reg_echo("host_res_sop_cnt",	0x0210);
	doe_reg_echo("host_res_eop_cnt",	0x0214);
	doe_reg_echo("spe_res_sop_cnt",		0x0220);
	doe_reg_echo("spe_res_eop_cnt",		0x0224);
	doe_reg_echo("iiu_err_res_cnt",		0x0228);
	doe_reg_echo("iiu_err_opcode_cnt",	0x022c);
	doe_reg_echo("axi_in_cmd_sop_cnt",	0x0230);
	doe_reg_echo("axi_in_cmd_eop_cnt",	0x0234);
	doe_reg_echo("iiu_axi_rd_status",	0x0250);
	doe_reg_echo("iiu_axi_wr_status",	0x0254);
	doe_reg_echo("iiu_int_status",		0x0258);
	doe_reg_echo("iiu_array_inst_status",	0x025c);
	doe_reg_echo("iiu_ainst_res_status",	0x0260);
	doe_reg_echo("iiu_hash_inst_status",	0x0264);
	doe_reg_echo("iiu_hinst_res_status",	0x0268);
	doe_reg_echo("res_status_reg",		0x026c);
	doe_reg_echo("dma_rd_cnt_in",		0x0270);
	doe_reg_echo("dma_rd_cnt_out",		0x0274);
	doe_reg_echo("dma_rd_cnt_all",		0x0278);
	doe_reg_echo("dma_wr_cnt_in",		0x0280);
	doe_reg_echo("dma_wr_cnt_out",		0x0284);
	doe_reg_echo("dma_wr_cnt_all",		0x0288);
	doe_reg_echo("dma_mode",		0x0300);
	doe_reg_echo("dma_rd_buf_space",	0x0304);
	doe_reg_echo("dma_wr_buf_space",	0x0308);
	doe_reg_echo("clk_gate_en",		0x0330);

	// doe_seq_blank();
	// doe_seq_echo("# ------ ABNOR ------");
	// doe_seq_blank();
	// doe_seq_echo("# ------ ABNOR ------");

	return 0;
}

static int
yk3_doe_sst_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_doe_priv *doe_priv = pdev_priv->doe_priv;
	struct yk3_doe_fast *fast = &doe_priv->fast;
	struct yk3_doe_if *doe_if;
	struct yk3_doe_cmd_manager *manager;
	struct yk3_doe_if_cfg *cfg;
	struct yk3_doe_vfc *vfc;
	struct yk3_doe_evq *evq;
	struct yk3_doe_fifo *fifo;
	struct doe_tag_pool *tag_pool;
	struct yk3_doe_cb_pool *cb_pool;
	struct yk3_doe_tag_slot *slot;
	int i, tag;

	if (!is_doe_pdev_mgr(pdev_priv))
		return 0;

	doe_seq_echo("fast-call stat:");
	doe_seq_echo(YK3_TAB "stop           : %d", fast->stop);
	doe_seq_echo(YK3_TAB "list count     : %u", fast->count);
	doe_seq_echo(YK3_TAB "list threshold : %u", fast->threshold);
	doe_seq_echo(YK3_TAB "cmd user nil   : %llu", (u64)atomic64_read(&fast->n_user_nil));
	doe_seq_echo(YK3_TAB "cmd user push  : %llu", fast->n_user_push);
	doe_seq_echo(YK3_TAB "cmd user pop   : %llu", fast->n_user_pop);
	doe_seq_echo(YK3_TAB "cmd work pop   : %llu", fast->n_work_pop);
	doe_seq_echo(YK3_TAB "timer busy     : %llu", fast->n_timer_busy);
	doe_seq_echo(YK3_TAB "timer slow     : %llu", fast->n_timer_slow);
	doe_seq_echo(YK3_TAB "user batch     : %llu", (u64)atomic64_read(&fast->n_user_batch));
	doe_seq_echo(YK3_TAB "user cmd error : %llu", (u64)atomic64_read(&fast->n_user_cmd_error));
	doe_seq_echo(YK3_TAB "work batch     : %llu", fast->n_work_batch);
	doe_seq_echo(YK3_TAB "work cmd error : %llu", (u64)atomic64_read(&fast->n_work_cmd_error));
	doe_seq_echo(YK3_TAB "work empty     : %llu", fast->n_work_empty);
	doe_seq_echo(YK3_TAB "irq  ack error : %llu", fast->n_ack_error);

	for (i = 0; i < 2; i++) {
		doe_if = &doe_priv->doe_if[i];
		cfg = doe_if->cfg;
		vfc = &doe_if->vfc;
		evq = &doe_if->evq;
		fifo = &doe_if->fifo;
		cb_pool = &doe_if->cb_pool;
		manager = &doe_if->cmd_manager;
		tag_pool = &manager->tag_pool;

		doe_seq_echo("%s-interface stat:", (i == YK3_DOE_RD) ? "read" : "write");
		doe_seq_echo(YK3_TAB "irq trigger   : 0x%llx",
			     (u64)atomic64_read(&doe_if->st.irq_trigger));
		doe_seq_echo(YK3_TAB "event unknown : %llu", doe_if->st.event_unknown);
		doe_seq_echo(YK3_TAB "event again   : %llu", doe_if->st.event_again);
		doe_seq_echo(YK3_TAB "event left    : %llu", doe_if->st.event_left);

		doe_seq_echo(YK3_TAB "cfg cmd buffer size     : %u", cfg->cmd_buffer_size);
		doe_seq_echo(YK3_TAB "cfg evq depth           : %u", cfg->evq_depth);
		doe_seq_echo(YK3_TAB "cfg evq block size      : %u", cfg->evq_block_size);
		doe_seq_echo(YK3_TAB "cfg evq block order     : %u", cfg->evq_block_order);
		doe_seq_echo(YK3_TAB "cfg evq batch count     : %u", cfg->evq_batch_count);
		doe_seq_echo(YK3_TAB "cfg irq coalesce timeout: %u", cfg->irq_coalesce.timeout);
		doe_seq_echo(YK3_TAB "cfg irq coalesce count  : %u", cfg->irq_coalesce.count);

		doe_seq_echo(YK3_TAB "vfc depth    : %llu", vfc->depth);
		doe_seq_echo(YK3_TAB "vfc count    : %llu", doe_vfc_count(vfc));
		doe_seq_echo(YK3_TAB "vfc prod     : %llu", vfc->prod);
		doe_seq_echo(YK3_TAB "vfc cons     : %llu", vfc->cons);
		doe_seq_echo(YK3_TAB "vfc times cons update : %llu", vfc->times_cons_update);

		doe_seq_echo(YK3_TAB "evq depth    : %d", evq->depth);
		doe_seq_echo(YK3_TAB "evq count    : %u", doe_evq_count(evq));
		doe_seq_echo(YK3_TAB "evq left     : %u", doe_evq_left(evq));
		doe_seq_echo(YK3_TAB "evq prod     : %u", doe_evq_prod(evq));
		doe_seq_echo(YK3_TAB "evq cons     : %u", doe_evq_cons(evq));
#if K3MAX_PREVIEW
		doe_seq_echo(YK3_TAB "evq dma_address      : 0x%llx",
			     le64_to_cpu(evq->desc->dma_address));
		doe_seq_echo(YK3_TAB "evq dma_wr_cnt_out   : 0x%x",
			     le16_to_cpu(evq->desc->dma_wr_cnt_out));
		doe_seq_echo(YK3_TAB "evq dma_rd_cnt_out   : 0x%x",
			     le16_to_cpu(evq->desc->dma_rd_cnt_out));
		doe_seq_echo(YK3_TAB "evq dma_wr_buf_space : 0x%x",
			     le16_to_cpu(evq->desc->dma_wr_buf_space));
		doe_seq_echo(YK3_TAB "evq dma_rd_buf_space : 0x%x",
			     le16_to_cpu(evq->desc->dma_rd_buf_space));
#endif
		doe_seq_echo(YK3_TAB "fifo depth   : %u", YK3_DOE_FIFO_DEPTH);
		doe_seq_echo(YK3_TAB "fifo count   : %u", doe_fifo_count(fifo));
		doe_seq_echo(YK3_TAB "fifo left    : %u", doe_fifo_left(fifo));
		doe_seq_echo(YK3_TAB "fifo prod raw: %u", fifo->prod);
		doe_seq_echo(YK3_TAB "fifo cons raw: %u", fifo->cons);
		doe_seq_echo(YK3_TAB "fifo prod    : %u", fifo->prod - fifo->init_cons);
		doe_seq_echo(YK3_TAB "fifo cons    : %u", fifo->cons - fifo->init_cons);
		doe_seq_echo(YK3_TAB "fifo init    : %u", fifo->init_cons);
		doe_seq_echo(YK3_TAB "fifo times cons update: %llu", fifo->times_cons_update);
		doe_seq_echo(YK3_TAB "fifo times dequeue    : %llu", fifo->times_dequeue);

		doe_seq_echo(YK3_TAB "cb pool depth: %u", YK3_DOE_CB_POOL_DEPTH);
		doe_seq_echo(YK3_TAB "cb pool count: %u", doe_cb_pool_count(cb_pool));
		doe_seq_echo(YK3_TAB "cb pool left : %u", doe_cb_pool_left(cb_pool));
		doe_seq_echo(YK3_TAB "cb pool prod : %llu", cb_pool->prod);
		doe_seq_echo(YK3_TAB "cb pool cons : %llu", cb_pool->cons);
		doe_seq_echo(YK3_TAB "cb pool bug_cb_pool_put_with_not_enough: %llu",
			     (u64)atomic64_read(&cb_pool->st.bug_cb_pool_put_with_not_enough));

		doe_seq_echo(YK3_TAB "tag depth    : %u", (u32)YK3_DOE_CMD_TAG_COUNT);
		doe_seq_echo(YK3_TAB "tag count    : %llu", doe_tag_pool_count(tag_pool));
		doe_seq_echo(YK3_TAB "tag left     : %llu", doe_tag_pool_left(tag_pool));
		doe_seq_echo(YK3_TAB "tag prod     : %llu", tag_pool->prod);
		doe_seq_echo(YK3_TAB "tag cons     : %llu", tag_pool->cons);

		doe_seq_echo(YK3_TAB "cmd pool     : %d", manager->pool_enable);
		doe_seq_echo(YK3_TAB "cmd bug_get_with_pool_disable    : %llu",
			     (u64)atomic64_read(&manager->st.bug_get_with_pool_disable));
		doe_seq_echo(YK3_TAB "cmd get_with_pool_empty          : %llu",
			     (u64)atomic64_read(&manager->st.get_with_pool_empty));
		doe_seq_echo(YK3_TAB "cmd bug_put_with_pool_disable    : %llu",
			     (u64)atomic64_read(&manager->st.bug_put_with_pool_disable));
		doe_seq_echo(YK3_TAB "cmd bug_put_with_pool_full       : %llu",
			     (u64)atomic64_read(&manager->st.bug_put_with_pool_full));
		doe_seq_echo(YK3_TAB "cmd register_with_pool_empty     : %llu",
			     (u64)atomic64_read(&manager->st.register_with_pool_empty));
		doe_seq_echo(YK3_TAB "cmd bug_unregister_with_pool_full: %llu",
			     (u64)atomic64_read(&manager->st.bug_unregister_with_pool_full));
		doe_seq_echo(YK3_TAB "cmd err_alloc_nil                : %llu",
			     (u64)atomic64_read(&manager->st.err_alloc_nil));

		for (tag = 0; tag < YK3_DOE_CMD_TAG_COUNT; tag++) {
			slot = &manager->tag_slot[tag];

			if (slot->times_unknown) {
				doe_seq_echo(YK3_TAB "tag[0x%04x] times times_unknown : %u",
					     tag, slot->times_unknown);
			}

			if (slot->times_timeout) {
				doe_seq_echo(YK3_TAB "tag[0x%04x] times times_timeout : %u",
					     tag, slot->times_timeout);
			}

			if (slot->times_lated) {
				doe_seq_echo(YK3_TAB "tag[0x%04x] times times_lated : %u",
					     tag, slot->times_lated);
			}

			if (slot->times_get != slot->times_put) {
				doe_seq_echo(YK3_TAB "tag[0x%04x] times get[%u] != put[%u]",
					     tag, slot->times_get, slot->times_put);
			}
		}
	}

#if K3MAX_PREVIEW
	for (i = 0; i < 2; i++) {
		doe_if = &doe_priv->doe_if[i];
		evq = &doe_if->evq;

		doe_seq_echo("%s-interface desc dma:", (i == YK3_DOE_RD) ? "read" : "write");
		doe_seq_dump_buffer(evq->desc, sizeof(*evq->desc));
	}
#endif

	return 0;
}

static int
yk3_doe_gst_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_doe_priv *doe_priv = pdev_priv->doe_priv;
	struct yk3_doe_page_group *group;
	struct yk3_doe_xie_info *info;
	struct yk3_doe_page *page;
	const char *name;
	int i, j, page_count;

	if (!is_doe_pdev_mgr(pdev_priv))
		return 0;

	// show xie info
	for (i = 0; i < YK3_DOE_XIE_END; i++) {
		info = &doe_priv->xie_info[i];

		doe_seq_echo("%s: %s kmax[%u] vmax[%u] cache[%u%u]",
			     doe_xie_name(i), doe_cache_mode_name(info->cache_mode),
			     info->max_key_len, info->max_value_len,
			     info->cache_entry_count, info->cache_entry_limit);
	}
	doe_seq_blank();

	// show page
	for (i = 0; i < DOE_PAGE_GROUP_END; i++) {
		name = doe_page_group_name(i);
		group = &doe_priv->page_group[i];
		page_count = group->page_count;
		if (!page_count)
			continue;

		doe_seq_echo("%s pageinfo start[%llx], end[%llx], size[0x%x], first_page_index[%u]",
			     name, group->start, group->end,
			     group->page_size, group->first_page_index);
		doe_seq_blank();

		for (j = 0; j < page_count; j++) {
			page = &group->pages[j];

			doe_seq_echo("%s pagemap[%d] size[%uM] pa[0x%llx] va[0x%llx] dma[%llx]",
				     name, j,
				     YK3_MB(page->size), page->pa, YK3_PTR(page->va), page->dma);
		}
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_doe_hst_debugfs);
DEFINE_SHOW_ATTRIBUTE(yk3_doe_sst_debugfs);
DEFINE_SHOW_ATTRIBUTE(yk3_doe_gst_debugfs);

void doe_debugfs_fini(struct yk3_doe_priv *doe_priv)
{
	debugfs_remove(doe_priv->dbgfs_doe_hst_file);
	doe_priv->dbgfs_doe_hst_file = NULL;

	debugfs_remove(doe_priv->dbgfs_doe_sst_file);
	doe_priv->dbgfs_doe_sst_file = NULL;

	debugfs_remove(doe_priv->dbgfs_doe_gst_file);
	doe_priv->dbgfs_doe_gst_file = NULL;
}

int doe_debugfs_init(struct yk3_doe_priv *doe_priv)
{
	struct yk3_pdev_priv *pdev_priv = doe_priv->pdev_priv;
	struct dentry *entry;

	if (!pdev_priv->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("doe_hst", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_doe_hst_debugfs_fops);
	if (!entry) {
		doe_err("Failed to create debugfs doe_hst for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	doe_priv->dbgfs_doe_hst_file = entry;
	doe_debug(DOE_DEBUG_INIT, "create debugfs doe_hst for pdev %s\n", pdev_priv->name);

	entry = debugfs_create_file("doe_sst", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_doe_sst_debugfs_fops);
	if (!entry) {
		doe_err("Failed to create debugfs doe_sst for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	doe_priv->dbgfs_doe_sst_file = entry;
	doe_debug(DOE_DEBUG_INIT, "create debugfs doe_sst for pdev %s\n", pdev_priv->name);

	entry = debugfs_create_file("doe_gst", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_doe_gst_debugfs_fops);
	if (!entry) {
		doe_err("Failed to create debugfs doe_gst for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	doe_priv->dbgfs_doe_gst_file = entry;
	doe_debug(DOE_DEBUG_INIT, "create debugfs doe_gst for pdev %s\n", pdev_priv->name);

	return 0;
}

/**************************************************************************************************/
static long
yk3_doe_cdev_create_tbl(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_cfg arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	return yk3_doe_create_tbl(priv->pdev_priv, &arg);
}

static long
yk3_doe_cdev_delete_tbl(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	int tbl_id;

	if (copy_from_user(&tbl_id, (void __user *)_arg, sizeof(tbl_id)))
		return -EFAULT;

	return yk3_doe_delete_tbl(priv->pdev_priv, tbl_id);
}

static long
yk3_doe_cdev_entry_add(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_entry arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	return yk3_doe_entry_add(priv->pdev_priv, &arg, arg.call_mode | YK3_DOE_USER_CALL);
}

static long
yk3_doe_cdev_entry_delete(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_entry arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	return yk3_doe_entry_delete(priv->pdev_priv, &arg, arg.call_mode | YK3_DOE_USER_CALL);
}

static long
yk3_doe_cdev_entry_update(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_entry arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	return yk3_doe_entry_update(priv->pdev_priv, &arg, arg.call_mode | YK3_DOE_USER_CALL);
}

static long
yk3_doe_cdev_entry_query(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_entry arg;
	int ret = 0;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	ret = yk3_doe_entry_query(priv->pdev_priv, &arg, YK3_DOE_SYNC_CALL | YK3_DOE_USER_CALL);

	if (copy_to_user((void __user *)_arg, &arg, sizeof(arg)))
		return -EFAULT;

	return ret;
}

static long
yk3_doe_cdev_ctl_get(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_ctl arg;
	int ret = 0;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	ret = yk3_doe_ctl_get(priv->pdev_priv, &arg);

	if (copy_to_user((void __user *)_arg, &arg, sizeof(arg)))
		return -EFAULT;

	return ret;
}

static long
yk3_doe_cdev_ctl_set(struct file *file, unsigned int cmd, unsigned long _arg)
{
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_tbl_ctl arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	return yk3_doe_ctl_set(priv->pdev_priv, &arg);
}

static long
yk3_doe_cdev_test(struct file *file, unsigned int cmd, unsigned long _arg)
{
	int ret = -EINVAL;
#if YK3_DOE_TEST
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_doe_priv_test_perf arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	ret = doe_ioctl_test(priv->pdev_priv, &arg);
#endif
	return ret;
}

static long
yk3_doe_cdev_measure(struct file *file, unsigned int cmd, unsigned long _arg)
{
	int ret = -EINVAL;
#if YK3_DOE_MEASURE
	struct yk3_cdev_priv *priv = file->private_data;
	struct yk3_doe_priv_measure arg;

	if (copy_from_user(&arg, (void __user *)_arg, sizeof(arg)))
		return -EFAULT;

	ret = doe_ioctl_measure(priv->pdev_priv, &arg);
	if (ret < 0)
		return ret;

	if (copy_to_user((void __user *)_arg, &arg, sizeof(arg)))
		return -EFAULT;
#endif
	return ret;
}

static unlocked_ioctl yk3_doe_ioctls[] = {
	[YK3_DOE_CREATE_TABLE]	= yk3_doe_cdev_create_tbl,
	[YK3_DOE_DELETE_TABLE]	= yk3_doe_cdev_delete_tbl,

	[YK3_DOE_ADD_ENTRY]	= yk3_doe_cdev_entry_add,
	[YK3_DOE_DELETE_ENTRY]	= yk3_doe_cdev_entry_delete,
	[YK3_DOE_UPDATE_ENTRY]	= yk3_doe_cdev_entry_update,
	[YK3_DOE_QUERY_ENTRY]	= yk3_doe_cdev_entry_query,

	[YK3_DOE_GET_CTRL]	= yk3_doe_cdev_ctl_get,
	[YK3_DOE_SET_CTRL]	= yk3_doe_cdev_ctl_set,

	[YK3_DOE_PRIV_TEST]	= yk3_doe_cdev_test,
	[YK3_DOE_PRIV_MEASURE]	= yk3_doe_cdev_measure,
};

static size_t yk3_doe_arg_sizes[] = {
	[YK3_DOE_CREATE_TABLE]	= sizeof(struct yk3_tbl_cfg),
	[YK3_DOE_DELETE_TABLE]	= sizeof(int),

	[YK3_DOE_ADD_ENTRY]	= sizeof(struct yk3_tbl_entry),
	[YK3_DOE_DELETE_ENTRY]	= sizeof(struct yk3_tbl_entry),
	[YK3_DOE_UPDATE_ENTRY]	= sizeof(struct yk3_tbl_entry),
	[YK3_DOE_QUERY_ENTRY]	= sizeof(struct yk3_tbl_entry),

	[YK3_DOE_GET_CTRL]	= sizeof(struct yk3_tbl_ctl),
	[YK3_DOE_SET_CTRL]	= sizeof(struct yk3_tbl_ctl),

	[YK3_DOE_PRIV_TEST]	= sizeof(struct yk3_doe_priv_test_perf),
	[YK3_DOE_PRIV_MEASURE]	= sizeof(struct yk3_doe_priv_measure),
};

long yk3_doe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_DOE_CMD_END)
		return -EINVAL;

	//目前还没有对struct yk3_cdev_priv的doe_priv赋值
	//使用pdev_priv即可，doe_priv用不着，可以考虑删除
	if (!priv->pdev_priv)
		return -ENODEV;

	if (_IOC_SIZE(cmd) != yk3_doe_arg_sizes[nr])
		return -EINVAL;

	//这里ioctl的返回值要copy_to_user给用户态
	return yk3_doe_ioctls[nr](file, cmd, arg);
}

/**************************************************************************************************/
#if YK3_DOE_ONLY
static struct miscdevice yk3doe_cdev;
static struct yk3_pdev_priv yk3doe_pdev_priv;
static struct yk3_card yk3doe_card;

static long
yk3doe_comm_dev_bind(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct yk3_cdev_priv *priv = file->private_data;

	if (priv->pdev_priv)
		return -EEXIST;

	priv->pdev_priv = &yk3doe_pdev_priv;
	return 0;
}

static unlocked_ioctl yk3doe_cdev_comm_ioctls[] = {
	[YK3_COMM_DEVBIND] = yk3doe_comm_dev_bind,
};

static size_t yk3doe_cdev_comm_arg_sizes[] = {
	[YK3_COMM_DEVBIND] = sizeof(struct yk3_comm_devid),
};

static long
yk3doe_cdev_comm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_COMM_MAX)
		return -EINVAL;

	if (_IOC_SIZE(cmd) != yk3doe_cdev_comm_arg_sizes[nr])
		return -EINVAL;

	if (nr != YK3_COMM_DEVBIND && !priv->pdev_priv)
		return -ENODEV;

	return yk3doe_cdev_comm_ioctls[nr](file, cmd, arg);
}

static long
yk3doe_cdev_doe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int nr = _IOC_NR(cmd);
	struct yk3_cdev_priv *priv = file->private_data;

	if (nr >= YK3_DOE_CMD_END)
		return -EINVAL;

	//目前还没有对struct yk3_cdev_priv的doe_priv赋值
	//使用pdev_priv即可，doe_priv用不着，可以考虑删除
	if (!priv->pdev_priv)
		return -ENODEV;

	if (_IOC_SIZE(cmd) != yk3_doe_arg_sizes[nr])
		return -EINVAL;

	//这里ioctl的返回值要copy_to_user给用户态
	return yk3_doe_ioctls[nr](file, cmd, arg);
}

static long
yk3doe_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (_IOC_TYPE(cmd)) {
	case YK3_IOCTL_COMM_TYPE:
		return yk3doe_cdev_comm_ioctl(file, cmd, arg);
	case YK3_IOCTL_DOE_TYPE:
		return yk3doe_cdev_doe_ioctl(file, cmd, arg);
	default:
		return -EINVAL;
	};
}

static int
yk3doe_cdev_open(struct inode *inode, struct file *file)
{
	struct yk3_cdev_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	file->private_data = priv;

	return 0;
}

static int
yk3doe_cdev_release(struct inode *inode, struct file *file)
{
	struct yk3_cdev_priv *priv = file->private_data;

	kfree(priv);
	file->private_data = NULL;
	return 0;
}

static const struct file_operations yk3doe_cdev_fops = {
	.owner = THIS_MODULE,
	.open = yk3doe_cdev_open,
	.unlocked_ioctl = yk3doe_cdev_ioctl,
	.release = yk3doe_cdev_release,
};

int yk3doe_cdev_init(void)
{
	int ret;

	yk3doe_cdev.minor = MISC_DYNAMIC_MINOR;
	yk3doe_cdev.name = "yk3";
	yk3doe_cdev.fops = &yk3doe_cdev_fops;

	ret = misc_register(&yk3doe_cdev);
	if (ret) {
		yk3_err("misc_register failed: %d\n", ret);
		return ret;
	}

	return 0;
}

void yk3doe_cdev_exit(void)
{
	misc_deregister(&yk3doe_cdev);
}

enum {
	yk3doe_enable_device,
	yk3doe_request_regions,
	yk3doe_bar_ioremap,
	yk3doe_debugfs_init,
	yk3doe_irq_alloc,
	yk3doe_doe_init,
};

static struct dentry *yk3doe_debugfs_root;
static u64 yk3doe_mask;

#define is_inited(bit)	(yk3doe_mask & BIT(bit))
#define set_init(bit)	(yk3doe_mask |= BIT(bit))

static void
yk3doe_remove(struct pci_dev *pdev)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(pdev);

	if (!pdev_priv)
		return;

	if (is_inited(yk3doe_doe_init))
		yk3_doe_exit(pdev_priv);

	if (is_inited(yk3doe_irq_alloc))
		pci_free_irq_vectors(pdev);

	if (is_inited(yk3doe_debugfs_init))
		debugfs_remove_recursive(pdev_priv->dbgfs_dir);

	if (is_inited(yk3doe_bar_ioremap))
		iounmap(pdev_priv->bar_addr[0]);

	if (is_inited(yk3doe_request_regions))
		pci_release_regions(pdev);

	if (is_inited(yk3doe_enable_device)) {
		pci_clear_master(pdev);
		pci_disable_device(pdev);
	}

	pci_set_drvdata(pdev, NULL);
}

static int
yk3doe_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct yk3_pdev_priv *pdev_priv = &yk3doe_pdev_priv;
	struct yk3_card *card = &yk3doe_card;
	struct device *dev = &pdev->dev;
	int ret;

	doe_crit("YUSUR K3/K3MAX DOE Driver %s Probe ...", THIS_MODULE->name);

	// 1. bind
	snprintf(pdev_priv->name, sizeof(pdev_priv->name), "%04x:%02x:%02x.%d",
		 pci_domain_nr(pdev->bus), pdev->bus->number,
		 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	pci_set_drvdata(pdev, pdev_priv);

	pdev_priv->dev = dev;
	pdev_priv->pdev = pdev;
	pdev_priv->card = card;
	pdev_priv->vendor = id->vendor;
	pdev_priv->device = id->device;
	pdev_priv->pf_id = PCI_FUNC(pdev->devfn);
	card->mgr_pdev_priv = pdev_priv;

	// 2. enable device
	ret = pci_enable_device(pdev);
	if (ret) {
		yk3_dev_err("pci_enable_device() failed\n");
		goto error;
	}
	pci_set_master(pdev);
	set_init(yk3doe_enable_device);

	// 3. Request MMIO/IOP resources
	ret = pci_request_regions(pdev, pdev->driver->name);
	if (ret) {
		yk3_dev_err("pci_request_regions() failed\n");
		goto error;
	}
	set_init(yk3doe_request_regions);

	// 4. bar remap
	pdev_priv->bar_pa[0] = pci_resource_start(pdev, 0);
	pdev_priv->bar_size[0] = pci_resource_len(pdev, 0);
	if (!pdev_priv->bar_size[0]) {
		ret = -EIO;
		goto error;
	}
	pdev_priv->bar_addr[0] = ioremap(pdev_priv->bar_pa[0], pdev_priv->bar_size[0]);
	if (!pdev_priv->bar_addr[0]) {
		ret = -EIO;
		yk3_dev_err("could not map bar0\n");
		goto error;
	}
	set_init(yk3doe_bar_ioremap);
	yk3_dev_info("bar0 pa[0x%llx] size[%uM] va[0x%llx]\n",
		     pdev_priv->bar_pa[0],
		     YK3_MB(pdev_priv->bar_size[0]),
		     YK3_PTR(pdev_priv->bar_addr[0]));

	// 5. dma mask
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		yk3_dev_err("dma_set_mask_and_coherent() failed\n");
		goto error;
	}
	dma_set_max_seg_size(&pdev->dev, DMA_BIT_MASK(32));

	// 6. debugfs
	pdev_priv->dbgfs_dir = debugfs_create_dir(pdev_priv->name, yk3doe_debugfs_root);
	if (!pdev_priv->dbgfs_dir) {
		yk3_err("Failed to create debugfs directory for pdev %s", pdev_priv->name);
		ret = -ENOMEM;
		goto error;
	}
	set_init(yk3doe_debugfs_init);

	// 7. irq
	ret = pci_alloc_irq_vectors(pdev_priv->pdev, 1, YK3_DOE_IRQ_EVQ + 2, PCI_IRQ_MSIX);
	if (ret <= 0) {
		yk3_dev_err("failed to allocate msix vectors, ret %d\n", ret);
		goto error;
	}
	yk3_dev_info("alloc irq vector: %d\n", ret);
	set_init(yk3doe_irq_alloc);

	// 8. doe init
	ret = yk3_doe_init(pdev_priv);
	if (ret) {
		yk3_dev_err("yk3_doe_init() failed\n");
		goto error;
	}
	set_init(yk3doe_doe_init);

	doe_crit("YUSUR K3/K3MAX DOE Driver %s Probe ok.", THIS_MODULE->name);
	return 0;
error:
	yk3doe_remove(pdev);

	return ret;
}

static const struct pci_device_id yk3doe_pci_ids[] = {
#if YK3_DOE_U200
	{ PCI_DEVICE(0x10ee, 0x9338) },
#else
	{ PCI_VDEVICE(YUSUR, YK3_DEV_ID_MGR) },
#endif
	{ 0, },
};

static struct pci_driver yk3doe_driver = {
	.name = "yk3doe",
	.id_table = yk3doe_pci_ids,
	.probe = yk3doe_probe,
	.remove = yk3doe_remove,
};

static int
yk3doe_param_init(void)
{
	int ret;

	ret = yk3_doe_param_init();
	if (ret)
		return ret;

	return 0;
}

static int __init yk3doe_init(void)
{
	int ret;

	doe_crit("YUSUR K3/K3MAX DOE Driver %s Init ...", THIS_MODULE->name);

	ret = yk3doe_param_init();
	if (ret < 0)
		return ret;

	yk3doe_debugfs_root = debugfs_create_dir("yk3", NULL);
	if (IS_ERR(yk3doe_debugfs_root))
		yk3_err("Failed to create debugfs root directory");

	ret = pci_register_driver(&yk3doe_driver);
	if (ret) {
		yk3_err("pci_register_driver() mgr failed\n");
		goto err_drv;
	}

	ret = yk3doe_cdev_init();
	if (ret) {
		yk3_err("yk3doe_cdev_init() failed\n");
		goto err_cdev;
	}

	doe_crit("YUSUR K3/K3MAX DOE Driver %s Init", THIS_MODULE->name);
	return 0;

err_cdev:
	pci_unregister_driver(&yk3doe_driver);
err_drv:
	debugfs_remove_recursive(yk3doe_debugfs_root);
	return ret;
}

static void __exit yk3doe_exit(void)
{
	doe_crit("YUSUR K3/K3MAX DOE Driver %s Exit ...", THIS_MODULE->name);

	yk3doe_cdev_exit();
	pci_unregister_driver(&yk3doe_driver);
	debugfs_remove_recursive(yk3doe_debugfs_root);

	doe_crit("YUSUR K3/K3MAX DOE Driver %s Exit", THIS_MODULE->name);
}

module_init(yk3doe_init);
module_exit(yk3doe_exit);

MODULE_DESCRIPTION("Yusur K3/K3MAX DOE Pcie Device Driver");
MODULE_AUTHOR("YUSUR Technology Co., Ltd.");
MODULE_LICENSE("GPL");
MODULE_VERSION(YK3_GIT_VERSION);
#endif
/**************************************************************************************************/
#if YK3_DOE_TEST
static void
doe_test_delay(int iter, u64 *ns_last)
{
	u64 ns_now;

	if ((iter & 0xff) == 0)  {
		ns_now = ktime_get_ns();
		if ((ns_now - *ns_last) > DOE_TEST_TIMEOUT) {
			*ns_last = ns_now;
			usleep_range(1, 2);
		}
	}
}

static int
doe_test_thread(void *data)
{
	struct doe_test_param *tparam = data;
	struct yk3_doe_priv *doe_priv = tparam->doe_priv;
	struct yk3_pdev_priv *pdev_priv = doe_priv->pdev_priv;
	struct yk3_doe_priv_test_perf *param = &tparam->param;
	int idx = tparam->idx;
	int tbl_id = param->tbl_id;
	int call_mode = param->call_mode;
	int count = param->depth / (param->n_thread ? : 1);
	int start = count * idx;
	int i, ret;
	struct yk3_tbl_entry entry = {
		.tbl_type = param->tbl_type,
		.tbl_id = tbl_id,
		.flags = param->flags,
		.call_mode = call_mode,
	};
	union {
		u8 raw[128];
		u32 v;
	} key, value;
	u64 ns_start = ktime_get_ns();
	u64 ns_stop, ns, pps;
	u64 ns_last = ns_start;

	doe_crit("%s table[%d] perf[%d] start[%d] count[%d]",
		 tparam->name, tbl_id, idx, start, count);
	atomic_inc(&doe_test_ref);

	while (!READ_ONCE(doe_test_start))
		usleep_range(100, 200);

	switch (param->tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_SMALL_ARRAY:
		entry.array.value_len	= param->value_len;
		entry.array.index	= start;
		entry.array._value	= value.raw;

		for (i = 0; i < count; i++) {
			ret = yk3_doe_entry_add(pdev_priv, &entry, call_mode);
			if (ret < 0)
				goto exit;

			entry.array.index++;
			value.v++;
			doe_test_delay(i, &ns_last);
		}
		break;
	case DOE_TABLE_COUNTER:
		entry.counter.update.enable = 1;
		entry.counter.update.index = start;

		for (i = 0; i < count; i++) {
			ret = yk3_doe_entry_update(pdev_priv, &entry, call_mode);
			if (ret < 0)
				goto exit;

			entry.counter.update.index++;
			doe_test_delay(i, &ns_last);
		}
		break;
	case DOE_TABLE_METER:
		entry.meter.index = start;

		for (i = 0; i < count; i++) {
			ret = yk3_doe_entry_update(pdev_priv, &entry, call_mode);
			if (ret < 0)
				goto exit;

			entry.meter.index++;
			doe_test_delay(i, &ns_last);
		}
		break;
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		entry.hash.key_len = param->key_len;
		entry.hash.value_len = param->value_len;
		entry.hash._key = key.raw;
		entry.hash._value = value.raw;
		key.v = start;

		for (i = 0; i < count; i++) {
			ret = yk3_doe_entry_add(pdev_priv, &entry, call_mode);
			if (ret < 0)
				goto exit;

			key.v++;
			value.v++;
			doe_test_delay(i, &ns_last);
		}
		break;
	default:
		ret = -E_DOE_NOTSUPP;
		goto exit;
	}

	ns_stop = ktime_get_ns();
	ns = ns_stop - ns_start;
	pps = (u64)count * 1000000000 / ns;
	doe_test_param[idx].pps = pps;
	doe_crit("%s table[%d] perf[%d] ns: %llu, count: %u, pps: %llu",
		 tparam->name, tbl_id, idx, ns, count, pps);
exit:
	if (ret)
		doe_error(ret, "%s table[%d] perf[%d]", tparam->name, tbl_id, idx);

	if (!atomic_dec_return(&doe_test_ref)) {
		usleep_range(10000, 20000);
		yk3_doe_delete_tbl(doe_priv->pdev_priv, param->tbl_id);

		if (param->n_thread > 1) {
			pps = 0;
			for (i = 0; i < param->n_thread; i++)
				pps += doe_test_param[i].pps;

			doe_crit("%s table[%d] perf total pps: %llu", tparam->name, tbl_id, pps);
		}
	}

	return ret;
}

static int
doe_ioctl_test(struct yk3_pdev_priv *pdev_priv, struct yk3_doe_priv_test_perf *param)
{
	struct yk3_tbl_cfg cfg = {
		.tbl_type = param->tbl_type,
		.tbl_id	= param->tbl_id,
		.depth = param->depth,
	};
	struct doe_test_param *tparam;
	struct yk3_doe_priv *doe_priv;
	int i, ret = 0, n_thread = param->n_thread ? : 1;

	atomic_set(&doe_test_ref, 0);
	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -EOPNOTSUPP;

	// all need hw check
	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	switch (param->tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_SMALL_ARRAY:
	case DOE_TABLE_COUNTER:
	case DOE_TABLE_METER:
		cfg.array.value_len = param->value_len;
		break;
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		cfg.hash.key_len = param->key_len;
		cfg.hash.value_len = param->value_len;
		cfg.hash.sdepth = param->depth;
		break;
	default:
		return -E_DOE_NOTSUPP;
	}

	doe_crit("doe test table[%d] depth[%u] thread[%d]",
		 param->tbl_id, param->depth, param->n_thread);

	ret = yk3_doe_create_tbl(doe_priv->pdev_priv, &cfg);
	if (ret < 0)
		return ret;

	yk3_objzero(&doe_test_param);
	WRITE_ONCE(doe_test_start, 0);
	for (i = 0; i < n_thread; i++) {
		tparam = &doe_test_param[i];
		tparam->doe_priv = doe_priv;
		yk3_objcpy(&tparam->param, param);
		tparam->idx = i;

		snprintf(tparam->name, sizeof(tparam->name), "doe-perf-%d", i);
		tparam->task = kthread_create(doe_test_thread, tparam, "%s", tparam->name);
		ret = PTR_ERR_OR_ZERO(tparam->task);
		if (ret)
			return doe_error(ret, "Failed to doe perf thread %d", i);
	}

	for (i = 0; i < n_thread; i++) {
		tparam = &doe_test_param[i];

		wake_up_process(tparam->task);
	}
	WRITE_ONCE(doe_test_start, 1);

	return 0;
}
#endif
/**************************************************************************************************/
#if YK3_DOE_MEASURE
static void
doe_measure_build(struct yk3_doe_if *doe_if)
{
#if YK3_DOE_MEASURE
	u64 times = 0, total = 0;
	int idx;

	for (idx = 0; idx < YK3_DOE_MEASURE_N; idx++) {
		times += doe_if->measure.avg[idx].times;
		total += doe_if->measure.avg[idx].total;
	}

	doe_if->measure.level[DOE_MEASURE_LEVEL_AVG].times = times;
	doe_if->measure.level[DOE_MEASURE_LEVEL_AVG].total = total;
#endif
}

static int
doe_ioctl_measure(struct yk3_pdev_priv *pdev_priv, struct yk3_doe_priv_measure *param)
{
	struct yk3_doe_priv *doe_priv = doe_priv_get(pdev_priv);
	struct yk3_doe_if *doe_if = doe_if_get(doe_priv, !!param->op);

	doe_measure_build(doe_if);
	yk3_objcpy(&param->level, &doe_if->measure.level);
	doe_measure_init(doe_if);

	return 0;
}
#endif
/**************************************************************************************************/
