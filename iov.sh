#!/bin/bash

patch -p1 -R < "/root/machinex/patches-all/2patches/3192-iov_iter-move-into-its-own-file-bug-fix.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3178-iov_iter-add-a-shorten-call.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3177-iov_iter-add-bvec-support.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3176-iov_iter-hide-iovec-details-behind-ops-function-poin.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3174-iov_iter-add-copy_to_user-support.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3173-iov_iter-iov_iter_copy_from_user-should-use-non-atom.patch"

patch -p1 -R < "/root/machinex/patches-all/2patches/3172-iov_iter-move-into-its-own-file.patch"