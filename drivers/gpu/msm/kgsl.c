/* Copyright (c) 2008-2012,2014 The Linux Foundation. All rights reserved.
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
 */
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/android_pmem.h>
#include <linux/vmalloc.h>
#include <linux/pm_runtime.h>
#include <linux/genlock.h>
#include <linux/rbtree.h>
#include <linux/ashmem.h>
#include <linux/major.h>
#include <linux/msm_ion.h>
#include <linux/io.h>
#include <mach/socinfo.h>
#include <linux/mman.h>

#include "kgsl.h"
#include "kgsl_debugfs.h"
#include "kgsl_cffdump.h"
#include "kgsl_log.h"
#include "kgsl_sharedmem.h"
#include "kgsl_device.h"
#include "kgsl_trace.h"
#include "kgsl_sync.h"
#include "adreno.h"

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "kgsl."

static int kgsl_pagetable_count = KGSL_PAGETABLE_COUNT;
static char *ksgl_mmu_type;
module_param_named(ptcount, kgsl_pagetable_count, int, 0);
MODULE_PARM_DESC(kgsl_pagetable_count,
"Minimum number of pagetables for KGSL to allocate at initialization time");
module_param_named(mmutype, ksgl_mmu_type, charp, 0);
MODULE_PARM_DESC(ksgl_mmu_type,
"Type of MMU to be used for graphics. Valid values are 'iommu' or 'gpummu' or 'nommu'");

static struct ion_client *kgsl_ion_client;

static void kgsl_mem_entry_detach_process(struct kgsl_mem_entry *entry);

/*
 * The memfree list contains the last N blocks of memory that have been freed.
 * On a GPU fault we walk the list to see if the faulting address had been
 * recently freed and print out a message to that effect
 */

#define MEMFREE_ENTRIES 512

static DEFINE_SPINLOCK(memfree_lock);

struct memfree_entry {
	unsigned long gpuaddr;
	unsigned long size;
	pid_t pid;
	unsigned int flags;
};

static struct {
	struct memfree_entry *list;
	unsigned int head;
	unsigned int tail;
} memfree;

/**
 * kgsl_hang_check() - Check for GPU hang
 * data: KGSL device structure
 *
 * This function is called every KGSL_TIMEOUT_HANG_DETECT time when
 * GPU is active to check for hang. If a hang is detected we
 * trigger fault tolerance.
 */
void kgsl_hang_check(struct work_struct *work)
{
	struct kgsl_device *device = container_of(work, struct kgsl_device,
							hang_check_ws);
	static unsigned int prev_reg_val[FT_DETECT_REGS_COUNT];

	mutex_lock(&device->mutex);

	if (device->state == KGSL_STATE_ACTIVE) {

		/* Check to see if the GPU is hung */
#if !defined(CONFIG_MSM_IOMMU) && defined(CONFIG_SEC_PRODUCT_8960)
		/*Hung detection should only be done on 2d device*/
		if (device->id == KGSL_DEVICE_3D0 )
		{
#endif
		if (adreno_ft_detect(device, prev_reg_val))
			adreno_dump_and_exec_ft(device);
#if !defined(CONFIG_MSM_IOMMU) && defined(CONFIG_SEC_PRODUCT_8960)
		}
#endif
		mod_timer(&device->hang_timer,
			(jiffies + msecs_to_jiffies(KGSL_TIMEOUT_HANG_DETECT)));
	}

	mutex_unlock(&device->mutex);
}

/**
 * hang_timer() - Hang timer function
 * data: KGSL device structure
 *
 * This function is called when hang timer expires, in this
 * function we check if GPU is in active state and queue the
 * work on device workqueue to check for the hang. We restart
 * the timer after KGSL_TIMEOUT_HANG_DETECT time.
 */
void hang_timer(unsigned long data)
{
	struct kgsl_device *device = (struct kgsl_device *) data;

	/* check Hang only for 3d device */
	if (device->id == KGSL_DEVICE_3D0) {
		if (device->state == KGSL_STATE_ACTIVE) {

			/* Have work run in a non-interrupt context. */
			queue_work(device->work_queue, &device->hang_check_ws);
		}
	}
}

/**
 * kgsl_trace_issueibcmds() - Call trace_issueibcmds by proxy
 * device: KGSL device
 * id: ID of the context submitting the command
 * ibdesc: Pointer to the list of IB descriptors
 * numib: Number of IBs in the list
 * timestamp: Timestamp assigned to the command batch
 * flags: Flags sent by the user
 * result: Result of the submission attempt
 * type: Type of context issuing the command
 *
 * Wrap the issueibcmds ftrace hook into a function that can be called from the
 * GPU specific modules.
 */
void kgsl_trace_issueibcmds(struct kgsl_device *device, int id,
		struct kgsl_ibdesc *ibdesc, int numibs,
		unsigned int timestamp, unsigned int flags,
		int result, unsigned int type)
{
	trace_kgsl_issueibcmds(device, id, ibdesc, numibs,
		timestamp, flags, result, type);
}
EXPORT_SYMBOL(kgsl_trace_issueibcmds);

/**
 * kgsl_trace_regwrite - call regwrite ftrace function by proxy
 * device: KGSL device
 * offset: dword offset of the register being written
 * value: Value of the register being written
 *
 * Wrap the regwrite ftrace hook into a function that can be called from the
 * GPU specific modules.
 */
void kgsl_trace_regwrite(struct kgsl_device *device, unsigned int offset,
		unsigned int value)
{
	trace_kgsl_regwrite(device, offset, value);
}
EXPORT_SYMBOL(kgsl_trace_regwrite);

void kgsl_trace_kgsl_tz_params(struct kgsl_device *device, s64 total_time,
		 s64 busy_time, int idle_time, int tz_val) {

       trace_kgsl_tz_params(device, total_time, busy_time, idle_time, tz_val);
}
EXPORT_SYMBOL(kgsl_trace_kgsl_tz_params);

int kgsl_memfree_init(void)
{
	memfree.list = kzalloc(MEMFREE_ENTRIES * sizeof(struct memfree_entry),
		GFP_KERNEL);

	return (memfree.list) ? 0 : -ENOMEM;
}

void kgsl_memfree_exit(void)
{
	kfree(memfree.list);
	memset(&memfree, 0, sizeof(memfree));
}

int kgsl_memfree_find_entry(pid_t pid, unsigned long *gpuaddr,
	unsigned long *size, unsigned int *flags)
{
	int ptr;

	if (memfree.list == NULL)
		return 0;

	spin_lock(&memfree_lock);

	ptr = memfree.head - 1;
	if (ptr < 0)
		ptr = MEMFREE_ENTRIES - 1;

	/* Walk backwards through the list looking for the last match  */
	while (ptr != memfree.tail) {
		struct memfree_entry *entry = &memfree.list[ptr];

		if ((entry->pid == pid) &&
			(*gpuaddr >= entry->gpuaddr &&
			 *gpuaddr < (entry->gpuaddr + entry->size))) {
			*gpuaddr = entry->gpuaddr;
			*flags = entry->flags;
			*size = entry->size;

			spin_unlock(&memfree_lock);
			return 1;
		}

		ptr = ptr - 1;

		if (ptr < 0)
			ptr = MEMFREE_ENTRIES - 1;
	}

	spin_unlock(&memfree_lock);
	return 0;
}

static void kgsl_memfree_add(pid_t pid, unsigned int gpuaddr,
		unsigned int size, int flags)

{
	struct memfree_entry *entry;

	if (memfree.list == NULL)
		return;

	spin_lock(&memfree_lock);

	entry = &memfree.list[memfree.head];

	entry->pid = pid;
	entry->gpuaddr = gpuaddr;
	entry->size = size;
	entry->flags = flags;

	memfree.head = (memfree.head + 1) % MEMFREE_ENTRIES;

	if (memfree.head == memfree.tail)
		memfree.tail = (memfree.tail + 1) % MEMFREE_ENTRIES;

	spin_unlock(&memfree_lock);
}

/* kgsl_get_mem_entry - get the mem_entry structure for the specified object
 * @device - Pointer to the device structure
 * @ptbase - the pagetable base of the object
 * @gpuaddr - the GPU address of the object
 * @size - Size of the region to search
 *
 * Caller must kgsl_mem_entry_put() the returned entry when finished using it.
 */

struct kgsl_mem_entry * __must_check
kgsl_get_mem_entry(struct kgsl_device *device, unsigned int ptbase,
		   unsigned int gpuaddr, unsigned int size)
{
	struct kgsl_process_private *priv;
	struct kgsl_mem_entry *entry;

	mutex_lock(&kgsl_driver.process_mutex);

	list_for_each_entry(priv, &kgsl_driver.process_list, list) {
		if (!kgsl_mmu_pt_equal(&device->mmu, priv->pagetable, ptbase))
			continue;
		entry = kgsl_sharedmem_find_region(priv, gpuaddr, size);

		if (entry) {
			mutex_unlock(&kgsl_driver.process_mutex);
			return entry;
		}
	}
	mutex_unlock(&kgsl_driver.process_mutex);

	return NULL;
}
EXPORT_SYMBOL(kgsl_get_mem_entry);

static inline struct kgsl_mem_entry *
kgsl_mem_entry_create(void)
{
	struct kgsl_mem_entry *entry = kzalloc(sizeof(*entry), GFP_KERNEL);

	if (!entry)
		KGSL_CORE_ERR("kzalloc(%d) failed\n", sizeof(*entry));
	else
		kref_init(&entry->refcount);

	return entry;
}

void
kgsl_mem_entry_destroy(struct kref *kref)
{
	struct kgsl_mem_entry *entry = container_of(kref,
						    struct kgsl_mem_entry,
						    refcount);

	/* Detach from process list */
	kgsl_mem_entry_detach_process(entry);

	if (entry->memtype != KGSL_MEM_ENTRY_KERNEL)
		kgsl_driver.stats.mapped -= entry->memdesc.size;

	/*
	 * Ion takes care of freeing the sglist for us so
	 * clear the sg before freeing the sharedmem so kgsl_sharedmem_free
	 * doesn't try to free it again
	 */

	if (entry->memtype == KGSL_MEM_ENTRY_ION) {
		entry->memdesc.sg = NULL;
	}

	kgsl_sharedmem_free(&entry->memdesc);

	switch (entry->memtype) {
	case KGSL_MEM_ENTRY_PMEM:
	case KGSL_MEM_ENTRY_ASHMEM:
		if (entry->priv_data)
			fput(entry->priv_data);
		break;
	case KGSL_MEM_ENTRY_ION:
		ion_free(kgsl_ion_client, entry->priv_data);
		break;
	}

	kfree(entry);
}
EXPORT_SYMBOL(kgsl_mem_entry_destroy);

/**
 * kgsl_mem_entry_track_gpuaddr - Insert a mem_entry in the address tree and
 * assign it with a gpu address space before insertion
 * @process: the process that owns the memory
 * @entry: the memory entry
 *
 * @returns - 0 on succcess else error code
 *
 * Insert the kgsl_mem_entry in to the rb_tree for searching by GPU address.
 * The assignment of gpu address and insertion into list needs to
 * happen with the memory lock held to avoid race conditions between
 * gpu address being selected and some other thread looking through the
 * rb list in search of memory based on gpuaddr
 * This function should be called with processes memory spinlock held
 */
static int
kgsl_mem_entry_track_gpuaddr(struct kgsl_process_private *process,
				struct kgsl_mem_entry *entry)
{
	int ret = 0;
	struct rb_node **node;
	struct rb_node *parent = NULL;

	assert_spin_locked(&process->mem_lock);
	/*
	 * If cpu=gpu map is used then caller needs to set the
	 * gpu address
	 */
	if (kgsl_memdesc_use_cpu_map(&entry->memdesc)) {
		if (!entry->memdesc.gpuaddr)
			goto done;
	} else if (entry->memdesc.gpuaddr) {
		WARN_ONCE(1, "gpuaddr assigned w/o holding memory lock\n");
		ret = -EINVAL;
		goto done;
	}
	if (!kgsl_memdesc_use_cpu_map(&entry->memdesc)) {
		ret = kgsl_mmu_get_gpuaddr(process->pagetable, &entry->memdesc);
		if (ret)
			goto done;
	}

	node = &process->mem_rb.rb_node;

	while (*node) {
		struct kgsl_mem_entry *cur;

		parent = *node;
		cur = rb_entry(parent, struct kgsl_mem_entry, node);

		if (entry->memdesc.gpuaddr < cur->memdesc.gpuaddr)
			node = &parent->rb_left;
		else
			node = &parent->rb_right;
	}

	rb_link_node(&entry->node, parent, node);
	rb_insert_color(&entry->node, &process->mem_rb);

done:
	return ret;
}

/**
 * kgsl_mem_entry_untrack_gpuaddr() - Untrack memory that is previously tracked
 * process - Pointer to process private to which memory belongs
 * entry - Memory entry to untrack
 *
 * Function just does the opposite of kgsl_mem_entry_track_gpuaddr. Needs to be
 * called with processes spin lock held
 */
static void
kgsl_mem_entry_untrack_gpuaddr(struct kgsl_process_private *process,
				struct kgsl_mem_entry *entry)
{
	assert_spin_locked(&process->mem_lock);
	if (entry->memdesc.gpuaddr) {
		kgsl_mmu_put_gpuaddr(process->pagetable, &entry->memdesc);
		rb_erase(&entry->node, &entry->priv->mem_rb);
	}
}

/**
 * kgsl_mem_entry_attach_process - Attach a mem_entry to its owner process
 * @entry: the memory entry
 * @process: the owner process
 *
 * Attach a newly created mem_entry to its owner process so that
 * it can be found later. The mem_entry will be added to mem_idr and have
 * its 'id' field assigned. If the GPU address has been set, the entry
 * will also be added to the mem_rb tree.
 *
 * @returns - 0 on success or error code on failure.
 */
static int
kgsl_mem_entry_attach_process(struct kgsl_mem_entry *entry,
				   struct kgsl_device_private *dev_priv)
{
	int ret;
	struct kgsl_process_private *process = dev_priv->process_priv;

