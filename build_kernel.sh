#!/bin/bash
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH

if [ -d $(pwd)/out ]; then
	rm -rf $(pwd)/out;
fi;
if [ -e $(pwd)/arch/arm/boot/dhd.ko ]; then
	rm $(pwd)/arch/arm/boot/dhd.ko;
fi;
if [ -e $(pwd)/arch/arm/boot/scsi_wait_scan.ko ]; then
	rm $(pwd)/arch/arm/boot/scsi_wait_scan.ko;
fi;
if [ -e $(pwd)/arch/arm/boot/zImage ]; then
	rm $(pwd)/arch/arm/boot/zImage;
fi;
if [ -e $(pwd)/arch/arm/boot/boot.img-zImage ]; then
	rm $(pwd)/arch/arm/boot/boot.img-zImage;
fi;
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
make ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e $(pwd)/out/arch/arm/boot/zImage ]; then
	cp -p $(pwd)/out/arch/arm/boot/zImage $(pwd)/arch/arm/boot/zImage;
	cp -p $(pwd)/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/arch/arm/boot/dhd.ko;
	cp -p $(pwd)/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/arch/arm/boot/scsi_wait_scan.ko;
	mv $(pwd)/arch/arm/boot/zImage $(pwd)/arch/arm/boot/boot.img-zImage;
	cd /media/root/robcore/AIK;
	rm -rf machinex-new;
	cp -R -p machina-new machinex-new;
	cp -p ~/machinex/arch/arm/boot/dhd.ko $(pwd)/machinex-new/system/lib/modules/dhd.ko;
	cp -p ~/machinex/arch/arm/boot/scsi_wait_scan.ko $(pwd)/machinex-new/system/lib/modules/scsi_wait_scan.ko;
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/arch/arm/boot/boot.img-zImage $(pwd)/split_img/boot.img-zImage;
	rm image-new.img;
	sh repackimg.sh --sudo;
	cp -p image-new.img $(pwd)/machinex-new/boot.img
else
	echo "Build failed, Skipped Ramdisk Creation"
fi;
