/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/bitops.h>
#include <linux/string_helpers.h>
#include <linux/delay.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/sysfs.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <asm/uaccess.h>

#include "queue.h"

MODULE_ALIAS("mmc:block");
#if defined(CONFIG_MMC_CPRM)
#include "cprmdrv_samsung.h"
#include <linux/ioctl.h>
#define MMC_IOCTL_BASE		0xB3 /* Same as MMC block device major number */
#define MMC_IOCTL_GET_SECTOR_COUNT	_IOR(MMC_IOCTL_BASE, 100, int)
#define MMC_IOCTL_GET_SECTOR_SIZE		_IOR(MMC_IOCTL_BASE, 101, int)
#define MMC_IOCTL_GET_BLOCK_SIZE		_IOR(MMC_IOCTL_BASE, 102, int)
#define MMC_IOCTL_SET_RETRY_AKE_PROCESS		_IOR(MMC_IOCTL_BASE, 104, int)

static int cprm_ake_retry_flag;
#endif
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "mmcblk."

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88
#define MMC_BLK_TIMEOUT_MS  (20 * 1000)        /* 20sec timeout */

#define MMC_SANITIZE_REQ_TIMEOUT (60*60*1000) /* 1 hour in msec */

#define mmc_req_rel_wr(req)	(((req->cmd_flags & REQ_FUA) || \
			(req->cmd_flags & REQ_META)) && \
			(rq_data_dir(req) == WRITE))
#define PACKED_CMD_VER		0x01
#define PACKED_CMD_WR		0x02
#define MMC_BLK_MAX_RETRIES 5 /* max # of retries before aborting a command */
#define MMC_BLK_UPDATE_STOP_REASON(stats, reason)			\
	do {								\
		if (stats->enabled)					\
			stats->pack_stop_reason[reason]++;		\
	} while (0)

static DEFINE_MUTEX(block_mutex);

/*
 * The defaults come from config options but can be overriden by module
 * or bootarg options.
 */
static int perdev_minors = CONFIG_MMC_BLOCK_MINORS;

/*
 * We've only got one major, so number of mmcblk devices is
 * limited to 256 / number of minors per device.
 */
static int max_devices;

/* 256 minors, so at most 256 separate devices */
static DECLARE_BITMAP(dev_use, 256);
static DECLARE_BITMAP(name_use, 256);

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;
	struct list_head part;

	unsigned int	flags;
#define MMC_BLK_CMD23	(1 << 0)	/* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR	(1 << 1)	/* MMC Reliable write support */

	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	part_type;
	unsigned int	name_idx;
	unsigned int	reset_done;
#define MMC_BLK_READ		BIT(0)
#define MMC_BLK_WRITE		BIT(1)
#define MMC_BLK_DISCARD		BIT(2)
#define MMC_BLK_SECDISCARD	BIT(3)

	/*
	 * Only set in main mmc_blk_data associated
	 * with mmc_card with mmc_set_drvdata, and keeps
	 * track of the current selected device partition.
	 */
	unsigned int	part_curr;
	struct device_attribute force_ro;
	struct device_attribute power_ro_lock;
	struct device_attribute num_wr_reqs_to_start_packing;
	struct device_attribute bkops_check_threshold;
	struct device_attribute no_pack_for_random;
	int	area_type;
};

static DEFINE_MUTEX(open_lock);

enum {
	MMC_PACKED_N_IDX = -1,
	MMC_PACKED_N_ZERO,
	MMC_PACKED_N_SINGLE,
};

module_param(perdev_minors, int, 0444);
MODULE_PARM_DESC(perdev_minors, "Minors numbers to allocate per device");

static inline void mmc_blk_clear_packed(struct mmc_queue_req *mqrq)
{
	mqrq->packed_cmd = MMC_PACKED_NONE;
	mqrq->packed_num = MMC_PACKED_N_ZERO;
}

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	mutex_lock(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	mutex_unlock(&open_lock);

	return md;
}

static inline int mmc_get_devidx(struct gendisk *disk)
{
	int devidx = disk->first_minor / perdev_minors;
	return devidx;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
	mutex_lock(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		int devidx = mmc_get_devidx(md->disk);
		blk_cleanup_queue(md->queue.queue);

		__clear_bit(devidx, dev_use);

		put_disk(md->disk);
		kfree(md);
	}
	mutex_unlock(&open_lock);
}

static ssize_t power_ro_lock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int locked = 0;

	if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PERM_WP_EN)
		locked = 2;
	else if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_EN)
		locked = 1;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", locked);

	mmc_blk_put(md);

	return ret;
}

static ssize_t power_ro_lock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct mmc_blk_data *md, *part_md;
	struct mmc_card *card;
	unsigned long set;

	if (kstrtoul(buf, 0, &set))
		return -EINVAL;

	if (set != 1)
		return count;

	md = mmc_blk_get(dev_to_disk(dev));
	card = md->queue.card;

	mmc_claim_host(card->host);

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP,
				card->ext_csd.boot_ro_lock |
				EXT_CSD_BOOT_WP_B_PWR_WP_EN,
				card->ext_csd.part_time);
	if (ret)
		pr_err("%s: Locking boot partition ro until next power on failed: %d\n", md->disk->disk_name, ret);
	else
		card->ext_csd.boot_ro_lock |= EXT_CSD_BOOT_WP_B_PWR_WP_EN;

	mmc_release_host(card->host);

	if (!ret) {
		pr_info("%s: Locking boot partition ro until next power on\n",
			md->disk->disk_name);
		set_disk_ro(md->disk, 1);

		list_for_each_entry(part_md, &md->part, part)
			if (part_md->area_type == MMC_BLK_DATA_AREA_BOOT) {
				pr_info("%s: Locking boot partition ro until next power on\n", part_md->disk->disk_name);
				set_disk_ro(part_md->disk, 1);
			}
	}

	mmc_blk_put(md);
	return count;
}

static ssize_t force_ro_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		       get_disk_ro(dev_to_disk(dev)) ^
		       md->read_only);
	mmc_blk_put(md);
	return ret;
}

static ssize_t force_ro_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *end;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	unsigned long set = simple_strtoul(buf, &end, 0);
	if (end == buf) {
		ret = -EINVAL;
		goto out;
	}

	set_disk_ro(dev_to_disk(dev), set || md->read_only);
	ret = count;
out:
	mmc_blk_put(md);
	return ret;
}

static ssize_t
num_wr_reqs_to_start_packing_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int num_wr_reqs_to_start_packing;
	int ret;

	num_wr_reqs_to_start_packing = md->queue.num_wr_reqs_to_start_packing;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", num_wr_reqs_to_start_packing);

	mmc_blk_put(md);
	return ret;
}

static ssize_t
num_wr_reqs_to_start_packing_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int value;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));

	sscanf(buf, "%d", &value);
	if (value >= 0)
		md->queue.num_wr_reqs_to_start_packing = value;

	mmc_blk_put(md);
	return count;
}

static ssize_t
bkops_check_threshold_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int ret;

	if (!card)
		ret = -EINVAL;
	else
	    ret = snprintf(buf, PAGE_SIZE, "%d\n",
		card->bkops_info.size_percentage_to_queue_delayed_work);

	mmc_blk_put(md);
	return ret;
}

static ssize_t
bkops_check_threshold_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int value;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	unsigned int card_size;
	int ret = count;

	if (!card) {
		ret = -EINVAL;
		goto exit;
	}

	sscanf(buf, "%d", &value);
	if ((value <= 0) || (value >= 100)) {
		ret = -EINVAL;
		goto exit;
	}

	card_size = (unsigned int)get_capacity(md->disk);
	if (card_size <= 0) {
		ret = -EINVAL;
		goto exit;
	}
	card->bkops_info.size_percentage_to_queue_delayed_work = value;
	card->bkops_info.min_sectors_to_queue_delayed_work =
		(card_size * value) / 100;

	pr_debug("%s: size_percentage = %d, min_sectors = %d",
			mmc_hostname(card->host),
			card->bkops_info.size_percentage_to_queue_delayed_work,
			card->bkops_info.min_sectors_to_queue_delayed_work);

exit:
	mmc_blk_put(md);
	return count;
}

static ssize_t
no_pack_for_random_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", md->queue.no_pack_for_random);

	mmc_blk_put(md);
	return ret;
}

static ssize_t
no_pack_for_random_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int value;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int ret = count;

	if (!card) {
		ret = -EINVAL;
		goto exit;
	}

	sscanf(buf, "%d", &value);

	if (value < 0) {
		pr_err("%s: value %d is not valid. old value remains = %d",
			mmc_hostname(card->host), value,
			md->queue.no_pack_for_random);
		ret = -EINVAL;
		goto exit;
	}

	md->queue.no_pack_for_random = (value > 0) ?  true : false;

	pr_debug("%s: no_pack_for_random: new value = %d",
		mmc_hostname(card->host),
		md->queue.no_pack_for_random);

exit:
	mmc_blk_put(md);
	return ret;
}

static int mmc_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
	int ret = -ENXIO;

	mutex_lock(&block_mutex);
	if (md) {
		if (md->usage == 2)
			check_disk_change(bdev);
		ret = 0;

		if ((mode & FMODE_WRITE) && md->read_only) {
			mmc_blk_put(md);
			ret = -EROFS;
		}
	}
	mutex_unlock(&block_mutex);

	return ret;
}

static void mmc_blk_release(struct gendisk *disk, fmode_t mode)
{
	struct mmc_blk_data *md = disk->private_data;

	mutex_lock(&block_mutex);
	mmc_blk_put(md);
	mutex_unlock(&block_mutex);
}

static int
mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	geo->heads = 4;
	geo->sectors = 16;
	return 0;
}

struct mmc_blk_ioc_data {
	struct mmc_ioc_cmd ic;
	unsigned char *buf;
	u64 buf_bytes;
};

static struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_user(
	struct mmc_ioc_cmd __user *user)
{
	struct mmc_blk_ioc_data *idata;
	int err;

	idata = kzalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(&idata->ic, user, sizeof(idata->ic))) {
		err = -EFAULT;
		goto idata_err;
	}

	idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;
	if (idata->buf_bytes > MMC_IOC_MAX_BYTES) {
		err = -EOVERFLOW;
		goto idata_err;
	}

	if (!idata->buf_bytes) {
		idata->buf = NULL;
		return idata;
	}

	idata->buf = kzalloc(idata->buf_bytes, GFP_KERNEL);
	if (!idata->buf) {
		err = -ENOMEM;
		goto idata_err;
	}

	if (copy_from_user(idata->buf, (void __user *)(unsigned long)
					idata->ic.data_ptr, idata->buf_bytes)) {
		err = -EFAULT;
		goto copy_err;
	}

	return idata;

copy_err:
	kfree(idata->buf);
idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

