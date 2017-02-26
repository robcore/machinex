#!/bin/bash

function WASHME()
{
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
rm -f $(pwd)/arch/arm/boot/*.cmd >> /dev/null;
rm -f $(pwd)/arch/arm/mach-msm/smd_rpc_sym.c >> /dev/null;
rm -f $(pwd)/arch/arm/crypto/aesbs-core.S >> /dev/null;
rm -f $(pwd)/r*.cpio >> /dev/null;
rm -rf $(pwd)/include/generated >> /dev/null;
rm -rf $(pwd)/arch/*/include/generated >> /dev/null;
}

if [ -e /media/root/robcore/AIK/previous.txt ]; then
	PREV=`cat /media/root/robcore/AIK/previous.txt`
fi;

function countdown()
{
	echo "3"
	sleep 1
	echo "2"
	sleep 1
	echo "1"
	sleep 1
}

function NORMAL()
{
echo "your previous version was $PREV"
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
		echo -n "Name? (Number-P-N) [ENTER]: "
		read OVNAME
		OUTFOLDER=machinex-Mark$OVNAME
		echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
	fi;
else
	PRVS=$PREV
	if [ -d /media/root/robcore/AIK/$PRVS ]; then
		echo "removing previously compiled folder and zip of the same name"
		rm -rf /media/root/robcore/AIK/$PRVS
	fi;
	OUTFOLDER=$PRVS
fi;

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
if [[ $ONLINE == device ]]; then
	echo "connected"
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "push complete"
else
	echo "disconnected, retrying"
	adb connect 192.168.1.103
	countdown
	if [[ $ONLINE == device ]]; then
		adb push $OUTFOLDER.zip /storage/extSdCard
		echo "pushed"
	else
		echo "push failed"
	fi
fi;
}

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig

echo -n "Automatically push to adb and cleanup the project?  y/n [ENTER]: "
read AUTO
#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
export SUBARCH=arm
export ARCH=arm
#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
export KBUILD_LOCALVERSION=-$OUTFOLDER
export KBUILD_BUILD_VERSION=04
export USE_CCACHE=1
export CCACHE_DIR=~/.ccache
#env KCONFIG_NOTIMESTAMP=true
WASHME
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
cp $(pwd)/arch/arm/configs/tmpconfig $(pwd)/out/.config;
rm arch/arm/configs/tmpconfig
make SUBARCH=arm ARCH=arm -j6 O=$(pwd)/out oldconfig;
make SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
		cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	fi
	if [ -e ~/machinex/out/drivers/scsi/scsi_wait_scan.ko ]; then
		cp -p ~/machinex/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/$OUTFOLDER/system/lib/modules/scsi_wait_scan.ko;
	fi
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	rm image-new.img;
	sh repackimg.sh --sudo;
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=`md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip`
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
	date >> ~/machinex/datetracker.txt
	echo "\n" >> ~/machinex/datetracker.txt
	if [[ $AUTO = "n" ]]; then
		echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
		read repadb
		if [[ $repadb = "y" ]]; then
			echo "ENABLE ADB WIRELESS"
			countdown
			adb connect 192.168.1.103
			countdown
			ADBRETRY
		fi;
		echo -n "Save Object Files?  y/n [ENTER]: "
		read objsave
		if [[ $objsave = "y" ]]; then
			cd ~/machinex
			rm -rf object-files.txt
			touch object-files.txt
			find -iname '*.o*' | sort -d > object-files.txt
			echo "Object Files Saved"
		fi;
		echo -n "Cleanup?  y/n [ENTER]: "
		read repcln
		if [[ $repcln = "y" ]]; then
			cd ~/machinex
			WASHME
			echo "Cleanup Finished"
		fi;
	else
		echo "ENABLE ADB WIRELESS"
		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		countdown
		adb connect 192.168.1.103
		countdown
		ADBRETRY
		cd ~/machinex
		WASHME
		echo "Push and Cleanup finished"
	fi;

	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "MD5 is $SUMMY"
else
	WASHME
	echo "Build failed, Skipped Ramdisk Creation"
fi;
}

function SPARSE()
{
echo "your previous version was $PREV"
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
		echo -n "Name? (Number-P-N) [ENTER]: "
		read OVNAME
		OUTFOLDER=machinex-Mark$OVNAME
		echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
	fi;
else
	PRVS=$PREV
	if [ -d /media/root/robcore/AIK/$PRVS ]; then
		echo "removing previously compiled folder and zip of the same name"
		rm -rf /media/root/robcore/AIK/$PRVS
	fi;
	OUTFOLDER=$PRVS
fi;

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
if [[ $ONLINE == device ]]; then
	echo "connected"
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "push complete"
else
	echo "disconnected, retrying"
	adb connect 192.168.1.103
	countdown
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "pushed"
fi;
}

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig

