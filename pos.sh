#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0027-what-am-i-doing-why-am-i-not-just-using-trlte.patch"
patch -p1 -R < "/root/machinex/patches/0026-ARM-perf-prepare-for-moving-CPU-PMU-code-into-separa.patch"
patch -p1 -R < "/root/machinex/patches/0025-ARM-perf-probe-devicetree-in-preference-to-current-C.patch"
patch -p1 -R < "/root/machinex/patches/0024-bs.patch"
patch -p1 -R < "/root/machinex/patches/0023-ARM-PMU-Add-runtime-PM-Support.patch"
patch -p1 -R < "/root/machinex/patches/0022-ARM-7448-1-perf-remove-arm_perf_pmu_ids-global-enume.patch"
patch -p1 -R < "/root/machinex/patches/0021-ARM-7441-1-perf-return-EOPNOTSUPP-if-requested-mode-.patch"