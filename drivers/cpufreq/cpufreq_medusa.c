/*
 * medusa cpufreq governor
 *
 * Copyright 2013-2014 NICTA
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE, GOOD TITLE or NON INFRINGEMENT. See the GNU General Public
 * License for more details.
 *
 * Author: Aaron Carroll <Aaron.Carroll@nicta.com.au>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include "../../kernel/sched/sched.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>

#define FTHRESH_DEFAULT_KHZ 0UL

static void medusa_tick(struct work_struct *);
static void medusa_update(struct work_struct *);
#if defined(MEDUSA_PMU)
static void medusa_pmu_start_local(int);
#endif

#define MEDUSA_CPUS 4
static const long supported_freqs[] = {
	384000, 486000, 594000, 702000, 810000, 918000,
	1026000, 1134000, 1242000, 1350000, 1458000,
	1566000, 1674000, 1782000, 1890000, 1998000,
};

static int freq_to_idx(int freq)
{
	switch (freq) {
	case 384000: return 0;
	case 486000: return 1;
	case 594000: return 2;
	case 702000: return 3;
	case 810000: return 4;
	case 918000: return 5;
	case 1026000: return 6;
	case 1134000: return 7;
	case 1242000: return 8;
	case 1350000: return 9;
	case 1458000: return 10;
	case 1566000: return 11;
	case 1674000: return 12;
	case 1782000: return 13;
	case 1890000: return 14;
	case 1998000: return 15;
	default: BUG();
	}
}

#define NFREQS (sizeof(supported_freqs)/sizeof(supported_freqs[0]))


#if defined(MEDUSA_PMU)
static struct perf_event_attr perf_attrs[] = {
	{
		.type   = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_CPU_CYCLES,
		.size   = sizeof(struct perf_event_attr),
		.pinned = 1,
	},
	{
		.type   = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_INSTRUCTIONS,
		.size   = sizeof(struct perf_event_attr),
		.pinned = 1,
	},
	{
		.type   = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
		.size   = sizeof(struct perf_event_attr),
		.pinned = 1,
	},
	{
		.type   = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
		.size   = sizeof(struct perf_event_attr),
		.pinned = 1,
	},
};
#define NUM_PERF_EVENTS (sizeof(perf_attrs)/sizeof(perf_attrs[0]))
#endif


struct medusa_cpu_data {
	unsigned int cur_freq;
	unsigned int set_freq;
	struct cpufreq_policy *policy;

	unsigned long active_time;
	unsigned long idle_time;

	u64 start;
	bool is_idle;

	/* number of update cycles this cpu was online */
	u64 ticks_online;
};

enum medusa_operations {
	MEDUSA_OP_NONE = 0,
	MEDUSA_OP_OFFLINE,
	MEDUSA_OP_ONLINE,
	MEDUSA_OP_FUP,
	MEDUSA_OP_FDOWN,
	MEDUSA_OP_FSTEP,
	MEDUSA_OP_FJUMP,
	MEDUSA_OP_FMAX,
};

struct medusa_global_stats {
	/*
	 * policy stuff
	 */
	unsigned long total_running;
	unsigned long max_running;

	unsigned long cur_load;
	unsigned long avg_load;

	unsigned long min_busy[3]; // last n min busy, idx 0 = most recent
	unsigned long max_busy[3]; // last n max busy, idx 0 = most recent
	unsigned long min_busy_avg;
	unsigned long max_busy_avg;
	unsigned long avg_busy;

	unsigned long ticks_at_op;

	unsigned last_operation;

	/*
	 * book keeping
	 */
	int medusa_users;
	unsigned long tick;

#if defined(MEDUSA_DEBUG)
	unsigned long frame_count;
#endif
};

static DEFINE_PER_CPU(struct medusa_cpu_data, cpu_data);
struct medusa_global_stats mstats;

static cpumask_t cpus_managed         = { CPU_BITS_NONE };
static cpumask_t cpus_online          = { CPU_BITS_NONE };
static cpumask_t cpus_online_managed  = { CPU_BITS_NONE };

/* all times in us */
#define TICK_RATE_DEF   10000UL
#define UPDATE_RATE_DEF 100000UL
#define MIN_UP_LOAD 100
static unsigned long tick_rate_us = TICK_RATE_DEF; // for updating stats
static unsigned long update_rate_us = UPDATE_RATE_DEF; // for policy decision
static unsigned long update_div = UPDATE_RATE_DEF / TICK_RATE_DEF;
static unsigned long fthresh_khz = FTHRESH_DEFAULT_KHZ;
/* only offline a core if its been idle for at least this many update cycles */
static unsigned long offline_delay_updates = 3;

static DECLARE_DELAYED_WORK(tick_work, medusa_tick);
static DECLARE_DELAYED_WORK(update_work, medusa_update);

static DEFINE_MUTEX(medusa_lock);

/* Protect active_time, idle_time, start, is_idle in cpu_data. They get accessed
 * in idle context (from the idle notifier), which may not block, so we can't use
 * the main mutex */
static DEFINE_SPINLOCK(idle_lock);


#if defined(MEDUSA_DEBUG)
static struct dentry *medusa_debug_root;
#endif


/*
 * logging ----------------------------------
 */

