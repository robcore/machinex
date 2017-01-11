/*
 * Debugfs support for hosts and cards
 *
 * Copyright (C) 2008 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/moduleparam.h>
#include <linux/export.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/fault-inject.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include "core.h"
#include "mmc_ops.h"

#ifdef CONFIG_FAIL_MMC_REQUEST

static DECLARE_FAULT_ATTR(fail_default_attr);
static char *fail_request;
module_param(fail_request, charp, 0);

#endif /* CONFIG_FAIL_MMC_REQUEST */

/* The debugfs functions are optimized away when CONFIG_DEBUG_FS isn't set. */
static int mmc_ios_show(struct seq_file *s, void *data)
{
	static const char *vdd_str[] = {
		[8]	= "2.0",
		[9]	= "2.1",
		[10]	= "2.2",
		[11]	= "2.3",
		[12]	= "2.4",
		[13]	= "2.5",
		[14]	= "2.6",
		[15]	= "2.7",
		[16]	= "2.8",
		[17]	= "2.9",
		[18]	= "3.0",
		[19]	= "3.1",
		[20]	= "3.2",
		[21]	= "3.3",
		[22]	= "3.4",
		[23]	= "3.5",
		[24]	= "3.6",
	};
	struct mmc_host	*host = s->private;
	struct mmc_ios	*ios = &host->ios;
	const char *str;

	seq_printf(s, "clock:\t\t%u Hz\n", ios->clock);
	if (host->actual_clock)
		seq_printf(s, "actual clock:\t%u Hz\n", host->actual_clock);
	seq_printf(s, "vdd:\t\t%u ", ios->vdd);
	if ((1 << ios->vdd) & MMC_VDD_165_195)
		seq_printf(s, "(1.65 - 1.95 V)\n");
	else if (ios->vdd < (ARRAY_SIZE(vdd_str) - 1)
			&& vdd_str[ios->vdd] && vdd_str[ios->vdd + 1])
		seq_printf(s, "(%s ~ %s V)\n", vdd_str[ios->vdd],
				vdd_str[ios->vdd + 1]);
	else
		seq_printf(s, "(invalid)\n");

	switch (ios->bus_mode) {
	case MMC_BUSMODE_OPENDRAIN:
		str = "open drain";
		break;
	case MMC_BUSMODE_PUSHPULL:
		str = "push-pull";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "bus mode:\t%u (%s)\n", ios->bus_mode, str);

	switch (ios->chip_select) {
	case MMC_CS_DONTCARE:
		str = "don't care";
		break;
	case MMC_CS_HIGH:
		str = "active high";
		break;
	case MMC_CS_LOW:
		str = "active low";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "chip select:\t%u (%s)\n", ios->chip_select, str);

	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		str = "off";
		break;
	case MMC_POWER_UP:
		str = "up";
		break;
	case MMC_POWER_ON:
		str = "on";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "power mode:\t%u (%s)\n", ios->power_mode, str);
	seq_printf(s, "bus width:\t%u (%u bits)\n",
			ios->bus_width, 1 << ios->bus_width);

	switch (ios->timing) {
	case MMC_TIMING_LEGACY:
		str = "legacy";
		break;
	case MMC_TIMING_MMC_HS:
		str = "mmc high-speed";
		break;
	case MMC_TIMING_SD_HS:
		str = "sd high-speed";
		break;
	case MMC_TIMING_UHS_SDR50:
		str = "sd uhs SDR50";
		break;
	case MMC_TIMING_UHS_SDR104:
		str = "sd uhs SDR104";
		break;
	case MMC_TIMING_UHS_DDR50:
		str = "sd uhs DDR50";
		break;
	case MMC_TIMING_MMC_HS200:
		str = "mmc high-speed SDR200";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "timing spec:\t%u (%s)\n", ios->timing, str);

	return 0;
}

static int mmc_ios_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmc_ios_show, inode->i_private);
}

static const struct file_operations mmc_ios_fops = {
	.open		= mmc_ios_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int mmc_clock_opt_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = host->ios.clock;

	return 0;
}

static int mmc_clock_opt_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	/* We need this check due to input value is u64 */
	if (val > host->f_max)
		return -EINVAL;

	mmc_claim_host(host);
	mmc_set_clock(host, (unsigned int) val);
	mmc_release_host(host);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mmc_clock_fops, mmc_clock_opt_get, mmc_clock_opt_set,
	"%llu\n");