	ret = kgsl_process_private_get(process);
	if (!ret)
		return -EBADF;

	while (1) {
		if (idr_pre_get(&process->mem_idr, GFP_KERNEL) == 0) {
			ret = -ENOMEM;
			goto err_put_proc_priv;
		}

		spin_lock(&process->mem_lock);
		ret = idr_get_new_above(&process->mem_idr, entry, 1,
					&entry->id);
		spin_unlock(&process->mem_lock);

		if (ret == 0)
			break;
		else if (ret != -EAGAIN)
			goto err_put_proc_priv;
	}
	entry->priv = process;
	entry->dev_priv = dev_priv;

	spin_lock(&process->mem_lock);
	ret = kgsl_mem_entry_track_gpuaddr(process, entry);
	if (ret)
		idr_remove(&process->mem_idr, entry->id);
	spin_unlock(&process->mem_lock);
	if (ret)
		goto err_put_proc_priv;
	/* map the memory after unlocking if gpuaddr has been assigned */
	if (entry->memdesc.gpuaddr) {
		ret = kgsl_mmu_map(process->pagetable, &entry->memdesc);
		if (ret)
			kgsl_mem_entry_detach_process(entry);
	}
	return ret;

err_put_proc_priv:
	kgsl_process_private_put(process);
	return ret;
}

/* Detach a memory entry from a process and unmap it from the MMU */

static void kgsl_mem_entry_detach_process(struct kgsl_mem_entry *entry)
{
	if (entry == NULL)
		return;

	/* Unmap here so that below we can call kgsl_mmu_put_gpuaddr */
	kgsl_mmu_unmap(entry->priv->pagetable, &entry->memdesc);

	spin_lock(&entry->priv->mem_lock);

	kgsl_mem_entry_untrack_gpuaddr(entry->priv, entry);
	if (entry->id != 0)
		idr_remove(&entry->priv->mem_idr, entry->id);
	entry->id = 0;

	entry->priv->stats[entry->memtype].cur -= entry->memdesc.size;
	spin_unlock(&entry->priv->mem_lock);
	kgsl_process_private_put(entry->priv);

	entry->priv = NULL;
}

/* Allocate a new context id */

static struct kgsl_context *
kgsl_create_context(struct kgsl_device_private *dev_priv)
{
	struct kgsl_device *device = dev_priv->device;
	struct kgsl_context *context;
	int ret, id = 0;

	context = kzalloc(sizeof(*context), GFP_KERNEL);

	if (context == NULL)
		return ERR_PTR(-ENOMEM);


	while (1) {
		if (idr_pre_get(&device->context_idr, GFP_KERNEL) == 0) {
			KGSL_DRV_INFO(device, "idr_pre_get: ENOMEM\n");
			ret = -ENOMEM;
			break;
		}

		write_lock(&device->context_lock);
		ret = idr_get_new_above(&device->context_idr, context, 1, &id);
		context->id = id;
		write_unlock(&device->context_lock);

		if (ret != -EAGAIN)
			break;
	}

	if (ret)
		goto func_end;

	/* MAX - 1, there is one memdesc in memstore for device info */
	if (id >= KGSL_MEMSTORE_MAX) {
		KGSL_DRV_INFO(device, "cannot have more than %d "
				"ctxts due to memstore limitation\n",
				KGSL_MEMSTORE_MAX);
		ret = -ENOSPC;
		goto fail_free_id;
	}

	kref_init(&context->refcount);
	context->dev_priv = dev_priv;
	ret = kgsl_sync_timeline_create(context);
	if (ret) {
		goto fail_free_id;
	}

	/* Initialize the pending event list */
	INIT_LIST_HEAD(&context->events);

	/*
	 * Initialize the node that is used to maintain the master list of
	 * contexts with pending events in the device structure. Normally we
	 * wouldn't take the time to initalize a node but at event add time we
	 * call list_empty() on the node as a quick way of determining if the
	 * context is already in the master list so it needs to always be either
	 * active or in an unused but initialized state
	 */

	INIT_LIST_HEAD(&context->events_list);

fail_free_id:
	if (ret) {
		write_lock(&device->context_lock);
		idr_remove(&device->context_idr, id);
		write_unlock(&device->context_lock);
	}

func_end:
	if (ret) {
		kfree(context);
		return ERR_PTR(ret);
	}

	return context;
}

/**
 * kgsl_context_detach - Release the "master" context reference
 * @context - The context that will be detached
 *
 * This is called when a context becomes unusable, because userspace
 * has requested for it to be destroyed. The context itself may
 * exist a bit longer until its reference count goes to zero.
 * Other code referencing the context can detect that it has been
 * detached because the context id will be set to KGSL_CONTEXT_INVALID.
 */
void
kgsl_context_detach(struct kgsl_context *context)
{
	int id;
	struct kgsl_device *device;
	if (context == NULL)
		return;
	device = context->dev_priv->device;
	trace_kgsl_context_detach(device, context);
	id = context->id;

	if (device->ftbl->drawctxt_destroy)
		device->ftbl->drawctxt_destroy(device, context);
	/*device specific drawctxt_destroy MUST clean up devctxt */
	BUG_ON(context->devctxt);
	/*
	 * Cancel events after the device-specific context is
	 * destroyed, to avoid possibly freeing memory while
	 * it is still in use by the GPU.
	 */
	kgsl_context_cancel_events(device, context);

	write_lock(&device->context_lock);
	context->id = KGSL_CONTEXT_INVALID;
	idr_remove(&device->context_idr, id);
	write_unlock(&device->context_lock);
	context->dev_priv = NULL;
	kgsl_context_put(context);
}

void
kgsl_context_destroy(struct kref *kref)
{
	struct kgsl_context *context = container_of(kref, struct kgsl_context,
						    refcount);
	kgsl_sync_timeline_destroy(context);
	kfree(context);
}

struct kgsl_device *kgsl_get_device(int dev_idx)
{
	int i;
	struct kgsl_device *ret = NULL;

	mutex_lock(&kgsl_driver.devlock);

	for (i = 0; i < KGSL_DEVICE_MAX; i++) {
		if (kgsl_driver.devp[i] && kgsl_driver.devp[i]->id == dev_idx) {
			ret = kgsl_driver.devp[i];
			break;
		}
	}

	mutex_unlock(&kgsl_driver.devlock);
	return ret;
}
EXPORT_SYMBOL(kgsl_get_device);

static struct kgsl_device *kgsl_get_minor(int minor)
{
	struct kgsl_device *ret = NULL;

	if (minor < 0 || minor >= KGSL_DEVICE_MAX)
		return NULL;

	mutex_lock(&kgsl_driver.devlock);
	ret = kgsl_driver.devp[minor];
	mutex_unlock(&kgsl_driver.devlock);

	return ret;
}

int kgsl_check_timestamp(struct kgsl_device *device,
	struct kgsl_context *context, unsigned int timestamp)
{
	unsigned int ts_processed;

	ts_processed = kgsl_readtimestamp(device, context,
					  KGSL_TIMESTAMP_RETIRED);

	return (timestamp_cmp(ts_processed, timestamp) >= 0);
}
EXPORT_SYMBOL(kgsl_check_timestamp);

static int kgsl_suspend_device(struct kgsl_device *device, pm_message_t state)
{
	int status = -EINVAL;
	unsigned int nap_allowed_saved;
	struct kgsl_pwrscale_policy *policy_saved;

	if (!device)
		return -EINVAL;

	KGSL_PWR_WARN(device, "suspend start\n");

	mutex_lock(&device->mutex);
	nap_allowed_saved = device->pwrctrl.nap_allowed;
	device->pwrctrl.nap_allowed = false;
	policy_saved = device->pwrscale.policy;
	device->pwrscale.policy = NULL;
	kgsl_pwrctrl_request_state(device, KGSL_STATE_SUSPEND);

	/*
	 * Make sure no user process is waiting for a timestamp
	 * before supending.
	 */
	kgsl_active_count_wait(device);

	/* Don't let the timer wake us during suspended sleep. */
	del_timer_sync(&device->idle_timer);
	del_timer_sync(&device->hang_timer);
	switch (device->state) {
		case KGSL_STATE_INIT:
			break;
		case KGSL_STATE_ACTIVE:
			/* Wait for the device to become idle */
			device->ftbl->idle(device);
		case KGSL_STATE_NAP:
		case KGSL_STATE_SLEEP:
			/* make sure power is on to stop the device */
			kgsl_pwrctrl_enable(device);
			kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_ON);
			/* Get the completion ready to be waited upon. */
			INIT_COMPLETION(device->hwaccess_gate);
			device->ftbl->suspend_context(device);
			device->ftbl->stop(device);
			pm_qos_update_request(&device->pm_qos_req_dma,
						PM_QOS_DEFAULT_VALUE);
			kgsl_pwrctrl_set_state(device, KGSL_STATE_SUSPEND);
			break;
		case KGSL_STATE_SLUMBER:
			INIT_COMPLETION(device->hwaccess_gate);
			kgsl_pwrctrl_set_state(device, KGSL_STATE_SUSPEND);
			break;
		default:
			KGSL_PWR_ERR(device, "suspend fail, device %d\n",
					device->id);
			goto end;
	}
	kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
	device->pwrctrl.nap_allowed = nap_allowed_saved;
	device->pwrscale.policy = policy_saved;
	status = 0;

end:
	mutex_unlock(&device->mutex);
	KGSL_PWR_WARN(device, "suspend end\n");
	return status;
}

static int kgsl_resume_device(struct kgsl_device *device)
{
	if (!device)
		return -EINVAL;

	KGSL_PWR_WARN(device, "resume start\n");
	mutex_lock(&device->mutex);
	if (device->state == KGSL_STATE_SUSPEND) {
		kgsl_pwrctrl_set_state(device, KGSL_STATE_SLUMBER);
		complete_all(&device->hwaccess_gate);
	} else if (device->state != KGSL_STATE_INIT) {
		/*
		 * This is an error situation,so wait for the device
		 * to idle and then put the device to SLUMBER state.
		 * This will put the device to the right state when
		 * we resume.
		 */
		if (device->state == KGSL_STATE_ACTIVE)
			device->ftbl->idle(device);
		kgsl_pwrctrl_request_state(device, KGSL_STATE_SLUMBER);
		kgsl_pwrctrl_sleep(device);
		KGSL_PWR_ERR(device,
			"resume invoked without a suspend\n");
	}
	kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);

	mutex_unlock(&device->mutex);
	KGSL_PWR_WARN(device, "resume end\n");
	return 0;
}

static int kgsl_suspend(struct device *dev)
{

	pm_message_t arg = {0};
	struct kgsl_device *device = dev_get_drvdata(dev);
	return kgsl_suspend_device(device, arg);
}

static int kgsl_resume(struct device *dev)
{
	struct kgsl_device *device = dev_get_drvdata(dev);
	return kgsl_resume_device(device);
}

static int kgsl_runtime_suspend(struct device *dev)
{
	return 0;
}

static int kgsl_runtime_resume(struct device *dev)
{
	return 0;
}

const struct dev_pm_ops kgsl_pm_ops = {
	.suspend = kgsl_suspend,
	.resume = kgsl_resume,
	.runtime_suspend = kgsl_runtime_suspend,
	.runtime_resume = kgsl_runtime_resume,
};
EXPORT_SYMBOL(kgsl_pm_ops);

void kgsl_early_suspend_driver(struct early_suspend *h)
{
	struct kgsl_device *device = container_of(h,
					struct kgsl_device, display_off);
	KGSL_PWR_WARN(device, "early suspend start\n");
	mutex_lock(&device->mutex);

	/* Only go to slumber if active_cnt is 0 */
	if (device->active_cnt == 0) {
		device->pwrctrl.restore_slumber = true;
		kgsl_pwrctrl_request_state(device, KGSL_STATE_SLUMBER);
		kgsl_pwrctrl_sleep(device);
	}

	mutex_unlock(&device->mutex);
	KGSL_PWR_WARN(device, "early suspend end\n");
}
EXPORT_SYMBOL(kgsl_early_suspend_driver);

int kgsl_suspend_driver(struct platform_device *pdev,
					pm_message_t state)
{
	struct kgsl_device *device = dev_get_drvdata(&pdev->dev);
	return kgsl_suspend_device(device, state);
}
EXPORT_SYMBOL(kgsl_suspend_driver);

int kgsl_resume_driver(struct platform_device *pdev)
{
	struct kgsl_device *device = dev_get_drvdata(&pdev->dev);
	return kgsl_resume_device(device);
}
EXPORT_SYMBOL(kgsl_resume_driver);

void kgsl_late_resume_driver(struct early_suspend *h)
{
	struct kgsl_device *device = container_of(h,
					struct kgsl_device, display_off);
	KGSL_PWR_WARN(device, "late resume start\n");
	mutex_lock(&device->mutex);
	device->pwrctrl.restore_slumber = false;
	if (device->pwrscale.policy == NULL)
		kgsl_pwrctrl_pwrlevel_change(device, KGSL_PWRLEVEL_TURBO);

	if (kgsl_pwrctrl_wake(device) != 0) {
		mutex_unlock(&device->mutex);
		return;
	}
	/*
	 * We don't have a way to go directly from
	 * a deeper sleep state to NAP, which is
	 * the desired state here.
	 *
	 * Except if active_cnt is non zero which means that
	 * we probably went to early_suspend with it non zero
	 * and thus the system is still in an active state.
	 */

	if (device->active_cnt == 0) {
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NAP);
		kgsl_pwrctrl_sleep(device);
	}

	mutex_unlock(&device->mutex);
	KGSL_PWR_WARN(device, "late resume end\n");
}
EXPORT_SYMBOL(kgsl_late_resume_driver);

/*
 * kgsl_destroy_process_private() - Cleanup function to free process private
 * @kref: - Pointer to object being destroyed's kref struct
 * Free struct object and all other resources attached to it.
 * Since the function can be used when not all resources inside process
 * private have been allocated, there is a check to (before each resource
 * cleanup) see if the struct member being cleaned is in fact allocated or not.
 * If the value is not NULL, resource is freed.
 */