struct scatterlist *mmc_blk_get_sg(struct mmc_card *card,
     unsigned char *buf, int *sg_len, int size)
{
	struct scatterlist *sg;
	struct scatterlist *sl;
	int total_sec_cnt, sec_cnt;
	int max_seg_size, len;

	total_sec_cnt = size;
	max_seg_size = card->host->max_seg_size;
	len = (size - 1 + max_seg_size) / max_seg_size;
	sl = kmalloc(sizeof(struct scatterlist) * len, GFP_KERNEL);

	if (!sl) {
		return NULL;
	}
	sg = (struct scatterlist *)sl;
	sg_init_table(sg, len);

	while (total_sec_cnt) {
		if (total_sec_cnt < max_seg_size)
			sec_cnt = total_sec_cnt;
		else
			sec_cnt = max_seg_size;
			sg_set_page(sg, virt_to_page(buf), sec_cnt, offset_in_page(buf));
			buf = buf + sec_cnt;
			total_sec_cnt = total_sec_cnt - sec_cnt;
			if (total_sec_cnt == 0)
				break;
			sg = sg_next(sg);
	}

	if (sg)
		sg_mark_end(sg);
	*sg_len = len;
	return sl;
}

static int mmc_blk_ioctl_copy_to_user(struct mmc_ioc_cmd __user *ic_ptr,
				      struct mmc_blk_ioc_data *idata)
{
	struct mmc_ioc_cmd *ic = &idata->ic;

	if (copy_to_user(&(ic_ptr->response), ic->response,
			 sizeof(ic->response)))
		return -EFAULT;

	if (!idata->ic.write_flag) {
		if (copy_to_user((void __user *)(unsigned long)ic->data_ptr,
				 idata->buf, idata->buf_bytes))
			return -EFAULT;
	}

	return 0;
}

static int __mmc_blk_ioctl_cmd(struct mmc_card *card, struct mmc_blk_data *md,
			       struct mmc_blk_ioc_data *idata)
{
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {NULL};
	struct scatterlist *sg = 0;
	int err = 0;

	cmd.opcode = idata->ic.opcode;
	cmd.arg = idata->ic.arg;
	cmd.flags = idata->ic.flags;

	if (idata->buf_bytes) {
		int len;
		data.blksz = idata->ic.blksz;
		data.blocks = idata->ic.blocks;

		sg = mmc_blk_get_sg(card, idata->buf, &len, idata->buf_bytes);
		data.sg = sg;
		data.sg_len = len;

		if (idata->ic.write_flag)
			data.flags = MMC_DATA_WRITE;
		else
			data.flags = MMC_DATA_READ;

		/* data.flags must already be set before doing this. */
		mmc_set_data_timeout(&data, card);

		/* Allow overriding the timeout_ns for empirical tuning. */
		if (idata->ic.data_timeout_ns)
			data.timeout_ns = idata->ic.data_timeout_ns;

		if ((cmd.flags & MMC_RSP_R1B) == MMC_RSP_R1B) {
			/*
			 * Pretend this is a data transfer and rely on the
			 * host driver to compute timeout.  When all host
			 * drivers support cmd.cmd_timeout for R1B, this
			 * can be changed to:
			 *
			 *     mrq.data = NULL;
			 *     cmd.cmd_timeout = idata->ic.cmd_timeout_ms;
			 */
			data.timeout_ns = idata->ic.cmd_timeout_ms * 1000000;
		}

		mrq.data = &data;
	}

	mrq.cmd = &cmd;

	if (idata->ic.is_acmd) {
		err = mmc_app_cmd(card->host, card);
		if (err)
			return err;
	}

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		dev_err(mmc_dev(card->host), "%s: cmd error %d\n",
						__func__, cmd.error);
		return cmd.error;
	}
	if (data.error) {
		dev_err(mmc_dev(card->host), "%s: data error %d\n",
						__func__, data.error);
		return data.error;
	}

	/*
	 * According to the SD specs, some commands require a delay after
	 * issuing the command.
	 */
	if (idata->ic.postsleep_min_us)
		usleep_range(idata->ic.postsleep_min_us, idata->ic.postsleep_max_us);

	memcpy(&(idata->ic.response), cmd.resp, sizeof(cmd.resp));

	return err;
}

static int mmc_blk_ioctl_cmd(struct block_device *bdev,
			     struct mmc_ioc_cmd __user *ic_ptr)
{
	struct mmc_blk_ioc_data *idata;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	int err = 0, ioc_err = 0;

	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
		return -EPERM;

	idata = mmc_blk_ioctl_copy_from_user(ic_ptr);
	if (IS_ERR(idata))
		return PTR_ERR(idata);

	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		err = -EINVAL;
		goto cmd_err;
	}

	card = md->queue.card;
	if (IS_ERR(card)) {
		err = PTR_ERR(card);
		goto cmd_done;
	}

	mmc_claim_host(card->host);

	ioc_err = __mmc_blk_ioctl_cmd(card, md, idata);

	mmc_release_host(card->host);

	err = mmc_blk_ioctl_copy_to_user(ic_ptr, idata);

cmd_done:
	mmc_blk_put(md);
cmd_err:
	kfree(idata->buf);
	kfree(idata);
	return ioc_err ? ioc_err : err;
}

static int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
#if defined(CONFIG_MMC_CPRM)
	struct mmc_blk_data *md = bdev->bd_disk->private_data;
	struct mmc_card *card = md->queue.card;

	static int i;
	static unsigned long temp_arg[16] = {0};
#endif
	int ret = -EINVAL;
	if (cmd == MMC_IOC_CMD)
		ret = mmc_blk_ioctl_cmd(bdev, (struct mmc_ioc_cmd __user *)arg);
#if defined(CONFIG_MMC_CPRM)
	printk(KERN_DEBUG " %s ], %x ", __func__, cmd);

	switch (cmd) {
	case MMC_IOCTL_SET_RETRY_AKE_PROCESS:
		cprm_ake_retry_flag = 1;
		ret = 0;
		break;

	case MMC_IOCTL_GET_SECTOR_COUNT: {
			int size = 0;

			size = (int)get_capacity(md->disk) << 9;
		printk(KERN_DEBUG "[%s]:MMC_IOCTL_GET_SECTOR_COUNT size = %d\n",
				__func__, size);

			return copy_to_user((void *)arg, &size, sizeof(u64));
		}
		break;
	case ACMD13:
	case ACMD18:
	case ACMD25:
	case ACMD43:
	case ACMD44:
	case ACMD45:
	case ACMD46:
	case ACMD47:
	case ACMD48: {
		struct cprm_request *req = (struct cprm_request *)arg;

		printk(KERN_DEBUG "%s:cmd [%x]\n",
			__func__, cmd);

		if (cmd == ACMD43) {
			printk(KERN_DEBUG"storing acmd43 arg[%d] = %ul\n",
				i, (unsigned int)req->arg);
			temp_arg[i] = req->arg;
			i++;
			if (i >= 16) {
				printk(KERN_DEBUG"reset acmd43 i = %d\n", i);
					i = 0;
			}
		}
			if (cmd == ACMD45 && cprm_ake_retry_flag == 1) {
				cprm_ake_retry_flag = 0;
				printk(KERN_DEBUG"ACMD45.. I'll call ACMD43 and ACMD44 first\n");

				for (i = 0; i < 16; i++) {
					printk(KERN_DEBUG"calling ACMD43 with arg[%d] = %ul\n",
					i, (unsigned int)temp_arg[i]);
				if (stub_sendcmd(card, ACMD43, temp_arg[i],
					512, NULL) < 0) {
					printk(KERN_DEBUG"error ACMD43 %d\n",
						 i);
					return -EINVAL;
				}
			}
			printk(KERN_DEBUG"calling ACMD44\n");
			if (stub_sendcmd(card, ACMD44, 0, 8, NULL) < 0) {

					printk(KERN_DEBUG"error in ACMD44 %d\n",
						i);
					return -EINVAL;
				}

			}
			return stub_sendcmd(card, req->cmd,
				req->arg, req->len, req->buff);
		}
		break;
	default:
		printk(KERN_DEBUG"%s: Invalid ioctl command\n", __func__);
		break;
	}
#endif
	return ret;
}

#ifdef CONFIG_COMPAT
static int mmc_blk_compat_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	return mmc_blk_ioctl(bdev, mode, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static const struct block_device_operations mmc_bdops = {
	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.getgeo			= mmc_blk_getgeo,
	.owner			= THIS_MODULE,
	.ioctl			= mmc_blk_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= mmc_blk_compat_ioctl,
#endif
};

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	int ret;
	struct mmc_blk_data *main_md = mmc_get_drvdata(card);

	if ((main_md->part_curr == md->part_type) &&
	    (card->part_curr == md->part_type))
		return 0;

	if (mmc_card_mmc(card)) {
		u8 part_config = card->ext_csd.part_config;

		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		part_config |= md->part_type;

		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, part_config,
				 card->ext_csd.part_time);
		if (ret)
			return ret;

		card->ext_csd.part_config = part_config;
		card->part_curr = md->part_type;
	}

	main_md->part_curr = md->part_type;
	return 0;
}

static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	__be32 *blocks;

	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};

	struct scatterlist sg;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	mmc_set_data_timeout(&data, card);

	mrq.cmd = &cmd;
	mrq.data = &data;

	blocks = kmalloc(4, GFP_KERNEL);
	if (!blocks)
		return (u32)-1;

	sg_init_one(&sg, blocks, 4);

	mmc_wait_for_req(card->host, &mrq);

	result = ntohl(*blocks);
	kfree(blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static int send_stop(struct mmc_card *card, u32 *status)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 5);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

static int get_card_status(struct mmc_card *card, u32 *status, int retries)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, retries);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

#define ERR_NOMEDIUM	3
#define ERR_RETRY	2
#define ERR_ABORT	1
#define ERR_CONTINUE	0

static int mmc_blk_cmd_error(struct request *req, const char *name, int error,
	bool status_valid, u32 status)
{
	switch (error) {
	case -EILSEQ:
		/* response crc error, retry the r/w cmd */
		pr_err_ratelimited(
			"%s: response CRC error sending %s command, card status %#x\n",
			req->rq_disk->disk_name,
			name, status);
		return ERR_RETRY;

	case -ETIMEDOUT:
		pr_err_ratelimited(
			"%s: timed out sending %s command, card status %#x\n",
			req->rq_disk->disk_name, name, status);

		/* If the status cmd initially failed, retry the r/w cmd */
		if (!status_valid) {
			pr_err_ratelimited("%s: status not valid, retrying timeout\n",
				req->rq_disk->disk_name);
			return ERR_RETRY;
		}
		/*
		 * If it was a r/w cmd crc error, or illegal command
		 * (eg, issued in wrong state) then retry - we should
		 * have corrected the state problem above.
		 */
		if (status & (R1_COM_CRC_ERROR | R1_ILLEGAL_COMMAND)) {
			pr_err_ratelimited(
				"%s: command error, retrying timeout\n",
				req->rq_disk->disk_name);
			return ERR_RETRY;
		}

		/* Otherwise abort the command */
		pr_err_ratelimited(
			"%s: not retrying timeout\n",
			req->rq_disk->disk_name);
		return ERR_ABORT;

	default:
		/* We don't understand the error code the driver gave us */
		pr_err_ratelimited(
			"%s: unknown error %d sending read/write command, card status %#x\n",
		       req->rq_disk->disk_name, error, status);
		return ERR_ABORT;
	}
}

