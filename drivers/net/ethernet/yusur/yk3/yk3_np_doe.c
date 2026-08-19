// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_np_priv.h"

struct yk3_np_cnt_tbl_entry {
	u64 pkts;
	u64 bytes;
};

enum { YK3_NP_CNT_TBL_ID = 103 };

enum {
	YK3_NP_METRICS_CNT_LOAD_TOTAL = 0,
	YK3_NP_METRICS_CNT_ENTRY_UPDATE_TOTAL,
	YK3_NP_METRICS_WORK_RUN_TOTAL,
	YK3_NP_METRICS_MAX,
};

enum {
	YK3_NP_STATS_CNT_LOAD_RET_FAIL = 0,
	YK3_NP_STATS_CNT_LOAD_TIMEOUT,
	YK3_NP_STATS_MAX,
};

struct yk3_np_cnt_tbl_priv {
	struct delayed_work          np_work;
	unsigned long                work_interval;
	struct mutex                 work_mutex; /* Protect cancel_delayed_work_sync. */
	bool                         work_cancel;
	struct                       yk3_np *np;
	u32                          nb_entries;
	u16                          id;
	int                          tbl_type;
	const char                   *name;
	struct dentry                *debugfs_file;
	struct dentry                *info_dbgfs;
	struct mutex                 mlock; /* for table entry update and debugfs */
	atomic64_t                   stats[YK3_NP_STATS_MAX];
	atomic64_t                   metrics[YK3_NP_METRICS_MAX];

	struct yk3_np_cnt_tbl_entry  entry_list[];
};

struct yk3_np_table {
	struct yk3_pdev_priv         *pdev_priv;
	const struct yk3_np_tbl_ops  *ops;
	struct list_head             node;
	void                         *priv;
};

struct yk3_np_tbl_ops {
	const char *name;
	const u32 mode_bitmap;
	struct yk3_np_table *(*create)(struct yk3_pdev_priv *pdev_priv);
	void (*destroy)(struct yk3_np_table *table);
};

int yk3_doe_tbl_exist(struct yk3_pdev_priv *pdev_priv, u8 tbl_id, bool *exist)
{
	int ret = 0;
	struct yk3_tbl_ctl ctrl = {
		.action_type = YK3_DOE_TLB_EXISTED,
		.tbl_existed.tbl_id = tbl_id
	};

	/* 1: exist
	 * 0: NOT exist
	 * <0: error
	 */
	ret = yk3_doe_ctl_get(pdev_priv, &ctrl);
	if (ret < 0)
		return ret;
	else if (ret == 1)
		*exist = true;
	else if (ret == 0)
		*exist = false;
	else
		return ret;

	return 0;
}

int yk3_doe_set_protect(struct yk3_pdev_priv *pdev_priv, bool protect)
{
	int ret = 0;
	size_t i = 0;
	const u8 status = protect ? 1 : 0;
	const size_t max_wait_loop = 10;
	const unsigned long wait_slice = 1000;

	struct yk3_tbl_ctl set_protect = {
		.action_type = YK3_DOE_SET_PROTECT,
		.protect.status = status,
	};
	struct yk3_tbl_ctl get_protect = {
		.action_type = YK3_DOE_GET_PROTECT,
	};

	yk3_dev_info("[NP] request to %s protect.", protect ? "set" : "unset");
	ret = yk3_doe_ctl_set(pdev_priv, &set_protect);
	if (ret) {
		yk3_dev_err("[NP] Failed to call doe yk3_doe_ctl_set, ret %d.", ret);
		return ret;
	}

	for (i = 0; i < max_wait_loop; i++) {
		usleep_range(wait_slice / 2, wait_slice);

		/* 1: protect open
		 * 0: protect close
		 * <0: error
		 */
		ret = yk3_doe_ctl_get(pdev_priv, &get_protect);
		if (ret < 0 || ret > 1) {
			yk3_dev_err("Failed to get doe protect status, ret %d.", ret);
			return ret;
		}
		if (ret == status)
			return 0;
	}

	yk3_dev_err("Failed to wait for doe protect status.");
	return -EINVAL;
}

