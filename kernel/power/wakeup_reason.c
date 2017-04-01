/*
 * kernel/power/wakeup_reason.c
 *
 * Logs the reasons which caused the kernel to resume from
 * the suspend mode.
 *
 * Copyright (C) 2014 Google, Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/wakeup_reason.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/debugfs.h>
#include <linux/slab.h>


#define MAX_WAKEUP_REASON_IRQS 32
static int irq_list[MAX_WAKEUP_REASON_IRQS];
static int irqcount;
static bool suspend_abort;
static char abort_reason[MAX_SUSPEND_ABORT_LEN];
static struct kobject *wakeup_reason;
static DEFINE_SPINLOCK(resume_reason_lock);

static ktime_t last_monotime; /* monotonic time before last suspend */
static ktime_t curr_monotime; /* monotonic time after last suspend */
static ktime_t last_stime; /* monotonic boottime offset before last suspend */
static ktime_t curr_stime; /* monotonic boottime offset after last suspend */
#if IS_ENABLED(CONFIG_SUSPEND_TIME)
static unsigned int time_in_suspend_bins[32];
#endif

static void init_wakeup_irq_node(struct wakeup_irq_node *p, int irq)
{
	p->irq = irq;
	p->desc = irq_to_desc(irq);
	p->child = NULL;
	p->parent = NULL;
	p->handled = false;
	INIT_LIST_HEAD(&p->siblings);
	INIT_LIST_HEAD(&p->next);
}

static struct wakeup_irq_node* alloc_irq_node(int irq)
{
	struct wakeup_irq_node *n;

	n = kmem_cache_alloc(wakeup_irq_nodes_cache, GFP_ATOMIC);
	if (!n) {
		pr_warning("Failed to log chained wakeup IRQ %d\n",
			irq);
		return NULL;
	}

	init_wakeup_irq_node(n, irq);
	return n;
}

static struct wakeup_irq_node *
search_siblings(struct wakeup_irq_node *root, int irq)
{
	bool found = false;
	struct wakeup_irq_node *n = NULL;
	BUG_ON(!root);

	if (root->irq == irq)
		return root;

	list_for_each_entry(n, &root->siblings, siblings) {
		if (n->irq == irq) {
			found = true;
			break;
		}
	}

	return found ? n : NULL;
}

static struct wakeup_irq_node *
add_to_siblings(struct wakeup_irq_node *root, int irq)
{
	struct wakeup_irq_node *n;
	if (root) {
		n = search_siblings(root, irq);
		if (n)
			return n;
	}
	n = alloc_irq_node(irq);

	if (n && root)
		list_add(&n->siblings, &root->siblings);
	return n;
}

#ifdef CONFIG_DEDUCE_WAKEUP_REASONS
static struct wakeup_irq_node* add_child(struct wakeup_irq_node *root, int irq)
{
	if (!root->child) {
		root->child = alloc_irq_node(irq);
		if (!root->child)
			return NULL;
		root->child->parent = root;
		return root->child;
	}

	return add_to_siblings(root->child, irq);
}

static struct wakeup_irq_node *find_first_sibling(struct wakeup_irq_node *node)
{
	struct wakeup_irq_node *n;
	if (node->parent)
		return node;
	list_for_each_entry(n, &node->siblings, siblings) {
		if (n->parent)
			return n;
	}
	return NULL;
}

static struct wakeup_irq_node *
get_base_node(struct wakeup_irq_node *node, unsigned depth)
{
	if (!node)
		return NULL;

	while (depth) {
		node = find_first_sibling(node);
		BUG_ON(!node);
		node = node->parent;
		depth--;
	}

	return node;
}
#endif /* CONFIG_DEDUCE_WAKEUP_REASONS */

static const struct list_head* get_wakeup_reasons_nosync(void);

static void print_wakeup_sources(void)
{
	struct wakeup_irq_node *n;
	const struct list_head *wakeups;

	if (suspend_abort) {
		pr_info("Abort: %s", abort_reason);
		return;
	}

	wakeups = get_wakeup_reasons_nosync();
	list_for_each_entry(n, wakeups, next) {
		if (n->desc && n->desc->action && n->desc->action->name)
			pr_info("Resume caused by IRQ %d, %s\n", n->irq,
				n->desc->action->name);
		else
			pr_info("Resume caused by IRQ %d\n", n->irq);
	}
}