/*
 * Initial r/w and stop cmd error recovery.
 * We don't know whether the card received the r/w cmd or not, so try to
 * restore things back to a sane state.  Essentially, we do this as follows:
 * - Obtain card status.  If the first attempt to obtain card status fails,
 *   the status word will reflect the failed status cmd, not the failed
 *   r/w cmd.  If we fail to obtain card status, it suggests we can no
 *   longer communicate with the card.
 * - Check the card state.  If the card received the cmd but there was a
 *   transient problem with the response, it might still be in a data transfer
 *   mode.  Try to send it a stop command.  If this fails, we can't recover.
 * - If the r/w cmd failed due to a response CRC error, it was probably
 *   transient, so retry the cmd.
 * - If the r/w cmd timed out, but we didn't get the r/w cmd status, retry.
 * - If the r/w cmd timed out, and the r/w cmd failed due to CRC error or
 *   illegal cmd, retry.
 * Otherwise we don't understand what happened, so abort.
 */
static int mmc_blk_cmd_recovery(struct mmc_card *card, struct request *req,
	struct mmc_blk_request *brq, int *ecc_err, int *gen_err)
{
	bool prev_cmd_status_valid = true;
	u32 status, stop_status = 0;
	int err, retry;

	if (mmc_card_removed(card))
		return ERR_NOMEDIUM;

	/*
	 * Try to get card status which indicates both the card state
	 * and why there was no response.  If the first attempt fails,
	 * we can't be sure the returned status is for the r/w command.
	 */
	for (retry = 2; retry >= 0; retry--) {
		err = get_card_status(card, &status, 0);
		if (!err)
			break;

		prev_cmd_status_valid = false;
		pr_err("%s: error %d sending status command, %sing\n",
		       req->rq_disk->disk_name, err, retry ? "retry" : "abort");
	}

	/* We couldn't get a response from the card.  Give up. */
	if (err) {
		/* Check if the card is removed */
		if (mmc_detect_card_removed(card->host))
			return ERR_NOMEDIUM;
		return ERR_ABORT;
	}

	/* Flag ECC errors */
	if ((status & R1_CARD_ECC_FAILED) ||
	    (brq->stop.resp[0] & R1_CARD_ECC_FAILED) ||
	    (brq->cmd.resp[0] & R1_CARD_ECC_FAILED))
		*ecc_err = 1;

	/* Flag General errors */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ)
		if ((status & R1_ERROR) ||
			(brq->stop.resp[0] & R1_ERROR)) {
			pr_err("%s: %s: general error sending stop or status command, stop cmd response %#x, card status %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0], status);
			*gen_err = 1;
		}

	/*
	 * Check the current card state.  If it is in some data transfer
	 * mode, tell it to stop (and hopefully transition back to TRAN.)
	 */
	if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
	    R1_CURRENT_STATE(status) == R1_STATE_RCV) {
		err = send_stop(card, &stop_status);
		if (err)
			pr_err("%s: error %d sending stop command\n",
			       req->rq_disk->disk_name, err);
			/*
			 * If the stop cmd also timed out, the card is probably
			 * not present, so abort. Other errors are bad news too.
			 */
			return ERR_ABORT;
		}

		if (stop_status & R1_CARD_ECC_FAILED)
			*ecc_err = 1;
		if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
			if (stop_status & R1_ERROR) {
				pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
				       req->rq_disk->disk_name, __func__,
				       stop_status);
				*gen_err = 1;
			}
		}

	/* Check for set block count errors */
	if (brq->sbc.error)
		return mmc_blk_cmd_error(req, "SET_BLOCK_COUNT", brq->sbc.error,
				prev_cmd_status_valid, status);

	/* Check for r/w command errors */
	if (brq->cmd.error)
		return mmc_blk_cmd_error(req, "r/w cmd", brq->cmd.error,
				prev_cmd_status_valid, status);

	/* Data errors */
	if (!brq->stop.error)
		return ERR_CONTINUE;

	/* Now for stop errors.  These aren't fatal to the transfer. */
	pr_err("%s: error %d sending stop command, original cmd response %#x, card status %#x\n",
	       req->rq_disk->disk_name, brq->stop.error,
	       brq->cmd.resp[0], status);

	/*
	 * Subsitute in our own stop status as this will give the error
	 * state which happened during the execution of the r/w command.
	 */
	if (stop_status) {
		brq->stop.resp[0] = stop_status;
		brq->stop.error = 0;
	}
	return ERR_CONTINUE;
}

static int mmc_blk_reset(struct mmc_blk_data *md, struct mmc_host *host,
			 int type)
{
	int err;

	if (md->reset_done & type)
		return -EEXIST;

	md->reset_done |= type;
	err = mmc_hw_reset(host);
	if (err && err != -EOPNOTSUPP) {
		/* We failed to reset so we need to abort the request */
		pr_err("%s: %s: failed to reset %d\n", mmc_hostname(host),
				__func__, err);
		return -ENODEV;
	}

	/* Ensure we switch back to the correct partition */
	if (host->card) {
		struct mmc_blk_data *main_md = mmc_get_drvdata(host->card);
		int part_err;

		main_md->part_curr = main_md->part_type;
		part_err = mmc_blk_part_switch(host->card, md);
		if (part_err) {
			/*
			 * We have failed to get back into the correct
			 * partition, so we need to abort the whole request.
			 */
			return -ENODEV;
		}
	}
	return err;
}

static inline void mmc_blk_reset_success(struct mmc_blk_data *md, int type)
{
	md->reset_done &= ~type;
}

static int mmc_blk_issue_discard_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_DISCARD;

	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (card->ext_csd.bkops_en)
		card->bkops_info.sectors_changed += blk_rq_sectors(req);

	if (mmc_can_discard(card))
		arg = MMC_DISCARD_ARG;
	else if (mmc_can_trim(card))
		arg = MMC_TRIM_ARG;
	else
		arg = MMC_ERASE_ARG;
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

#ifdef CONFIG_MMC_SECDISCARD
static int mmc_blk_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_SECDISCARD;

	if (!(mmc_can_secure_erase_trim(card))) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_SECURE_TRIM1_ARG ?
				 INAND_CMD38_ARG_SECTRIM1 :
				 INAND_CMD38_ARG_SECERASE,
				 0);
		if (err)
			goto out_retry;
	}

	err = mmc_erase(card, from, nr, arg);
	if (err == -EIO)
		goto out_retry;
	if (err)
		goto out;

	if (arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 INAND_CMD38_ARG_SECTRIM2,
					 0);
			if (err)
				goto out_retry;
		}

		err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
		if (err == -EIO)
			goto out_retry;
		if (err)
			goto out;
	}

	if (mmc_can_sanitize(card) &&
	     (card->host->caps2 & MMC_CAP2_SANITIZE))
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_SANITIZE_START, 1, 0);
out_retry:
	if (err && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
out:
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}
#endif
static int mmc_blk_issue_sanitize_rq(struct mmc_queue *mq,
				      struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int err = 0;

	BUG_ON(!card);
	BUG_ON(!card->host);

	if (!(mmc_can_sanitize(card) &&
	     (card->host->caps2 & MMC_CAP2_SANITIZE))) {
			pr_warning("%s: %s - SANITIZE is not supported\n",
				   mmc_hostname(card->host), __func__);
			err = -EOPNOTSUPP;
			goto out;
	}

	pr_debug("%s: %s - SANITIZE IN PROGRESS...\n",
		mmc_hostname(card->host), __func__);

	err = mmc_switch_ignore_timeout(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_SANITIZE_START, 1,
					MMC_SANITIZE_REQ_TIMEOUT);

	if (err)
		pr_err("%s: %s - mmc_switch() with "
		       "EXT_CSD_SANITIZE_START failed. err=%d\n",
		       mmc_hostname(card->host), __func__, err);

	pr_debug("%s: %s - SANITIZE COMPLETED\n", mmc_hostname(card->host),
					     __func__);

out:
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

static int mmc_blk_issue_flush(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int ret = 0;

	ret = mmc_flush_cache(card);
	if (ret) {
		pr_err("%s: %s: notify flush error to upper layers",
				req->rq_disk->disk_name, __func__);
		ret = -EIO;
	}

	blk_end_request_all(req, ret);

	return ret ? 0 : 1;
}

/*
 * Reformat current write as a reliable write, supporting
 * both legacy and the enhanced reliable write MMC cards.
 * In each transfer we'll handle only as much as a single
 * reliable write can handle, thus finish the request in
 * partial completions.
 */
static inline void mmc_apply_rel_rw(struct mmc_blk_request *brq,
				    struct mmc_card *card,
				    struct request *req)
{
	if (!(card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN)) {
		/* Legacy mode imposes restrictions on transfers. */
		if (!IS_ALIGNED(brq->cmd.arg, card->ext_csd.rel_sectors))
			brq->data.blocks = 1;

		if (brq->data.blocks > card->ext_csd.rel_sectors)
			brq->data.blocks = card->ext_csd.rel_sectors;
		else if (brq->data.blocks < card->ext_csd.rel_sectors)
			brq->data.blocks = 1;
	}
}

#define CMD_ERRORS							\
	(R1_OUT_OF_RANGE |	/* Command argument out of range */	\
	 R1_ADDRESS_ERROR |	/* Misaligned address */		\
	 R1_BLOCK_LEN_ERROR |	/* Transferred block length incorrect */\
	 R1_WP_VIOLATION |	/* Tried to write to protected block */	\
	 R1_CC_ERROR |		/* Card controller error */		\
	 R1_ERROR)		/* General/unknown error */

static int mmc_blk_err_check(struct mmc_card *card,
			     struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_mrq = container_of(areq, struct mmc_queue_req,
						    mmc_active);
	struct mmc_blk_request *brq = &mq_mrq->brq;
	struct request *req = mq_mrq->req;
	int ecc_err = 0, gen_err = 0;

	/*
	 * sbc.error indicates a problem with the set block count
	 * command.  No data will have been transferred.
	 *
	 * cmd.error indicates a problem with the r/w command.  No
	 * data will have been transferred.
	 *
	 * stop.error indicates a problem with the stop command.  Data
	 * may have been transferred, or may still be transferring.
	 */
	if (brq->sbc.error || brq->cmd.error || brq->stop.error ||
	    brq->data.error) {
		switch (mmc_blk_cmd_recovery(card, req, brq, &ecc_err, &gen_err)) {
		case ERR_RETRY:
			return MMC_BLK_RETRY;
		case ERR_ABORT:
			return MMC_BLK_ABORT;
		case ERR_NOMEDIUM:
			return MMC_BLK_NOMEDIUM;
		case ERR_CONTINUE:
			break;
		}
	}

	/*
	 * Check for errors relating to the execution of the
	 * initial command - such as address errors.  No data
	 * has been transferred.
	 */
	if (brq->cmd.resp[0] & CMD_ERRORS) {
		pr_err("%s: r/w command failed, status = %#x\n",
		       req->rq_disk->disk_name, brq->cmd.resp[0]);
		return MMC_BLK_ABORT;
	}

	/*
	 * Everything else is either success, or a data error of some
	 * kind.  If it was a write, we may have transitioned to
	 * program mode, which we have to wait for it to complete.
	 */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
		u32 status;
		unsigned long timeout;

		/* Check stop command response */
		if (brq->stop.resp[0] & R1_ERROR) {
			pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0]);
			gen_err = 1;
		}

		timeout = jiffies + msecs_to_jiffies(MMC_BLK_TIMEOUT_MS);

		/* Check stop command response */
		if (brq->stop.resp[0] & R1_ERROR) {
			pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0]);
			gen_err = 1;
		}

		do {
			int err = get_card_status(card, &status, 5);
			if (err) {
				pr_err("%s: error %d requesting status\n",
				       req->rq_disk->disk_name, err);
				return MMC_BLK_CMD_ERR;
			}

			/* Timeout if the device never becomes ready for data
			 * and never leaves the program state.
			 */
			if (time_after(jiffies, timeout)) {
				pr_err("%s: Card stuck in programming state!"\
					" %s %s\n", mmc_hostname(card->host),
					req->rq_disk->disk_name, __func__);

				return MMC_BLK_CMD_ERR;
			}

			if (status & R1_ERROR) {
				pr_err("%s: %s: general error sending status command, card status %#x\n",
				       req->rq_disk->disk_name, __func__,
				       status);
				gen_err = 1;
			}

			/*
			 * Some cards mishandle the status bits,
			 * so make sure to check both the busy
			 * indication and the card state.
			 */
		} while (!(status & R1_READY_FOR_DATA) ||
			 (R1_CURRENT_STATE(status) == R1_STATE_PRG));
	}

	/* if general error occurs, retry the write operation. */
	if (gen_err) {
		pr_warn("%s: retrying write for general error\n",
				req->rq_disk->disk_name);
		return MMC_BLK_RETRY;
	}

	if (brq->data.error) {
		pr_err("%s: error %d transferring data, sector %u, nr %u, cmd response %#x, card status %#x\n",
		       req->rq_disk->disk_name, brq->data.error,
		       (unsigned)blk_rq_pos(req),
		       (unsigned)blk_rq_sectors(req),
		       brq->cmd.resp[0], brq->stop.resp[0]);

		if (rq_data_dir(req) == READ) {
			if (ecc_err)
				return MMC_BLK_ECC_ERR;
			return MMC_BLK_DATA_ERR;
		} else {
			return MMC_BLK_CMD_ERR;
		}
	}

	if (!brq->data.bytes_xfered)
		return MMC_BLK_RETRY;

	if (mq_mrq->packed_cmd != MMC_PACKED_NONE) {
		if (unlikely(brq->data.blocks << 9 != brq->data.bytes_xfered))
			return MMC_BLK_PARTIAL;
		else
			return MMC_BLK_SUCCESS;
	}

	if (blk_rq_bytes(req) != brq->data.bytes_xfered)
		return MMC_BLK_PARTIAL;

	return MMC_BLK_SUCCESS;
}

