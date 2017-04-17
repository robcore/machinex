#!/bin/bash

patch -p1 < "/root/machinex/patches/0025-workqueue-use-pool-cpu-0-to-stand-for-an-unbound-poo.patch"
patch -p1 < "/root/machinex/patches/0026-workqueue-remove-the-empty-check-in-too_many_workers.patch"
patch -p1 < "/root/machinex/patches/0028-workqueue-remove-useless-WARN_ON_ONCE.patch"
patch -p1 < "/root/machinex/patches/0029-workqueue-clear-leftover-flags-when-detached.patch"
patch -p1 < "/root/machinex/patches/0030-workqueue-sanity-check-pool-cpu-in-wq_worker_sleepin.patch"
patch -p1 < "/root/machinex/patches/0031-workqueue-clear-POOL_DISASSOCIATED-in-rebind_workers.patch"
patch -p1 < "/root/machinex/patches/0032-workqueue-fix-dev_set_uevent_suppress-imbalance.patch"