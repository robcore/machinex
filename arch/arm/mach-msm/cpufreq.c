/* arch/arm/mach-msm/cpufreq.c
 *
 * MSM architecture cpufreq driver
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2014, The Linux Foundation. All rights reserved.
 * Author: Mike A. Chan <mikechan@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/suspend.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <trace/events/power.h>
#include <mach/cpufreq.h>
#include <mach/msm_bus.h>

#include "acpuclock.h"

#ifdef CONFIG_USE_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <asm/div64.h>
#endif

static DEFINE_MUTEX(l2bw_lock);

static struct clk *cpu_clk[NR_CPUS];
static struct clk *l2_clk;
static unsigned int freq_index[NR_CPUS];
static struct cpufreq_frequency_table *freq_table;
static unsigned int *l2_khz;
static bool is_sync;
static struct msm_bus_vectors *bus_vec_lst;
static struct msm_bus_scale_pdata bus_bw = {
	.name = "msm-cpufreq",
	.active_only = 1,
};
static u32 bus_client;

struct cpufreq_work_struct {
	struct work_struct work;
	struct cpufreq_policy *policy;
	struct completion complete;
	int frequency;
	unsigned int index;
	int status;
};

static DEFINE_PER_CPU(struct cpufreq_work_struct, cpufreq_work);
static struct workqueue_struct *msm_cpufreq_wq;

struct cpufreq_suspend_t {
	struct mutex suspend_mutex;
	int device_suspended;
};

static DEFINE_PER_CPU(struct cpufreq_suspend_t, cpufreq_suspend);

static DEFINE_PER_CPU(struct cpu_freq, cpu_freq_info);

static void update_l2_bw(int *also_cpu)
{
	int rc = 0, cpu;
	unsigned int index = 0;

	mutex_lock(&l2bw_lock);

	if (also_cpu)
		index = freq_index[*also_cpu];

	for_each_online_cpu(cpu) {
		index = max(index, freq_index[cpu]);
	}

	if (l2_clk)
		rc = clk_set_rate(l2_clk, l2_khz[index] * 1000);
	if (rc) {
		pr_err("Error setting L2 clock rate!\n");
		goto out;
	}

	if (bus_client)
		rc = msm_bus_scale_client_update_request(bus_client, index);
	if (rc)
		pr_err("Bandwidth req failed (%d)\n", rc);

out:
	mutex_unlock(&l2bw_lock);
}

static int set_cpu_freq(struct cpufreq_policy *policy, unsigned int new_freq,
			unsigned int index)
{
	int ret = 0;
	int saved_sched_policy = -EINVAL;
	int saved_sched_rt_prio = -EINVAL;
	struct cpufreq_freqs freqs;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	unsigned long rate;

	freqs.old = policy->cur;
	freqs.new = new_freq;
	freqs.cpu = policy->cpu;

	/*
	 * Put the caller into SCHED_FIFO priority to avoid cpu starvation
	 * while increasing frequencies
	 */

	if (freqs.new > freqs.old && current->policy != SCHED_FIFO) {
		saved_sched_policy = current->policy;
		saved_sched_rt_prio = current->rt_priority;
		sched_setscheduler_nocheck(current, SCHED_FIFO, &param);
	}

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	rate = new_freq * 1000;
	rate = clk_round_rate(cpu_clk[policy->cpu], rate);
	ret = clk_set_rate(cpu_clk[policy->cpu], rate);
	if (!ret) {
		freq_index[policy->cpu] = index;
		update_l2_bw(NULL);
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	}

	/* Restore priority after clock ramp-up */
	if (freqs.new > freqs.old && saved_sched_policy >= 0) {
		param.sched_priority = saved_sched_rt_prio;
		sched_setscheduler_nocheck(current, saved_sched_policy, &param);
	}
	return ret;
}

