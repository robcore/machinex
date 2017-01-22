/* Copyright (c) 2013, Steve Loebrich <sloebric@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Generic auto hotplug driver for ARM SoCs. Targeted at current generation
 * SoCs with dual and quad core applications processors.
 * Automatically hotplugs online and offline CPUs based on system load.
 * It is also capable of immediately onlining a core based on an external
 * event by calling void hotplug_boostpulse(void)
 *
 * Not recommended for use with OMAP4460 due to the potential for lockups
 * whilst hotplugging.
 * 
 * Thanks to Thalamus for the inspiration!
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
//#include <linux/rq_stats.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define IX_HOTPLUG "ix_hotplug"

/*
 * Load defines:
 * ENABLE_ALL is a high watermark to rapidly online all CPUs
 *
 * ENABLE is the load which is required to enable 1 extra CPU
 * DISABLE is the load at which a CPU is disabled
 * These two are scaled based on num_online_cpus()
 */

static struct delayed_work hotplug_decision_work;
static struct work_struct suspend;
static struct work_struct resume;
static struct workqueue_struct *ixwq;

static unsigned int enable_all_load = 800;
static unsigned int enable_load[5] = {0, 120, 220, 340, 0};
static unsigned int disable_load[5] = {0, 0, 60, 120, 260};
static unsigned int sample_rate[5] = {0, 25, 50, 100, 50};
static unsigned int online_sampling_periods[5] = {0, 3, 3, 5, 0};
static unsigned int offline_sampling_periods[5] = {0, 0, 8, 3, 4};
static unsigned int online_cpus;
static unsigned int min_cpus_online = 1;
static unsigned int sampling_rate;
static unsigned int available_cpus;
static unsigned int online_sample;
static unsigned int offline_sample;

static void hotplug_online_single_work(void)
{
	cpu_up(online_cpus);
	//pr_info("ix_hotplug: CPU%d up.\n", online_cpus);
	return;
}

static void hotplug_online_all_work(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		if (likely(!cpu_online(cpu))) {
			cpu_up(cpu);
			//pr_info("ix_hotplug: CPU%d up.\n", cpu);
		}
	}
	return;
}

static void hotplug_offline_work(void)
{
	cpu_down(online_cpus - 1);
	//pr_info("ix_hotplug: CPU%d down.\n", (online_cpus - 1));
	return;
}

static void __ref hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int avg_running, io_wait;
	//unsigned int rq_depth;
	
	sched_get_nr_running_avg(&avg_running, &io_wait);
	//rq_depth = rq_info.rq_avg;
	
	if ((avg_running <= disable_load[online_cpus]) &&
			(online_cpus > min_cpus_online)) {
		//pr_info("ix_hotplug: Disable Exit - %d %d %d %d %d\n", online_cpus, offline_sample, offline_sampling_periods[online_cpus], disable_load[online_cpus], avg_running);
		if (offline_sample >= offline_sampling_periods[online_cpus]) {
			//pr_info("ix_hotplug: Disable Single\n");
			hotplug_offline_work();
			offline_sample = 0;
			online_cpus = num_online_cpus();
			//pr_info("ix_hotplug: Threshold: %d Load: %d Sampling: %d RQ: %d\n", load_disable, avg_running, sampling_rate, rq_depth);
		}
		offline_sample++;
		online_sample = 1;
		goto exit;
	}

	if ((avg_running >= enable_all_load || avg_running >= enable_load[online_cpus]) &&
			(online_cpus < available_cpus)) {
		//pr_info("ix_hotplug: Enable Exit - %d %d %d %d %d\n", online_cpus, online_sample, online_sampling_periods[online_cpus], enable_load[online_cpus], avg_running);
		if (online_sample >= online_sampling_periods[online_cpus]) {
			if (avg_running >= enable_all_load) {
				//pr_info("ix_hotplug: Enable All\n");
				hotplug_online_all_work();
			} else {
				//pr_info("ix_hotplug: Enable Single\n");
				hotplug_online_single_work();
			}
			online_cpus = num_online_cpus();
			online_sample = 0;
		}		
		online_sample++;
		offline_sample = 1;
		goto exit;
	}
	
	//pr_info("ix_hotplug: Idle, CPUs: %d %d\n", online_cpus, num_online_cpus());
	
exit:

	sampling_rate = sample_rate[online_cpus];
	
	//pr_info("ix_hotplug: CPUs: %d Load: %d Sampling: %d\n", online_cpus, avg_running, sampling_rate);
	
	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, msecs_to_jiffies(sampling_rate));
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ix_hotplug_suspend(struct work_struct *work)
{
	cancel_delayed_work_sync(&hotplug_decision_work);
	drain_workqueue(ixwq);
	cpu_down(1);
	cpu_down(2);
	cpu_down(3);
	pr_info("ix_hotplug: Early Suspend\n");
}

static void __ref ix_hotplug_resume(struct work_struct *work)
{
	offline_sample = 1;
	online_sample = 1;
	cpu_up(1);
	online_cpus = num_online_cpus();
	queue_delayed_work_on(0,  ixwq, &hotplug_decision_work, msecs_to_jiffies(2500));
	pr_info("ix_hotplug: Late Resume\n");
}

static void ix_hotplug_early_suspend(struct early_suspend *handler)
{
	schedule_work(&suspend);
}

static void ix_hotplug_late_resume(struct early_suspend *handler)
{
	schedule_work(&resume);
}

static struct early_suspend early_suspend = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 10,
	.suspend = ix_hotplug_early_suspend,
	.resume = ix_hotplug_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int __devinit ix_hotplug_probe(struct platform_device *pdev)
{	
	online_cpus = num_online_cpus();
	available_cpus = num_possible_cpus();
	
	ixwq = alloc_workqueue("ix_hotplug_workqueue", WQ_HIGHPRI, 1);
    
    if (!ixwq)
        return -ENOMEM;
        
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&early_suspend);
#endif
	
	INIT_WORK(&suspend, ix_hotplug_suspend);
	INIT_WORK(&resume, ix_hotplug_resume);
	INIT_DELAYED_WORK(&hotplug_decision_work, hotplug_decision_work_fn);

	/*
	 * Give the system time to boot before fiddling with hotplugging.
	 */

	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, msecs_to_jiffies(10000));

	pr_info("ix_hotplug: v2.0 - InstigatorX\n");
	pr_info("ix_hotplug: based on v0.220 by _thalamus\n");
	
	return 0;
}

static struct platform_device ix_hotplug_device = {
	.name = IX_HOTPLUG,
	.id = -1,
};

static int ix_hotplug_remove(struct platform_device *pdev)
{

	cancel_delayed_work_sync(&hotplug_decision_work);
	drain_workqueue(ixwq);
	
	return 0;
}

static struct platform_driver ix_hotplug_driver = {
	.probe = ix_hotplug_probe,
	.remove = ix_hotplug_remove,
	.driver = {
		.name = IX_HOTPLUG,
		.owner = THIS_MODULE,
	},
};

static int __init ix_hotplug_init(void)
{
	int ret;

	ret = platform_driver_register(&ix_hotplug_driver);

	if (ret)
	{
		return ret;
	}

	ret = platform_device_register(&ix_hotplug_device);

	if (ret)
	{
		return ret;
	}

	pr_info("%s: init\n", IX_HOTPLUG);

	return ret;
}

static void __exit ix_hotplug_exit(void)
{
	platform_device_unregister(&ix_hotplug_device);
	platform_driver_unregister(&ix_hotplug_driver);
}

late_initcall(ix_hotplug_init);
module_exit(ix_hotplug_exit);
