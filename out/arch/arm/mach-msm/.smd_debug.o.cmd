cmd_arch/arm/mach-msm/smd_debug.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,arch/arm/mach-msm/.smd_debug.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/arch/arm/mach-msm -Iarch/arm/mach-msm -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing -fno-common -mtune=cortex-a15 -mfpu=neon-vfpv4 -std=gnu89 -Wno-format-security -Wno-unused-function -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -fno-delete-null-pointer-checks -Wno-cpp -Wno-declaration-after-statement -fno-var-tracking-assignments -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Wno-sequence-point -O3 -marm -fno-dwarf2-cfi-asm -fstack-protector -fno-ipa-sra -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=2048 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -mcpu=cortex-a15 -mtune=cortex-a15 -mfpu=neon-vfpv4 -fno-align-labels -fno-prefetch-loop-arrays -mvectorize-with-neon-quad -funsafe-math-optimizations -munaligned-access -ftree-vectorize -funroll-loops -fno-align-functions -fno-align-jumps -fno-align-loops -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(smd_debug)"  -D"KBUILD_MODNAME=KBUILD_STR(smd_debug)" -c -o arch/arm/mach-msm/.tmp_smd_debug.o /media/root/robcore/android/machinex/arch/arm/mach-msm/smd_debug.c

source_arch/arm/mach-msm/smd_debug.o := /media/root/robcore/android/machinex/arch/arm/mach-msm/smd_debug.c