static void set_cpu_work(struct work_struct *work)
{
	struct cpufreq_work_struct *cpu_work =
		container_of(work, struct cpufreq_work_struct, work);

	cpu_work->status = set_cpu_freq(cpu_work->policy, cpu_work->frequency,
					cpu_work->index);
	complete(&cpu_work->complete);
}

static int msm_cpufreq_target(struct cpufreq_policy *policy,
				unsigned int target_freq,
				unsigned int relation)
{
	int ret = 0;
	int index;
	struct cpufreq_frequency_table *table;

	struct cpufreq_work_struct *cpu_work = NULL;

	mutex_lock(&per_cpu(cpufreq_suspend, policy->cpu).suspend_mutex);

	if (target_freq == policy->cur)
		goto done;

	if (per_cpu(cpufreq_suspend, policy->cpu).device_suspended) {
		pr_debug("cpufreq: cpu%d scheduling frequency change "
				"in suspend.\n", policy->cpu);
		ret = -EFAULT;
		goto done;
	}

	table = cpufreq_frequency_get_table(policy->cpu);
	if (cpufreq_frequency_table_target(policy, table, target_freq, relation,
			&index)) {
		pr_err("cpufreq: invalid target_freq: %d\n", target_freq);
		ret = -EINVAL;
		goto done;
	}

	pr_debug("CPU[%d] target %d relation %d (%d-%d) selected %d\n",
		policy->cpu, target_freq, relation,
		policy->min, policy->max, table[index].frequency);

	cpu_work = &per_cpu(cpufreq_work, policy->cpu);
	cpu_work->policy = policy;
	cpu_work->frequency = table[index].frequency;
	cpu_work->index = table[index].driver_data;
	cpu_work->status = -ENODEV;

	cancel_work_sync(&cpu_work->work);
	INIT_COMPLETION(cpu_work->complete);
	queue_work_on(policy->cpu, msm_cpufreq_wq, &cpu_work->work);
	wait_for_completion(&cpu_work->complete);

	ret = cpu_work->status;

done:
	mutex_unlock(&per_cpu(cpufreq_suspend, policy->cpu).suspend_mutex);
	return ret;
}

static int msm_cpufreq_verify(struct cpufreq_policy *policy)
{
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);
	return 0;
}

static unsigned int msm_cpufreq_get_freq(unsigned int cpu)
{
	if (is_sync)
		cpu = 0;
	return clk_get_rate(cpu_clk[cpu]) / 1000;
}

static int msm_cpufreq_init(struct cpufreq_policy *policy)
{
	int cur_freq;
	int index;
	int ret = 0;
	struct cpufreq_frequency_table *table;
	struct cpufreq_work_struct *cpu_work = NULL;

	table = cpufreq_frequency_get_table(policy->cpu);
	if (table == NULL)
		return -ENODEV;
	/*
	 * In some SoC, cpu cores' frequencies can not
	 * be changed independently. Each cpu is bound to
	 * same frequency. Hence set the cpumask to all cpu.
	 */
	if (is_sync)
		cpumask_setall(policy->cpus);

	cpu_work = &per_cpu(cpufreq_work, policy->cpu);
	INIT_WORK(&cpu_work->work, set_cpu_work);
	init_completion(&cpu_work->complete);

	/* synchronous cpus share the same policy */
	if (!cpu_clk[policy->cpu])
		return 0;

	if (cpufreq_frequency_table_cpuinfo(policy, table)) {
#ifdef CONFIG_MSM_CPU_FREQ_SET_MIN_MAX
		policy->cpuinfo.min_freq = CONFIG_MSM_CPU_FREQ_MIN;
		policy->cpuinfo.max_freq = CONFIG_MSM_CPU_FREQ_MAX;
#endif
	}
#ifdef CONFIG_MSM_CPU_FREQ_SET_MIN_MAX
	policy->min = CONFIG_MSM_CPU_FREQ_MIN;
	policy->max = CONFIG_MSM_CPU_FREQ_MAX;
#endif

	cur_freq = clk_get_rate(cpu_clk[policy->cpu])/1000;

	if (cpufreq_frequency_table_target(policy, table, cur_freq,
	    CPUFREQ_RELATION_H, &index) &&
	    cpufreq_frequency_table_target(policy, table, cur_freq,
	    CPUFREQ_RELATION_L, &index)) {
		pr_info("cpufreq: cpu%d at invalid freq: %d\n",
				policy->cpu, cur_freq);
		return -EINVAL;
	}
	/*
	 * Call set_cpu_freq unconditionally so that when cpu is set to
	 * online, frequency limit will always be updated.
	 */
	ret = set_cpu_freq(policy, table[index].frequency,
			   table[index].driver_data);
	if (ret)
		return ret;
	pr_debug("cpufreq: cpu%d init at %d switching to %d\n",
			policy->cpu, cur_freq, table[index].frequency);
	policy->cur = table[index].frequency;

	return 0;
}

