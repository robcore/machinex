#ifndef _UAPI_LINUX_KEXEC_H
#define _UAPI_LINUX_KEXEC_H

/* kexec system call -  It loads the new kernel to boot into.
 * kexec does not sync, or unmount filesystems so if you need
 * that to happen you need to do that yourself.
 */

#include <linux/types.h>
#include <linux/elfcore.h>
#include <linux/elf.h>


/* kexec flags for different usage scenarios */
#define KEXEC_ON_CRASH		0x00000001
#define KEXEC_PRESERVE_CONTEXT	0x00000002
#ifdef CONFIG_KEXEC_HARDBOOT
#define KEXEC_HARDBOOT		0x00000004
#endif
#define KEXEC_ARCH_MASK		0xffff0000

/* These values match the ELF architecture values.
 * Unless there is a good reason that should continue to be the case.
 */
#define KEXEC_ARCH_DEFAULT ( 0 << 16)
#define KEXEC_ARCH_386     ( 3 << 16)
#define KEXEC_ARCH_X86_64  (62 << 16)
#define KEXEC_ARCH_PPC     (20 << 16)
#define KEXEC_ARCH_PPC64   (21 << 16)
#define KEXEC_ARCH_IA_64   (50 << 16)
#define KEXEC_ARCH_ARM     (40 << 16)
#define KEXEC_ARCH_S390    (22 << 16)
#define KEXEC_ARCH_SH      (42 << 16)
#define KEXEC_ARCH_MIPS_LE (10 << 16)
#define KEXEC_ARCH_MIPS    ( 8 << 16)

/* The artificial cap on the number of segments passed to kexec_load. */
#define KEXEC_SEGMENT_MAX 16

#endif /* _UAPILINUX_KEXEC_H */
