/* drivers/cpufreq/qcom-cpufreq.c
 *
 * MSM architecture cpufreq driver
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2016, The Linux Foundation. All rights reserved.
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
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/suspend.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <trace/events/power.h>
#include <mach/cpufreq.h>
#include <mach/msm_bus.h>
#include <linux/qcom-cpufreq.h>
#include <linux/cpufreq_hardlimit.h>

static DEFINE_MUTEX(l2bw_lock);

static struct clk *cpu_clk[NR_CPUS];
static struct clk *l2_clk;
static unsigned int freq_index[NR_CPUS];
static unsigned int *l2_khz;
static bool is_clk;
static bool is_sync;
static struct msm_bus_vectors *bus_vec_lst;
static struct msm_bus_scale_pdata bus_bw = {
	.name = "msm-cpufreq",
	.active_only = 1,
};
static u32 bus_client;

struct cpufreq_suspend_t {
	struct mutex suspend_mutex;
	int device_suspended;
};

static DEFINE_PER_CPU(struct cpufreq_suspend_t, suspend_data);

struct cpu_freq {
	uint32_t max;
	uint32_t min;
	uint32_t allowed_max;
	uint32_t allowed_min;
	uint32_t limits_init;
};

static DEFINE_PER_CPU(struct cpu_freq, cpu_freq_info);

struct qcom_cpufreq_krait {
	struct acpu_level *priv;
	struct scalable *scalable;
	const struct l2_level *l2_freq_tbl;
	struct device *dev;
	struct hfpll_data *hfpll_data;
} qck;

static DEFINE_PER_CPU(struct cpufreq_frequency_table *, freq_table);

static int set_cpu_freq(struct cpufreq_policy *policy, unsigned int new_freq,
			unsigned int index)
{
	int ret = 0;
	struct cpufreq_freqs freqs;

	freqs.old = policy->cur;
	freqs.new = new_freq;
	freqs.cpu = policy->cpu;

	cpufreq_freq_transition_begin(policy, &freqs);
	ret = acpuclk_set_rate(policy->cpu, new_freq, SETRATE_CPUFREQ);
	cpufreq_freq_transition_end(policy, &freqs, ret);

	return ret;
}

static int msm_cpufreq_target(struct cpufreq_policy *policy,
				unsigned int target_freq,
				unsigned int relation)
{
	int ret = 0;
	int index;
	struct cpufreq_frequency_table *table;

	mutex_lock(&per_cpu(suspend_data, policy->cpu).suspend_mutex);

	if (target_freq == policy->cur)
		goto done;

	if (per_cpu(suspend_data, policy->cpu).device_suspended) {
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

	ret = set_cpu_freq(policy, table[index].frequency,
			   table[index].driver_data);
done:
	mutex_unlock(&per_cpu(suspend_data, policy->cpu).suspend_mutex);
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
	return acpuclk_get_rate(cpu);
}

static inline int msm_cpufreq_limits_init(void)
{
	int cpu = 0;
	int i = 0;
	struct cpufreq_frequency_table *table = NULL;
	uint32_t min = (uint32_t) -1;
	uint32_t max = 0;
	struct cpu_freq *limit = NULL;

	for_each_possible_cpu(cpu) {
		limit = &per_cpu(cpu_freq_info, cpu);
		table = cpufreq_frequency_get_table(cpu);
		if (table == NULL) {
			pr_err("%s: error reading cpufreq table for cpu %d\n",
					__func__, cpu);
			continue;
		}
		for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) {
			if (table[i].frequency > max)
				max = table[i].frequency;
			if (table[i].frequency < min)
				min = table[i].frequency;
		}
		limit->allowed_min = min;
		limit->allowed_max = max;
		limit->min = min;
		limit->max = max;
		limit->limits_init = 1;
	}

	return 0;
}

int msm_cpufreq_set_freq_limits(uint32_t cpu, uint32_t min, uint32_t max)
{
	struct cpu_freq *limit = &per_cpu(cpu_freq_info, cpu);

	if (!limit->limits_init)
		msm_cpufreq_limits_init();

	if ((min != MSM_CPUFREQ_NO_LIMIT) &&
		min >= limit->min && min <= limit->max)
		limit->allowed_min = min;
	else
		limit->allowed_min = limit->min;


	if ((max != MSM_CPUFREQ_NO_LIMIT) &&
		max <= limit->max && max >= limit->min)
		limit->allowed_max = max;
	else
		limit->allowed_max = limit->max;

	return 0;
}
EXPORT_SYMBOL(msm_cpufreq_set_freq_limits);

static struct cpufreq_frequency_table *cpufreq_table_init(struct cpufreq_policy *policy)
{
	struct cpufreq_frequency_table *ktable[35];
	int cpu;
	struct cpufreq_frequency_table *freq_table;

	int freq_cnt;

		int i;
		/* Construct the freq_table tables from priv. */
		for (i = 0, freq_cnt = 0; qck.priv[i].speed.khz != 0
				&& freq_cnt < sizeof(ktable[35 - 1]); i++) {
			if (qck.priv[i].use_for_scaling) {
				freq_table[freq_cnt].driver_data = freq_cnt;
				freq_table[freq_cnt].frequency = qck.priv[i].speed.khz;
					
				freq_cnt++;
			}
		}
		/* freq_table not big enough to store all usable freqs. */
		BUG_ON(qck.priv[i].speed.khz != 0);

		freq_table[freq_cnt].driver_data = freq_cnt;
		freq_table[freq_cnt].frequency = CPUFREQ_TABLE_END;

		/* Register table with CPUFreq. */
		cpufreq_frequency_get_table(cpu);
		return freq_table;
}