static int msm_cpufreq_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	int rc;

	switch (action & ~CPU_TASKS_FROZEN) {
	/*
	 * Scale down clock/power of CPU that is dead and scale it back up
	 * before the CPU is brought up.
	 */
	case CPU_DEAD:
		clk_disable_unprepare(cpu_clk[cpu]);
		clk_disable_unprepare(l2_clk);
		update_l2_bw(NULL);
		break;
	case CPU_UP_CANCELED:
		clk_unprepare(cpu_clk[cpu]);
		clk_unprepare(l2_clk);
		update_l2_bw(NULL);
		break;
	case CPU_UP_PREPARE:
		rc = clk_prepare(l2_clk);
		if (rc < 0)
			return NOTIFY_BAD;
		rc = clk_prepare(cpu_clk[cpu]);
		if (rc < 0) {
			clk_unprepare(l2_clk);
			return NOTIFY_BAD;
		}
		update_l2_bw(&cpu);
		break;

	case CPU_STARTING:
		rc = clk_enable(l2_clk);
		if (rc < 0)
			return NOTIFY_BAD;
		rc = clk_enable(cpu_clk[cpu]);
		if (rc) {
			clk_disable(l2_clk);
			return NOTIFY_BAD;
		}
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata msm_cpufreq_cpu_notifier = {
	.notifier_call = msm_cpufreq_cpu_callback,
};

static int msm_cpufreq_suspend(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_lock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
		per_cpu(cpufreq_suspend, cpu).device_suspended = 1;
		mutex_unlock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
	}

	return NOTIFY_DONE;
}

static int msm_cpufreq_resume(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		per_cpu(cpufreq_suspend, cpu).device_suspended = 0;
	}

	return NOTIFY_DONE;
}

static int msm_cpufreq_pm_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		return msm_cpufreq_resume();
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		return msm_cpufreq_suspend();
	default:
		return NOTIFY_DONE;
	}
}

static struct notifier_block msm_cpufreq_pm_notifier = {
	.notifier_call = msm_cpufreq_pm_event,
};

static struct freq_attr *msm_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver msm_cpufreq_driver = {
	/* lps calculations are handled here. */
	.flags		= CPUFREQ_STICKY | CPUFREQ_CONST_LOOPS,
	.init		= msm_cpufreq_init,
	.verify		= msm_cpufreq_verify,
	.target		= msm_cpufreq_target,
	.get		= msm_cpufreq_get_freq,
	.name		= "msm",
	.attr		= msm_freq_attr,
};

