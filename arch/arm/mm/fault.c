/*
 *  linux/arch/arm/mm/fault.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995-2004 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/page-flags.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/perf_event.h>

#include <asm/exception.h>
#include <asm/pgtable.h>
#include <asm/system_misc.h>
#include <asm/system_info.h>
#include <asm/tlbflush.h>
#include <asm/cputype.h>
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
#include <asm/io.h>
#include <mach/msm_iomap.h>
#endif

#include "fault.h"

#define CREATE_TRACE_POINTS
#include <trace/events/exception.h>

#ifdef CONFIG_MMU

#ifdef CONFIG_KPROBES
static inline int notify_page_fault(struct pt_regs *regs, unsigned int fsr)
{
	int ret = 0;

	if (!user_mode(regs)) {
		/* kprobe_running() needs smp_processor_id() */
		preempt_disable();
		if (kprobe_running() && kprobe_fault_handler(regs, fsr))
			ret = 1;
		preempt_enable();
	}

	return ret;
}
#else
static inline int notify_page_fault(struct pt_regs *regs, unsigned int fsr)
{
	return 0;
}
#endif

/*
 * This is useful to dump out the page tables associated with
 * 'addr' in mm 'mm'.
 */
void show_pte(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;

	if (!mm)
		mm = &init_mm;

	printk(KERN_ALERT "pgd = %p\n", mm->pgd);
	pgd = pgd_offset(mm, addr);
	printk(KERN_ALERT "[%08lx] *pgd=%08llx",
			addr, (long long)pgd_val(*pgd));

	do {
		pud_t *pud;
		pmd_t *pmd;
		pte_t *pte;

		if (pgd_none(*pgd))
			break;

		if (pgd_bad(*pgd)) {
			printk("(bad)");
			break;
		}

		pud = pud_offset(pgd, addr);
		if (PTRS_PER_PUD != 1)
			printk(", *pud=%08llx", (long long)pud_val(*pud));

		if (pud_none(*pud))
			break;

		if (pud_bad(*pud)) {
			printk("(bad)");
			break;
		}

		pmd = pmd_offset(pud, addr);
		if (PTRS_PER_PMD != 1)
			printk(", *pmd=%08llx", (long long)pmd_val(*pmd));

		if (pmd_none(*pmd))
			break;

		if (pmd_bad(*pmd)) {
			printk("(bad)");
			break;
		}

		/* We must not map this if we have highmem enabled */
		if (PageHighMem(pfn_to_page(pmd_val(*pmd) >> PAGE_SHIFT)))
			break;

		pte = pte_offset_map(pmd, addr);
		printk(", *pte=%08llx", (long long)pte_val(*pte));
#ifndef CONFIG_ARM_LPAE
		printk(", *ppte=%08llx",
		       (long long)pte_val(pte[PTE_HWTABLE_PTRS]));
#endif
		pte_unmap(pte);
	} while(0);

	printk("\n");
}
#else					/* CONFIG_MMU */
void show_pte(struct mm_struct *mm, unsigned long addr)
{ }
#endif
					/* CONFIG_MMU */
#ifdef TIMA_ENABLED
inline void tima_dump_log2()
{
        char *tima_log = (char *)0xde100000, *ptr, *ptr1;
        int line_ctr=0;

	return; /* WARNING: THIS FUNCTION HAS BEEN DISABLED */
	/* After the move to the new memory address, there is no virtual address for the log
	 * This function is disabled pending the availability of the same
	 */
        ptr = tima_log;
        ptr1 = ptr;
        while(line_ctr<100) {
                line_ctr++;
                while(*ptr1 != '\n')
                        ptr1++;
                *ptr1 = '\0';
                printk(KERN_EMERG"%s\n", ptr);
                *ptr1 = '\n';
                ptr1++;
                if(*ptr1 == '\0')
                        break;
                ptr = ptr1;
        }
}