static void kgsl_destroy_process_private(struct kref *kref)
{
	struct kgsl_process_private *private = container_of(kref,
			struct kgsl_process_private, refcount);

	/*
	 * Remove this process from global process list
	 * We do not acquire a lock first as it is expected that
	 * kgsl_destroy_process_private() is only going to be called
	 * through kref_put() which is only called after acquiring
	 * the lock.
	 */
	if (!private) {
		KGSL_CORE_ERR("Cannot destroy null process private\n");
		mutex_unlock(&kgsl_driver.process_mutex);
		return;
	}

	if (private->kobj.parent)
		kgsl_process_uninit_sysfs(private);
	if (private->debug_root)
		debugfs_remove_recursive(private->debug_root);

	list_del(&private->list);
	mutex_unlock(&kgsl_driver.process_mutex);

	kgsl_mmu_putpagetable(private->pagetable);
	idr_destroy(&private->mem_idr);

	kfree(private);
	return;
}

void
kgsl_process_private_put(struct kgsl_process_private *private)
{
	mutex_lock(&kgsl_driver.process_mutex);

	/*
	 * kref_put() returns 1 when the refcnt has reached 0 and the destroy
	 * function is called. Mutex is released in the destroy function if
	 * its called, so only release mutex if kref_put() return 0
	 */
	if (!kref_put(&private->refcount, kgsl_destroy_process_private))
		mutex_unlock(&kgsl_driver.process_mutex);
	return;
}

/**
 * kgsl_process_private_find() - Find the process associated with the specified
 * name
 * @name: pid_t of the process to search for
 * Return the process struct for the given ID.
 */
struct kgsl_process_private *kgsl_process_private_find(pid_t pid)
{
	struct kgsl_process_private *p, *private = NULL;

	mutex_lock(&kgsl_driver.process_mutex);
	list_for_each_entry(p, &kgsl_driver.process_list, list) {
		if (p->pid == pid) {
			if (kgsl_process_private_get(p))
				private = p;
			break;
		}
	}
	mutex_unlock(&kgsl_driver.process_mutex);
	return private;
}

/**
 * kgsl_process_private_new() - Helper function to search for process private
 * Returns: Pointer to the found/newly created private struct
 */
static struct kgsl_process_private *kgsl_process_private_new(void)
{
	struct kgsl_process_private *private;

	/* Search in the process list */
	mutex_lock(&kgsl_driver.process_mutex);
	list_for_each_entry(private, &kgsl_driver.process_list, list) {
		if (private->pid == task_tgid_nr(current)) {
			if (!kgsl_process_private_get(private))
				private = NULL;
			goto done;
		}
	}

	/* no existing process private found for this dev_priv, create one */
	private = kzalloc(sizeof(struct kgsl_process_private), GFP_KERNEL);
	if (private == NULL)
		goto done;

	kref_init(&private->refcount);

	private->pid = task_tgid_nr(current);
	spin_lock_init(&private->mem_lock);
	mutex_init(&private->process_private_mutex);
	/* Add the newly created process struct obj to the process list */
	list_add(&private->list, &kgsl_driver.process_list);
done:
	mutex_unlock(&kgsl_driver.process_mutex);
	return private;
}

/*
 * kgsl_get_process_private() - Used to find the process private structure
 * @cur_dev_priv: Current device pointer
 * Finds or creates a new porcess private structire and initializes its members
 * Returns: Pointer to the private process struct obj found/created or
 * NULL if pagetable creation for this process private obj failed.
 */
static struct kgsl_process_private *
kgsl_get_process_private(struct kgsl_device *device)
{
	struct kgsl_process_private *private;

	private = kgsl_process_private_new();

	if (!private)
		return NULL;

	if (!private)
		return NULL;

	mutex_lock(&private->process_private_mutex);

	if (test_bit(KGSL_PROCESS_INIT, &private->priv))
		goto done;

	private->mem_rb = RB_ROOT;
	idr_init(&private->mem_idr);

	if ((!private->pagetable) && kgsl_mmu_enabled()) {
		unsigned long pt_name;

		pt_name = task_tgid_nr(current);
		private->pagetable =
			kgsl_mmu_getpagetable(pt_name);
		if (private->pagetable == NULL)
			goto error;
	}

	if (kgsl_process_init_sysfs(private))
		goto error;	
	
	if (kgsl_process_init_debugfs(private))
		goto error;

	set_bit(KGSL_PROCESS_INIT, &private->priv);

done:
	mutex_unlock(&private->process_private_mutex);
	return private;

error:
	mutex_unlock(&private->process_private_mutex);
	kgsl_process_private_put(private);
	return NULL;
}


static int kgsl_release(struct inode *inodep, struct file *filep)
{
	int result = 0;
	struct kgsl_device_private *dev_priv = filep->private_data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_device *device = dev_priv->device;
	struct kgsl_context *context;
	struct kgsl_mem_entry *entry;
	int next = 0;

	filep->private_data = NULL;

	mutex_lock(&device->mutex);
	kgsl_active_count_get(device);

	while (1) {
		read_lock(&device->context_lock);
		context = idr_get_next(&device->context_idr, &next);
		read_unlock(&device->context_lock);

		if (context == NULL)
			break;

		if (context->dev_priv == dev_priv)
			kgsl_context_detach(context);

		next = next + 1;
	}
	next = 0;
	while (1) {
		spin_lock(&private->mem_lock);
		entry = idr_get_next(&private->mem_idr, &next);
		spin_unlock(&private->mem_lock);
		if (entry == NULL)
			break;
		/*
		 * If the free pending flag is not set it means that user space
		 * did not free it's reference to this entry, in that case
		 * free a reference to this entry, other references are from
		 * within kgsl so they will be freed eventually by kgsl
		 */
		if (entry->dev_priv == dev_priv && !entry->pending_free) {
			entry->pending_free = 1;
			kgsl_mem_entry_put(entry);
		}
		next = next + 1;
	}
	/*
	 * Clean up any to-be-freed entries that belong to this
	 * process and this device. This is done after the context
	 * are destroyed to avoid possibly freeing memory while
	 * it is still in use by the GPU.
	 */
	kgsl_cancel_events(device, dev_priv);

	device->open_count--;
	if (device->open_count == 0) {
		BUG_ON(device->active_cnt > 1);
		result = device->ftbl->stop(device);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_INIT);
		/*
		 * active_cnt special case: we just stopped the device,
		 * so no need to use kgsl_active_count_put()
		 */
		device->active_cnt--;
	} else {
		kgsl_active_count_put(device);
	}
	mutex_unlock(&device->mutex);
	kfree(dev_priv);

	kgsl_process_private_put(private);

	pm_runtime_put(device->parentdev);
	return result;
}

static int kgsl_open(struct inode *inodep, struct file *filep)
{
	int result;
	struct kgsl_device_private *dev_priv;
	struct kgsl_device *device;
	unsigned int minor = iminor(inodep);

	device = kgsl_get_minor(minor);
	BUG_ON(device == NULL);

	if (filep->f_flags & O_EXCL) {
		KGSL_DRV_ERR(device, "O_EXCL not allowed\n");
		return -EBUSY;
	}

	result = pm_runtime_get_sync(device->parentdev);
	if (result < 0) {
		KGSL_DRV_ERR(device,
			"Runtime PM: Unable to wake up the device, rc = %d\n",
			result);
		return result;
	}
	result = 0;

	dev_priv = kzalloc(sizeof(struct kgsl_device_private), GFP_KERNEL);
	if (dev_priv == NULL) {
		KGSL_DRV_ERR(device, "kzalloc failed(%d)\n",
			sizeof(struct kgsl_device_private));
		result = -ENOMEM;
		goto err_pmruntime;
	}

	dev_priv->device = device;
	filep->private_data = dev_priv;

	mutex_lock(&device->mutex);

	if (device->open_count == 0) {
		/*
		 * active_cnt special case: we are starting up for the first
		 * time, so use this sequence instead of the kgsl_pwrctrl_wake()
		 * which will be called by kgsl_active_count_get().
		 */
		device->active_cnt++;
		kgsl_sharedmem_set(&device->memstore, 0, 0,
				device->memstore.size);

		result = device->ftbl->init(device);
		if (result)
			goto err_freedevpriv;

		result = device->ftbl->start(device);
		if (result)
			goto err_freedevpriv;
		/*
		 * Make sure the gates are open, so they don't block until
		 * we start suspend or FT.
		 */
		complete_all(&device->ft_gate);
		complete_all(&device->hwaccess_gate);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_ACTIVE);
		kgsl_active_count_put(device);
	}
	device->open_count++;
	mutex_unlock(&device->mutex);

	/*
	 * Get file (per process) private struct. This must be done
	 * after the first start so that the global pagetable mappings
	 * are set up before we create the per-process pagetable.
	 */
	dev_priv->process_priv = kgsl_get_process_private(device);
	if (dev_priv->process_priv ==  NULL) {
		result = -ENOMEM;
		goto err_stop;
	}

	KGSL_DRV_INFO(device, "Initialized %s: mmu=%s pagetable_count=%d\n",
		device->name, kgsl_mmu_enabled() ? "on" : "off",
		kgsl_pagetable_count);

	return result;

err_stop:
	mutex_lock(&device->mutex);
	device->open_count--;
	if (device->open_count == 0) {
		/* make sure power is on to stop the device */
		kgsl_pwrctrl_enable(device);
		result = device->ftbl->stop(device);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_INIT);
	}
err_freedevpriv:
	/* only the first open takes an active count */
	if (device->open_count == 0)
		device->active_cnt--;
	mutex_unlock(&device->mutex);
	filep->private_data = NULL;
	kfree(dev_priv);
err_pmruntime:
	pm_runtime_put(device->parentdev);
	return result;
}

/**
 * kgsl_sharedmem_find_region() - Find a gpu memory allocation
 *
 * @private: private data for the process to check.
 * @gpuaddr: start address of the region
 * @size: size of the region
 *
 * Find a gpu allocation. Caller must kgsl_mem_entry_put()
 * the returned entry when finished using it.
 */
struct kgsl_mem_entry * __must_check
kgsl_sharedmem_find_region(struct kgsl_process_private *private,
	unsigned int gpuaddr, size_t size)
{
	struct rb_node *node;

	if (!kgsl_mmu_gpuaddr_in_range(gpuaddr))
		return NULL;

	spin_lock(&private->mem_lock);
	node = private->mem_rb.rb_node;
	while (node != NULL) {
		struct kgsl_mem_entry *entry;

		entry = rb_entry(node, struct kgsl_mem_entry, node);

		if (kgsl_gpuaddr_in_memdesc(&entry->memdesc, gpuaddr, size)) {
			if (!kgsl_mem_entry_get(entry))
				break;
			spin_unlock(&private->mem_lock);
			return entry;
		}
		if (gpuaddr < entry->memdesc.gpuaddr)
			node = node->rb_left;
		else if (gpuaddr >=
			(entry->memdesc.gpuaddr + entry->memdesc.size))
			node = node->rb_right;
		else {
			spin_unlock(&private->mem_lock);
			return NULL;
		}
	}
	spin_unlock(&private->mem_lock);

	return NULL;
}
EXPORT_SYMBOL(kgsl_sharedmem_find_region);

/**
 * kgsl_sharedmem_find() - Find a gpu memory allocation
 *
 * @private: private data for the process to check.
 * @gpuaddr: start address of the region
 *
 * Find a gpu allocation. Caller must kgsl_mem_entry_put()
 * the returned entry when finished using it.
 */
static inline struct kgsl_mem_entry * __must_check
kgsl_sharedmem_find(struct kgsl_process_private *private, unsigned int gpuaddr)
{
	return kgsl_sharedmem_find_region(private, gpuaddr, 1);
}

/**
 * kgsl_sharedmem_region_empty() - Check if an addression region is empty
 *
 * @private: private data for the process to check.
 * @gpuaddr: start address of the region
 * @size: length of the region.
 *
 * Checks that there are no existing allocations within an address
 * region. This function should be called with processes spin lock
 * held.
 */
static int
kgsl_sharedmem_region_empty(struct kgsl_process_private *private,
	unsigned int gpuaddr, size_t size)
{
	int result = 1;
	unsigned int gpuaddr_end = gpuaddr + size;

	struct rb_node *node;

	assert_spin_locked(&private->mem_lock);

	if (!kgsl_mmu_gpuaddr_in_range(gpuaddr))
		return 0;

	/* don't overflow */
	if (gpuaddr_end < gpuaddr)
		return 0;

	node = private->mem_rb.rb_node;
	while (node != NULL) {
		struct kgsl_mem_entry *entry;
		unsigned int memdesc_start, memdesc_end;

		entry = rb_entry(node, struct kgsl_mem_entry, node);

		memdesc_start = entry->memdesc.gpuaddr;
		memdesc_end = memdesc_start
				+ kgsl_memdesc_mmapsize(&entry->memdesc);

		if (gpuaddr_end <= memdesc_start)
			node = node->rb_left;
		else if (memdesc_end <= gpuaddr)
			node = node->rb_right;
		else {
			result = 0;
			break;
		}
	}
	return result;
}

/**
 * kgsl_sharedmem_find_id() - find a memory entry by id
 * @process: the owning process
 * @id: id to find
 *
 * @returns - the mem_entry or NULL
 *
 * Caller must kgsl_mem_entry_put() the returned entry, when finished using
 * it.
 */
static inline struct kgsl_mem_entry * __must_check
kgsl_sharedmem_find_id(struct kgsl_process_private *process, unsigned int id)
{
	int result = 0;
	struct kgsl_mem_entry *entry;

	rcu_read_lock();
	entry = idr_find(&process->mem_idr, id);
	if (entry)
		result = kgsl_mem_entry_get(entry);
	rcu_read_unlock();

	if (!result)
		return NULL;
	return entry;
}

