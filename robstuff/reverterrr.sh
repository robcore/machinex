#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0100-state-notifier-make-the-debugging-less-of-a-hassle-s.patch"
patch -p1 -R < "/root/machinex/patches/0099-rename-state-notifier-definition-so-as-not-to-cause-.patch"
patch -p1 -R < "/root/machinex/patches/0098-syncronized-suspend-resume-driver-printing-of-ignore.patch"
patch -p1 -R < "/root/machinex/patches/0097-modified-state-notifier-to-again-only-register-a-wak.patch"
patch -p1 -R < "/root/machinex/patches/0096-reverted-that-knew-i-probably-didnt-have-the-prereq.patch"
patch -p1 -R < "/root/machinex/patches/0095-soc-qcom-bam_dmux-Use-SPS-atomic-allocation-flag.patch"
patch -p1 -R < "/root/machinex/patches/0094-enable-mdp-hw-init-unconditionally-again.patch"
patch -p1 -R < "/root/machinex/patches/0093-REVERTED-FOR-NOW-THAT-I-HAVE-IT-BACK-ON-MY-RADAR.-CA.patch"
patch -p1 -R < "/root/machinex/patches/0092-PM-IRQF_NO_SUSPEND-PLACEHOLDER.patch"
patch -p1 -R < "/root/machinex/patches/0091-REVERT-cfb-fill-rect-again.patch"
patch -p1 -R < "/root/machinex/patches/0090-REVERT-added-showp-generic-sweep2wake.-k-i-am-now-re.patch"
patch -p1 -R < "/root/machinex/patches/0089-added-showp-generic-sweep2wake-sleep.patch"
patch -p1 -R < "/root/machinex/patches/0088-ARM-8517-1-ICST-avoid-arithmetic-overflow-in-icst_hz.patch"
patch -p1 -R < "/root/machinex/patches/0087-trying-again-for-fun-and-little-profit-Optimize-kern.patch"
patch -p1 -R < "/root/machinex/patches/0086-brought-those-back-the-camera-not-only-works-but-is-.patch"
patch -p1 -R < "/root/machinex/patches/0085-reverted-the-test-patches-quickly-in-order-to-releas.patch"
patch -p1 -R < "/root/machinex/patches/0084-this-will-probably-break-the-camera-but-msm-mpq8064-.patch"
patch -p1 -R < "/root/machinex/patches/0083-interesting.msm-vidc-Fix-clock-and-bus-scaling.patch"
patch -p1 -R < "/root/machinex/patches/0082-msm_fb-MHL-MSC-cmd-schedule-to-prevent-overload.patch"
patch -p1 -R < "/root/machinex/patches/0081-msm_fb-MHL-MSC-RCP-RAP-feature-implementation.patch"
patch -p1 -R < "/root/machinex/patches/0080-dont-need-it.patch"
patch -p1 -R < "/root/machinex/patches/0079-usb-dwc3-Disable-host-mode-if-operational-mode-is-DR.patch"
patch -p1 -R < "/root/machinex/patches/0078-arm-dt-msm8974-Add-SDCC-BAM-details.patch"
patch -p1 -R < "/root/machinex/patches/0077-DANGER-msm-media-Add-error-check-in-memory-alloc-fun.patch"
patch -p1 -R < "/root/machinex/patches/0076-reverted.patch"
patch -p1 -R < "/root/machinex/patches/0075-fucker.patch"
patch -p1 -R < "/root/machinex/patches/0074-ROBTEST-REVERT-video-msm_fb-Increase-display-timeout.patch"
patch -p1 -R < "/root/machinex/patches/0073-ROBTEST-REVERT-msm_fb-Allow-MDP-hardware-init-for-co.patch"
patch -p1 -R < "/root/machinex/patches/0072-REVERTish-well-comment-out-Update-footswitch_disable.patch"


