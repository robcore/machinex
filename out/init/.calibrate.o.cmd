cmd_init/calibrate.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,init/.calibrate.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/init -Iinit -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing -fno-common -mtune="cortex-a15" -mfpu=neon-vfpv4 -std=gnu89 -Wno-format-security -Wno-unused-function -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -fno-delete-null-pointer-checks -Wno-cpp -Wno-declaration-after-statement -fno-var-tracking-assignments -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Wno-sequence-point -O2 -marm -fno-dwarf2-cfi-asm -fstack-protector -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=1024 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -mcpu=cortex-a15 -mtune=cortex-a15 -mfpu=neon-vfpv4 -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(calibrate)"  -D"KBUILD_MODNAME=KBUILD_STR(calibrate)" -c -o init/.tmp_calibrate.o /media/root/robcore/android/machinex/init/calibrate.c

source_init/calibrate.o := /media/root/robcore/android/machinex/init/calibrate.c

deps_init/calibrate.o := \
  /media/root/robcore/android/machinex/include/linux/jiffies.h \
  /media/root/robcore/android/machinex/include/linux/math64.h \
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
  /media/root/robcore/android/machinex/include/linux/compiler-gcc5.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/posix_types.h \
  /media/root/robcore/android/machinex/include/asm-generic/posix_types.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/div64.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/compiler.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/debug/bugverbose.h) \
    $(wildcard include/config/arm/lpae.h) \
  /media/root/robcore/android/machinex/include/linux/linkage.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
    $(wildcard include/config/smp.h) \
  /media/root/robcore/android/machinex/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/lge/crash/handler.h) \
    $(wildcard include/config/cpu/cp15/mmu.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /media/root/robcore/android/machinex/include/linux/sysinfo.h \
  /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include/stdarg.h \
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
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  /media/root/robcore/android/machinex/include/linux/dynamic_debug.h \
  /media/root/robcore/android/machinex/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /media/root/robcore/android/machinex/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
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
  /media/root/robcore/android/machinex/include/linux/timex.h \
  /media/root/robcore/android/machinex/include/linux/param.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/timex.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/timex.h \
    $(wildcard include/config/have/arch/has/current/timer.h) \
  /media/root/robcore/android/machinex/include/linux/delay.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/delay.h \
  /media/root/robcore/android/machinex/include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  /media/root/robcore/android/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno-base.h \
  /media/root/robcore/android/machinex/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  /media/root/robcore/android/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /media/root/robcore/android/machinex/include/linux/bitmap.h \
  /media/root/robcore/android/machinex/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/string.h \
  /media/root/robcore/android/machinex/include/linux/bug.h \
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

init/calibrate.o: $(deps_init/calibrate.o)

$(deps_init/calibrate.o):
