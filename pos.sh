#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0017-what-the-FUCK-is-happening.patch"
patch -p1 -R < "/root/machinex/patches/0016-cgroup-relocate-cgroup_advance_iter.patch"
patch -p1 -R < "/root/machinex/patches/0015-cgroup-make-hierarchy-iterators-deal-with-cgroup_sub.patch"
patch -p1 -R < "/root/machinex/patches/0014-cgroup-always-use-cgroup_next_child-to-walk-the-chil.patch"
patch -p1 -R < "/root/machinex/patches/0013-cgroup-convert-cgroup_next_sibling-to-cgroup_next.patch"
patch -p1 -R < "/root/machinex/patches/0012-cgroup-add-cgroup-dummy_css.patch"
patch -p1 -R < "/root/machinex/patches/0011-cgroup-pin-cgroup_subsys_state-when-opening-a-cgroup.patch"
patch -p1 -R < "/root/machinex/patches/0010-cgroup-add-subsys-backlink-pointer-to-cftype.patch"
patch -p1 -R < "/root/machinex/patches/0009-cgroup-pass-around-cgroup_subsys_state-instead-of-cg.patch"
patch -p1 -R < "/root/machinex/patches/0008-cgroup-add-css_parent.patch"
patch -p1 -R < "/root/machinex/patches/0007-brought-those-back.patch"
patch -p1 -R < "/root/machinex/patches/0006-hugetlb_cgroup-pass-around-hugetlb_cgroup.patch"
patch -p1 -R < "/root/machinex/patches/0005-i-see-now.patch"
patch -p1 -R < "/root/machinex/patches/0004-reverted.patch"
patch -p1 -R < "/root/machinex/patches/0003-fuck.patch"
patch -p1 -R < "/root/machinex/patches/0002-cgroup-add-subsystem-pointer-to-cgroup_subsys_state.patch"
patch -p1 -R < "/root/machinex/patches/0001-cgroup-add-update-accessors-which-obtain-subsys.patch"