#ifdef CONFIG_MMC_CMD_LOG
/*
 * Add a command and argument to the ring buffer.  The host must be claimed.
 */
void mmc_cmd_log(struct mmc_host *host, u32 cmd, u32 arg)
{
	int i = host->mmc_cmd_log_idx;

	if (!(host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_EN) ||
			!host->mmc_cmd_log)
		return;

	host->mmc_cmd_log[i++] = cmd;
	host->mmc_cmd_log[i++] = arg;
	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME)
		host->mmc_cmd_log[i++] = (u32)sched_clock();

	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_RESP)
		host->mmc_cmd_log[i++] = UINT_MAX;  /* mark as in-progress */
}

/*
 * Add a command response to the ring buffer.  The host must be claimed.
 */
void mmc_cmd_log_resp(struct mmc_host *host, u32 resp)
{
	int i = host->mmc_cmd_log_idx + MMC_CMD_LOG_RECSIZE_BASE;

	if (!(host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_EN) ||
			!host->mmc_cmd_log)
		return;

	/* If we are measuring deltas, replace the start time with it. */
	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME) {
		if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_DELTA) {
			u32 now = (u32)sched_clock();
			u32 then = host->mmc_cmd_log[i];

			if (now < then)
				then = (UINT_MAX - then) + now;
			else
				then = now - then;
			host->mmc_cmd_log[i] = then;
		}
		i++;
	}

	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_RESP)
		host->mmc_cmd_log[i++] = resp;

	host->mmc_cmd_log_idx = i;
	if (host->mmc_cmd_log_idx >= host->mmc_cmd_log_len)
		host->mmc_cmd_log_idx = 0;
}

static int _mmc_cmd_log_dump(struct mmc_host *host, struct seq_file *s)
{
	int i;

	if (!host->mmc_cmd_log)
		return 0;

	mmc_claim_host(host);

	i = host->mmc_cmd_log_idx;  /* next slot should be the oldest */
	do {
		u32 cmd = host->mmc_cmd_log[i++];
		u32 arg = host->mmc_cmd_log[i++];
		u32 resp = 0;
		u32 when = 0;

		if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME)
			when = host->mmc_cmd_log[i++];
		if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_RESP)
			resp = host->mmc_cmd_log[i++];

		if (i >= host->mmc_cmd_log_len)
			i = 0;

		/* Skip empty or partial records */
		if (cmd == UINT_MAX || resp == UINT_MAX)
			continue;

		if ((host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME) &&
		    !(host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_DELTA)) {
			if (s)
				seq_printf(s, "[%u] ", when);
			else
				pr_info("[%u] ", when);
		}
		if (s)
			seq_printf(s, "CMD%d: 0x%08X", cmd & 0x3F, arg);
		else
			pr_info("CMD%d: 0x%08X", cmd & 0x3F, arg);
		if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_RESP) {
			if (s)
				seq_printf(s, " R:0x%08X", resp);
			else
				pr_info(" R:0x%08X", resp);
		}
		if ((host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME) &&
		    (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_DELTA)) {
			if (s)
				seq_printf(s, " %uns", when);
			else
				pr_info(" %uns", when);
		}
		if (s)
			seq_printf(s, "\n");
		else
			pr_info("\n");
	} while (i != host->mmc_cmd_log_idx);

	mmc_release_host(host);

	return 0;
}

/*
 * Dump the command log to the kernel log.  This is useful to call from the
 * host driver while debugging controller or card problems.
 */
void mmc_cmd_log_dump(struct mmc_host *host)
{
	pr_err("%s: Command history (oldest first):\n",
		mmc_hostname(host));

	_mmc_cmd_log_dump(host, NULL);
}

static int mmc_cmd_log_show(struct seq_file *s, void *v)
{
	struct mmc_host *host = (struct mmc_host *)s->private;

	return _mmc_cmd_log_dump(host, s);
}

static int mmc_cmd_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmc_cmd_log_show, inode->i_private);
}

