#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0060-possed.patch"
patch -p1 -R < "/root/machinex/patches/0059-possing.patch"
patch -p1 -R < "/root/machinex/patches/0058-we-will-never-get-nice-things.patch"
patch -p1 -R < "/root/machinex/patches/0057-there.patch"
patch -p1 -R < "/root/machinex/patches/0056-fuck-this.patch"
patch -p1 -R < "/root/machinex/patches/0055-move-shit.patch"
patch -p1 -R < "/root/machinex/patches/0054-stop-updating-cpu-policy-from-stats-again.patch"
patch -p1 -R < "/root/machinex/patches/0053-get-rid-of-cpu-avg-and-util.patch"
patch -p1 -R < "/root/machinex/patches/0052-remove-unused-mperf-support.patch"
patch -p1 -R < "/root/machinex/patches/0051-cpufreq-Fix-serialization-of-frequency-transitions.patch"
patch -p1 -R < "/root/machinex/patches/0050-cpufreq-make-sure-frequency-transitions-are-serializ.patch"
patch -p1 -R < "/root/machinex/patches/0049-cpufreq-Make-sure-CPU-is-running-on-a-freq-from-freq.patch"
patch -p1 -R < "/root/machinex/patches/0048-now-that-ive-found-that-rt-really-its-preempt-is-the.patch"
patch -p1 -R < "/root/machinex/patches/0047-nevermind-should-probably-have-it.patch"
patch -p1 -R < "/root/machinex/patches/0046-turn-off-suspend-time-timekeeping-again.patch"
patch -p1 -R < "/root/machinex/patches/0045-reverted-those.patch"
patch -p1 -R < "/root/machinex/patches/0044-i-will-never-understand-fucking-cpufreq.patch"
patch -p1 -R < "/root/machinex/patches/0043-attempting-again-the-revert-of-adding-removing-sysfs.patch"
patch -p1 -R < "/root/machinex/patches/0042-cpufreq-Introduce-macros-for-cpufreq_frequency_table.patch"