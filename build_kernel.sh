#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/arm-cortex_a15-linux-gnueabihf-
rm -rf $(pwd)/out
rm $(pwd)/arch/arm/boot/dhd.ko
rm $(pwd)/arch/arm/boot/scsi_wait_scan.ko
rm $(pwd)/arch/arm/boot/zImage
make clean
make mrproper
mkdir $(pwd)/out
cp $(pwd)/arch/arm/configs/AEL_config $(pwd)/out/.config
make ARCH=arm -j7 -C $(pwd) O=$(pwd)/out oldconfig
make ARCH=arm -j7 -C $(pwd) O=$(pwd)/out
cp $(pwd)/out/arch/arm/boot/zImage $(pwd)/arch/arm/boot/zImage
cp $(pwd)/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/arch/arm/boot/dhd.ko
cp $(pwd)/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/arch/arm/boot/scsi_wait_scan.ko