/**
 * kgsl_mem_entry_set_pend() - Set the pending free flag of a memory entry
 * @entry - The memory entry
 *
 * @returns - true if pending flag was 0 else false
 *
 * This function will set the pending free flag if it is previously unset. Used
 * to prevent race condition between ioctls calling free/freememontimestamp
 * on the same entry. Whichever thread set's the flag first will do the free.
 */
static inline bool kgsl_mem_entry_set_pend(struct kgsl_mem_entry *entry)
{
	bool ret = false;

	if (entry == NULL)
		return false;

	spin_lock(&entry->priv->mem_lock);
	if (!entry->pending_free) {
		entry->pending_free = 1;
		ret = true;
	}
	spin_unlock(&entry->priv->mem_lock);
	return ret;
}

/*call all ioctl sub functions with driver locked*/
static long kgsl_ioctl_device_getproperty(struct kgsl_device_private *dev_priv,
					  unsigned int cmd, void *data)
{
	int result = 0;
	struct kgsl_device_getproperty *param = data;

	switch (param->type) {
	case KGSL_PROP_VERSION:
	{
		struct kgsl_version version;
		if (param->sizebytes != sizeof(version)) {
			result = -EINVAL;
			break;
		}

		version.drv_major = KGSL_VERSION_MAJOR;
		version.drv_minor = KGSL_VERSION_MINOR;
		version.dev_major = dev_priv->device->ver_major;
		version.dev_minor = dev_priv->device->ver_minor;

		if (copy_to_user(param->value, &version, sizeof(version)))
			result = -EFAULT;

		break;
	}
	case KGSL_PROP_GPU_RESET_STAT:
	{
		/* Return reset status of given context and clear it */
		uint32_t id;
		struct kgsl_context *context;

		if (param->sizebytes != sizeof(unsigned int)) {
			result = -EINVAL;
			break;
		}
		/* We expect the value passed in to contain the context id */
		if (copy_from_user(&id, param->value,
			sizeof(unsigned int))) {
			result = -EFAULT;
			break;
		}
		context = kgsl_context_get_owner(dev_priv, id);
		if (!context) {
			result = -EINVAL;
			break;
		}
		/*
		 * Copy the reset status to value which also serves as
		 * the out parameter
		 */
		if (copy_to_user(param->value, &(context->reset_status),
			sizeof(unsigned int)))
			result = -EFAULT;
		else {
			/* Clear reset status once its been queried */
			context->reset_status = KGSL_CTX_STAT_NO_ERROR;
		}

		kgsl_context_put(context);
		break;
	}
	default:
		result = dev_priv->device->ftbl->getproperty(
					dev_priv->device, param->type,
					param->value, param->sizebytes);
	}


	return result;
}

static long kgsl_ioctl_device_setproperty(struct kgsl_device_private *dev_priv,
					  unsigned int cmd, void *data)
{
	int result = 0;
	/* The getproperty struct is reused for setproperty too */
	struct kgsl_device_getproperty *param = data;

	if (dev_priv->device->ftbl->setproperty)
		result = dev_priv->device->ftbl->setproperty(
			dev_priv->device, param->type,
			param->value, param->sizebytes);

	return result;
}

static long _device_waittimestamp(struct kgsl_device_private *dev_priv,
		struct kgsl_context *context,
		unsigned int timestamp,
		unsigned int timeout)
{
	int result = 0;
	struct kgsl_device *device = dev_priv->device;
	unsigned int context_id = context ? context->id : KGSL_MEMSTORE_GLOBAL;

	trace_kgsl_waittimestamp_entry(device, context_id,
				       kgsl_readtimestamp(device, context,
							KGSL_TIMESTAMP_RETIRED),
				       timestamp, timeout);

	result = device->ftbl->waittimestamp(dev_priv->device,
					context, timestamp, timeout);

	trace_kgsl_waittimestamp_exit(device,
				      kgsl_readtimestamp(device, context,
							KGSL_TIMESTAMP_RETIRED),
				      result);

	return result;
}

static long kgsl_ioctl_device_waittimestamp(struct kgsl_device_private
						*dev_priv, unsigned int cmd,
						void *data)
{
	struct kgsl_device_waittimestamp *param = data;

	return _device_waittimestamp(dev_priv, NULL,
			param->timestamp, param->timeout);
}

static long kgsl_ioctl_device_waittimestamp_ctxtid(struct kgsl_device_private
						*dev_priv, unsigned int cmd,
						void *data)
{
	struct kgsl_device_waittimestamp_ctxtid *param = data;
	struct kgsl_context *context;
	long result = -EINVAL;

	context = kgsl_context_get_owner(dev_priv, param->context_id);

	if (context)
		result = _device_waittimestamp(dev_priv, context,
			param->timestamp, param->timeout);

	kgsl_context_put(context);
	return result;
}

static long kgsl_ioctl_rb_issueibcmds(struct kgsl_device_private *dev_priv,
				      unsigned int cmd, void *data)
{
	int result = 0;
	struct kgsl_ringbuffer_issueibcmds *param = data;
	struct kgsl_ibdesc *ibdesc;
	struct kgsl_context *context;

	context = kgsl_context_get_owner(dev_priv, param->drawctxt_id);
	if (context == NULL) {
		result = -EINVAL;
		goto done;
	}

	if (param->flags & KGSL_CONTEXT_SUBMIT_IB_LIST) {
		KGSL_DRV_INFO(dev_priv->device,
			"Using IB list mode for ib submission, numibs: %d\n",
			param->numibs);
		if (!param->numibs) {
			KGSL_DRV_ERR(dev_priv->device,
				"Invalid numibs as parameter: %d\n",
				 param->numibs);
			result = -EINVAL;
			goto done;
		}

		/*
		 * Put a reasonable upper limit on the number of IBs that can be
		 * submitted
		 */

		if (param->numibs > 10000) {
			KGSL_DRV_ERR(dev_priv->device,
				"Too many IBs submitted. count: %d max 10000\n",
				param->numibs);
			result = -EINVAL;
			goto done;
		}

		ibdesc = kzalloc(sizeof(struct kgsl_ibdesc) * param->numibs,
					GFP_KERNEL);
		if (!ibdesc) {
			KGSL_MEM_ERR(dev_priv->device,
				"kzalloc(%d) failed\n",
				sizeof(struct kgsl_ibdesc) * param->numibs);
			result = -ENOMEM;
			goto done;
		}

		if (copy_from_user(ibdesc, (void *)param->ibdesc_addr,
				sizeof(struct kgsl_ibdesc) * param->numibs)) {
			result = -EFAULT;
			KGSL_DRV_ERR(dev_priv->device,
				"copy_from_user failed\n");
			goto free_ibdesc;
		}
	} else {
		KGSL_DRV_INFO(dev_priv->device,
			"Using single IB submission mode for ib submission\n");
		/* If user space driver is still using the old mode of
		 * submitting single ib then we need to support that as well */
		ibdesc = kzalloc(sizeof(struct kgsl_ibdesc), GFP_KERNEL);
		if (!ibdesc) {
			KGSL_MEM_ERR(dev_priv->device,
				"kzalloc(%d) failed\n",
				sizeof(struct kgsl_ibdesc));
			result = -ENOMEM;
			goto done;
		}
		ibdesc[0].gpuaddr = param->ibdesc_addr;
		ibdesc[0].sizedwords = param->numibs;
		param->numibs = 1;
	}

	result = dev_priv->device->ftbl->issueibcmds(dev_priv,
					     context,
					     ibdesc,
					     param->numibs,
					     &param->timestamp,
					     param->flags);

free_ibdesc:
	kfree(ibdesc);
done:
	kgsl_context_put(context);
	return result;
}

static long _cmdstream_readtimestamp(struct kgsl_device_private *dev_priv,
		struct kgsl_context *context, unsigned int type,
		unsigned int *timestamp)
{
	*timestamp = kgsl_readtimestamp(dev_priv->device, context, type);

	trace_kgsl_readtimestamp(dev_priv->device,
			context ? context->id : KGSL_MEMSTORE_GLOBAL,
			type, *timestamp);

	return 0;
}

static long kgsl_ioctl_cmdstream_readtimestamp(struct kgsl_device_private
						*dev_priv, unsigned int cmd,
						void *data)
{
	struct kgsl_cmdstream_readtimestamp *param = data;

	return _cmdstream_readtimestamp(dev_priv, NULL,
			param->type, &param->timestamp);
}

static long kgsl_ioctl_cmdstream_readtimestamp_ctxtid(struct kgsl_device_private
						*dev_priv, unsigned int cmd,
						void *data)
{
	struct kgsl_cmdstream_readtimestamp_ctxtid *param = data;
	struct kgsl_context *context;
	long result = -EINVAL;

	context = kgsl_context_get_owner(dev_priv, param->context_id);

	if (context)
		result = _cmdstream_readtimestamp(dev_priv, context,
			param->type, &param->timestamp);

	kgsl_context_put(context);
	return result;
}

static void kgsl_freemem_event_cb(struct kgsl_device *device,
	void *priv, u32 id, u32 timestamp, u32 type)
{
	struct kgsl_mem_entry *entry = priv;

	/* Free the memory for all event types */
	trace_kgsl_mem_timestamp_free(device, entry, id, timestamp, 0);
	kgsl_mem_entry_put(entry);
}

static long _cmdstream_freememontimestamp(struct kgsl_device_private *dev_priv,
		unsigned int gpuaddr, struct kgsl_context *context,
		unsigned int timestamp, unsigned int type)
{
	int result = 0;
	struct kgsl_mem_entry *entry = NULL;
	struct kgsl_device *device = dev_priv->device;
	unsigned int context_id = context ? context->id : KGSL_MEMSTORE_GLOBAL;

	entry = kgsl_sharedmem_find(dev_priv->process_priv, gpuaddr);

	if (!entry) {
		KGSL_DRV_ERR(dev_priv->device,
				"invalid gpuaddr %08x\n", gpuaddr);
		return -EINVAL;
	}
	if (!kgsl_mem_entry_set_pend(entry)) {
		KGSL_DRV_WARN(dev_priv->device,
		"Cannot set pending bit for gpuaddr %08x\n", gpuaddr);
		kgsl_mem_entry_put(entry);
		return -EBUSY;
	}
	trace_kgsl_mem_timestamp_queue(device, entry, context_id,
				       kgsl_readtimestamp(device, context,
						  KGSL_TIMESTAMP_RETIRED),
				       timestamp);
	result = kgsl_add_event(dev_priv->device, context_id, timestamp,
				kgsl_freemem_event_cb, entry, dev_priv);
	kgsl_mem_entry_put(entry);
	return result;
}

static long kgsl_ioctl_cmdstream_freememontimestamp(struct kgsl_device_private
						    *dev_priv, unsigned int cmd,
						    void *data)
{
	struct kgsl_cmdstream_freememontimestamp *param = data;

	return _cmdstream_freememontimestamp(dev_priv, param->gpuaddr,
			NULL, param->timestamp, param->type);
}

static long kgsl_ioctl_cmdstream_freememontimestamp_ctxtid(
						struct kgsl_device_private
						*dev_priv, unsigned int cmd,
						void *data)
{
	struct kgsl_cmdstream_freememontimestamp_ctxtid *param = data;
	struct kgsl_context *context;
	long result = -EINVAL;

	context = kgsl_context_get_owner(dev_priv, param->context_id);
	if (context)
		result = _cmdstream_freememontimestamp(dev_priv, param->gpuaddr,
			context, param->timestamp, param->type);
	kgsl_context_put(context);
	return result;
}

static long kgsl_ioctl_drawctxt_create(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	int result = 0;
	struct kgsl_drawctxt_create *param = data;
	struct kgsl_context *context = NULL;

	context = kgsl_create_context(dev_priv);

	if (IS_ERR(context)) {
		result = PTR_ERR(context);
		goto done;
	}

	if (dev_priv->device->ftbl->drawctxt_create) {
		result = dev_priv->device->ftbl->drawctxt_create(
			dev_priv->device, dev_priv->process_priv->pagetable,
			context, &param->flags);
		if (result)
			goto done;
	}
	trace_kgsl_context_create(dev_priv->device, context, param->flags);
	param->drawctxt_id = context->id;
done:
	if (result && !IS_ERR(context))
		kgsl_context_detach(context);

	return result;
}

static long kgsl_ioctl_drawctxt_destroy(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_drawctxt_destroy *param = data;
	struct kgsl_context *context;
	long result = -EINVAL;

	context = kgsl_context_get_owner(dev_priv, param->drawctxt_id);

	if (context) {
		kgsl_context_detach(context);
		result = 0;
	}

	kgsl_context_put(context);
	return result;
}

static long _sharedmem_free_entry(struct kgsl_mem_entry *entry)
{
	if (!kgsl_mem_entry_set_pend(entry)) {
		kgsl_mem_entry_put(entry);
		return -EBUSY;
	}

	trace_kgsl_mem_free(entry);

	kgsl_memfree_add(entry->priv->pid, entry->memdesc.gpuaddr,
		entry->memdesc.size, entry->memdesc.flags);

	/*
	 * First kgsl_mem_entry_put is for the reference that we took in
	 * this function when calling kgsl_sharedmem_find, second one is
	 * to free the memory since this is a free ioctl
	 */
	kgsl_mem_entry_put(entry);
	kgsl_mem_entry_put(entry);

	return 0;
}

static long kgsl_ioctl_sharedmem_free(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_sharedmem_free *param = data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;

	entry = kgsl_sharedmem_find(private, param->gpuaddr);
	if (!entry) {
		KGSL_MEM_INFO(dev_priv->device, "invalid gpuaddr %08x\n",
				param->gpuaddr);
		return -EINVAL;
	}

	return _sharedmem_free_entry(entry);
}

static long kgsl_ioctl_gpumem_free_id(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_gpumem_free_id *param = data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;

	entry = kgsl_sharedmem_find_id(private, param->id);

	if (!entry) {
		KGSL_MEM_INFO(dev_priv->device, "invalid id %d\n", param->id);
		return -EINVAL;
	}

	return _sharedmem_free_entry(entry);
}

