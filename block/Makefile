#
# Makefile for the kernel block layer
#

obj-$(CONFIG_BLOCK) := elevator.o blk-core.o blk-tag.o blk-sysfs.o \
			blk-flush.o blk-settings.o blk-ioc.o blk-map.o \
			blk-exec.o blk-merge.o blk-softirq.o blk-timeout.o \
			blk-iopoll.o blk-lib.o ioctl.o genhd.o scsi_ioctl.o \
			cause_tags.o partition-generic.o partitions/
obj-$(CONFIG_IOSCHED_AFQ) := acct_afq_sched.o \
	split_sched.o split_account.o ioctl_helper.o io_batch.o disk_sim.o
obj-$(CONFIG_IOSCHED_SPLIT_DEADLINE) := split_deadline_sched.o \
	split_sched.o split_account.o ioctl_helper.o io_batch.o disk_sim.o \
	expire_rb.o io_work_expire_rb.o
obj-$(CONFIG_IOSCHED_TB) := tbucket.o ioctl_helper.o io_batch.o disk_sim.o

obj-$(CONFIG_BLK_DEV_BSG)	+= bsg.o
obj-$(CONFIG_BLK_DEV_BSGLIB)	+= bsg-lib.o
obj-$(CONFIG_BLK_CGROUP)	+= blk-cgroup.o
obj-$(CONFIG_BLK_DEV_THROTTLING)	+= blk-throttle.o
obj-$(CONFIG_IOSCHED_NOOP)	+= noop-iosched.o
obj-$(CONFIG_IOSCHED_DEADLINE)	+= deadline-iosched.o
obj-$(CONFIG_IOSCHED_ROW)	+= row-iosched.o
obj-$(CONFIG_IOSCHED_CFQ)	+= cfq-iosched.o
obj-$(CONFIG_IOSCHED_BFQ)	+= bfq-iosched.o
obj-$(CONFIG_IOSCHED_TEST)	+= test-iosched.o
obj-$(CONFIG_IOSCHED_SIO)       += sio-iosched.o
obj-$(CONFIG_IOSCHED_FIFO)      += fifo-iosched.o
obj-$(CONFIG_IOSCHED_ZEN)       += zen-iosched.o
obj-$(CONFIG_IOSCHED_VR)        += vr-iosched.o
obj-$(CONFIG_IOSCHED_FIOPS)     += fiops-iosched.o
obj-$(CONFIG_IOSCHED_SIOPLUS)  += sioplus-iosched.o
obj-$(CONFIG_IOSCHED_TRIPNDROID)	+= tripndroid-iosched.o

obj-$(CONFIG_BLOCK_COMPAT)	+= compat_ioctl.o
obj-$(CONFIG_BLK_DEV_INTEGRITY)	+= blk-integrity.o