static const struct file_operations mmc_cmd_log_fops = {
	.owner		= THIS_MODULE,
	.open		= mmc_cmd_log_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int mmc_cmd_log_mode_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = (u64)host->mmc_cmd_log_mode;

	return 0;
}

static int mmc_cmd_log_mode_set(void *data, u64 val)
{
	struct mmc_host *host = data;
	int record_size = MMC_CMD_LOG_RECSIZE_BASE;

	if (val & MMC_CMD_LOG_MODE_FORCE_DUMP) {
		mmc_cmd_log_dump(host);
		return 0;
	}

	mmc_claim_host(host);

	host->mmc_cmd_log_mode = (int)val;

	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_RESP)
		record_size += MMC_CMD_LOG_RECSIZE_RESP;
	if (host->mmc_cmd_log_mode & MMC_CMD_LOG_MODE_TIME)
		record_size += MMC_CMD_LOG_RECSIZE_TIME;

	/* Free the ring buffer if the record size changes */
	if (host->mmc_cmd_log && record_size != host->mmc_cmd_log_recsize) {
		kfree(host->mmc_cmd_log);
		host->mmc_cmd_log = NULL;
		host->mmc_cmd_log_len = 0;
		host->mmc_cmd_log_idx = 0;
	}

	host->mmc_cmd_log_recsize = record_size;

	mmc_release_host(host);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mmc_cmd_log_mode_fops, mmc_cmd_log_mode_get,
			mmc_cmd_log_mode_set, "%llu\n");

static int mmc_cmd_log_size_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = (u64)(host->mmc_cmd_log_len / host->mmc_cmd_log_recsize);

	return 0;
}

static int mmc_cmd_log_size_set(void *data, u64 val)
{
	struct mmc_host *host = data;
	u32 size = val;

	mmc_claim_host(host);

	kfree(host->mmc_cmd_log);
	host->mmc_cmd_log = NULL;
	host->mmc_cmd_log_len = 0;
	host->mmc_cmd_log_idx = 0;

	size *= host->mmc_cmd_log_recsize;	/* slots per command */
	if (size > 0) {
		host->mmc_cmd_log = kmalloc(size * sizeof(*host->mmc_cmd_log),
				GFP_KERNEL);
	}
	if (host->mmc_cmd_log) {
		memset(host->mmc_cmd_log, 0xFF,
				size * sizeof(*host->mmc_cmd_log));
		host->mmc_cmd_log_len = size;
	}

	mmc_release_host(host);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mmc_cmd_log_size_fops, mmc_cmd_log_size_get,
			mmc_cmd_log_size_set, "%llu\n");
#endif /* CONFIG_MMC_CMDLOG /*/

void mmc_add_host_debugfs(struct mmc_host *host)
{
	struct dentry *root;

	root = debugfs_create_dir(mmc_hostname(host), NULL);
	if (IS_ERR(root))
		/* Don't complain -- debugfs just isn't enabled */
		return;
	if (!root)
		/* Complain -- debugfs is enabled, but it failed to
		 * create the directory. */
		goto err_root;

	host->debugfs_root = root;

	if (!debugfs_create_file("ios", S_IRUSR, root, host, &mmc_ios_fops))
		goto err_node;

	if (!debugfs_create_file("clock", S_IRUSR | S_IWUSR, root, host,
			&mmc_clock_fops))
		goto err_node;

#ifdef CONFIG_MMC_CLKGATE
	if (!debugfs_create_u32("clk_delay", (S_IRUSR | S_IWUSR),
				root, &host->clk_delay))
		goto err_node;
#endif
#ifdef CONFIG_FAIL_MMC_REQUEST
	if (fail_request)
		setup_fault_attr(&fail_default_attr, fail_request);
	host->fail_mmc_request = fail_default_attr;
	if (IS_ERR(fault_create_debugfs_attr("fail_mmc_request",
					     root,
					     &host->fail_mmc_request)))
		goto err_node;
#endif
#ifdef CONFIG_MMC_CMD_LOG
	if (!debugfs_create_file("cmd_log", S_IRUSR, root, host,
			&mmc_cmd_log_fops))
		goto err_node;
	if (!debugfs_create_file("cmd_log_mode", S_IRUSR | S_IWUSR, root, host,
			&mmc_cmd_log_mode_fops))
		goto err_node;
	if (!debugfs_create_file("cmd_log_size", S_IRUSR | S_IWUSR, root, host,
			&mmc_cmd_log_size_fops))
		goto err_node;
#endif
	return;

err_node:
	debugfs_remove_recursive(root);
	host->debugfs_root = NULL;
err_root:
	dev_err(&host->class_dev, "failed to initialize debugfs\n");
}