static struct vm_area_struct *kgsl_get_vma_from_start_addr(unsigned int addr)
{
	struct vm_area_struct *vma;

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, addr);
	up_read(&current->mm->mmap_sem);
	if (!vma)
		KGSL_CORE_ERR("find_vma(%x) failed\n", addr);

	return vma;
}

static inline int _check_region(unsigned long start, unsigned long size,
				uint64_t len)
{
	uint64_t end = ((uint64_t) start) + size;
	return (end > len);
}

static int kgsl_get_phys_file(int fd, unsigned long *start, unsigned long *len,
			      unsigned long *vstart, struct file **filep)
{
	struct file *fbfile;
	int ret = 0;
	dev_t rdev;
	struct fb_info *info;

	*filep = NULL;
#ifdef CONFIG_ANDROID_PMEM
	if (!get_pmem_file(fd, start, vstart, len, filep))
		return 0;
#endif

	fbfile = fget(fd);
	if (fbfile == NULL) {
		KGSL_CORE_ERR("fget_light failed\n");
		return -1;
	}

	rdev = fbfile->f_dentry->d_inode->i_rdev;
	info = MAJOR(rdev) == FB_MAJOR ? registered_fb[MINOR(rdev)] : NULL;
	if (info) {
		*start = info->fix.smem_start;
		*len = info->fix.smem_len;
		*vstart = (unsigned long)__va(info->fix.smem_start);
		ret = 0;
	} else {
		KGSL_CORE_ERR("framebuffer minor %d not found\n",
			      MINOR(rdev));
		ret = -1;
	}

	fput(fbfile);

	return ret;
}

static int kgsl_setup_phys_file(struct kgsl_mem_entry *entry,
				struct kgsl_pagetable *pagetable,
				unsigned int fd, unsigned int offset,
				size_t size)
{
	int ret;
	unsigned long phys, virt, len;
	struct file *filep;

	ret = kgsl_get_phys_file(fd, &phys, &len, &virt, &filep);
	if (ret)
		return ret;

	ret = -ERANGE;

	if (phys == 0) {
		KGSL_CORE_ERR("kgsl_get_phys_file returned phys=0\n");
		goto err;
	}

	/* Make sure the length of the region, the offset and the desired
	 * size are all page aligned or bail
	 */
	if ((len & ~PAGE_MASK) ||
		(offset & ~PAGE_MASK) ||
		(size & ~PAGE_MASK)) {
		KGSL_CORE_ERR("length %lu, offset %u or size %u "
				"is not page aligned\n",
				len, offset, size);
		goto err;
	}

	/* The size or offset can never be greater than the PMEM length */
	if (offset >= len || size > len) {
		KGSL_CORE_ERR("offset %u or size %u "
				"exceeds pmem length %lu\n",
				offset, size, len);
		goto err;
	}

	/* If size is 0, then adjust it to default to the size of the region
	 * minus the offset.  If size isn't zero, then make sure that it will
	 * fit inside of the region.
	 */
	if (size == 0)
		size = len - offset;

	else if (_check_region(offset, size, len))
		goto err;

	entry->priv_data = filep;

	entry->memdesc.pagetable = pagetable;
	entry->memdesc.size = size;
	entry->memdesc.physaddr = phys + offset;
	entry->memdesc.hostptr = (void *) (virt + offset);
	/* USE_CPU_MAP is not impemented for PMEM. */
	entry->memdesc.flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;

	ret = memdesc_sg_phys(&entry->memdesc, phys + offset, size);
	if (ret)
		goto err;

	return 0;
err:
#ifdef CONFIG_ANDROID_PMEM
	put_pmem_file(filep);
#endif
	return ret;
}

static int memdesc_sg_virt(struct kgsl_memdesc *memdesc,
	unsigned long paddr, int size)
{
	int i;
	int sglen = PAGE_ALIGN(size) / PAGE_SIZE;

	memdesc->sg = kgsl_sg_alloc(sglen);

	if (memdesc->sg == NULL)
		return -ENOMEM;

	memdesc->sglen = sglen;
	memdesc->sglen_alloc = sglen;

	sg_init_table(memdesc->sg, sglen);

	spin_lock(&current->mm->page_table_lock);

	for (i = 0; i < sglen; i++, paddr += PAGE_SIZE) {
		struct page *page;
		pmd_t *ppmd;
		pte_t *ppte;
		pgd_t *ppgd = pgd_offset(current->mm, paddr);

		if (pgd_none(*ppgd) || pgd_bad(*ppgd))
			goto err;

		ppmd = pmd_offset(pud_offset(ppgd, paddr), paddr);
		if (pmd_none(*ppmd) || pmd_bad(*ppmd))
			goto err;

		ppte = pte_offset_map(ppmd, paddr);
		if (ppte == NULL)
			goto err;

		page = pfn_to_page(pte_pfn(*ppte));
		if (!page)
			goto err;

		sg_set_page(&memdesc->sg[i], page, PAGE_SIZE, 0);
		pte_unmap(ppte);
	}

	spin_unlock(&current->mm->page_table_lock);

	return 0;

err:
	spin_unlock(&current->mm->page_table_lock);
	kgsl_sg_free(memdesc->sg,  sglen);
	memdesc->sg = NULL;

	return -EINVAL;
}

static int kgsl_setup_useraddr(struct kgsl_mem_entry *entry,
			      struct kgsl_pagetable *pagetable,
			      unsigned long useraddr, unsigned int offset,
			      size_t size)
{
	struct vm_area_struct *vma;
	unsigned int len;

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, useraddr);
	up_read(&current->mm->mmap_sem);

	if (!vma) {
		KGSL_CORE_ERR("find_vma(%lx) failed\n", useraddr);
		return -EINVAL;
	}

	/* We don't necessarily start at vma->vm_start */
	len = vma->vm_end - useraddr;

	if (offset >= len)
		return -EINVAL;

	if (!KGSL_IS_PAGE_ALIGNED(useraddr) ||
	    !KGSL_IS_PAGE_ALIGNED(len)) {
		KGSL_CORE_ERR("bad alignment: start(%lx) len(%u)\n",
			      useraddr, len);
		return -EINVAL;
	}

	if (size == 0)
		size = len;

	/* Adjust the size of the region to account for the offset */
	size += offset & ~PAGE_MASK;

	size = ALIGN(size, PAGE_SIZE);

	if (_check_region(offset & PAGE_MASK, size, len)) {
		KGSL_CORE_ERR("Offset (%ld) + size (%d) is larger"
			      "than region length %d\n",
			      offset & PAGE_MASK, size, len);
		return -EINVAL;
	}

	entry->memdesc.pagetable = pagetable;
	entry->memdesc.size = size;
	entry->memdesc.useraddr = useraddr + (offset & PAGE_MASK);
	if (kgsl_memdesc_use_cpu_map(&entry->memdesc))
		entry->memdesc.gpuaddr = entry->memdesc.useraddr;

	return memdesc_sg_virt(&entry->memdesc, entry->memdesc.useraddr,
				size);
}

#ifdef CONFIG_ASHMEM
static int kgsl_setup_ashmem(struct kgsl_mem_entry *entry,
			     struct kgsl_pagetable *pagetable,
			     int fd, unsigned long useraddr, size_t size)
{
	int ret;
	struct vm_area_struct *vma;
	struct file *filep, *vmfile;
	unsigned long len;

	vma = kgsl_get_vma_from_start_addr(useraddr);
	if (vma == NULL)
		return -EINVAL;

	if (vma->vm_pgoff || vma->vm_start != useraddr) {
		KGSL_CORE_ERR("Invalid vma region\n");
		return -EINVAL;
	}

	len = vma->vm_end - vma->vm_start;

	if (size == 0)
		size = len;

	if (size != len) {
		KGSL_CORE_ERR("Invalid size %d for vma region %lx\n",
			      size, useraddr);
		return -EINVAL;
	}

	ret = get_ashmem_file(fd, &filep, &vmfile, &len);

	if (ret) {
		KGSL_CORE_ERR("get_ashmem_file failed\n");
		return ret;
	}

	if (vmfile != vma->vm_file) {
		KGSL_CORE_ERR("ashmem shmem file does not match vma\n");
		ret = -EINVAL;
		goto err;
	}

	entry->priv_data = filep;
	entry->memdesc.pagetable = pagetable;
	entry->memdesc.size = ALIGN(size, PAGE_SIZE);
	entry->memdesc.useraddr = useraddr;
	if (kgsl_memdesc_use_cpu_map(&entry->memdesc))
		entry->memdesc.gpuaddr = entry->memdesc.useraddr;

	ret = memdesc_sg_virt(&entry->memdesc, useraddr, size);
	if (ret)
		goto err;

	return 0;

err:
	put_ashmem_file(filep);
	return ret;
}
#else
static int kgsl_setup_ashmem(struct kgsl_mem_entry *entry,
			     struct kgsl_pagetable *pagetable,
			     int fd, unsigned long useraddr, size_t size)
{
	return -EINVAL;
}
#endif

static int kgsl_setup_ion(struct kgsl_mem_entry *entry,
		struct kgsl_pagetable *pagetable, int fd)
{
	struct ion_handle *handle;
	struct scatterlist *s;
	struct sg_table *sg_table;

	if (IS_ERR_OR_NULL(kgsl_ion_client))
		return -ENODEV;

	handle = ion_import_dma_buf(kgsl_ion_client, fd);
	if (IS_ERR(handle))
		return PTR_ERR(handle);
	else if (!handle)
		return -EINVAL;

	entry->memtype = KGSL_MEM_ENTRY_ION;
	entry->priv_data = handle;
	entry->memdesc.pagetable = pagetable;
	entry->memdesc.size = 0;
	/* USE_CPU_MAP is not impemented for ION. */
	entry->memdesc.flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;

	sg_table = ion_sg_table(kgsl_ion_client, handle);

	if (IS_ERR_OR_NULL(sg_table))
		goto err;

	entry->memdesc.sg = sg_table->sgl;

	/* Calculate the size of the memdesc from the sglist */

	entry->memdesc.sglen = 0;

	for (s = entry->memdesc.sg; s != NULL; s = sg_next(s)) {
		entry->memdesc.size += s->length;
		entry->memdesc.sglen++;
	}

	return 0;
err:
	ion_free(kgsl_ion_client, handle);
	return -ENOMEM;
}

static inline int
can_use_cpu_map(void)
{
	return (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_IOMMU
		&& kgsl_mmu_is_perprocess());
}

static long kgsl_ioctl_map_user_mem(struct kgsl_device_private *dev_priv,
				     unsigned int cmd, void *data)
{
	int result = -EINVAL;
	struct kgsl_map_user_mem *param = data;
	struct kgsl_mem_entry *entry = NULL;
	struct kgsl_process_private *private = dev_priv->process_priv;
	enum kgsl_user_mem_type memtype;

	entry = kgsl_mem_entry_create();

	if (entry == NULL)
		return -ENOMEM;

	if (_IOC_SIZE(cmd) == sizeof(struct kgsl_sharedmem_from_pmem))
		memtype = KGSL_USER_MEM_TYPE_PMEM;
	else
		memtype = param->memtype;

	/*
	 * Mask off unknown flags from userspace. This way the caller can
	 * check if a flag is supported by looking at the returned flags.
	 * Note: CACHEMODE is ignored for this call. Caching should be
	 * determined by type of allocation being mapped.
	 */
	param->flags &= KGSL_MEMFLAGS_GPUREADONLY
			| KGSL_MEMTYPE_MASK
			| KGSL_MEMALIGN_MASK
			| KGSL_MEMFLAGS_USE_CPU_MAP;

	entry->memdesc.flags = param->flags;
	if (!can_use_cpu_map())
		entry->memdesc.flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;

	if (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_IOMMU)
		entry->memdesc.priv |= KGSL_MEMDESC_GUARD_PAGE;

	switch (memtype) {
	case KGSL_USER_MEM_TYPE_PMEM:
		if (param->fd == 0 || param->len == 0)
			break;

		result = kgsl_setup_phys_file(entry, private->pagetable,
					      param->fd, param->offset,
					      param->len);
		entry->memtype = KGSL_MEM_ENTRY_PMEM;
		break;

	case KGSL_USER_MEM_TYPE_ADDR:
		KGSL_DEV_ERR_ONCE(dev_priv->device, "User mem type "
				"KGSL_USER_MEM_TYPE_ADDR is deprecated\n");
		if (!kgsl_mmu_enabled()) {
			KGSL_DRV_ERR(dev_priv->device,
				"Cannot map paged memory with the "
				"MMU disabled\n");
			break;
		}

		if (param->hostptr == 0)
			break;

		result = kgsl_setup_useraddr(entry, private->pagetable,
					    param->hostptr,
					    param->offset, param->len);
		entry->memtype = KGSL_MEM_ENTRY_USER;
		break;

	case KGSL_USER_MEM_TYPE_ASHMEM:
		if (!kgsl_mmu_enabled()) {
			KGSL_DRV_ERR(dev_priv->device,
				"Cannot map paged memory with the "
				"MMU disabled\n");
			break;
		}

		if (param->hostptr == 0)
			break;

		result = kgsl_setup_ashmem(entry, private->pagetable,
					   param->fd, param->hostptr,
					   param->len);

		entry->memtype = KGSL_MEM_ENTRY_ASHMEM;
		break;
	case KGSL_USER_MEM_TYPE_ION:
		result = kgsl_setup_ion(entry, private->pagetable,
			param->fd);
		break;
	default:
		KGSL_CORE_ERR("Invalid memory type: %x\n", memtype);
		break;
	}

	if (result)
		goto error;

	if (entry->memdesc.size >= SZ_1M)
		kgsl_memdesc_set_align(&entry->memdesc, ilog2(SZ_1M));
	else if (entry->memdesc.size >= SZ_64K)
		kgsl_memdesc_set_align(&entry->memdesc, ilog2(SZ_64));

	/* echo back flags */
	param->flags = entry->memdesc.flags;

	result = kgsl_mem_entry_attach_process(entry, dev_priv);
	if (result)
		goto error_attach;

	/* Adjust the returned value for a non 4k aligned offset */
	param->gpuaddr = entry->memdesc.gpuaddr + (param->offset & ~PAGE_MASK);

	KGSL_STATS_ADD(param->len, kgsl_driver.stats.mapped,
		kgsl_driver.stats.mapped_max);

	kgsl_process_add_stats(private, entry->memtype, param->len);

	trace_kgsl_mem_map(entry, param->fd);

	return result;

error_attach:
	switch (entry->memtype) {
	case KGSL_MEM_ENTRY_PMEM:
	case KGSL_MEM_ENTRY_ASHMEM:
		if (entry->priv_data)
			fput(entry->priv_data);
		break;
	case KGSL_MEM_ENTRY_ION:
		ion_free(kgsl_ion_client, entry->priv_data);
		break;
	default:
		break;
	}
error:
	kfree(entry);
	return result;
}

