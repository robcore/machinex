cmd_sound/core/pcm_lib.o := /root/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,sound/core/.pcm_lib.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/root/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/root/machinex/include -include /root/machinex/include/linux/kconfig.h  -I/root/machinex/sound/core -Isound/core -D__KERNEL__ -mlittle-endian   -I/root/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing -fno-common -mtune=cortex-a15 -mfpu=neon-vfpv4 -std=gnu89 -Wno-format-security -Wno-unused-function -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -fno-delete-null-pointer-checks -Wno-cpp -Wno-declaration-after-statement -fno-var-tracking-assignments -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Wno-sequence-point -O3 -marm -fno-dwarf2-cfi-asm -fstack-protector -fno-conserve-stack -fno-ipa-sra -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -mtune=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=2048 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -mcpu=cortex-a15 -mtune=cortex-a15 -mfpu=neon-vfpv4  -mvectorize-with-neon-quad -munaligned-access -fgraphite-identity -floop-parallelize-all -ftree-loop-linear -floop-interchange -floop-strip-mine -floop-block -ftree-vectorize -funroll-loops -fno-align-functions -fno-align-jumps -fno-align-loops -fno-align-labels -fno-prefetch-loop-arrays -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(pcm_lib)"  -D"KBUILD_MODNAME=KBUILD_STR(snd_pcm)" -c -o sound/core/.tmp_pcm_lib.o /root/machinex/sound/core/pcm_lib.c

source_sound/core/pcm_lib.o := /root/machinex/sound/core/pcm_lib.c

