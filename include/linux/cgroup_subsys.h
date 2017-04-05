/*
 * List of cgroup subsystems.
 *
 * DO NOT ADD ANY SUBSYSTEM WITHOUT EXPLICIT ACKS FROM CGROUP MAINTAINERS.
 */
#if IS_SUBSYS_ENABLED(CONFIG_CPUSETS)
SUBSYS(cpuset)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_DEBUG)
SUBSYS(debug)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_SCHED)
SUBSYS(cpu_cgroup)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_CPUACCT)
SUBSYS(cpuacct)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_MEMCG)
SUBSYS(mem_cgroup)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_DEVICE)
SUBSYS(devices)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_FREEZER)
SUBSYS(freezer)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_NET_CLS_CGROUP)
SUBSYS(net_cls)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_BLK_CGROUP)
SUBSYS(blkio)
#endif

/* */

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_BFQIO)
SUBSYS(bfqio)
#endif

/* */

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_PERF)
SUBSYS(perf)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_NETPRIO_CGROUP)
SUBSYS(net_prio)
#endif

#if IS_SUBSYS_ENABLED(CONFIG_CGROUP_HUGETLB)
SUBSYS(hugetlb)
#endif

#ifdef CONFIG_CGROUP_BCACHE
SUBSYS(bcache)
#endif
/*
 * DO NOT ADD ANY SUBSYSTEM WITHOUT EXPLICIT ACKS FROM CGROUP MAINTAINERS.
 */