static int _kgsl_gpumem_sync_cache(struct kgsl_mem_entry *entry, int op)
{
	int ret = 0;
	int cacheop;
	int mode;

	/*
	 * Flush is defined as (clean | invalidate).  If both bits are set, then
	 * do a flush, otherwise check for the individual bits and clean or inv
	 * as requested
	 */

	if ((op & KGSL_GPUMEM_CACHE_FLUSH) == KGSL_GPUMEM_CACHE_FLUSH)
		cacheop = KGSL_CACHE_OP_FLUSH;
	else if (op & KGSL_GPUMEM_CACHE_CLEAN)
		cacheop = KGSL_CACHE_OP_CLEAN;
	else if (op & KGSL_GPUMEM_CACHE_INV)
		cacheop = KGSL_CACHE_OP_INV;
	else {
		ret = -EINVAL;
		goto done;
	}

	mode = kgsl_memdesc_get_cachemode(&entry->memdesc);
	if (mode != KGSL_CACHEMODE_UNCACHED
		&& mode != KGSL_CACHEMODE_WRITECOMBINE)
		kgsl_cache_range_op(&entry->memdesc, cacheop);

done:
	return ret;
}

/* New cache sync function - supports both directions (clean and invalidate) */

static long
kgsl_ioctl_gpumem_sync_cache(struct kgsl_device_private *dev_priv,
	unsigned int cmd, void *data)
{
	struct kgsl_gpumem_sync_cache *param = data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;
	long ret;

	if (param->id != 0) {
		entry = kgsl_sharedmem_find_id(private, param->id);
		if (entry == NULL) {
			KGSL_MEM_INFO(dev_priv->device, "can't find id %d\n",
					param->id);
			return -EINVAL;
		}
	} else if (param->gpuaddr != 0) {
		entry = kgsl_sharedmem_find(private, param->gpuaddr);
		if (entry == NULL) {
			KGSL_MEM_INFO(dev_priv->device,
					"can't find gpuaddr %x\n",
					param->gpuaddr);
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	ret = _kgsl_gpumem_sync_cache(entry, param->op);
	kgsl_mem_entry_put(entry);
	return ret;
}

/* Legacy cache function, does a flush (clean  + invalidate) */

static long
kgsl_ioctl_sharedmem_flush_cache(struct kgsl_device_private *dev_priv,
				 unsigned int cmd, void *data)
{
	struct kgsl_sharedmem_free *param = data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;
	long ret;

	entry = kgsl_sharedmem_find(private, param->gpuaddr);
	if (entry == NULL) {
		KGSL_MEM_INFO(dev_priv->device,
				"can't find gpuaddr %x\n",
				param->gpuaddr);
		return -EINVAL;
	}
	ret = _kgsl_gpumem_sync_cache(entry, KGSL_GPUMEM_CACHE_FLUSH);
	kgsl_mem_entry_put(entry);
	return ret;
}

/*
 * The common parts of kgsl_ioctl_gpumem_alloc and kgsl_ioctl_gpumem_alloc_id.
 */
int
_gpumem_alloc(struct kgsl_device_private *dev_priv,
		struct kgsl_mem_entry **ret_entry,
		unsigned int size, unsigned int flags)
{
	int result;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry;

	/*
	 * Mask off unknown flags from userspace. This way the caller can
	 * check if a flag is supported by looking at the returned flags.
	 */
	flags &= KGSL_MEMFLAGS_GPUREADONLY
		| KGSL_CACHEMODE_MASK
		| KGSL_MEMTYPE_MASK
		| KGSL_MEMALIGN_MASK
		| KGSL_MEMFLAGS_USE_CPU_MAP;

	entry = kgsl_mem_entry_create();
	if (entry == NULL)
		return -ENOMEM;

	if (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_IOMMU)
		entry->memdesc.priv |= KGSL_MEMDESC_GUARD_PAGE;

	result = kgsl_allocate_user(&entry->memdesc, private->pagetable, size,
				    flags);
	if (result != 0)
		goto err;

	entry->memtype = KGSL_MEM_ENTRY_KERNEL;

	*ret_entry = entry;
	return result;
err:
	kfree(entry);
	*ret_entry = NULL;
	return result;
}

static long
kgsl_ioctl_gpumem_alloc(struct kgsl_device_private *dev_priv,
			unsigned int cmd, void *data)
{
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_gpumem_alloc *param = data;
	struct kgsl_mem_entry *entry = NULL;
	int result;

	param->flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;
	result = _gpumem_alloc(dev_priv, &entry, param->size, param->flags);
	if (result)
		return result;

	result = kgsl_mem_entry_attach_process(entry, dev_priv);
	if (result != 0)
		goto err;

	kgsl_process_add_stats(private, entry->memtype, param->size);
	trace_kgsl_mem_alloc(entry);

	param->gpuaddr = entry->memdesc.gpuaddr;
	param->size = entry->memdesc.size;
	param->flags = entry->memdesc.flags;
	return result;
err:
	kgsl_sharedmem_free(&entry->memdesc);
	kfree(entry);
	return result;
}

static long
kgsl_ioctl_gpumem_alloc_id(struct kgsl_device_private *dev_priv,
			unsigned int cmd, void *data)
{
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_gpumem_alloc_id *param = data;
	struct kgsl_mem_entry *entry = NULL;
	int result;

	if (!can_use_cpu_map())
		param->flags &= ~KGSL_MEMFLAGS_USE_CPU_MAP;

	result = _gpumem_alloc(dev_priv, &entry, param->size, param->flags);
	if (result != 0)
		goto err;

	result = kgsl_mem_entry_attach_process(entry, dev_priv);
	if (result != 0)
		goto err;

	kgsl_process_add_stats(private, entry->memtype, param->size);
	trace_kgsl_mem_alloc(entry);

	param->id = entry->id;
	param->flags = entry->memdesc.flags;
	param->size = entry->memdesc.size;
	param->mmapsize = kgsl_memdesc_mmapsize(&entry->memdesc);
	param->gpuaddr = entry->memdesc.gpuaddr;
	return result;
err:
	if (entry)
		kgsl_sharedmem_free(&entry->memdesc);
	kfree(entry);
	return result;
}

static long
kgsl_ioctl_gpumem_get_info(struct kgsl_device_private *dev_priv,
			unsigned int cmd, void *data)
{
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_gpumem_get_info *param = data;
	struct kgsl_mem_entry *entry = NULL;
	int result = 0;

	if (param->id != 0) {
		entry = kgsl_sharedmem_find_id(private, param->id);
		if (entry == NULL) {
			KGSL_MEM_INFO(dev_priv->device, "can't find id %d\n",
					param->id);
			return -EINVAL;
		}
	} else if (param->gpuaddr != 0) {
		entry = kgsl_sharedmem_find(private, param->gpuaddr);
		if (entry == NULL) {
			KGSL_MEM_INFO(dev_priv->device,
					"can't find gpuaddr %lx\n",
					param->gpuaddr);
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	param->gpuaddr = entry->memdesc.gpuaddr;
	param->id = entry->id;
	param->flags = entry->memdesc.flags;
	param->size = entry->memdesc.size;
	param->mmapsize = kgsl_memdesc_mmapsize(&entry->memdesc);
	param->useraddr = entry->memdesc.useraddr;

	kgsl_mem_entry_put(entry);
	return result;
}

static long kgsl_ioctl_cff_syncmem(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	int result = 0;
	struct kgsl_cff_syncmem *param = data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;

	entry = kgsl_sharedmem_find_region(private, param->gpuaddr, param->len);
	if (!entry)
		return -EINVAL;

	kgsl_cffdump_syncmem(dev_priv, &entry->memdesc, param->gpuaddr,
			     param->len, true);

	kgsl_mem_entry_put(entry);
	return result;
}

static long kgsl_ioctl_cff_user_event(struct kgsl_device_private *dev_priv,
		unsigned int cmd, void *data)
{
	int result = 0;
	struct kgsl_cff_user_event *param = data;

	kgsl_cffdump_user_event(param->cff_opcode, param->op1, param->op2,
			param->op3, param->op4, param->op5);

	return result;
}

#ifdef CONFIG_GENLOCK
struct kgsl_genlock_event_priv {
	struct genlock_handle *handle;
	struct genlock *lock;
};

/**
 * kgsl_genlock_event_cb - Event callback for a genlock timestamp event
 * @device - The KGSL device that expired the timestamp
 * @priv - private data for the event
 * @context_id - the context id that goes with the timestamp
 * @timestamp - the timestamp that triggered the event
 *
 * Release a genlock lock following the expiration of a timestamp
 */

static void kgsl_genlock_event_cb(struct kgsl_device *device,
	void *priv, u32 context_id, u32 timestamp, u32 type)
{
	struct kgsl_genlock_event_priv *ev = priv;
	int ret;

	/* Signal the lock for every event type */
	ret = genlock_lock(ev->handle, GENLOCK_UNLOCK, 0, 0);
	if (ret)
		KGSL_CORE_ERR("Error while unlocking genlock: %d\n", ret);

	genlock_put_handle(ev->handle);

	kfree(ev);
}

/**
 * kgsl_add_genlock-event - Create a new genlock event
 * @device - KGSL device to create the event on
 * @timestamp - Timestamp to trigger the event
 * @data - User space buffer containing struct kgsl_genlock_event_priv
 * @len - length of the userspace buffer
 * @owner - driver instance that owns this event
 * @returns 0 on success or error code on error
 *
 * Attack to a genlock handle and register an event to release the
 * genlock lock when the timestamp expires
 */

static int kgsl_add_genlock_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	struct kgsl_genlock_event_priv *event;
	struct kgsl_timestamp_event_genlock priv;
	int ret;

	if (len !=  sizeof(priv))
		return -EINVAL;

	if (copy_from_user(&priv, data, sizeof(priv)))
		return -EFAULT;

	event = kzalloc(sizeof(*event), GFP_KERNEL);

	if (event == NULL)
		return -ENOMEM;

	event->handle = genlock_get_handle_fd(priv.handle);

	if (IS_ERR(event->handle)) {
		int ret = PTR_ERR(event->handle);
		kfree(event);
		return ret;
	}

	ret = kgsl_add_event(device, context_id, timestamp,
			kgsl_genlock_event_cb, event, owner);
	if (ret)
		kfree(event);

	return ret;
}
#else
static long kgsl_add_genlock_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	return -EINVAL;
}
#endif

/**
 * kgsl_ioctl_timestamp_event - Register a new timestamp event from userspace
 * @dev_priv - pointer to the private device structure
 * @cmd - the ioctl cmd passed from kgsl_ioctl
 * @data - the user data buffer from kgsl_ioctl
 * @returns 0 on success or error code on failure
 */

static long kgsl_ioctl_timestamp_event(struct kgsl_device_private *dev_priv,
		unsigned int cmd, void *data)
{
	struct kgsl_timestamp_event *param = data;
	int ret;

	switch (param->type) {
	case KGSL_TIMESTAMP_EVENT_GENLOCK:
		ret = kgsl_add_genlock_event(dev_priv->device,
			param->context_id, param->timestamp, param->priv,
			param->len, dev_priv);
		break;
	case KGSL_TIMESTAMP_EVENT_FENCE:
		ret = kgsl_add_fence_event(dev_priv->device,
			param->context_id, param->timestamp, param->priv,
			param->len, dev_priv);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

typedef long (*kgsl_ioctl_func_t)(struct kgsl_device_private *,
	unsigned int, void *);

#define KGSL_IOCTL_FUNC(_cmd, _func, _flags) \
	[_IOC_NR((_cmd))] = \
		{ .cmd = (_cmd), .func = (_func), .flags = (_flags) }

#define KGSL_IOCTL_LOCK		BIT(0)
#define KGSL_IOCTL_WAKE		BIT(1)

static const struct {
	unsigned int cmd;
	kgsl_ioctl_func_t func;
	int flags;
} kgsl_ioctl_funcs[] = {
	KGSL_IOCTL_FUNC(IOCTL_KGSL_DEVICE_GETPROPERTY,
			kgsl_ioctl_device_getproperty,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_DEVICE_WAITTIMESTAMP,
			kgsl_ioctl_device_waittimestamp,
			KGSL_IOCTL_LOCK | KGSL_IOCTL_WAKE),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID,
			kgsl_ioctl_device_waittimestamp_ctxtid,
			KGSL_IOCTL_LOCK | KGSL_IOCTL_WAKE),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS,
			kgsl_ioctl_rb_issueibcmds,
			KGSL_IOCTL_LOCK | KGSL_IOCTL_WAKE),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CMDSTREAM_READTIMESTAMP,
			kgsl_ioctl_cmdstream_readtimestamp,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CMDSTREAM_READTIMESTAMP_CTXTID,
			kgsl_ioctl_cmdstream_readtimestamp_ctxtid,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP,
			kgsl_ioctl_cmdstream_freememontimestamp,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP_CTXTID,
			kgsl_ioctl_cmdstream_freememontimestamp_ctxtid,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_DRAWCTXT_CREATE,
			kgsl_ioctl_drawctxt_create,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_DRAWCTXT_DESTROY,
			kgsl_ioctl_drawctxt_destroy,
			KGSL_IOCTL_LOCK | KGSL_IOCTL_WAKE),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_MAP_USER_MEM,
			kgsl_ioctl_map_user_mem, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_SHAREDMEM_FROM_PMEM,
			kgsl_ioctl_map_user_mem, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_SHAREDMEM_FREE,
			kgsl_ioctl_sharedmem_free, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_SHAREDMEM_FLUSH_CACHE,
			kgsl_ioctl_sharedmem_flush_cache, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_GPUMEM_ALLOC,
			kgsl_ioctl_gpumem_alloc, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CFF_SYNCMEM,
			kgsl_ioctl_cff_syncmem, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_CFF_USER_EVENT,
			kgsl_ioctl_cff_user_event, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_TIMESTAMP_EVENT,
			kgsl_ioctl_timestamp_event,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_SETPROPERTY,
			kgsl_ioctl_device_setproperty,
			KGSL_IOCTL_LOCK),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_GPUMEM_ALLOC_ID,
			kgsl_ioctl_gpumem_alloc_id, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_GPUMEM_FREE_ID,
			kgsl_ioctl_gpumem_free_id, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_GPUMEM_GET_INFO,
			kgsl_ioctl_gpumem_get_info, 0),
	KGSL_IOCTL_FUNC(IOCTL_KGSL_GPUMEM_SYNC_CACHE,
			kgsl_ioctl_gpumem_sync_cache, 0),
};