inline void tima_verify_state(unsigned long pmdp, unsigned long val, unsigned long rd_only, unsigned long caller)
{
        unsigned long pmdp_addr = (unsigned long)pmdp;
        unsigned long init_pgd, pgd_val;
        unsigned long init_pte;
        unsigned long pte_val;
	unsigned long npte_val;
	static unsigned long call_count = 0;

	return; /* WARNING: THIS FUNCTION HAS BEEN DISABLED BECAUSE SECT_TO_PGT CAN NO LONGER BE ACCESSED
		   VIA VIRTUAL MEMORY MAPPING */

	if ((pmdp>>24)==0xc0)	return;
	call_count++;
	init_pgd = (unsigned long)init_mm.pgd;
        init_pgd += (pmdp_addr >> 20) << 2;

	pgd_val = (unsigned long)*(unsigned long *)init_pgd;
	if ((pgd_val & 0x3) != 0x1) {
		printk(KERN_ERR"TIMA: Entry is not L2 page. VA:%lx, PGD=%lx\n", pmdp, pgd_val);
		return;
	}

        init_pte = (unsigned long)__va(pgd_val & (~0x3ff));
        init_pte += ((pmdp_addr >> 12) & 0xff) << 2;

        pte_val = *(unsigned long *)init_pte;
        invalidate_caches(init_pte, 4, __pa(init_pte));
	npte_val = *(unsigned long *)init_pte;
        if (rd_only) {
                if ((pte_val & 0x230) != 0x210) { 	/* Page is RO */
                        printk(KERN_ERR"Page is NOT RO, CALLER=%lx VA=%lx, PTE=%lx FLUSHED PTE=%lx PA=%lx\n", caller, pmdp_addr, pte_val, npte_val, __pa(pmdp_addr));
			//tima_send_cmd(pmdp_addr, 0x3f80e221);
			//tima_dump_log2();
		}
        } else {
                if ((pte_val & 0x230) != 0x010) {	/* Page is RW */
                        printk(KERN_ERR"Page is NOT RW, CALLER=%lx VA=%lx, PTE=%lx FLUSHED PTE=%lx PA=%lx\n", caller, pmdp_addr, pte_val, npte_val, __pa(pmdp_addr));
			//tima_send_cmd(pmdp_addr, 0x3f80e221);
			//tima_dump_log2();
		}
        }
}
#endif

/*
 * Oops.  The kernel tried to access some page that wasn't present.
 */
static void
__do_kernel_fault(struct mm_struct *mm, unsigned long addr, unsigned int fsr,
		  struct pt_regs *regs)
{
	/*
	 * Are we prepared to handle this kernel fault?
	 */
	if (fixup_exception(regs))
		return;

#ifdef	TIMA_ENABLED
	printk(KERN_ERR"====> %lx\n", addr);
	if (addr >= 0xc0000000 && (fsr & FSR_WRITE)) {
		printk(KERN_ERR"==> Handling fault for %lx\n", addr);
		__asm__("mcr     p15, 0, %0, c7, c14, 1\n"
		    "isb"
			::"r"(addr));
		//tima_send_cmd(addr, 0x3f80e221);
		tima_send_cmd(addr, 0x3f80f221);
		__asm__ ("mcr     p15, 0, %0, c7, c6,  1\n"
		     "isb"
		     ::"r"(addr));
		__asm__ ("mcr    p15, 0, %0, c8, c3, 0\n"
			"isb"
			::"r"(0));
		//tima_dump_log2();
		return; 
	}
#endif

	/*
	 * No handler, we'll have to terminate things with extreme prejudice.
	 */
	bust_spinlocks(1);
	printk(KERN_ALERT
		"Unable to handle kernel %s at virtual address %08lx\n",
		(addr < PAGE_SIZE) ? "NULL pointer dereference" :
		"paging request", addr);

	show_pte(mm, addr);
#ifdef TIMA_ENABLED
	tima_send_cmd(addr, 0x3f80e221);
#endif
	die("Oops", regs, fsr);
	bust_spinlocks(0);
	do_exit(SIGKILL);
}

/*
 * Something tried to access memory that isn't in our memory map..
 * User mode accesses just cause a SIGSEGV
 */
static void
__do_user_fault(struct task_struct *tsk, unsigned long addr,
		unsigned int fsr, unsigned int sig, int code,
		struct pt_regs *regs)
{
	struct siginfo si;

	trace_user_fault(tsk, addr, fsr);

#ifdef CONFIG_DEBUG_USER
	if (((user_debug & UDBG_SEGV) && (sig == SIGSEGV)) ||
	    ((user_debug & UDBG_BUS)  && (sig == SIGBUS))) {
		printk(KERN_DEBUG "%s: unhandled page fault (%d) at 0x%08lx, code 0x%03x\n",
		       tsk->comm, sig, addr, fsr);
		show_pte(tsk->mm, addr);
		show_regs(regs);
	}
#endif

