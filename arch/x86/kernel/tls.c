#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/user.h>
#include <linux/regset.h>

#include <asm/uaccess.h>
#include <asm/desc.h>
#include <asm/ldt.h>
#include <asm/processor.h>
#include <asm/proto.h>
#include <asm/syscalls.h>

#include "tls.h"

/*
 * sys_alloc_thread_area: get a yet unused TLS descriptor index.
 */
static int get_free_idx(void)
{
	struct thread_struct *t = &current->thread;
	int idx;

	for (idx = 0; idx < GDT_ENTRY_TLS_ENTRIES; idx++)
		if (desc_empty(&t->tls_array[idx]))
			return idx + GDT_ENTRY_TLS_MIN;
	return -ESRCH;
}

static bool tls_desc_okay(const struct user_desc *info)
{
	/*
	 * For historical reasons (i.e. no one ever documented how any
	 * of the segmentation APIs work), user programs can and do
	 * assume that a struct user_desc that's all zeros except for
	 * entry_number means "no segment at all".  This never actually
	 * worked.  In fact, up to Linux 3.19, a struct user_desc like
	 * this would create a 16-bit read-write segment with base and
	 * limit both equal to zero.
	 *
	 * That was close enough to "no segment at all" until we
	 * hardened this function to disallow 16-bit TLS segments.  Fix
	 * it up by interpreting these zeroed segments the way that they
	 * were almost certainly intended to be interpreted.
	 *
	 * The correct way to ask for "no segment at all" is to specify
	 * a user_desc that satisfies LDT_empty.  To keep everything
	 * working, we accept both.
	 *
	 * Note that there's a similar kludge in modify_ldt -- look at
	 * the distinction between modes 1 and 0x11.
	 */
	if (LDT_empty(info) || LDT_zero(info))
		return true;

	/*
	 * espfix is required for 16-bit data segments, but espfix
	 * only works for LDT segments.
	 */
	if (!info->seg_32bit)
		return false;

	return true;
}

static void set_tls_desc(struct task_struct *p, int idx,
			 const struct user_desc *info, int n)
{
	struct thread_struct *t = &p->thread;
	struct desc_struct *desc = &t->tls_array[idx - GDT_ENTRY_TLS_MIN];
	int cpu;

	/*
	 * We must not get preempted while modifying the TLS.
	 */
	cpu = get_cpu();

	while (n-- > 0) {
		if (LDT_empty(info) || LDT_zero(info))
			desc->a = desc->b = 0;
		else
			fill_ldt(desc, info);
		++info;
		++desc;
	}

	if (t == &current->thread)
		load_TLS(t, cpu);

	put_cpu();
}

/*
 * Set a given TLS descriptor:
 */
int do_set_thread_area(struct task_struct *p, int idx,
		       struct user_desc __user *u_info,
		       int can_allocate)
{
	struct user_desc info;

	if (copy_from_user(&info, u_info, sizeof(info)))
		return -EFAULT;

	if (!tls_desc_okay(&info))
		return -EINVAL;

	if (idx == -1)
		idx = info.entry_number;

	/*
	 * index -1 means the kernel should try to find and
	 * allocate an empty descriptor:
	 */
	if (idx == -1 && can_allocate) {
		idx = get_free_idx();
		if (idx < 0)
			return idx;
		if (put_user(idx, &u_info->entry_number))
			return -EFAULT;
	}

	if (idx < GDT_ENTRY_TLS_MIN || idx > GDT_ENTRY_TLS_MAX)
		return -EINVAL;

	set_tls_desc(p, idx, &info, 1);

	return 0;
}

asmlinkage int sys_set_thread_area(struct user_desc __user *u_info)
{
	int ret = do_set_thread_area(current, -1, u_info, 1);
	asmlinkage_protect(1, ret, u_info);
	return ret;
}


/*
 * Get the current Thread-Local Storage area:
 */

static void fill_user_desc(struct user_desc *info, int idx,
			   const struct desc_struct *desc)

{
	memset(info, 0, sizeof(*info));
	info->entry_number = idx;
	info->base_addr = get_desc_base(desc);
	info->limit = get_desc_limit(desc);
	info->seg_32bit = desc->d;
	info->contents = desc->type >> 2;
	info->read_exec_only = !(desc->type & 2);
	info->limit_in_pages = desc->g;
	info->seg_not_present = !desc->p;
	info->useable = desc->avl;
#ifdef CONFIG_X86_64
	info->lm = desc->l;
#endif
}

int do_get_thread_area(struct task_struct *p, int idx,
		       struct user_desc __user *u_info)
{
	struct user_desc info;

	if (idx == -1 && get_user(idx, &u_info->entry_number))
		return -EFAULT;

	if (idx < GDT_ENTRY_TLS_MIN || idx > GDT_ENTRY_TLS_MAX)
		return -EINVAL;

	fill_user_desc(&info, idx,
		       &p->thread.tls_array[idx - GDT_ENTRY_TLS_MIN]);

	if (copy_to_user(u_info, &info, sizeof(info)))
		return -EFAULT;
	return 0;
}

asmlinkage int sys_get_thread_area(struct user_desc __user *u_info)
{
	int ret = do_get_thread_area(current, -1, u_info);
	asmlinkage_protect(1, ret, u_info);
	return ret;
}

int regset_tls_active(struct task_struct *target,
		      const struct user_regset *regset)
{
	struct thread_struct *t = &target->thread;
	int n = GDT_ENTRY_TLS_ENTRIES;
	while (n > 0 && desc_empty(&t->tls_array[n - 1]))
		--n;
	return n;
}

int regset_tls_get(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   void *kbuf, void __user *ubuf)
{
	const struct desc_struct *tls;

	if (pos >= GDT_ENTRY_TLS_ENTRIES * sizeof(struct user_desc) ||
	    (pos % sizeof(struct user_desc)) != 0 ||
	    (count % sizeof(struct user_desc)) != 0)
		return -EINVAL;

	pos /= sizeof(struct user_desc);
	count /= sizeof(struct user_desc);

	tls = &target->thread.tls_array[pos];

	if (kbuf) {
		struct user_desc *info = kbuf;
		while (count-- > 0)
			fill_user_desc(info++, GDT_ENTRY_TLS_MIN + pos++,
				       tls++);
	} else {
		struct user_desc __user *u_info = ubuf;
		while (count-- > 0) {
			struct user_desc info;
			fill_user_desc(&info, GDT_ENTRY_TLS_MIN + pos++, tls++);
			if (__copy_to_user(u_info++, &info, sizeof(info)))
				return -EFAULT;
		}
	}

	return 0;
}

int regset_tls_set(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	struct user_desc infobuf[GDT_ENTRY_TLS_ENTRIES];
	const struct user_desc *info;
	int i;

	if (pos >= GDT_ENTRY_TLS_ENTRIES * sizeof(struct user_desc) ||
	    (pos % sizeof(struct user_desc)) != 0 ||
	    (count % sizeof(struct user_desc)) != 0)
		return -EINVAL;

	if (kbuf)
		info = kbuf;
	else if (__copy_from_user(infobuf, ubuf, count))
		return -EFAULT;
	else
		info = infobuf;

	for (i = 0; i < count / sizeof(struct user_desc); i++)
		if (!tls_desc_okay(info + i))
			return -EINVAL;

	set_tls_desc(target,
		     GDT_ENTRY_TLS_MIN + (pos / sizeof(struct user_desc)),
		     info, count / sizeof(struct user_desc));

	return 0;
}
