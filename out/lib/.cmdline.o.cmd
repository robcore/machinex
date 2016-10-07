cmd_lib/cmdline.o := /root/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,lib/.cmdline.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/root/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/root/machinex/include -include /root/machinex/include/linux/kconfig.h  -I/root/machinex/lib -Ilib -D__KERNEL__ -mlittle-endian   -I/root/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing -fno-common -mtune=cortex-a15 -mfpu=neon-vfpv4 -std=gnu89 -Wno-format-security -Wno-unused-function -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -fno-delete-null-pointer-checks -Wno-cpp -Wno-declaration-after-statement -fno-var-tracking-assignments -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Wno-sequence-point -O3 -marm -fno-dwarf2-cfi-asm -fstack-protector -fno-conserve-stack -fno-ipa-sra -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -mtune=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=2048 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -mcpu=cortex-a15 -mtune=cortex-a15 -mfpu=neon-vfpv4  -mvectorize-with-neon-quad -munaligned-access -fgraphite-identity -floop-parallelize-all -ftree-loop-linear -floop-interchange -floop-strip-mine -floop-block -ftree-vectorize -funroll-loops -fno-align-functions -fno-align-jumps -fno-align-loops -fno-align-labels -fno-prefetch-loop-arrays -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(cmdline)"  -D"KBUILD_MODNAME=KBUILD_STR(cmdline)" -c -o lib/.tmp_cmdline.o /root/machinex/lib/cmdline.c

source_lib/cmdline.o := /root/machinex/lib/cmdline.c

deps_lib/cmdline.o := \
  /root/machinex/include/linux/export.h \
    $(wildcard include/config/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /root/machinex/include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
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
  /root/machinex/include/linux/sysinfo.h \
  /root/machinex/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
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
  /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include/stdarg.h \
  /root/machinex/include/linux/linkage.h \
  /root/machinex/arch/arm/include/asm/linkage.h \
  /root/machinex/include/linux/bitops.h \
  /root/machinex/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  /root/machinex/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /root/machinex/include/linux/typecheck.h \
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
  /root/machinex/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /root/machinex/include/linux/printk.h \
    $(wildcard include/config/sec/ssr/dump.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  /root/machinex/include/linux/init.h \
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
  /root/machinex/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /root/machinex/arch/arm/include/asm/string.h \

lib/cmdline.o: $(deps_lib/cmdline.o)

$(deps_lib/cmdline.o):