/*
 * mmc_blk_reinsert_req() - re-insert request back to the scheduler
 * @areq:	request to re-insert.
 *
 * Request may be packed or single. When fails to reinsert request, it will be
 * requeued to the the dispatch queue.
 */
static void mmc_blk_reinsert_req(struct mmc_async_req *areq)
{
	struct request *prq;
	int ret = 0;
	struct mmc_queue_req *mq_rq;
	struct request_queue *q;

	mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
	q = mq_rq->req->q;
	if (mq_rq->packed_cmd != MMC_PACKED_NONE) {
		while (!list_empty(&mq_rq->packed_list)) {
			/* return requests in reverse order */
			prq = list_entry_rq(mq_rq->packed_list.prev);
			list_del_init(&prq->queuelist);
			spin_lock_irq(q->queue_lock);
			ret = blk_reinsert_request(q, prq);
			if (ret) {
				blk_requeue_request(q, prq);
				spin_unlock_irq(q->queue_lock);
				goto reinsert_error;
			}
			spin_unlock_irq(q->queue_lock);
		}
	} else {
		spin_lock_irq(q->queue_lock);
		ret = blk_reinsert_request(q, mq_rq->req);
		if (ret)
			blk_requeue_request(q, mq_rq->req);
		spin_unlock_irq(q->queue_lock);
	}
	return;

reinsert_error:
	pr_err("%s: blk_reinsert_request() failed (%d)",
			mq_rq->req->rq_disk->disk_name, ret);
	/*
	 * -EIO will be reported for this request and rest of packed_list.
	 *  Urgent request will be proceeded anyway, while upper layer
	 *  responsibility to re-send failed requests
	 */
	while (!list_empty(&mq_rq->packed_list)) {
		prq = list_entry_rq(mq_rq->packed_list.next);
		list_del_init(&prq->queuelist);
		spin_lock_irq(q->queue_lock);
		blk_requeue_request(q, prq);
		spin_unlock_irq(q->queue_lock);
	}
}

/*
 * mmc_blk_update_interrupted_req() - update of the stopped request
 * @card:	the MMC card associated with the request.
 * @areq:	interrupted async request.
 *
 * Get stopped request state from card and update successfully done part of
 * the request by setting packed_fail_idx.  The packed_fail_idx is index of
 * first uncompleted request in packed request list, for non-packed request
 * packed_fail_idx remains unchanged.
 *
 * Returns: MMC_BLK_SUCCESS for success, MMC_BLK_ABORT otherwise
 */
static int mmc_blk_update_interrupted_req(struct mmc_card *card,
					struct mmc_async_req *areq)
{
	int ret = MMC_BLK_SUCCESS;
	u8 *ext_csd;
	int correctly_done;
	struct mmc_queue_req *mq_rq = container_of(areq, struct mmc_queue_req,
				      mmc_active);
	struct request *prq;
	u8 req_index = 0;

	if (mq_rq->packed_cmd == MMC_PACKED_NONE)
		return MMC_BLK_SUCCESS;

	ext_csd = kmalloc(512, GFP_KERNEL);
	if (!ext_csd)
		return MMC_BLK_ABORT;

	/* get correctly programmed sectors number from card */
	ret = mmc_send_ext_csd(card, ext_csd);
	if (ret) {
		pr_err("%s: error %d reading ext_csd\n",
				mmc_hostname(card->host), ret);
		ret = MMC_BLK_ABORT;
		goto exit;
	}
	correctly_done = card->ext_csd.data_sector_size *
		(ext_csd[EXT_CSD_CORRECTLY_PRG_SECTORS_NUM + 0] << 0 |
		 ext_csd[EXT_CSD_CORRECTLY_PRG_SECTORS_NUM + 1] << 8 |
		 ext_csd[EXT_CSD_CORRECTLY_PRG_SECTORS_NUM + 2] << 16 |
		 ext_csd[EXT_CSD_CORRECTLY_PRG_SECTORS_NUM + 3] << 24);

	list_for_each_entry(prq, &mq_rq->packed_list, queuelist) {
		if ((correctly_done - (int)blk_rq_bytes(prq)) < 0) {
			/* prq is not successfull */
			mq_rq->packed_fail_idx = req_index;
			break;
		}
		correctly_done -= blk_rq_bytes(prq);
		req_index++;
	}
exit:
	kfree(ext_csd);
	return ret;
}

static int mmc_blk_packed_err_check(struct mmc_card *card,
				    struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_rq = container_of(areq, struct mmc_queue_req,
			mmc_active);
	struct request *req = mq_rq->req;
	int err, check, status;
	u8 ext_csd[512];

	mq_rq->packed_retries--;
	check = mmc_blk_err_check(card, areq);
	err = get_card_status(card, &status, 0);
	if (err) {
		pr_err("%s: error %d sending status command\n",
				req->rq_disk->disk_name, err);
		return MMC_BLK_ABORT;
	}

	if (status & R1_EXCEPTION_EVENT) {
		err = mmc_send_ext_csd(card, ext_csd);
		if (err) {
			pr_err("%s: error %d sending ext_csd\n",
					req->rq_disk->disk_name, err);
			return MMC_BLK_ABORT;
		}

		if ((ext_csd[EXT_CSD_EXP_EVENTS_STATUS] &
					EXT_CSD_PACKED_FAILURE) &&
				(ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
				 EXT_CSD_PACKED_GENERIC_ERROR)) {
			if (ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
					EXT_CSD_PACKED_INDEXED_ERROR) {
				mq_rq->packed_fail_idx =
				  ext_csd[EXT_CSD_PACKED_FAILURE_INDEX] - 1;
				check = MMC_BLK_PARTIAL;
			}
			pr_err("packed cmd failed\n");
		}
		kfree(ext_csd);
	}

	return check;
}

static void mmc_blk_rw_rq_prep(struct mmc_queue_req *mqrq,
			       struct mmc_card *card,
			       int disable_multi,
			       struct mmc_queue *mq)
{
	u32 readcmd, writecmd;
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_data_tag;

	/*
	 * Reliable writes are used to implement Forced Unit Access and
	 * REQ_META accesses, and are supported only on MMCs.
	 *
	 * XXX: this really needs a good explanation of why REQ_META
	 * is treated special.
	 */
	bool do_rel_wr = ((req->cmd_flags & REQ_FUA) ||
			  (req->cmd_flags & REQ_META)) &&
		(rq_data_dir(req) == WRITE) &&
		(md->flags & MMC_BLK_REL_WR);

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;

	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	brq->data.blksz = 512;
	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	if (rq_data_dir(req) == WRITE)
		brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	else
		brq->stop.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1B | MMC_CMD_AC;
	brq->data.blocks = blk_rq_sectors(req);

	brq->data.fault_injected = false;
	/*
	 * The block layer doesn't support all sector count
	 * restrictions, so we need to be prepared for too big
	 * requests.
	 */
	if (brq->data.blocks > card->host->max_blk_count)
		brq->data.blocks = card->host->max_blk_count;

	if (brq->data.blocks > 1) {
		/*
		 * After a read error, we redo the request one sector
		 * at a time in order to accurately determine which
		 * sectors can be read successfully.
		 */
		if (disable_multi)
			brq->data.blocks = 1;

		/* Some controllers can't do multiblock reads due to hw bugs */
		if (card->host->caps2 & MMC_CAP2_NO_MULTI_READ &&
		    rq_data_dir(req) == READ)
			brq->data.blocks = 1;
	}

