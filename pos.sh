#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0060-perhaps-this-should-be-wait_on_bit_lock-as-we-clear-.patch"
patch -p1 -R < "/root/machinex/patches/0059-possed.patch"
patch -p1 -R < "/root/machinex/patches/0058-possing-a-bit.patch"
patch -p1 -R < "/root/machinex/patches/0057-brought-back-mpu-and-cache-erp.patch"
patch -p1 -R < "/root/machinex/patches/0056-i-may-want-to-try-using-count-and-just-making-it-nr_.patch"
patch -p1 -R < "/root/machinex/patches/0055-TECHNICALLY-i-should-be-able-to-get-away-with-this.patch"
patch -p1 -R < "/root/machinex/patches/0054-fix-up-nr_running-count.patch"
patch -p1 -R < "/root/machinex/patches/0053-sched-avoid-running-idle_balance-on-behalf-of-wrong-.patch"
patch -p1 -R < "/root/machinex/patches/0052-sched-Fix-bug-in-average-nr_running-and-nr_iowait-ca.patch"
patch -p1 -R < "/root/machinex/patches/0051-fix-wait.c-smp_store_mb.patch"
patch -p1 -R < "/root/machinex/patches/0050-dunno-if-we-were-building-our-much-need-generic-hard.patch"
patch -p1 -R < "/root/machinex/patches/0049-endif.patch"
patch -p1 -R < "/root/machinex/patches/0048-a-doubled-commit-in-wait.c.patch"
patch -p1 -R < "/root/machinex/patches/0047-add-need_rebind.patch"
patch -p1 -R < "/root/machinex/patches/0046-cleaned-up-config.patch"
patch -p1 -R < "/root/machinex/patches/0045-synced-workqueue-with-3.18-sammy-changed-idr_get_new.patch"
patch -p1 -R < "/root/machinex/patches/0044-workqueue-apply-__WQ_ORDERED-to-create_singlethread.patch"
patch -p1 -R < "/root/machinex/patches/0043-workqueue-use-nr_node_ids-instead-of-wq_numa_tbl_len.patch"
patch -p1 -R < "/root/machinex/patches/0042-workqueue-remove-the-misnamed-out_unlock-label.patch"
patch -p1 -R < "/root/machinex/patches/0041-dox.patch"
patch -p1 -R < "/root/machinex/patches/0040-fixup-remove-rescuer-pool.patch"
patch -p1 -R < "/root/machinex/patches/0039-workqueue-unfold-start_worker-into-create_worker.patch"
patch -p1 -R < "/root/machinex/patches/0038-workqueue-remove-wakeup-from-worker_set_flags.patch"
patch -p1 -R < "/root/machinex/patches/0037-workqueue-remove-an-unneeded-UNBOUND-test-before-wak.patch"
patch -p1 -R < "/root/machinex/patches/0036-workqueue-alloc-struct-worker-on-its-local-node.patch"
patch -p1 -R < "/root/machinex/patches/0035-workqueue-reuse-the-already-calculated-pwq.patch"
patch -p1 -R < "/root/machinex/patches/0034-workqueue-zero-cpumask-of-wq_numa_possible_cpumask.patch"
patch -p1 -R < "/root/machinex/patches/0033-missing-POOL-DISASOSSIATED-flag-in-rebind.patch"
patch -p1 -R < "/root/machinex/patches/0032-workqueue-fix-dev_set_uevent_suppress-imbalance.patch"
patch -p1 -R < "/root/machinex/patches/0031-workqueue-clear-POOL_DISASSOCIATED-in-rebind_workers.patch"
patch -p1 -R < "/root/machinex/patches/0030-workqueue-sanity-check-pool-cpu-in-wq_worker_sleepin.patch"
patch -p1 -R < "/root/machinex/patches/0029-workqueue-clear-leftover-flags-when-detached.patch"
patch -p1 -R < "/root/machinex/patches/0028-workqueue-remove-useless-WARN_ON_ONCE.patch"
patch -p1 -R < "/root/machinex/patches/0027-turn-off-arm-mpu-and-msm-cache-erp.patch"
patch -p1 -R < "/root/machinex/patches/0026-workqueue-remove-the-empty-check-in-too_many_workers.patch"
patch -p1 -R < "/root/machinex/patches/0025-workqueue-use-pool-cpu-0-to-stand-for-an-unbound-poo.patch"

