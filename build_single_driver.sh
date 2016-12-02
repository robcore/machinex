#!/bin/bash
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
rm -rf $(pwd)/out;
rm $(pwd)/arch/arm/boot/dhd.ko;
rm $(pwd)/arch/arm/boot/scsi_wait_scan.ko;
rm $(pwd)/arch/arm/boot/zImage;
rm $(pwd)/arch/arm/boot/boot.img-zImage;
# clean up leftover junk
find . -type f \( -iname \*.rej \
				-o -iname \*.orig \
				-o -iname \*.bkp \
				-o -iname \*.ko \) \
					| parallel rm -fv {};
export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
env KCONFIG_NOTIMESTAMP=true
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
cp $(pwd)/arch/arm/configs/canadefconfig $(pwd)/out/.config;
make ARCH=arm -j6 O=$(pwd)/out oldconfig;
make ARCH=arm -S -s -j6 O=$(pwd)/out $(pwd)/kernel/power/;