static long kgsl_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct kgsl_device_private *dev_priv = filep->private_data;
	unsigned int nr;
	kgsl_ioctl_func_t func;
	int lock, ret, use_hw;
	char ustack[64];
	void *uptr = NULL;

	BUG_ON(dev_priv == NULL);

	/* Workaround for an previously incorrectly defined ioctl code.
	   This helps ensure binary compatability */

	if (cmd == IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP_OLD)
		cmd = IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP;
	else if (cmd == IOCTL_KGSL_CMDSTREAM_READTIMESTAMP_OLD)
		cmd = IOCTL_KGSL_CMDSTREAM_READTIMESTAMP;
	else if (cmd == IOCTL_KGSL_TIMESTAMP_EVENT_OLD)
		cmd = IOCTL_KGSL_TIMESTAMP_EVENT;

	nr = _IOC_NR(cmd);

	if (cmd & (IOC_IN | IOC_OUT)) {
		if (_IOC_SIZE(cmd) < sizeof(ustack))
			uptr = ustack;
		else {
			uptr = kzalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (uptr == NULL) {
				KGSL_MEM_ERR(dev_priv->device,
					"kzalloc(%d) failed\n", _IOC_SIZE(cmd));
				ret = -ENOMEM;
				goto done;
			}
		}

		if (cmd & IOC_IN) {
			if (copy_from_user(uptr, (void __user *) arg,
				_IOC_SIZE(cmd))) {
				ret = -EFAULT;
				goto done;
			}
		} else
			memset(uptr, 0, _IOC_SIZE(cmd));
	}

	if (nr < ARRAY_SIZE(kgsl_ioctl_funcs) &&
		kgsl_ioctl_funcs[nr].func != NULL) {

		/*
		 * Make sure that nobody tried to send us a malformed ioctl code
		 * with a valid NR but bogus flags
		 */

		if (kgsl_ioctl_funcs[nr].cmd != cmd) {
			KGSL_DRV_ERR(dev_priv->device,
				"Malformed ioctl code %08x\n", cmd);
			ret = -ENOIOCTLCMD;
			goto done;
		}

		func = kgsl_ioctl_funcs[nr].func;
		lock = kgsl_ioctl_funcs[nr].flags & KGSL_IOCTL_LOCK;
		use_hw = kgsl_ioctl_funcs[nr].flags & KGSL_IOCTL_WAKE;
	} else {
		func = dev_priv->device->ftbl->ioctl;
		if (!func) {
			KGSL_DRV_INFO(dev_priv->device,
				      "invalid ioctl code %08x\n", cmd);
			ret = -ENOIOCTLCMD;
			goto done;
		}
		lock = 1;
		use_hw = 1;
	}

	if (lock) {
		mutex_lock(&dev_priv->device->mutex);
		if (use_hw) {
			ret = kgsl_active_count_get(dev_priv->device);
			if (ret < 0) {
				use_hw = 0;
				goto unlock;
			}
		}
	}

	ret = func(dev_priv, cmd, uptr);

unlock:
	if (lock) {
		if (use_hw)
			kgsl_active_count_put(dev_priv->device);
		mutex_unlock(&dev_priv->device->mutex);
	}

	if (ret == 0 && (cmd & IOC_OUT)) {
		if (copy_to_user((void __user *) arg, uptr, _IOC_SIZE(cmd)))
			ret = -EFAULT;
	}

done:
	if (_IOC_SIZE(cmd) >= sizeof(ustack))
		kfree(uptr);

	return ret;
}

static int
kgsl_mmap_memstore(struct kgsl_device *device, struct vm_area_struct *vma)
{
	struct kgsl_memdesc *memdesc = &device->memstore;
	int result;
	unsigned int vma_size = vma->vm_end - vma->vm_start;

	/* The memstore can only be mapped as read only */

	if (vma->vm_flags & VM_WRITE)
		return -EPERM;

	if (memdesc->size  !=  vma_size) {
		KGSL_MEM_ERR(device, "memstore bad size: %d should be %d\n",
			     vma_size, memdesc->size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	result = remap_pfn_range(vma, vma->vm_start,
				device->memstore.physaddr >> PAGE_SHIFT,
				 vma_size, vma->vm_page_prot);
	if (result != 0)
		KGSL_MEM_ERR(device, "remap_pfn_range failed: %d\n",
			     result);

	return result;
}

/*
 * kgsl_gpumem_vm_open is called whenever a vma region is copied or split.
 * Increase the refcount to make sure that the accounting stays correct
 */

static void kgsl_gpumem_vm_open(struct vm_area_struct *vma)
{
	struct kgsl_mem_entry *entry = vma->vm_private_data;
	if (!kgsl_mem_entry_get(entry))
		vma->vm_private_data = NULL;
}

static int
kgsl_gpumem_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct kgsl_mem_entry *entry = vma->vm_private_data;

	if (!entry)
		return VM_FAULT_SIGBUS;
	if (!entry->memdesc.ops || !entry->memdesc.ops->vmfault)
		return VM_FAULT_SIGBUS;

	return entry->memdesc.ops->vmfault(&entry->memdesc, vma, vmf);
}

static void
kgsl_gpumem_vm_close(struct vm_area_struct *vma)
{
	struct kgsl_mem_entry *entry  = vma->vm_private_data;

	if (!entry)
		return;

	entry->memdesc.useraddr = 0;
	kgsl_mem_entry_put(entry);
}

static struct vm_operations_struct kgsl_gpumem_vm_ops = {
	.open  = kgsl_gpumem_vm_open,
	.fault = kgsl_gpumem_vm_fault,
	.close = kgsl_gpumem_vm_close,
};

static int
get_mmap_entry(struct kgsl_process_private *private,
		struct kgsl_mem_entry **out_entry, unsigned long pgoff,
		unsigned long len)
{
	int ret = 0;
	struct kgsl_mem_entry *entry;

	entry = kgsl_sharedmem_find_id(private, pgoff);
	if (entry == NULL) {
		entry = kgsl_sharedmem_find(private, pgoff << PAGE_SHIFT);
	}

	if (!entry)
		return -EINVAL;

	if (!entry->memdesc.ops ||
		!entry->memdesc.ops->vmflags ||
		!entry->memdesc.ops->vmfault) {
		ret = -EINVAL;
		goto err_put;
	}

	if (entry->memdesc.useraddr != 0) {
		ret = -EBUSY;
		goto err_put;
	}

	if (len != kgsl_memdesc_mmapsize(&entry->memdesc)) {
		ret = -ERANGE;
		goto err_put;
	}

	*out_entry = entry;
	return 0;
err_put:
	kgsl_mem_entry_put(entry);
	return ret;
}

static inline bool
mmap_range_valid(unsigned long addr, unsigned long len)
{
	return ((ULONG_MAX - addr) > len) && ((addr + len) < TASK_SIZE);
}

static unsigned long
kgsl_get_unmapped_area(struct file *file, unsigned long addr,
			unsigned long len, unsigned long pgoff,
			unsigned long flags)
{
	unsigned long ret = 0, orig_len = len;
	unsigned long vma_offset = pgoff << PAGE_SHIFT;
	struct kgsl_device_private *dev_priv = file->private_data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_device *device = dev_priv->device;
	struct kgsl_mem_entry *entry = NULL;
	unsigned int align;
	unsigned int retry = 0;

	if (vma_offset == device->memstore.gpuaddr)
		return get_unmapped_area(NULL, addr, len, pgoff, flags);

	ret = get_mmap_entry(private, &entry, pgoff, len);
	if (ret)
		return ret;

	if (!kgsl_memdesc_use_cpu_map(&entry->memdesc) || (flags & MAP_FIXED)) {
		/*
		 * If we're not going to use the same mapping on the gpu,
		 * any address is fine.
		 * For MAP_FIXED, hopefully the caller knows what they're doing,
		 * but we may fail in mmap() if there is already something
		 * at the virtual address chosen.
		 */
		ret = get_unmapped_area(NULL, addr, len, pgoff, flags);
		goto put;
	}
	if (entry->memdesc.gpuaddr != 0) {
		KGSL_MEM_INFO(device,
				"pgoff %lx already mapped to gpuaddr %x\n",
				pgoff, entry->memdesc.gpuaddr);
		ret = -EBUSY;
		goto put;
	}

	align = kgsl_memdesc_get_align(&entry->memdesc);
	if (align >= ilog2(SZ_1M))
		align = ilog2(SZ_1M);
	else if (align >= ilog2(SZ_64K))
		align = ilog2(SZ_64K);
	else if (align <= PAGE_SHIFT)
		align = 0;

	if (align)
		len += 1 << align;

	if (!mmap_range_valid(addr, len))
		addr = 0;
	do {
		ret = get_unmapped_area(NULL, addr, len, pgoff, flags);
		if (IS_ERR_VALUE(ret)) {
			/*
			 * If we are really fragmented, there may not be room
			 * for the alignment padding, so try again without it.
			 */
			if (!retry && (ret == (unsigned long)-ENOMEM)
				&& (align > PAGE_SHIFT)) {
				align = 0;
				addr = 0;
				len = orig_len;
				retry = 1;
				continue;
			}
			break;
		}
		if (align)
			ret = ALIGN(ret, (1 << align));

		/*make sure there isn't a GPU only mapping at this address */
		spin_lock(&private->mem_lock);
		if (kgsl_sharedmem_region_empty(private, ret, orig_len)) {
			int ret_val;
			/*
			 * We found a free memory map, claim it here with
			 * memory lock held
			 */
			entry->memdesc.gpuaddr = ret;
			/* This should never fail */
			ret_val = kgsl_mem_entry_track_gpuaddr(private, entry);
			spin_unlock(&private->mem_lock);
			BUG_ON(ret_val);
			/* map cannot be called with lock held */
			ret_val = kgsl_mmu_map(private->pagetable,
						&entry->memdesc);
			if (ret_val) {
				spin_lock(&private->mem_lock);
				kgsl_mem_entry_untrack_gpuaddr(private, entry);
				spin_unlock(&private->mem_lock);
				ret = ret_val;
			}
			break;
		}
		spin_unlock(&private->mem_lock);

		trace_kgsl_mem_unmapped_area_collision(entry, addr, orig_len,
							ret);

		/*
		 * If we collided, bump the hint address so that
		 * get_umapped_area knows to look somewhere else.
		 */
		addr = (addr == 0) ? ret + orig_len : addr + orig_len;

		/*
		 * The addr hint can be set by userspace to be near
		 * the end of the address space. Make sure we search
		 * the whole address space at least once by wrapping
		 * back around once.
		 */
		if (!retry && !mmap_range_valid(addr, len)) {
			addr = 0;
			retry = 1;
		} else {
			ret = -EBUSY;
		}
	} while (mmap_range_valid(addr, len));

	if (IS_ERR_VALUE(ret))
		KGSL_MEM_ERR(device,
				"pid %d pgoff %lx len %ld failed error %ld\n",
				private->pid, pgoff, len, ret);
put:
	kgsl_mem_entry_put(entry);
	return ret;
}

static int kgsl_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned int ret, cache;
	unsigned long vma_offset = vma->vm_pgoff << PAGE_SHIFT;
	struct kgsl_device_private *dev_priv = file->private_data;
	struct kgsl_process_private *private = dev_priv->process_priv;
	struct kgsl_mem_entry *entry = NULL;
	struct kgsl_device *device = dev_priv->device;

	/* Handle leagacy behavior for memstore */

	if (vma_offset == device->memstore.gpuaddr)
		return kgsl_mmap_memstore(device, vma);

	/*
	 * The reference count on the entry that we get from
	 * get_mmap_entry() will be held until kgsl_gpumem_vm_close().
	 */
	ret = get_mmap_entry(private, &entry, vma->vm_pgoff,
				vma->vm_end - vma->vm_start);
	if (ret)
		return ret;

	vma->vm_flags |= entry->memdesc.ops->vmflags(&entry->memdesc);

	vma->vm_private_data = entry;

	/* Determine user-side caching policy */

	cache = kgsl_memdesc_get_cachemode(&entry->memdesc);

	switch (cache) {
	case KGSL_CACHEMODE_UNCACHED:
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		break;
	case KGSL_CACHEMODE_WRITETHROUGH:
		vma->vm_page_prot = pgprot_writethroughcache(vma->vm_page_prot);
		break;
	case KGSL_CACHEMODE_WRITEBACK:
		vma->vm_page_prot = pgprot_writebackcache(vma->vm_page_prot);
		break;
	case KGSL_CACHEMODE_WRITECOMBINE:
	default:
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		break;
	}

	vma->vm_ops = &kgsl_gpumem_vm_ops;

	if (cache == KGSL_CACHEMODE_WRITEBACK
		|| cache == KGSL_CACHEMODE_WRITETHROUGH) {
		struct scatterlist *s;
		int i;
		int sglen = entry->memdesc.sglen;
		unsigned long addr = vma->vm_start;

		for_each_sg(entry->memdesc.sg, s, sglen, i) {
			int j;
			for (j = 0; j < (sg_dma_len(s) >> PAGE_SHIFT); j++) {
				struct page *page = sg_page(s);
				page = nth_page(page, j);
				vm_insert_page(vma, addr, page);
				addr += PAGE_SIZE;
			}
		}
	}

	vma->vm_file = file;

	entry->memdesc.useraddr = vma->vm_start;

	trace_kgsl_mem_mmap(entry);
	return 0;
}

