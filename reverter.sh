#!/bin/bash

patch -p1 -R < patches/0150-reverted-memory-stuff-again-for-now.patch;
patch -p1 -R < patches/0149-msm-simplify-placing-memory-pools.patch;
patch -p1 -R < patches/0148-msm-Remove-android_pmem.h-from-board-and-devices.patch;
patch -p1 -R < patches/0147-msm-Remove-alloc-bootmem-aligned.patch;
patch -p1 -R < patches/0146-Add-watchdog-init-complete-hooks-for-partial-resume.patch;
patch -p1 -R < patches/0142-regulator-fixed-Add-support-for-parent-supply-device.patch;
patch -p1 -R < patches/0140-revert-this.patch;
patch -p1 -R < patches/0136-lib-memory_alloc-Support-64-bit-physical-addresses.patch;
patch -p1 -R < patches/0135-msm-memory-Use-phys_addr_t-as-the-return-type-for-eb.patch;
patch -p1 -R < patches/0134-revert-this.patch;
patch -p1 -R < patches/0133-lib-genalloc-Use-64-bit-types-for-holding-allocation.patch
