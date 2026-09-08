// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"

#define YK3_N_PF_IRQ_MAXNUM		2048

struct yk3_irq_ent {
	struct notifier_block nb;
	char name[YK3_N_NAME_LEN];
	int vector;
	irq_handler_t handler;
	void *data;
	struct list_head irq_node;
};

struct yk3_irq {
	char name[YK3_N_NAME_LEN];
	atomic_t used;
	int vector;
	int irqn;
	int state;
	u64 flags;
	int cpu;
	struct atomic_notifier_head nh;
	struct mutex mlock;		/* protect ent_head */
	struct list_head ent_head;
};

struct yk3_irq_mgr {
	atomic_t free;
	atomic_t rdma_used;
	u16 num[YK3_N_PF_MAX_FUNC];
};

struct yk3_irq_table {
	u16 count;
	u16 limit;
	u16 sriov_num;
	u16 rdma_num;
	atomic_t used;

	/* debugfs */
	struct dentry *dbgfs_info_file;

	/* pf irq mgr */
	struct yk3_irq_mgr *mgr;

	struct mutex mlock;		/* protect irqs */
	struct yk3_irq irqs[];
};

static int yk3_pdev_irq_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_irq_table *table = pdev_priv->irq_table;
	struct yk3_irq *irq;
	struct yk3_irq_ent *ent;
	int i;

	seq_printf(seq, "\t    %-16s : %-16d\n", "count", table->count);
	seq_printf(seq, "\t    %-16s : %-16d\n", "limit", table->limit);
	seq_printf(seq, "\t    %-16s : %-16d\n", "used", atomic_read(&table->used));

	mutex_lock(&table->mlock);
	for (i = 0; i < table->count; i++) {
		irq = &table->irqs[i];
		if (!atomic_read(&irq->used))
			continue;

		seq_printf(seq, "\t    %-16s %-4d :\n", "vector", irq->vector);
		seq_printf(seq, "\t\t    %-16s : %-16s\n", "name", irq->name);
		seq_printf(seq, "\t\t    %-16s : %-16d\n", "used", atomic_read(&irq->used));
		seq_printf(seq, "\t\t    %-16s : %-16d\n", "irqn", irq->irqn);
		seq_printf(seq, "\t\t    %-16s : %-16d\n", "state", irq->state);
		seq_printf(seq, "\t\t    %-16s : %-16llx\n", "flags", irq->flags);
		seq_printf(seq, "\t\t    %-16s : %-16d\n", "cpu", irq->cpu);

		mutex_lock(&irq->mlock);
		list_for_each_entry(ent, &irq->ent_head, irq_node) {
			seq_printf(seq, "\t\t    %-16s : %-16s\n", "ent_name", ent->name);
		}
		mutex_unlock(&irq->mlock);
	}
	mutex_unlock(&table->mlock);

	if (yk3_pdev_is_pf(pdev_priv)) {
		seq_printf(seq, "\t    %-16s : %-16d\n", "mgr free",
			   atomic_read(&table->mgr->free));
		seq_printf(seq, "\t    %-16s : %-16d\n", "mgr rdma_used",
			   atomic_read(&table->mgr->rdma_used));
		for (i = 0; i < YK3_N_PF_MAX_FUNC; i++) {
			if (!table->mgr->num[i])
				continue;
			seq_printf(seq, "\t    %-16s %-4d : %-16d\n", "func",
				   i, table->mgr->num[i]);
		}
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_pdev_irq_debugfs);

static int yk3_debug_irq_init(struct yk3_pdev_priv *pdev_priv)
{
	struct dentry *entry;

	if (!pdev_priv->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("irq_info", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_pdev_irq_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs irq info file for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	pdev_priv->irq_table->dbgfs_info_file = entry;

	return 0;
}

static void yk3_debug_irq_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (!pdev_priv->irq_table)
		return;

	debugfs_remove(pdev_priv->irq_table->dbgfs_info_file);
	pdev_priv->irq_table->dbgfs_info_file = NULL;
}

static int yk3_notifier_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	struct yk3_irq_ent *ent = container_of(nb, struct yk3_irq_ent, nb);

	ent->handler(ent->vector, ent->data);
	return NOTIFY_DONE;
}

