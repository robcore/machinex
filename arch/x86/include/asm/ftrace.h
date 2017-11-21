/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_FTRACE_H
#define _ASM_X86_FTRACE_H

#ifdef __ASSEMBLY__

	/* skip is set if the stack was already partially adjusted */
	.macro MCOUNT_SAVE_FRAME skip=0
	 /*
	  * We add enough stack to save all regs.
	  */
	subq $(SS+8-\skip), %rsp
	movq %rax, RAX(%rsp)
	movq %rcx, RCX(%rsp)
	movq %rdx, RDX(%rsp)
	movq %rsi, RSI(%rsp)
	movq %rdi, RDI(%rsp)
	movq %r8, R8(%rsp)
	movq %r9, R9(%rsp)
	 /* Move RIP to its proper location */
	movq SS+8(%rsp), %rdx
	movq %rdx, RIP(%rsp)
	.endm

	.macro MCOUNT_RESTORE_FRAME skip=0
	movq R9(%rsp), %r9
	movq R8(%rsp), %r8
	movq RDI(%rsp), %rdi
	movq RSI(%rsp), %rsi
	movq RDX(%rsp), %rdx
	movq RCX(%rsp), %rcx
	movq RAX(%rsp), %rax
	addq $(SS+8-\skip), %rsp
	.endm

#endif

#ifdef CONFIG_FUNCTION_TRACER
#define MCOUNT_ADDR		((long)(mcount))
#define MCOUNT_INSN_SIZE	5 /* sizeof mcount call */

#if defined(CONFIG_DYNAMIC_FTRACE) && defined(CONFIG_X86_64)
#define ARCH_SUPPORTS_FTRACE_OPS 1
#ifdef CONFIG_X86_64
#define ARCH_SUPPORTS_FTRACE_SAVE_REGS
#endif
#endif

#ifndef __ASSEMBLY__
extern void mcount(void);

static inline unsigned long ftrace_call_adjust(unsigned long addr)
{
	/*
	 * addr is the address of the mcount call instruction.
	 * recordmcount does the necessary offset calculation.
	 */
	return addr;
}

#ifdef CONFIG_DYNAMIC_FTRACE

struct dyn_arch_ftrace {
	/* No extra data needed for x86 */
};

#endif /*  CONFIG_DYNAMIC_FTRACE */
#endif /* __ASSEMBLY__ */
#endif /* CONFIG_FUNCTION_TRACER */

#endif /* _ASM_X86_FTRACE_H */
