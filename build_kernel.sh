#!/bin/bash

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
echo -n "Enter Kernel major version and press [ENTER]: "
read MAJOR
KERNEL_NAME=machinex
KERNEL_VERSION=Mark$MAJOR

read -s -n 1 -p "Enter 1 for Next, 2 for Proto, or 3 for Release  " rep
if [ $rep == 1 ]; then
	read -s -n 1 -p "Enter Next Version  " NEXT
	SUBVERSION=Next$NEXT
	OUTFOLDER=$KERNEL_NAME-$KERNEL_VERSION-$SUBVERSION
else if [ $rep == 2 ]; then
	read -s -n 1 -p "Enter Proto Version  " PROTO
	SUBVERSION=P$PROTO
	OUTFOLDER=$KERNEL_NAME-$KERNEL_VERSION-$SUBVERSION
else if [ $rep == 3 ]; then
	OUTFOLDER=$KERNEL_NAME-$KERNEL_VERSION
fi

export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
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
	mv $(pwd)/machinex-new $(pwd)/$OUTFOLDER
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	read -s -n 1 -p "Shall I adb push this for you, sir?  y/n  " repadb
	if [[ $repadb = "y" ]]; then
		adb connect 192.168.1.103
		sleep 5
		adb push $OUTFOLDER.zip /storage/extSdCard
		echo "Your kernel is ready to flash"
	else
		echo "Fin"
	fi;
else
	echo "Build failed, Skipped Ramdisk Creation"
fi;
