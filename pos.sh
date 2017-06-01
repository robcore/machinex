#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0006-maybe-i-dont-need-get-put-online-cpus-in-thermal.patch"
patch -p1 -R < "/root/machinex/patches/0005-technically-we-dont-do-post-outside-of-our-target-ro.patch"
patch -p1 -R < "/root/machinex/patches/0004-semi-use-original-sizing.patch"
patch -p1 -R < "/root/machinex/patches/0003-betcha-that-is-my-section-mismatch.patch"
patch -p1 -R < "/root/machinex/patches/0002-holy-shit-it-built.patch"
patch -p1 -R < "/root/machinex/patches/0001-krait-fix-my-hotplug-call-back-so-we-are-sending-fre.patch"