static bool walk_irq_node_tree(struct wakeup_irq_node *root,
		bool (*visit)(struct wakeup_irq_node *, void *),
		void *cookie)
{
	struct wakeup_irq_node *n, *t;

	if (!root)
		return true;

	list_for_each_entry_safe(n, t, &root->siblings, siblings) {
		if (!walk_irq_node_tree(n->child, visit, cookie))
			return false;
		if (!visit(n, cookie))
			return false;
	}

	if (!walk_irq_node_tree(root->child, visit, cookie))
		return false;
	return visit(root, cookie);
}

#ifdef CONFIG_DEDUCE_WAKEUP_REASONS
static bool is_node_handled(struct wakeup_irq_node *n, void *_p)
{
	return n->handled;
}

static bool base_irq_nodes_done(void)
{
	return walk_irq_node_tree(base_irq_nodes, is_node_handled, NULL);
}
#endif

struct buf_cookie {
	char *buf;
	int buf_offset;
};

static bool print_leaf_node(struct wakeup_irq_node *n, void *_p)
{
	struct buf_cookie *b = _p;
	if (!n->child) {
		if (n->desc && n->desc->action && n->desc->action->name)
			b->buf_offset +=
				snprintf(b->buf + b->buf_offset,
					PAGE_SIZE - b->buf_offset,
					"%d %s\n",
					n->irq, n->desc->action->name);
		else
			b->buf_offset +=
				snprintf(b->buf + b->buf_offset,
					PAGE_SIZE - b->buf_offset,
					"%d\n",
					n->irq);
	}
	return true;
}

static ssize_t last_resume_reason_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	unsigned long flags;

	struct buf_cookie b = {
		.buf = buf,
		.buf_offset = 0
	};

	spin_lock_irqsave(&resume_reason_lock, flags);
	if (suspend_abort)
		b.buf_offset = snprintf(buf, PAGE_SIZE, "Abort: %s", abort_reason);
	else
		walk_irq_node_tree(base_irq_nodes, print_leaf_node, &b);
	spin_unlock_irqrestore(&resume_reason_lock, flags);

	return b.buf_offset;
}

static ssize_t last_suspend_time_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct timespec sleep_time;
	struct timespec total_time;
	struct timespec suspend_resume_time;

	/*
	 * total_time is calculated from monotonic bootoffsets because
	 * unlike CLOCK_MONOTONIC it include the time spent in suspend state.
	 */
	total_time = ktime_to_timespec(ktime_sub(curr_stime, last_stime));

	/*
	 * suspend_resume_time is calculated as monotonic (CLOCK_MONOTONIC)
	 * time interval before entering suspend and post suspend.
	 */
	suspend_resume_time = ktime_to_timespec(ktime_sub(curr_monotime, last_monotime));

	/* sleep_time = total_time - suspend_resume_time */
	sleep_time = timespec_sub(total_time, suspend_resume_time);

	/* Export suspend_resume_time and sleep_time in pair here. */
	return sprintf(buf, "%lu.%09lu %lu.%09lu\n",
				suspend_resume_time.tv_sec, suspend_resume_time.tv_nsec,
				sleep_time.tv_sec, sleep_time.tv_nsec);
}

static struct kobj_attribute resume_reason = __ATTR_RO(last_resume_reason);
static struct kobj_attribute suspend_time = __ATTR_RO(last_suspend_time);

static struct attribute *attrs[] = {
	&resume_reason.attr,
	&suspend_time.attr,
	NULL,
};
static struct attribute_group attr_group = {
	.attrs = attrs,
};

/*
 * logs all the wake up reasons to the kernel
 * stores the irqs to expose them to the userspace via sysfs
 */
void log_wakeup_reason(int irq)
{
	struct irq_desc *desc;
	desc = irq_to_desc(irq);
	if (desc && desc->action && desc->action->name)
		printk(KERN_INFO "Resume caused by IRQ %d, %s\n", irq,
				desc->action->name);
	else
		printk(KERN_INFO "Resume caused by IRQ %d\n", irq);

	spin_lock(&resume_reason_lock);
	if (irqcount == MAX_WAKEUP_REASON_IRQS) {
		spin_unlock(&resume_reason_lock);
		printk(KERN_WARNING "Resume caused by more than %d IRQs\n",
				MAX_WAKEUP_REASON_IRQS);
		return;
	}

	irq_list[irqcount++] = irq;
	spin_unlock(&resume_reason_lock);
}

