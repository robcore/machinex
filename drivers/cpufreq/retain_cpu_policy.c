/*
 * Copyright (c) 2015, Emmanuel Utomi <emmanuelutomi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/retain_cpu_policy.h>

struct cpufreq_retain_policy retained_policy[CONFIG_NR_CPUS];

int policy_sync;
static struct kobject *policy_sync_kobj;

static ssize_t show_policy_sync(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", policy_sync);
}

static ssize_t store_policy_sync(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%du", &policy_sync);
	return count;
}

static struct kobj_attribute policy_sync_attr = __ATTR(policy_sync, 0666, show_policy_sync, store_policy_sync);

static struct attribute *attrs[] = {
	&policy_sync_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
.attrs = attrs,
};

void retain_cpu_policy(struct cpufreq_policy *policy)
{
	if (likely(policy)){
		retained_policy[policy->cpu].min = policy->min;
		retained_policy[policy->cpu].max = policy->max;
		retained_policy[policy->cpu].governor = policy->governor;
	}
}

void restore_cpu_policy(struct cpufreq_policy *policy, enum cpufreq_restore_flags flag)
{
	if (retained_cpu_policy(policy->cpu)) {

		switch(flag){

			case CPUFREQ_RESTORE_GOVERNOR:
				if(policy->governor != get_retained_governor(policy->cpu))
					policy->governor = get_retained_governor(policy->cpu);
					break;
			
			case CPUFREQ_RESTORE_FREQ:
				if(policy->min != get_retained_min_cpu_freq(policy->cpu))
					policy->min = get_retained_min_cpu_freq(policy->cpu);
				if(policy->max != get_retained_max_cpu_freq(policy->cpu))	
					policy->max = get_retained_max_cpu_freq(policy->cpu);
					break;

			default:
				if(policy->min != get_retained_min_cpu_freq(policy->cpu))
					policy->min = get_retained_min_cpu_freq(policy->cpu);
				if(policy->max != get_retained_max_cpu_freq(policy->cpu))	
					policy->max = get_retained_max_cpu_freq(policy->cpu);
				if(policy->governor != get_retained_governor(policy->cpu))
					policy->governor = get_retained_governor(policy->cpu);
				break;
		}
	}
}

bool retained_cpu_policy(int cpu)
{
	return (get_retained_min_cpu_freq(cpu) && get_retained_max_cpu_freq(cpu) && get_retained_governor(cpu));
}

bool sync_retained_cpu_policy()
{
	return (policy_sync == 1 ? true : false);
}

struct cpufreq_governor* get_retained_governor(int cpu)
{
	return (sync_retained_cpu_policy() ? retained_policy[0].governor : retained_policy[cpu].governor);
}

unsigned int get_retained_min_cpu_freq(int cpu)
{
	return (sync_retained_cpu_policy() ? retained_policy[0].min : retained_policy[cpu].min);
}

unsigned int get_retained_max_cpu_freq(int cpu)
{
	return (sync_retained_cpu_policy() ? retained_policy[0].max : retained_policy[cpu].max);
}

static int sync_cpu_policy(struct notifier_block *nb, unsigned long val, void *data)
{
	struct cpufreq_policy *policy = data;

	switch(val){

		case CPUFREQ_START:
			restore_cpu_policy(policy, CPUFREQ_RESTORE_ALL);
			break;

		case CPUFREQ_ADJUST:
			if(sync_retained_cpu_policy() && policy->cpu > 0)
				restore_cpu_policy(policy, CPUFREQ_RESTORE_ALL);
			retain_cpu_policy(policy);
			break;
	}
	
	return NOTIFY_OK;
}

static struct notifier_block sync_cpu_policy_nb = {
	.notifier_call = sync_cpu_policy,
};

int retain_cpu_policy_init(void)
{
	int retval;
	policy_sync = 1;
	printk("retain_cpu_policy: Emman was here.\n");
	
	policy_sync_kobj = kobject_create_and_add("retain_cpu_policy", kernel_kobj);
        if (!policy_sync_kobj) {
                return -ENOMEM;
        }
        retval = sysfs_create_group(policy_sync_kobj, &attr_group);
        if (retval)
                kobject_put(policy_sync_kobj);
              
	cpufreq_register_notifier(&sync_cpu_policy_nb, CPUFREQ_POLICY_NOTIFIER);
                
	return retval;
}

module_init(retain_cpu_policy_init);
MODULE_AUTHOR("Emmanuel Utomi <emmanuelutomi@gmail.com>");