	tsk->thread.address = addr;
	tsk->thread.error_code = fsr;
	tsk->thread.trap_no = 14;
	si.si_signo = sig;
	si.si_errno = 0;
	si.si_code = code;
	si.si_addr = (void __user *)addr;
	force_sig_info(sig, &si, tsk);
}

void do_bad_area(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	struct task_struct *tsk = current;
	struct mm_struct *mm = tsk->active_mm;

	/*
	 * If we are in kernel mode at this point, we
	 * have no context to handle this fault with.
	 */
	if (user_mode(regs))
		__do_user_fault(tsk, addr, fsr, SIGSEGV, SEGV_MAPERR, regs);
	else
		__do_kernel_fault(mm, addr, fsr, regs);
}

#ifdef CONFIG_MMU
#define VM_FAULT_BADMAP		0x010000
#define VM_FAULT_BADACCESS	0x020000

/*
 * Check that the permissions on the VMA allow for the fault which occurred.
 * If we encountered a write fault, we must have write permission, otherwise
 * we allow any permission.
 */
static inline bool access_error(unsigned int fsr, struct vm_area_struct *vma)
{
	unsigned int mask = VM_READ | VM_WRITE | VM_EXEC;

	if (fsr & FSR_WRITE)
		mask = VM_WRITE;
	if (fsr & FSR_LNX_PF)
		mask = VM_EXEC;

	return vma->vm_flags & mask ? false : true;
}

static int __kprobes
__do_page_fault(struct mm_struct *mm, unsigned long addr, unsigned int fsr,
		unsigned int flags, struct task_struct *tsk)
{
	struct vm_area_struct *vma;
	int fault;

	vma = find_vma(mm, addr);
	fault = VM_FAULT_BADMAP;
	if (unlikely(!vma))
		goto out;
	if (unlikely(vma->vm_start > addr))
		goto check_stack;

	/*
	 * Ok, we have a good vm_area for this
	 * memory access, so we can handle it.
	 */
good_area:
	if (access_error(fsr, vma)) {
		fault = VM_FAULT_BADACCESS;
		goto out;
	}

	return handle_mm_fault(mm, vma, addr & PAGE_MASK, flags);

check_stack:
	/* Don't allow expansion below FIRST_USER_ADDRESS */
	if (vma->vm_flags & VM_GROWSDOWN &&
	    addr >= FIRST_USER_ADDRESS && !expand_stack(vma, addr))
		goto good_area;
out:
	return fault;
}

static int __kprobes
do_page_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	struct task_struct *tsk;
	struct mm_struct *mm;
	int fault, sig, code;
	int write = fsr & FSR_WRITE;
	unsigned int flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE |
				(write ? FAULT_FLAG_WRITE : 0);

	if (notify_page_fault(regs, fsr))
		return 0;

	tsk = current;
	mm  = tsk->mm;

	/* Enable interrupts if they were enabled in the parent context. */
	if (interrupts_enabled(regs))
		local_irq_enable();

	/*
	 * If we're in an interrupt, or have no irqs, or have no user
	 * context, we must not take the fault..
	 */
	if (in_atomic() || irqs_disabled() || !mm)
		goto no_context;

	/*
	 * As per x86, we may deadlock here.  However, since the kernel only
	 * validly references user space from well defined areas of the code,
	 * we can bug out early if this is from code which shouldn't.
	 */
	if (!down_read_trylock(&mm->mmap_sem)) {
		if (!user_mode(regs) && !search_exception_tables(regs->ARM_pc))
			goto no_context;
retry:
		down_read(&mm->mmap_sem);
	} else {
		/*
		 * The above down_read_trylock() might have succeeded in
		 * which case, we'll have missed the might_sleep() from
		 * down_read()
		 */
		might_sleep();
#ifdef CONFIG_DEBUG_VM
		if (!user_mode(regs) &&
		    !search_exception_tables(regs->ARM_pc))
			goto no_context;
#endif
	}

	fault = __do_page_fault(mm, addr, fsr, flags, tsk);

	/* If we need to retry but a fatal signal is pending, handle the
	 * signal first. We do not need to release the mmap_sem because
	 * it would already be released in __lock_page_or_retry in
	 * mm/filemap.c. */
	if ((fault & VM_FAULT_RETRY) && fatal_signal_pending(current))
		return 0;

	/*
	 * Major/minor page fault accounting is only done on the
	 * initial attempt. If we go through a retry, it is extremely
	 * likely that the page will be found in page cache at that point.
	 */

	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, addr);
	if (!(fault & VM_FAULT_ERROR) && flags & FAULT_FLAG_ALLOW_RETRY) {
		if (fault & VM_FAULT_MAJOR) {
			tsk->maj_flt++;
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MAJ, 1,
					regs, addr);
		} else {
			tsk->min_flt++;
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MIN, 1,
					regs, addr);
		}
		if (fault & VM_FAULT_RETRY) {
			/* Clear FAULT_FLAG_ALLOW_RETRY to avoid any risk
			* of starvation. */
			flags &= ~FAULT_FLAG_ALLOW_RETRY;
			flags |= FAULT_FLAG_TRIED;
			goto retry;
		}
	}

	up_read(&mm->mmap_sem);

	/*
	 * Handle the "normal" case first - VM_FAULT_MAJOR / VM_FAULT_MINOR
	 */
	if (likely(!(fault & (VM_FAULT_ERROR | VM_FAULT_BADMAP | VM_FAULT_BADACCESS))))
		return 0;

	/*
	 * If we are in kernel mode at this point, we
	 * have no context to handle this fault with.
	 */
	if (!user_mode(regs))
		goto no_context;

	if (fault & VM_FAULT_OOM) {
		/*
		 * We ran out of memory, call the OOM killer, and return to
		 * userspace (which will retry the fault, or kill us if we
		 * got oom-killed)
		 */
		pagefault_out_of_memory();
		return 0;
	}

	if (fault & VM_FAULT_SIGBUS) {
		/*
		 * We had some memory, but were unable to
		 * successfully fix up this page fault.
		 */
		sig = SIGBUS;
		code = BUS_ADRERR;
	} else {
		/*
		 * Something tried to access memory that
		 * isn't in our memory map..
		 */
		sig = SIGSEGV;
		code = fault == VM_FAULT_BADACCESS ?
			SEGV_ACCERR : SEGV_MAPERR;
	}

	__do_user_fault(tsk, addr, fsr, sig, code, regs);
	return 0;