int yk3_doe_counter_enable(struct yk3_pdev_priv *pdev_priv, u8 tbl_id,
			   u32 idx,
			   bool op_enable,
			   bool pri,
			   bool clear_data)
{
	int ret = 0;
	struct yk3_tbl_entry tbl_req = {
		.tbl_type = DOE_TABLE_COUNTER,
		.tbl_id = tbl_id,
		.counter.update = {
			.index = idx,
			.enable = op_enable ? 1 : 0,
			.high_pri = pri ? 1 : 0,
			.clear_data = clear_data ? 1 : 0,
		},
	};

	ret = yk3_doe_entry_update(pdev_priv, &tbl_req, 0);
	if (ret)
		yk3_dev_err("[NP] %s call doe entry_update failed, ret %d.",
			    __func__, ret);
	return ret;
}

int yk3_doe_counter_load(struct yk3_pdev_priv *pdev_priv, u8 tbl_id,
			 u32 idx, u64 *pkts, u64 *bytes,
			 bool allow_sleep)
{
	int ret = 0;
	struct yk3_tbl_entry tbl_req = {
		.tbl_type = DOE_TABLE_COUNTER,
		.tbl_id = tbl_id,
		.counter.query = {
			.index = idx,
		},
		.flags = allow_sleep ? YK3_DOE_F_WAIT_SLEEP : 0,
	};

	ret = yk3_doe_entry_query(pdev_priv, &tbl_req, 0);
	if (ret)
		return ret;

	*pkts = be64_to_cpu((__force __be64)tbl_req.counter.query.entry.count);
	*bytes = be64_to_cpu((__force __be64)tbl_req.counter.query.entry.bytes);

	return 0;
}

static void yk3_np_cnt_tbl_walk_update_counter_load(struct yk3_np_cnt_tbl_priv *priv)
{
	int ret = 0;
	u32 idx_pos = 0;
	const bool allow_sleep = true;
	struct yk3_np *np = priv->np;
	struct yk3_np_cnt_tbl_entry *entry = NULL;

	mutex_lock(&priv->mlock);
	for (idx_pos = 0; idx_pos < priv->nb_entries; idx_pos++) {
		atomic64_inc(&priv->metrics[YK3_NP_METRICS_CNT_LOAD_TOTAL]);

		entry = &priv->entry_list[idx_pos];
		ret = yk3_doe_counter_load(np->pdev_priv, priv->id, idx_pos,
					   &entry->pkts, &entry->bytes, allow_sleep);
		if (ret)
			atomic64_inc(&priv->stats[YK3_NP_STATS_CNT_LOAD_RET_FAIL]);
		else
			atomic64_inc(&priv->metrics[YK3_NP_METRICS_CNT_ENTRY_UPDATE_TOTAL]);

		if (ret == -E_DOE_TIMEOUT)
			atomic64_inc(&priv->stats[YK3_NP_STATS_CNT_LOAD_TIMEOUT]);
	}
	mutex_unlock(&priv->mlock);
}

static void yk3_np_cnt_work(struct work_struct *work)
{
	struct yk3_np_cnt_tbl_priv *priv = container_of(work,
						       struct yk3_np_cnt_tbl_priv, np_work.work);
	struct yk3_np *np = priv->np;

	yk3_np_cnt_tbl_walk_update_counter_load(priv);
	atomic64_inc(&priv->metrics[YK3_NP_METRICS_WORK_RUN_TOTAL]);

	mutex_lock(&priv->work_mutex);
	if (!priv->work_cancel)
		queue_delayed_work(np->wq, &priv->np_work, priv->work_interval);
	mutex_unlock(&priv->work_mutex);
}