	if (brq->data.blocks > 1 || do_rel_wr) {
		/* SPI multiblock writes terminate using a special
		 * token, not a STOP_TRANSMISSION request.
		 */
		if (!mmc_host_is_spi(card->host) ||
		    rq_data_dir(req) == READ)
			brq->mrq.stop = &brq->stop;
		readcmd = MMC_READ_MULTIPLE_BLOCK;
		writecmd = MMC_WRITE_MULTIPLE_BLOCK;
	} else {
		brq->mrq.stop = NULL;
		readcmd = MMC_READ_SINGLE_BLOCK;
		writecmd = MMC_WRITE_BLOCK;
	}
	if (rq_data_dir(req) == READ) {
		brq->cmd.opcode = readcmd;
		brq->data.flags |= MMC_DATA_READ;
	} else {
		brq->cmd.opcode = writecmd;
		brq->data.flags |= MMC_DATA_WRITE;
	}

	if (do_rel_wr)
		mmc_apply_rel_rw(brq, card, req);

	/*
	 * Data tag is used only during writing meta data to speed
	 * up write and any subsequent read of this meta data
	 */
	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((brq->data.blocks * brq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);

	/*
	 * Pre-defined multi-block transfers are preferable to
	 * open ended-ones (and necessary for reliable writes).
	 * However, it is not sufficient to just send CMD23,
	 * and avoid the final CMD12, as on an error condition
	 * CMD12 (stop) needs to be sent anyway. This, coupled
	 * with Auto-CMD23 enhancements provided by some
	 * hosts, means that the complexity of dealing
	 * with this is best left to the host. If CMD23 is
	 * supported by card and host, we'll fill sbc in and let
	 * the host deal with handling it correctly. This means
	 * that for hosts that don't expose MMC_CAP_CMD23, no
	 * change of behavior will be observed.
	 *
	 * N.B: Some MMC cards experience perf degradation.
	 * We'll avoid using CMD23-bounded multiblock writes for
	 * these, while retaining features like reliable writes.
	 */
	if ((md->flags & MMC_BLK_CMD23) && mmc_op_multi(brq->cmd.opcode) &&
	    (do_rel_wr || !(card->quirks & MMC_QUIRK_BLK_NO_CMD23) ||
	     do_data_tag)) {
		brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
		brq->sbc.arg = brq->data.blocks |
			(do_rel_wr ? (1 << 31) : 0) |
			(do_data_tag ? (1 << 29) : 0);
		brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;
		brq->mrq.sbc = &brq->sbc;
	}

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (brq->data.blocks != blk_rq_sectors(req)) {
		int i, data_size = brq->data.blocks << 9;
		struct scatterlist *sg;

		for_each_sg(brq->data.sg, sg, brq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		brq->data.sg_len = i;
	}

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.cmd_flags = req->cmd_flags;
	mqrq->mmc_active.err_check = mmc_blk_err_check;
	mqrq->mmc_active.reinsert_req = mmc_blk_reinsert_req;
	mqrq->mmc_active.update_interrupted_req =
		mmc_blk_update_interrupted_req;

	mmc_queue_bounce_pre(mqrq);
}

/**
 * mmc_blk_disable_wr_packing() - disables packing mode
 * @mq:	MMC queue.
 *
 */
void mmc_blk_disable_wr_packing(struct mmc_queue *mq)
{
	if (mq) {
		mq->wr_packing_enabled = false;
		mq->num_of_potential_packed_wr_reqs = 0;
	}
}
EXPORT_SYMBOL(mmc_blk_disable_wr_packing);

static void mmc_blk_write_packing_control(struct mmc_queue *mq,
					  struct request *req)
{
	struct mmc_host *host = mq->card->host;
	int data_dir;

	if (!(host->caps2 & MMC_CAP2_PACKED_WR))
		return;

	/* Support for the write packing on eMMC 4.5 or later */
	if (mq->card->ext_csd.rev <= 5)
		return;

	/*
	 * In case the packing control is not supported by the host, it should
	 * not have an effect on the write packing. Therefore we have to enable
	 * the write packing
	 */
	if (!(host->caps2 & MMC_CAP2_PACKED_WR_CONTROL)) {
		mq->wr_packing_enabled = true;
		return;
	}

	if (!req || (req && (req->cmd_flags & REQ_FLUSH))) {
		if (mq->num_of_potential_packed_wr_reqs >
				mq->num_wr_reqs_to_start_packing)
			mq->wr_packing_enabled = true;
		mq->num_of_potential_packed_wr_reqs = 0;
		return;
	}

	data_dir = rq_data_dir(req);

	if (data_dir == READ) {
		mmc_blk_disable_wr_packing(mq);
		return;
	} else if (data_dir == WRITE) {
		mq->num_of_potential_packed_wr_reqs++;
	}

	if (mq->num_of_potential_packed_wr_reqs >
			mq->num_wr_reqs_to_start_packing)
		mq->wr_packing_enabled = true;

}

struct mmc_wr_pack_stats *mmc_blk_get_packed_statistics(struct mmc_card *card)
{
	if (!card)
		return NULL;

	return &card->wr_pack_stats;
}
EXPORT_SYMBOL(mmc_blk_get_packed_statistics);

void mmc_blk_init_packed_statistics(struct mmc_card *card)
{
	int max_num_of_packed_reqs = 0;

	if (!card || !card->wr_pack_stats.packing_events)
		return;

	max_num_of_packed_reqs = card->ext_csd.max_packed_writes;

	spin_lock(&card->wr_pack_stats.lock);
	memset(card->wr_pack_stats.packing_events, 0,
		(max_num_of_packed_reqs + 1) *
	       sizeof(*card->wr_pack_stats.packing_events));
	memset(&card->wr_pack_stats.pack_stop_reason, 0,
		sizeof(card->wr_pack_stats.pack_stop_reason));
	card->wr_pack_stats.enabled = true;
	spin_unlock(&card->wr_pack_stats.lock);
}
EXPORT_SYMBOL(mmc_blk_init_packed_statistics);

static u8 mmc_blk_prep_packed_list(struct mmc_queue *mq, struct request *req)
{
	struct request_queue *q = mq->queue;
	struct mmc_card *card = mq->card;
	struct request *cur = req, *next = NULL;
	struct mmc_blk_data *md = mq->data;
	bool en_rel_wr = card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN;
	unsigned int req_sectors = 0, phys_segments = 0;
	unsigned int max_blk_count, max_phys_segs;
	u8 put_back = 0;
	u8 max_packed_rw = 0;
	u8 reqs = 0;
	struct mmc_wr_pack_stats *stats = &card->wr_pack_stats;

	mmc_blk_clear_packed(mq->mqrq_cur);

	if (!(md->flags & MMC_BLK_CMD23) ||
			!card->ext_csd.packed_event_en)
		goto no_packed;

	if (!mq->wr_packing_enabled)
		goto no_packed;

	if ((rq_data_dir(cur) == WRITE) &&
			(card->host->caps2 & MMC_CAP2_PACKED_WR))
		max_packed_rw = card->ext_csd.max_packed_writes;

	if (max_packed_rw == 0)
		goto no_packed;

	if (mmc_req_rel_wr(cur) &&
			(md->flags & MMC_BLK_REL_WR) &&
			!en_rel_wr)
		goto no_packed;

	if (mmc_large_sec(card) &&
			!IS_ALIGNED(blk_rq_sectors(cur), 8))
		goto no_packed;

	if (cur->cmd_flags & REQ_FUA)
		goto no_packed;

	max_blk_count = min(card->host->max_blk_count,
			card->host->max_req_size >> 9);
	if (unlikely(max_blk_count > 0xffff))
		max_blk_count = 0xffff;

	max_phys_segs = queue_max_segments(q);
	req_sectors += blk_rq_sectors(cur);
	phys_segments += cur->nr_phys_segments;

	if (rq_data_dir(cur) == WRITE) {
		req_sectors++;
		phys_segments++;
	}

	spin_lock(&stats->lock);

	while (reqs < max_packed_rw - 1) {
		/* We should stop no-more packing its nopacked_period */
		if ((card->host->caps2 & MMC_CAP2_ADAPT_PACKED)
				&& time_is_after_jiffies(mq->nopacked_period))
			break;

		spin_lock_irq(q->queue_lock);
		next = blk_fetch_request(q);
		spin_unlock_irq(q->queue_lock);
		if (!next) {
			MMC_BLK_UPDATE_STOP_REASON(stats, EMPTY_QUEUE);
			break;
		}

		if (mmc_large_sec(card) &&
				!IS_ALIGNED(blk_rq_sectors(next), 8)) {
			MMC_BLK_UPDATE_STOP_REASON(stats, LARGE_SEC_ALIGN);
			put_back = 1;
			break;
		}

		if (next->cmd_flags & REQ_DISCARD ||
				next->cmd_flags & REQ_FLUSH) {
			MMC_BLK_UPDATE_STOP_REASON(stats, FLUSH_OR_DISCARD);
			put_back = 1;
			break;
		}

		if (next->cmd_flags & REQ_FUA) {
			MMC_BLK_UPDATE_STOP_REASON(stats, FUA);
			put_back = 1;
			break;
		}

		if (rq_data_dir(cur) != rq_data_dir(next)) {
			MMC_BLK_UPDATE_STOP_REASON(stats, WRONG_DATA_DIR);
			put_back = 1;
			break;
		}

		if (mmc_req_rel_wr(next) &&
				(md->flags & MMC_BLK_REL_WR) &&
				!en_rel_wr) {
			MMC_BLK_UPDATE_STOP_REASON(stats, REL_WRITE);
			put_back = 1;
			break;
		}

		req_sectors += blk_rq_sectors(next);
		if (req_sectors > max_blk_count) {
			if (stats->enabled)
				stats->pack_stop_reason[EXCEEDS_SECTORS]++;
			put_back = 1;
			break;
		}

		phys_segments +=  next->nr_phys_segments;
		if (phys_segments > max_phys_segs) {
			MMC_BLK_UPDATE_STOP_REASON(stats, EXCEEDS_SEGMENTS);
			put_back = 1;
			break;
		}

		if (mq->no_pack_for_random) {
			if ((blk_rq_pos(cur) + blk_rq_sectors(cur)) !=
			    blk_rq_pos(next)) {
				MMC_BLK_UPDATE_STOP_REASON(stats, RANDOM);
				put_back = 1;
				break;
			}
		}

		if (rq_data_dir(next) == WRITE) {
			mq->num_of_potential_packed_wr_reqs++;
			if (card->ext_csd.bkops_en)
				card->bkops_info.sectors_changed +=
					blk_rq_sectors(next);
		}
		list_add_tail(&next->queuelist, &mq->mqrq_cur->packed_list);
		cur = next;
		reqs++;
	}

	if (put_back) {
		spin_lock_irq(q->queue_lock);
		blk_requeue_request(q, next);
		spin_unlock_irq(q->queue_lock);
	}

	if (stats->enabled) {
		if (reqs + 1 <= card->ext_csd.max_packed_writes)
			stats->packing_events[reqs + 1]++;
		if (reqs + 1 == max_packed_rw)
			MMC_BLK_UPDATE_STOP_REASON(stats, THRESHOLD);
	}

	spin_unlock(&stats->lock);

	if (reqs > 0) {
		list_add(&req->queuelist, &mq->mqrq_cur->packed_list);
		mq->mqrq_cur->packed_num = ++reqs;
		mq->mqrq_cur->packed_retries = reqs;
		return reqs;
	}

no_packed:
	mmc_blk_clear_packed(mq->mqrq_cur);
	return 0;
}

static void mmc_blk_packed_hdr_wrq_prep(struct mmc_queue_req *mqrq,
					struct mmc_card *card,
					struct mmc_queue *mq)
{
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct request *prq;
	struct mmc_blk_data *md = mq->data;
	bool do_rel_wr, do_data_tag;
	u32 *packed_cmd_hdr = mqrq->packed_cmd_hdr;
	u8 i = 1;

	mqrq->packed_cmd = MMC_PACKED_WRITE;
	mqrq->packed_blocks = 0;
	mqrq->packed_fail_idx = MMC_PACKED_N_IDX;

	memset(packed_cmd_hdr, 0, sizeof(mqrq->packed_cmd_hdr));
	packed_cmd_hdr[0] = (mqrq->packed_num << 16) |
		(PACKED_CMD_WR << 8) | PACKED_CMD_VER;

	/*
	 * Argument for each entry of packed group
	 */
	list_for_each_entry(prq, &mqrq->packed_list, queuelist) {
		do_rel_wr = mmc_req_rel_wr(prq) && (md->flags & MMC_BLK_REL_WR);
		do_data_tag = (card->ext_csd.data_tag_unit_size) &&
			(prq->cmd_flags & REQ_META) &&
			(rq_data_dir(prq) == WRITE) &&
			((brq->data.blocks * brq->data.blksz) >=
			 card->ext_csd.data_tag_unit_size);
		/* Argument of CMD23 */
		packed_cmd_hdr[(i * 2)] =
			(do_rel_wr ? MMC_CMD23_ARG_REL_WR : 0) |
			(do_data_tag ? MMC_CMD23_ARG_TAG_REQ : 0) |
			blk_rq_sectors(prq);
		/* Argument of CMD18 or CMD25 */
		packed_cmd_hdr[((i * 2)) + 1] =
			mmc_card_blockaddr(card) ?
			blk_rq_pos(prq) : blk_rq_pos(prq) << 9;
		mqrq->packed_blocks += blk_rq_sectors(prq);
		i++;
	}

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;
	brq->mrq.sbc = &brq->sbc;
	brq->mrq.stop = &brq->stop;

	brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
	brq->sbc.arg = MMC_CMD23_ARG_PACKED | (mqrq->packed_blocks + 1);
	brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;

	brq->cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	brq->data.blksz = 512;
	brq->data.blocks = mqrq->packed_blocks + 1;
	brq->data.flags |= MMC_DATA_WRITE;
	brq->data.fault_injected = false;

	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.cmd_flags = req->cmd_flags;

	/*
	 * This is intended for packed commands tests usage - in case these
	 * functions are not in use the respective pointers are NULL
	 */
	if (mq->err_check_fn)
		mqrq->mmc_active.err_check = mq->err_check_fn;
	else
		mqrq->mmc_active.err_check = mmc_blk_packed_err_check;

	if (mq->packed_test_fn)
		mq->packed_test_fn(mq->queue, mqrq);


	mqrq->mmc_active.reinsert_req = mmc_blk_reinsert_req;
	mqrq->mmc_active.update_interrupted_req =
		mmc_blk_update_interrupted_req;

	mmc_queue_bounce_pre(mqrq);
}

static int mmc_blk_cmd_err(struct mmc_blk_data *md, struct mmc_card *card,
			   struct mmc_blk_request *brq, struct request *req,
			   int ret)
{
	struct mmc_queue_req *mq_rq;
	mq_rq = container_of(brq, struct mmc_queue_req, brq);

	/*
	 * If this is an SD card and we're writing, we can first
	 * mark the known good sectors as ok.
	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;
		if (!brq->data.fault_injected) {
			blocks = mmc_sd_num_wr_blocks(card);
			if (blocks != (u32)-1)
				ret = blk_end_request(req, 0, blocks << 9);
		} else
			ret = blk_end_request(req, 0, brq->data.bytes_xfered);
	} else {
		if (mq_rq->packed_cmd == MMC_PACKED_NONE)
			ret = blk_end_request(req, 0, brq->data.bytes_xfered);
	}
	return ret;
}

static int mmc_blk_end_packed_req(struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	int idx = mq_rq->packed_fail_idx, i = 0;
	int ret = 0;

	while (!list_empty(&mq_rq->packed_list)) {
		prq = list_entry_rq(mq_rq->packed_list.next);
		if (idx == i) {
			/* retry from error index */
			mq_rq->packed_num -= idx;
			mq_rq->req = prq;
			ret = 1;

			if (mq_rq->packed_num == MMC_PACKED_N_SINGLE) {
				list_del_init(&prq->queuelist);
				mmc_blk_clear_packed(mq_rq);
			}
			return ret;
		}
		list_del_init(&prq->queuelist);
		blk_end_request(prq, 0, blk_rq_bytes(prq));
		i++;
	}

