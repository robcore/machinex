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
	sleep 1
}

function adbcountdown()
{
	echo "3"
	sleep 1
	echo "2"
	sleep 1
	echo "1"
	sleep 1
}

function ADBRETRY()
{
ONLINE=`adb get-state 2> /dev/null`
adb start-server
adb root
adbcountdown

if [[ $ONLINE == recovery ]]; then #if we are in recovery
	echo "recovery connected"
	adbcountdown
	adb push $1.zip /external_sd
	echo "push complete"
	adb kill-server
elif [[ $ONLINE == device ]]; then #if we are in os, connected via usb
	echo "connected"
	adbcountdown
	adb shell su -c "input keyevent KEYCODE_WAKEUP"
	countdown
	adb push $1.zip /storage/extSdCard
	echo "push complete, booting recovery"
	adb reboot recovery
	adb kill-server
else
	echo "Nothing Connected, giving up"
	adb kill-server
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
	echo "------------------------" >> ~/machinex/datetracker.txt
	cp ~/machinex/out/vmlinux ~/machinex/robstuff/vmlinux;
	if [[ $AUTO = "n" ]]; then
		echo -n "Shall I adb push this for you, sir?  y/n [ENTER]: "
		read repadb
		if [[ $repadb = "y" ]]; then
			echo "ENABLE ADB"
			ADBRETRY $OUTFOLDER
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
		echo "ENABLE ADB"
		echo "Kernel is located in /media/root/robcore/AIK/$OUTFOLDER/$OUTFOLDER.zip"
		ADBRETRY $OUTFOLDER
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
	echo "------------------------" >> ~/machinex/datetracker.txt
	cp ~/machinex/out/vmlinux ~/machinex/robstuff/vmlinux;
	echo "ENABLE ADB"
	ADBRETRY $OUTFOLDER
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
