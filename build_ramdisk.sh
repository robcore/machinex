#!/bin/bash

PREV=`cat /media/root/robcore/AIK/previous.txt`

cd /media/root/robcore/AIK;
echo -n "New Name? [ENTER]: "
read NEW
cp -pR $PREV $NEW
rm $NEW/boot.img
rm image-new.img;
sh repackimg.sh --sudo;
cp -p image-new.img $NEW/boot.img
cd $NEW
rm $PREV.zip
zip -r -9 - * > $NEW.zip