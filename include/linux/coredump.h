#ifndef _LINUX_COREDUMP_H
#define _LINUX_COREDUMP_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <asm/siginfo.h>

/*
 * These are the only things you should do on a core-file: use only these
 * functions to write out all the necessary info.
 */
extern int dump_write(struct file *file, const void *addr, int nr);
extern int dump_seek(struct file *file, loff_t off);
extern void do_coredump(siginfo_t *siginfo, struct pt_regs *regs);
#if 0
static inline void do_coredump(siginfo_t *siginfo, struct pt_regs *regs) {}
#endif

#endif /* _LINUX_COREDUMP_H */
