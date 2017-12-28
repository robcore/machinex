#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0007-trying-this-the-flar2-way.-disable-compander-adjustm.patch"
patch -p1 -R < "/root/machinex/patches/0006-thus-initialize-it-to-12-at-the-outset.patch"
patch -p1 -R < "/root/machinex/patches/0005-make-my-offset-easier-leave-it-out-of-userspace.patch"
patch -p1 -R < "/root/machinex/patches/0004-should-probably-explicitly-initialize-that.patch"
patch -p1 -R < "/root/machinex/patches/0003-new-headphone-pa-gain-implementation-inaugerel-test-.patch"
patch -p1 -R < "/root/machinex/patches/0002-move-some-more-defines-to-the-header.patch"
patch -p1 -R < "/root/machinex/patches/0001-throw-the-snd-engine-codec-ptr-in-the-local-header.patch"
