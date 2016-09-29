#!/bin/bash
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
