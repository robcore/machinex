cmd_arch/arm/mm/abort-ev7.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,arch/arm/mm/.abort-ev7.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -include asm/unified.h -msoft-float -gdwarf-2 -Wa,-march=armv7-a   -c -o arch/arm/mm/abort-ev7.o /media/root/robcore/android/machinex/arch/arm/mm/abort-ev7.S

source_arch/arm/mm/abort-ev7.o := /media/root/robcore/android/machinex/arch/arm/mm/abort-ev7.S

deps_arch/arm/mm/abort-ev7.o := \
    $(wildcard include/config/verify/permission/fault.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  /media/root/robcore/android/machinex/include/linux/linkage.h \
  /media/root/robcore/android/machinex/include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /media/root/robcore/android/machinex/arch/arm/include/asm/hwcap.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/emulate/domain/manager/v7.h) \

arch/arm/mm/abort-ev7.o: $(deps_arch/arm/mm/abort-ev7.o)

$(deps_arch/arm/mm/abort-ev7.o):
