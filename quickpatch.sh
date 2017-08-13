#!/bin/bash
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/2556-dm-thin-synchronize-the-pool-mode-during-suspend.patch"
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/3096-dm-thin-ensure-user-takes-action-to-validate-data-an.patch"
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/3097-dm-thin-fix-out-of-data-space-handling.patch"
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/3098-dm-thin-fix-deadlock-in-__requeue_bio_list.patch"
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/3099-dm-thin-fix-noflush-suspend-IO-queueing.patch"
patch -p1 < "/media/root/robcore/android/linaro-8064-lt/3.18/patches/6patches/3673-dm-space-map-metadata-fix-refcount-decrement-below-0.patch"