#if defined(MEDUSA_LOG) && defined(MEDUSA_DEBUG)
static char* e_names[] = { "none", "offline", "online", "fup", "fdown", "fstep", "fjump", "fmax" };

struct medusa_log_entry {
	unsigned long tick;
	unsigned event : 3;
	unsigned cpu : 3;
	unsigned long val0, val1, val2;
};

#define MEDUSA_LOG_SIZE 1500
static struct medusa_log_entry medusa_log[MEDUSA_LOG_SIZE];
static unsigned long medusa_log_head = 0;

static inline void medusa_log_event(unsigned event, unsigned cpu,
		unsigned long val0, unsigned long val1, unsigned long val2)
{
	medusa_log[medusa_log_head].tick = mstats.tick;
	medusa_log[medusa_log_head].event = event;
	medusa_log[medusa_log_head].cpu = cpu;
	medusa_log[medusa_log_head].val0 = val0;
	medusa_log[medusa_log_head].val1 = val1;
	medusa_log[medusa_log_head].val2 = val2;

	if (++medusa_log_head >= MEDUSA_LOG_SIZE) {
		medusa_log_head = 0;
	}
}

static struct dentry *medusa_log_dentry;

static int log_show(struct seq_file *s, void *unused)
{
	unsigned long i;

	for (i = 0; i < medusa_log_head; i++) {
		struct medusa_log_entry *e = &medusa_log[i];
		seq_printf(s, "%6lu: c%u %-7s (%lu, %lu, %lu)\n", e->tick, e->cpu,
			e_names[e->event], e->val0, e->val1, e->val2);
	}

	return 0;
}

ssize_t log_write(struct file *file, const char __user *ptr, size_t len, loff_t *off)
{
	medusa_log_head = 0;
	return len;
}

static int log_open(struct inode *inode, struct file *file)
{
	return single_open(file, log_show, NULL);
}

static struct file_operations log_fops = {
	.open =   log_open,
	.read =   seq_read,
	.write =  log_write,
	.llseek = seq_lseek,
};

#else
static inline void medusa_log_event(unsigned event, unsigned cpu,
		unsigned long val0, unsigned long val1, unsigned long val2) {}
#endif

/*
 * -------------oracle--------------------------
 */

#if defined(MEDUSA_ORACLE) && defined(MEDUSA_DEBUG)

#define ORACLE_TRACE_SIZE 2048
static struct dentry *trace_dentry;

struct trace_entry {
	unsigned long tick;
	unsigned load;
	int8_t busy[MEDUSA_CPUS];
};

static struct trace_entry oracle_trace[ORACLE_TRACE_SIZE];
static unsigned long trace_head = 0;
static bool trace_is_running = false;

static inline void trace_start_logging(void)
{
	trace_is_running = true;
}

static inline void trace_stop_logging(void)
{
	trace_is_running = false;
}

static inline void trace_event(unsigned load, int8_t busy[])
{
	if (!trace_is_running) {
		return;
	}

	load = clamp(load, 0U, 9999U);
	oracle_trace[trace_head].tick = mstats.tick;
	oracle_trace[trace_head].load = load;
	memcpy(oracle_trace[trace_head].busy, busy, MEDUSA_CPUS);
	if (++trace_head >= ORACLE_TRACE_SIZE) {
		trace_head = 0;
	}
}

static int trace_show(struct seq_file *s, void *unused)
{
	unsigned long i;

	for (i = 0; i < trace_head; i++) {
		struct trace_entry *e = &oracle_trace[i];
		seq_printf(s, "%8lu: %4u %3i %3i %3i %3i\n", e->tick, e->load,
				e->busy[0], e->busy[1], e->busy[2], e->busy[3]);
	}

	return 0;
}

ssize_t trace_write(struct file *file, const char __user *ptr, size_t len, loff_t *off)
{
	trace_head = 0;
	return len;
}

static int trace_open(struct inode *inode, struct file *file)
{
	return single_open(file, trace_show, NULL);
}

static struct file_operations trace_fops = {
	.open =   trace_open,
	.read =   seq_read,
	.write =  trace_write,
	.llseek = seq_lseek,
};

#else
static inline void trace_event(unsigned load, int8_t busy[]) {}
#endif


/*
 * ---------------------------------------------
 */

// time in us
static u64 medusa_time(void)
{
	struct timespec now;
	getrawmonotonic(&now);
	return (u64)now.tv_sec * USEC_PER_SEC + (u64)(now.tv_nsec / NSEC_PER_USEC);
}

static int idle_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
	unsigned int cpu = smp_processor_id();
	struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);
	u64 now = medusa_time();

	spin_lock(&idle_lock);

	if (mcd->start > now) {
		/* HTF does this happen? */
		WARN_ON(1);
		mcd->start = now;
	}

	if (val == IDLE_START) {
		mcd->active_time += (now - mcd->start);
		mcd->is_idle = true;
	} else { /* IDLE_END */
		mcd->idle_time += (now - mcd->start);
		mcd->is_idle = false;
	}

	mcd->start = now;

	spin_unlock(&idle_lock);

	return 0;
}

