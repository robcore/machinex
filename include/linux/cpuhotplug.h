#ifndef __CPUHOTPLUG_H
#define __CPUHOTPLUG_H

#include <linux/types.h>

/*
 * CPU-up			CPU-down
 *
 * BP		AP		BP		AP
 *
 * OFFLINE			OFFLINE
 *   |				  ^
 *   v				  |
 * BRINGUP_CPU->AP_OFFLINE	BRINGUP_CPU  <- AP_IDLE_DEAD (idle thread/play_dead)
 *		  |				AP_OFFLINE
 *		  v (IRQ-off)	  ,---------------^
 *		AP_ONLNE	  | (stop_machine)
 *		  |		TEARDOWN_CPU <-	AP_ONLINE_IDLE
 *		  |				  ^
 *		  v				  |
 *              AP_ACTIVE			AP_ACTIVE
 */

enum cpuhp_state {
	CPUHP_INVALID = -1,
	CPUHP_OFFLINE = 0,
	CPUHP_CREATE_THREADS,
	CPUHP_PERF_PREPARE,
	CPUHP_SLUB_DEAD,
	CPUHP_MM_WRITEBACK_DEAD,
	CPUHP_SOFTIRQ_DEAD,
	CPUHP_IRQ_POLL_DEAD,
	CPUHP_BLOCK_SOFTIRQ_DEAD,
	CPUHP_PERCPU_CNT_DEAD,
	CPUHP_WORKQUEUE_PREP,
	CPUHP_HRTIMERS_PREPARE,
	CPUHP_SMPCFD_PREPARE,
	CPUHP_RELAY_PREPARE,
	CPUHP_SLAB_PREPARE,
	CPUHP_RCUTREE_PREP,
	CPUHP_CPUIDLE_COUPLED_PREPARE,
	CPUHP_NOTIFY_PREPARE,
	CPUHP_TOPOLOGY_PREPARE,
	CPUHP_TIMERS_DEAD,
	CPUHP_BP_PREPARE_DYN,
	CPUHP_BP_PREPARE_DYN_END		= CPUHP_BP_PREPARE_DYN + 20,
	CPUHP_BRINGUP_CPU,
	CPUHP_AP_IDLE_DEAD,
	CPUHP_AP_OFFLINE,
	CPUHP_AP_SCHED_STARTING,
	CPUHP_AP_RCUTREE_DYING,
	CPUHP_AP_IRQ_GIC_STARTING,
	CPUHP_AP_ARM_VFP_STARTING,
	CPUHP_AP_SMPCFD_DYING,
	CPUHP_AP_ONLINE,
	CPUHP_TEARDOWN_CPU,
	CPUHP_AP_ONLINE_IDLE,
	CPUHP_AP_SMPBOOT_THREADS,
	CPUHP_AP_IRQ_AFFINITY_ONLINE,
	CPUHP_AP_PERF_ONLINE,
	CPUHP_AP_WORKQUEUE_ONLINE,
	CPUHP_AP_RCUTREE_ONLINE,
	CPUHP_AP_NOTIFY_ONLINE,
	CPUHP_AP_ONLINE_DYN,
	CPUHP_AP_ONLINE_DYN_END		= CPUHP_AP_ONLINE_DYN + 30,
	CPUHP_AP_ACTIVE,
	CPUHP_ONLINE,
};

int __cpuhp_setup_state(enum cpuhp_state state,	const char *name, bool invoke,
			int (*startup)(unsigned int cpu),
			int (*teardown)(unsigned int cpu), bool multi_instance);

int __cpuhp_setup_state_cpuslocked(enum cpuhp_state state, const char *name,
				   bool invoke,
				   int (*startup)(unsigned int cpu),
				   int (*teardown)(unsigned int cpu),
				   bool multi_instance);
/**
 * cpuhp_setup_state - Setup hotplug state callbacks with calling the callbacks
 * @state:	The state for which the calls are installed
 * @name:	Name of the callback (will be used in debug output)
 * @startup:	startup callback function
 * @teardown:	teardown callback function
 *
 * Installs the callback functions and invokes the startup callback on
 * the present cpus which have already reached the @state.
 */
static inline int cpuhp_setup_state(enum cpuhp_state state,
				    const char *name,
				    int (*startup)(unsigned int cpu),
				    int (*teardown)(unsigned int cpu))
{
	return __cpuhp_setup_state(state, name, true, startup, teardown, false);
}

static inline int cpuhp_setup_state_cpuslocked(enum cpuhp_state state,
					       const char *name,
					       int (*startup)(unsigned int cpu),
					       int (*teardown)(unsigned int cpu))
{
	return __cpuhp_setup_state_cpuslocked(state, name, true, startup,
					      teardown, false);
}

/**
 * cpuhp_setup_state_nocalls - Setup hotplug state callbacks without calling the
 *			       callbacks
 * @state:	The state for which the calls are installed
 * @name:	Name of the callback.
 * @startup:	startup callback function
 * @teardown:	teardown callback function
 *
 * Same as @cpuhp_setup_state except that no calls are executed are invoked
 * during installation of this callback. NOP if SMP=n or HOTPLUG_CPU=n.
 */
static inline int cpuhp_setup_state_nocalls(enum cpuhp_state state,
					    const char *name,
					    int (*startup)(unsigned int cpu),
					    int (*teardown)(unsigned int cpu))
{
	return __cpuhp_setup_state(state, name, false, startup, teardown,
				   false);
}

