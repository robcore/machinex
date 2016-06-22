cmd_kernel/printk.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,kernel/.printk.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/kernel -Ikernel -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing -fno-common -mtune="cortex-a15" -mfpu=neon-vfpv4 -std=gnu89 -Wno-format-security -Wno-unused-function -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -fno-delete-null-pointer-checks -Wno-cpp -Wno-declaration-after-statement -fno-var-tracking-assignments -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Wno-sequence-point -O2 -marm -fno-dwarf2-cfi-asm -fstack-protector -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=1024 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -mcpu=cortex-a15 -mtune=cortex-a15 -mfpu=neon-vfpv4 -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(printk)"  -D"KBUILD_MODNAME=KBUILD_STR(printk)" -c -o kernel/.tmp_printk.o /media/root/robcore/android/machinex/kernel/printk.c

source_kernel/printk.o := /media/root/robcore/android/machinex/kernel/printk.c

deps_kernel/printk.o := \
    $(wildcard include/config/sec/debug.h) \
    $(wildcard include/config/print/extra/info.h) \
    $(wildcard include/config/log/buf/shift.h) \
    $(wildcard include/config/default/message/loglevel.h) \
    $(wildcard include/config/a11y/braille/console.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/sec/ssr/dump.h) \
    $(wildcard include/config/kexec.h) \
    $(wildcard include/config/printk/nocache.h) \
    $(wildcard include/config/sec/log/last/kmsg.h) \
    $(wildcard include/config/sec/debug/subsys.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/phys/offset.h) \
    $(wildcard include/config/sec/debug/nocache/log/in/level/low.h) \
    $(wildcard include/config/sec/debug/low/log.h) \
    $(wildcard include/config/boot/printk/delay.h) \
    $(wildcard include/config/security/dmesg/restrict.h) \
    $(wildcard include/config/kgdb/kdb.h) \
    $(wildcard include/config/printk/time.h) \
    $(wildcard include/config/msm/rtb.h) \
    $(wildcard include/config/lge/crash/handler.h) \
  /media/root/robcore/android/machinex/include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/cpu/cp15/mmu.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /media/root/robcore/android/machinex/include/linux/sysinfo.h \
  /media/root/robcore/android/machinex/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/types.h \
  /media/root/robcore/android/machinex/include/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitsperlong.h \
  /media/root/robcore/android/machinex/include/linux/posix_types.h \
  /media/root/robcore/android/machinex/include/linux/stddef.h \
  /media/root/robcore/android/machinex/include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /media/root/robcore/android/machinex/include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  /media/root/robcore/android/machinex/include/linux/compiler-gcc5.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/posix_types.h \
  /media/root/robcore/android/machinex/include/asm-generic/posix_types.h \
  /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include/stdarg.h \
  /media/root/robcore/android/machinex/include/linux/linkage.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/include/linux/bitops.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  /media/root/robcore/android/machinex/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /media/root/robcore/android/machinex/include/linux/typecheck.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/irqflags.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/hwcap.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/non-atomic.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/fls64.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/sched.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/hweight.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/arch_hweight.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/const_hweight.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/lock.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/le.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/byteorder.h \
  /media/root/robcore/android/machinex/include/linux/byteorder/little_endian.h \
  /media/root/robcore/android/machinex/include/linux/swab.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/swab.h \
  /media/root/robcore/android/machinex/include/linux/byteorder/generic.h \
  /media/root/robcore/android/machinex/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /media/root/robcore/android/machinex/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /media/root/robcore/android/machinex/include/linux/printk.h \
    $(wildcard include/config/dynamic/debug.h) \
  /media/root/robcore/android/machinex/include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  /media/root/robcore/android/machinex/include/linux/dynamic_debug.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/div64.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/compiler.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/debug/bugverbose.h) \
    $(wildcard include/config/arm/lpae.h) \
  /media/root/robcore/android/machinex/include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /media/root/robcore/android/machinex/include/linux/mm.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/fix/movable/zone.h) \
    $(wildcard include/config/sysctl.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/transparent/hugepage.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/ksm.h) \
    $(wildcard include/config/have/memblock/node/map.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/proc/fs.h) \
    $(wildcard include/config/debug/pagealloc.h) \
    $(wildcard include/config/hibernation.h) \
    $(wildcard include/config/use/user/accessible/timers.h) \
    $(wildcard include/config/hugetlbfs.h) \
  /media/root/robcore/android/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno-base.h \
  /media/root/robcore/android/machinex/include/linux/gfp.h \
    $(wildcard include/config/kmemcheck.h) \
    $(wildcard include/config/cma.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/pm/sleep.h) \
  /media/root/robcore/android/machinex/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/cgroup/mem/res/ctlr.h) \
    $(wildcard include/config/no/bootmem.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/have/memblock/node.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  /media/root/robcore/android/machinex/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  /media/root/robcore/android/machinex/include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  /media/root/robcore/android/machinex/include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/domain.h \
    $(wildcard include/config/verify/permission/fault.h) \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \
    $(wildcard include/config/emulate/domain/manager/v7.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/barrier.h \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/arch/has/barriers.h) \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/outercache.h \
    $(wildcard include/config/outer/cache/sync.h) \
    $(wildcard include/config/outer/cache.h) \
  /media/root/robcore/android/machinex/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /media/root/robcore/android/machinex/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  /media/root/robcore/android/machinex/include/linux/const.h \
  /media/root/robcore/android/machinex/include/linux/stringify.h \
  /media/root/robcore/android/machinex/include/linux/bottom_half.h \
  /media/root/robcore/android/machinex/include/linux/spinlock_types.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/spinlock_types.h \
  /media/root/robcore/android/machinex/include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/prove/rcu.h) \
  /media/root/robcore/android/machinex/include/linux/rwlock_types.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/spinlock.h \
    $(wildcard include/config/msm/krait/wfe/fixup.h) \
    $(wildcard include/config/arm/ticket/locks.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/processor.h \
    $(wildcard include/config/have/hw/breakpoint.h) \
    $(wildcard include/config/arm/errata/754327.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/hw_breakpoint.h \
  /media/root/robcore/android/machinex/include/linux/rwlock.h \
  /media/root/robcore/android/machinex/include/linux/spinlock_api_smp.h \
    $(wildcard include/config/inline/spin/lock.h) \
    $(wildcard include/config/inline/spin/lock/bh.h) \
    $(wildcard include/config/inline/spin/lock/irq.h) \
    $(wildcard include/config/inline/spin/lock/irqsave.h) \
    $(wildcard include/config/inline/spin/trylock.h) \
    $(wildcard include/config/inline/spin/trylock/bh.h) \
    $(wildcard include/config/uninline/spin/unlock.h) \
    $(wildcard include/config/inline/spin/unlock/bh.h) \
    $(wildcard include/config/inline/spin/unlock/irq.h) \
    $(wildcard include/config/inline/spin/unlock/irqrestore.h) \
  /media/root/robcore/android/machinex/include/linux/rwlock_api_smp.h \
    $(wildcard include/config/inline/read/lock.h) \
    $(wildcard include/config/inline/write/lock.h) \
    $(wildcard include/config/inline/read/lock/bh.h) \
    $(wildcard include/config/inline/write/lock/bh.h) \
    $(wildcard include/config/inline/read/lock/irq.h) \
    $(wildcard include/config/inline/write/lock/irq.h) \
    $(wildcard include/config/inline/read/lock/irqsave.h) \
    $(wildcard include/config/inline/write/lock/irqsave.h) \
    $(wildcard include/config/inline/read/trylock.h) \
    $(wildcard include/config/inline/write/trylock.h) \
    $(wildcard include/config/inline/read/unlock.h) \
    $(wildcard include/config/inline/write/unlock.h) \
    $(wildcard include/config/inline/read/unlock/bh.h) \
    $(wildcard include/config/inline/write/unlock/bh.h) \
    $(wildcard include/config/inline/read/unlock/irq.h) \
    $(wildcard include/config/inline/write/unlock/irq.h) \
    $(wildcard include/config/inline/read/unlock/irqrestore.h) \
    $(wildcard include/config/inline/write/unlock/irqrestore.h) \
  /media/root/robcore/android/machinex/include/linux/atomic.h \
    $(wildcard include/config/arch/has/atomic/or.h) \
    $(wildcard include/config/generic/atomic64.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/atomic.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/cmpxchg.h \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/v6.h) \
  /media/root/robcore/android/machinex/include/asm-generic/cmpxchg-local.h \
  /media/root/robcore/android/machinex/include/asm-generic/atomic-long.h \
  /media/root/robcore/android/machinex/include/linux/wait.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/current.h \
  /media/root/robcore/android/machinex/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  /media/root/robcore/android/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /media/root/robcore/android/machinex/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  /media/root/robcore/android/machinex/include/linux/seqlock.h \
  /media/root/robcore/android/machinex/include/linux/nodemask.h \
  /media/root/robcore/android/machinex/include/linux/bitmap.h \
  /media/root/robcore/android/machinex/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/string.h \
  /media/root/robcore/android/machinex/include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/generated/bounds.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/fa.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/kuser/helpers.h) \
    $(wildcard include/config/memory/hotplug/sparse.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/glue.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable-2level-types.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/memory.h \
    $(wildcard include/config/need/mach/memory/h.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
  arch/arm/include/generated/asm/sizes.h \
  /media/root/robcore/android/machinex/include/asm-generic/sizes.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/memory.h \
    $(wildcard include/config/have/end/mem.h) \
    $(wildcard include/config/end/mem.h) \
    $(wildcard include/config/arch/msm7x30.h) \
    $(wildcard include/config/vmsplit/3g.h) \
    $(wildcard include/config/arch/msm/arm11.h) \
    $(wildcard include/config/arch/msm/cortex/a5.h) \
    $(wildcard include/config/cache/l2x0.h) \
    $(wildcard include/config/arch/msm8x60.h) \
    $(wildcard include/config/arch/msm8960.h) \
    $(wildcard include/config/dont/map/hole/after/membank0.h) \
    $(wildcard include/config/arch/msm/scorpion.h) \
    $(wildcard include/config/arch/msm/krait.h) \
    $(wildcard include/config/arch/msm7x27.h) \
  /media/root/robcore/android/machinex/include/asm-generic/memory_model.h \
  /media/root/robcore/android/machinex/include/asm-generic/getorder.h \
  /media/root/robcore/android/machinex/include/linux/memory_hotplug.h \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  /media/root/robcore/android/machinex/include/linux/notifier.h \
  /media/root/robcore/android/machinex/include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  /media/root/robcore/android/machinex/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/rwsem.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/exec.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/switch_to.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system_info.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system_misc.h \
  /media/root/robcore/android/machinex/include/linux/srcu.h \
  /media/root/robcore/android/machinex/include/linux/rcupdate.h \
    $(wildcard include/config/rcu/torture/test.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tree/preempt/rcu.h) \
    $(wildcard include/config/rcu/trace.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/tiny/preempt/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/preempt/rt.h) \
  /media/root/robcore/android/machinex/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  /media/root/robcore/android/machinex/include/linux/bug.h \
  /media/root/robcore/android/machinex/include/linux/completion.h \
  /media/root/robcore/android/machinex/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  /media/root/robcore/android/machinex/include/linux/rcutree.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/sparsemem.h \
  /media/root/robcore/android/machinex/include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
    $(wildcard include/config/sched/book.h) \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
  /media/root/robcore/android/machinex/include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/smp.h \
  /media/root/robcore/android/machinex/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /media/root/robcore/android/machinex/include/linux/pfn.h \
  arch/arm/include/generated/asm/percpu.h \
  /media/root/robcore/android/machinex/include/asm-generic/percpu.h \
  /media/root/robcore/android/machinex/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/topology.h \
    $(wildcard include/config/arm/cpu/topology.h) \
  /media/root/robcore/android/machinex/include/asm-generic/topology.h \
  /media/root/robcore/android/machinex/include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/virtual.h) \
  /media/root/robcore/android/machinex/include/linux/rbtree.h \
  /media/root/robcore/android/machinex/include/linux/prio_tree.h \
  /media/root/robcore/android/machinex/include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  /media/root/robcore/android/machinex/include/linux/mm_types.h \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/have/cmpxchg/double.h) \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/want/page/debug/flags.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mm/owner.h) \
    $(wildcard include/config/mmu/notifier.h) \
  /media/root/robcore/android/machinex/include/linux/auxvec.h \
  arch/arm/include/generated/asm/auxvec.h \
  /media/root/robcore/android/machinex/include/asm-generic/auxvec.h \
  /media/root/robcore/android/machinex/include/linux/page-debug-flags.h \
    $(wildcard include/config/page/poisoning.h) \
    $(wildcard include/config/page/guard.h) \
    $(wildcard include/config/page/debug/something/else.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/mmu.h \
    $(wildcard include/config/cpu/has/asid.h) \
  /media/root/robcore/android/machinex/include/linux/range.h \
  /media/root/robcore/android/machinex/include/linux/bit_spinlock.h \
  /media/root/robcore/android/machinex/include/linux/shrinker.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable.h \
    $(wildcard include/config/highpte.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/proc-fns.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/glue-proc.h \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm7tdmi.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/arm720t.h) \
    $(wildcard include/config/cpu/arm740t.h) \
    $(wildcard include/config/cpu/arm9tdmi.h) \
    $(wildcard include/config/cpu/arm920t.h) \
    $(wildcard include/config/cpu/arm922t.h) \
    $(wildcard include/config/cpu/arm925t.h) \
    $(wildcard include/config/cpu/arm926t.h) \
    $(wildcard include/config/cpu/arm940t.h) \
    $(wildcard include/config/cpu/arm946e.h) \
    $(wildcard include/config/cpu/arm1020.h) \
    $(wildcard include/config/cpu/arm1020e.h) \
    $(wildcard include/config/cpu/arm1022.h) \
    $(wildcard include/config/cpu/arm1026.h) \
    $(wildcard include/config/cpu/mohawk.h) \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/cpu/v6k.h) \
    $(wildcard include/config/cpu/v7.h) \
  /media/root/robcore/android/machinex/include/asm-generic/pgtable-nopud.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable-hwdef.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable-2level-hwdef.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable-2level.h \
  /media/root/robcore/android/machinex/include/asm-generic/pgtable.h \
  /media/root/robcore/android/machinex/include/linux/page-flags.h \
    $(wildcard include/config/pageflags/extended.h) \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/scfs/lower/pagecache/invalidation.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/s390.h) \
  /media/root/robcore/android/machinex/include/linux/huge_mm.h \
  /media/root/robcore/android/machinex/include/linux/vmstat.h \
    $(wildcard include/config/vm/event/counters.h) \
  /media/root/robcore/android/machinex/include/linux/vm_event_item.h \
  /media/root/robcore/android/machinex/include/linux/tty.h \
    $(wildcard include/config/audit.h) \
  /media/root/robcore/android/machinex/include/linux/fs.h \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/fs/posix/acl.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/fsnotify.h) \
    $(wildcard include/config/ima.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/debug/writecount.h) \
    $(wildcard include/config/file/locking.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/fs/xip.h) \
    $(wildcard include/config/migration.h) \
  /media/root/robcore/android/machinex/include/linux/limits.h \
  /media/root/robcore/android/machinex/include/linux/ioctl.h \
  arch/arm/include/generated/asm/ioctl.h \
  /media/root/robcore/android/machinex/include/asm-generic/ioctl.h \
  /media/root/robcore/android/machinex/include/linux/blk_types.h \
    $(wildcard include/config/blk/dev/integrity.h) \
  /media/root/robcore/android/machinex/include/linux/kdev_t.h \
  /media/root/robcore/android/machinex/include/linux/dcache.h \
  /media/root/robcore/android/machinex/include/linux/rculist.h \
  /media/root/robcore/android/machinex/include/linux/rculist_bl.h \
  /media/root/robcore/android/machinex/include/linux/list_bl.h \
  /media/root/robcore/android/machinex/include/linux/path.h \
  /media/root/robcore/android/machinex/include/linux/stat.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/stat.h \
  /media/root/robcore/android/machinex/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /media/root/robcore/android/machinex/include/linux/math64.h \
  /media/root/robcore/android/machinex/include/linux/radix-tree.h \
  /media/root/robcore/android/machinex/include/linux/pid.h \
  /media/root/robcore/android/machinex/include/linux/capability.h \
  /media/root/robcore/android/machinex/include/linux/semaphore.h \
  /media/root/robcore/android/machinex/include/linux/fiemap.h \
  /media/root/robcore/android/machinex/include/linux/migrate_mode.h \
  /media/root/robcore/android/machinex/include/linux/quota.h \
    $(wildcard include/config/quota/netlink/interface.h) \
  /media/root/robcore/android/machinex/include/linux/percpu_counter.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_xfs.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_v1.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_v2.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_qtree.h \
  /media/root/robcore/android/machinex/include/linux/nfs_fs_i.h \
  /media/root/robcore/android/machinex/include/linux/fcntl.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/fcntl.h \
  /media/root/robcore/android/machinex/include/asm-generic/fcntl.h \
  /media/root/robcore/android/machinex/include/linux/err.h \
  /media/root/robcore/android/machinex/include/linux/major.h \
  /media/root/robcore/android/machinex/include/linux/termios.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/termios.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/termbits.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/ioctls.h \
  /media/root/robcore/android/machinex/include/asm-generic/ioctls.h \
  /media/root/robcore/android/machinex/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
  /media/root/robcore/android/machinex/include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  /media/root/robcore/android/machinex/include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  /media/root/robcore/android/machinex/include/linux/jiffies.h \
  /media/root/robcore/android/machinex/include/linux/timex.h \
  /media/root/robcore/android/machinex/include/linux/param.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/timex.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/timex.h \
    $(wildcard include/config/have/arch/has/current/timer.h) \
  /media/root/robcore/android/machinex/include/linux/tty_driver.h \
    $(wildcard include/config/console/poll.h) \
  /media/root/robcore/android/machinex/include/linux/export.h \
    $(wildcard include/config/symbol/prefix.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /media/root/robcore/android/machinex/include/linux/cdev.h \
  /media/root/robcore/android/machinex/include/linux/kobject.h \
  /media/root/robcore/android/machinex/include/linux/sysfs.h \
  /media/root/robcore/android/machinex/include/linux/kobject_ns.h \
  /media/root/robcore/android/machinex/include/linux/kref.h \
  /media/root/robcore/android/machinex/include/linux/tty_ldisc.h \
  /media/root/robcore/android/machinex/include/linux/pps_kernel.h \
    $(wildcard include/config/ntp/pps.h) \
  /media/root/robcore/android/machinex/include/linux/pps.h \
  /media/root/robcore/android/machinex/include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
    $(wildcard include/config/devtmpfs.h) \
    $(wildcard include/config/sysfs/deprecated.h) \
  /media/root/robcore/android/machinex/include/linux/ioport.h \
  /media/root/robcore/android/machinex/include/linux/klist.h \
  /media/root/robcore/android/machinex/include/linux/pm.h \
    $(wildcard include/config/pm.h) \
    $(wildcard include/config/pm/runtime.h) \
    $(wildcard include/config/pm/clk.h) \
    $(wildcard include/config/pm/generic/domains.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/device.h \
    $(wildcard include/config/dmabounce.h) \
    $(wildcard include/config/iommu/api.h) \
    $(wildcard include/config/arm/dma/use/iommu.h) \
    $(wildcard include/config/arch/omap.h) \
  /media/root/robcore/android/machinex/include/linux/pm_wakeup.h \
  /media/root/robcore/android/machinex/include/linux/console.h \
    $(wildcard include/config/hw/console.h) \
    $(wildcard include/config/vga/console.h) \
  /media/root/robcore/android/machinex/include/linux/nmi.h \
    $(wildcard include/config/have/nmi/watchdog.h) \
    $(wildcard include/config/hardlockup/detector.h) \
    $(wildcard include/config/lockup/detector.h) \
  /media/root/robcore/android/machinex/include/linux/sched.h \
    $(wildcard include/config/runtime/compcache.h) \
    $(wildcard include/config/sched/debug.h) \
    $(wildcard include/config/no/hz.h) \
    $(wildcard include/config/detect/hung/task.h) \
    $(wildcard include/config/core/dump/default/elf/headers.h) \
    $(wildcard include/config/sched/autogroup.h) \
    $(wildcard include/config/virt/cpu/accounting.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/samp/hotness.h) \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/fanotify.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/perf/events.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/rt/group/sched.h) \
    $(wildcard include/config/cgroup/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/compat/brk.h) \
    $(wildcard include/config/generic/hardirqs.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/latencytop.h) \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/have/unstable/sched/clock.h) \
    $(wildcard include/config/irq/time/accounting.h) \
    $(wildcard include/config/cfs/bandwidth.h) \
    $(wildcard include/config/debug/stack/usage.h) \
  arch/arm/include/generated/asm/cputime.h \
  /media/root/robcore/android/machinex/include/asm-generic/cputime.h \
  /media/root/robcore/android/machinex/include/linux/sem.h \
  /media/root/robcore/android/machinex/include/linux/ipc.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/ipcbuf.h \
  /media/root/robcore/android/machinex/include/asm-generic/ipcbuf.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/sembuf.h \
  /media/root/robcore/android/machinex/include/linux/signal.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/signal.h \
  /media/root/robcore/android/machinex/include/asm-generic/signal-defs.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/sigcontext.h \
  arch/arm/include/generated/asm/siginfo.h \
  /media/root/robcore/android/machinex/include/asm-generic/siginfo.h \
  /media/root/robcore/android/machinex/include/linux/proportions.h \
  /media/root/robcore/android/machinex/include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
  /media/root/robcore/android/machinex/include/linux/rtmutex.h \
    $(wildcard include/config/debug/rt/mutexes.h) \
  /media/root/robcore/android/machinex/include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  /media/root/robcore/android/machinex/include/linux/resource.h \
  arch/arm/include/generated/asm/resource.h \
  /media/root/robcore/android/machinex/include/asm-generic/resource.h \
  /media/root/robcore/android/machinex/include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
    $(wildcard include/config/timerfd.h) \
  /media/root/robcore/android/machinex/include/linux/timerqueue.h \
  /media/root/robcore/android/machinex/include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  /media/root/robcore/android/machinex/include/linux/latencytop.h \
  /media/root/robcore/android/machinex/include/linux/cred.h \
    $(wildcard include/config/debug/credentials.h) \
    $(wildcard include/config/user/ns.h) \
  /media/root/robcore/android/machinex/include/linux/key.h \
  /media/root/robcore/android/machinex/include/linux/sysctl.h \
  /media/root/robcore/android/machinex/include/linux/selinux.h \
    $(wildcard include/config/security/selinux.h) \
  /media/root/robcore/android/machinex/include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /media/root/robcore/android/machinex/include/linux/aio.h \
  /media/root/robcore/android/machinex/include/linux/aio_abi.h \
  /media/root/robcore/android/machinex/include/linux/uio.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/irq.h \
    $(wildcard include/config/sparse/irq.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/irqs.h \
    $(wildcard include/config/arch/apq8064.h) \
    $(wildcard include/config/arch/msm8930.h) \
    $(wildcard include/config/mfd/max77693.h) \
    $(wildcard include/config/pci/msi.h) \
    $(wildcard include/config/arch/msm8974.h) \
    $(wildcard include/config/arch/msm9615.h) \
    $(wildcard include/config/arch/msm9625.h) \
    $(wildcard include/config/arch/msm8226.h) \
    $(wildcard include/config/arch/qsd8x50.h) \
    $(wildcard include/config/arch/msm7x01a.h) \
    $(wildcard include/config/arch/msm7x25.h) \
    $(wildcard include/config/arch/fsm9xxx.h) \
    $(wildcard include/config/msm/pcie.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/irqs-8625.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/irqs-8960.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/irqs-8064.h \
  /media/root/robcore/android/machinex/include/linux/module.h \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/tracepoints.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/debug/set/module/ronx.h) \
  /media/root/robcore/android/machinex/include/linux/kmod.h \
  /media/root/robcore/android/machinex/include/linux/elf.h \
  /media/root/robcore/android/machinex/include/linux/elf-em.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/elf.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/user.h \
  /media/root/robcore/android/machinex/include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ppc64.h) \
  /media/root/robcore/android/machinex/include/linux/tracepoint.h \
  /media/root/robcore/android/machinex/include/linux/static_key.h \
  /media/root/robcore/android/machinex/include/linux/jump_label.h \
    $(wildcard include/config/jump/label.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/module.h \
    $(wildcard include/config/arm/unwind.h) \
  /media/root/robcore/android/machinex/include/linux/interrupt.h \
    $(wildcard include/config/irq/forced/threading.h) \
    $(wildcard include/config/generic/irq/probe.h) \
  /media/root/robcore/android/machinex/include/linux/irqreturn.h \
  /media/root/robcore/android/machinex/include/linux/irqnr.h \
  /media/root/robcore/android/machinex/include/linux/hardirq.h \
  /media/root/robcore/android/machinex/include/linux/ftrace_irq.h \
    $(wildcard include/config/ftrace/nmi/enter.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/hardirq.h \
  /media/root/robcore/android/machinex/include/linux/irq_cpustat.h \
  /media/root/robcore/android/machinex/include/linux/delay.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/delay.h \
  /media/root/robcore/android/machinex/include/linux/security.h \
    $(wildcard include/config/security/path.h) \
    $(wildcard include/config/security/network.h) \
    $(wildcard include/config/security/network/xfrm.h) \
    $(wildcard include/config/securityfs.h) \
  /media/root/robcore/android/machinex/include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/failslab.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/slab.h) \
  /media/root/robcore/android/machinex/include/linux/slub_def.h \
    $(wildcard include/config/slub/stats.h) \
    $(wildcard include/config/slub/debug.h) \
  /media/root/robcore/android/machinex/include/linux/kmemleak.h \
    $(wildcard include/config/debug/kmemleak.h) \
  /media/root/robcore/android/machinex/include/linux/bootmem.h \
    $(wildcard include/config/have/arch/bootmem/node.h) \
    $(wildcard include/config/have/arch/alloc/remap.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/dma.h \
    $(wildcard include/config/isa/dma/api.h) \
    $(wildcard include/config/pci.h) \
  /media/root/robcore/android/machinex/include/linux/memblock.h \
    $(wildcard include/config/have/memblock.h) \
    $(wildcard include/config/arch/discard/memblock.h) \
  /media/root/robcore/android/machinex/include/linux/syscalls.h \
    $(wildcard include/config/ftrace/syscalls.h) \
    $(wildcard include/config/mips.h) \
    $(wildcard include/config/have/syscall/wrappers.h) \
  /media/root/robcore/android/machinex/include/linux/unistd.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/unistd.h \
    $(wildcard include/config/oabi/compat.h) \
  /media/root/robcore/android/machinex/include/trace/syscall.h \
    $(wildcard include/config/have/syscall/tracepoints.h) \
  /media/root/robcore/android/machinex/include/linux/ftrace_event.h \
  /media/root/robcore/android/machinex/include/linux/ring_buffer.h \
    $(wildcard include/config/ring/buffer/allow/swap.h) \
  /media/root/robcore/android/machinex/include/linux/kmemcheck.h \
  /media/root/robcore/android/machinex/include/linux/seq_file.h \
  /media/root/robcore/android/machinex/include/linux/trace_seq.h \
  /media/root/robcore/android/machinex/include/linux/perf_event.h \
    $(wildcard include/config/cgroup/perf.h) \
    $(wildcard include/config/function/tracer.h) \
    $(wildcard include/config/cpu/sup/intel.h) \
  /media/root/robcore/android/machinex/include/linux/cgroup.h \
  /media/root/robcore/android/machinex/include/linux/cgroupstats.h \
  /media/root/robcore/android/machinex/include/linux/taskstats.h \
  /media/root/robcore/android/machinex/include/linux/prio_heap.h \
  /media/root/robcore/android/machinex/include/linux/idr.h \
  /media/root/robcore/android/machinex/include/linux/cgroup_subsys.h \
    $(wildcard include/config/cgroup/debug.h) \
    $(wildcard include/config/cgroup/cpuacct.h) \
    $(wildcard include/config/cgroup/device.h) \
    $(wildcard include/config/cgroup/freezer.h) \
    $(wildcard include/config/net/cls/cgroup.h) \
    $(wildcard include/config/blk/cgroup.h) \
    $(wildcard include/config/cgroup/bfqio.h) \
    $(wildcard include/config/netprio/cgroup.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/perf_event.h \
  arch/arm/include/generated/asm/local64.h \
  /media/root/robcore/android/machinex/include/asm-generic/local64.h \
  /media/root/robcore/android/machinex/include/linux/pid_namespace.h \
    $(wildcard include/config/pid/ns.h) \
  /media/root/robcore/android/machinex/include/linux/nsproxy.h \
  /media/root/robcore/android/machinex/include/linux/ftrace.h \
    $(wildcard include/config/dynamic/ftrace.h) \
    $(wildcard include/config/stack/tracer.h) \
    $(wildcard include/config/frame/pointer.h) \
  /media/root/robcore/android/machinex/include/linux/trace_clock.h \
  /media/root/robcore/android/machinex/include/linux/kallsyms.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/ftrace.h \
    $(wildcard include/config/old/mcount.h) \
  /media/root/robcore/android/machinex/include/linux/cpu.h \
    $(wildcard include/config/arch/has/cpu/autoprobe.h) \
    $(wildcard include/config/arch/cpu/probe/release.h) \
    $(wildcard include/config/pm/sleep/smp.h) \
  /media/root/robcore/android/machinex/include/linux/node.h \
  /media/root/robcore/android/machinex/include/linux/irq_work.h \
  arch/arm/include/generated/asm/local.h \
  /media/root/robcore/android/machinex/include/asm-generic/local.h \
  /media/root/robcore/android/machinex/include/linux/kexec.h \
    $(wildcard include/config/kexec/hardboot.h) \
    $(wildcard include/config/.h) \
    $(wildcard include/config/kexec/jump.h) \
  /media/root/robcore/android/machinex/include/linux/compat.h \
    $(wildcard include/config/arch/want/old/compat/ipc.h) \
  /media/root/robcore/android/machinex/include/linux/elfcore.h \
    $(wildcard include/config/binfmt/elf/fdpic.h) \
  /media/root/robcore/android/machinex/include/linux/user.h \
  /media/root/robcore/android/machinex/include/linux/ptrace.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/kexec.h \
    $(wildcard include/config/kexec/hb/page/addr.h) \
  /media/root/robcore/android/machinex/include/linux/kdb.h \
  /media/root/robcore/android/machinex/include/linux/ratelimit.h \
  /media/root/robcore/android/machinex/include/linux/kmsg_dump.h \
  /media/root/robcore/android/machinex/include/linux/syslog.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/uaccess.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/sec_debug.h \
    $(wildcard include/config/sec/peripheral/secure/chk.h) \
    $(wildcard include/config/mach/jf.h) \
    $(wildcard include/config/sec/debug/sched/log.h) \
    $(wildcard include/config/sec/debug/irq/exit/log.h) \
    $(wildcard include/config/sec/debug/semaphore/log.h) \
    $(wildcard include/config/sec/debug/msg/log.h) \
    $(wildcard include/config/sec/debug/avc/log.h) \
    $(wildcard include/config/sec/debug/dcvs/log.h) \
    $(wildcard include/config/sec/debug/fuelgauge/log.h) \
    $(wildcard include/config/sec/monitor/battery/removal.h) \
    $(wildcard include/config/sec/debug/mdm/file/info.h) \
    $(wildcard include/config/sec/debug/double/free.h) \
  /media/root/robcore/android/machinex/include/linux/io.h \
    $(wildcard include/config/has/ioport.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/io.h \
    $(wildcard include/config/need/mach/io/h.h) \
    $(wildcard include/config/pcmcia/soc/common.h) \
    $(wildcard include/config/isa.h) \
    $(wildcard include/config/pccard.h) \
  /media/root/robcore/android/machinex/include/asm-generic/pci_iomap.h \
    $(wildcard include/config/no/generic/pci/ioport/map.h) \
    $(wildcard include/config/generic/pci/iomap.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_rtb.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/io.h \
  /media/root/robcore/android/machinex/include/linux/proc_fs.h \
    $(wildcard include/config/proc/devicetree.h) \
    $(wildcard include/config/proc/kcore.h) \
  /media/root/robcore/android/machinex/include/linux/magic.h \
  /media/root/robcore/android/machinex/include/trace/events/printk.h \
  /media/root/robcore/android/machinex/include/trace/define_trace.h \
  /media/root/robcore/android/machinex/include/trace/ftrace.h \
  /media/root/robcore/android/machinex/include/linux/coresight-stm.h \
    $(wildcard include/config/coresight/stm.h) \

kernel/printk.o: $(deps_kernel/printk.o)

$(deps_kernel/printk.o):
