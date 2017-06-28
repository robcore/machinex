#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0081-another-squash.patch"
patch -p1 -R < "/root/machinex/patches/0080-blk-mq-blk_mq_tag_to_rq-should-handle-flush-request.patch"
patch -p1 -R < "/root/machinex/patches/0079-block-remove-dead-code-in-scsi_ioctl-blk_verify_com.patch"
patch -p1 -R < "/root/machinex/patches/0077-block-Convert-integrity-to-bvec_alloc_bs.patch"