static irqreturn_t yk3_irq_handler(int irqn, void *data)
{
	struct yk3_irq *irq = data;

	atomic_notifier_call_chain(&irq->nh, 0, irq);
	return IRQ_HANDLED;
}

static int yk3_irq_table_get_free(struct yk3_irq_table *table)
{
	int i;
	struct yk3_irq *irq = NULL;

	mutex_lock(&table->mlock);
	for (i = 0; i < table->limit; i++) {
		if (!table->irqs[i].state) {
			irq = &table->irqs[i];
			irq->state = 1;
			atomic_inc(&irq->used);
			break;
		}
	}
	mutex_unlock(&table->mlock);

	if (irq)
		return irq->vector;

	return -ENOENT;
}

static int yk3_irq_table_get_lru(struct yk3_irq_table *table)
{
	int i;
	struct yk3_irq *irq = NULL;
	int min = INT_MAX;

	mutex_lock(&table->mlock);
	for (i = 0; i < table->limit; i++) {
		if (atomic_read(&table->irqs[i].used) < min &&
		    !(table->irqs[i].flags & YK3_F_IRQ_EXCLUSIVE)) {
			min = atomic_read(&table->irqs[i].used);
			irq = &table->irqs[i];
		}
	}

	if (irq) {
		atomic_inc(&irq->used);
		mutex_unlock(&table->mlock);
		return irq->vector;
	}

	mutex_unlock(&table->mlock);
	return -ENOENT;
}

static int yk3_irq_table_get_vector(struct yk3_irq_table *table, int vector, u64 flags)
{
	int ret;
	struct yk3_irq *irq;

	if (vector >= table->limit)
		return -EINVAL;

	if (vector >= 0) {
		mutex_lock(&table->mlock);
		irq = &table->irqs[vector];
		if (irq->flags & YK3_F_IRQ_EXCLUSIVE) {
			mutex_unlock(&table->mlock);
			return -EBUSY;
		}

		irq->state = 1;
		atomic_inc(&irq->used);
		mutex_unlock(&table->mlock);
		return irq->vector;
	}

	ret = yk3_irq_table_get_free(table);
	if (ret >= 0)
		return ret;

	if (!(flags & YK3_F_IRQ_EXCLUSIVE)) {
		ret = yk3_irq_table_get_lru(table);
		if (ret >= 0)
			return ret;
	}

	return -ENOENT;
}

static void yk3_irq_table_put_vector(struct yk3_irq_table *table, int vector)
{
	struct yk3_irq *irq = &table->irqs[vector];

	mutex_lock(&table->mlock);
	if (atomic_dec_and_test(&irq->used))
		irq->state = 0;
	mutex_unlock(&table->mlock);
}

static int yk3_irq_table_get_affinity(struct yk3_pdev_priv *pdev_priv)
{
	u32 cpu_count;
	struct yk3_card *card = pdev_priv->card;
	u32 local_cpunum = cpumask_weight(card->local_cpumask);
	u32 neigh_cpunum = cpumask_weight(card->neigh_cpumask);
	int i, cpu;

	cpu_count = local_cpunum + neigh_cpunum;
	i = card->affinity_seqnum % cpu_count;
	card->affinity_seqnum++;

	if (i < local_cpunum) {
		for_each_cpu(cpu, card->local_cpumask) {
			if (i-- == 0)
				return cpu;
		}
	}

	i -= local_cpunum;
	for_each_cpu(cpu, card->neigh_cpumask) {
		if (i-- == 0)
			return cpu;
	}

	/* it cannot be here*/
	return -EFAULT;
}