	mmc_blk_clear_packed(mq_rq);
	return ret;
}
static void mmc_blk_abort_packed_req(struct mmc_queue_req *mq_rq,
					unsigned int cmd_flags)
{
	struct request *prq;

	while (!list_empty(&mq_rq->packed_list)) {
		prq = list_entry_rq(mq_rq->packed_list.next);
		list_del_init(&prq->queuelist);
		prq->cmd_flags |= cmd_flags;
		blk_end_request(prq, -EIO, blk_rq_bytes(prq));
	}

	mmc_blk_clear_packed(mq_rq);
}

static void mmc_blk_revert_packed_req(struct mmc_queue *mq,
				      struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct request_queue *q = mq->queue;

	while (!list_empty(&mq_rq->packed_list)) {
		prq = list_entry_rq(mq_rq->packed_list.prev);
		if (prq->queuelist.prev != &mq_rq->packed_list) {
			list_del_init(&prq->queuelist);
			spin_lock_irq(q->queue_lock);
			blk_requeue_request(mq->queue, prq);
			spin_unlock_irq(q->queue_lock);
		} else {
			list_del_init(&prq->queuelist);
		}
	}

	mmc_blk_clear_packed(mq_rq);
}

static int mmc_blk_issue_rw_rq(struct mmc_queue *mq, struct request *rqc)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_blk_request *brq = &mq->mqrq_cur->brq;
	int ret = 1, disable_multi = 0, retry = 0, type;
	enum mmc_blk_status status;
	struct mmc_queue_req *mq_rq;
	struct request *req;
	struct mmc_async_req *areq;
	const u8 packed_num = 2;
	u8 reqs = 0;

	if (!rqc && !mq->mqrq_prev->req)
		return 0;

	if (rqc) {
		if ((card->ext_csd.bkops_en) && (rq_data_dir(rqc) == WRITE))
			card->bkops_info.sectors_changed += blk_rq_sectors(rqc);
		reqs = mmc_blk_prep_packed_list(mq, rqc);
	}

	do {
		if (rqc) {
			if (reqs >= packed_num)
				mmc_blk_packed_hdr_wrq_prep(mq->mqrq_cur,
						card, mq);
			else
				mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			areq = &mq->mqrq_cur->mmc_active;
		} else
			areq = NULL;
		areq = mmc_start_req(card->host, areq, (int *) &status);
		if (!areq) {
			if (status == MMC_BLK_NEW_REQUEST)
				set_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags);
			return 0;
		}

		mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
		brq = &mq_rq->brq;
		req = mq_rq->req;
		type = rq_data_dir(req) == READ ? MMC_BLK_READ : MMC_BLK_WRITE;
		mmc_queue_bounce_post(mq_rq);

		switch (status) {
		case MMC_BLK_URGENT:
			if (mq_rq->packed_cmd != MMC_PACKED_NONE) {
				/* complete successfully transmitted part */
				if (mmc_blk_end_packed_req(mq_rq))
					/* process for not transmitted part */
					mmc_blk_reinsert_req(areq);
			} else {
				mmc_blk_reinsert_req(areq);
			}

			set_bit(MMC_QUEUE_URGENT_REQUEST, &mq->flags);
			ret = 0;
			break;
		case MMC_BLK_URGENT_DONE:
		case MMC_BLK_SUCCESS:
		case MMC_BLK_PARTIAL:
			/*
			 * A block was successfully transferred.
			 */
			mmc_blk_reset_success(md, type);

			if (mq_rq->packed_cmd != MMC_PACKED_NONE) {
				ret = mmc_blk_end_packed_req(mq_rq);
				break;
			} else {
				ret = blk_end_request(req, 0,
						brq->data.bytes_xfered);
			}

			/*
			 * If the blk_end_request function returns non-zero even
			 * though all data has been transferred and no errors
			 * were returned by the host controller, it's a bug.
			 */
			if (status == MMC_BLK_SUCCESS && ret) {
				pr_err("%s BUG rq_tot %d d_xfer %d\n",
				       __func__, blk_rq_bytes(req),
				       brq->data.bytes_xfered);
				rqc = NULL;
				goto cmd_abort;
			}
			break;
		case MMC_BLK_CMD_ERR:
			ret = mmc_blk_cmd_err(md, card, brq, req, ret);
			if (!mmc_blk_reset(md, card->host, type)) {
				if (!ret) {
					/*
					 * We have successfully completed block
					 * request and notified to upper layers.
					 * As the reset is successful, assume
					 * h/w is in clean state and proceed
					 * with new request.
					 */
					BUG_ON(card->host->areq);
					goto start_new_req;
				}
				break;
			}
			goto cmd_abort;
		case MMC_BLK_RETRY:
			if (retry++ < MMC_BLK_MAX_RETRIES)
				break;
			/* Fall through */
		case MMC_BLK_ABORT:
			if (!mmc_blk_reset(md, card->host, type) &&
					(retry++ < (MMC_BLK_MAX_RETRIES + 1)))
					break;
			goto cmd_abort;
		case MMC_BLK_DATA_ERR: {
			int err;

			err = mmc_blk_reset(md, card->host, type);
			if (!err)
				break;
			if (err == -ENODEV ||
				mq_rq->packed_cmd != MMC_PACKED_NONE)
				goto cmd_abort;
			/* Fall through */
		}
		case MMC_BLK_ECC_ERR:
			if (brq->data.blocks > 1) {
				/* Redo read one sector at a time */
				pr_warning("%s: retrying using single block read\n",
					   req->rq_disk->disk_name);
				disable_multi = 1;
				break;
			}
			/*
			 * After an error, we redo I/O one sector at a
			 * time, so we only reach here after trying to
			 * read a single sector.
			 */
			ret = blk_end_request(req, -EIO,
						brq->data.blksz);
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_NOMEDIUM:
			goto cmd_abort;
		default:
			pr_err("%s: Unhandled return value (%d)",
					req->rq_disk->disk_name, status);
			goto cmd_abort;
		}

		if (ret) {
			if (mq_rq->packed_cmd == MMC_PACKED_NONE) {
				/*
				 * In case of a incomplete request
				 * prepare it again and resend.
				 */
				mmc_blk_rw_rq_prep(mq_rq, card,
						disable_multi, mq);
				mmc_start_req(card->host,
						&mq_rq->mmc_active, NULL);
			} else {
				if (!mq_rq->packed_retries)
					goto cmd_abort;
				mmc_blk_packed_hdr_wrq_prep(mq_rq, card, mq);
				mmc_start_req(card->host,
						&mq_rq->mmc_active, NULL);
			}
		}
	} while (ret);

	return 1;

 cmd_abort:
	if (mq_rq->packed_cmd == MMC_PACKED_NONE) {
		if (mmc_card_removed(card))
			req->cmd_flags |= REQ_QUIET;
		while (ret)
			ret = blk_end_request(req, -EIO,
					blk_rq_cur_bytes(req));
	} else {
		mmc_blk_abort_packed_req(mq_rq, 0);
	}

 start_new_req:
	if (rqc) {
		if (mmc_card_removed(card)) {
			if (mq_rq->packed_cmd == MMC_PACKED_NONE) {
				rqc->cmd_flags |= REQ_QUIET;
				blk_end_request_all(rqc, -EIO);
			} else {
				mmc_blk_abort_packed_req(mq_rq, REQ_QUIET);
			}
		} else {
			/* If current request is packed, it needs to put back */
			if (mq_rq->packed_cmd != MMC_PACKED_NONE)
				mmc_blk_revert_packed_req(mq, mq->mqrq_cur);

			mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			mmc_start_req(card->host,
					&mq->mqrq_cur->mmc_active,
					NULL);
		}
	}

	return 0;
}

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_host *host = card->host;
	unsigned long flags;
	unsigned int cmd_flags = req ? req->cmd_flags : 0;

	if (req && !mq->mqrq_prev->req) {
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
		if (mmc_bus_needs_resume(card->host))
			mmc_resume_bus(card->host);
#endif
		/* claim host only for the first request */
		mmc_claim_host(card->host);
		if (card->ext_csd.bkops_en) {
			ret = mmc_stop_bkops(card);
			if (ret)
				goto out;
		}
	}

	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		if (req) {
			blk_end_request_all(req, -EIO);
		}
		ret = 0;
		goto out;
	}

	mmc_blk_write_packing_control(mq, req);

	clear_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags);
	clear_bit(MMC_QUEUE_URGENT_REQUEST, &mq->flags);
	if (cmd_flags & REQ_DISCARD) {
		/* complete ongoing async transfer before issuing discard */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
#ifdef CONFIG_MMC_SECDISCARD
		if (cmd_flags & REQ_SECURE &&
			!(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
			ret = mmc_blk_issue_secdiscard_rq(mq, req);
		else
#endif
			ret = mmc_blk_issue_discard_rq(mq, req);
	} else if (cmd_flags & REQ_FLUSH) {
		/* complete ongoing async transfer before issuing flush */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		ret = mmc_blk_issue_flush(mq, req);
	} else {
		if (!req && host->areq) {
			spin_lock_irqsave(&host->context_info.lock, flags);
			host->context_info.is_waiting_last_req = true;
			spin_unlock_irqrestore(&host->context_info.lock, flags);
		}
		ret = mmc_blk_issue_rw_rq(mq, req);
	}

out:
	/*
	 * packet burst is over, when one of the following occurs:
	 * - no more requests and new request notification is not in progress
	 * - urgent notification in progress and current request is not urgent
	 *   (all existing requests completed or reinserted to the block layer)
	 */
	if ((!req && !(test_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags))) ||
			((test_bit(MMC_QUEUE_URGENT_REQUEST, &mq->flags)) &&
				!(mq->mqrq_cur->req->cmd_flags & REQ_URGENT))) {
		/*
		 * Release host when there are no more requests
		 * and after special request(discard, flush) is done.
		 * In case sepecial request, there is no reentry to
		 * the 'mmc_blk_issue_rq' with 'mqrq_prev->req'.
		 */
		mmc_release_host(card->host);
	return ret;
}

static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

static struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,
					      struct device *parent,
					      sector_t size,
					      bool default_ro,
					      const char *subname,
					      int area_type)
{
	struct mmc_blk_data *md;
	int devidx, ret;
	unsigned int percentage =
		BKOPS_SIZE_PERCENTAGE_TO_QUEUE_DELAYED_WORK;

	devidx = find_first_zero_bit(dev_use, max_devices);
	if (devidx >= max_devices)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * !subname implies we are creating main mmc_blk_data that will be
	 * associated with mmc_card with mmc_set_drvdata. Due to device
	 * partitions, devidx will not coincide with a per-physical card
	 * index anymore so we keep track of a name index.
	 */
	if (!subname) {
		md->name_idx = find_first_zero_bit(name_use, max_devices);
		__set_bit(md->name_idx, name_use);
	} else
		md->name_idx = ((struct mmc_blk_data *)
				dev_to_disk(parent)->private_data)->name_idx;

	md->area_type = area_type;

	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);

	md->disk = alloc_disk(perdev_minors);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}

	spin_lock_init(&md->lock);
	INIT_LIST_HEAD(&md->part);
	md->usage = 1;

	ret = mmc_init_queue(&md->queue, card, &md->lock, subname);
	if (ret)
		goto err_putdisk;

	md->queue.issue_fn = mmc_blk_issue_rq;
	md->queue.data = md;

	md->disk->major	= MMC_BLOCK_MAJOR;
	md->disk->first_minor = devidx * perdev_minors;
	md->disk->fops = &mmc_bdops;
	md->disk->private_data = md;
	md->disk->queue = md->queue.queue;
	md->disk->driverfs_dev = parent;
	set_disk_ro(md->disk, md->read_only || default_ro);
	if (area_type & (MMC_BLK_DATA_AREA_RPMB | MMC_BLK_DATA_AREA_BOOT))
		md->disk->flags |= GENHD_FL_NO_PART_SCAN;
	md->disk->flags = GENHD_FL_EXT_DEVT;
	if (area_type & MMC_BLK_DATA_AREA_RPMB)
		md->disk->flags |= GENHD_FL_NO_PART_SCAN;

	/*
	 * As discussed on lkml, GENHD_FL_REMOVABLE should:
	 *
	 * - be set for removable media with permanent block devices
	 * - be unset for removable block devices with permanent media
	 *
	 * Since MMC block devices clearly fall under the second
	 * case, we do not set GENHD_FL_REMOVABLE.  Userspace
	 * should use the block device creation/destruction hotplug
	 * messages to tell when the card is present.
	 */

	snprintf(md->disk->disk_name, sizeof(md->disk->disk_name),
		 "mmcblk%d%s", md->name_idx, subname ? subname : "");

	blk_queue_logical_block_size(md->queue.queue, 512);
	set_capacity(md->disk, size);

	card->bkops_info.size_percentage_to_queue_delayed_work = percentage;
	card->bkops_info.min_sectors_to_queue_delayed_work =
		((unsigned int)size * percentage) / 100;

	if (mmc_host_cmd23(card->host)) {
		if (mmc_card_mmc(card) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT &&
		     mmc_sd_card_uhs(card)))
			md->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    md->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		md->flags |= MMC_BLK_REL_WR;
		blk_queue_flush(md->queue.queue, REQ_FLUSH | REQ_FUA);
	}

	return md;

 err_putdisk:
	put_disk(md->disk);
 err_kfree:
	if (!subname)
		__clear_bit(md->name_idx, name_use);
	kfree(md);
 out:
	__clear_bit(devidx, dev_use);
	return ERR_PTR(ret);
}

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	sector_t size;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		//size = card->csd.capacity << (card->csd.read_blkbits - 9);
		size = (typeof(sector_t))card->csd.capacity
			<< (card->csd.read_blkbits - 9);
	}

	return mmc_blk_alloc_req(card, &card->dev, size, false, NULL,
					MMC_BLK_DATA_AREA_MAIN);
}