deps_arch/arm/mach-msm/smd_debug.o := \
    $(wildcard include/config/debug/fs.h) \
    $(wildcard include/config/msm/smd/pkg4.h) \
    $(wildcard include/config/msm/smd/pkg3.h) \
  /media/root/robcore/android/machinex/include/linux/debugfs.h \
  /media/root/robcore/android/machinex/include/linux/fs.h \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/fs/posix/acl.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/fsnotify.h) \
    $(wildcard include/config/ima.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/debug/writecount.h) \
    $(wildcard include/config/file/locking.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
    $(wildcard include/config/fs/xip.h) \
    $(wildcard include/config/migration.h) \
  /media/root/robcore/android/machinex/include/linux/limits.h \
  /media/root/robcore/android/machinex/include/linux/ioctl.h \
  arch/arm/include/generated/asm/ioctl.h \
  /media/root/robcore/android/machinex/include/asm-generic/ioctl.h \
  /media/root/robcore/android/machinex/include/linux/blk_types.h \
    $(wildcard include/config/blk/dev/integrity.h) \
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
  /media/root/robcore/android/machinex/include/linux/linkage.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/include/linux/wait.h \
    $(wildcard include/config/lockdep.h) \
  /media/root/robcore/android/machinex/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /media/root/robcore/android/machinex/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  /media/root/robcore/android/machinex/include/linux/const.h \
  /media/root/robcore/android/machinex/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
  /media/root/robcore/android/machinex/include/linux/typecheck.h \
  /media/root/robcore/android/machinex/include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  /media/root/robcore/android/machinex/include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /media/root/robcore/android/machinex/include/linux/bitops.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/bitops.h \
  /media/root/robcore/android/machinex/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
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
  /media/root/robcore/android/machinex/include/linux/stringify.h \
  /media/root/robcore/android/machinex/include/linux/bottom_half.h \
  /media/root/robcore/android/machinex/include/linux/spinlock_types.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/spinlock_types.h \
  /media/root/robcore/android/machinex/include/linux/lockdep.h \
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
  /media/root/robcore/android/machinex/include/asm-generic/processor.h \
  /media/root/robcore/android/machinex/include/asm-generic/relaxed.h \
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
  /media/root/robcore/android/machinex/include/linux/prefetch.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/cmpxchg.h \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/v6.h) \
  /media/root/robcore/android/machinex/include/asm-generic/cmpxchg-local.h \
  /media/root/robcore/android/machinex/include/asm-generic/atomic-long.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/current.h \
  /media/root/robcore/android/machinex/include/linux/kdev_t.h \
  /media/root/robcore/android/machinex/include/linux/dcache.h \
  /media/root/robcore/android/machinex/include/linux/rculist.h \
  /media/root/robcore/android/machinex/include/linux/rcupdate.h \
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
  /media/root/robcore/android/machinex/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /media/root/robcore/android/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /media/root/robcore/android/machinex/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  /media/root/robcore/android/machinex/include/linux/bitmap.h \
  /media/root/robcore/android/machinex/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/string.h \
  /media/root/robcore/android/machinex/include/linux/bug.h \
  /media/root/robcore/android/machinex/include/linux/seqlock.h \
  /media/root/robcore/android/machinex/include/linux/completion.h \
  /media/root/robcore/android/machinex/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  /media/root/robcore/android/machinex/include/linux/rcutree.h \
  /media/root/robcore/android/machinex/include/linux/rculist_bl.h \
  /media/root/robcore/android/machinex/include/linux/list_bl.h \
  /media/root/robcore/android/machinex/include/linux/bit_spinlock.h \
  /media/root/robcore/android/machinex/include/linux/path.h \
  /media/root/robcore/android/machinex/include/linux/stat.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/stat.h \
  /media/root/robcore/android/machinex/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /media/root/robcore/android/machinex/include/linux/math64.h \
  /media/root/robcore/android/machinex/include/linux/radix-tree.h \
  /media/root/robcore/android/machinex/include/linux/prio_tree.h \
  /media/root/robcore/android/machinex/include/linux/pid.h \
  /media/root/robcore/android/machinex/include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  /media/root/robcore/android/machinex/include/linux/capability.h \
  /media/root/robcore/android/machinex/include/linux/semaphore.h \
  /media/root/robcore/android/machinex/include/linux/fiemap.h \
  /media/root/robcore/android/machinex/include/linux/shrinker.h \
  /media/root/robcore/android/machinex/include/linux/migrate_mode.h \
  /media/root/robcore/android/machinex/include/linux/quota.h \
    $(wildcard include/config/quota/netlink/interface.h) \
  /media/root/robcore/android/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno-base.h \
  /media/root/robcore/android/machinex/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/rwsem.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/exec.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/switch_to.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system_info.h \
    $(wildcard include/config/sec/debug/subsys.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/system_misc.h \
  /media/root/robcore/android/machinex/include/linux/percpu_counter.h \
  /media/root/robcore/android/machinex/include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/smp.h \
  /media/root/robcore/android/machinex/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /media/root/robcore/android/machinex/include/linux/pfn.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/percpu.h \
  /media/root/robcore/android/machinex/include/asm-generic/percpu.h \
  /media/root/robcore/android/machinex/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
  /media/root/robcore/android/machinex/include/linux/dqblk_xfs.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_v1.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_v2.h \
  /media/root/robcore/android/machinex/include/linux/dqblk_qtree.h \
  /media/root/robcore/android/machinex/include/linux/nfs_fs_i.h \
  /media/root/robcore/android/machinex/include/linux/fcntl.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/fcntl.h \
  /media/root/robcore/android/machinex/include/asm-generic/fcntl.h \
    $(wildcard include/config/scfs/lower/pagecache/invalidation.h) \
  /media/root/robcore/android/machinex/include/linux/err.h \
  /media/root/robcore/android/machinex/include/linux/seq_file.h \
  /media/root/robcore/android/machinex/include/linux/nodemask.h \
    $(wildcard include/config/highmem.h) \
  /media/root/robcore/android/machinex/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  /media/root/robcore/android/machinex/include/linux/ctype.h \
  /media/root/robcore/android/machinex/include/linux/jiffies.h \
  /media/root/robcore/android/machinex/include/linux/timex.h \
  /media/root/robcore/android/machinex/include/linux/param.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/timex.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/timex.h \
    $(wildcard include/config/have/arch/has/current/timer.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap.h \
    $(wildcard include/config/debug/msm/uart1.h) \
    $(wildcard include/config/debug/msm/uart2.h) \
    $(wildcard include/config/debug/msm/uart3.h) \
    $(wildcard include/config/msm/debug/uart/phys.h) \
    $(wildcard include/config/arch/msm8960.h) \
    $(wildcard include/config/arch/apq8064.h) \
    $(wildcard include/config/arch/msm8930.h) \
    $(wildcard include/config/arch/msm9615.h) \
    $(wildcard include/config/arch/msm8974.h) \
    $(wildcard include/config/arch/msm7x27.h) \
    $(wildcard include/config/arch/msm7x25.h) \
    $(wildcard include/config/arch/msm7x01a.h) \
    $(wildcard include/config/arch/msm8625.h) \
    $(wildcard include/config/arch/msm7x30.h) \
    $(wildcard include/config/arch/msm9625.h) \
    $(wildcard include/config/arch/qsd8x50.h) \
    $(wildcard include/config/arch/msm8x60.h) \
    $(wildcard include/config/arch/fsm9xxx.h) \
  arch/arm/include/generated/asm/sizes.h \
  /media/root/robcore/android/machinex/include/asm-generic/sizes.h \
  /media/root/robcore/android/machinex/include/linux/sizes.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-7xxx.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-7x30.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-8625.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-8960.h \
    $(wildcard include/config/debug/msm8960/uart.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-8930.h \
    $(wildcard include/config/debug/msm8930/uart.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-8064.h \
    $(wildcard include/config/debug/apq8064/uart.h) \
    $(wildcard include/config/mach/apq8064/mako.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-9615.h \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-8974.h \
    $(wildcard include/config/debug/msm8974/uart.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_iomap-9625.h \
    $(wildcard include/config/debug/msm9625/uart.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/smd_private.h \
    $(wildcard include/config/msm/smd.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_smsm.h \
    $(wildcard include/config/msm/n/way/smsm.h) \
  /media/root/robcore/android/machinex/include/linux/notifier.h \
  /media/root/robcore/android/machinex/include/linux/srcu.h \
  /media/root/robcore/android/machinex/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
  /media/root/robcore/android/machinex/include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  /media/root/robcore/android/machinex/include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_smd.h \
  /media/root/robcore/android/machinex/include/linux/io.h \
    $(wildcard include/config/has/ioport.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/io.h \
    $(wildcard include/config/need/mach/io/h.h) \
    $(wildcard include/config/pcmcia/soc/common.h) \
    $(wildcard include/config/pci.h) \
    $(wildcard include/config/isa.h) \
    $(wildcard include/config/pccard.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/memory.h \
    $(wildcard include/config/need/mach/memory/h.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/phys/offset.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/memory.h \
    $(wildcard include/config/have/end/mem.h) \
    $(wildcard include/config/end/mem.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/vmsplit/3g.h) \
    $(wildcard include/config/arch/msm/arm11.h) \
    $(wildcard include/config/arch/msm/cortex/a5.h) \
    $(wildcard include/config/cache/l2x0.h) \
    $(wildcard include/config/dont/map/hole/after/membank0.h) \
    $(wildcard include/config/arch/msm/scorpion.h) \
    $(wildcard include/config/arch/msm/krait.h) \
  /media/root/robcore/android/machinex/include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
  /media/root/robcore/android/machinex/include/asm-generic/pci_iomap.h \
    $(wildcard include/config/no/generic/pci/ioport/map.h) \
    $(wildcard include/config/generic/pci/iomap.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/msm_rtb.h \
    $(wildcard include/config/msm/rtb.h) \
  /media/root/robcore/android/machinex/arch/arm/mach-msm/include/mach/io.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/fa.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/kuser/helpers.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/memory/hotplug/sparse.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/glue.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/pgtable-2level-types.h \
  /media/root/robcore/android/machinex/include/asm-generic/getorder.h \

arch/arm/mach-msm/smd_debug.o: $(deps_arch/arm/mach-msm/smd_debug.o)

$(deps_arch/arm/mach-msm/smd_debug.o):