static int cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
	struct medusa_cpu_data *mcd;
	struct cpufreq_freqs *freq = data;

	if (val == CPUFREQ_POSTCHANGE) {
		mcd = &per_cpu(cpu_data, freq->cpu);
		mcd->cur_freq = freq->new;
	}

	return 0;
}

static int cpu_state_notifier(struct notifier_block *nb, unsigned long state, void *data)
{
	unsigned int cpu = (unsigned int)data;

	/* this is racy... */

	if (state == CPU_ONLINE) {
		cpumask_set_cpu(cpu, &cpus_online);
		cpumask_and(&cpus_online_managed, &cpus_online, &cpus_managed);

#if defined(MEDUSA_PMU)
		// appears to be an implicit stop when core goes offline
		medusa_pmu_start_local(cpu);
#endif

	} else if (state == CPU_DOWN_PREPARE) {
		cpumask_clear_cpu(cpu, &cpus_online);
		cpumask_and(&cpus_online_managed, &cpus_online, &cpus_managed);
	}

	return 0;
}

static struct notifier_block cpufreq_notifier_block = { .notifier_call = cpufreq_notifier, };
static struct notifier_block idle_nb = { .notifier_call = idle_notifier, };
static struct notifier_block cpu_nb = { .notifier_call = cpu_state_notifier, };


/* for `scaling_setspeed': show the set (not actual) freq */
static ssize_t cpufreq_show(struct cpufreq_policy *policy, char *buf)
{
	ssize_t len;
	unsigned int cpu = policy->cpu;
	struct medusa_cpu_data *mcd;
	mcd = &per_cpu(cpu_data, cpu);

	mutex_lock(&medusa_lock);
	len = sprintf(buf, "%u\n", mcd->set_freq);
	mutex_unlock(&medusa_lock);

	return len;
}

static int cpufreq_set(struct cpufreq_policy *policy, unsigned int freq)
{
	int ret;
	unsigned int cpu = policy->cpu;
	struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);

	mutex_lock(&medusa_lock);
	mcd->set_freq = freq;
	ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
	mutex_unlock(&medusa_lock);

	return ret;
}

/*
 * ------------- sysfs ------------------
 */

static ssize_t show_managed_mask(struct kobject *kobj, struct attribute *attr, char *buf)
{
	ssize_t len;

	mutex_lock(&medusa_lock);
	len = cpulist_scnprintf(buf, PAGE_SIZE-1, &cpus_managed);
	mutex_unlock(&medusa_lock);
	buf[len] = '\n';
	buf[len+1] = 0;
	return len + 1;
}
static ssize_t store_managed_mask(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct cpumask dest = { CPU_BITS_NONE };

	if (strcmp(buf, "") && strcmp(buf, "\n")) {
		ret = cpulist_parse(buf, &dest);
		if (ret)
			return ret;
	}

	mutex_lock(&medusa_lock);
	cpus_managed = dest;
	cpumask_and(&cpus_online_managed, &cpus_online, &cpus_managed);
	mutex_unlock(&medusa_lock);

	return count;
}
define_one_global_rw(managed_mask);

static ssize_t show_online_mask(struct kobject *kobj, struct attribute *attr, char *buf)
{
	ssize_t len;

	mutex_lock(&medusa_lock);
	len = cpulist_scnprintf(buf, PAGE_SIZE-1, &cpus_online);
	mutex_unlock(&medusa_lock);
	buf[len] = '\n';
	buf[len+1] = 0;
	return len + 1;
}
static ssize_t store_online_mask(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	struct cpumask dest;
	unsigned int cpu;

	cpulist_parse(buf, &dest);
	cpumask_set_cpu(0, &dest); // CPU0 is always online

	for_each_cpu(cpu, &dest) {
		if (!cpu_online(cpu)) {
			cpu_up(cpu);
		}
	}

	for_each_cpu_not(cpu, &dest) {
		if (cpu_online(cpu)) {
			cpu_down(cpu);
		}
	}

	return count;
}
define_one_global_rw(online_mask);

static ssize_t show_update_ratio(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", update_div);
}
define_one_global_ro(update_ratio);

#if defined(MEDUSA_PMU)
static ssize_t show_events(struct kobject *kobj, struct attribute *attr, char *buf)
{
	u64 data, enabled, running;
	int cpu, i;
	char *str = buf;

	for_each_possible_cpu(cpu) {
		struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);

		for (i=0; i<NUM_PERF_EVENTS; i++) {
			if (mcd->perf_events[i])
				data = perf_event_read_value(mcd->perf_events[i], &enabled, &running);
			else
				data = 0;
			str += snprintf(str, PAGE_SIZE, "%llu ", data - mcd->events_start[i]);
		}
		str += snprintf(str, PAGE_SIZE, "\n");
	}

	return str - buf;
}
static ssize_t store_events(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	u64 data, enabled, running;
	int cpu, i;

	for_each_possible_cpu(cpu) {
		struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);
		for (i=0; i<NUM_PERF_EVENTS; i++) {
			if (mcd->perf_events[i])
				data = perf_event_read_value(mcd->perf_events[i], &enabled, &running);
			else
				data = 0;
			mcd->events_start[i] = data;
		}
	}
	return count;
}
define_one_global_rw(events);
#endif

