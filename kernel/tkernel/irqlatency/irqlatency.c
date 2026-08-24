// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Trace Irqs latency
 *
 * Copyright (C) 2022 tencent, Inc., liu hua
 *
 * shookliu <shookliu@tencent.com>
 */
#define pr_fmt(fmt) "irqlatency: " fmt

#include <linux/cpu.h>
#include <linux/cpuhotplug.h>
#include <linux/hrtimer.h>
#include <linux/irqflags.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/stacktrace.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <asm/irq_regs.h>
#include <linux/sched/clock.h>

#define MAX_STACK_ENTRIES           (4096 / sizeof(unsigned long))
#define PER_STACK_ENTRIES_AVERAGE   (8 + 8)
#define MAX_STACK_ENTRIES_INDEX     (MAX_STACK_ENTRIES / PER_STACK_ENTRIES_AVERAGE)

#define MAX_LATENCY_RECORD          10

#define NS_TO_MS(ns) ((ns) / 1000000)

#define MIN_FREQ_MS  (5)
#define MAX_FREQ_MS  (5000)

struct per_stack {
	unsigned int nr_entries;
	unsigned long *perstack;
};

struct latency_data {
	raw_spinlock_t lock;
	u64 last_timestamp;
	unsigned long stack_index;
	struct per_stack stacks[MAX_STACK_ENTRIES];
	unsigned long total_entries;
	unsigned long entries[MAX_STACK_ENTRIES];
	atomic_long_t latency_count[MAX_LATENCY_RECORD];

	/* Task command names */
	char comms[MAX_STACK_ENTRIES_INDEX][TASK_COMM_LEN];

	/* Task pids */
	pid_t pids[MAX_STACK_ENTRIES_INDEX];

	struct {
		u64 msecs:63;
		u64 plus:1;
	} latency[MAX_STACK_ENTRIES_INDEX];
};

struct stack_snapshot {
	unsigned int nr_entries;
	unsigned int first_entry;
};

struct latency_snapshot {
	unsigned long stack_index;
	struct stack_snapshot stacks[MAX_STACK_ENTRIES_INDEX];
	unsigned long entries[MAX_STACK_ENTRIES];
	char comms[MAX_STACK_ENTRIES_INDEX][TASK_COMM_LEN];
	pid_t pids[MAX_STACK_ENTRIES_INDEX];
	u64 msecs[MAX_STACK_ENTRIES_INDEX];
	bool plus[MAX_STACK_ENTRIES_INDEX];
};

struct per_cpu_detect_data {
	unsigned int soft_in_irq;
	bool timers_active;
	struct timer_list softirq_timer;
	struct hrtimer irq_timer;
	struct latency_data irq_data;
	struct latency_data softirq_data;
};

enum irqlatency_state {
	IRQLATENCY_STOPPED,
	IRQLATENCY_IRQ_RUNNING,
	IRQLATENCY_SOFTIRQ_RUNNING,
	IRQLATENCY_EXITING,
};

static u64 freq_ms = 10;
static u64 irq_latency_ms = 30;
static enum irqlatency_state detector_state;
static int irqlatency_hp_state;
static DEFINE_MUTEX(control_lock);
static struct proc_dir_entry *latency_dir;

static struct per_cpu_detect_data __percpu *detect_data;

static bool irqlatency_running(enum irqlatency_state state)
{
	return state == IRQLATENCY_IRQ_RUNNING ||
	       state == IRQLATENCY_SOFTIRQ_RUNNING;
}

static unsigned int irqlatency_mode(void)
{
	enum irqlatency_state state = READ_ONCE(detector_state);

	return irqlatency_running(state) ? state : 0;
}

/*
 * Note: Must be called with irq disabled.
 */