static inline int cpuhp_setup_state_nocalls_cpuslocked(enum cpuhp_state state,
						     const char *name,
						     int (*startup)(unsigned int cpu),
						     int (*teardown)(unsigned int cpu))
{
	return __cpuhp_setup_state_cpuslocked(state, name, false, startup,
					    teardown, false);
}

/**
 * cpuhp_setup_state_multi - Add callbacks for multi state
 * @state:	The state for which the calls are installed
 * @name:	Name of the callback.
 * @startup:	startup callback function
 * @teardown:	teardown callback function
 *
 * Sets the internal multi_instance flag and prepares a state to work as a multi
 * instance callback. No callbacks are invoked at this point. The callbacks are
 * invoked once an instance for this state are registered via
 * @cpuhp_state_add_instance or @cpuhp_state_add_instance_nocalls.
 */
static inline int cpuhp_setup_state_multi(enum cpuhp_state state,
					  const char *name,
					  int (*startup)(unsigned int cpu,
							 struct hlist_node *node),
					  int (*teardown)(unsigned int cpu,
							  struct hlist_node *node))
{
	return __cpuhp_setup_state(state, name, false,
				   (void *) startup,
				   (void *) teardown, true);
}

int __cpuhp_state_add_instance(enum cpuhp_state state, struct hlist_node *node,
			       bool invoke);
int __cpuhp_state_add_instance_cpuslocked(enum cpuhp_state state,
					  struct hlist_node *node, bool invoke);

/**
 * cpuhp_state_add_instance - Add an instance for a state and invoke startup
 *                            callback.
 * @state:	The state for which the instance is installed
 * @node:	The node for this individual state.
 *
 * Installs the instance for the @state and invokes the startup callback on
 * the present cpus which have already reached the @state. The @state must have
 * been earlier marked as multi-instance by @cpuhp_setup_state_multi.
 */
static inline int cpuhp_state_add_instance(enum cpuhp_state state,
					   struct hlist_node *node)
{
	return __cpuhp_state_add_instance(state, node, true);
}

/**
 * cpuhp_state_add_instance_nocalls - Add an instance for a state without
 *                                    invoking the startup callback.
 * @state:	The state for which the instance is installed
 * @node:	The node for this individual state.
 *
 * Installs the instance for the @state The @state must have been earlier
 * marked as multi-instance by @cpuhp_setup_state_multi.
 */
static inline int cpuhp_state_add_instance_nocalls(enum cpuhp_state state,
						   struct hlist_node *node)
{
	return __cpuhp_state_add_instance(state, node, false);
}

static inline int
cpuhp_state_add_instance_nocalls_cpuslocked(enum cpuhp_state state,
					    struct hlist_node *node)
{
	return __cpuhp_state_add_instance_cpuslocked(state, node, false);
}

void __cpuhp_remove_state(enum cpuhp_state state, bool invoke);
void __cpuhp_remove_state_cpuslocked(enum cpuhp_state state, bool invoke);

/**
 * cpuhp_remove_state - Remove hotplug state callbacks and invoke the teardown
 * @state:	The state for which the calls are removed
 *
 * Removes the callback functions and invokes the teardown callback on
 * the present cpus which have already reached the @state.
 */
static inline void cpuhp_remove_state(enum cpuhp_state state)
{
	__cpuhp_remove_state(state, true);
}

/**
 * cpuhp_remove_state_nocalls - Remove hotplug state callbacks without invoking
 *				teardown
 * @state:	The state for which the calls are removed
 */
static inline void cpuhp_remove_state_nocalls(enum cpuhp_state state)
{
	__cpuhp_remove_state(state, false);
}

static inline void cpuhp_remove_state_nocalls_cpuslocked(enum cpuhp_state state)
{
	__cpuhp_remove_state_cpuslocked(state, false);
}

/**
 * cpuhp_remove_multi_state - Remove hotplug multi state callback
 * @state:	The state for which the calls are removed
 *
 * Removes the callback functions from a multi state. This is the reverse of
 * cpuhp_setup_state_multi(). All instances should have been removed before
 * invoking this function.
 */
static inline void cpuhp_remove_multi_state(enum cpuhp_state state)
{
	__cpuhp_remove_state(state, false);
}

int __cpuhp_state_remove_instance(enum cpuhp_state state,
				  struct hlist_node *node, bool invoke);

/**
 * cpuhp_state_remove_instance - Remove hotplug instance from state and invoke
 *                               the teardown callback
 * @state:	The state from which the instance is removed
 * @node:	The node for this individual state.
 *
 * Removes the instance and invokes the teardown callback on the present cpus
 * which have already reached the @state.
 */
static inline int cpuhp_state_remove_instance(enum cpuhp_state state,
					      struct hlist_node *node)
{
	return __cpuhp_state_remove_instance(state, node, true);
}

/**
 * cpuhp_state_remove_instance_nocalls - Remove hotplug instance from state
 *					 without invoking the reatdown callback
 * @state:	The state from which the instance is removed
 * @node:	The node for this individual state.
 *
 * Removes the instance without invoking the teardown callback.
 */
static inline int cpuhp_state_remove_instance_nocalls(enum cpuhp_state state,
						      struct hlist_node *node)
{
	return __cpuhp_state_remove_instance(state, node, false);
}

#ifdef CONFIG_SMP
void cpuhp_online_idle(enum cpuhp_state state);
#else
static inline void cpuhp_online_idle(enum cpuhp_state state) { }
#endif

#endif
