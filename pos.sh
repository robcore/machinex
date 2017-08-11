#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0014-that-will-fix-intelli-we-were-preventing-cpus-going-.patch"
patch -p1 -R < "/root/machinex/patches/0013-fix-late-init-in-cpu-and-add-common-functions-for-ha.patch"
patch -p1 -R < "/root/machinex/patches/0012-oops-dont-need.patch"
patch -p1 -R < "/root/machinex/patches/0011-move-checks-for-cpu-allowed-outside-of-semaphore-and.patch"
patch -p1 -R < "/root/machinex/patches/0010-fix-up-some-semantics.patch"
patch -p1 -R < "/root/machinex/patches/0009-trying-a-new-interface.patch"
