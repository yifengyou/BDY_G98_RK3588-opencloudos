// SPDX-License-Identifier: GPL-2.0
/*
 * Protect some processes from being killed
 *
 * Copyright (c) 2024 Tencent. All Rights reserved.
 * Author: Yongliang Gao <leonylgao@tencent.com>
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kill_hook.h>

#define KILL_PROTECT_DIR           "kill_protect"
#define KILL_PROTECT_CMD_LEN       64
#define KILL_PROTECT_RULES_MAX_CNT 128

struct kp_blacklist_rule {
	char comm[TASK_COMM_LEN];
	struct list_head node;
};

static unsigned int two = 2;
static unsigned int sysctl_sig_kill_protect;
static atomic_t kp_rule_cnt = ATOMIC_INIT(0);
static atomic64_t kp_protect_cnt = ATOMIC64_INIT(0);
static struct ctl_table_header *kp_sysctl_header;
static struct proc_dir_entry *kp_proc_dir;
static struct proc_dir_entry *blacklist_entry;
static struct proc_dir_entry *stat_entry;
static LIST_HEAD(blacklist_list);
static DEFINE_RWLOCK(blacklist_lock);
static struct kill_hook kill_protect_hook;

static ssize_t blacklist_write(struct file *file, const char __user *ubuf,
		size_t count, loff_t *ppos)
{
	char cmd[KILL_PROTECT_CMD_LEN] = {0};
	char comm[TASK_COMM_LEN] = {0};
	char *token[2], *str;
	int cnt, i;
	struct kp_blacklist_rule *rule, *tmp;

	cnt = min_t(size_t, count, KILL_PROTECT_CMD_LEN - 1);
	if (strncpy_from_user(cmd, ubuf, cnt) < 0)
		return -EFAULT;

	if (strlen(cmd) < 1)
		return -EINVAL;

	str = cmd;
	str[cnt - 1] = '\0';

	i = 0;
	while ((token[i] = strsep(&str, " ")) != NULL) {
		if (!strlen(token[i]))
			break;
		i++;
		if (i == 2)
			break;
	}

	if (i == 1) {
		if (strcmp(token[0], "flush") == 0) {
			write_lock(&blacklist_lock);
			list_for_each_entry_safe(rule, tmp, &blacklist_list, node) {
				list_del(&rule->node);
				kfree(rule);
			}
			atomic_set(&kp_rule_cnt, 0);
			write_unlock(&blacklist_lock);
			return count;
		}
		return -EINVAL;
	} else if (i != 2) {
		return -EINVAL;
	}

	if (strcmp(token[0], "add") == 0) {
		if (atomic_read(&kp_rule_cnt) >= KILL_PROTECT_RULES_MAX_CNT)
			return -ENOMEM;

		rule = kzalloc(sizeof(struct kp_blacklist_rule), GFP_KERNEL);
		if (!rule)
			return -ENOMEM;

		cnt = min_t(size_t, TASK_COMM_LEN - 1, strlen(token[1]));
		strncpy(rule->comm, token[1], cnt);
		rule->comm[cnt] = '\0';

		write_lock(&blacklist_lock);
		list_for_each_entry(tmp, &blacklist_list, node) {
			if (strcmp(tmp->comm, rule->comm) == 0) {
				write_unlock(&blacklist_lock);
				kfree(rule);
				return -EEXIST;
			}
		}
		list_add(&rule->node, &blacklist_list);
		atomic_inc(&kp_rule_cnt);
		write_unlock(&blacklist_lock);
	} else if (strcmp(token[0], "del") == 0) {
		cnt = min_t(size_t, TASK_COMM_LEN - 1, strlen(token[1]));
		strncpy(comm, token[1], cnt);
		comm[cnt] = '\0';

		write_lock(&blacklist_lock);
		list_for_each_entry_safe(rule, tmp, &blacklist_list, node) {
			if (strcmp(rule->comm, comm) == 0) {
				list_del(&rule->node);
				atomic_dec(&kp_rule_cnt);
				write_unlock(&blacklist_lock);
				kfree(rule);
				return count;
			}
		}
		write_unlock(&blacklist_lock);
		return -ESRCH;
	} else {
		return -EINVAL;
	}

	return count;
}

static void *blacklist_seq_start(struct seq_file *m, loff_t *pos)
{
	read_lock(&blacklist_lock);
	return seq_list_start_head(&blacklist_list, *pos);
}

static void *blacklist_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	return seq_list_next(v, &blacklist_list, pos);
}

static void blacklist_seq_stop(struct seq_file *m, void *v)
{
	read_unlock(&blacklist_lock);
}

static int blacklist_seq_show(struct seq_file *m, void *v)
{
	struct kp_blacklist_rule *rule;

	if (v == &blacklist_list) {
		seq_puts(m, "comm\n");
	} else {
		rule = list_entry(v, struct kp_blacklist_rule, node);
		seq_printf(m, "%s\n", rule->comm);
	}

	return 0;
}

static const struct seq_operations blacklist_seq_ops = {
	.start  = blacklist_seq_start,
	.next   = blacklist_seq_next,
	.stop   = blacklist_seq_stop,
	.show   = blacklist_seq_show,
};

static int blacklist_seq_open(struct inode *inode, struct file *filp)
{
	return seq_open(filp, &blacklist_seq_ops);
}

static const struct proc_ops blacklist_fops = {
	.proc_open	= blacklist_seq_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_write	= blacklist_write,
	.proc_release	= seq_release,
};

static int stat_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "rule count: %d\n", atomic_read(&kp_rule_cnt));
	seq_printf(m, "protect count: %lld\n", atomic64_read(&kp_protect_cnt));
	return 0;
}

bool kill_protect_blacklist_match(struct task_struct *p, int sig)
{
	struct kp_blacklist_rule *rule;
	bool curr_match = false;
	bool p_match = false;

	if (sysctl_sig_kill_protect == 0)
		return false;

	read_lock(&blacklist_lock);
	if (list_empty(&blacklist_list)) {
		read_unlock(&blacklist_lock);
		return false;
	}

	list_for_each_entry(rule, &blacklist_list, node) {
		if (!curr_match && strcmp(rule->comm, current->comm) == 0)
			curr_match = true;
		if (!p_match && strcmp(rule->comm, p->comm) == 0)
			p_match = true;
		if (curr_match && p_match)
			break;
	}
	read_unlock(&blacklist_lock);

	if (p_match && !curr_match) {
		atomic64_inc(&kp_protect_cnt);
		if (sysctl_sig_kill_protect == 2)
			pr_warn_ratelimited(
				"kill_protect: %s/%d send signal %d to %s/%d is not allowed\n",
				current->comm, current->pid, sig, p->comm, p->pid);
		return true;
	}

	return false;
}

static int kill_protect_hook_func(int sig, struct kernel_siginfo *info, struct task_struct *p)
{
	if (sig != SIGKILL && sig != SIGTERM)
		return 0;

	if (kill_protect_blacklist_match(p, sig))
		return -EPERM;

	return 0;
}

static int register_kill_protect_hook(void)
{
	kill_protect_hook.fn = kill_protect_hook_func;
	kill_protect_hook.priority = KILL_HOOK_PRIORITY_HIGH;

	return register_kill_hook(&kill_protect_hook);
}

static void unregister_kill_protect_hook(void)
{
	(void)unregister_kill_hook(&kill_protect_hook);
}

static struct ctl_table kp_sysctl_table[] = {
	{
		.procname       = "sig_kill_protect",
		.data           = &sysctl_sig_kill_protect,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0644,
		.proc_handler   = proc_douintvec_minmax,
		.extra1         = SYSCTL_ZERO,
		.extra2         = &two,
	},
	{ }
};

static int __init kill_protect_mod_init(void)
{
	int ret;

	kp_sysctl_header = register_sysctl("kernel", kp_sysctl_table);
	if (!kp_sysctl_header) {
		pr_err("register kill_protect sysctl table failed\n");
		return -ENOMEM;
	}

	kp_proc_dir = proc_mkdir(KILL_PROTECT_DIR, NULL);
	if (!kp_proc_dir) {
		pr_err("create kill_protect proc dir failed\n");
		goto out_dir;
	}

	blacklist_entry = proc_create("blacklist", 0644, kp_proc_dir, &blacklist_fops);
	if (!blacklist_entry) {
		pr_err("create blacklist proc entry failed\n");
		goto out_entry;
	}

	stat_entry = proc_create_single("stat", 0444, kp_proc_dir, stat_proc_show);
	if (!stat_entry) {
		pr_err("create stat proc entry failed\n");
		goto out_entry;
	}

	ret = register_kill_protect_hook();
	if (ret) {
		pr_err("register kill protect hook failed\n");
		goto out_entry;
	}

	pr_info("signal kill protect module init\n");
	return 0;

out_entry:
	remove_proc_subtree(KILL_PROTECT_DIR, NULL);
out_dir:
	unregister_sysctl_table(kp_sysctl_header);
	return -ENOMEM;
}

static void __exit kill_protect_mod_exit(void)
{
	struct kp_blacklist_rule *rule, *tmp;

	unregister_kill_protect_hook();

	remove_proc_subtree(KILL_PROTECT_DIR, NULL);
	write_lock(&blacklist_lock);
	list_for_each_entry_safe(rule, tmp, &blacklist_list, node) {
		list_del(&rule->node);
		kfree(rule);
	}
	write_unlock(&blacklist_lock);
	unregister_sysctl_table(kp_sysctl_header);

	pr_info("signal kill protect module exit\n");
}

module_init(kill_protect_mod_init);
module_exit(kill_protect_mod_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Yongliang Gao <leonylgao@tencent.com>");
MODULE_DESCRIPTION("Protect some processes from being killed");
MODULE_VERSION("1.0");
