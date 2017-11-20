#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0036-aaaand-make-sure-that-we-actually-call-the-function.patch"
patch -p1 -R < "/root/machinex/patches/0035-just-set-up-the-policy-vals-in-the-percpu-policy-set.patch"
patch -p1 -R < "/root/machinex/patches/0034-move-some-initialization-around-also-convert-default.patch"
patch -p1 -R < "/root/machinex/patches/0033-the-rwsem-was-a-bad-idea-lets-see-if-the-pointers-he.patch"
patch -p1 -R < "/root/machinex/patches/0032-syncronize-our-reads-and-writes-to-hardlimit-policy-.patch"
patch -p1 -R < "/root/machinex/patches/0031-finished-up-the-percpu-hardlimit-implementation.-let.patch"
patch -p1 -R < "/root/machinex/patches/0030-this-may-be-for-efficient-turned-hardlimit-into-a-pe.patch"