static int mmc_blk_alloc_part(struct mmc_card *card,
			      struct mmc_blk_data *md,
			      unsigned int part_type,
			      sector_t size,
			      bool default_ro,
			      const char *subname,
			      int area_type)
{
	char cap_str[10];
	struct mmc_blk_data *part_md;

	part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,
				    subname, area_type);
	if (IS_ERR(part_md))
		return PTR_ERR(part_md);
	part_md->part_type = part_type;
	list_add(&part_md->part, &md->part);

	string_get_size((u64)get_capacity(part_md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s partition %u %s\n",
	       part_md->disk->disk_name, mmc_card_id(card),
	       mmc_card_name(card), part_md->part_type, cap_str);
	return 0;
}

/* MMC Physical partitions consist of two boot partitions and
 * up to four general purpose partitions.
 * For each partition enabled in EXT_CSD a block device will be allocatedi
 * to provide access to the partition.
 */

static int mmc_blk_alloc_parts(struct mmc_card *card, struct mmc_blk_data *md)
{
	int idx, ret = 0;

	if (!mmc_card_mmc(card))
		return 0;

	for (idx = 0; idx < card->nr_parts; idx++) {
		if (card->part[idx].size) {
			ret = mmc_blk_alloc_part(card, md,
				card->part[idx].part_cfg,
				card->part[idx].size >> 9,
				card->part[idx].force_ro,
				card->part[idx].name,
				card->part[idx].area_type);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void mmc_blk_remove_req(struct mmc_blk_data *md)
{
	struct mmc_card *card;

	if (md) {
		card = md->queue.card;
		mmc_cleanup_queue(&md->queue);
		device_remove_file(disk_to_dev(md->disk),
				   &md->num_wr_reqs_to_start_packing);
		if (md->disk->flags & GENHD_FL_UP) {
			device_remove_file(disk_to_dev(md->disk), &md->force_ro);
			if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
					card->ext_csd.boot_ro_lockable)
				device_remove_file(disk_to_dev(md->disk),
					&md->power_ro_lock);

			/* Stop new requests from getting into the queue */
			del_gendisk(md->disk);
		}
		mmc_blk_put(md);
	}
}

static void mmc_blk_remove_parts(struct mmc_card *card,
				 struct mmc_blk_data *md)
{
	struct list_head *pos, *q;
	struct mmc_blk_data *part_md;

	__clear_bit(md->name_idx, name_use);
	list_for_each_safe(pos, q, &md->part) {
		part_md = list_entry(pos, struct mmc_blk_data, part);
		list_del(pos);
		mmc_blk_remove_req(part_md);
	}
}

static int mmc_add_disk(struct mmc_blk_data *md)
{
	int ret;
	struct mmc_card *card = md->queue.card;

	add_disk(md->disk);
	md->force_ro.show = force_ro_show;
	md->force_ro.store = force_ro_store;
	sysfs_attr_init(&md->force_ro.attr);
	md->force_ro.attr.name = "force_ro";
	md->force_ro.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk), &md->force_ro);
	if (ret)
		goto force_ro_fail;

	if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
	     card->ext_csd.boot_ro_lockable) {
		umode_t mode;

		if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_DIS)
			mode = S_IRUGO;
		else
			mode = S_IRUGO | S_IWUSR;

		md->power_ro_lock.show = power_ro_lock_show;
		md->power_ro_lock.store = power_ro_lock_store;
		sysfs_attr_init(&md->power_ro_lock.attr);
		md->power_ro_lock.attr.mode = mode;
		md->power_ro_lock.attr.name =
					"ro_lock_until_next_power_on";
		ret = device_create_file(disk_to_dev(md->disk),
				&md->power_ro_lock);
		if (ret)
			goto power_ro_lock_fail;
	}

	md->num_wr_reqs_to_start_packing.show =
		num_wr_reqs_to_start_packing_show;
	md->num_wr_reqs_to_start_packing.store =
		num_wr_reqs_to_start_packing_store;
	sysfs_attr_init(&md->num_wr_reqs_to_start_packing.attr);
	md->num_wr_reqs_to_start_packing.attr.name =
		"num_wr_reqs_to_start_packing";
	md->num_wr_reqs_to_start_packing.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk),
				 &md->num_wr_reqs_to_start_packing);
	if (ret)
		goto num_wr_reqs_to_start_packing_fail;

	md->bkops_check_threshold.show = bkops_check_threshold_show;
	md->bkops_check_threshold.store = bkops_check_threshold_store;
	sysfs_attr_init(&md->bkops_check_threshold.attr);
	md->bkops_check_threshold.attr.name = "bkops_check_threshold";
	md->bkops_check_threshold.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk),
				 &md->bkops_check_threshold);
	if (ret)
		goto bkops_check_threshold_fails;

	md->no_pack_for_random.show = no_pack_for_random_show;
	md->no_pack_for_random.store = no_pack_for_random_store;
	sysfs_attr_init(&md->no_pack_for_random.attr);
	md->no_pack_for_random.attr.name = "no_pack_for_random";
	md->no_pack_for_random.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk),
				 &md->no_pack_for_random);
	if (ret)
		goto no_pack_for_random_fails;

	return ret;