static bool save_stack(struct per_cpu_detect_data *detect, u64 latency,
		       unsigned int isirq, unsigned int soft_in_irq)
{
	unsigned long nr_entries, stack_index;
	struct per_stack *pstack;
	struct latency_data *lat_data;
	bool saved = false;

	lat_data = isirq ? &detect->irq_data : &detect->softirq_data;

	raw_spin_lock(&lat_data->lock);
	stack_index = lat_data->stack_index;
	if (unlikely(stack_index >= MAX_STACK_ENTRIES_INDEX - 1))
		goto unlock;

	nr_entries = lat_data->total_entries;
	if (unlikely(nr_entries >= MAX_STACK_ENTRIES - 1))
		goto unlock;

	strlcpy(lat_data->comms[stack_index], current->comm, TASK_COMM_LEN);
	lat_data->pids[stack_index] = current->pid;
	lat_data->latency[stack_index].msecs = latency;
	lat_data->latency[stack_index].plus = !isirq && soft_in_irq;

	pstack = lat_data->stacks + stack_index;
	pstack->perstack = lat_data->entries + nr_entries;
	pstack->nr_entries = stack_trace_save(pstack->perstack,
				MAX_STACK_ENTRIES - nr_entries, 0);
	lat_data->total_entries += pstack->nr_entries;

	/*
	 * Ensure that the initialisation of @stacks is complete before we
	 * update the @index.
	 */
	smp_store_release(&lat_data->stack_index, stack_index + 1);

	if (unlikely(lat_data->total_entries >= MAX_STACK_ENTRIES - 1)) {
		pr_info("BUG: MAX_STACK_ENTRIES too low!");
		goto unlock;
	}
	saved = true;

unlock:
	raw_spin_unlock(&lat_data->lock);
	return saved;
}

static bool record_latency(struct per_cpu_detect_data *detect, u64 delta,
			   unsigned int isirq, unsigned int soft_in_irq)
{
	int index = 0;
	u64 frequency = READ_ONCE(freq_ms);
	u64 throttle = frequency << 1;

	if (delta < throttle)
		return false;

	if (unlikely(delta >= READ_ONCE(irq_latency_ms)))
		save_stack(detect, delta, isirq, soft_in_irq);

	delta -= frequency;
	delta >>= 1;
	while (delta >= frequency) {
		index++;
		delta >>= 1;
	}

	if (unlikely(index >= MAX_LATENCY_RECORD))
		index = MAX_LATENCY_RECORD - 1;

	if (isirq) {
		atomic_long_t *count;

		count = &detect->irq_data.latency_count[index];
		atomic_long_inc(count);
	} else if (!soft_in_irq) {
		atomic_long_t *count;

		count = &detect->softirq_data.latency_count[index];
		atomic_long_inc(count);
	}

	return true;
}

static void reset_latency_trace(void *data)
{
	int i;
	struct per_cpu_detect_data *detect_data = data;

	raw_spin_lock(&detect_data->irq_data.lock);
	detect_data->irq_data.total_entries = 0;
	detect_data->irq_data.stack_index = 0;
	raw_spin_unlock(&detect_data->irq_data.lock);

	raw_spin_lock(&detect_data->softirq_data.lock);
	detect_data->softirq_data.total_entries = 0;
	detect_data->softirq_data.stack_index = 0;
	raw_spin_unlock(&detect_data->softirq_data.lock);

	for (i = 0; i < MAX_LATENCY_RECORD; i++) {
		atomic_long_set(&detect_data->irq_data.latency_count[i], 0);
		atomic_long_set(&detect_data->softirq_data.latency_count[i], 0);
	}
}

static void softirq_timer_func(struct timer_list *softirq_timer)
{
	struct per_cpu_detect_data *data =
		from_timer(data, softirq_timer, softirq_timer);
	u64 now = local_clock(), delta;

	if (!READ_ONCE(data->timers_active) ||
	    !irqlatency_running(READ_ONCE(detector_state)))
		return;

	delta = now - data->softirq_data.last_timestamp;
	data->softirq_data.last_timestamp = now;
	data->soft_in_irq = 0;

	record_latency(data, NS_TO_MS(delta), 0, 0);

	if (READ_ONCE(data->timers_active) &&
	    irqlatency_running(READ_ONCE(detector_state)))
		mod_timer(softirq_timer,
			  jiffies + msecs_to_jiffies(READ_ONCE(freq_ms)));
}

static enum hrtimer_restart irq_hrtimer_func(struct hrtimer *irq_timer)
{
	struct per_cpu_detect_data *data =
		container_of(irq_timer, struct per_cpu_detect_data, irq_timer);
	u64 now = local_clock(), delta;