static int yk3_np_cnt_tbl_info_dbgfs_show(struct seq_file *seq, void *data)
{
	struct yk3_np_cnt_tbl_priv *priv = seq->private;
	size_t i = 0;
	u64 val_show = 0;
	static const char *metric_name[ARRAY_SIZE(priv->metrics)] = {
		[YK3_NP_METRICS_CNT_LOAD_TOTAL]         = "Counter load total",
		[YK3_NP_METRICS_CNT_ENTRY_UPDATE_TOTAL] = "Counter entry update total",
		[YK3_NP_METRICS_WORK_RUN_TOTAL]         = "Work run total",
	};
	static const char *stat_name[ARRAY_SIZE(priv->stats)] = {
		[YK3_NP_STATS_CNT_LOAD_RET_FAIL]        = "Counter load ret fail total",
		[YK3_NP_STATS_CNT_LOAD_TIMEOUT]         = "Counter load timeout total",
	};

	seq_printf(seq, "%-30s: %s\n", "name", priv->name);
	seq_printf(seq, "%-30s: %d\n", "type", priv->tbl_type);
	seq_printf(seq, "%-30s: %u\n", "size", priv->nb_entries);
	seq_printf(seq, "%-30s: %lu\n", "work interval", priv->work_interval);

	/* Metrics */
	for (i = 0; i < ARRAY_SIZE(priv->metrics); i++) {
		val_show = atomic64_read(&priv->metrics[i]);
		seq_printf(seq, "%-30s: %llu\n", metric_name[i], val_show);
	}

	/* Stats errors */
	for (i = 0; i < ARRAY_SIZE(priv->stats); i++) {
		val_show = atomic64_read(&priv->stats[i]);
		if (val_show)
			seq_printf(seq, "%-30s: %llu\n", stat_name[i], val_show);
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_np_cnt_tbl_info_dbgfs);

static int yk3_np_cnt_tbl_debugfs_show(struct seq_file *seq, void *data)
{
	struct yk3_np_cnt_tbl_priv *priv = seq->private;
	const struct yk3_np_cnt_tbl_entry *entry = NULL;
	size_t i = 0;

	seq_printf(seq, "name %s : size %u.\n", priv->name, priv->nb_entries);
	seq_printf(seq, "%-8s + %-8s\n", "idx", "value");

	mutex_lock(&priv->mlock);
	for (i = 0; i < priv->nb_entries; i++) {
		entry = &priv->entry_list[i];
		seq_printf(seq, "index : %-8lu", i);
		seq_printf(seq, "packets : %-20llu", entry->pkts);
		seq_printf(seq, "bytes : %-20llu", entry->bytes);

		seq_puts(seq, "\n");
	}
	mutex_unlock(&priv->mlock);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_np_cnt_tbl_debugfs);

static struct yk3_np_table *
yk3_np_cnt_tbl_create(struct yk3_pdev_priv *pdev_priv)
{
	const char *tbl_name = "np_internal_cnt_table";
	const int tbl_id = YK3_NP_CNT_TBL_ID;
	struct yk3_tbl_cfg extra = {
		.tbl_type      = DOE_TABLE_COUNTER,
		.tbl_id        = tbl_id,
		.endian        = YK3_DOE_ENDIAN_BIG,
		.depth         = 1024,
	};
	const unsigned long work_interval = 10000;
	const bool op_enable = true;
	const bool clear_data = true;
	const bool pri = false;

	int ret = 0;
	u32 i = 0;
	char name[32] = {0};
	size_t alloc_size = 0;
	struct yk3_np_table *table = NULL;
	struct yk3_np_cnt_tbl_priv *priv = NULL;
	bool tbl_exist = false;

	struct yk3_np *np = pdev_priv->card->np;
	const int mode = pdev_priv->card->mode;

	/* Overwrite the size as it's on ram. */
	if (mode == YK3_MODE_TCARD)
		extra.depth = 64;

	ret = yk3_doe_tbl_exist(pdev_priv, tbl_id, &tbl_exist);
	if (ret) {
		yk3_dev_err("Failed to get table exists %d, ret %d.", tbl_id, ret);
		goto fail;
	}
	if (tbl_exist)
		yk3_doe_delete_tbl(pdev_priv, tbl_id);

	ret = yk3_doe_create_tbl(pdev_priv, &extra);
	if (ret) {
		yk3_dev_err("Failed to create table %d, ret %d.", tbl_id, ret);
		goto fail;
	}

	for (i = 0; i < extra.depth; i++) {
		ret = yk3_doe_counter_enable(pdev_priv, tbl_id, i, op_enable, pri, clear_data);
		if (ret) {
			yk3_dev_err("Failed to enable index %u, table %d, ret %d.", i, tbl_id, ret);
			goto fail_with_tbl;
		}
	}

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto fail_with_tbl;

	alloc_size = sizeof(*priv) + sizeof(struct yk3_np_cnt_tbl_entry) * extra.depth;
	priv = kzalloc(alloc_size, GFP_KERNEL);
	if (!priv)
		goto fail_with_alloc;

	if (np->debugfs_root) {
		snprintf(name, sizeof(name), "table_%d", tbl_id);
		priv->debugfs_file = debugfs_create_file(name, 0400, np->debugfs_root,
							 priv, &yk3_np_cnt_tbl_debugfs_fops);
		if (IS_ERR_OR_NULL(priv->debugfs_file)) {
			yk3_dev_err("Failed to create debugfs file %s\n", name);
			priv->debugfs_file = NULL;
		}

		snprintf(name, sizeof(name), "table_info_%d", tbl_id);
		priv->info_dbgfs = debugfs_create_file(name, 0400, np->debugfs_root,
						       priv, &yk3_np_cnt_tbl_info_dbgfs_fops);
		if (IS_ERR_OR_NULL(priv->info_dbgfs)) {
			yk3_dev_err("Failed to create debugfs file %s.\n", name);
			priv->info_dbgfs = NULL;
		}
	}

	mutex_init(&priv->mlock);
	priv->np = np;
	priv->work_interval = msecs_to_jiffies(work_interval);
	priv->nb_entries = extra.depth;
	priv->id = tbl_id;
	priv->tbl_type = extra.tbl_type;
	priv->name = tbl_name;

	for (i = 0; i < ARRAY_SIZE(priv->metrics); i++)
		atomic64_set(&priv->metrics[i], 0);
	for (i = 0; i < ARRAY_SIZE(priv->stats); i++)
		atomic64_set(&priv->stats[i], 0);

	table->priv = priv;
	table->pdev_priv = pdev_priv;

	mutex_init(&priv->work_mutex);
	INIT_DELAYED_WORK(&priv->np_work, yk3_np_cnt_work);
	queue_delayed_work(np->wq, &priv->np_work, priv->work_interval);
	return table;

fail_with_alloc:
	kfree(table);
fail_with_tbl:
	yk3_doe_delete_tbl(pdev_priv, tbl_id);
fail:
	return NULL;
}

static void yk3_np_cnt_tbl_destroy(struct yk3_np_table *table)
{
	struct yk3_np_cnt_tbl_priv *priv = table->priv;
	struct yk3_pdev_priv *pdev_priv = table->pdev_priv;

	/*
	 * Issue refer to https://lore.kernel.org/all/aZLotq3aZY0b-dI8@v4bel
	 * In short, with out mutex, queue_delayed_work may run after
	 * cancel_delayed_work_sync, which could result in crash.
	 *
	 * cancel_delayed_work_sync in new kernel can replace the mutex.
	 */
	mutex_lock(&priv->work_mutex);
	priv->work_cancel = true;
	mutex_unlock(&priv->work_mutex);
	cancel_delayed_work_sync(&priv->np_work);

	debugfs_remove(priv->debugfs_file);
	debugfs_remove(priv->info_dbgfs);

	yk3_doe_delete_tbl(pdev_priv, priv->id);

	kfree(priv);
	kfree(table);
}

static const struct yk3_np_tbl_ops yk3_np_cnt_tbl_ops = {
	.name = "NP counter table",
	//TODO: Add dpu soc bit.
	.mode_bitmap = BIT(YK3_MODE_TCARD) | BIT(YK3_MODE_ECARD),
	.create = yk3_np_cnt_tbl_create,
	.destroy = yk3_np_cnt_tbl_destroy,
};

static const struct yk3_np_tbl_ops *yk3_np_tbl_ops_list[] = {
	&yk3_np_cnt_tbl_ops,
};

int yk3_np_doe_tbl_init(struct yk3_pdev_priv *pdev_priv)
{
	size_t i = 0;
	const struct yk3_np_tbl_ops *tbl_ops = NULL;
	struct yk3_np_table *table = NULL;
	struct yk3_np *np = pdev_priv->card->np;
	const int mode = pdev_priv->card->mode;

	for (i = 0; i < ARRAY_SIZE(yk3_np_tbl_ops_list); i++) {
		tbl_ops = yk3_np_tbl_ops_list[i];
		if (!(tbl_ops->mode_bitmap & BIT(mode)))
			continue;

		yk3_dev_info("np doe table create: %s.", tbl_ops->name);
		table = tbl_ops->create(pdev_priv);
		if (!table) {
			yk3_dev_err("Failed to run %s create.", tbl_ops->name);
			goto fail;
		}

		table->ops = tbl_ops;
		list_add(&table->node, &np->table_head);
	}

	yk3_dev_info("np doe table init success.");
	return 0;

fail:
	yk3_np_doe_tbl_fini(pdev_priv);
	return -EINVAL;
}

void yk3_np_doe_tbl_fini(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_np_table *table = NULL;
	struct yk3_np_table *tmp = NULL;
	struct yk3_np *np = pdev_priv->card->np;

	list_for_each_entry_safe(table, tmp, &np->table_head, node) {
		yk3_dev_info("np doe table destroy: %s.", table->ops->name);
		list_del(&table->node);
		table->ops->destroy(table);
	}
	yk3_dev_info("np doe table fini done.");
}
