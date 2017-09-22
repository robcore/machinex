#ifndef __LINUX_KEXEC_H
#define __LINUX_KEXEC_H

#define IND_DESTINATION_BIT 0
#define IND_INDIRECTION_BIT 1
#define IND_DONE_BIT        2
#define IND_SOURCE_BIT      3

#define IND_DESTINATION  (1 << IND_DESTINATION_BIT)
#define IND_INDIRECTION  (1 << IND_INDIRECTION_BIT)
#define IND_DONE         (1 << IND_DONE_BIT)
#define IND_SOURCE       (1 << IND_SOURCE_BIT)

#if !defined(__ASSEMBLY__)

#include <uapi/linux/kexec.h>

#ifdef CONFIG_KEXEC
#include <linux/list.h>
#include <linux/linkage.h>
#include <linux/compat.h>
#include <linux/ioport.h>
#include <asm/kexec.h>


/* Verify architecture specific macros are defined */

#ifndef KEXEC_SOURCE_MEMORY_LIMIT
#error KEXEC_SOURCE_MEMORY_LIMIT not defined
#endif

#ifndef KEXEC_DESTINATION_MEMORY_LIMIT
#error KEXEC_DESTINATION_MEMORY_LIMIT not defined
#endif

#ifndef KEXEC_CONTROL_MEMORY_LIMIT
#error KEXEC_CONTROL_MEMORY_LIMIT not defined
#endif

#ifndef KEXEC_CONTROL_PAGE_SIZE
#error KEXEC_CONTROL_PAGE_SIZE not defined
#endif

#ifndef KEXEC_ARCH
#error KEXEC_ARCH not defined
#endif

#ifndef KEXEC_CRASH_CONTROL_MEMORY_LIMIT
#define KEXEC_CRASH_CONTROL_MEMORY_LIMIT KEXEC_CONTROL_MEMORY_LIMIT
#endif

#ifndef KEXEC_CRASH_MEM_ALIGN
#define KEXEC_CRASH_MEM_ALIGN PAGE_SIZE
#endif

#define KEXEC_NOTE_HEAD_BYTES ALIGN(sizeof(struct elf_note), 4)
#define KEXEC_CORE_NOTE_NAME "CORE"
#define KEXEC_CORE_NOTE_NAME_BYTES ALIGN(sizeof(KEXEC_CORE_NOTE_NAME), 4)
#define KEXEC_CORE_NOTE_DESC_BYTES ALIGN(sizeof(struct elf_prstatus), 4)
/*
 * The per-cpu notes area is a list of notes terminated by a "NULL"
 * note header.  For kdump, the code in vmcore.c runs in the context
 * of the second kernel to combine them into one note.
 */
#ifndef KEXEC_NOTE_BYTES
#define KEXEC_NOTE_BYTES ( (KEXEC_NOTE_HEAD_BYTES * 2) +		\
			    KEXEC_CORE_NOTE_NAME_BYTES +		\
			    KEXEC_CORE_NOTE_DESC_BYTES )
#endif

/*
 * This structure is used to hold the arguments that are used when loading
 * kernel binaries.
 */

typedef unsigned long kimage_entry_t;

struct kexec_segment {
	/*
	 * This pointer can point to user memory if kexec_load() system
	 * call is used or will point to kernel memory if
	 * kexec_file_load() system call is used.
	 *
	 * Use ->buf when expecting to deal with user memory and use ->kbuf
	 * when expecting to deal with kernel memory.
	 */
	union {
		void __user *buf;
		void *kbuf;
	};
	size_t bufsz;
	unsigned long mem;
	size_t memsz;
};

#ifdef CONFIG_COMPAT
struct compat_kexec_segment {
	compat_uptr_t buf;
	compat_size_t bufsz;
	compat_ulong_t mem;	/* User space sees this as a (void *) ... */
	compat_size_t memsz;
};
#endif

struct kimage {
	kimage_entry_t head;
	kimage_entry_t *entry;
	kimage_entry_t *last_entry;