	if (!READ_ONCE(data->timers_active) ||
	    !irqlatency_running(READ_ONCE(detector_state)))
		return HRTIMER_NORESTART;

	delta = now - data->irq_data.last_timestamp;
	data->irq_data.last_timestamp = now;

	if (record_latency(data, NS_TO_MS(delta), 1, 0)) {
		data->softirq_data.last_timestamp = now;
	} else if (READ_ONCE(detector_state) == IRQLATENCY_SOFTIRQ_RUNNING &&
		   !data->soft_in_irq) {
		delta = now - data->softirq_data.last_timestamp;
		if (unlikely(NS_TO_MS(delta) >= READ_ONCE(irq_latency_ms) +
			     READ_ONCE(freq_ms))) {
			record_latency(data, NS_TO_MS(delta), 0, 1);
			data->soft_in_irq = 1;
		}
	}

	if (!READ_ONCE(data->timers_active) ||
	    !irqlatency_running(READ_ONCE(detector_state)))
		return HRTIMER_NORESTART;

	hrtimer_forward_now(irq_timer, ms_to_ktime(READ_ONCE(freq_ms)));

	return HRTIMER_RESTART;
}

static void percpu_timers_start(void *data)
{
	u64 now = local_clock();
	struct per_cpu_detect_data *detect_data = data;
	struct timer_list *softirq_timer = &detect_data->softirq_timer;
	struct hrtimer *irq_timer = &detect_data->irq_timer;

	if (READ_ONCE(detect_data->timers_active))
		return;

	detect_data->irq_data.last_timestamp = now;
	detect_data->softirq_data.last_timestamp = now;
	detect_data->soft_in_irq = 0;
	WRITE_ONCE(detect_data->timers_active, true);

	hrtimer_start_range_ns(irq_timer, ms_to_ktime(READ_ONCE(freq_ms)),
			       0, HRTIMER_MODE_REL_PINNED);

	mod_timer(softirq_timer,
		  jiffies + msecs_to_jiffies(READ_ONCE(freq_ms)));
}

static void percpu_data_init(unsigned int cpu)
{
	struct per_cpu_detect_data *data = per_cpu_ptr(detect_data, cpu);
	int i;

	raw_spin_lock_init(&data->irq_data.lock);
	raw_spin_lock_init(&data->softirq_data.lock);
	for (i = 0; i < MAX_LATENCY_RECORD; i++) {
		atomic_long_set(&data->irq_data.latency_count[i], 0);
		atomic_long_set(&data->softirq_data.latency_count[i], 0);
	}

	timer_setup(&data->softirq_timer, softirq_timer_func,
		    TIMER_PINNED | TIMER_IRQSAFE);

	hrtimer_init(&data->irq_timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_PINNED);
	data->irq_timer.function = irq_hrtimer_func;
}

static void percpu_timers_stop(struct per_cpu_detect_data *data)
{
	WRITE_ONCE(data->timers_active, false);
	del_timer_sync(&data->softirq_timer);
	hrtimer_cancel(&data->irq_timer);
}

static int irqlatency_cpu_online(unsigned int cpu)
{
	struct per_cpu_detect_data *data = per_cpu_ptr(detect_data, cpu);

	if (irqlatency_running(READ_ONCE(detector_state)))
		percpu_timers_start(data);

	return 0;
}

static int irqlatency_cpu_offline(unsigned int cpu)
{
	struct per_cpu_detect_data *data = per_cpu_ptr(detect_data, cpu);

	percpu_timers_stop(data);

	return 0;
}

static void latency_timers_start(void)
{
	int cpu;

	for_each_online_cpu(cpu)
		smp_call_function_single(cpu, percpu_timers_start,
				per_cpu_ptr(detect_data, cpu), true);
}

static void latency_timers_stop(void)
{
	int cpu;

	for_each_online_cpu(cpu) {
		percpu_timers_stop(per_cpu_ptr(detect_data, cpu));
	}
}

