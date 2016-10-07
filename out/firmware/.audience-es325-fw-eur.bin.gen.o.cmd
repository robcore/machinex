cmd_firmware/audience-es325-fw-eur.bin.gen.o := /root/machinex/scripts/gcc-wrapper.py /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-gcc -Wp,-MD,firmware/.audience-es325-fw-eur.bin.gen.o.d  -nostdinc -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include -I/root/machinex/arch/arm/include -Iarch/arm/include/generated -Iinclude  -I/root/machinex/include -include /root/machinex/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian   -I/root/machinex/arch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=7 -mcpu=cortex-a15 -mtune=cortex-a15 -include asm/unified.h -msoft-float   -c -o firmware/audience-es325-fw-eur.bin.gen.o firmware/audience-es325-fw-eur.bin.gen.S

source_firmware/audience-es325-fw-eur.bin.gen.o := firmware/audience-es325-fw-eur.bin.gen.S

deps_firmware/audience-es325-fw-eur.bin.gen.o := \
  /root/machinex/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

firmware/audience-es325-fw-eur.bin.gen.o: $(deps_firmware/audience-es325-fw-eur.bin.gen.o)

$(deps_firmware/audience-es325-fw-eur.bin.gen.o):
