#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0013-fixing-hardplug-for-now.patch"
patch -p1 -R < "/root/machinex/patches/0012-intelli-hard-plugs-if-hardplug-has-any-screen-on-wor.patch"
patch -p1 -R < "/root/machinex/patches/0011-check-for-limit_screen_on_cpus-in-prometheus.-bump-t.patch"
patch -p1 -R < "/root/machinex/patches/0010-hardplug-2.3-stricter-checking-for-the-two-condition.patch"
patch -p1 -R < "/root/machinex/patches/0009-cpu_hardplug-2.2-apply-a-global-cpumask-utilizing-th.patch"
patch -p1 -R < "/root/machinex/patches/0008-create-a-new-global-cpumask-that-tracks-just-the-non.patch"
patch -p1 -R < "/root/machinex/patches/0007-intelli-14.5-cleanup-some-of-our-initializers-to-avo.patch"
patch -p1 -R < "/root/machinex/patches/0006-hardplug-2.1-forget-testing-for-cpu_online-regarding.patch"
patch -p1 -R < "/root/machinex/patches/0005-remove-the-unnecessary-continue-from-my-cpu-iterator.patch"
patch -p1 -R < "/root/machinex/patches/0004-throw-in-some-extra-conditions-to-nr-hardplugged-cpu.patch"
patch -p1 -R < "/root/machinex/patches/0003-2.1-check-to-see-if-the-cpu-is-allowed-on-hardplug-c.patch"
patch -p1 -R < "/root/machinex/patches/0002-fixed-an-Lanchor-thing.patch"
patch -p1 -R < "/root/machinex/patches/0001-intelliplug-14.4-hardplug-2.0-expose-the-real-max-cp.patch"