echo -n "Automatically push to adb and cleanup the project?  y/n [ENTER]: "
read AUTO
#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
export SUBARCH=arm
export ARCH=arm
#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
export KBUILD_LOCALVERSION=-$OUTFOLDER
export KBUILD_BUILD_VERSION=04
export USE_CCACHE=1
export CCACHE_DIR=~/.ccache
#env KCONFIG_NOTIMESTAMP=true
WASHME
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
cp $(pwd)/arch/arm/configs/tmpconfig $(pwd)/out/.config;
rm arch/arm/configs/tmpconfig
make SUBARCH=arm ARCH=arm -j6 O=$(pwd)/out oldconfig;
make C=1 SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
		cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	fi
	if [ -e ~/machinex/out/drivers/scsi/scsi_wait_scan.ko ]; then
		cp -p ~/machinex/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/$OUTFOLDER/system/lib/modules/scsi_wait_scan.ko;
	fi
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	rm image-new.img;
	sh repackimg.sh --sudo;
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=`md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip`
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
	date >> ~/machinex/datetracker.txt
	echo "\n" >> ~/machinex/datetracker.txt
	if [[ $AUTO = "n" ]]; then
		echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
		read repadb
		if [[ $repadb = "y" ]]; then
			echo "ENABLE ADB WIRELESS"
			countdown
			adb connect 192.168.1.103
			countdown
			ADBRETRY
		fi;
		echo -n "Save Object Files?  y/n [ENTER]: "
		read objsave
		if [[ $objsave = "y" ]]; then
			cd ~/machinex
			rm -rf object-files.txt
			touch object-files.txt
			find -iname '*.o*' | sort -d > object-files.txt
			echo "Object Files Saved"
		fi;
		echo -n "Cleanup?  y/n [ENTER]: "
		read repcln
		if [[ $repcln = "y" ]]; then
			cd ~/machinex
			WASHME
			echo "Cleanup Finished"
		fi;
	else
		echo "ENABLE ADB WIRELESS"
		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		countdown
		adb connect 192.168.1.103
		countdown
		ADBRETRY
		cd ~/machinex
		WASHME
		echo "Push and Cleanup finished"
	fi;

	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "MD5 is $SUMMY"
else
	WASHME
	echo "Build failed, Skipped Ramdisk Creation"
fi;
}

function MISMATCH()
{
echo "Building CONFIG_SECTION_MISMATCH kernel"
	echo "your previous version was $PREV"
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
			echo -n "Name? (Number-P-N) [ENTER]: "
			read OVNAME
			OUTFOLDER=machinex-Mark$OVNAME
			echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
		fi;
	else
		PRVS=$PREV
		if [ -d /media/root/robcore/AIK/$PRVS ]; then
			echo "removing previously compiled folder and zip of the same name"
			rm -rf /media/root/robcore/AIK/$PRVS
		fi;
		OUTFOLDER=$PRVS
	fi;

	echo -n "Automatically push to adb and cleanup the project?  y/n [ENTER]: "
	read AUTO

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
if [[ $ONLINE == device ]]; then
	echo "connected"
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "push complete"
else
	echo "disconnected, retrying"
	adb connect 192.168.1.103
	countdown
	if [[ $ONLINE == device ]]; then
		adb push $OUTFOLDER.zip /storage/extSdCard
		echo "pushed"
	else
		echo "push failed"
	fi
fi;
}

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig

	#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
	export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
	export SUBARCH=arm
	export ARCH=arm
	#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
	export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
	export KBUILD_LOCALVERSION=-$OUTFOLDER
	export KBUILD_BUILD_VERSION=007
	export USE_CCACHE=1
	export CCACHE_DIR=~/.ccache
	#env KCONFIG_NOTIMESTAMP=true
	WASHME
	make clean;
	make distclean;
	make mrproper;
	mkdir $(pwd)/out;
	cp $(pwd)/arch/arm/configs/tmpconfig $(pwd)/out/.config;
	rm arch/arm/configs/tmpconfig
	make SUBARCH=arm ARCH=arm -j6 O=$(pwd)/out oldconfig;
	make CONFIG_DEBUG_SECTION_MISMATCH=y SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
		if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
			cd /media/root/robcore/AIK;
			cp -R -p machina-new $OUTFOLDER;
		if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
			cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
		fi
		if [ -e ~/machinex/out/drivers/scsi/scsi_wait_scan.ko ]; then
			cp -p ~/machinex/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/$OUTFOLDER/system/lib/modules/scsi_wait_scan.ko;
		fi
		rm $(pwd)/split_img/boot.img-zImage;
		cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
		rm image-new.img;
		sh repackimg.sh --sudo;
		cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
		cd $OUTFOLDER
		zip -r -9 - * > $OUTFOLDER.zip
		#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
		SUMMY=`md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip`
		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
		date >> ~/machinex/datetracker.txt
		echo "\n" >> ~/machinex/datetracker.txt
		if [[ $AUTO = "n" ]]; then
			echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
			read repadb
			if [[ $repadb = "y" ]]; then
				echo "ENABLE ADB WIRELESS"
				countdown
				adb connect 192.168.1.103
				countdown
				ADBRETRY
			fi;
			echo -n "Cleanup?  y/n [ENTER]: "
			read repcln
			if [[ $repcln = "y" ]]; then
				cd ~/machinex
				WASHME
				echo "cleanup finished"
			fi;
		else
			echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
			echo "ENABLE ADB WIRELESS"
			countdown
			adb connect 192.168.1.103
			countdown
			ADBRETRY
			cd ~/machinex
			WASHME
			echo "push and cleanup finished"
		fi;

		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		echo "MD5 is $SUMMY"
	else
		WASHME
		echo "Build failed, Skipped Ramdisk Creation"
	fi;
}

