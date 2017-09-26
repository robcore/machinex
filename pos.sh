#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0043-bus-drv-p-is-freed-in-driver_release.patch"
patch -p1 -R < "/root/machinex/patches/0042-add-the-missing-kobject-actions.patch"
patch -p1 -R < "/root/machinex/patches/0041-oops-missed-the-first-half-of-the-file.patch"
patch -p1 -R < "/root/machinex/patches/0040-added-some-helpers-cleaned-up-deferred-probing-a-bit.patch"
patch -p1 -R < "/root/machinex/patches/0039-my-guess-is-we-dont-use-soc-bus-driver.patch"
patch -p1 -R < "/root/machinex/patches/0038-im-guessing-we-barely-use-device-memory.patch"