#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0010-fuck-i-would-have-to-look-into-this-too-much-and-am-.patch"
patch -p1 -R < "/root/machinex/patches/0009-fix-up-some-jiffies-calculations.patch"
patch -p1 -R < "/root/machinex/patches/0008-ntp-fix-timespec-adjust-calculation.patch"