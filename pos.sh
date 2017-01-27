#!/bin/bash

patch -p1 < "patches/0400-update-gitignore-ps-HOLY-SHIT-IT-BOOTS-AND-WORKS.patch"
patch -p1 < "patches/0399-fuck-off.patch"
patch -p1 < "patches/0397-thp-copy_huge_pmd-copy-huge-zero-page.patch"
patch -p1 < "patches/0396-totally-fixed-it.-maybe.patch"
patch -p1 < "patches/0395-dont-need-that.patch"
patch -p1 < "patches/0394-I-HAVE-BEEN-LOOKING-IN-THE-WRONG-PLACE.patch"
patch -p1 < "patches/0393-thp-huge-zero-page-basic-preparation.patch"
patch -p1 < "patches/0392-bootmem-remove-alloc_arch_preferred_bootmem.patch"
patch -p1 < "patches/0391-bootmem-remove-not-implemented-function-call-bootmem.patch"
patch -p1 < "patches/0390-uapi-add-missing-netconf.h-to-export-list.patch"
patch -p1 < "patches/0389-ktest-fixes.patch"
patch -p1 < "patches/0388-trying-again.patch"
