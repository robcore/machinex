#!/bin/bash
patch -p1 -R < "patches/0338-ion-fixup-some-of-our-updated-lock-removals-in-failc.patch"
patch -p1 -R < "patches/0337-gpu-ion-Delete-invalid-extra-file.patch"
patch -p1 -R < "patches/0336-gpu-ion-add-locking-to-traversal-of-volatile-rb-tree.patch"
patch -p1 -R < "patches/0335-gpu-ion-fix-locking-issues-in-debug-code.patch"
patch -p1 -R < "patches/0334-ion-cma-Add-debug-heap-ops-for-CMA-heap.patch"
patch -p1 -R < "patches/0333-gpu-ion-use-a-list-instead-of-a-tree-for-heap-debug.patch"
patch -p1 -R < "patches/0332-TEST-gpu-ion-refactor-locking.patch"
