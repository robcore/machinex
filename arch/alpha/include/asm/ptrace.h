/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASMAXP_PTRACE_H
#define _ASMAXP_PTRACE_H

#include <uapi/asm/ptrace.h>


#define arch_has_single_step()		(1)
#define user_mode(regs) (((regs)->ps & 8) != 0)
#define instruction_pointer(regs) ((regs)->pc)
#define profile_pc(regs) instruction_pointer(regs)
#define current_user_stack_pointer() rdusp()

#define task_pt_regs(task) \
  ((struct pt_regs *) (task_stack_page(task) + 2*PAGE_SIZE) - 1)

#define force_successful_syscall_return() (task_pt_regs(current)->r0 = 0)

#endif