static int irqlatency_set_state(enum irqlatency_state new_state)
{
	enum irqlatency_state old_state;

	lockdep_assert_held(&control_lock);

	old_state = READ_ONCE(detector_state);
	if (old_state == IRQLATENCY_EXITING)
		return -ENODEV;
	if (new_state > IRQLATENCY_SOFTIRQ_RUNNING)
		return -EINVAL;
	if (new_state == old_state)
		return 0;

	if (irqlatency_running(old_state) == irqlatency_running(new_state)) {
		WRITE_ONCE(detector_state, new_state);
		return 0;
	}

	cpus_read_lock();
	WRITE_ONCE(detector_state, new_state);
	if (irqlatency_running(new_state))
		latency_timers_start();
	else
		latency_timers_stop();
	cpus_read_unlock();

	return 0;
}

static int enable_show(struct seq_file *m, void *ptr)
{
	seq_printf(m, "%u\n", irqlatency_mode());

	return 0;
}

static int enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, enable_show, inode->i_private);
}

static ssize_t enable_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	unsigned int enable;
	int ret;

	if (kstrtouint_from_user(buf, count, 0, &enable))
		return -EINVAL;

	if (enable > 2)
		return -EINVAL;

	mutex_lock(&control_lock);
	ret = irqlatency_set_state(enable);
	mutex_unlock(&control_lock);

	return ret ? ret : count;
}

static const struct proc_ops enable_fops = {
	.proc_open	= enable_open,
	.proc_read	= seq_read,
	.proc_write	= enable_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int freq_show(struct seq_file *m, void *ptr)
{
	seq_printf(m, "%llu\n", READ_ONCE(freq_ms));

	return 0;
}

static int freq_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_show, inode->i_private);
}

static ssize_t freq_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	unsigned long freq;

	if (kstrtoul_from_user(buf, count, 0, &freq))
		return -EINVAL;

	mutex_lock(&control_lock);
	if (READ_ONCE(detector_state) == IRQLATENCY_EXITING) {
		mutex_unlock(&control_lock);
		return -ENODEV;
	}
	if (irqlatency_running(READ_ONCE(detector_state))) {
		mutex_unlock(&control_lock);
		return -EINVAL;
	}

	if (freq == freq_ms)
		goto unlock;

	if (freq < MIN_FREQ_MS)
		freq = MIN_FREQ_MS;
	else if (freq > MAX_FREQ_MS)
		freq = MAX_FREQ_MS;

	if (freq > (irq_latency_ms >> 1))
		freq = irq_latency_ms >> 1;

	WRITE_ONCE(freq_ms, freq);

unlock:
	mutex_unlock(&control_lock);
	return count;
}

static const struct proc_ops freq_fops = {
	.proc_open	= freq_open,
	.proc_read	= seq_read,
	.proc_write	= freq_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int lat_show(struct seq_file *m, void *ptr)
{
	seq_printf(m, "%llu\n", READ_ONCE(irq_latency_ms));

	return 0;
}

static int lat_open(struct inode *inode, struct file *file)
{
	return single_open(file, lat_show, inode->i_private);
}

static ssize_t lat_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	unsigned long lat_ms;

	if (kstrtoul_from_user(buf, count, 0, &lat_ms))
		return -EINVAL;

	mutex_lock(&control_lock);
	if (READ_ONCE(detector_state) == IRQLATENCY_EXITING) {
		mutex_unlock(&control_lock);
		return -ENODEV;
	}
	if (irqlatency_running(READ_ONCE(detector_state))) {
		mutex_unlock(&control_lock);
		return -EINVAL;
	}

	if (lat_ms == irq_latency_ms)
		goto unlock;

	if (lat_ms < (MIN_FREQ_MS >> 1))
		lat_ms = MIN_FREQ_MS >> 1;
	else if (lat_ms > (MAX_FREQ_MS >> 1))
		lat_ms = MAX_FREQ_MS >> 1;

	if (lat_ms < (freq_ms << 1))
		lat_ms = freq_ms << 1;

	WRITE_ONCE(irq_latency_ms, lat_ms);

unlock:
	mutex_unlock(&control_lock);
	return count;
}

