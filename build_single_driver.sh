#!/bin/bash
washme()
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

washme
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
make ARCH=arm -S -s -j6 O=$(pwd)/out $(pwd)/kernel/;
washme