static ssize_t show_tick_rate(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu us\n", tick_rate_us);
}
static ssize_t store_tick_rate(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long new_rate;

	ret = sscanf(buf, "%lu", &new_rate);
	if (ret != 1)
		return -EINVAL;

	tick_rate_us = new_rate;
	update_div = update_rate_us / tick_rate_us;

	return count;
}
define_one_global_rw(tick_rate);

static ssize_t show_update_rate(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu us\n", update_rate_us);
}
static ssize_t store_update_rate(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long new_rate;

	ret = sscanf(buf, "%lu", &new_rate);
	if (ret != 1)
		return -EINVAL;

	update_rate_us = new_rate;
	update_div = update_rate_us / tick_rate_us;

	return count;
}
define_one_global_rw(update_rate);

static ssize_t show_offline_delay(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", offline_delay_updates);
}
static ssize_t store_offline_delay(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long new_delay;

	ret = sscanf(buf, "%lu", &new_delay);
	if (ret != 1)
		return -EINVAL;

	offline_delay_updates = new_delay;
	return count;
}
define_one_global_rw(offline_delay);

static ssize_t show_stats(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int cpu;
	char *str = buf;
	for_each_possible_cpu(cpu) {
		str += snprintf(str, PAGE_SIZE-(str-buf), "%i %llu\n", cpu,
				per_cpu(cpu_data, cpu).ticks_online);
	}
	return str - buf;
}
static ssize_t store_stats(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		per_cpu(cpu_data, cpu).ticks_online = 0;
	}
	return count;
}
define_one_global_rw(stats);

static int test_cpu;
static int offline_iters;
static int freq_iters;
static ssize_t show_offlinetest(struct kobject *kobj, struct attribute *attr, char *buf)
{
	char *b = buf;
	u64 start, end;
	int i;
	struct cpufreq_policy *p = cpufreq_cpu_get(test_cpu);

	start = medusa_time();
	for (i = 0; i < offline_iters; i++) {
		cpu_down(test_cpu);
		cpu_up(test_cpu);
	}
	end = medusa_time();
	b += snprintf(b, PAGE_SIZE, "offline, %llu us, %i iters\n", end-start, offline_iters);

	/* highest to 2nd highest */
	start = medusa_time();
	for (i = 0; i < freq_iters*9; i++) {
		cpufreq_driver_target(p, p->max, CPUFREQ_RELATION_L);
		cpufreq_driver_target(p, p->max - 1, CPUFREQ_RELATION_H);

	}
	end = medusa_time();
	b += snprintf(b, PAGE_SIZE, "h-h, %llu us, %i iters\n", end-start, 9*freq_iters);

	/* lowest to 2nd lowest */
	start = medusa_time();
	for (i = 0; i < freq_iters*25; i++) {
		cpufreq_driver_target(p, p->min, CPUFREQ_RELATION_H);
		cpufreq_driver_target(p, p->min + 1, CPUFREQ_RELATION_L);
	}
	end = medusa_time();
	b += snprintf(b, PAGE_SIZE, "l-l, %llu us, %i iters\n", end-start, 25*freq_iters);

	/* lowest to highest */
	start = medusa_time();
	for (i = 0; i < 5*freq_iters; i++) {
		cpufreq_driver_target(p, p->min, CPUFREQ_RELATION_H);
		cpufreq_driver_target(p, p->max, CPUFREQ_RELATION_L);
	}
	end = medusa_time();
	b += snprintf(b, PAGE_SIZE, "l-h, %llu us, %i iters\n", end-start, 5*freq_iters);

	cpufreq_cpu_put(p);

	return b - buf;
}
static ssize_t store_offlinetest(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int cpu, i1, i2;

	ret = sscanf(buf, "%i %i %i", &cpu, &i1, &i2);
	if (ret != 3)
		return -EINVAL;

	test_cpu = cpu;
	offline_iters = i1;
	freq_iters = i2;

	return count;
}
define_one_global_rw(offlinetest);

static ssize_t show_fthresh(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu kHz\n", fthresh_khz);
}
static ssize_t store_fthresh(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long new_fthresh;

	ret = sscanf(buf, "%lu", &new_fthresh);
	if (ret != 1)
		return -EINVAL;

	fthresh_khz = new_fthresh;

	return count;
}
define_one_global_rw(fthresh);

#if defined(MEDUSA_ORACLE) && defined(MEDUSA_DEBUG)
static ssize_t show_trace_running(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%i\n", trace_is_running);
}
static ssize_t store_trace_running(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%i", &val);
	if (ret != 1)
		return -EINVAL;

	if (val == 0)
		trace_stop_logging();
	else
		trace_start_logging();

	return count;
}
define_one_global_rw(trace_running);
#endif

#if defined(MEDUSA_DEBUG)
static ssize_t show_frame_counter(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", mstats.frame_count);
}
static ssize_t store_frame_counter(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = sscanf(buf, "%lu", &val);
	if (ret != 1)
		return -EINVAL;

	if (val == 0)
		mstats.frame_count = 0;
	else
		mstats.frame_count += val;

	return count;
}
static struct global_attr frame_counter = __ATTR(frame_counter, 0666, show_frame_counter, store_frame_counter);
#endif

