#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0024-lseek_execute-doesn-t-need-an-inode-passed-to-it.patch"
patch -p1 -R < "/root/machinex/patches/0023-block_dev-switch-to-fixed_size_llseek.patch"
