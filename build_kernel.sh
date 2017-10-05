#!/bin/bash

export CONCURRENCY_LEVEL=$(cat /proc/cpuinfo | grep processor | wc -l)
export CCACHE_NLEVELS=8

################
##LINARO-May#
#TOOLCHAIN=/root/toolchains/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
#export PATH=/root/toolchains/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin:$PATH
###############

################
##LINARO-January#
#TOOLCHAIN=/root/toolchains/gcc-linaro-5.4.1-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
#export PATH=/root/toolchains/gcc-linaro-5.4.1-2017.01-x86_64_arm-linux-gnueabihf/bin:$PATH
###############

###############
#LATEST_CORTEX#
#TOOLCHAIN=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf/bin:$PATH
##############

########
#OLDEST#
#TOOLCHAIN=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
#export PATH=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin:$PATH
#######

#######
#Skydragon
#TOOLCHAIN=/root/skydragon/bin/arm-cortex_a15-linux-gnueabihf-
#export PATH=/root/skydragon/bin:$PATH
#######

#ROBCORE'S TOOLCHAIN
TOOLCHAIN=/root/x-tools/arm-cortex_a15-linux-gnueabihf/bin/arm-cortex_a15-linux-gnueabihf-
export PATH=/root/x-tools/arm-cortex_a15-linux-gnueabihf/bin:$PATH
#######

declare -g OUTFOLDER

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
if [ -e /media/root/robcore/AIK/prev_mx_ver.txt ]; then
	MXPREV=`cat /media/root/robcore/AIK/prev_mx_ver.txt`
fi;

function countdown()
{
	echo "short sleep"
	sleep 1
}

function adbcountdown()
{
	echo "countdown start"
	sleep 3
	echo "countdown end"
}

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
ADBTYPE=`adb get-devpath 2> /dev/null`
adbcountdown
adb start-server
adbcountdown
if [ "$ONLINE" = "recovery" ]; then #if we are in recovery
		echo "recovery connected"
		echo "pushing $1"
		adb push $1 /external_sd;
		echo "push complete"
		adb kill-server
elif [ "$ONLINE" = "device" ]; then #if we are in recovery
		echo "connected"
		echo "pushing $1"
		adb shell su -c "input keyevent KEYCODE_WAKEUP"
		sleep 1
		adb shell su -c "input touchscreen swipe 930 880 930 380"
		sleep 1
		adb push $1 /storage/extSdCard;
		echo "push complete, booting recovery"
		adb shell su -c "input keyevent KEYCODE_WAKEUP"
		adb shell su -c "echo 0 > /sys/module/restart/parameters/download_mode"
		adb shell su -c "reboot recovery"
		adb kill-server
else
	adb kill-server
	adbcountdown
	adb connect 192.168.1.111
	if [ $? -eq 0 ]; then
		echo "Connected! Pushing $1!"
		adbcountdown
		echo "Trying Wireless"
		adb push $1 /storage/extSdCard;
		adb disconnect
		adb kill-server
	else
		echo "Failed!"
	fi;
fi
}