static struct attribute *medusa_attributes[] = {
	&online_mask.attr,
	&managed_mask.attr,
	&tick_rate.attr,
	&update_rate.attr,
	&update_ratio.attr,
	&offline_delay.attr,
	&stats.attr,
	&fthresh.attr,
	&offlinetest.attr,
#if defined(MEDUSA_PMU)
	&events.attr,
#endif
#if defined(MEDUSA_DEBUG)
	&frame_counter.attr,
#if defined(MEDUSA_ORACLE)
	&trace_running.attr,
#endif
#endif
	NULL
};

static struct attribute_group medusa_attr_group = {
	.attrs = medusa_attributes,
	.name = "medusa",
};

/*
 * --------------- end sysfs ----------------------
 */

static void medusa_tick(struct work_struct *work)
{
	unsigned long running;
	mutex_lock(&medusa_lock);

	/* dont include this worker thread */
	running = nr_running() - 1;
	mstats.total_running += running;

	schedule_delayed_work(&tick_work, usecs_to_jiffies(tick_rate_us));
	mutex_unlock(&medusa_lock);
}

/*
 * Find the highest CPU id enabled in mask
 */
static int cpumask_last(const struct cpumask *mask)
{
	int cpu, last = 0;
	for_each_cpu(cpu, mask) {
		last = cpu;
	}
	return last;
}

static inline int next_offline_managed(void)
{
	int cpu = -1;
	do {
		cpu = cpumask_next_zero(cpu, &cpus_online);
	} while (!cpumask_test_cpu(cpu, &cpus_managed));

	BUG_ON(cpu == 0); // cpu0 should never be offline

	return cpu;
}

static void medusa_cpu_up(unsigned int cpu)
{
	mutex_unlock(&medusa_lock);
	cpu_up(cpu);
	mutex_lock(&medusa_lock);
	mstats.last_operation = MEDUSA_OP_ONLINE;
}

static void medusa_cpu_down(unsigned int cpu)
{
	mutex_unlock(&medusa_lock);
	cpu_down(cpu);
	mutex_lock(&medusa_lock);
	mstats.last_operation = MEDUSA_OP_OFFLINE;
}

static inline unsigned long medusa_cur_freq(void)
{
	struct medusa_cpu_data *mcd0 = &per_cpu(cpu_data, 0);
	return mcd0->cur_freq;
}

static void medusa_update_stats(void)
{
	int cpu;
	unsigned long max_busy = 0;
	unsigned long min_busy = 100;
	unsigned long avg_busy = 0;
	int8_t busy_list[4] = { -1, -1, -1, -1 };

	/* calculate load average since the last update */
	mstats.cur_load = (100 * mstats.total_running) / update_div;
	mstats.avg_load = mstats.cur_load;
	mstats.ticks_at_op++;

	/*
	 * calculate busy values
	 */
	for_each_cpu(cpu, &cpus_online) {
		struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);
		unsigned long busy, total;
		u64 now = medusa_time();
		mcd->ticks_online++;

		spin_lock(&idle_lock);

		if (mcd->start > now) {
			/* HTF does this happen? */
			WARN_ON(1);
			mcd->start = now;
		}

		if (mcd->is_idle) {
			mcd->idle_time += (now - mcd->start);
		} else {
			mcd->active_time += (now - mcd->start);
		}

		total = mcd->active_time + mcd->idle_time;
		if (total == 0) {
			WARN_ON(1);
			busy = 0;
		} else {
			busy = 100 * mcd->active_time / total;
			busy = clamp(busy, 0UL, 100UL);
		}

		mcd->start = now;
		mcd->active_time = mcd->idle_time = 0;
		spin_unlock(&idle_lock);

		min_busy = min(busy, min_busy);
		max_busy = max(busy, max_busy);
		avg_busy += busy;
		busy_list[cpu] = busy;
	}

	trace_event(mstats.cur_load, busy_list);

	mstats.min_busy[2] = mstats.min_busy[1];
	mstats.min_busy[1] = mstats.min_busy[0];
	mstats.min_busy[0] = min_busy;

	mstats.max_busy[2] = mstats.max_busy[1];
	mstats.max_busy[1] = mstats.max_busy[0];
	mstats.max_busy[0] = max_busy;

	mstats.avg_busy = avg_busy / cpumask_weight(&cpus_online);

	mstats.min_busy_avg = (mstats.min_busy[0] + mstats.min_busy[1] + mstats.min_busy[2]) / 3;
	mstats.max_busy_avg = (mstats.max_busy[0] + mstats.max_busy[1] + mstats.max_busy[2]) / 3;
}