deps_sound/core/pcm_lib.o := \
    $(wildcard include/config/snd/debug.h) \
    $(wildcard include/config/snd/pcm/xrun/debug.h) \
  /root/machinex/include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/kmemcheck.h) \
    $(wildcard include/config/failslab.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/slab.h) \
    $(wildcard include/config/tracing.h) \
  /root/machinex/include/linux/gfp.h \
    $(wildcard include/config/cma.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/pm/sleep.h) \
  /root/machinex/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/compaction.h) \
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
  /root/machinex/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  /root/machinex/include/linux/typecheck.h \
  /root/machinex/include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  /root/machinex/include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /root/machinex/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /root/machinex/arch/arm/include/asm/types.h \
  /root/machinex/include/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  /root/machinex/include/asm-generic/bitsperlong.h \
  /root/machinex/include/linux/posix_types.h \
  /root/machinex/include/linux/stddef.h \
  /root/machinex/include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /root/machinex/include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  /root/machinex/include/linux/compiler-gcc5.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /root/machinex/arch/arm/include/asm/posix_types.h \
  /root/machinex/include/asm-generic/posix_types.h \
  /root/machinex/include/linux/bitops.h \
  /root/machinex/arch/arm/include/asm/bitops.h \
  /root/machinex/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /root/machinex/arch/arm/include/asm/irqflags.h \
  /root/machinex/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /root/machinex/arch/arm/include/asm/hwcap.h \
  /root/machinex/include/asm-generic/bitops/non-atomic.h \
  /root/machinex/include/asm-generic/bitops/fls64.h \
  /root/machinex/include/asm-generic/bitops/sched.h \
  /root/machinex/include/asm-generic/bitops/hweight.h \
  /root/machinex/include/asm-generic/bitops/arch_hweight.h \
  /root/machinex/include/asm-generic/bitops/const_hweight.h \
  /root/machinex/include/asm-generic/bitops/lock.h \
  /root/machinex/include/asm-generic/bitops/le.h \
  /root/machinex/arch/arm/include/asm/byteorder.h \
  /root/machinex/include/linux/byteorder/little_endian.h \
  /root/machinex/include/linux/swab.h \
  /root/machinex/arch/arm/include/asm/swab.h \
  /root/machinex/include/linux/byteorder/generic.h \
  /root/machinex/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /root/machinex/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /root/machinex/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  /root/machinex/arch/arm/include/asm/domain.h \
    $(wildcard include/config/verify/permission/fault.h) \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /root/machinex/arch/arm/include/asm/barrier.h \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/arch/has/barriers.h) \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
  /root/machinex/arch/arm/include/asm/outercache.h \
    $(wildcard include/config/outer/cache/sync.h) \
    $(wildcard include/config/outer/cache.h) \
  /root/machinex/include/linux/linkage.h \
  /root/machinex/arch/arm/include/asm/linkage.h \
  /root/machinex/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /root/machinex/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  /root/machinex/include/linux/const.h \
  /root/machinex/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/lge/crash/handler.h) \
    $(wildcard include/config/cpu/cp15/mmu.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /root/machinex/include/linux/sysinfo.h \
  /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include/stdarg.h \
  /root/machinex/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /root/machinex/include/linux/printk.h \
    $(wildcard include/config/sec/ssr/dump.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  /root/machinex/include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  /root/machinex/include/linux/dynamic_debug.h \
  /root/machinex/arch/arm/include/asm/div64.h \
  /root/machinex/arch/arm/include/asm/compiler.h \
  /root/machinex/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/debug/bugverbose.h) \
    $(wildcard include/config/arm/lpae.h) \
  /root/machinex/include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /root/machinex/include/linux/stringify.h \
  /root/machinex/include/linux/bottom_half.h \
  /root/machinex/include/linux/spinlock_types.h \
  /root/machinex/arch/arm/include/asm/spinlock_types.h \
  /root/machinex/include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/prove/rcu.h) \
  /root/machinex/include/linux/rwlock_types.h \
  /root/machinex/arch/arm/include/asm/spinlock.h \
    $(wildcard include/config/msm/krait/wfe/fixup.h) \
    $(wildcard include/config/arm/ticket/locks.h) \
  /root/machinex/arch/arm/include/asm/processor.h \
    $(wildcard include/config/have/hw/breakpoint.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/arm/errata/754327.h) \
  /root/machinex/arch/arm/include/asm/hw_breakpoint.h \
  /root/machinex/include/asm-generic/processor.h \
  /root/machinex/include/asm-generic/relaxed.h \
  /root/machinex/include/linux/rwlock.h \
  /root/machinex/include/linux/spinlock_api_smp.h \
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
  /root/machinex/include/linux/rwlock_api_smp.h \
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
  /root/machinex/include/linux/atomic.h \
    $(wildcard include/config/arch/has/atomic/or.h) \
    $(wildcard include/config/generic/atomic64.h) \
  /root/machinex/arch/arm/include/asm/atomic.h \
  /root/machinex/include/linux/prefetch.h \
  /root/machinex/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  /root/machinex/arch/arm/include/asm/cmpxchg.h \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/v6.h) \
  /root/machinex/include/asm-generic/cmpxchg-local.h \
  /root/machinex/include/asm-generic/atomic-long.h \
  /root/machinex/include/linux/wait.h \
  /root/machinex/arch/arm/include/asm/current.h \
  /root/machinex/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /root/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /root/machinex/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  /root/machinex/include/linux/seqlock.h \
  /root/machinex/arch/arm/include/asm/relaxed.h \
  /root/machinex/include/linux/nodemask.h \
  /root/machinex/include/linux/bitmap.h \
  /root/machinex/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /root/machinex/arch/arm/include/asm/string.h \
  /root/machinex/include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/generated/bounds.h \
  /root/machinex/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/fa.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/kuser/helpers.h) \
    $(wildcard include/config/memory/hotplug/sparse.h) \
  /root/machinex/arch/arm/include/asm/glue.h \
  /root/machinex/arch/arm/include/asm/pgtable-2level-types.h \
  /root/machinex/arch/arm/include/asm/memory.h \
    $(wildcard include/config/need/mach/memory/h.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/phys/offset.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
  arch/arm/include/generated/asm/sizes.h \
  /root/machinex/include/asm-generic/sizes.h \
  /root/machinex/include/linux/sizes.h \
  /root/machinex/arch/arm/mach-msm/include/mach/memory.h \
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
  /root/machinex/include/asm-generic/memory_model.h \
    $(wildcard include/config/sparsemem/vmemmap.h) \
  /root/machinex/include/asm-generic/getorder.h \
  /root/machinex/include/linux/memory_hotplug.h \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  /root/machinex/include/linux/notifier.h \
  /root/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /root/machinex/include/asm-generic/errno.h \
  /root/machinex/include/asm-generic/errno-base.h \
  /root/machinex/include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  /root/machinex/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  /root/machinex/arch/arm/include/asm/rwsem.h \
  /root/machinex/arch/arm/include/asm/system.h \
  /root/machinex/arch/arm/include/asm/exec.h \
  /root/machinex/arch/arm/include/asm/switch_to.h \
  /root/machinex/arch/arm/include/asm/system_info.h \
    $(wildcard include/config/sec/debug/subsys.h) \
  /root/machinex/arch/arm/include/asm/system_misc.h \
  /root/machinex/include/linux/srcu.h \
  /root/machinex/include/linux/rcupdate.h \
    $(wildcard include/config/rcu/torture/test.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tree/preempt/rcu.h) \
    $(wildcard include/config/rcu/trace.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/rcu/user/qs.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/tiny/preempt/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/preempt/rt.h) \
  /root/machinex/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  /root/machinex/include/linux/bug.h \
  /root/machinex/include/linux/completion.h \
  /root/machinex/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects/free.h) \
  /root/machinex/include/linux/rcutree.h \
  /root/machinex/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
  /root/machinex/include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  /root/machinex/include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  /root/machinex/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /root/machinex/include/linux/math64.h \
  /root/machinex/include/linux/jiffies.h \
  /root/machinex/include/linux/timex.h \
  /root/machinex/include/linux/param.h \
  /root/machinex/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /root/machinex/arch/arm/include/asm/timex.h \
  /root/machinex/arch/arm/mach-msm/include/mach/timex.h \
    $(wildcard include/config/have/arch/has/current/timer.h) \
  /root/machinex/arch/arm/include/asm/sparsemem.h \
  /root/machinex/include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
    $(wildcard include/config/sched/book.h) \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
  /root/machinex/include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  /root/machinex/arch/arm/include/asm/smp.h \
  /root/machinex/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /root/machinex/include/linux/pfn.h \
  /root/machinex/arch/arm/include/asm/percpu.h \
  /root/machinex/include/asm-generic/percpu.h \
  /root/machinex/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
  /root/machinex/arch/arm/include/asm/topology.h \
    $(wildcard include/config/arm/cpu/topology.h) \
  /root/machinex/include/asm-generic/topology.h \
  /root/machinex/include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/virtual.h) \
  /root/machinex/include/linux/slub_def.h \
    $(wildcard include/config/slub/stats.h) \
    $(wildcard include/config/slub/debug.h) \
    $(wildcard include/config/sysfs.h) \
  /root/machinex/include/linux/kobject.h \
  /root/machinex/include/linux/sysfs.h \
  /root/machinex/include/linux/kobject_ns.h \
  /root/machinex/include/linux/kref.h \
  /root/machinex/include/linux/kmemleak.h \
    $(wildcard include/config/debug/kmemleak.h) \
  /root/machinex/include/linux/export.h \
    $(wildcard include/config/symbol/prefix.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /root/machinex/include/sound/core.h \
    $(wildcard include/config/snd/dynamic/minors.h) \
    $(wildcard include/config/snd/major.h) \
    $(wildcard include/config/pm.h) \
    $(wildcard include/config/snd/mixer/oss.h) \
    $(wildcard include/config/snd/ossemul.h) \
    $(wildcard include/config/isa/dma/api.h) \
    $(wildcard include/config/snd/verbose/printk.h) \
    $(wildcard include/config/snd/debug/verbose.h) \
    $(wildcard include/config/gameport.h) \
    $(wildcard include/config/pci.h) \
  /root/machinex/include/linux/sched.h \
    $(wildcard include/config/intelli/hotplug.h) \
    $(wildcard include/config/msm/run/queue/stats/be/conservative.h) \
    $(wildcard include/config/runtime/compcache.h) \
    $(wildcard include/config/sched/debug.h) \
    $(wildcard include/config/no/hz.h) \
    $(wildcard include/config/lockup/detector.h) \
    $(wildcard include/config/core/dump/default/elf/headers.h) \
    $(wildcard include/config/sched/autogroup.h) \
    $(wildcard include/config/virt/cpu/accounting.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/samp/hotness.h) \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/fanotify.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/perf/events.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/rt/group/sched.h) \
    $(wildcard include/config/sched/freq/input.h) \
    $(wildcard include/config/cgroup/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/compat/brk.h) \
    $(wildcard include/config/generic/hardirqs.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/detect/hung/task.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/latencytop.h) \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/have/unstable/sched/clock.h) \
    $(wildcard include/config/irq/time/accounting.h) \
    $(wildcard include/config/proc/fs.h) \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/debug/stack/usage.h) \
    $(wildcard include/config/mm/owner.h) \
  /root/machinex/include/linux/capability.h \
  /root/machinex/include/linux/rbtree.h \
  /root/machinex/include/linux/mm_types.h \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/have/cmpxchg/double.h) \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/want/page/debug/flags.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mmu/notifier.h) \
    $(wildcard include/config/transparent/hugepage.h) \
  /root/machinex/include/linux/auxvec.h \
  arch/arm/include/generated/asm/auxvec.h \
  /root/machinex/include/asm-generic/auxvec.h \
  /root/machinex/include/linux/radix-tree.h \
  /root/machinex/include/linux/prio_tree.h \
  /root/machinex/include/linux/page-debug-flags.h \
    $(wildcard include/config/page/poisoning.h) \
    $(wildcard include/config/page/guard.h) \
    $(wildcard include/config/page/debug/something/else.h) \
  /root/machinex/arch/arm/include/asm/mmu.h \
    $(wildcard include/config/cpu/has/asid.h) \
  arch/arm/include/generated/asm/cputime.h \
  /root/machinex/include/asm-generic/cputime.h \
  /root/machinex/include/linux/sem.h \
  /root/machinex/include/linux/ipc.h \
  /root/machinex/arch/arm/include/asm/ipcbuf.h \
  /root/machinex/include/asm-generic/ipcbuf.h \
  /root/machinex/arch/arm/include/asm/sembuf.h \
  /root/machinex/include/linux/signal.h \
  /root/machinex/arch/arm/include/asm/signal.h \
  /root/machinex/include/asm-generic/signal-defs.h \
  /root/machinex/arch/arm/include/asm/sigcontext.h \
  arch/arm/include/generated/asm/siginfo.h \
  /root/machinex/include/asm-generic/siginfo.h \
  /root/machinex/include/linux/pid.h \
  /root/machinex/include/linux/proportions.h \
  /root/machinex/include/linux/percpu_counter.h \
  /root/machinex/include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
    $(wildcard include/config/seccomp/filter.h) \
  /root/machinex/include/linux/rculist.h \
  /root/machinex/include/linux/rtmutex.h \
    $(wildcard include/config/debug/rt/mutexes.h) \
  /root/machinex/include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  /root/machinex/include/linux/resource.h \
  arch/arm/include/generated/asm/resource.h \
  /root/machinex/include/asm-generic/resource.h \
  /root/machinex/include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
    $(wildcard include/config/timerfd.h) \
  /root/machinex/include/linux/timerqueue.h \
  /root/machinex/include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  /root/machinex/include/linux/latencytop.h \
  /root/machinex/include/linux/cred.h \
    $(wildcard include/config/debug/credentials.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/user/ns.h) \
  /root/machinex/include/linux/key.h \
    $(wildcard include/config/sysctl.h) \
  /root/machinex/include/linux/sysctl.h \
  /root/machinex/include/linux/selinux.h \
    $(wildcard include/config/security/selinux.h) \
  /root/machinex/include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /root/machinex/include/linux/aio.h \
  /root/machinex/include/linux/aio_abi.h \
  /root/machinex/include/linux/uio.h \
  /root/machinex/include/linux/pm.h \
    $(wildcard include/config/pm/runtime.h) \
    $(wildcard include/config/pm/clk.h) \
    $(wildcard include/config/pm/generic/domains.h) \
  /root/machinex/include/sound/control.h \
  /root/machinex/include/sound/asound.h \
  /root/machinex/include/linux/ioctl.h \
  arch/arm/include/generated/asm/ioctl.h \
  /root/machinex/include/asm-generic/ioctl.h \
  /root/machinex/include/sound/info.h \
  /root/machinex/include/linux/poll.h \
  arch/arm/include/generated/asm/poll.h \
  /root/machinex/include/asm-generic/poll.h \
  /root/machinex/include/linux/fs.h \
    $(wildcard include/config/fs/posix/acl.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/fsnotify.h) \
    $(wildcard include/config/ima.h) \
    $(wildcard include/config/debug/writecount.h) \
    $(wildcard include/config/file/locking.h) \
    $(wildcard include/config/fs/xip.h) \
    $(wildcard include/config/migration.h) \
  /root/machinex/include/linux/limits.h \
  /root/machinex/include/linux/blk_types.h \
    $(wildcard include/config/blk/dev/integrity.h) \
  /root/machinex/include/linux/kdev_t.h \
  /root/machinex/include/linux/dcache.h \
  /root/machinex/include/linux/rculist_bl.h \
  /root/machinex/include/linux/list_bl.h \
  /root/machinex/include/linux/bit_spinlock.h \
  /root/machinex/include/linux/path.h \
  /root/machinex/include/linux/stat.h \
  /root/machinex/arch/arm/include/asm/stat.h \
  /root/machinex/include/linux/semaphore.h \
  /root/machinex/include/linux/fiemap.h \
  /root/machinex/include/linux/shrinker.h \
  /root/machinex/include/linux/migrate_mode.h \
  /root/machinex/include/linux/quota.h \
    $(wildcard include/config/quota/netlink/interface.h) \
  /root/machinex/include/linux/dqblk_xfs.h \
  /root/machinex/include/linux/dqblk_v1.h \
  /root/machinex/include/linux/dqblk_v2.h \
  /root/machinex/include/linux/dqblk_qtree.h \
  /root/machinex/include/linux/nfs_fs_i.h \
  /root/machinex/include/linux/fcntl.h \
  /root/machinex/arch/arm/include/asm/fcntl.h \
  /root/machinex/include/asm-generic/fcntl.h \
    $(wildcard include/config/scfs/lower/pagecache/invalidation.h) \
  /root/machinex/include/linux/err.h \
  /root/machinex/arch/arm/include/asm/uaccess.h \
  /root/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
  /root/machinex/include/sound/pcm.h \
    $(wildcard include/config/snd/pcm/oss.h) \
    $(wildcard include/config/snd/verbose/procfs.h) \
    $(wildcard include/config/snd/dma/sgbuf.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/ppc.h) \
    $(wildcard include/config/alpha.h) \
  /root/machinex/include/sound/memalloc.h \
  /root/machinex/include/sound/minors.h \
  /root/machinex/include/linux/mm.h \
    $(wildcard include/config/fix/movable/zone.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ksm.h) \
    $(wildcard include/config/debug/pagealloc.h) \
    $(wildcard include/config/hibernation.h) \
    $(wildcard include/config/use/user/accessible/timers.h) \
    $(wildcard include/config/hugetlbfs.h) \
  /root/machinex/include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  /root/machinex/include/linux/range.h \
  /root/machinex/arch/arm/include/asm/pgtable.h \
    $(wildcard include/config/highpte.h) \
  /root/machinex/arch/arm/include/asm/proc-fns.h \
  /root/machinex/arch/arm/include/asm/glue-proc.h \
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
  /root/machinex/include/asm-generic/pgtable-nopud.h \
  /root/machinex/arch/arm/include/asm/pgtable-hwdef.h \
  /root/machinex/arch/arm/include/asm/pgtable-2level-hwdef.h \
  /root/machinex/arch/arm/include/asm/pgtable-2level.h \
  /root/machinex/include/asm-generic/pgtable.h \
  /root/machinex/include/linux/page-flags.h \
    $(wildcard include/config/pageflags/extended.h) \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/ksm/check/page.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/s390.h) \
  /root/machinex/include/linux/huge_mm.h \
  /root/machinex/include/linux/vmstat.h \
    $(wildcard include/config/vm/event/counters.h) \
  /root/machinex/include/linux/vm_event_item.h \
  /root/machinex/include/linux/pm_qos.h \
  /root/machinex/include/linux/miscdevice.h \
  /root/machinex/include/linux/major.h \
  /root/machinex/include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
    $(wildcard include/config/devtmpfs.h) \
    $(wildcard include/config/sysfs/deprecated.h) \
  /root/machinex/include/linux/ioport.h \
  /root/machinex/include/linux/klist.h \
  /root/machinex/arch/arm/include/asm/device.h \
    $(wildcard include/config/dmabounce.h) \
    $(wildcard include/config/iommu/api.h) \
    $(wildcard include/config/arm/dma/use/iommu.h) \
    $(wildcard include/config/arch/omap.h) \
  /root/machinex/include/linux/pm_wakeup.h \
  /root/machinex/include/sound/pcm_params.h \
  /root/machinex/include/sound/timer.h \
  /root/machinex/include/linux/interrupt.h \
    $(wildcard include/config/irq/forced/threading.h) \
    $(wildcard include/config/generic/irq/probe.h) \
  /root/machinex/include/linux/irqreturn.h \
  /root/machinex/include/linux/irqnr.h \
  /root/machinex/include/linux/hardirq.h \
  /root/machinex/include/linux/ftrace_irq.h \
    $(wildcard include/config/ftrace/nmi/enter.h) \
  /root/machinex/arch/arm/include/asm/hardirq.h \
  /root/machinex/arch/arm/include/asm/irq.h \
    $(wildcard include/config/sparse/irq.h) \
  /root/machinex/arch/arm/mach-msm/include/mach/irqs.h \
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
  /root/machinex/arch/arm/mach-msm/include/mach/irqs-8625.h \
  /root/machinex/arch/arm/mach-msm/include/mach/irqs-8960.h \
  /root/machinex/arch/arm/mach-msm/include/mach/irqs-8064.h \
  /root/machinex/include/linux/irq_cpustat.h \

sound/core/pcm_lib.o: $(deps_sound/core/pcm_lib.o)

$(deps_sound/core/pcm_lib.o):
