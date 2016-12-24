/* Add subsystem definitions of the form SUBSYS(<name>) in this
 * file. Surround each one by a line of comment markers so that
 * patches don't collide
 */

/* */

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CPUSETS)
SUBSYS(cpuset)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_DEBUG)
SUBSYS(debug)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_SCHED)
SUBSYS(cpu_cgroup)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_CPUACCT)
SUBSYS(cpuacct)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_MEMCG)
SUBSYS(mem_cgroup)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_DEVICE)
SUBSYS(devices)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_FREEZER)
SUBSYS(freezer)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_NET_CLS_CGROUP)
SUBSYS(net_cls)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_BLK_CGROUP)
SUBSYS(blkio)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_BFQIO)
SUBSYS(bfqio)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_PERF)
SUBSYS(perf)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_NETPRIO_CGROUP)
SUBSYS(net_prio)
#endif

/* */

#ifdef IS_SUBSYS_ENABLED(CONFIG_CGROUP_HUGETLB)
SUBSYS(hugetlb)
#endif

/* */