static inline unsigned medusa_choose_next_freq(unsigned long fcur, unsigned long *next)
{
	unsigned long freq_next = 0;
	unsigned op = MEDUSA_OP_NONE;
	struct medusa_cpu_data *mcd0 = &per_cpu(cpu_data, 0);
	int idx = freq_to_idx(fcur);

	if ((mstats.max_busy[0] >= MIN_UP_LOAD) && (fcur == mcd0->policy->max)) {
		/* at least one core is pegged at fmax */
		op = MEDUSA_OP_FMAX;
		freq_next = 0;
	}

	/* up if any 1 core is fully loaded */
	else if ((mstats.max_busy[0] >= MIN_UP_LOAD) && (fcur < mcd0->policy->max)) {

		if ((mstats.max_busy[1] >= MIN_UP_LOAD) && (mstats.max_busy[2] >= MIN_UP_LOAD)) {
			/* overloaded 3rd time in a row - jump to max */
			freq_next = mcd0->policy->max;
			op = MEDUSA_OP_FJUMP;
		} else if ((mstats.max_busy[1] >= MIN_UP_LOAD)) {
			/* overloaded 2nd time - jump half way to max */
			int idx_max = freq_to_idx(mcd0->policy->max);
			freq_next = supported_freqs[idx + (idx_max - idx) / 2];
			op = MEDUSA_OP_FSTEP;
		} else if ((mstats.max_busy[1] < 10) && (mstats.max_busy[2] < 10)) {
			/* react quickly to sudden increases in load */
			freq_next = mcd0->policy->max;
			op = MEDUSA_OP_FJUMP;
		} else {
			freq_next = supported_freqs[idx + 1];
			op = MEDUSA_OP_FUP;
		}
	}

	/* down if all cores are under-loaded */
	else if ((fcur > mcd0->policy->min) &&
			((mstats.ticks_at_op >= offline_delay_updates) || (mstats.last_operation == MEDUSA_OP_FJUMP)) &&
			(mstats.max_busy_avg < MIN_UP_LOAD)) {
		int prev;
		prev = supported_freqs[idx - 1];

		// offset to add a bit of hysteresis
		freq_next = (mstats.max_busy_avg + 5) * fcur / 100;
		if (freq_next <= prev) {
			op = MEDUSA_OP_FDOWN;
		}
	}

	*next = freq_next;
	return op;
}

/*
 * currently running nonline cores at cur_freq
 * predict what equivalent freq would be if we offlined a core
 */
static inline unsigned long
predict_offline_freq(unsigned long cur_freq, unsigned long nonline)
{
	unsigned long predicted_freq;
	unsigned long busy;

	busy = min(100UL, mstats.avg_busy + 5);

	predicted_freq = (busy * cur_freq * nonline) / (100 * (nonline - 1));
	return predicted_freq;
}

static inline void medusa_set_freq(unsigned long fop, unsigned long freq)
{
	int cpu, ret;

	BUG_ON(freq == 0);

	if (fop == MEDUSA_OP_NONE) {
		return;
	}

	/* XXX: only need to do this for the root cpu in each cluster */
	for_each_cpu(cpu, &cpus_online_managed) {
		struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);
		ret = __cpufreq_driver_target(mcd->policy, freq, CPUFREQ_RELATION_L);
		BUG_ON(ret);
	}

	mstats.last_operation = fop;
	medusa_log_event(fop, 0, mstats.max_busy[0], mstats.max_busy_avg, freq);
}