no_context:
	__do_kernel_fault(mm, addr, fsr, regs);
	return 0;
}
#else					/* CONFIG_MMU */
static int
do_page_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	return 0;
}
#endif					/* CONFIG_MMU */

/*
 * First Level Translation Fault Handler
 *
 * We enter here because the first level page table doesn't contain
 * a valid entry for the address.
 *
 * If the address is in kernel space (>= TASK_SIZE), then we are
 * probably faulting in the vmalloc() area.
 *
 * If the init_task's first level page tables contains the relevant
 * entry, we copy the it to this task.  If not, we send the process
 * a signal, fixup the exception, or oops the kernel.
 *
 * NOTE! We MUST NOT take any locks for this case. We may be in an
 * interrupt or a critical region, and should only copy the information
 * from the master page table, nothing more.
 */
#ifdef CONFIG_MMU
static int __kprobes
do_translation_fault(unsigned long addr, unsigned int fsr,
		     struct pt_regs *regs)
{
	unsigned int index;
	pgd_t *pgd, *pgd_k;
	pud_t *pud, *pud_k;
	pmd_t *pmd, *pmd_k;

	if (addr < TASK_SIZE)
		return do_page_fault(addr, fsr, regs);

	if (user_mode(regs))
		goto bad_area;

	index = pgd_index(addr);

	/*
	 * FIXME: CP15 C1 is write only on ARMv3 architectures.
	 */
	pgd = cpu_get_pgd() + index;
	pgd_k = init_mm.pgd + index;

	if (pgd_none(*pgd_k))
		goto bad_area;
	if (!pgd_present(*pgd))
		set_pgd(pgd, *pgd_k);

	pud = pud_offset(pgd, addr);
	pud_k = pud_offset(pgd_k, addr);

	if (pud_none(*pud_k))
		goto bad_area;
	if (!pud_present(*pud)) {
		set_pud(pud, *pud_k);
		/*
		 * There is a small window during free_pgtables() where the
		 * user *pud entry is 0 but the TLB has not been invalidated
		 * and we get a level 2 (pmd) translation fault caused by the
		 * intermediate TLB caching of the old level 1 (pud) entry.
		 */
		flush_tlb_kernel_page(addr);
	}

