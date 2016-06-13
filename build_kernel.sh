#!/bin/bash

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
make ARCH=arm -j7 O=$(pwd)/out oldconfig;
make ARCH=arm -S -s -j7 O=$(pwd)/out;
cp $(pwd)/out/arch/arm/boot/zImage $(pwd)/arch/arm/boot/zImage;
cp $(pwd)/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/arch/arm/boot/dhd.ko;
cp $(pwd)/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/arch/arm/boot/scsi_wait_scan.ko;
mv $(pwd)/arch/arm/boot/zImage $(pwd)/arch/arm/boot/boot.img-zImage;
