#ifndef _H8300_PTRACE_H
#define _H8300_PTRACE_H

#include <uapi/asm/ptrace.h>

#ifndef __ASSEMBLY__
#if defined(CONFIG_CPU_H8S)
#endif
#ifndef PS_S
#define PS_S  (0x10)
#endif

#if defined(__H8300H__)
#define H8300_REGS_NO 11
#endif
#if defined(__H8300S__)
#define H8300_REGS_NO 12
#endif

/* Find the stack offset for a register, relative to thread.esp0. */
#define PT_REG(reg)	((long)&((struct pt_regs *)0)->reg)

#define arch_has_single_step()	(1)

#define user_mode(regs) (!((regs)->ccr & PS_S))
#define instruction_pointer(regs) ((regs)->pc)
#define profile_pc(regs) instruction_pointer(regs)
#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */
#endif /* _H8300_PTRACE_H */