void mmc_remove_host_debugfs(struct mmc_host *host)
{
	debugfs_remove_recursive(host->debugfs_root);
}

static int mmc_dbg_card_status_get(void *data, u64 *val)
{
	struct mmc_card	*card = data;
	u32		status;
	int		ret;

	mmc_claim_host(card->host);

	ret = mmc_send_status(data, &status);
	if (!ret)
		*val = status;

	mmc_release_host(card->host);

	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(mmc_dbg_card_status_fops, mmc_dbg_card_status_get,
		NULL, "%08llx\n");

#define EXT_CSD_STR_LEN 1025

static int mmc_ext_csd_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;
	char *buf;
	ssize_t n = 0;
	u8 *ext_csd;
	int err, i;

	buf = kmalloc(EXT_CSD_STR_LEN + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ext_csd = kmalloc(512, GFP_KERNEL);
	if (!ext_csd) {
		err = -ENOMEM;
		goto out_free;
	}

	mmc_claim_host(card->host);
	err = mmc_send_ext_csd(card, ext_csd);
	mmc_release_host(card->host);
	if (err)
		goto out_free;

	for (i = 0; i < 512; i++)
		n += sprintf(buf + n, "%02x", ext_csd[i]);
	n += sprintf(buf + n, "\n");
	BUG_ON(n != EXT_CSD_STR_LEN);

	filp->private_data = buf;
	kfree(ext_csd);
	return 0;

out_free:
	kfree(buf);
	kfree(ext_csd);
	return err;
}

static ssize_t mmc_ext_csd_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char *buf = filp->private_data;

	return simple_read_from_buffer(ubuf, cnt, ppos,
				       buf, EXT_CSD_STR_LEN);
}

static int mmc_ext_csd_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static const struct file_operations mmc_dbg_ext_csd_fops = {
	.open		= mmc_ext_csd_open,
	.read		= mmc_ext_csd_read,
	.release	= mmc_ext_csd_release,
	.llseek		= default_llseek,
};

static int mmc_wr_pack_stats_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;

	filp->private_data = card;
	card->wr_pack_stats.print_in_read = 1;
	return 0;
}

#define TEMP_BUF_SIZE 256
static ssize_t mmc_wr_pack_stats_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	struct mmc_card *card = filp->private_data;
	struct mmc_wr_pack_stats *pack_stats;
	int i;
	int max_num_of_packed_reqs = 0;
	char *temp_buf;

	if (!card)
		return cnt;

	if (!card->wr_pack_stats.print_in_read)
		return 0;

	if (!card->wr_pack_stats.enabled) {
		pr_info("%s: write packing statistics are disabled\n",
			 mmc_hostname(card->host));
		goto exit;
	}

	pack_stats = &card->wr_pack_stats;

	if (!pack_stats->packing_events) {
		pr_info("%s: NULL packing_events\n", mmc_hostname(card->host));
		goto exit;
	}

	max_num_of_packed_reqs = card->ext_csd.max_packed_writes;

	temp_buf = kmalloc(TEMP_BUF_SIZE, GFP_KERNEL);
	if (!temp_buf)
		goto exit;

	spin_lock(&pack_stats->lock);

	snprintf(temp_buf, TEMP_BUF_SIZE, "%s: write packing statistics:\n",
		mmc_hostname(card->host));
	strlcat(ubuf, temp_buf, cnt);

	for (i = 1 ; i <= max_num_of_packed_reqs ; ++i) {
		if (pack_stats->packing_events[i]) {
			snprintf(temp_buf, TEMP_BUF_SIZE,
				 "%s: Packed %d reqs - %d times\n",
				mmc_hostname(card->host), i,
				pack_stats->packing_events[i]);
			strlcat(ubuf, temp_buf, cnt);
		}
	}

	snprintf(temp_buf, TEMP_BUF_SIZE,
		 "%s: stopped packing due to the following reasons:\n",
		 mmc_hostname(card->host));
	strlcat(ubuf, temp_buf, cnt);

	if (pack_stats->pack_stop_reason[EXCEEDS_SEGMENTS]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: exceed max num of segments\n",
			 mmc_hostname(card->host),
			 pack_stats->pack_stop_reason[EXCEEDS_SEGMENTS]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[EXCEEDS_SECTORS]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: exceed max num of sectors\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[EXCEEDS_SECTORS]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[WRONG_DATA_DIR]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: wrong data direction\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[WRONG_DATA_DIR]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[FLUSH_OR_DISCARD]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: flush or discard\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[FLUSH_OR_DISCARD]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[EMPTY_QUEUE]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: empty queue\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[EMPTY_QUEUE]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[REL_WRITE]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: rel write\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[REL_WRITE]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[THRESHOLD]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: Threshold\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[THRESHOLD]);
		strlcat(ubuf, temp_buf, cnt);
	}

	if (pack_stats->pack_stop_reason[LARGE_SEC_ALIGN]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: Large sector alignment\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[LARGE_SEC_ALIGN]);
		strlcat(ubuf, temp_buf, cnt);
	}
	if (pack_stats->pack_stop_reason[RANDOM]) {
		snprintf(temp_buf, TEMP_BUF_SIZE,
			 "%s: %d times: random request\n",
			mmc_hostname(card->host),
			pack_stats->pack_stop_reason[RANDOM]);
		strlcat(ubuf, temp_buf, cnt);
	}

	spin_unlock(&pack_stats->lock);

	kfree(temp_buf);

	pr_info("%s", ubuf);

