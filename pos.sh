#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0039-syntax-is-everything.patch"
patch -p1 -R < "/root/machinex/patches/0038-use-clamp_val-instead-of-sanitize.patch"
patch -p1 -R < "/root/machinex/patches/0037-aaaand-go-to-top-now-because-having-two-gotos-is-unn.patch"
patch -p1 -R < "/root/machinex/patches/0036-methinks-we-need-to-wait-until-the-system-is-running.patch"
patch -p1 -R < "/root/machinex/patches/0035-iron-out-some-of-the-little-details-and-see-how-this.patch"
patch -p1 -R < "/root/machinex/patches/0034-OOPS-i-confused-my-last_fuelcheck-with-last-boost-ti.patch"
patch -p1 -R < "/root/machinex/patches/0033-thermal-simplify-init-by-copying-the-nonboot-cpumask.patch"