static void medusa_update(struct work_struct *work)
{
	unsigned long nonline, nmanaged;
	unsigned long next_freq, cur_freq = medusa_cur_freq();
	int cpu, fop;
	bool op_changed = false;
	bool can_online, can_offline;

	mutex_lock(&medusa_lock);
	medusa_update_stats();

	nonline = cpumask_weight(&cpus_online_managed);
	nmanaged = cpumask_weight(&cpus_managed);

	if (nmanaged == 0) {
		goto out_inactive;
	}

	fop = medusa_choose_next_freq(cur_freq, &next_freq);
	can_online = (nonline < nmanaged) && (mstats.cur_load > 105*nonline);
	can_offline = (nonline > 1) && (mstats.ticks_at_op >= offline_delay_updates);

	if ((fop == MEDUSA_OP_FJUMP) || (fop == MEDUSA_OP_FSTEP)) {
		/* set freq - no screwing around */
		medusa_set_freq(fop, next_freq);
		op_changed = true;
	}

	else if (fop == MEDUSA_OP_FMAX) {
		/* we are overloaded but already at max freq */
		if (can_online) {
			do {
				cpu = next_offline_managed();
				medusa_log_event(MEDUSA_OP_ONLINE, cpu, mstats.cur_load, 0, nonline);
				medusa_cpu_up(cpu);
				nonline++;
			} while ((nonline < nmanaged) && (mstats.cur_load > 105*nonline));
			op_changed = true;
		} else {
			/* either we are at highest OP, or not enough threads to run more
			 * cores. nothing we can do */
		}
	}

	else if (((fop != MEDUSA_OP_NONE) && (next_freq > fthresh_khz)) ||
			 ((fop == MEDUSA_OP_NONE) && (cur_freq > fthresh_khz))) {

		/* we would be running at >fthresh - can we online? */
		if (can_online) {
			do {
				cpu = next_offline_managed();
				medusa_log_event(MEDUSA_OP_ONLINE, cpu, mstats.cur_load, 0, nonline);
				medusa_cpu_up(cpu);
				nonline++;
			} while ((nonline < nmanaged) && (mstats.cur_load > 105*nonline));
			op_changed = true;

		} else if (fop != MEDUSA_OP_NONE) {
			// run at next
			medusa_set_freq(fop, next_freq);
			op_changed = true;
		}
	}

	else if (((fop != MEDUSA_OP_NONE) && (next_freq <= fthresh_khz)) ||
			 ((fop == MEDUSA_OP_NONE) && (cur_freq <= fthresh_khz))) {

		/* next point would be running <= fthresh
		 * decide if offlining a core would put us above fthresh
		 * if so, run at next/cur
		 * if not, turn off a core instead
		 */

		BUG_ON(fthresh_khz == 0);

		if (can_offline) {
			unsigned long predicted_freq = predict_offline_freq(cur_freq, nonline);

			if ((predicted_freq > fthresh_khz) || (predicted_freq > supported_freqs[NFREQS-2])) {
				// run at next/cur
				if (fop != MEDUSA_OP_NONE) {
					medusa_set_freq(fop, next_freq);
					op_changed = true;
				}
			} else {
				// turn off a core and switch to predicted freq
				cpu = cpumask_last(&cpus_online_managed);
				WARN_ON(cpu == 0);
				medusa_log_event(MEDUSA_OP_OFFLINE, cpu, mstats.min_busy_avg, mstats.max_busy_avg, mstats.avg_busy);
				medusa_cpu_down(cpu);
				medusa_set_freq(MEDUSA_OP_FUP, predicted_freq);
				nonline--;
				op_changed = true;
			}
		}
		else {
			/* dont have anything to turn off, run at next/cur */

			if (fop != MEDUSA_OP_NONE) {
				medusa_set_freq(fop, next_freq);
				op_changed = true;
			}
		}
	}

	/* if we have idle cores, turn them off */
	if (!op_changed && can_offline && (mstats.cur_load <= 105*nonline)) {

		if (mstats.max_busy_avg < 5) {
			/* all at once */
			while (nonline > 1) {
				cpu = cpumask_last(&cpus_online_managed);
				WARN_ON(cpu == 0);
				medusa_log_event(MEDUSA_OP_OFFLINE, cpu, mstats.min_busy_avg, mstats.max_busy_avg, mstats.avg_load);
				medusa_cpu_down(cpu);
				nonline--;
			}
			op_changed = true;
		}
		else if (mstats.min_busy_avg < 5) {
			// one at a time
			cpu = cpumask_last(&cpus_online_managed);
			WARN_ON(cpu == 0);
			medusa_log_event(MEDUSA_OP_OFFLINE, cpu, mstats.min_busy_avg, mstats.max_busy_avg, mstats.avg_load);
			medusa_cpu_down(cpu);
			nonline--;
			op_changed = true;
		}
	}

	if (op_changed) {
		mstats.ticks_at_op = 0;
	}

 out_inactive:
	mstats.total_running = 0;
	mstats.tick++;

	schedule_delayed_work(&update_work, usecs_to_jiffies(update_rate_us));
	mutex_unlock(&medusa_lock);
}

#if defined(MEDUSA_PMU)
static void medusa_pmu_start_local(int cpu)
{
	struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);
	int i;

	for (i=0; i<NUM_PERF_EVENTS; i++) {
		mcd->perf_events[i] = perf_event_create_kernel_counter(&perf_attrs[i], cpu, NULL, NULL, NULL);

		if (IS_ERR(mcd->perf_events[i])) {
			printk("%s: perf_create failed (err=%li,cpu=%i,event=%i)\n",
					__func__, PTR_ERR(mcd->perf_events[i]), cpu, i);
			mcd->perf_events[i] = NULL;
		} else {
			u64 enabled, running;
			mcd->events_start[i] = perf_event_read_value(mcd->perf_events[i], &enabled, &running);
		}
	}
}
#endif

#if defined(MEDUSA_PMU)
static int medusa_pmu_start(void)
{
	int cpu;

	for_each_online_cpu(cpu) {
		medusa_pmu_start_local(cpu);
		// the others start lazily when the cores come up
	}

	return 0;
}

static int medusa_pmu_stop(void)
{
	int cpu, i;

	for_each_online_cpu(cpu) {
		struct medusa_cpu_data *mcd = &per_cpu(cpu_data, cpu);

		for (i=0; i<NUM_PERF_EVENTS; i++) {
			if (mcd->perf_events[i]) {
				perf_event_release_kernel(mcd->perf_events[i]);
				mcd->perf_events[i] = NULL;
			}
		}
	}

	return 0;
}
#else
static int medusa_pmu_start(void) { return 0; }
static int medusa_pmu_stop(void) { return 0; }
#endif