int yk3_irq_request(struct yk3_pdev_priv *pdev_priv, struct yk3_irq_param *param)
{
	int ret;
	int vector;
	struct yk3_irq_ent *ent, *find = NULL;
	struct atomic_notifier_head *nh;
	struct yk3_irq *irq;
	struct yk3_irq_table *table = pdev_priv->irq_table;

	if (!param->handler || param->name[0] == '\0') {
		yk3_dev_err("request irq must have handler and name\n");
		return -EINVAL;
	}

	vector = param->vector;
	ret = yk3_irq_table_get_vector(table, vector, param->flags);
	if (ret < 0) {
		yk3_dev_err("failed to get vector, param vector %d\n", param->vector);
		goto err_get_vector;
	}
	vector = ret;

	irq = &table->irqs[vector];

	mutex_lock(&irq->mlock);
	list_for_each_entry(ent, &irq->ent_head, irq_node) {
		if (ent->handler == param->handler && ent->data == param->data) {
			find = ent;
			break;
		}
	}
	mutex_unlock(&irq->mlock);
	if (find) {
		yk3_dev_err("irq ent already exist\n");
		ret = -EEXIST;
		goto err_already_vector;
	}

	ent = kzalloc(sizeof(*ent), GFP_KERNEL);
	if (!ent) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	memcpy(ent->name, param->name, sizeof(ent->name));
	ent->vector = vector;
	ent->handler = param->handler;
	ent->data = param->data;
	INIT_LIST_HEAD(&ent->irq_node);
	ent->nb.notifier_call = yk3_notifier_handler;

	mutex_lock(&irq->mlock);
	list_add_tail(&ent->irq_node, &irq->ent_head);

	nh = &table->irqs[ent->vector].nh;
	ret = atomic_notifier_chain_register(nh, &ent->nb);
	if (ret < 0) {
		yk3_dev_err("failed to register notifier\n");
		goto err_register_notifier;
	}

	if (atomic_read(&irq->used) > 1)
		goto out;

	ret = request_irq(irq->irqn, yk3_irq_handler, 0, irq->name, irq);
	if (ret < 0) {
		yk3_dev_err("failed to request vector %d\n", irq->vector);
		goto err_request_irq;
	}

	irq->flags = param->flags;

	if (irq->flags & YK3_F_IRQ_AFFINITY) {
		irq->cpu = yk3_irq_table_get_affinity(pdev_priv);

		if (irq->cpu < 0)
			irq_set_affinity_hint(irq->irqn, pdev_priv->card->local_cpumask);
		else
			irq_set_affinity_hint(irq->irqn, get_cpu_mask(irq->cpu));
	} else {
		irq->cpu = -1;
	}
	atomic_inc(&table->used);

out:
	mutex_unlock(&irq->mlock);

	return irq->vector;

err_request_irq:
	atomic_notifier_chain_unregister(&irq->nh, &ent->nb);
err_register_notifier:
	list_del(&ent->irq_node);
	mutex_unlock(&irq->mlock);
	kfree(ent);
err_kzalloc:
err_already_vector:
	yk3_irq_table_put_vector(table, vector);
err_get_vector:
	return ret;
}

void yk3_irq_free(struct yk3_pdev_priv *pdev_priv, struct yk3_irq_param *param)
{
	int vector;
	struct yk3_irq_ent *ent, *find = NULL;
	struct yk3_irq *irq;

	if (param->vector < 0 || param->vector >= pdev_priv->irq_table->limit) {
		yk3_dev_err("invalid vector %d\n", param->vector);
		return;
	}

	vector = param->vector;
	irq = &pdev_priv->irq_table->irqs[vector];

	mutex_lock(&irq->mlock);

	list_for_each_entry(ent, &irq->ent_head, irq_node) {
		if (ent->handler == param->handler && ent->data == param->data) {
			find = ent;
			break;
		}
	}

	if (!find) {
		dump_stack();
		yk3_dev_err("failed to find irq ent\n");
		mutex_unlock(&irq->mlock);
		return;
	}

	if (atomic_read(&irq->used) == 1) {
		irq_set_affinity_hint(irq->irqn, NULL);
		irq->cpu = -1;
		free_irq(irq->irqn, irq);
		irq->flags = 0;
		atomic_dec(&pdev_priv->irq_table->used);
	}

	atomic_notifier_chain_unregister(&irq->nh, &find->nb);
	list_del(&find->irq_node);
	mutex_unlock(&irq->mlock);

	yk3_irq_table_put_vector(pdev_priv->irq_table, vector);

	kfree(find);
}