exit:
	if (card->wr_pack_stats.print_in_read == 1) {
		card->wr_pack_stats.print_in_read = 0;
		return strnlen(ubuf, cnt);
	}

	return 0;
}

static ssize_t mmc_wr_pack_stats_write(struct file *filp,
				       const char __user *ubuf, size_t cnt,
				       loff_t *ppos)
{
	struct mmc_card *card = filp->private_data;
	int value;

	if (!card)
		return cnt;

	sscanf(ubuf, "%d", &value);
	if (value) {
		mmc_blk_init_packed_statistics(card);
	} else {
		spin_lock(&card->wr_pack_stats.lock);
		card->wr_pack_stats.enabled = false;
		spin_unlock(&card->wr_pack_stats.lock);
	}

	return cnt;
}

static const struct file_operations mmc_dbg_wr_pack_stats_fops = {
	.open		= mmc_wr_pack_stats_open,
	.read		= mmc_wr_pack_stats_read,
	.write		= mmc_wr_pack_stats_write,
};

void mmc_add_card_debugfs(struct mmc_card *card)
{
	struct mmc_host	*host = card->host;
	struct dentry	*root;

	if (!host->debugfs_root)
		return;

	root = debugfs_create_dir(mmc_card_id(card), host->debugfs_root);
	if (IS_ERR(root))
		/* Don't complain -- debugfs just isn't enabled */
		return;
	if (!root)
		/* Complain -- debugfs is enabled, but it failed to
		 * create the directory. */
		goto err;

	card->debugfs_root = root;

	if (!debugfs_create_x32("state", S_IRUSR, root, &card->state))
		goto err;

	if (mmc_card_mmc(card) || mmc_card_sd(card))
		if (!debugfs_create_file("status", S_IRUSR, root, card,
					&mmc_dbg_card_status_fops))
			goto err;

	if (mmc_card_mmc(card))
		if (!debugfs_create_file("ext_csd", S_IRUSR, root, card,
					&mmc_dbg_ext_csd_fops))
			goto err;

	if (mmc_card_mmc(card) && (card->ext_csd.rev >= 6) &&
	    (card->host->caps2 & MMC_CAP2_PACKED_WR))
		if (!debugfs_create_file("wr_pack_stats", S_IRUSR, root, card,
					 &mmc_dbg_wr_pack_stats_fops))
			goto err;

	return;

err:
	debugfs_remove_recursive(root);
	card->debugfs_root = NULL;
	dev_err(&card->dev, "failed to initialize debugfs\n");
}

void mmc_remove_card_debugfs(struct mmc_card *card)
{
	debugfs_remove_recursive(card->debugfs_root);
}
