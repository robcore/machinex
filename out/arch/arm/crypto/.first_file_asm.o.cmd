cmd_arch/arm/crypto/first_file_asm.o := /media/root/robcore/android/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,arch/arm/crypto/.first_file_asm.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/media/root/robcore/android/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/media/root/robcore/android/machinex/include -include /media/root/robcore/android/machinex/include/linux/kconfig.h  -I/media/root/robcore/android/machinex/arch/arm/crypto -Iarch/arm/crypto -D__KERNEL__ -mlittle-endian   -I/media/root/robcore/android/machinex/arch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-unused-value -Wno-format-security -Wdeprecated-declarations -Wno-aggressive-loop-optimizations -Wno-unused-label -Wno-logical-not-parentheses -Wno-discarded-array-qualifiers -Werror-implicit-function-declaration -Wno-uninitialized -Wno-sequence-point -Wno-unused-variable -Wno-unused-function -fno-delete-null-pointer-checks -Wno-declaration-after-statement -mtune=cortex-a15 -mfpu=neon-vfpv4 -funsafe-math-optimizations -ftree-vectorize -std=gnu89 -Wbool-compare -Wno-unused-variable -marm -mtune=cortex-a15 -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -fgcse-after-reload -fgcse-sm -ftree-loop-im -ftree-loop-ivcanon -fivopts -ftree-vectorize -fmodulo-sched -ffast-math -marm -mtune=cortex-a15 -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -fgcse-after-reload -fgcse-sm -fgcse-las -ftree-loop-im -ftree-loop-ivcanon -fivopts -ftree-vectorize -fmodulo-sched -ffast-math -fweb -frename-registers -ftree-loop-linear -std=gnu89 -fmodulo-sched -ffast-math -funsafe-math-optimizations -Ofast -fconserve-stack -marm -fno-dwarf2-cfi-asm -fstack-protector -fno-conserve-stack -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -msoft-float -Uarm -Wframe-larger-than=2048 --param=allow-store-data-races=0 -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -Werror=implicit-int -Werror=strict-prototypes -DCC_HAVE_ASM_GOTO   -marm -mtune=cortex-a15 -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -fgcse-after-reload -fgcse-sm -fgcse-las -ftree-loop-im -ftree-loop-ivcanon -fivopts -ftree-vectorize -fmodulo-sched -ffast-math -fweb -frename-registers -ftree-loop-linear -std=gnu89 -fmodulo-sched -ffast-math -funsafe-math-optimizations -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(first_file_asm)"  -D"KBUILD_MODNAME=KBUILD_STR(first_file_asm)" -c -o arch/arm/crypto/.tmp_first_file_asm.o /media/root/robcore/android/machinex/arch/arm/crypto/first_file_asm.c

source_arch/arm/crypto/first_file_asm.o := /media/root/robcore/android/machinex/arch/arm/crypto/first_file_asm.c

deps_arch/arm/crypto/first_file_asm.o := \
  /media/root/robcore/android/machinex/include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
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
  /media/root/robcore/android/machinex/arch/arm/include/asm/posix_types.h \
  /media/root/robcore/android/machinex/include/asm-generic/posix_types.h \

arch/arm/crypto/first_file_asm.o: $(deps_arch/arm/crypto/first_file_asm.o)

$(deps_arch/arm/crypto/first_file_asm.o):
