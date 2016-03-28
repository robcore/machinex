#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/arm-cortex_a15-linux-gnueabihf-
rm -rf $(pwd)/out
rm $(pwd)/arch/arm/boot/dhd.ko
rm $(pwd)/arch/arm/boot/scsi_wait_scan.ko
rm $(pwd)/arch/arm/boot/zImage
make clean
make mrproper
make ARCH=arm -j7 -C $(pwd) $(pwd)/arch/arm/configs/AEL_config
make check ARCH=arm -j7 -C $(pwd)