static int msm_cpufreq_init(struct cpufreq_policy *policy)
{
	int cur_freq;
	int index;
	int ret = 0;

	int cpu;
	policy = per_cpu(policy, cpu);
	struct cpufreq_frequency_table *table;
	
	for_each_possible_cpu(cpu)
		table = cpufreq_table_init(cpu);
	
	/*
	 * In some SoC, cpu cores' frequencies can not
	 * be changed independently. Each cpu is bound to
	 * same frequency. Hence set the cpumask to all cpu.
	 * (but ours isn't one of them)
	 */
	if (is_sync)
		cpumask_setall(policy->cpus);

	ret = cpufreq_table_validate_and_show(policy, table);
	if (ret) {
		pr_err("cpufreq: failed to get policy min/max\n");
		return ret;
	}

	cur_freq = acpuclk_get_rate(policy->cpu);

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

	policy->cpuinfo.transition_latency =
		acpuclk_get_switch_time() * NSEC_PER_USEC;

	return 0;
}

static int msm_cpufreq_suspend(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_lock(&per_cpu(suspend_data, cpu).suspend_mutex);
		per_cpu(suspend_data, cpu).device_suspended = 1;
		mutex_unlock(&per_cpu(suspend_data, cpu).suspend_mutex);
	}

	return NOTIFY_DONE;
}

static int msm_cpufreq_resume(void)
{
	int cpu, ret;
	struct cpufreq_policy policy;

	for_each_possible_cpu(cpu) {
		per_cpu(suspend_data, cpu).device_suspended = 0;
	}

	/*
	 * Freq request might be rejected during suspend, resulting
	 * in policy->cur violating min/max constraint.
	 * Correct the frequency as soon as possible.
	 */
	get_online_cpus();
	for_each_online_cpu(cpu) {
		ret = cpufreq_get_policy(&policy, cpu);
		if (ret)
			continue;
		if (policy.cur <= policy.max && policy.cur >= policy.min)
			continue;
		ret = cpufreq_update_policy(cpu);
		if (ret)
			pr_info("cpufreq: Current frequency violates policy min/max for CPU%d\n",
			       cpu);
		else
			pr_info("cpufreq: Frequency violation fixed for CPU%d\n",
				cpu);
	}
	put_online_cpus();

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

static struct platform_driver msm_cpufreq_plat_driver = {
	.driver = {
		.name = "msm-cpufreq",
		.owner = THIS_MODULE,
	},
};

static int __init msm_cpufreq_register(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_init(&(per_cpu(suspend_data, cpu).suspend_mutex));
		per_cpu(suspend_data, cpu).device_suspended = 0;
	}

	register_pm_notifier(&msm_cpufreq_pm_notifier);
	return cpufreq_register_driver(&msm_cpufreq_driver);
}

late_initcall(msm_cpufreq_register);