#define PROP_TBL "qcom,cpufreq-table"
#define PROP_PORTS "qcom,cpu-mem-ports"
static int cpufreq_parse_dt(struct device *dev)
{
	int ret, len, nf, num_cols = 1, num_paths = 0, i, j, k;
	u32 *data, *ports = NULL;
	struct msm_bus_vectors *v = NULL;

	if (l2_clk)
		num_cols++;

	/* Parse optional bus ports parameter */
	if (of_find_property(dev->of_node, PROP_PORTS, &len)) {
		len /= sizeof(*ports);
		if (len % 2)
			return -EINVAL;

		ports = devm_kzalloc(dev, len * sizeof(*ports), GFP_KERNEL);
		if (!ports)
			return -ENOMEM;
		ret = of_property_read_u32_array(dev->of_node, PROP_PORTS,
						 ports, len);
		if (ret)
			return ret;
		num_paths = len / 2;
		num_cols++;
	}

	/* Parse CPU freq -> L2/Mem BW map table. */
	if (!of_find_property(dev->of_node, PROP_TBL, &len))
		return -EINVAL;
	len /= sizeof(*data);

	if (len % num_cols || len == 0)
		return -EINVAL;
	nf = len / num_cols;

	data = devm_kzalloc(dev, len * sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = of_property_read_u32_array(dev->of_node, PROP_TBL, data, len);
	if (ret)
		return ret;

	/* Allocate all data structures. */
	freq_table = devm_kzalloc(dev, (nf + 1) * sizeof(*freq_table),
				  GFP_KERNEL);
	if (!freq_table)
		return -ENOMEM;

	if (l2_clk) {
		l2_khz = devm_kzalloc(dev, nf * sizeof(*l2_khz), GFP_KERNEL);
		if (!l2_khz)
			return -ENOMEM;
	}

	if (num_paths) {
		int sz_u = nf * sizeof(*bus_bw.usecase);
		int sz_v = nf * num_paths * sizeof(*bus_vec_lst);
		bus_bw.usecase = devm_kzalloc(dev, sz_u, GFP_KERNEL);
		v = bus_vec_lst = devm_kzalloc(dev, sz_v, GFP_KERNEL);
		if (!bus_bw.usecase || !bus_vec_lst)
			return -ENOMEM;
	}

	j = 0;
	for (i = 0; i < nf; i++) {
		unsigned long f;

		f = clk_round_rate(cpu_clk[0], data[j++] * 1000);
		if (IS_ERR_VALUE(f))
			break;
		f /= 1000;

		/*
		 * Check if this is the last feasible frequency in the table.
		 *
		 * The table listing frequencies higher than what the HW can
		 * support is not an error since the table might be shared
		 * across CPUs in different speed bins. It's also not
		 * sufficient to check if the rounded rate is lower than the
		 * requested rate as it doesn't cover the following example:
		 *
		 * Table lists: 2.2 GHz and 2.5 GHz.
		 * Rounded rate returns: 2.2 GHz and 2.3 GHz.
		 *
		 * In this case, we can CPUfreq to use 2.2 GHz and 2.3 GHz
		 * instead of rejecting the 2.5 GHz table entry.
		 */
		if (i > 0 && f <= freq_table[i-1].frequency)
			break;

		freq_table[i].driver_data = i;
		freq_table[i].frequency = f;

		if (l2_clk) {
			f = clk_round_rate(l2_clk, data[j++] * 1000);
			if (IS_ERR_VALUE(f)) {
				pr_err("Error finding L2 rate for CPU %d KHz\n",
					freq_table[i].frequency);
				freq_table[i].frequency = CPUFREQ_ENTRY_INVALID;
			} else {
				f /= 1000;
				l2_khz[i] = f;
			}
		}

		if (num_paths) {
			unsigned int bw_mbps = data[j++];
			bus_bw.usecase[i].num_paths = num_paths;
			bus_bw.usecase[i].vectors = v;
			for (k = 0; k < num_paths; k++) {
				v->src = ports[k * 2];
				v->dst = ports[k * 2 + 1];
				v->ib = bw_mbps * 1000000ULL;
				v++;
			}
		}
	}

	bus_bw.num_usecases = i;
	freq_table[i].driver_data = i;
	freq_table[i].frequency = CPUFREQ_TABLE_END;

	if (ports)
		devm_kfree(dev, ports);
	devm_kfree(dev, data);

	return 0;
}

#ifdef CONFIG_USE_DEBUG_FS
static int msm_cpufreq_show(struct seq_file *m, void *unused)
{
	unsigned int i, cpu_freq;

	if (!freq_table)
		return 0;

	seq_printf(m, "%10s%10s", "CPU (KHz)", "L2 (KHz)");
	seq_printf(m, "%12s\n", "Mem (MBps)");

	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		cpu_freq = freq_table[i].frequency;
		if (cpu_freq == CPUFREQ_ENTRY_INVALID)
			continue;
		seq_printf(m, "%10d", cpu_freq);
		seq_printf(m, "%10d", l2_khz ? l2_khz[i] : cpu_freq);
		seq_printf(m, "%12lu", mem_bw[i]);
		seq_printf(m, "\n");
	}
	return 0;
}