static void yk3_irq_free_force(struct yk3_irq *irq)
{
	struct yk3_irq_ent *ent, *tmp;

	mutex_lock(&irq->mlock);
	list_for_each_entry_safe(ent, tmp, &irq->ent_head, irq_node) {
		atomic_notifier_chain_unregister(&irq->nh, &ent->nb);
		list_del(&ent->irq_node);
		kfree(ent);
	}
	mutex_unlock(&irq->mlock);

	free_irq(irq->irqn, irq);
	atomic_set(&irq->used, 0);
}

void yk3_irq_set_limit(struct yk3_pdev_priv *pdev_priv, u16 limit)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;

	if (limit > table->count)
		limit = table->count;

	if (limit < atomic_read(&table->used)) {
		yk3_dev_warn("limit %d is less than used %d, so limit %d\n",
			     limit, atomic_read(&table->used), atomic_read(&table->used));
		limit = atomic_read(&table->used);
	}

	table->limit = limit;
	if (yk3_pdev_is_pf(pdev_priv)) {
		atomic_set(&table->mgr->free, table->count - limit);
		table->mgr->num[0] = limit;
	}
}

void yk3_irq_add_reserve(struct yk3_pdev_priv *pdev_priv, u16 reserve)
{
	u16 limit;

	if (pdev_priv->irq_table->limit < reserve)
		return;
	limit = pdev_priv->irq_table->limit - reserve;
	yk3_irq_set_limit(pdev_priv, limit);
}

int yk3_irq_get_freenum(struct yk3_pdev_priv *pdev_priv)
{
	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	return atomic_read(&pdev_priv->irq_table->mgr->free);
}

void yk3_irq_set_func_irqnum(struct yk3_pdev_priv *pdev_priv, u16 func, u16 num)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;
	u32 val;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	if (num > atomic_read(&table->mgr->free) + table->mgr->num[func])
		return;

	atomic_add(table->mgr->num[func], &table->mgr->free);
	table->mgr->num[func] = num;
	atomic_sub(num, &table->mgr->free);
	val = FIELD_PREP(YK3_RP_VFX_IRQNUM_GMASK, num);
	yk3_wr32(pdev_priv->bar_addr[YK3_BAR0], YK3_RP_VFX_IRQNUM(func), val);
}

void yk3_irq_rdma_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;
	struct yk3_card *card = pdev_priv->card;
	int count;

	if (!yk3_pdev_is_pf(pdev_priv) || card->mode != YK3_MODE_RCARD)
		return;

	count = min_t(int, 256, table->limit);
	atomic_set(&table->mgr->rdma_used, count);
	yk3_irq_add_reserve(pdev_priv, count);
}

void yk3_irq_sriov_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;
	struct yk3_card *card = pdev_priv->card;
	int count;
	int pf_num;
	int vf_num;
	int rdma_num;
	int totalvfs = pci_sriov_get_totalvfs(pdev_priv->pdev);

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	if (pdev_priv->card->mode == YK3_MODE_RCARD)
		rdma_num = 256;
	else
		rdma_num = 0;

	pf_num = card->pf_ndev_qnum * 2 + 2;

	vf_num = ((card->qnum / card->pf_num) - card->pf_ndev_qnum) * 2;
	vf_num += totalvfs;

	if (table->limit < rdma_num + pf_num) {
		if (pdev_priv->card->mode == YK3_MODE_RCARD) {
			rdma_num = table->limit - pf_num;
			rdma_num = max_t(int, rdma_num, 1);
		}
	}

	count = min_t(int, table->limit - pf_num - rdma_num, vf_num);
	yk3_irq_add_reserve(pdev_priv, count);
}