function HEADERS()
{
echo "Building HEADERS kernel"
PRVS=$PREV
if [ -e /media/root/robcore/AIK/previous.txt ]; then
	echo "your previous version was $PRVS"
fi;
	OUTFOLDER=$PRVS

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig
	#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
	export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
	export SUBARCH=arm
	export ARCH=arm
	#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
	export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
	export KBUILD_LOCALVERSION=-$OUTFOLDER
	export KBUILD_BUILD_VERSION=4
	export USE_CCACHE=1
	export CCACHE_DIR=~/.ccache
	#env KCONFIG_NOTIMESTAMP=true
	mkdir $(pwd)/out;
	cp $(pwd)/arch/arm/configs/tmpconfig $(pwd)/out/.config;
	rm arch/arm/configs/tmpconfig
	make SUBARCH=arm ARCH=arm -j6 O=$(pwd)/out oldconfig;
	make SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
	if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
		echo "Success!"
	else
		echo "Build failed, Skipped Ramdisk Creation"
	fi;
}

function REBUILD()
{
echo "your previous version was $PREV"
sleep 1

PRVS=$PREV
if [ -d /media/root/robcore/AIK/$PRVS ]; then
	echo "removing previously compiled folder and zip of the same name"
	sleep 1
	rm -rf /media/root/robcore/AIK/$PRVS
fi;
OUTFOLDER=$PRVS

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
if [[ $ONLINE == device ]]; then
	echo "connected"
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "push complete"
else
	echo "disconnected, retrying"
	adb connect 192.168.1.103
	countdown
	adb push $OUTFOLDER.zip /storage/extSdCard
	echo "pushed"
fi;
}

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig

#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
export SUBARCH=arm
export ARCH=arm
#export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
export KBUILD_LOCALVERSION=-$OUTFOLDER
export KBUILD_BUILD_VERSION=04
export USE_CCACHE=1
export CCACHE_DIR=~/.ccache
#env KCONFIG_NOTIMESTAMP=true
WASHME
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
cp $(pwd)/arch/arm/configs/tmpconfig $(pwd)/out/.config;
rm arch/arm/configs/tmpconfig
make SUBARCH=arm ARCH=arm -j6 O=$(pwd)/out oldconfig;
make SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
		cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	fi
	if [ -e ~/machinex/out/drivers/scsi/scsi_wait_scan.ko ]; then
		cp -p ~/machinex/out/drivers/scsi/scsi_wait_scan.ko $(pwd)/$OUTFOLDER/system/lib/modules/scsi_wait_scan.ko;
	fi
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	rm image-new.img;
	sh repackimg.sh --sudo;
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=`md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip`
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
	date >> ~/machinex/datetracker.txt
	echo "\n" >> ~/machinex/datetracker.txt
	echo "ENABLE ADB WIRELESS"
	countdown
	adb connect 192.168.1.103
	countdown
	ADBRETRY
	cd ~/machinex
	WASHME
	echo "push and cleanup finished"
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "MD5 is $SUMMY"
else
	WASHME
	echo "Build failed, Skipped Ramdisk Creation"
fi;
}

function CHKVERS() {
	PERV=`cat /media/root/robcore/AIK/previous.txt`
	echo "your previous version was $PERV"
}

if [ $# = 0 ] ; then
	NORMAL
fi

while [[ $# > 0 ]]
	do
	key="$1"

	case $key in
	     -m|--mismatch)
	    	MISMATCH
	    	break
	    	;;

	     -r|--rebuild)
	    	REBUILD
	    	break
	    	;;

	     -c|--chkvers)
	    	CHKVERS
	    	break
	    	;;

	     -s|--sparse)
	    	SPARSE
	    	break
	    	;;

	     -h|--headers)
	    	HEADERS
	    	break
	    	;;

	    *)
	    	NORMAL
	    	break;
	    	;;
	esac
	shift # past argument or value
done
