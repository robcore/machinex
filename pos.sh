#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0015-changed-my-mind-about-workqueues-but-down-bother-war.patch"
patch -p1 -R < "/root/machinex/patches/0014-if-the-workqueue-is-already-freezing-still-label-it-.patch"
patch -p1 -R < "/root/machinex/patches/0013-fix-one-last-conversion-to-call_single_data_t.patch"
patch -p1 -R < "/root/machinex/patches/0012-added-smps-own-typedef-to-avoid-using-two-cachelines.patch"