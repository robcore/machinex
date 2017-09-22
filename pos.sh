#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0056-squashed-include-work-and-dox.patch"
patch -p1 -R < "/root/machinex/patches/0054-lib-idr.c-remove-redundant-include.patch"
patch -p1 -R < "/root/machinex/patches/0053-lib-halfmd4.c-simplify-include.patch"
patch -p1 -R < "/root/machinex/patches/0052-lib-dynamic_queue_limits.c-simplify-includes.patch"
patch -p1 -R < "/root/machinex/patches/0051-lib-interval_tree.c-simplify-includes.patch"