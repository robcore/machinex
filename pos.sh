#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0359-i-think-early-is-too-much-for-it-now.patch"
patch -p1 -R < "/root/machinex/patches/0358-put-the-reboot-command-back-in-sbin-not-actually-jus.patch"
patch -p1 -R < "/root/machinex/patches/0357-remove-improper-reboot-notifier-usage-change-back-to.patch"
patch -p1 -R < "/root/machinex/patches/0356-did-it-just-imported-the-platform-driver-code-ideas-.patch"
patch -p1 -R < "/root/machinex/patches/0355-another-fix.-motherFUCK-jian-yang.patch"
patch -p1 -R < "/root/machinex/patches/0354-fixup-setup-asm-also-use-online-cpu-in-c_show.patch"
patch -p1 -R < "/root/machinex/patches/0353-fix-my-reboot-mode-in-arm-setup.patch"
patch -p1 -R < "/root/machinex/patches/0352-stole-notifier-logic-from-s8-restart-driver.patch"
patch -p1 -R < "/root/machinex/patches/0351-import-reset-drivers-from-samsung-gs8.patch"
patch -p1 -R < "/root/machinex/patches/0350-a-few-minor-cleanups-in-sys.c-also-moved-the-autogro.patch"
patch -p1 -R < "/root/machinex/patches/0349-androidify-our-reboot-path.patch"
patch -p1 -R < "/root/machinex/patches/0348-there-synced-reboot-mostly-with-mainline.patch"
patch -p1 -R < "/root/machinex/patches/0347-updates-to-reboot-bringing-it-up-to-mainline-there-h.patch"