	pmd = pmd_offset(pud, addr);
	pmd_k = pmd_offset(pud_k, addr);

#ifdef CONFIG_ARM_LPAE
	/*
	 * Only one hardware entry per PMD with LPAE.
	 */
	index = 0;
#else
	/*
	 * On ARM one Linux PGD entry contains two hardware entries (see page
	 * tables layout in pgtable.h). We normally guarantee that we always
	 * fill both L1 entries. But create_mapping() doesn't follow the rule.
	 * It can create inidividual L1 entries, so here we have to call
	 * pmd_none() check for the entry really corresponded to address, not
	 * for the first of pair.
	 */
	index = (addr >> SECTION_SHIFT) & 1;
#endif
	if (pmd_none(pmd_k[index]))
		goto bad_area;
	if (!pmd_present(pmd[index]))
		copy_pmd(pmd, pmd_k);

	return 0;

bad_area:
	do_bad_area(addr, fsr, regs);
	return 0;
}
#else					/* CONFIG_MMU */
static int
do_translation_fault(unsigned long addr, unsigned int fsr,
		     struct pt_regs *regs)
{
	return 0;
}
#endif					/* CONFIG_MMU */

/*
 * Some section permission faults need to be handled gracefully.
 * They can happen due to a __{get,put}_user during an oops.
 */
static int
do_sect_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	do_bad_area(addr, fsr, regs);
	return 0;
}

/*
 * This abort handler always returns "fault".
 */
static int
do_bad(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	return 1;
}

