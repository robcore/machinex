#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0008-nope.patch"
patch -p1 -R < "/root/machinex/patches/0007-cleaned-up-the-aftermath.patch"
patch -p1 -R < "/root/machinex/patches/0006-Intelli_plug-kernel-sched-core-add-per-cpu-nr_runnin.patch"
patch -p1 -R < "/root/machinex/patches/0005-intelli_plug-refactor-stats-calculation-code-to-be-l.patch"
patch -p1 -R < "/root/machinex/patches/0004-REVERT-sched-computer-time-agverage-nrJ_running.patch"
patch -p1 -R < "/root/machinex/patches/0003-Revert-scheduler-Re-compute-time-average-nr_running-.patch"
patch -p1 -R < "/root/machinex/patches/0002-create-wrapper-for-on-rq-and-some-other-hax.patch"
patch -p1 -R < "/root/machinex/patches/0001-some-cleanup-in-cpu-accounting.patch"
