#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0125-wakelock-function-again.patch"
patch -p1 -R < "/root/machinex/patches/0124-sensorhub-again.patch"
patch -p1 -R < "/root/machinex/patches/0123-make-zswap-an-int-again.patch"
patch -p1 -R < "/root/machinex/patches/0122-REAL-configurable-bam-waklock-timeout-again.patch"
patch -p1 -R < "/root/machinex/patches/0121-possed.patch"