#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
#define __str(x) #x
#define MRC(x, v1, v2, v4, v5, v6) do {					\
	unsigned int __##x;						\
	asm("mrc " __str(v1) ", " __str(v2) ", %0, " __str(v4) ", "	\
		__str(v5) ", " __str(v6) "\n" \
		: "=r" (__##x));					\
	pr_info("%s: %s = 0x%.8x\n", __func__, #x, __##x);		\
} while(0)

#define MSM_TCSR_SPARE2 (MSM_TCSR_BASE + 0x60)

#endif

int
do_imprecise_ext(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
	MRC(ADFSR,    p15, 0,  c5, c1, 0);
	MRC(DFSR,     p15, 0,  c5, c0, 0);
	MRC(ACTLR,    p15, 0,  c1, c0, 1);
	MRC(EFSR,     p15, 7, c15, c0, 1);
	MRC(L2SR,     p15, 3, c15, c1, 0);
	MRC(L2CR0,    p15, 3, c15, c0, 1);
	MRC(L2CPUESR, p15, 3, c15, c1, 1);
	MRC(L2CPUCR,  p15, 3, c15, c0, 2);
	MRC(SPESR,    p15, 1,  c9, c7, 0);
	MRC(SPCR,     p15, 0,  c9, c7, 0);
	MRC(DMACHSR,  p15, 1, c11, c0, 0);
	MRC(DMACHESR, p15, 1, c11, c0, 1);
	MRC(DMACHCR,  p15, 0, c11, c0, 2);

	/* clear out EFSR and ADFSR after fault */
	asm volatile ("mcr p15, 7, %0, c15, c0, 1\n\t"
		      "mcr p15, 0, %0, c5, c1, 0"
		      : : "r" (0));
#endif
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
	pr_info("%s: TCSR_SPARE2 = 0x%.8x\n", __func__, readl(MSM_TCSR_SPARE2));
#endif
	return 1;
}

struct fsr_info {
	int	(*fn)(unsigned long addr, unsigned int fsr, struct pt_regs *regs);
	int	sig;
	int	code;
	const char *name;
};

/* FSR definition */
#ifdef CONFIG_ARM_LPAE
#include "fsr-3level.c"
#else
#include "fsr-2level.c"
#endif

void __init
hook_fault_code(int nr, int (*fn)(unsigned long, unsigned int, struct pt_regs *),
		int sig, int code, const char *name)
{
	if (nr < 0 || nr >= ARRAY_SIZE(fsr_info))
		BUG();

	fsr_info[nr].fn   = fn;
	fsr_info[nr].sig  = sig;
	fsr_info[nr].code = code;
	fsr_info[nr].name = name;
}

#ifdef CONFIG_MSM_KRAIT_TBB_ABORT_HANDLER
static int krait_tbb_fixup(unsigned int fsr, struct pt_regs *regs)
{
	int base_cond, cond = 0;
	unsigned int p1, cpsr_z, cpsr_c, cpsr_n, cpsr_v;

	if ((read_cpuid_id() & 0xFFFFFFFC) != 0x510F04D0)
		return 0;

	if (!thumb_mode(regs))
		return 0;

	/* If ITSTATE is 0, return quickly */
	if ((regs->ARM_cpsr & PSR_IT_MASK) == 0)
		return 0;

	cpsr_n = (regs->ARM_cpsr & PSR_N_BIT) ? 1 : 0;
	cpsr_z = (regs->ARM_cpsr & PSR_Z_BIT) ? 1 : 0;
	cpsr_c = (regs->ARM_cpsr & PSR_C_BIT) ? 1 : 0;
	cpsr_v = (regs->ARM_cpsr & PSR_V_BIT) ? 1 : 0;

	p1 = (regs->ARM_cpsr & BIT(12)) ? 1 : 0;

	base_cond = (regs->ARM_cpsr >> 13) & 0x07;

	switch (base_cond) {
	case 0x0:	/* equal */
		cond = cpsr_z;
		break;

	case 0x1:	/* carry set */
		cond = cpsr_c;
		break;

	case 0x2:	/* minus / negative */
		cond = cpsr_n;
		break;

	case 0x3:	/* overflow */
		cond = cpsr_v;
		break;

	case 0x4:	/* unsigned higher */
		cond = (cpsr_c == 1) && (cpsr_z == 0);
		break;

	case 0x5:	/* signed greater / equal */
		cond = (cpsr_n == cpsr_v);
		break;

	case 0x6:	/* signed greater */
		cond = (cpsr_z == 0) && (cpsr_n == cpsr_v);
		break;

	case 0x7:	/* always */
		cond = 1;
		break;
	};

	if (cond == p1) {
		pr_debug("Conditional abort fixup, PC=%08x, base=%d, cond=%d\n",
			 (unsigned int) regs->ARM_pc, base_cond, cond);
		regs->ARM_pc += 2;
		return 1;
	}
	return 0;
}
#endif

/*
 * Dispatch a data abort to the relevant handler.
 */
asmlinkage void __exception
do_DataAbort(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	const struct fsr_info *inf = fsr_info + fsr_fs(fsr);
	struct siginfo info;

#ifdef CONFIG_MSM_KRAIT_TBB_ABORT_HANDLER
	if (krait_tbb_fixup(fsr, regs))
		return;
#endif

	if (!inf->fn(addr, fsr & ~FSR_LNX_PF, regs))
		return;

	printk(KERN_ALERT "Unhandled fault: %s (0x%03x) at 0x%08lx\n",
		inf->name, fsr, addr);

	info.si_signo = inf->sig;
	info.si_errno = 0;
	info.si_code  = inf->code;
	info.si_addr  = (void __user *)addr;
	arm_notify_die("", regs, &info, fsr, 0);
}

void __init
hook_ifault_code(int nr, int (*fn)(unsigned long, unsigned int, struct pt_regs *),
		 int sig, int code, const char *name)
{
	if (nr < 0 || nr >= ARRAY_SIZE(ifsr_info))
		BUG();

	ifsr_info[nr].fn   = fn;
	ifsr_info[nr].sig  = sig;
	ifsr_info[nr].code = code;
	ifsr_info[nr].name = name;
}

asmlinkage void __exception
do_PrefetchAbort(unsigned long addr, unsigned int ifsr, struct pt_regs *regs)
{
	const struct fsr_info *inf = ifsr_info + fsr_fs(ifsr);
	struct siginfo info;

	if (!inf->fn(addr, ifsr | FSR_LNX_PF, regs))
		return;

	printk(KERN_ALERT "Unhandled prefetch abort: %s (0x%03x) at 0x%08lx\n",
		inf->name, ifsr, addr);

	info.si_signo = inf->sig;
	info.si_errno = 0;
	info.si_code  = inf->code;
	info.si_addr  = (void __user *)addr;
	arm_notify_die("", regs, &info, ifsr, 0);
}

#ifndef CONFIG_ARM_LPAE
static int __init exceptions_init(void)
{
	if (cpu_architecture() >= CPU_ARCH_ARMv6) {
		hook_fault_code(4, do_translation_fault, SIGSEGV, SEGV_MAPERR,
				"I-cache maintenance fault");
	}

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		/*
		 * TODO: Access flag faults introduced in ARMv6K.
		 * Runtime check for 'K' extension is needed
		 */
		hook_fault_code(3, do_bad, SIGSEGV, SEGV_MAPERR,
				"section access flag fault");
		hook_fault_code(6, do_bad, SIGSEGV, SEGV_MAPERR,
				"section access flag fault");
	}

	return 0;
}

arch_initcall(exceptions_init);
#endif