int yk3_irq_rdma_get(struct yk3_pdev_priv *pdev_priv, int *vec_start, int *vec_num)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	if (pdev_priv->card->mode != YK3_MODE_RCARD ||
	    atomic_read(&table->mgr->rdma_used) == 0)
		return -ENOSPC;

	*vec_start = table->limit;
	*vec_num = atomic_read(&table->mgr->rdma_used);

	return 0;
}

int yk3_irq_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_irq_table *table;
	size_t size;
	int ret, i;
	struct yk3_irq *irq;
	u32 val;

	ret = pci_msix_vec_count(pdev_priv->pdev);
	if (ret < 0) {
		yk3_dev_err("failed to get msix vec count\n");
		return ret;
	}

	if (yk3_pdev_is_vf(pdev_priv)) {
		val = yk3_rd32(pdev_priv->bar_addr[YK3_BAR0], YK3_RP_VFX_IRQNUM(pdev_priv->vf_id));
		i = FIELD_GET(YK3_RP_VFX_IRQNUM_GMASK, val);
		ret = min_t(int, i, ret);
	}

	ret = pci_alloc_irq_vectors(pdev_priv->pdev, 1, ret, PCI_IRQ_MSIX);
	if (ret <= 0) {
		yk3_dev_err("failed to allocate msix vectors, ret %d\n", ret);
		return ret;
	}

	size = sizeof(*table) + sizeof(struct yk3_irq) * ret;
	if (yk3_pdev_is_pf(pdev_priv))
		size += sizeof(struct yk3_irq_mgr);

	table = kzalloc(size, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	table->count = (u16)ret;
	table->limit = table->count;
	atomic_set(&table->used, 0);
	mutex_init(&table->mlock);

	if (yk3_pdev_is_pf(pdev_priv)) {
		table->mgr = (struct yk3_irq_mgr *)(&table->irqs[table->count]);
		atomic_set(&table->mgr->free, 0);
		table->mgr->num[0] = table->count;
	}

	for (i = 0; i < table->count; i++) {
		irq = &table->irqs[i];

		snprintf(irq->name, sizeof(irq->name), "%s-vector-%d",
			 pdev_priv->name, i);
		irq->state = 0;
		irq->vector = i;
		irq->irqn = pci_irq_vector(pdev_priv->pdev, i);
		ATOMIC_INIT_NOTIFIER_HEAD(&irq->nh);
		mutex_init(&irq->mlock);
		atomic_set(&irq->used, 0);
		INIT_LIST_HEAD(&irq->ent_head);
	}

	pdev_priv->irq_table = table;

	ret = yk3_debug_irq_init(pdev_priv);
	if (ret) {
		yk3_dev_err("failed to init debug irq\n");
		goto err_debug_irq_init;
	}

	return 0;

err_debug_irq_init:
	kfree(table);
	pdev_priv->irq_table = NULL;
	pci_free_irq_vectors(pdev_priv->pdev);

	return ret;
}

void yk3_irq_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_irq_table *table = pdev_priv->irq_table;
	int i;
	struct yk3_irq *irq;

	if (!table)
		return;

	yk3_debug_irq_exit(pdev_priv);

	for (i = 0; i < table->count; i++) {
		irq = &table->irqs[i];

		if (!irq->state)
			continue;

		yk3_dev_err("vector %d is not free, now force free it\n", irq->vector);
		yk3_irq_free_force(irq);
	}

	kfree(table);
	pdev_priv->irq_table = NULL;

	pci_free_irq_vectors(pdev_priv->pdev);
}
