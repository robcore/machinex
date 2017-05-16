#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0200-REVERT-add-rcu-fastpath-for-funnel-lock.patch"
patch -p1 -R < "/root/machinex/patches/0199-fix-a-const-at-least.patch"
patch -p1 -R < "/root/machinex/patches/0198-reverting-ALL-the-fucking-rcu-commits-from-today-in-.patch"
patch -p1 -R < "/root/machinex/patches/0197-FUCK.patch"
patch -p1 -R < "/root/machinex/patches/0196-rcu-Drive-expedited-grace-periods-from-workqueue-thi.patch"
patch -p1 -R < "/root/machinex/patches/0195-REVERT-uaccess-commits.patch"
patch -p1 -R < "/root/machinex/patches/0194-use-rcu_gp_kthread_wake-rsp.patch"
patch -p1 -R < "/root/machinex/patches/0193-i-am-missing-rcu-prereqs-hard.patch"