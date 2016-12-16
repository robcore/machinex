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
if [ -e /media/root/robcore/AIK/previous.txt ]; then
	PREV=`cat /media/root/robcore/AIK/previous.txt`
	echo "your previous version was $PREV"
fi;

echo -n "Use Previous Name?  y/n [ENTER]: "
read USEPRV
if [[ $USEPRV = "n" ]]; then
	echo -n "Override Naming Process?  y/n [ENTER]: "
	read overide
	if [[ $overide = "n" ]]; then
		echo -n "Enter Kernel major version and press [ENTER]: "
		read MAJOR
		KERNEL_NAME=machinex
		KERNEL_VERSION=Mark$MAJOR
		echo -n "Is this a BETA?  y/n [ENTER]: "
		read rep
		if [[ $rep = "y" ]]; then
			echo -n "Is this Next or Proto Version? n/p [ENTER]: "
			read reply
			if [[ $reply = "n" ]]; then
				echo -n "Enter Next Version and press [ENTER]: "
				read NEXT
				SUBVERSION=Next$NEXT
			else
				echo -n "Enter Proto Version and press [ENTER]: "
				read PROTO
				SUBVERSION=P$PROTO
			fi
		OUTFOLDER=$KERNEL_NAME-$KERNEL_VERSION-$SUBVERSION
		echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
		else
		OUTFOLDER=$KERNEL_NAME-$KERNEL_VERSION
		echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
		fi
	else
		echo -n "What is its name? [ENTER]: "
		read OVNAME
		OUTFOLDER=$OVNAME
		echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
	fi;
else
	PRVS=`cat /media/root/robcore/AIK/previous.txt`
	if [ -d /media/root/robcore/AIK/$PRVS ]; then
		echo "removing previously compiled folder and zip of the same name"
		rm -rf /media/root/robcore/AIK/$PRVS
	fi;
	OUTFOLDER=$PRVS
fi;

echo -n "Automatically push to adb and cleanup the project?  y/n [ENTER]: "
read AUTO

#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
export ARCH=arm
#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
export USE_CCACHE=1
export CCACHE_DIR=~/.ccache
env KCONFIG_NOTIMESTAMP=true
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
cp $(pwd)/arch/arm/configs/canadefconfig $(pwd)/out/.config;
make ARCH=arm -j6 O=$(pwd)/out oldconfig;
make ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	cp -p ~/machinex/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/$OUTFOLDER/system/lib/modules/scsi_wait_scan.ko;
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	rm image-new.img;
	sh repackimg.sh --sudo;
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=`md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip`
	if [[ $AUTO = "n" ]]; then
		echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
		read repadb
		if [[ $repadb = "y" ]]; then
			echo "ENABLE ADB WIRELESS"
			sleep 10
			adb connect 192.168.1.103
			sleep 7
			adb connect 192.168.1.103
			sleep 5
			adb push $OUTFOLDER.zip /storage/extSdCard
			if [ $SDB == $SUMMY ]; then
				echo "MD5 MATCHES - Your kernel is ready to flash"
			else
				echo "MD5 MISMATCH, push again!"
			fi;
		fi;
		echo -n "Cleanup?  y/n [ENTER]: "
		read repcln
		if [[ $repcln = "y" ]]; then
			cd ~/machinex
			sh $(pwd)/cleanup.sh
			echo "cleanup finished"
		fi;
	else
		echo "ENABLE ADB WIRELESS"
		sleep 10
		adb connect 192.168.1.103
		sleep 7
		adb connect 192.168.1.103
		sleep 5
		adb push $OUTFOLDER.zip /storage/extSdCard
			if [ $SDB == $SUMMY ]; then
				echo "MD5 MATCHES - Your kernel is ready to flash"
			else
				echo "MD5 MISMATCH, push again!"
			fi;
		cd ~/machinex
		sh $(pwd)/cleanup.sh
		echo "cleanup finished"
	fi;

	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "MD5 is $SUMMY"
else
	echo "Build failed, Skipped Ramdisk Creation"
fi;
