#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0010-those-can-stay-i-did-some-clock-tinkering-too.patch"
patch -p1 -R < "/root/machinex/patches/0009-better.patch"
patch -p1 -R < "/root/machinex/patches/0008-did-just-that.patch"
patch -p1 -R < "/root/machinex/patches/0007-fixups-in-our-clock-driver-too.patch"
patch -p1 -R < "/root/machinex/patches/0006-oh-snap-i-definitely-thought-that-clk_disable_unprep.patch"
patch -p1 -R < "/root/machinex/patches/0005-fuck-me-disable_unprepare.patch"
patch -p1 -R < "/root/machinex/patches/0004-after-all-that-a-superfluous-endif.patch"
patch -p1 -R < "/root/machinex/patches/0003-a-great-deal-of-hacking.-this-will-take-a-while-but-.patch"
