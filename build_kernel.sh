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
cp -p $(pwd)/out/arch/arm/boot/zImage $(pwd)/arch/arm/boot/zImage;
cp -p $(pwd)/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/arch/arm/boot/dhd.ko;
cp -p $(pwd)/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/arch/arm/boot/scsi_wait_scan.ko;
mv $(pwd)/arch/arm/boot/zImage $(pwd)/arch/arm/boot/boot.img-zImage;
cd /media/root/robcore/AIK;
rm -rf machinex-new;
cp -R -p machina-new machinex-new;
cp -p /media/root/robcore/android/machinex/arch/arm/boot/dhd.ko $(pwd)/machinex-new/system/lib/modules/dhd.ko;
cp -p /media/root/robcore/android/machinex/arch/arm/boot/scsi_wait_scan.ko $(pwd)/machinex-new/system/lib/modules/scsi_wait_scan.ko;
rm $(pwd)/split_img/boot.img-zImage;
cp -p /media/root/robcore/android/machinex/arch/arm/boot/boot.img-zImage $(pwd)/split_img/boot.img-zImage;
rm image-new.img;
sudo sh repackimg.sh;
cp -p image-new.img $(pwd)/machinex-new/boot.img
