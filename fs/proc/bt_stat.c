// SPDX-License-Identifier: GPL-2.0
#include <linux/cpumask.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/stat.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/irqnr.h>
#include <linux/sched/cputime.h>
#include <linux/tick.h>
#include <linux/sched/sysctl.h>

#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif

extern u64 get_idle_time(struct kernel_cpustat *kcs, int cpu);
extern u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu);
extern void show_all_irqs(struct seq_file *p);

#ifdef arch_idle_time
u64 get_iowait_time_bt(struct kernel_cpustat *kcs, int cpu)
{

	u64 iowait_bt;

	iowait_bt = kcs->cpustat[CPUTIME_BT_IOWAIT];
	if (cpu_online(cpu) && should_account_iowait_bt(cpu))
		iowait_bt += iowait_bt_cputime(cpu, arch_idle_time(cpu));
	return iowait_bt;
}
#else
u64 get_iowait_time_bt(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait_bt, iowait_bt_usecs = -1ULL;

	if (cpu_online(cpu))
		iowait_bt_usecs = get_cpu_iowait_bt_time_us(cpu, NULL);

	if (iowait_bt_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait_bt = kcs->cpustat[CPUTIME_BT_IOWAIT];
	else
		iowait_bt = iowait_bt_usecs * NSEC_PER_USEC;

	return iowait_bt;
}
#endif

static int __show_stat(struct seq_file *p, void *v, bool show_iowait_bt)
{
	int i, j;
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 bt, iowait_bt, iowait_bt_tmp;
	u64 guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec64 boottime;

	user = nice = system = idle = iowait =
		irq = softirq = steal = bt = iowait_bt = 0;
	guest = guest_nice = 0;
	getboottime64(&boottime);

	for_each_possible_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);

		user += kcs->cpustat[CPUTIME_USER];
		nice += kcs->cpustat[CPUTIME_NICE];
		system += kcs->cpustat[CPUTIME_SYSTEM];
		idle += get_idle_time(kcs, i);
		iowait += get_iowait_time(kcs, i);
		irq += kcs->cpustat[CPUTIME_IRQ];
		softirq += kcs->cpustat[CPUTIME_SOFTIRQ];
		steal += kcs->cpustat[CPUTIME_STEAL];
		guest += kcs->cpustat[CPUTIME_GUEST];
		guest_nice += kcs->cpustat[CPUTIME_GUEST_NICE];
		bt += kcs->cpustat[CPUTIME_BT];
		if (show_iowait_bt)
			iowait_bt += get_iowait_time_bt(kcs, i);
		else {
			if (sysctl_sched_bt_iowait) {
				iowait_bt_tmp = min_t(u64, get_iowait_time_bt(kcs, i), iowait);
				iowait = iowait - iowait_bt_tmp;
				idle += iowait_bt_tmp;
			}
		}
		sum += kstat_cpu_irqs_sum(i);
		sum += arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	seq_put_decimal_ull(p, "cpu  ", nsec_to_clock_t(user));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(nice));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(system));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(idle));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(iowait));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(irq));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(softirq));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(steal));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(guest));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(guest_nice));
	seq_put_decimal_ull(p, " ", nsec_to_clock_t(bt));
	if (show_iowait_bt)
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(iowait_bt));
	seq_putc(p, '\n');

	for_each_online_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);

		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user = kcs->cpustat[CPUTIME_USER];
		nice = kcs->cpustat[CPUTIME_NICE];
		system = kcs->cpustat[CPUTIME_SYSTEM];
		idle = get_idle_time(kcs, i);
		iowait = get_iowait_time(kcs, i);
		irq = kcs->cpustat[CPUTIME_IRQ];
		softirq = kcs->cpustat[CPUTIME_SOFTIRQ];
		steal = kcs->cpustat[CPUTIME_STEAL];
		guest = kcs->cpustat[CPUTIME_GUEST];
		guest_nice = kcs->cpustat[CPUTIME_GUEST_NICE];
		bt = kcs->cpustat[CPUTIME_BT];
		if (show_iowait_bt)
			iowait_bt = get_iowait_time_bt(kcs, i);
		else {
			if (sysctl_sched_bt_iowait) {
				iowait_bt_tmp = min_t(u64, get_iowait_time_bt(kcs, i), iowait);
				iowait = iowait - iowait_bt_tmp;
				idle += iowait_bt_tmp;
			}
		}
		seq_printf(p, "cpu%d", i);
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(user));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(nice));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(system));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(idle));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(iowait));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(irq));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(softirq));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(steal));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(guest));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(guest_nice));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(bt));
		if (show_iowait_bt)
			seq_put_decimal_ull(p, " ", nsec_to_clock_t(iowait_bt));
		seq_putc(p, '\n');
	}
	seq_put_decimal_ull(p, "intr ", (unsigned long long)sum);

	show_all_irqs(p);

	seq_printf(p,
		"\nctxt %llu\n"
		"btime %llu\n"
		"processes %lu\n"
		"procs_running %d\n"
		"procs_blocked %d\n"
		"procs_blocked_bt %lu\n",
		nr_context_switches(),
		(unsigned long long)boottime.tv_sec,
		nr_forks(),
		nr_running(),
		nr_iowait(),
		nr_iowait_bt());

	seq_put_decimal_ull(p, "softirq ", (unsigned long long)sum_softirq);

	for (i = 0; i < NR_SOFTIRQS; i++)
		seq_put_decimal_ull(p, " ", per_softirq_sums[i]);
	seq_putc(p, '\n');

	return 0;
}

int bt_show_stat(struct seq_file *p, void *v)
{
	return __show_stat(p, v, false);
}

int iowait_show_stat(struct seq_file *p, void *v)
{
	return __show_stat(p, v, true);
}

static int bt_aug_stat_open(struct inode *inode, struct file *file)
{
	unsigned int size = 1024 + 128 * num_online_cpus();

	/* minimum size to display an interrupt count : 2 bytes */
	size += 2 * nr_irqs;
	return single_open_size(file, iowait_show_stat, NULL, size);
}

static const struct proc_ops proc_bt_aug_stat_operations = {
	.proc_open              = bt_aug_stat_open,
	.proc_read              = seq_read,
	.proc_lseek             = seq_lseek,
	.proc_release   = single_release,
};

static int __init proc_stat_init(void)
{
	proc_create("bt_aug_stat", 0, NULL, &proc_bt_aug_stat_operations);
	return 0;
}
fs_initcall(proc_stat_init);