function NORMAL()
{
echo "Building NORMAL kernel"
echo "your previous version was $PREV"
echo -n "Name? (Number-P-N) [ENTER]: "
read OVNAME
OUTFOLDER=machinex-Mark$OVNAME
if [ -d /media/root/robcore/AIK/$OUTFOLDER ]; then
	echo "removing previously compiled folder and zip of the same name"
	rm -rf /media/root/robcore/AIK/$OUTFOLDER
fi;
	echo "$OUTFOLDER" > /media/root/robcore/AIK/previous.txt
	echo "$OVNAME" > /media/root/robcore/AIK/prev_mx_ver.txt

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
sed -i '/CONFIG_MACHINEX_VERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig
echo CONFIG_MACHINEX_VERSION='"'$OVNAME'"' >> arch/arm/configs/tmpconfig

echo -n "Automatically push to adb and cleanup the project?  y/n [ENTER]: "
read AUTO
export SUBARCH=arm
export ARCH=arm
export CROSS_COMPILE=$TOOLCHAIN
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
schedtool -B -n 1 -e make SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
		cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	fi
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	if [ -e image-new.img ]; then
		rm image-new.img;
	fi
	sudo sh repackimg.sh
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=($(md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip))
	echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
	date >> ~/machinex/datetracker.txt
	echo "------------------------" >> ~/machinex/datetracker.txt
	cp ~/machinex/out/vmlinux ~/machinex/robstuff/vmlinux;
	if [[ $AUTO = "n" ]]; then
		echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
		read repadb
		if [[ $repadb = "y" ]]; then
			echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
			echo "ENABLE ADB"
			ADBPATH=/media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip
			ADBRETRY $ADBPATH
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
		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		echo "ENABLE ADB"
		ADBPATH=/media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip
		ADBRETRY $ADBPATH
		cd ~/machinex
		WASHME
		echo "cleanup finished"
	fi;

	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "MD5 is $SUMMY"
else
	WASHME
	echo "Build failed, Skipped Ramdisk Creation"
fi;
}

function REBUILD()
{
echo "REBUILDING Previous Kernel"
echo "your previous version was $PREV"

PRVS=$PREV
if [ -d /media/root/robcore/AIK/$PRVS ]; then
	echo "removing previously compiled folder and zip of the same name"
	rm -rf /media/root/robcore/AIK/$PRVS
fi;
OUTFOLDER=$PRVS

cp -pf arch/arm/configs/canadefconfig arch/arm/configs/tmpconfig
sed -i '/CONFIG_LOCALVERSION=/d' arch/arm/configs/tmpconfig
sed -i '/CONFIG_MACHINEX_VERSION=/d' arch/arm/configs/tmpconfig
echo CONFIG_LOCALVERSION='"''-'$OUTFOLDER'"' >> arch/arm/configs/tmpconfig
echo CONFIG_MACHINEX_VERSION='"'$MXPREV'"' >> arch/arm/configs/tmpconfig

export SUBARCH=arm
export ARCH=arm
export CROSS_COMPILE=$TOOLCHAIN
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
schedtool -B -n 1 -e make SUBARCH=arm ARCH=arm -S -s -j6 O=$(pwd)/out;
if [ -e ~/machinex/out/arch/arm/boot/zImage ]; then
	cd /media/root/robcore/AIK;
	cp -R -p machina-new $OUTFOLDER;
	if [ -e ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko ]; then
		cp -p ~/machinex/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/$OUTFOLDER/system/lib/modules/dhd.ko;
	fi
	rm $(pwd)/split_img/boot.img-zImage;
	cp -p ~/machinex/out/arch/arm/boot/zImage $(pwd)/split_img/boot.img-zImage;
	if [ -e image-new.img ]; then
		rm image-new.img;
	fi
	sudo sh repackimg.sh
	cp -p image-new.img $(pwd)/$OUTFOLDER/boot.img
	cd $OUTFOLDER
	zip -r -9 - * > $OUTFOLDER.zip
	#SDB=`adb shell md5sum /storage/extSdCard/$OUTFOLDER.zip`
	SUMMY=($(md5sum /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip))
	echo "$OUTFOLDER was built on:" >> ~/machinex/datetracker.txt
	date >> ~/machinex/datetracker.txt
	echo "------------------------" >> ~/machinex/datetracker.txt
	cp ~/machinex/out/vmlinux ~/machinex/robstuff/vmlinux;
	echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
	echo "ENABLE ADB"
	ADBPATH=/media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip
	ADBRETRY $ADBPATH
	cd ~/machinex
	WASHME
	echo "cleanup finished"
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
	     -r|--rebuild)
	    	REBUILD
	    	break
	    	;;

	     -c|--chkvers)
	    	CHKVERS
	    	break
	    	;;

	    *)
	    	NORMAL
	    	break;
	    	;;
	esac
	shift # past argument or value
done