static int msm_cpufreq_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_cpufreq_show, inode->i_private);
}

const struct file_operations msm_cpufreq_fops = {
	.open		= msm_cpufreq_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif

static int __init msm_cpufreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	char clk_name[] = "cpu??_clk";
	struct clk *c;
	int cpu, ret;

	l2_clk = devm_clk_get(dev, "l2_clk");
	if (IS_ERR(l2_clk))
		l2_clk = NULL;

	for_each_possible_cpu(cpu) {
		snprintf(clk_name, sizeof(clk_name), "cpu%d_clk", cpu);
		c = devm_clk_get(dev, clk_name);
		if (!IS_ERR(c))
			cpu_clk[cpu] = c;
		else
			is_sync = true;
	}

	if (!cpu_clk[0])
		return -ENODEV;

	ret = cpufreq_parse_dt(dev);
	if (ret)
		return ret;

	for_each_possible_cpu(cpu) {
		cpufreq_frequency_table_get_attr(freq_table, cpu);
	}

	if (bus_bw.usecase) {
		bus_client = msm_bus_scale_register_client(&bus_bw);
		if (!bus_client)
			dev_warn(dev, "Unable to register bus client\n");
	}

#ifdef CONFIG_USE_DEBUG_FS
	if (!debugfs_create_file("msm_cpufreq", S_IRUGO, NULL, NULL,
		&msm_cpufreq_fops))
		return -ENOMEM;
#endif

	return 0;
}

static struct of_device_id match_table[] = {
	{ .compatible = "qcom,msm-cpufreq" },
	{}
};

static struct platform_driver msm_cpufreq_plat_driver = {
	.driver = {
		.name = "msm-cpufreq",
		.of_match_table = match_table,
		.owner = THIS_MODULE,
	},
};

static int __init msm_cpufreq_register(void)
{
	int cpu, rc;

	for_each_possible_cpu(cpu) {
		mutex_init(&(per_cpu(cpufreq_suspend, cpu).suspend_mutex));
		per_cpu(cpufreq_suspend, cpu).device_suspended = 0;
	}

	rc = platform_driver_probe(&msm_cpufreq_plat_driver,
				   msm_cpufreq_probe);
	if (rc < 0) {
		/* Unblock hotplug if msm-cpufreq probe fails */
		unregister_hotcpu_notifier(&msm_cpufreq_cpu_notifier);
		for_each_possible_cpu(cpu)
			mutex_destroy(&(per_cpu(cpufreq_suspend, cpu).
					suspend_mutex));
		return rc;
	}

	msm_cpufreq_wq = alloc_workqueue("msm-cpufreq", WQ_HIGHPRI, 0);
	register_pm_notifier(&msm_cpufreq_pm_notifier);
	return cpufreq_register_driver(&msm_cpufreq_driver);
}

subsys_initcall(msm_cpufreq_register);

static int __init msm_cpufreq_early_register(void)
{
	return register_hotcpu_notifier(&msm_cpufreq_cpu_notifier);
}
core_initcall(msm_cpufreq_early_register);