	unsigned long start;
	struct page *control_code_page;
	struct page *swap_page;

	unsigned long nr_segments;
	struct kexec_segment segment[KEXEC_SEGMENT_MAX];

	struct list_head control_pages;
	struct list_head dest_pages;
	struct list_head unusable_pages;

	/* Address of next control page to allocate for crash kernels. */
	unsigned long control_page;

	/* Flags to indicate special processing */
	unsigned int type : 1;
#define KEXEC_TYPE_DEFAULT 0
#define KEXEC_TYPE_CRASH   1
	unsigned int preserve_context : 1;
	/* If set, we are using file mode kexec syscall */
	unsigned int file_mode:1;
#ifdef CONFIG_KEXEC_HARDBOOT
	unsigned int hardboot : 1;
#endif

#ifdef ARCH_HAS_KIMAGE_ARCH
	struct kimage_arch arch;
#endif

	/* Additional fields for file based kexec syscall */
	void *kernel_buf;
	unsigned long kernel_buf_len;

	void *initrd_buf;
	unsigned long initrd_buf_len;

	char *cmdline_buf;
	unsigned long cmdline_buf_len;

	/* File operations provided by image loader */
	struct kexec_file_ops *fops;

	/* Image loader handling the kernel can store a pointer here */
	void *image_loader_data;
};

/*
 * Keeps track of buffer parameters as provided by caller for requesting
 * memory placement of buffer.
 */
struct kexec_buf {
	struct kimage *image;
	char *buffer;
	unsigned long bufsz;
	unsigned long mem;
	unsigned long memsz;
	unsigned long buf_align;
	unsigned long buf_min;
	unsigned long buf_max;
	bool top_down;		/* allocate from top of memory hole */
};

typedef int (kexec_probe_t)(const char *kernel_buf, unsigned long kernel_size);
typedef void *(kexec_load_t)(struct kimage *image, char *kernel_buf,
			     unsigned long kernel_len, char *initrd,
			     unsigned long initrd_len, char *cmdline,
			     unsigned long cmdline_len);
typedef int (kexec_cleanup_t)(struct kimage *image);

struct kexec_file_ops {
	kexec_probe_t *probe;
	kexec_load_t *load;
	kexec_cleanup_t *cleanup;
};

/* kexec interface functions */
extern void machine_kexec(struct kimage *image);
extern int machine_kexec_prepare(struct kimage *image);
extern void machine_kexec_cleanup(struct kimage *image);
extern asmlinkage long sys_kexec_load(unsigned long entry,
					unsigned long nr_segments,
					struct kexec_segment __user *segments,
					unsigned long flags);
extern void __weak arch_kexec(void);
extern int kernel_kexec(void);
extern int kexec_add_buffer(struct kimage *image, char *buffer,
			    unsigned long bufsz, unsigned long memsz,
			    unsigned long buf_align, unsigned long buf_min,
			    unsigned long buf_max, bool top_down,
			    unsigned long *load_addr);
extern struct page *kimage_alloc_control_pages(struct kimage *image,
						unsigned int order);
extern void crash_kexec(struct pt_regs *);
int kexec_should_crash(struct task_struct *);
void crash_save_cpu(struct pt_regs *regs, int cpu);
void crash_save_vmcoreinfo(void);
void crash_map_reserved_pages(void);
void crash_unmap_reserved_pages(void);
void arch_crash_save_vmcoreinfo(void);
__printf(1, 2)
void vmcoreinfo_append_str(const char *fmt, ...);
unsigned long paddr_vmcoreinfo_note(void);

#define VMCOREINFO_OSRELEASE(value) \
	vmcoreinfo_append_str("OSRELEASE=%s\n", value)
#define VMCOREINFO_PAGESIZE(value) \
	vmcoreinfo_append_str("PAGESIZE=%ld\n", value)