static const struct proc_ops lat_fops = {
	.proc_open	= lat_open,
	.proc_read	= seq_read,
	.proc_write	= lat_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static ssize_t trace_stack_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{
	unsigned long lat;

	if (kstrtoul_from_user(buf, count, 0, &lat))
		return -EINVAL;

	if (!lat) {
		int cpu;

		mutex_lock(&control_lock);
		cpus_read_lock();
		for_each_online_cpu(cpu)
			smp_call_function_single(cpu, reset_latency_trace,
				per_cpu_ptr(detect_data, cpu), true);
		cpus_read_unlock();
		mutex_unlock(&control_lock);
		return count;
	}

	return -EINVAL;
}

static void snapshot_latency_data(struct latency_data *lat_data,
				  struct latency_snapshot *snapshot)
{
	unsigned long flags;
	unsigned long stack_index;
	int i;

	raw_spin_lock_irqsave(&lat_data->lock, flags);
	stack_index = min_t(unsigned long, lat_data->stack_index,
			    MAX_STACK_ENTRIES_INDEX);
	snapshot->stack_index = stack_index;
	memcpy(snapshot->entries, lat_data->entries,
	       sizeof(snapshot->entries));
	for (i = 0; i < stack_index; i++) {
		snapshot->stacks[i].nr_entries =
			lat_data->stacks[i].nr_entries;
		snapshot->stacks[i].first_entry =
			lat_data->stacks[i].perstack - lat_data->entries;
		memcpy(snapshot->comms[i], lat_data->comms[i], TASK_COMM_LEN);
		snapshot->pids[i] = lat_data->pids[i];
		snapshot->msecs[i] = lat_data->latency[i].msecs;
		snapshot->plus[i] = lat_data->latency[i].plus;
	}
	raw_spin_unlock_irqrestore(&lat_data->lock, flags);
}

static void trace_stack_print(struct seq_file *m,
			      const struct latency_snapshot *snapshot,
			      const struct stack_snapshot *stack)
{
	unsigned int i;

	for (i = 0; i < stack->nr_entries; i++)
		seq_printf(m, "%*c%pS\n", 5, ' ',
			   (void *)snapshot->entries[stack->first_entry + i]);
}

static void trace_stack_irq_show(struct seq_file *m, unsigned int isirq,
				 struct latency_snapshot *snapshot)
{
	int cpu;

	for_each_online_cpu(cpu) {
		int i;
		struct latency_data *lat_data;

		lat_data = isirq ? per_cpu_ptr(&detect_data->irq_data, cpu) :
			per_cpu_ptr(&detect_data->softirq_data, cpu);

		snapshot_latency_data(lat_data, snapshot);
		if (!snapshot->stack_index)
			continue;

		seq_printf(m, " cpu: %d\n", cpu);

		for (i = 0; i < snapshot->stack_index; i++) {
			seq_printf(m, "%*cCOMMAND: %s PID: %d LATENCY: %llu%s\n",
				5, ' ', snapshot->comms[i], snapshot->pids[i],
				snapshot->msecs[i],
				snapshot->plus[i] ? "+ms" : "ms");
			trace_stack_print(m, snapshot, snapshot->stacks + i);
			seq_putc(m, '\n');

			cond_resched();
		}
	}
}

static int trace_stack_show(struct seq_file *m, void *v)
{
	struct latency_snapshot *snapshot;

	snapshot = kzalloc(sizeof(*snapshot), GFP_KERNEL);
	if (!snapshot)
		return -ENOMEM;

	seq_printf(m, "irq_latency_ms: %llu\n\n", READ_ONCE(irq_latency_ms));

	seq_puts(m, " irq:\n");
	trace_stack_irq_show(m, true, snapshot);

	seq_putc(m, '\n');

	seq_puts(m, " softirq:\n");
	trace_stack_irq_show(m, false, snapshot);

	kfree(snapshot);
	return 0;
}

static int trace_stack_open(struct inode *inode, struct file *file)
{
	return single_open(file, trace_stack_show, inode->i_private);
}

static const struct proc_ops trace_stack_fops = {
	.proc_open	= trace_stack_open,
	.proc_read	= seq_read,
	.proc_write	= trace_stack_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

#define NUMBER_CHARACTER	40

static bool trace_histogram_show(struct seq_file *m, const char *header,
			   const unsigned long *hist, unsigned long size,
			   unsigned int factor)
{
	int i, zero_index = 0;
	unsigned long count_max = 0;

	for (i = 0; i < size; i++) {
		unsigned long count = hist[i];

		if (count > count_max)
			count_max = count;

		if (count)
			zero_index = i + 1;
	}
	if (count_max == 0)
		return false;

	/* print header */
	if (header)
		seq_printf(m, "%s\n", header);
	seq_printf(m, "%*c%s%*c : %-9s %s\n", 9, ' ', "msecs", 10, ' ', "count",
		   "latency distribution");

	for (i = 0; i < zero_index; i++) {
		int num;
		int scale_min, scale_max;
		char str[NUMBER_CHARACTER + 1];

		scale_max = 2 << i;
		scale_min = unlikely(i == 0) ? 1 : scale_max / 2;

		num = hist[i] * NUMBER_CHARACTER / count_max;
		memset(str, '*', num);
		memset(str + num, ' ', NUMBER_CHARACTER - num);
		str[NUMBER_CHARACTER] = '\0';

		seq_printf(m, "%10d -> %-10d : %-8lu |%s|\n",
			   scale_min * factor, scale_max * factor - 1,
			   hist[i], str);
	}

	return true;
}

static void trace_dist_show_irq(struct seq_file *m, void *v, unsigned int isirq)
{
	int cpu;
	unsigned long latency_count[MAX_LATENCY_RECORD] = { 0 };

	for_each_online_cpu(cpu) {
		int i;
		atomic_long_t *count;

		count = isirq ?
			per_cpu_ptr(detect_data->irq_data.latency_count, cpu) :
			per_cpu_ptr(detect_data->softirq_data.latency_count,
				    cpu);

		for (i = 0; i < MAX_LATENCY_RECORD; i++)
			latency_count[i] += atomic_long_read(&count[i]);
	}

	trace_histogram_show(m, isirq ? "irq-disable:" : "softirq-disable:",
		       latency_count, MAX_LATENCY_RECORD, READ_ONCE(freq_ms));
}

static int trace_dist_show(struct seq_file *m, void *v)
{
	trace_dist_show_irq(m, v, 1);
	trace_dist_show_irq(m, v, 0);

	return 0;
}

static int trace_dist_open(struct inode *inode, struct file *file)
{
	return single_open(file, trace_dist_show, inode->i_private);
}

static const struct proc_ops trace_dist_fops = {
	.proc_open	= trace_dist_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int irqlatency_proc_create(void)
{
	latency_dir = proc_mkdir("irq_latency", NULL);
	if (!latency_dir)
		return -ENOMEM;

	if (!proc_create("enable", 0600, latency_dir, &enable_fops) ||
	    !proc_create("freq_ms", 0600, latency_dir, &freq_fops) ||
	    !proc_create("latency_thresh_ms", 0600, latency_dir, &lat_fops) ||
	    !proc_create("trace_stack", 0600, latency_dir,
			 &trace_stack_fops) ||
	    !proc_create("trace_dist", 0400, latency_dir, &trace_dist_fops)) {
		proc_remove(latency_dir);
		latency_dir = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int __init trace_latency_init(void)
{
	int cpu, ret;

	detect_data = alloc_percpu(struct per_cpu_detect_data);
	if (!detect_data)
		return -ENOMEM;
	for_each_possible_cpu(cpu)
		percpu_data_init(cpu);

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
				"tkernel/irqlatency:online",
				irqlatency_cpu_online,
				irqlatency_cpu_offline);
	if (ret < 0)
		goto free_data;
	irqlatency_hp_state = ret;

	ret = irqlatency_proc_create();
	if (ret)
		goto remove_hp_state;

	pr_info("Load irq latency check module!\n");
	return 0;

remove_hp_state:
	WRITE_ONCE(detector_state, IRQLATENCY_EXITING);
	cpuhp_remove_state(irqlatency_hp_state);
free_data:
	free_percpu(detect_data);

	return ret;
}

static void __exit trace_latency_exit(void)
{
	mutex_lock(&control_lock);
	WRITE_ONCE(detector_state, IRQLATENCY_EXITING);
	mutex_unlock(&control_lock);

	proc_remove(latency_dir);
	latency_dir = NULL;
	cpuhp_remove_state(irqlatency_hp_state);
	free_percpu(detect_data);
	pr_info("Unload irq latency check module!\n");
}

module_init(trace_latency_init);
module_exit(trace_latency_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("shookliu <shookliu@tencent.com>");
MODULE_DESCRIPTION("TKernel IRQ and softirq latency detector");