int check_wakeup_reason(int irq)
{
	int irq_no;
	int ret = false;

	spin_lock(&resume_reason_lock);
	for (irq_no = 0; irq_no < irqcount; irq_no++)
		if (irq_list[irq_no] == irq) {
			ret = true;
			break;
	}
	spin_unlock(&resume_reason_lock);
	return ret;
}

void log_suspend_abort_reason(const char *fmt, ...)
{
	va_list args;

	spin_lock(&resume_reason_lock);

	//Suspend abort reason has already been logged.
	if (suspend_abort) {
		spin_unlock(&resume_reason_lock);
		return;
	}

	suspend_abort = true;
	va_start(args, fmt);
	vsnprintf(abort_reason, MAX_SUSPEND_ABORT_LEN, fmt, args);
	va_end(args);
	spin_unlock(&resume_reason_lock);
}

/* Detects a suspend and clears all the previous wake up reasons*/
static int wakeup_reason_pm_event(struct notifier_block *notifier,
		unsigned long pm_event, void *unused)
{
#if IS_ENABLED(CONFIG_SUSPEND_TIME)
	ktime_t temp;
	struct timespec suspend_time;
#endif

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		spin_lock(&resume_reason_lock);
		irqcount = 0;
		suspend_abort = false;
		spin_unlock(&resume_reason_lock);
		/* monotonic time since boot */
		last_monotime = ktime_get();
		/* monotonic time since boot including the time spent in suspend */
		last_stime = ktime_get_boottime();
		break;
	case PM_POST_SUSPEND:
		/* monotonic time since boot */
		curr_monotime = ktime_get();
		/* monotonic time since boot including the time spent in suspend */
		curr_stime = ktime_get_boottime();

#if IS_ENABLED(CONFIG_SUSPEND_TIME)
		temp = ktime_sub(ktime_sub(curr_stime, last_stime),
				ktime_sub(curr_monotime, last_monotime));
		suspend_time = ktime_to_timespec(temp);
		time_in_suspend_bins[fls(suspend_time.tv_sec)]++;
		pr_info("Suspended for %lu.%03lu seconds\n", suspend_time.tv_sec,
			suspend_time.tv_nsec / NSEC_PER_MSEC);
#endif
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block wakeup_reason_pm_notifier_block = {
	.notifier_call = wakeup_reason_pm_event,
};

#if IS_ENABLED(CONFIG_DEBUG_FS) && IS_ENABLED(CONFIG_SUSPEND_TIME)
static int suspend_time_debug_show(struct seq_file *s, void *data)
{
	int bin;
	seq_printf(s, "time (secs)  count\n");
	seq_printf(s, "------------------\n");
	for (bin = 0; bin < 32; bin++) {
		if (time_in_suspend_bins[bin] == 0)
			continue;
		seq_printf(s, "%4d - %4d %4u\n",
			bin ? 1 << (bin - 1) : 0, 1 << bin,
				time_in_suspend_bins[bin]);
	}
	return 0;
}

static int suspend_time_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, suspend_time_debug_show, NULL);
}

static const struct file_operations suspend_time_debug_fops = {
	.open		= suspend_time_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init suspend_time_debug_init(void)
{
	struct dentry *d;

	d = debugfs_create_file("suspend_time", 0755, NULL, NULL,
		&suspend_time_debug_fops);
	if (!d) {
		pr_err("Failed to create suspend_time debug file\n");
		return -ENOMEM;
	}

	return 0;
}

late_initcall(suspend_time_debug_init);
#endif

/* Initializes the sysfs parameter
 * registers the pm_event notifier
 */
int __init wakeup_reason_init(void)
{
	int retval;

	retval = register_pm_notifier(&wakeup_reason_pm_notifier_block);
	if (retval)
		printk(KERN_WARNING "[%s] failed to register PM notifier %d\n",
				__func__, retval);

	wakeup_reason = kobject_create_and_add("wakeup_reasons", kernel_kobj);
	if (!wakeup_reason) {
		printk(KERN_WARNING "[%s] failed to create a sysfs kobject\n",
				__func__);
		return 1;
	}
	retval = sysfs_create_group(wakeup_reason, &attr_group);
	if (retval) {
		kobject_put(wakeup_reason);
		printk(KERN_WARNING "[%s] failed to create a sysfs group %d\n",
				__func__, retval);
	}
	return 0;
}

late_initcall(wakeup_reason_init);
