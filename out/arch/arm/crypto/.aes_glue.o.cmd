cmd_arch/arm/crypto/aes_glue.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,arch/arm/crypto/.aes_glue.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/arch/arm/crypto -Iarch/arm/crypto -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-unused-value -Wno-format-security -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Werror-implicit-function-declaration -Wno-uninitialized -Wno-format-security -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -fno-delete-null-pointer-checks -Wno-maybe-uninitialized -O3 -marm -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -fstack-protector -fno-conserve-stack -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=1048 -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -munaligned-access -mfpu=neon-vfpv4 -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(aes_glue)"  -D"KBUILD_MODNAME=KBUILD_STR(aes_arm)" -c -o arch/arm/crypto/.tmp_aes_glue.o /media/root/robcore/android/machinex/arch/arm/crypto/aes_glue.c

source_arch/arm/crypto/aes_glue.o := /media/root/robcore/android/machinex/arch/arm/crypto/aes_glue.c

deps_arch/arm/crypto/aes_glue.o := \
  /media/root/robcore/android/machinex/include/linux/module.h \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/tracepoints.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/debug/set/module/ronx.h) \
  /media/root/robcore/android/machinex/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /media/root/robcore/android/machinex/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
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
  /media/root/robcore/android/machinex/include/linux/compiler-gcc4.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/posix_types.h \
  /media/root/robcore/android/machinex/include/asm-generic/posix_types.h \
  /media/root/robcore/android/machinex/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  /media/root/robcore/android/machinex/include/linux/const.h \
  /media/root/robcore/android/machinex/include/linux/stat.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/stat.h \
  /media/root/robcore/android/machinex/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /media/root/robcore/android/machinex/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /media/root/robcore/android/machinex/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/lge/crash/handler.h) \
    $(wildcard include/config/cpu/cp15/mmu.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/compaction.h) \
  /media/root/robcore/android/machinex/include/linux/sysinfo.h \
  /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/include/stdarg.h \
  /media/root/robcore/android/machinex/include/linux/linkage.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/include/linux/bitops.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/bitops.h \
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
    $(wildcard include/config/sec/ssr/dump.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  /media/root/robcore/android/machinex/include/linux/init.h \
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
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  /media/root/robcore/android/machinex/include/linux/seqlock.h \
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
    $(wildcard include/config/mmu.h) \
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
  /media/root/robcore/android/machinex/include/linux/math64.h \
  /media/root/robcore/android/machinex/include/linux/kmod.h \
  /media/root/robcore/android/machinex/include/linux/gfp.h \
    $(wildcard include/config/kmemcheck.h) \
    $(wildcard include/config/cma.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/pm/sleep.h) \
  /media/root/robcore/android/machinex/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/have/memblock/node/map.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/cgroup/mem/res/ctlr.h) \
    $(wildcard include/config/no/bootmem.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/have/memblock/node.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  /media/root/robcore/android/machinex/include/linux/wait.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/current.h \
  /media/root/robcore/android/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /media/root/robcore/android/machinex/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
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
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
    $(wildcard include/config/phys/offset.h) \
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
    $(wildcard include/config/sparsemem/vmemmap.h) \
  /media/root/robcore/android/machinex/include/asm-generic/getorder.h \
  /media/root/robcore/android/machinex/include/linux/memory_hotplug.h \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  /media/root/robcore/android/machinex/include/linux/notifier.h \
  /media/root/robcore/android/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno-base.h \
  /media/root/robcore/android/machinex/include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  /media/root/robcore/android/machinex/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  /media/root/robcore/android/machinex/include/linux/rwsem-spinlock.h \
    $(wildcard include/config/sec/forkhang/debug.h) \
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
  /media/root/robcore/android/machinex/include/linux/sysctl.h \
    $(wildcard include/config/sysctl.h) \
  /media/root/robcore/android/machinex/include/linux/rbtree.h \
  /media/root/robcore/android/machinex/include/linux/elf.h \
  /media/root/robcore/android/machinex/include/linux/elf-em.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/elf.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/user.h \
  /media/root/robcore/android/machinex/include/linux/kobject.h \
  /media/root/robcore/android/machinex/include/linux/sysfs.h \
  /media/root/robcore/android/machinex/include/linux/kobject_ns.h \
  /media/root/robcore/android/machinex/include/linux/kref.h \
  /media/root/robcore/android/machinex/include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  /media/root/robcore/android/machinex/include/linux/tracepoint.h \
  /media/root/robcore/android/machinex/include/linux/static_key.h \
  /media/root/robcore/android/machinex/include/linux/jump_label.h \
    $(wildcard include/config/jump/label.h) \
  /media/root/robcore/android/machinex/include/linux/export.h \
    $(wildcard include/config/symbol/prefix.h) \
    $(wildcard include/config/modversions.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/module.h \
    $(wildcard include/config/arm/unwind.h) \
  /media/root/robcore/android/machinex/include/linux/crypto.h \
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
  /media/root/robcore/android/machinex/include/linux/uaccess.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/uaccess.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
  /media/root/robcore/android/machinex/include/crypto/aes.h \

arch/arm/crypto/aes_glue.o: $(deps_arch/arm/crypto/aes_glue.o)

$(deps_arch/arm/crypto/aes_glue.o):
