cmd_kernel/sys_ni.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,kernel/.sys_ni.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/kernel -Ikernel -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-unused-value -Wno-format-security -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations -Werror-implicit-function-declaration -Wno-uninitialized -Wno-format-security -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -fno-delete-null-pointer-checks -Wno-maybe-uninitialized -O3 -marm -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -fstack-protector -fno-conserve-stack -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=1048 -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO   -munaligned-access -mfpu=neon-vfpv4 -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(sys_ni)"  -D"KBUILD_MODNAME=KBUILD_STR(sys_ni)" -c -o kernel/.tmp_sys_ni.o /media/root/robcore/android/machinex/kernel/sys_ni.c

source_kernel/sys_ni.o := /media/root/robcore/android/machinex/kernel/sys_ni.c

deps_kernel/sys_ni.o := \
  /media/root/robcore/android/machinex/include/linux/linkage.h \
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
  /media/root/robcore/android/machinex/arch/arm/include/asm/linkage.h \
  /media/root/robcore/android/machinex/include/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno.h \
  /media/root/robcore/android/machinex/include/asm-generic/errno-base.h \
  /media/root/robcore/android/machinex/arch/arm/include/asm/unistd.h \
    $(wildcard include/config/aeabi.h) \
    $(wildcard include/config/oabi/compat.h) \

kernel/sys_ni.o: $(deps_kernel/sys_ni.o)

$(deps_kernel/sys_ni.o):
