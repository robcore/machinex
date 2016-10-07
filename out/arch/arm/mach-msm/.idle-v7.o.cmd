cmd_arch/arm/mach-msm/idle-v7.o := /root/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,arch/arm/mach-msm/.idle-v7.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/root/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/root/machinex/include -include /root/machinex/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian   -I/root/machinex/arch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -mtune=cortex-a15 -include asm/unified.h -msoft-float   -c -o arch/arm/mach-msm/idle-v7.o /root/machinex/arch/arm/mach-msm/idle-v7.S

source_arch/arm/mach-msm/idle-v7.o := /root/machinex/arch/arm/mach-msm/idle-v7.S

deps_arch/arm/mach-msm/idle-v7.o := \
    $(wildcard include/config/arch/msm/krait.h) \
    $(wildcard include/config/arch/msm8x60.h) \
    $(wildcard include/config/msm/fiq/support.h) \
    $(wildcard include/config/msm/jtag.h) \
    $(wildcard include/config/msm/jtag/mm.h) \
    $(wildcard include/config/arch/msm/scorpion.h) \
  /root/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  /root/machinex/include/linux/linkage.h \
  /root/machinex/include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /root/machinex/arch/arm/include/asm/linkage.h \
  /root/machinex/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /root/machinex/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /root/machinex/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /root/machinex/arch/arm/include/asm/hwcap.h \
  /root/machinex/arch/arm/include/asm/domain.h \
    $(wildcard include/config/verify/permission/fault.h) \
    $(wildcard include/config/io/36.h) \
  /root/machinex/arch/arm/mach-msm/idle.h \
    $(wildcard include/config/cpu/v7.h) \
  /root/machinex/arch/arm/mach-msm/idle-macros.S \
  /root/machinex/arch/arm/include/asm/hardware/cache-l2x0.h \
    $(wildcard include/config/cache/l2x0.h) \
    $(wildcard include/config/of.h) \
  /root/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /root/machinex/include/asm-generic/errno.h \
  /root/machinex/include/asm-generic/errno-base.h \

arch/arm/mach-msm/idle-v7.o: $(deps_arch/arm/mach-msm/idle-v7.o)

$(deps_arch/arm/mach-msm/idle-v7.o):