static int medusa_start(void)
{
	int ret;

	cpumask_clear(&cpus_online);
	cpumask_clear(&cpus_managed);
	cpumask_clear(&cpus_online_managed);

	cpus_online = *cpu_online_mask;

	/* register for notifications of online/offline, freq change, and idle entry/exit */
	register_cpu_notifier(&cpu_nb);
	idle_notifier_register(&idle_nb);
	cpufreq_register_notifier(&cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);

	/* global sysfs config */
	ret = sysfs_create_group(cpufreq_global_kobject, &medusa_attr_group);
	if (ret) {
		pr_err("%s: sysfs_create_group failed\n", __func__);
		goto out_sysfs;
	}

	/* debugfs config */
#if defined(MEDUSA_DEBUG)
	medusa_debug_root = debugfs_create_dir("medusa", NULL);
	if (PTR_ERR(medusa_debug_root) == -ENODEV) {
		pr_info("%s: kernel does not support debugfs\n", __func__);
		medusa_debug_root = NULL;
	} else {
#if defined(MEDUSA_LOG)
		medusa_log_dentry = debugfs_create_file("log", S_IRUSR|S_IWUSR,
				medusa_debug_root, NULL, &log_fops);
		if (IS_ERR(medusa_log_dentry)) {
			pr_info("%s: failed to create log dentry\n", __func__);
			medusa_log_dentry = NULL;
		}
#endif
#if defined(MEDUSA_ORACLE)
		trace_dentry = debugfs_create_file("trace", S_IRUSR|S_IWUSR,
				medusa_debug_root, NULL, &trace_fops);
		if (IS_ERR(trace_dentry)) {
			pr_info("%s: failed to create trace dentry\n", __func__);
			medusa_log_dentry = NULL;
		}
#endif
	}
#endif

	medusa_pmu_start();

	/* start the tick and update work items */
	ret = schedule_delayed_work(&tick_work, usecs_to_jiffies(tick_rate_us));
	BUG_ON(!ret);
	ret = schedule_delayed_work(&update_work, usecs_to_jiffies(update_rate_us));
	BUG_ON(!ret);

	return 0;

 out_sysfs:
	unregister_cpu_notifier(&cpu_nb);
	idle_notifier_unregister(&idle_nb);
	cpufreq_unregister_notifier(&cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);

	return ret;
}

static void medusa_stop(void)
{
	mutex_unlock(&medusa_lock);
	cancel_delayed_work_sync(&tick_work);
	cancel_delayed_work_sync(&update_work);
	mutex_lock(&medusa_lock);

	sysfs_remove_group(cpufreq_global_kobject, &medusa_attr_group);

#if defined(MEDUSA_DEBUG)
	debugfs_remove_recursive(medusa_debug_root);
	medusa_debug_root = NULL;
#endif

	medusa_pmu_stop();

	unregister_cpu_notifier(&cpu_nb);
	idle_notifier_unregister(&idle_nb);
	cpufreq_unregister_notifier(&cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);

	cpumask_clear(&cpus_managed);
}

static int cpufreq_governor_medusa(struct cpufreq_policy *policy,
		unsigned int event)
{
	int ret = 0;
	unsigned int rootcpu = policy->cpu;
	unsigned int cpu;
	struct medusa_cpu_data *mcd;
	struct medusa_cpu_data *mcd0 = &per_cpu(cpu_data, 0);

	mutex_lock(&medusa_lock);

	switch (event) {
	case CPUFREQ_GOV_START:

		if (mstats.medusa_users++ == 0) {
		}

		for_each_cpu(cpu, policy->cpus) {
			mcd = &per_cpu(cpu_data, cpu);
			mcd->policy = policy;
			mcd->cur_freq = policy->cur;
			mcd->set_freq = policy->cur;
			mcd->ticks_online = 0;
			mcd->is_idle = false;
			mcd->start = medusa_time();
			mcd->active_time = 0;
			mcd->idle_time = 0;
		}

		cpumask_or(&cpus_managed, &cpus_managed, policy->cpus);
		cpumask_and(&cpus_online_managed, &cpus_online, &cpus_managed);

		/* Everything tied to CPU0 freq... hacky */
		if (rootcpu != 0) {
			__cpufreq_driver_target(policy, mcd0->cur_freq, CPUFREQ_RELATION_H);
		}

		break;

	case CPUFREQ_GOV_STOP:
		for_each_cpu(cpu, policy->cpus) {
			mcd = &per_cpu(cpu_data, cpu);
			mcd->set_freq = 0;
			mcd->policy = NULL;
		}

		if (--mstats.medusa_users == 0) {
			/* hack: cant differentiate here between core going down and
			 * governor switching
			 */
			cpumask_andnot(&cpus_managed, &cpus_managed, policy->cpus);
			cpumask_and(&cpus_online_managed, &cpus_online, &cpus_managed);
		}

		break;

	case CPUFREQ_GOV_LIMITS:

		for_each_cpu(cpu, policy->cpus) {
			mcd = &per_cpu(cpu_data, cpu);
			if (policy->max < mcd->cur_freq)
				mcd->set_freq = policy->max;
			else if (policy->min > mcd->cur_freq)
				mcd->set_freq = policy->min;
		}

		if (policy->max < policy->cur) {
			__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
		} else if (policy->min > policy->cur) {
			__cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
		}

		break;
	}

	mutex_unlock(&medusa_lock);
	return ret;
}

static struct cpufreq_governor cpufreq_gov_medusa = {
	.name			= "medusa",
	.governor		= cpufreq_governor_medusa,
	.store_setspeed = cpufreq_set,
	.show_setspeed  = cpufreq_show,
	.owner			= THIS_MODULE,
};

static int __init init_medusa(void)
{
	int ret;
	ret = cpufreq_register_governor(&cpufreq_gov_medusa);

	if (!ret) {
		medusa_start();
	}

	return ret;
}

static void __exit exit_medusa(void)
{
	mutex_lock(&medusa_lock);
	medusa_stop();

	cpufreq_unregister_governor(&cpufreq_gov_medusa);
	mutex_unlock(&medusa_lock);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aaron Carroll <Aaron.Carroll@nicta.com.au>");

module_init(init_medusa);
module_exit(exit_medusa);