no_pack_for_random_fails:
	device_remove_file(disk_to_dev(md->disk),
			   &md->bkops_check_threshold);
bkops_check_threshold_fails:
	device_remove_file(disk_to_dev(md->disk),
			   &md->num_wr_reqs_to_start_packing);
num_wr_reqs_to_start_packing_fail:
	device_remove_file(disk_to_dev(md->disk), &md->power_ro_lock);
power_ro_lock_fail:
	device_remove_file(disk_to_dev(md->disk), &md->force_ro);
force_ro_fail:
	del_gendisk(md->disk);

	return ret;
}

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_TOSHIBA	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15
#define CID_MANFID_KINGSTON	0x70

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_TOSHIBA	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15

static const struct mmc_fixup blk_fixups[] =
{
	MMC_FIXUP("SEM02G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM04G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM08G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM16G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM32G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),

	/*
	 * Some MMC cards experience performance degradation with CMD23
	 * instead of CMD12-bounded multiblock transfers. For now we'll
	 * black list what's bad...
	 * - Certain Toshiba cards.
	 *
	 * N.B. This doesn't affect SD cards.
	 */
	MMC_FIXUP("SDMB-32", CID_MANFID_SANDISK, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("SDM032", CID_MANFID_SANDISK, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC08G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC16G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC32G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),

	/*
	 * Some MMC cards need longer data read timeout than indicated in CSD.
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_MICRON, 0x200, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),
	MMC_FIXUP("008GE0", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/* Some INAND MCP devices advertise incorrect timeout values */
	MMC_FIXUP("SEM04G", 0x45, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_INAND_DATA_TIMEOUT),

	/*
	 * Some Samsung MMC cards need longer data read timeout than
	 * indicated in CSD.
	 */
	MMC_FIXUP("Q7XSAB", CID_MANFID_SAMSUNG, 0x100, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/*
	 * Hynix eMMC cards need longer data read timeout than
	 * indicated in CSD.
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_HYNIX, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/*
	 * On these Samsung MoviNAND parts, performing secure erase or
	 * secure trim can result in unrecoverable corruption due to a
	 * firmware bug.
	 */
	MMC_FIXUP("M8G2FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MAG4FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MBG8FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MCGAFA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VAL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("KYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VZL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),

	/* Some INAND MCP devices advertise incorrect timeout values */
	MMC_FIXUP("SEM04G", 0x45, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_INAND_DATA_TIMEOUT),

	END_FIXUP
};

#ifdef CONFIG_MMC_SUPPORT_BKOPS_MODE
static ssize_t bkops_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gendisk *disk;
	struct mmc_blk_data *md;
	struct mmc_card *card;

	disk = dev_to_disk(dev);

	if (disk)
		md = disk->private_data;
	else
		goto show_out;

	if (md)
		card = md->queue.card;
	else
		goto show_out;

	return snprintf(buf, PAGE_SIZE, "%u\n", card->bkops_enable);

show_out:
	return snprintf(buf, PAGE_SIZE, "\n");
}

static ssize_t bkops_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct gendisk *disk;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	u8 value;
	int err = 0;

	disk = dev_to_disk(dev);

	if (disk)
		md = disk->private_data;
	else
		goto store_out;

	if (md)
		card = md->queue.card;
	else
		goto store_out;

	if (kstrtou8(buf, 0, &value))
		goto store_out;

	err = mmc_bkops_enable(card->host, value);
	if (err)
		return err;

	return count;
store_out:
	return -EINVAL;
}

static inline void mmc_blk_bkops_sysfs_init(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	card->bkops_attr.show = bkops_mode_show;
	card->bkops_attr.store = bkops_mode_store;
	sysfs_attr_init(&card->bkops_attr.attr);
	card->bkops_attr.attr.name = "bkops_en";
	card->bkops_attr.attr.mode = S_IRUGO | S_IWUSR | S_IWGRP;

	if (device_create_file((disk_to_dev(md->disk)), &card->bkops_attr)) {
		pr_err("%s: Failed to create bkops_en sysfs entry\n",
				mmc_hostname(card->host));
#if defined(CONFIG_MMC_BKOPS_NODE_UID) || defined(CONFIG_MMC_BKOPS_NODE_GID)
        } else {
                int rc;
                struct device * dev;

                dev = disk_to_dev(md->disk);
                rc = sysfs_chown_file(&dev->kobj, &card->bkops_attr.attr,
                                CONFIG_MMC_BKOPS_NODE_UID,
                                CONFIG_MMC_BKOPS_NODE_GID);
                if (rc)
                        pr_err("%s: Failed to change mode of sysfs entry\n",
                                        mmc_hostname(card->host));
#endif
        }
}
#else
static inline void mmc_blk_bkops_sysfs_init(struct mmc_card *card)
{
}
#endif

static int mmc_blk_probe(struct mmc_card *card)
{
	struct mmc_blk_data *md, *part_md;
	char cap_str[10];

	/*
	 * Check that the card supports the command class(es) we need.
	 */
	if (!(card->csd.cmdclass & CCC_BLOCK_READ))
		return -ENODEV;

	mmc_fixup_device(card, blk_fixups);

	md = mmc_blk_alloc(card);
	if (IS_ERR(md))
		return PTR_ERR(md);

	string_get_size((u64)get_capacity(md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s %s %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		cap_str, md->read_only ? "(ro)" : "");

	if (mmc_blk_alloc_parts(card, md))
		goto out;

	mmc_set_drvdata(card, md);

#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	/* applying only MMC TYPE */
	if (card && mmc_card_mmc(card))
		mmc_set_bus_resume_policy(card->host, 1);
#endif
	if (mmc_add_disk(md))
		goto out;

	list_for_each_entry(part_md, &md->part, part) {
		if (mmc_add_disk(part_md))
			goto out;
	}

	/* init sysfs for bkops mode */
	if (card && mmc_card_mmc(card)) {
		mmc_blk_bkops_sysfs_init(card);
		spin_lock_init(&card->bkops_lock);
	}

	return 0;

 out:
	mmc_blk_remove_parts(card, md);
	mmc_blk_remove_req(md);
	return 0;
}

static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	mmc_blk_remove_parts(card, md);
	mmc_claim_host(card->host);
	mmc_blk_part_switch(card, md);
	mmc_release_host(card->host);
	mmc_blk_remove_req(md);
	mmc_set_drvdata(card, NULL);
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	mmc_set_bus_resume_policy(card->host, 0);
#endif
}

#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card, pm_message_t state)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);
	int rc = 0;

	if (md) {
		rc = mmc_queue_suspend(&md->queue, 0);
		if (rc)
			goto out;
		list_for_each_entry(part_md, &md->part, part) {
			rc = mmc_queue_suspend(&part_md->queue, 0);
			if (rc)
				goto out_resume;
		}
	}
	goto out;

 out_resume:
	mmc_queue_resume(&md->queue);
	list_for_each_entry(part_md, &md->part, part) {
		mmc_queue_resume(&part_md->queue);
	}
 out:
	return rc;
}

static int mmc_blk_resume(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		/*
		 * Resume involves the card going into idle state,
		 * so current partition is always the main one.
		 */
		md->part_curr = md->part_type;
		mmc_queue_resume(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_resume(&part_md->queue);
		}
	}
	return 0;
}
#else
#define	mmc_blk_suspend	NULL
#define mmc_blk_resume	NULL
#endif

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "mmcblk",
	},
	.probe		= mmc_blk_probe,
	.remove		= mmc_blk_remove,
	.suspend	= mmc_blk_suspend,
	.resume		= mmc_blk_resume,
};

static int __init mmc_blk_init(void)
{
	int res;

	if (perdev_minors != CONFIG_MMC_BLOCK_MINORS)
		pr_info("mmcblk: using %d minors per device\n", perdev_minors);

	max_devices = 256 / perdev_minors;

	res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
	if (res)
		goto out;

	res = mmc_register_driver(&mmc_driver);
	if (res)
		goto out2;

	return 0;
 out2:
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
 out:
	return res;
}

static void __exit mmc_blk_exit(void)
{
	mmc_unregister_driver(&mmc_driver);
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
}

module_init(mmc_blk_init);
module_exit(mmc_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");