#define VMCOREINFO_SYMBOL(name) \
	vmcoreinfo_append_str("SYMBOL(%s)=%lx\n", #name, (unsigned long)&name)
#define VMCOREINFO_SIZE(name) \
	vmcoreinfo_append_str("SIZE(%s)=%lu\n", #name, \
			      (unsigned long)sizeof(name))
#define VMCOREINFO_STRUCT_SIZE(name) \
	vmcoreinfo_append_str("SIZE(%s)=%lu\n", #name, \
			      (unsigned long)sizeof(struct name))
#define VMCOREINFO_OFFSET(name, field) \
	vmcoreinfo_append_str("OFFSET(%s.%s)=%lu\n", #name, #field, \
			      (unsigned long)offsetof(struct name, field))
#define VMCOREINFO_LENGTH(name, value) \
	vmcoreinfo_append_str("LENGTH(%s)=%lu\n", #name, (unsigned long)value)
#define VMCOREINFO_NUMBER(name) \
	vmcoreinfo_append_str("NUMBER(%s)=%ld\n", #name, (long)name)
#define VMCOREINFO_CONFIG(name) \
	vmcoreinfo_append_str("CONFIG_%s=y\n", #name)

extern struct kimage *kexec_image;
extern struct kimage *kexec_crash_image;

#ifndef kexec_flush_icache_page
#define kexec_flush_icache_page(page)
#endif

/* List of defined/legal kexec flags */
#if defined(CONFIG_KEXEC_JUMP) && defined(CONFIG_KEXEC_HARDBOOT)
#define KEXEC_FLAGS    (KEXEC_ON_CRASH | KEXEC_PRESERVE_CONTEXT | KEXEC_HARDBOOT)
#elif defined(CONFIG_KEXEC_JUMP)
#define KEXEC_FLAGS    (KEXEC_ON_CRASH | KEXEC_PRESERVE_CONTEXT)
#elif defined(CONFIG_KEXEC_HARDBOOT)
#define KEXEC_FLAGS    (KEXEC_ON_CRASH | KEXEC_HARDBOOT)
#else
#define KEXEC_FLAGS    (KEXEC_ON_CRASH)
#endif

/* List of defined/legal kexec file flags */
#define KEXEC_FILE_FLAGS	(KEXEC_FILE_UNLOAD | KEXEC_FILE_ON_CRASH | \
				 KEXEC_FILE_NO_INITRAMFS)

#define VMCOREINFO_BYTES           (4096)
#define VMCOREINFO_NOTE_NAME       "VMCOREINFO"
#define VMCOREINFO_NOTE_NAME_BYTES ALIGN(sizeof(VMCOREINFO_NOTE_NAME), 4)
#define VMCOREINFO_NOTE_SIZE       (KEXEC_NOTE_HEAD_BYTES*2 + VMCOREINFO_BYTES \
				    + VMCOREINFO_NOTE_NAME_BYTES)

/* Location of a reserved region to hold the crash kernel.
 */
extern struct resource crashk_res;
typedef u32 note_buf_t[KEXEC_NOTE_BYTES/4];
extern note_buf_t __percpu *crash_notes;
extern u32 vmcoreinfo_note[VMCOREINFO_NOTE_SIZE/4];
extern size_t vmcoreinfo_size;
extern size_t vmcoreinfo_max_size;

int __init parse_crashkernel(char *cmdline, unsigned long long system_ram,
		unsigned long long *crash_size, unsigned long long *crash_base);
int crash_shrink_memory(unsigned long new_size);
size_t crash_get_memory_size(void);
void crash_free_reserved_phys_range(unsigned long begin, unsigned long end);

#else /* !CONFIG_KEXEC */
struct pt_regs;
struct task_struct;
static inline void crash_kexec(struct pt_regs *regs) { }
static inline int kexec_should_crash(struct task_struct *p) { return 0; }

/* Load a new kernel image as described by the kexec_segment array
 * consisting of passed number of segments at the entry-point address.
 * The flags allow different useage types.
 */
extern int kexec_load(void *, size_t, struct kexec_segment *,
		unsigned long int);
#endif /* CONFIG_KEXEC */

#endif /* !defined(__ASSEBMLY__) */

#endif /* LINUX_KEXEC_H */