static irqreturn_t kgsl_irq_handler(int irq, void *data)
{
	struct kgsl_device *device = data;

	return device->ftbl->irq_handler(device);

}

static const struct file_operations kgsl_fops = {
	.owner = THIS_MODULE,
	.release = kgsl_release,
	.open = kgsl_open,
	.mmap = kgsl_mmap,
	.get_unmapped_area = kgsl_get_unmapped_area,
	.unlocked_ioctl = kgsl_ioctl,
};

struct kgsl_driver kgsl_driver  = {
	.process_mutex = __MUTEX_INITIALIZER(kgsl_driver.process_mutex),
	.ptlock = __SPIN_LOCK_UNLOCKED(kgsl_driver.ptlock),
	.devlock = __MUTEX_INITIALIZER(kgsl_driver.devlock),
};
EXPORT_SYMBOL(kgsl_driver);

static void _unregister_device(struct kgsl_device *device)
{
	int minor;

	mutex_lock(&kgsl_driver.devlock);
	for (minor = 0; minor < KGSL_DEVICE_MAX; minor++) {
		if (device == kgsl_driver.devp[minor])
			break;
	}
	if (minor != KGSL_DEVICE_MAX) {
		device_destroy(kgsl_driver.class,
				MKDEV(MAJOR(kgsl_driver.major), minor));
		kgsl_driver.devp[minor] = NULL;
	}
	mutex_unlock(&kgsl_driver.devlock);
}

static int _register_device(struct kgsl_device *device)
{
	int minor, ret;
	dev_t dev;

	/* Find a minor for the device */

	mutex_lock(&kgsl_driver.devlock);
	for (minor = 0; minor < KGSL_DEVICE_MAX; minor++) {
		if (kgsl_driver.devp[minor] == NULL) {
			kgsl_driver.devp[minor] = device;
			break;
		}
	}
	mutex_unlock(&kgsl_driver.devlock);

	if (minor == KGSL_DEVICE_MAX) {
		KGSL_CORE_ERR("minor devices exhausted\n");
		return -ENODEV;
	}

	/* Create the device */
	dev = MKDEV(MAJOR(kgsl_driver.major), minor);
	device->dev = device_create(kgsl_driver.class,
				    device->parentdev,
				    dev, device,
				    device->name);

	if (IS_ERR(device->dev)) {
		mutex_lock(&kgsl_driver.devlock);
		kgsl_driver.devp[minor] = NULL;
		mutex_unlock(&kgsl_driver.devlock);
		ret = PTR_ERR(device->dev);
		KGSL_CORE_ERR("device_create(%s): %d\n", device->name, ret);
		return ret;
	}

	dev_set_drvdata(device->parentdev, device);
	return 0;
}

int kgsl_device_platform_probe(struct kgsl_device *device)
{
	int result;
	int status = -EINVAL;
	struct resource *res;
	struct platform_device *pdev =
		container_of(device->parentdev, struct platform_device, dev);

	status = _register_device(device);
	if (status)
		return status;

	/* Initialize logging first, so that failures below actually print. */
	kgsl_device_debugfs_init(device);

	status = kgsl_pwrctrl_init(device);
	if (status)
		goto error;

	kgsl_ion_client = msm_ion_client_create(UINT_MAX, KGSL_NAME);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   device->iomemname);
	if (res == NULL) {
		KGSL_DRV_ERR(device, "platform_get_resource_byname failed\n");
		status = -EINVAL;
		goto error_pwrctrl_close;
	}
	if (res->start == 0 || resource_size(res) == 0) {
		KGSL_DRV_ERR(device, "dev %d invalid register region\n",
			device->id);
		status = -EINVAL;
		goto error_pwrctrl_close;
	}

	device->reg_phys = res->start;
	device->reg_len = resource_size(res);

	if (!devm_request_mem_region(device->dev, device->reg_phys,
				device->reg_len, device->name)) {
		KGSL_DRV_ERR(device, "request_mem_region failed\n");
		status = -ENODEV;
		goto error_pwrctrl_close;
	}

	device->reg_virt = devm_ioremap(device->dev, device->reg_phys,
					device->reg_len);

	if (device->reg_virt == NULL) {
		KGSL_DRV_ERR(device, "ioremap failed\n");
		status = -ENODEV;
		goto error_pwrctrl_close;
	}
	/*acquire interrupt */
	device->pwrctrl.interrupt_num =
		platform_get_irq_byname(pdev, device->pwrctrl.irq_name);

	if (device->pwrctrl.interrupt_num <= 0) {
		KGSL_DRV_ERR(device, "platform_get_irq_byname failed: %d\n",
					 device->pwrctrl.interrupt_num);
		status = -EINVAL;
		goto error_pwrctrl_close;
	}

	status = devm_request_irq(device->dev, device->pwrctrl.interrupt_num,
				  kgsl_irq_handler, IRQF_TRIGGER_HIGH,
				  device->name, device);
	if (status) {
		KGSL_DRV_ERR(device, "request_irq(%d) failed: %d\n",
			      device->pwrctrl.interrupt_num, status);
		goto error_pwrctrl_close;
	}
	disable_irq(device->pwrctrl.interrupt_num);

	KGSL_DRV_INFO(device,
		"dev_id %d regs phys 0x%08lx size 0x%08x virt %p\n",
		device->id, device->reg_phys, device->reg_len,
		device->reg_virt);

	rwlock_init(&device->context_lock);

	result = kgsl_drm_init(pdev);
	if (result)
		goto error_pwrctrl_close;


	setup_timer(&device->idle_timer, kgsl_timer, (unsigned long) device);
	setup_timer(&device->hang_timer, hang_timer, (unsigned long) device);
	status = kgsl_create_device_workqueue(device);
	if (status)
		goto error_pwrctrl_close;

	status = kgsl_mmu_init(device);
	if (status != 0) {
		KGSL_DRV_ERR(device, "kgsl_mmu_init failed %d\n", status);
		goto error_dest_work_q;
	}

	status = kgsl_allocate_contiguous(&device->memstore,
		KGSL_MEMSTORE_SIZE);

	if (status != 0) {
		KGSL_DRV_ERR(device, "kgsl_allocate_contiguous failed %d\n",
				status);
		goto error_close_mmu;
	}

	pm_qos_add_request(&device->pm_qos_req_dma, PM_QOS_CPU_DMA_LATENCY,
				PM_QOS_DEFAULT_VALUE);

	/* Initalize the snapshot engine */
	kgsl_device_snapshot_init(device);

	/* Initialize common sysfs entries */
	kgsl_pwrctrl_init_sysfs(device);

	return 0;

error_close_mmu:
	kgsl_mmu_close(device);
error_dest_work_q:
	destroy_workqueue(device->work_queue);
	device->work_queue = NULL;
error_pwrctrl_close:
	kgsl_pwrctrl_close(device);
error:
	_unregister_device(device);
	return status;
}
EXPORT_SYMBOL(kgsl_device_platform_probe);

int kgsl_postmortem_dump(struct kgsl_device *device, int manual)
{
	bool saved_nap;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	BUG_ON(device == NULL);

	kgsl_cffdump_hang(device->id);

	/* For a manual dump, make sure that the system is idle */

	if (manual) {
		kgsl_active_count_wait(device);

		if (device->state == KGSL_STATE_ACTIVE)
			kgsl_idle(device);

	}

	if (device->pm_dump_enable) {
		KGSL_LOG_DUMP(device, "POWER: INTERVAL TIMEOUT = %08X ",
				pwr->interval_timeout);
		KGSL_LOG_DUMP(device, "POWER: NAP ALLOWED = %d | START_STOP_SLEEP_WAKE = %d\n",
				pwr->nap_allowed, pwr->strtstp_sleepwake);

		KGSL_LOG_DUMP(device, "GRP_CLK = %lu ",
				  kgsl_get_clkrate(pwr->grp_clks[0]));

	}

	/* Disable the idle timer so we don't get interrupted */
	del_timer_sync(&device->idle_timer);
	del_timer_sync(&device->hang_timer);

	/* Turn off napping to make sure we have the clocks full
	   attention through the following process */
	saved_nap = device->pwrctrl.nap_allowed;
	device->pwrctrl.nap_allowed = false;

	/* Force on the clocks */
	kgsl_pwrctrl_wake(device);

	/* Disable the irq */
	kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_OFF);

	/*Call the device specific postmortem dump function*/
	device->ftbl->postmortem_dump(device, manual);

	/* Restore nap mode */
	device->pwrctrl.nap_allowed = saved_nap;

	/* On a manual trigger, turn on the interrupts and put
	   the clocks to sleep.  They will recover themselves
	   on the next event.  For a hang, leave things as they
	   are until fault tolerance kicks in. */

	if (manual) {
		kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_ON);

		/* try to go into a sleep mode until the next event */
		kgsl_pwrctrl_request_state(device, KGSL_STATE_SLEEP);
		kgsl_pwrctrl_sleep(device);
	}

	return 0;
}
EXPORT_SYMBOL(kgsl_postmortem_dump);

void kgsl_device_platform_remove(struct kgsl_device *device)
{
	kgsl_device_snapshot_close(device);

	kgsl_pwrctrl_uninit_sysfs(device);

	pm_qos_remove_request(&device->pm_qos_req_dma);

	idr_destroy(&device->context_idr);

	kgsl_sharedmem_free(&device->memstore);

	kgsl_mmu_close(device);

	if (device->work_queue) {
		destroy_workqueue(device->work_queue);
		device->work_queue = NULL;
	}
	kgsl_pwrctrl_close(device);

	_unregister_device(device);
}
EXPORT_SYMBOL(kgsl_device_platform_remove);

static int __devinit
kgsl_ptdata_init(void)
{
	kgsl_driver.ptpool = kgsl_mmu_ptpool_init(kgsl_pagetable_count);

	if (!kgsl_driver.ptpool)
		return -ENOMEM;
	return 0;
}

static void kgsl_core_exit(void)
{
	kgsl_mmu_ptpool_destroy(kgsl_driver.ptpool);
	kgsl_driver.ptpool = NULL;

	kgsl_drm_exit();
	kgsl_cffdump_destroy();
	kgsl_core_debugfs_close();

	/*
	 * We call kgsl_sharedmem_uninit_sysfs() and device_unregister()
	 * only if kgsl_driver.virtdev has been populated.
	 * We check at least one member of kgsl_driver.virtdev to
	 * see if it is not NULL (and thus, has been populated).
	 */
	if (kgsl_driver.virtdev.class) {
		kgsl_sharedmem_uninit_sysfs();
		device_unregister(&kgsl_driver.virtdev);
	}

	if (kgsl_driver.class) {
		class_destroy(kgsl_driver.class);
		kgsl_driver.class = NULL;
	}

 	kgsl_memfree_exit();
	unregister_chrdev_region(kgsl_driver.major, KGSL_DEVICE_MAX);
}

static int __init kgsl_core_init(void)
{
	int result = 0;
	/* alloc major and minor device numbers */
	result = alloc_chrdev_region(&kgsl_driver.major, 0, KGSL_DEVICE_MAX,
				  KGSL_NAME);
	if (result < 0) {
		KGSL_CORE_ERR("alloc_chrdev_region failed err = %d\n", result);
		goto err;
	}

	cdev_init(&kgsl_driver.cdev, &kgsl_fops);
	kgsl_driver.cdev.owner = THIS_MODULE;
	kgsl_driver.cdev.ops = &kgsl_fops;
	result = cdev_add(&kgsl_driver.cdev, MKDEV(MAJOR(kgsl_driver.major), 0),
		       KGSL_DEVICE_MAX);

	if (result) {
		KGSL_CORE_ERR("kgsl: cdev_add() failed, dev_num= %d,"
			     " result= %d\n", kgsl_driver.major, result);
		goto err;
	}

	kgsl_driver.class = class_create(THIS_MODULE, KGSL_NAME);

	if (IS_ERR(kgsl_driver.class)) {
		result = PTR_ERR(kgsl_driver.class);
		KGSL_CORE_ERR("failed to create class %s", KGSL_NAME);
		goto err;
	}

	/* Make a virtual device for managing core related things
	   in sysfs */
	kgsl_driver.virtdev.class = kgsl_driver.class;
	dev_set_name(&kgsl_driver.virtdev, "kgsl");
	result = device_register(&kgsl_driver.virtdev);
	if (result) {
		KGSL_CORE_ERR("driver_register failed\n");
		goto err;
	}

	/* Make kobjects in the virtual device for storing statistics */

	kgsl_driver.ptkobj =
	  kobject_create_and_add("pagetables",
				 &kgsl_driver.virtdev.kobj);

	kgsl_driver.prockobj =
		kobject_create_and_add("proc",
				       &kgsl_driver.virtdev.kobj);

	kgsl_core_debugfs_init();

	kgsl_sharedmem_init_sysfs();
	kgsl_cffdump_init();

	INIT_LIST_HEAD(&kgsl_driver.process_list);

	INIT_LIST_HEAD(&kgsl_driver.pagetable_list);

	kgsl_mmu_set_mmutype(ksgl_mmu_type);

	if (KGSL_MMU_TYPE_GPU == kgsl_mmu_get_mmutype()) {
		result = kgsl_ptdata_init();
		if (result)
			goto err;
	}

	kgsl_memfree_init();

	return 0;

err:
	kgsl_core_exit();
	return result;
}

module_init(kgsl_core_init);
module_exit(kgsl_core_exit);

MODULE_AUTHOR("Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("MSM GPU driver");
MODULE_LICENSE("GPL");
