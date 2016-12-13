#!/bin/bash
PREV=`cat /media/root/robcore/AIK/previous.txt`

cd /media/root/robcore/AIK;
rm $PREV/boot.img
rm image-new.img;
sh repackimg.sh --sudo;
cp -p image-new.img $(pwd)/$PREV/boot.img
cd $PREV
zip -r -9 - * > $PREV.zip
