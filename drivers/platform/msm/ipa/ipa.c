/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/fs.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rbtree.h>
#include <linux/uaccess.h>
#include "ipa_i.h"

#define IPA_SUMMING_THRESHOLD (0x10)
#define IPA_PIPE_MEM_START_OFST (0x0)
#define IPA_PIPE_MEM_SIZE (0x0)
#define IPA_READ_MAX (16)
#define IPA_MOBILE_AP_MODE(x) (x == IPA_MODE_MOBILE_AP_ETH || \
			       x == IPA_MODE_MOBILE_AP_WAN || \
			       x == IPA_MODE_MOBILE_AP_WLAN)
#define IPA_CNOC_CLK_RATE (75 * 1000 * 1000UL)
#define IPA_V1_CLK_RATE (92.31 * 1000 * 1000UL)
#define IPA_DMA_POOL_SIZE (512)
#define IPA_DMA_POOL_ALIGNMENT (4)
#define IPA_DMA_POOL_BOUNDARY (1024)
#define WLAN_AMPDU_TX_EP (15)
#define IPA_ROUTING_RULE_BYTE_SIZE (4)
#define IPA_BAM_CNFG_BITS_VAL (0x7FFFE004)

#define IPA_AGGR_MAX_STR_LENGTH (10)

#define IPA_AGGR_STR_IN_BYTES(str) \
	(strnlen((str), IPA_AGGR_MAX_STR_LENGTH - 1) + 1)

struct ipa_plat_drv_res {
	u32 ipa_mem_base;
	u32 ipa_mem_size;
	u32 bam_mem_base;
	u32 bam_mem_size;
	u32 ipa_irq;
	u32 bam_irq;
	u32 ipa_pipe_mem_start_ofst;
	u32 ipa_pipe_mem_size;
	struct a2_mux_pipe_connection a2_to_ipa_pipe;
	struct a2_mux_pipe_connection ipa_to_a2_pipe;
};

static struct ipa_plat_drv_res ipa_res = {0, };
static struct of_device_id ipa_plat_drv_match[] = {
	{
		.compatible = "qcom,ipa",
	},

	{
	}
};

static struct clk *ipa_clk_src;
static struct clk *ipa_clk;
static struct clk *sys_noc_ipa_axi_clk;
static struct clk *ipa_cnoc_clk;
static struct clk *ipa_inactivity_clk;
static struct device *ipa_dev;

struct ipa_context *ipa_ctx;

static bool polling_mode;
module_param(polling_mode, bool, 0644);
MODULE_PARM_DESC(polling_mode,
		"1 - pure polling mode; 0 - interrupt+polling mode");
static uint polling_delay_ms = 50;
module_param(polling_delay_ms, uint, 0644);
MODULE_PARM_DESC(polling_delay_ms, "set to desired delay between polls");
static bool hdr_tbl_lcl = 1;
module_param(hdr_tbl_lcl, bool, 0644);
MODULE_PARM_DESC(hdr_tbl_lcl, "where hdr tbl resides 1-local; 0-system");
static bool ip4_rt_tbl_lcl = 1;
module_param(ip4_rt_tbl_lcl, bool, 0644);
MODULE_PARM_DESC(ip4_rt_tbl_lcl,
		"where ip4 rt tables reside 1-local; 0-system");
static bool ip6_rt_tbl_lcl = 1;
module_param(ip6_rt_tbl_lcl, bool, 0644);
MODULE_PARM_DESC(ip6_rt_tbl_lcl,
		"where ip6 rt tables reside 1-local; 0-system");
static bool ip4_flt_tbl_lcl = 1;
module_param(ip4_flt_tbl_lcl, bool, 0644);
MODULE_PARM_DESC(ip4_flt_tbl_lcl,
		"where ip4 flt tables reside 1-local; 0-system");
static bool ip6_flt_tbl_lcl = 1;
module_param(ip6_flt_tbl_lcl, bool, 0644);
MODULE_PARM_DESC(ip6_flt_tbl_lcl,
		"where ip6 flt tables reside 1-local; 0-system");

static int ipa_load_pipe_connection(struct platform_device *pdev,
				    enum a2_mux_pipe_direction pipe_dir,
				    struct a2_mux_pipe_connection     *pdata);

static int ipa_update_connections_info(struct device_node *node,
			struct a2_mux_pipe_connection *pipe_connection);

static void ipa_set_aggregation_params(void);

static ssize_t ipa_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	u32 reg_val = 0xfeedface;
	char str[IPA_READ_MAX];
	int result;
	static int read_cnt;

	if (read_cnt) {
		IPAERR("only supports one call to read\n");
		return 0;
	}

	reg_val = ipa_read_reg(ipa_ctx->mmio, IPA_COMP_HW_VERSION_OFST);
	result = scnprintf(str, IPA_READ_MAX, "%x\n", reg_val);
	if (copy_to_user(buf, str, result))
		return -EFAULT;
	read_cnt = 1;

	return result;
}

static int ipa_open(struct inode *inode, struct file *filp)
{
	struct ipa_context *ctx = NULL;

	IPADBG("ENTER\n");
	ctx = container_of(inode->i_cdev, struct ipa_context, cdev);
	filp->private_data = ctx;

	return 0;
}

static long ipa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	u32 pyld_sz;
	u8 header[128] = { 0 };
	u8 *param = NULL;
	struct ipa_ioc_nat_alloc_mem nat_mem;
	struct ipa_ioc_v4_nat_init nat_init;
	struct ipa_ioc_v4_nat_del nat_del;

	IPADBG("cmd=%x nr=%d\n", cmd, _IOC_NR(cmd));

	if (_IOC_TYPE(cmd) != IPA_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) >= IPA_IOCTL_MAX)
		return -ENOTTY;

	switch (cmd) {
	case IPA_IOC_ALLOC_NAT_MEM:
		if (copy_from_user((u8 *)&nat_mem, (u8 *)arg,
					sizeof(struct ipa_ioc_nat_alloc_mem))) {
			retval = -EFAULT;
			break;
		}

		if (allocate_nat_device(&nat_mem)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, (u8 *)&nat_mem,
					sizeof(struct ipa_ioc_nat_alloc_mem))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_V4_INIT_NAT:
		if (copy_from_user((u8 *)&nat_init, (u8 *)arg,
					sizeof(struct ipa_ioc_v4_nat_init))) {
			retval = -EFAULT;
			break;
		}
		if (ipa_nat_init_cmd(&nat_init)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_NAT_DMA:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_nat_dma_cmd))) {
			retval = -EFAULT;
			break;
		}

		pyld_sz =
		   sizeof(struct ipa_ioc_nat_dma_cmd) +
		   ((struct ipa_ioc_nat_dma_cmd *)header)->entries *
		   sizeof(struct ipa_ioc_nat_dma_one);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}

		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}

		if (ipa_nat_dma_cmd((struct ipa_ioc_nat_dma_cmd *)param)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_V4_DEL_NAT:
		if (copy_from_user((u8 *)&nat_del, (u8 *)arg,
					sizeof(struct ipa_ioc_v4_nat_del))) {
			retval = -EFAULT;
			break;
		}
		if (ipa_nat_del_cmd(&nat_del)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_hdr))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_add_hdr) +
		   ((struct ipa_ioc_add_hdr *)header)->num_hdrs *
		   sizeof(struct ipa_hdr_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_add_hdr((struct ipa_ioc_add_hdr *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_hdr))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_del_hdr) +
		   ((struct ipa_ioc_del_hdr *)header)->num_hdls *
		   sizeof(struct ipa_hdr_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_del_hdr((struct ipa_ioc_del_hdr *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_RT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_rt_rule))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_add_rt_rule) +
		   ((struct ipa_ioc_add_rt_rule *)header)->num_rules *
		   sizeof(struct ipa_rt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_add_rt_rule((struct ipa_ioc_add_rt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_RT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_rt_rule))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_del_rt_rule) +
		   ((struct ipa_ioc_del_rt_rule *)header)->num_hdls *
		   sizeof(struct ipa_rt_rule_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_del_rt_rule((struct ipa_ioc_del_rt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_FLT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_flt_rule))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_add_flt_rule) +
		   ((struct ipa_ioc_add_flt_rule *)header)->num_rules *
		   sizeof(struct ipa_flt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_add_flt_rule((struct ipa_ioc_add_flt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_FLT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_flt_rule))) {
			retval = -EFAULT;
			break;
		}
		pyld_sz =
		   sizeof(struct ipa_ioc_del_flt_rule) +
		   ((struct ipa_ioc_del_flt_rule *)header)->num_hdls *
		   sizeof(struct ipa_flt_rule_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		if (ipa_del_flt_rule((struct ipa_ioc_del_flt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_COMMIT_HDR:
		retval = ipa_commit_hdr();
		break;
	case IPA_IOC_RESET_HDR:
		retval = ipa_reset_hdr();
		break;
	case IPA_IOC_COMMIT_RT:
		retval = ipa_commit_rt(arg);
		break;
	case IPA_IOC_RESET_RT:
		retval = ipa_reset_rt(arg);
		break;
	case IPA_IOC_COMMIT_FLT:
		retval = ipa_commit_flt(arg);
		break;
	case IPA_IOC_RESET_FLT:
		retval = ipa_reset_flt(arg);
		break;
	case IPA_IOC_DUMP:
		ipa_dump();
		break;
	case IPA_IOC_GET_RT_TBL:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_get_rt_tbl))) {
			retval = -EFAULT;
			break;
		}
		if (ipa_get_rt_tbl((struct ipa_ioc_get_rt_tbl *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_get_rt_tbl))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_PUT_RT_TBL:
		retval = ipa_put_rt_tbl(arg);
		break;
	case IPA_IOC_GET_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_get_hdr))) {
			retval = -EFAULT;
			break;
		}
		if (ipa_get_hdr((struct ipa_ioc_get_hdr *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_get_hdr))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_PUT_HDR:
		retval = ipa_put_hdr(arg);
		break;
	case IPA_IOC_SET_FLT:
		retval = ipa_cfg_filter(arg);
		break;
	case IPA_IOC_COPY_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_copy_hdr))) {
			retval = -EFAULT;
			break;
		}
		if (ipa_copy_hdr((struct ipa_ioc_copy_hdr *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_copy_hdr))) {
			retval = -EFAULT;
			break;
		}
		break;
	default:        /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	kfree(param);

	return retval;
}

/**
* ipa_setup_dflt_rt_tables() - Setup default routing tables
*
* Return codes:
* 0: success
* -ENOMEM: failed to allocate memory
* -EPERM: failed to add the tables
*/
int ipa_setup_dflt_rt_tables(void)
{
	struct ipa_ioc_add_rt_rule *rt_rule;
	struct ipa_rt_rule_add *rt_rule_entry;

	rt_rule =
	   kzalloc(sizeof(struct ipa_ioc_add_rt_rule) + 1 *
			   sizeof(struct ipa_rt_rule_add), GFP_KERNEL);
	if (!rt_rule) {
		IPAERR("fail to alloc mem\n");
		return -ENOMEM;
	}
	/* setup a default v4 route to point to A5 */
	rt_rule->num_rules = 1;
	rt_rule->commit = 1;
	rt_rule->ip = IPA_IP_v4;
	strlcpy(rt_rule->rt_tbl_name, IPA_DFLT_RT_TBL_NAME,
			IPA_RESOURCE_NAME_MAX);

	rt_rule_entry = &rt_rule->rules[0];
	rt_rule_entry->at_rear = 1;
	rt_rule_entry->rule.dst = IPA_CLIENT_A5_LAN_WAN_CONS;
	rt_rule_entry->rule.hdr_hdl = ipa_ctx->excp_hdr_hdl;

	if (ipa_add_rt_rule(rt_rule)) {
		IPAERR("fail to add dflt v4 rule\n");
		kfree(rt_rule);
		return -EPERM;
	}
	IPADBG("dflt v4 rt rule hdl=%x\n", rt_rule_entry->rt_rule_hdl);
	ipa_ctx->dflt_v4_rt_rule_hdl = rt_rule_entry->rt_rule_hdl;

	/* setup a default v6 route to point to A5 */
	rt_rule->ip = IPA_IP_v6;
	if (ipa_add_rt_rule(rt_rule)) {
		IPAERR("fail to add dflt v6 rule\n");
		kfree(rt_rule);
		return -EPERM;
	}
	IPADBG("dflt v6 rt rule hdl=%x\n", rt_rule_entry->rt_rule_hdl);
	ipa_ctx->dflt_v6_rt_rule_hdl = rt_rule_entry->rt_rule_hdl;

	/*
	 * because these tables are the very first to be added, they will both
	 * have the same index (0) which is essential for programming the
	 * "route" end-point config
	 */

	kfree(rt_rule);

	return 0;
}

static int ipa_setup_exception_path(void)
{
	struct ipa_ioc_add_hdr *hdr;
	struct ipa_hdr_add *hdr_entry;
	struct ipa_route route = { 0 };
	int ret;

	/* install the basic exception header */
	hdr = kzalloc(sizeof(struct ipa_ioc_add_hdr) + 1 *
		      sizeof(struct ipa_hdr_add), GFP_KERNEL);
	if (!hdr) {
		IPAERR("fail to alloc exception hdr\n");
		return -ENOMEM;
	}
	hdr->num_hdrs = 1;
	hdr->commit = 1;
	hdr_entry = &hdr->hdr[0];
	strlcpy(hdr_entry->name, IPA_DFLT_HDR_NAME, IPA_RESOURCE_NAME_MAX);

	/*
	 * only single stream for MBIM supported and no exception packets
	 * expected so set default header to zero
	 */
	hdr_entry->hdr_len = 1;
	hdr_entry->hdr[0] = 0;

	/*
	 * SW does not know anything about default exception header so
	 * we don't set it. IPA HW will use it as a template
	 */
	if (ipa_add_hdr(hdr)) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	if (hdr_entry->status) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	ipa_ctx->excp_hdr_hdl = hdr_entry->hdr_hdl;

	/* exception packets goto LAN-WAN pipe from IPA to A5 */
	route.route_def_pipe = IPA_A5_LAN_WAN_IN;
	route.route_def_hdr_table = !ipa_ctx->hdr_tbl_lcl;

	if (ipa_cfg_route(&route)) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	ret = 0;
bail:
	kfree(hdr);
	return ret;
}

static void ipa_handle_tx_poll_for_pipe(struct ipa_sys_context *sys)
{
	struct ipa_tx_pkt_wrapper *tx_pkt, *t;
	struct sps_iovec iov;
	unsigned long irq_flags;
	int ret;

	while (1) {
		iov.addr = 0;
		ret = sps_get_iovec(sys->ep->ep_hdl, &iov);
		if (ret) {
			pr_err("%s: sps_get_iovec failed %d\n", __func__, ret);
			break;
		}
		if (!iov.addr)
			break;
		spin_lock_irqsave(&sys->spinlock, irq_flags);
		tx_pkt = list_first_entry(&sys->head_desc_list,
					  struct ipa_tx_pkt_wrapper, link);
		spin_unlock_irqrestore(&sys->spinlock, irq_flags);

		switch (tx_pkt->cnt) {
		case 1:
			ipa_write_done(&tx_pkt->work);
			break;
		case 0xFFFF:
			/* reached end of set */
			spin_lock_irqsave(&sys->spinlock, irq_flags);
			list_for_each_entry_safe(tx_pkt, t,
						 &sys->wait_desc_list, link) {
				list_del(&tx_pkt->link);
				list_add(&tx_pkt->link, &sys->head_desc_list);
			}
			tx_pkt =
			   list_first_entry(&sys->head_desc_list,
					    struct ipa_tx_pkt_wrapper, link);
			spin_unlock_irqrestore(&sys->spinlock, irq_flags);
			ipa_write_done(&tx_pkt->work);
			break;
		default:
			/* keep looping till reach the end of the set */
			spin_lock_irqsave(&sys->spinlock,
					  irq_flags);
			list_del(&tx_pkt->link);
			list_add_tail(&tx_pkt->link,
				      &sys->wait_desc_list);
			spin_unlock_irqrestore(&sys->spinlock,
					       irq_flags);
			break;
		}
	}
}

static void ipa_poll_function(struct work_struct *work)
{
	int ret;
	int tx_pipes[] = { IPA_A5_CMD, IPA_A5_LAN_WAN_OUT,
		IPA_A5_WLAN_AMPDU_OUT };
	int i;
	int num_tx_pipes;

	/* check all the system pipes for tx completions and rx available */
	if (ipa_ctx->sys[IPA_A5_LAN_WAN_IN].ep->valid)
		ipa_handle_rx_core();

	num_tx_pipes = sizeof(tx_pipes) / sizeof(tx_pipes[0]);

	if (!IPA_MOBILE_AP_MODE(ipa_ctx->mode))
		num_tx_pipes--;

	for (i = 0; i < num_tx_pipes; i++)
		if (ipa_ctx->sys[tx_pipes[i]].ep->valid)
			ipa_handle_tx_poll_for_pipe(&ipa_ctx->sys[tx_pipes[i]]);

	/* re-post the poll work */
	INIT_DELAYED_WORK(&ipa_ctx->poll_work, ipa_poll_function);
	ret = schedule_delayed_work_on(smp_processor_id(), &ipa_ctx->poll_work,
			msecs_to_jiffies(polling_delay_ms));

	return;
}

static int ipa_setup_a5_pipes(void)
{
	struct ipa_sys_connect_params sys_in;
	int result = 0;

	/* CMD OUT (A5->IPA) */
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_A5_CMD_PROD;
	sys_in.desc_fifo_sz = IPA_SYS_DESC_FIFO_SZ;
	sys_in.ipa_ep_cfg.mode.mode = IPA_DMA;
	sys_in.ipa_ep_cfg.mode.dst = IPA_CLIENT_A5_LAN_WAN_CONS;
	if (ipa_setup_sys_pipe(&sys_in, &ipa_ctx->clnt_hdl_cmd)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail;
	}

	if (ipa_setup_exception_path()) {
		IPAERR(":fail to setup excp path\n");
		result = -EPERM;
		goto fail_cmd;
	}

	/* LAN-WAN IN (IPA->A5) */
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_A5_LAN_WAN_CONS;
	sys_in.desc_fifo_sz = IPA_SYS_DESC_FIFO_SZ;
	sys_in.ipa_ep_cfg.hdr.hdr_a5_mux = 1;
	sys_in.ipa_ep_cfg.hdr.hdr_len = 8;  /* size of A5 exception hdr */
	if (ipa_setup_sys_pipe(&sys_in, &ipa_ctx->clnt_hdl_data_in)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail_cmd;
	}
	/* LAN-WAN OUT (A5->IPA) */
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_A5_LAN_WAN_PROD;
	sys_in.desc_fifo_sz = IPA_SYS_DESC_FIFO_SZ;
	sys_in.ipa_ep_cfg.mode.mode = IPA_BASIC;
	sys_in.ipa_ep_cfg.mode.dst = IPA_CLIENT_A5_LAN_WAN_CONS;
	if (ipa_setup_sys_pipe(&sys_in, &ipa_ctx->clnt_hdl_data_out)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail_data_out;
	}
	if (ipa_ctx->polling_mode) {
		INIT_DELAYED_WORK(&ipa_ctx->poll_work, ipa_poll_function);
		result =
		   schedule_delayed_work_on(smp_processor_id(),
					&ipa_ctx->poll_work,
					msecs_to_jiffies(polling_delay_ms));
		if (!result) {
			IPAERR(":schedule delayed work failed.\n");
			goto fail_schedule_delayed_work;
		}
	}

	return 0;

fail_schedule_delayed_work:
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_data_out);
fail_data_out:
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_data_in);
fail_cmd:
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_cmd);
fail:
	return result;
}

static void ipa_teardown_a5_pipes(void)
{
	cancel_delayed_work(&ipa_ctx->poll_work);
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_data_out);
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_data_in);
	ipa_teardown_sys_pipe(ipa_ctx->clnt_hdl_cmd);
}

static int ipa_load_pipe_connection(struct platform_device *pdev,
				    enum a2_mux_pipe_direction  pipe_dir,
				    struct a2_mux_pipe_connection *pdata)
{
	struct device_node *node = pdev->dev.of_node;
	int rc = 0;

	if (!pdata || !pdev)
		goto err;

	/* retrieve device tree parameters */
	for_each_child_of_node(pdev->dev.of_node, node)
	{
		const char *str;

		rc = of_property_read_string(node, "label", &str);
		if (rc) {
			IPAERR("Cannot read string\n");
			goto err;
		}

		/* Check if connection type is supported */
		if (strncmp(str, "a2-to-ipa", 10)
			&& strncmp(str, "ipa-to-a2", 10))
			goto err;

		if (strnstr(str, "a2-to-ipa", strnlen("a2-to-ipa", 10))
				&& IPA_TO_A2 == pipe_dir)
			continue; /* skip to the next pipe */
		else if (strnstr(str, "ipa-to-a2", strnlen("ipa-to-a2", 10))
				&& A2_TO_IPA == pipe_dir)
			continue; /* skip to the next pipe */


		rc = ipa_update_connections_info(node, pdata);
		if (rc)
			goto err;
	}

	return 0;
err:
	IPAERR("%s: failed\n", __func__);

	return rc;
}

static int ipa_update_connections_info(struct device_node *node,
		struct a2_mux_pipe_connection     *pipe_connection)
{
	u32      rc;
	char     *key;
	uint32_t val;
	enum ipa_pipe_mem_type mem_type;

	if (!pipe_connection || !node)
		goto err;

	key = "qcom,src-bam-physical-address";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->src_phy_addr = val;

	key = "qcom,ipa-bam-mem-type";
	rc = of_property_read_u32(node, key, &mem_type);
	if (rc)
		goto err;
	pipe_connection->mem_type = mem_type;

	key = "qcom,src-bam-pipe-index";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->src_pipe_index = val;

	key = "qcom,dst-bam-physical-address";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->dst_phy_addr = val;

	key = "qcom,dst-bam-pipe-index";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->dst_pipe_index = val;

	key = "qcom,data-fifo-offset";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->data_fifo_base_offset = val;

	key = "qcom,data-fifo-size";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->data_fifo_size = val;

	key = "qcom,descriptor-fifo-offset";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->desc_fifo_base_offset = val;

	key = "qcom,descriptor-fifo-size";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;

	pipe_connection->desc_fifo_size = val;

	return 0;
err:
	IPAERR("%s: Error in name %s key %s\n", __func__, node->full_name, key);

	return rc;
}

/**
* ipa_get_a2_mux_pipe_info() - Exposes A2 parameters fetched from DTS
*
* @pipe_dir: pipe direction
* @pipe_connect: connect structure containing the parameters fetched from DTS
*
* Return codes:
* 0: success
* -EFAULT: invalid parameters
*/
int ipa_get_a2_mux_pipe_info(enum a2_mux_pipe_direction  pipe_dir,
			     struct a2_mux_pipe_connection *pipe_connect)
{
	if (!pipe_connect) {
		IPAERR("ipa_get_a2_mux_pipe_info switch null args\n");
		return -EFAULT;
	}

	switch (pipe_dir) {
	case A2_TO_IPA:
		*pipe_connect = ipa_res.a2_to_ipa_pipe;
		break;
	case IPA_TO_A2:
		*pipe_connect = ipa_res.ipa_to_a2_pipe;
		break;
	default:
		IPAERR("ipa_get_a2_mux_pipe_info switch in default\n");
		return -EFAULT;
	}

	return 0;
}

static void ipa_set_aggregation_params(void)
{
	struct ipa_ep_cfg_aggr agg_params;
	u32 producer_hdl = 0;
	u32 consumer_hdl = 0;

	rmnet_bridge_get_client_handles(&producer_hdl, &consumer_hdl);

	agg_params.aggr = ipa_ctx->aggregation_type;
	agg_params.aggr_byte_limit = ipa_ctx->aggregation_byte_limit;
	agg_params.aggr_time_limit = ipa_ctx->aggregation_time_limit;

	/* configure aggregation on producer */
	agg_params.aggr_en = IPA_ENABLE_AGGR;
	ipa_cfg_ep_aggr(producer_hdl, &agg_params);

	/* configure deaggregation on consumer */
	agg_params.aggr_en = IPA_ENABLE_DEAGGR;
	ipa_cfg_ep_aggr(consumer_hdl, &agg_params);

}

/*
 * The following device attributes are for configuring the aggregation
 * attributes when the driver is already running.
 * The attributes are for configuring the aggregation type
 * (MBIM_16/MBIM_32/TLP), the aggregation byte limit and the aggregation
 * time limit.
 */
static ssize_t ipa_show_aggregation_type(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t ret_val;
	char str[IPA_AGGR_MAX_STR_LENGTH];

	if (!buf) {
		IPAERR("buffer for ipa_show_aggregation_type is NULL\n");
		return -EINVAL;
	}

	memset(str, 0, sizeof(str));

	switch (ipa_ctx->aggregation_type) {
	case IPA_MBIM_16:
		strlcpy(str, "MBIM_16", IPA_AGGR_STR_IN_BYTES("MBIM_16"));
		break;
	case IPA_MBIM_32:
		strlcpy(str, "MBIM_32", IPA_AGGR_STR_IN_BYTES("MBIM_32"));
		break;
	case IPA_TLP:
		strlcpy(str, "TLP", IPA_AGGR_STR_IN_BYTES("TLP"));
		break;
	default:
		strlcpy(str, "NONE", IPA_AGGR_STR_IN_BYTES("NONE"));
		break;
	}

	ret_val = scnprintf(buf, PAGE_SIZE, "%s\n", str);

	return ret_val;
}

static ssize_t ipa_store_aggregation_type(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char str[IPA_AGGR_MAX_STR_LENGTH], *pstr;

	if (!buf) {
		IPAERR("buffer for ipa_store_aggregation_type is NULL\n");
		return -EINVAL;
	}

	strlcpy(str, buf, sizeof(str));
	pstr = strim(str);

	if (!strncmp(pstr, "MBIM_16", IPA_AGGR_STR_IN_BYTES("MBIM_16")))
		ipa_ctx->aggregation_type = IPA_MBIM_16;
	else if (!strncmp(pstr, "MBIM_32", IPA_AGGR_STR_IN_BYTES("MBIM_32")))
		ipa_ctx->aggregation_type = IPA_MBIM_32;
	else if (!strncmp(pstr, "TLP", IPA_AGGR_STR_IN_BYTES("TLP")))
		ipa_ctx->aggregation_type = IPA_TLP;
	else {
		IPAERR("ipa_store_aggregation_type wrong input\n");
		return -EINVAL;
	}

	ipa_set_aggregation_params();

	return count;
}

static DEVICE_ATTR(aggregation_type, S_IWUSR | S_IRUSR,
		ipa_show_aggregation_type,
		ipa_store_aggregation_type);

static ssize_t ipa_show_aggregation_byte_limit(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t ret_val;

	if (!buf) {
		IPAERR("buffer for ipa_show_aggregation_byte_limit is NULL\n");
		return -EINVAL;
	}

	ret_val = scnprintf(buf, PAGE_SIZE, "%u\n",
			    ipa_ctx->aggregation_byte_limit);

	return ret_val;
}

static ssize_t ipa_store_aggregation_byte_limit(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char str[IPA_AGGR_MAX_STR_LENGTH];
	char *pstr;
	u32 ret = 0;

	if (!buf) {
		IPAERR("buffer for ipa_store_aggregation_byte_limit is NULL\n");
		return -EINVAL;
	}

	strlcpy(str, buf, sizeof(str));
	pstr = strim(str);

	if (kstrtouint(pstr, IPA_AGGR_MAX_STR_LENGTH, &ret)) {
		IPAERR("ipa_store_aggregation_byte_limit wrong input\n");
		return -EINVAL;
	}

	ipa_ctx->aggregation_byte_limit = ret;

	ipa_set_aggregation_params();

	return count;
}

static DEVICE_ATTR(aggregation_byte_limit, S_IWUSR | S_IRUSR,
		ipa_show_aggregation_byte_limit,
		ipa_store_aggregation_byte_limit);

static ssize_t ipa_show_aggregation_time_limit(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t ret_val;

	if (!buf) {
		IPAERR("buffer for ipa_show_aggregation_time_limit is NULL\n");
		return -EINVAL;
	}

	ret_val = scnprintf(buf,
			    PAGE_SIZE,
			    "%u\n",
			    ipa_ctx->aggregation_time_limit);

	return ret_val;
}

static ssize_t ipa_store_aggregation_time_limit(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char str[IPA_AGGR_MAX_STR_LENGTH], *pstr;
	u32 ret = 0;

	if (!buf) {
		IPAERR("buffer for ipa_store_aggregation_time_limit is NULL\n");
		return -EINVAL;
	}

	strlcpy(str, buf, sizeof(str));
	pstr = strim(str);

	if (kstrtouint(pstr, IPA_AGGR_MAX_STR_LENGTH, &ret)) {
		IPAERR("ipa_store_aggregation_time_limit wrong input\n");
		return -EINVAL;
	}

	ipa_ctx->aggregation_time_limit = ret;

	ipa_set_aggregation_params();

	return count;
}

static DEVICE_ATTR(aggregation_time_limit, S_IWUSR | S_IRUSR,
		ipa_show_aggregation_time_limit,
		ipa_store_aggregation_time_limit);

static const struct file_operations ipa_drv_fops = {
	.owner = THIS_MODULE,
	.open = ipa_open,
	.read = ipa_read,
	.unlocked_ioctl = ipa_ioctl,
};

static int ipa_get_clks(struct device *dev)
{
	ipa_cnoc_clk = clk_get(dev, "iface_clk");
	if (IS_ERR(ipa_cnoc_clk)) {
		ipa_cnoc_clk = NULL;
		IPAERR("fail to get cnoc clk\n");
		return -ENODEV;
	}

	ipa_clk_src = clk_get(dev, "core_src_clk");
	if (IS_ERR(ipa_clk_src)) {
		ipa_clk_src = NULL;
		IPAERR("fail to get ipa clk src\n");
		return -ENODEV;
	}

	ipa_clk = clk_get(dev, "core_clk");
	if (IS_ERR(ipa_clk)) {
		ipa_clk = NULL;
		IPAERR("fail to get ipa clk\n");
		return -ENODEV;
	}

	sys_noc_ipa_axi_clk = clk_get(dev, "bus_clk");
	if (IS_ERR(sys_noc_ipa_axi_clk)) {
		sys_noc_ipa_axi_clk = NULL;
		IPAERR("fail to get sys_noc_ipa_axi clk\n");
		return -ENODEV;
	}

	ipa_inactivity_clk = clk_get(dev, "inactivity_clk");
	if (IS_ERR(ipa_inactivity_clk)) {
		ipa_inactivity_clk = NULL;
		IPAERR("fail to get inactivity clk\n");
		return -ENODEV;
	}

	return 0;
}

/**
* ipa_enable_clks() - Turn on IPA clocks
*
* Return codes:
* None
*/
void ipa_enable_clks(void)
{
	if (ipa_cnoc_clk) {
		clk_prepare(ipa_cnoc_clk);
		clk_enable(ipa_cnoc_clk);
		clk_set_rate(ipa_cnoc_clk, IPA_CNOC_CLK_RATE);
	} else {
		WARN_ON(1);
	}

	if (ipa_clk_src)
		clk_set_rate(ipa_clk_src, IPA_V1_CLK_RATE);
	else
		WARN_ON(1);

	if (ipa_clk)
		clk_prepare(ipa_clk);
	else
		WARN_ON(1);

	if (sys_noc_ipa_axi_clk)
		clk_prepare(sys_noc_ipa_axi_clk);
	else
		WARN_ON(1);

	if (ipa_inactivity_clk)
		clk_prepare(ipa_inactivity_clk);
	else
		WARN_ON(1);

	if (ipa_clk)
		clk_enable(ipa_clk);
	else
		WARN_ON(1);

	if (sys_noc_ipa_axi_clk)
		clk_enable(sys_noc_ipa_axi_clk);
	else
		WARN_ON(1);

	if (ipa_inactivity_clk)
		clk_enable(ipa_inactivity_clk);
	else
		WARN_ON(1);
}

/**
* ipa_disable_clks() - Turn off IPA clocks
*
* Return codes:
* None
*/
void ipa_disable_clks(void)
{
	if (ipa_inactivity_clk)
		clk_disable_unprepare(ipa_inactivity_clk);
	else
		WARN_ON(1);

	if (sys_noc_ipa_axi_clk)
		clk_disable_unprepare(sys_noc_ipa_axi_clk);
	else
		WARN_ON(1);

	if (ipa_clk)
		clk_disable_unprepare(ipa_clk);
	else
		WARN_ON(1);

	if (ipa_cnoc_clk)
		clk_disable_unprepare(ipa_cnoc_clk);
	else
		WARN_ON(1);
}

static int ipa_setup_bam_cfg(const struct ipa_plat_drv_res *res)
{
	void *bam_cnfg_bits;

	bam_cnfg_bits = ioremap(res->ipa_mem_base + IPA_BAM_REG_BASE_OFST,
				IPA_BAM_REMAP_SIZE);
	if (!bam_cnfg_bits)
		return -ENOMEM;
	ipa_write_reg(bam_cnfg_bits, IPA_BAM_CNFG_BITS_OFST,
		      IPA_BAM_CNFG_BITS_VAL);
	iounmap(bam_cnfg_bits);

	return 0;
}
/**
* ipa_init() - Initialize the IPA Driver
*@resource_p:	contain platform specific values from DST file
*
* Function initialization process:
* - Allocate memory for the driver context data struct
* - Initializing the ipa_ctx with:
*    1)parsed values from the dts file
*    2)parameters passed to the module initialization
*    3)read HW values(such as core memory size)
* - Map IPA core registers to CPU memory
* - Restart IPA core(HW reset)
* - Register IPA BAM to SPS driver and get a BAM handler
* - Set configuration for IPA BAM via BAM_CNFG_BITS
* - Initialize the look-aside caches(kmem_cache/slab) for filter,
*   routing and IPA-tree
* - Create memory pool with 4 objects for DMA operations(each object
*   is 512Bytes long), this object will be use for tx(A5->IPA)
* - Initialize lists head(routing,filter,hdr,system pipes)
* - Initialize mutexes (for ipa_ctx and NAT memory mutexes)
* - Initialize spinlocks (for list related to A5<->IPA pipes)
* - Initialize 2 single-threaded work-queue named "ipa rx wq" and "ipa tx wq"
* - Initialize Red-Black-Tree(s) for handles of header,routing rule,
*   routing table ,filtering rule
* - Setup all A5<->IPA pipes by calling to ipa_setup_a5_pipes
* - Preparing the descriptors for System pipes
* - Initialize the filter block by committing IPV4 and IPV6 default rules
* - Create empty routing table in system memory(no committing)
* - Initialize pipes memory pool with ipa_pipe_mem_init for supported platforms
* - Create a char-device for IPA
*/
static int ipa_init(const struct ipa_plat_drv_res *resource_p)
{
	int result = 0;
	int i;
	struct sps_bam_props bam_props = { 0 };
	struct ipa_flt_tbl *flt_tbl;
	struct ipa_rt_tbl_set *rset;

	IPADBG("IPA init\n");

	ipa_ctx = kzalloc(sizeof(*ipa_ctx), GFP_KERNEL);
	if (!ipa_ctx) {
		IPAERR(":kzalloc err.\n");
		result = -ENOMEM;
		goto fail_mem;
	}

	IPADBG("polling_mode=%u delay_ms=%u\n", polling_mode, polling_delay_ms);
	ipa_ctx->polling_mode = polling_mode;
	IPADBG("hdr_lcl=%u ip4_rt=%u ip6_rt=%u ip4_flt=%u ip6_flt=%u\n",
	       hdr_tbl_lcl, ip4_rt_tbl_lcl, ip6_rt_tbl_lcl, ip4_flt_tbl_lcl,
	       ip6_flt_tbl_lcl);
	ipa_ctx->hdr_tbl_lcl = hdr_tbl_lcl;
	ipa_ctx->ip4_rt_tbl_lcl = ip4_rt_tbl_lcl;
	ipa_ctx->ip6_rt_tbl_lcl = ip6_rt_tbl_lcl;
	ipa_ctx->ip4_flt_tbl_lcl = ip4_flt_tbl_lcl;
	ipa_ctx->ip6_flt_tbl_lcl = ip6_flt_tbl_lcl;

	ipa_ctx->ipa_wrapper_base = resource_p->ipa_mem_base;

	/* setup IPA register access */
	ipa_ctx->mmio = ioremap(resource_p->ipa_mem_base + IPA_REG_BASE_OFST,
			resource_p->ipa_mem_size);
	if (!ipa_ctx->mmio) {
		IPAERR(":ipa-base ioremap err.\n");
		result = -EFAULT;
		goto fail_remap;
	}
	/* do POR programming to setup HW */
	result = ipa_init_hw();
	if (result) {
		IPAERR(":error initializing driver.\n");
		result = -ENODEV;
		goto fail_init_hw;
	}
	/* read how much SRAM is available for SW use */
	ipa_ctx->smem_sz = ipa_read_reg(ipa_ctx->mmio,
			IPA_SHARED_MEM_SIZE_OFST);

	if (IPA_RAM_END_OFST > ipa_ctx->smem_sz) {
		IPAERR("SW expect more core memory, needed %d, avail %d\n",
				IPA_RAM_END_OFST, ipa_ctx->smem_sz);
		result = -ENOMEM;
		goto fail_init_hw;
	}
	/* register IPA with SPS driver */
	bam_props.phys_addr = resource_p->bam_mem_base;
	bam_props.virt_addr = ioremap(resource_p->bam_mem_base,
			resource_p->bam_mem_size);
	if (!bam_props.virt_addr) {
		IPAERR(":bam-base ioremap err.\n");
		result = -EFAULT;
		goto fail_bam_remap;
	}
	bam_props.virt_size = resource_p->bam_mem_size;
	bam_props.irq = resource_p->bam_irq;
	bam_props.num_pipes = IPA_NUM_PIPES;
	bam_props.summing_threshold = IPA_SUMMING_THRESHOLD;
	bam_props.event_threshold = IPA_EVENT_THRESHOLD;

	result = sps_register_bam_device(&bam_props, &ipa_ctx->bam_handle);
	if (result) {
		IPAERR(":bam register err.\n");
		result = -ENODEV;
		goto fail_bam_register;
	}

	if (ipa_setup_bam_cfg(resource_p)) {
		IPAERR(":bam cfg err.\n");
		result = -ENODEV;
		goto fail_flt_rule_cache;
	}

	/* set up the default op mode */
	ipa_ctx->mode = IPA_MODE_USB_DONGLE;

	/* init the lookaside cache */
	ipa_ctx->flt_rule_cache = kmem_cache_create("IPA FLT",
			sizeof(struct ipa_flt_entry), 0, 0, NULL);
	if (!ipa_ctx->flt_rule_cache) {
		IPAERR(":ipa flt cache create failed\n");
		result = -ENOMEM;
		goto fail_flt_rule_cache;
	}
	ipa_ctx->rt_rule_cache = kmem_cache_create("IPA RT",
			sizeof(struct ipa_rt_entry), 0, 0, NULL);
	if (!ipa_ctx->rt_rule_cache) {
		IPAERR(":ipa rt cache create failed\n");
		result = -ENOMEM;
		goto fail_rt_rule_cache;
	}
	ipa_ctx->hdr_cache = kmem_cache_create("IPA HDR",
			sizeof(struct ipa_hdr_entry), 0, 0, NULL);
	if (!ipa_ctx->hdr_cache) {
		IPAERR(":ipa hdr cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_cache;
	}
	ipa_ctx->hdr_offset_cache =
	   kmem_cache_create("IPA HDR OFF", sizeof(struct ipa_hdr_offset_entry),
			   0, 0, NULL);
	if (!ipa_ctx->hdr_offset_cache) {
		IPAERR(":ipa hdr off cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_offset_cache;
	}
	ipa_ctx->rt_tbl_cache = kmem_cache_create("IPA RT TBL",
			sizeof(struct ipa_rt_tbl), 0, 0, NULL);
	if (!ipa_ctx->rt_tbl_cache) {
		IPAERR(":ipa rt tbl cache create failed\n");
		result = -ENOMEM;
		goto fail_rt_tbl_cache;
	}
	ipa_ctx->tx_pkt_wrapper_cache =
	   kmem_cache_create("IPA TX PKT WRAPPER",
			   sizeof(struct ipa_tx_pkt_wrapper), 0, 0, NULL);
	if (!ipa_ctx->tx_pkt_wrapper_cache) {
		IPAERR(":ipa tx pkt wrapper cache create failed\n");
		result = -ENOMEM;
		goto fail_tx_pkt_wrapper_cache;
	}
	ipa_ctx->rx_pkt_wrapper_cache =
	   kmem_cache_create("IPA RX PKT WRAPPER",
			   sizeof(struct ipa_rx_pkt_wrapper), 0, 0, NULL);
	if (!ipa_ctx->rx_pkt_wrapper_cache) {
		IPAERR(":ipa rx pkt wrapper cache create failed\n");
		result = -ENOMEM;
		goto fail_rx_pkt_wrapper_cache;
	}
	ipa_ctx->tree_node_cache =
	   kmem_cache_create("IPA TREE", sizeof(struct ipa_tree_node), 0, 0,
			   NULL);
	if (!ipa_ctx->tree_node_cache) {
		IPAERR(":ipa tree node cache create failed\n");
		result = -ENOMEM;
		goto fail_tree_node_cache;
	}

	/*
	 * setup DMA pool 4 byte aligned, don't cross 1k boundaries, nominal
	 * size 512 bytes
	 */
	ipa_ctx->one_kb_no_straddle_pool = dma_pool_create("ipa_1k", NULL,
			IPA_DMA_POOL_SIZE, IPA_DMA_POOL_ALIGNMENT,
			IPA_DMA_POOL_BOUNDARY);
	if (!ipa_ctx->one_kb_no_straddle_pool) {
		IPAERR("cannot setup 1kb alloc DMA pool.\n");
		result = -ENOMEM;
		goto fail_dma_pool;
	}

	ipa_ctx->glob_flt_tbl[IPA_IP_v4].in_sys = !ipa_ctx->ip4_flt_tbl_lcl;
	ipa_ctx->glob_flt_tbl[IPA_IP_v6].in_sys = !ipa_ctx->ip6_flt_tbl_lcl;

	/* init the various list heads */
	INIT_LIST_HEAD(&ipa_ctx->glob_flt_tbl[IPA_IP_v4].head_flt_rule_list);
	INIT_LIST_HEAD(&ipa_ctx->glob_flt_tbl[IPA_IP_v6].head_flt_rule_list);
	INIT_LIST_HEAD(&ipa_ctx->hdr_tbl.head_hdr_entry_list);
	for (i = 0; i < IPA_HDR_BIN_MAX; i++) {
		INIT_LIST_HEAD(&ipa_ctx->hdr_tbl.head_offset_list[i]);
		INIT_LIST_HEAD(&ipa_ctx->hdr_tbl.head_free_offset_list[i]);
	}
	INIT_LIST_HEAD(&ipa_ctx->rt_tbl_set[IPA_IP_v4].head_rt_tbl_list);
	INIT_LIST_HEAD(&ipa_ctx->rt_tbl_set[IPA_IP_v6].head_rt_tbl_list);
	for (i = 0; i < IPA_NUM_PIPES; i++) {
		flt_tbl = &ipa_ctx->flt_tbl[i][IPA_IP_v4];
		INIT_LIST_HEAD(&flt_tbl->head_flt_rule_list);
		flt_tbl->in_sys = !ipa_ctx->ip4_flt_tbl_lcl;

		flt_tbl = &ipa_ctx->flt_tbl[i][IPA_IP_v6];
		INIT_LIST_HEAD(&flt_tbl->head_flt_rule_list);
		flt_tbl->in_sys = !ipa_ctx->ip6_flt_tbl_lcl;
	}

	rset = &ipa_ctx->reap_rt_tbl_set[IPA_IP_v4];
	INIT_LIST_HEAD(&rset->head_rt_tbl_list);
	rset = &ipa_ctx->reap_rt_tbl_set[IPA_IP_v6];
	INIT_LIST_HEAD(&rset->head_rt_tbl_list);

	mutex_init(&ipa_ctx->lock);
	mutex_init(&ipa_ctx->nat_mem.lock);

	for (i = 0; i < IPA_A5_SYS_MAX; i++) {
		INIT_LIST_HEAD(&ipa_ctx->sys[i].head_desc_list);
		spin_lock_init(&ipa_ctx->sys[i].spinlock);
		if (i != IPA_A5_WLAN_AMPDU_OUT)
			ipa_ctx->sys[i].ep = &ipa_ctx->ep[i];
		else
			ipa_ctx->sys[i].ep = &ipa_ctx->ep[WLAN_AMPDU_TX_EP];
		INIT_LIST_HEAD(&ipa_ctx->sys[i].wait_desc_list);
	}

	ipa_ctx->rx_wq = create_singlethread_workqueue("ipa rx wq");
	if (!ipa_ctx->rx_wq) {
		IPAERR(":fail to create rx wq\n");
		result = -ENOMEM;
		goto fail_rx_wq;
	}

	ipa_ctx->tx_wq = create_singlethread_workqueue("ipa tx wq");
	if (!ipa_ctx->tx_wq) {
		IPAERR(":fail to create tx wq\n");
		result = -ENOMEM;
		goto fail_tx_wq;
	}

	ipa_ctx->hdr_hdl_tree = RB_ROOT;
	ipa_ctx->rt_rule_hdl_tree = RB_ROOT;
	ipa_ctx->rt_tbl_hdl_tree = RB_ROOT;
	ipa_ctx->flt_rule_hdl_tree = RB_ROOT;

	atomic_set(&ipa_ctx->ipa_active_clients, 0);

	result = ipa_bridge_init();
	if (result) {
		IPAERR("ipa bridge init err.\n");
		result = -ENODEV;
		goto fail_bridge_init;
	}

	/* setup the A5-IPA pipes */
	if (ipa_setup_a5_pipes()) {
		IPAERR(":failed to setup IPA-A5 pipes.\n");
		result = -ENODEV;
		goto fail_a5_pipes;
	}

	ipa_replenish_rx_cache();

	/* init the filtering block */
	ipa_commit_flt(IPA_IP_v4);
	ipa_commit_flt(IPA_IP_v6);

	/*
	 * setup an empty routing table in system memory, this will be used
	 * to delete a routing table cleanly and safely
	 */
	ipa_ctx->empty_rt_tbl_mem.size = IPA_ROUTING_RULE_BYTE_SIZE;

	ipa_ctx->empty_rt_tbl_mem.base =
		dma_alloc_coherent(NULL, ipa_ctx->empty_rt_tbl_mem.size,
				    &ipa_ctx->empty_rt_tbl_mem.phys_base,
				    GFP_KERNEL);
	if (!ipa_ctx->empty_rt_tbl_mem.base) {
		IPAERR("DMA buff alloc fail %d bytes for empty routing tbl\n",
				ipa_ctx->empty_rt_tbl_mem.size);
		result = -ENOMEM;
		goto fail_empty_rt_tbl;
	}
	memset(ipa_ctx->empty_rt_tbl_mem.base, 0,
			ipa_ctx->empty_rt_tbl_mem.size);

	/* setup the IPA pipe mem pool */
	ipa_pipe_mem_init(resource_p->ipa_pipe_mem_start_ofst,
			resource_p->ipa_pipe_mem_size);

	ipa_ctx->class = class_create(THIS_MODULE, DRV_NAME);

	result = alloc_chrdev_region(&ipa_ctx->dev_num, 0, 1, DRV_NAME);
	if (result) {
		IPAERR("alloc_chrdev_region err.\n");
		result = -ENODEV;
		goto fail_alloc_chrdev_region;
	}

	ipa_ctx->dev = device_create(ipa_ctx->class, NULL, ipa_ctx->dev_num,
			ipa_ctx, DRV_NAME);
	if (IS_ERR(ipa_ctx->dev)) {
		IPAERR(":device_create err.\n");
		result = -ENODEV;
		goto fail_device_create;
	}

	cdev_init(&ipa_ctx->cdev, &ipa_drv_fops);
	ipa_ctx->cdev.owner = THIS_MODULE;
	ipa_ctx->cdev.ops = &ipa_drv_fops;  /* from LDD3 */

	result = cdev_add(&ipa_ctx->cdev, ipa_ctx->dev_num, 1);
	if (result) {
		IPAERR(":cdev_add err=%d\n", -result);
		result = -ENODEV;
		goto fail_cdev_add;
	}

	/* default aggregation parameters */
	ipa_ctx->aggregation_type = IPA_MBIM_16;
	ipa_ctx->aggregation_byte_limit = 1;
	ipa_ctx->aggregation_time_limit = 0;
	IPADBG(":IPA driver init OK.\n");

	/* gate IPA clocks */
	ipa_disable_clks();

	return 0;

fail_cdev_add:
	device_destroy(ipa_ctx->class, ipa_ctx->dev_num);
fail_device_create:
	unregister_chrdev_region(ipa_ctx->dev_num, 1);
fail_alloc_chrdev_region:
	if (ipa_ctx->pipe_mem_pool)
		gen_pool_destroy(ipa_ctx->pipe_mem_pool);
	dma_free_coherent(NULL,
			  ipa_ctx->empty_rt_tbl_mem.size,
			  ipa_ctx->empty_rt_tbl_mem.base,
			  ipa_ctx->empty_rt_tbl_mem.phys_base);
fail_empty_rt_tbl:
	ipa_cleanup_rx();
	ipa_teardown_a5_pipes();
fail_a5_pipes:
	ipa_bridge_cleanup();
fail_bridge_init:
	destroy_workqueue(ipa_ctx->tx_wq);
fail_tx_wq:
	destroy_workqueue(ipa_ctx->rx_wq);
fail_rx_wq:
	dma_pool_destroy(ipa_ctx->one_kb_no_straddle_pool);
fail_dma_pool:
	kmem_cache_destroy(ipa_ctx->tree_node_cache);
fail_tree_node_cache:
	kmem_cache_destroy(ipa_ctx->rx_pkt_wrapper_cache);
fail_rx_pkt_wrapper_cache:
	kmem_cache_destroy(ipa_ctx->tx_pkt_wrapper_cache);
fail_tx_pkt_wrapper_cache:
	kmem_cache_destroy(ipa_ctx->rt_tbl_cache);
fail_rt_tbl_cache:
	kmem_cache_destroy(ipa_ctx->hdr_offset_cache);
fail_hdr_offset_cache:
	kmem_cache_destroy(ipa_ctx->hdr_cache);
fail_hdr_cache:
	kmem_cache_destroy(ipa_ctx->rt_rule_cache);
fail_rt_rule_cache:
	kmem_cache_destroy(ipa_ctx->flt_rule_cache);
fail_flt_rule_cache:
	sps_deregister_bam_device(ipa_ctx->bam_handle);
fail_bam_register:
	iounmap(bam_props.virt_addr);
fail_bam_remap:
fail_init_hw:
	iounmap(ipa_ctx->mmio);
fail_remap:
	kfree(ipa_ctx);
	ipa_ctx = NULL;
fail_mem:
	/* gate IPA clocks */
	ipa_disable_clks();
	return result;
}

static int ipa_plat_drv_probe(struct platform_device *pdev_p)
{
	int result = 0;
	struct resource *resource_p;
	IPADBG("IPA plat drv probe\n");

	/* initialize ipa_res */
	ipa_res.ipa_pipe_mem_start_ofst = IPA_PIPE_MEM_START_OFST;
	ipa_res.ipa_pipe_mem_size = IPA_PIPE_MEM_SIZE;

	result = ipa_load_pipe_connection(pdev_p,
					A2_TO_IPA,
					&ipa_res.a2_to_ipa_pipe);
	if (0 != result)
		IPAERR(":ipa_load_pipe_connection failed!\n");

	result = ipa_load_pipe_connection(pdev_p, IPA_TO_A2,
					  &ipa_res.ipa_to_a2_pipe);
	if (0 != result)
		IPAERR(":ipa_load_pipe_connection failed!\n");

	/* Get IPA wrapper address */
	resource_p = platform_get_resource_byname(pdev_p, IORESOURCE_MEM,
			"ipa-base");

	if (!resource_p) {
		IPAERR(":get resource failed for ipa-base!\n");
		return -ENODEV;
	} else {
		ipa_res.ipa_mem_base = resource_p->start;
		ipa_res.ipa_mem_size = resource_size(resource_p);
	}

	/* Get IPA BAM address */
	resource_p = platform_get_resource_byname(pdev_p, IORESOURCE_MEM,
			"bam-base");

	if (!resource_p) {
		IPAERR(":get resource failed for bam-base!\n");
		return -ENODEV;
	} else {
		ipa_res.bam_mem_base = resource_p->start;
		ipa_res.bam_mem_size = resource_size(resource_p);
	}

	/* Get IPA pipe mem start ofst */
	resource_p = platform_get_resource_byname(pdev_p, IORESOURCE_MEM,
			"ipa-pipe-mem");

	if (!resource_p) {
		IPADBG(":get resource failed for ipa-pipe-mem\n");
	} else {
		ipa_res.ipa_pipe_mem_start_ofst = resource_p->start;
		ipa_res.ipa_pipe_mem_size = resource_size(resource_p);
	}

	/* Get IPA IRQ number */
	resource_p = platform_get_resource_byname(pdev_p, IORESOURCE_IRQ,
			"ipa-irq");

	if (!resource_p) {
		IPAERR(":get resource failed for ipa-irq!\n");
		return -ENODEV;
	} else {
		ipa_res.ipa_irq = resource_p->start;
	}

	/* Get IPA BAM IRQ number */
	resource_p = platform_get_resource_byname(pdev_p, IORESOURCE_IRQ,
			"bam-irq");

	if (!resource_p) {
		IPAERR(":get resource failed for bam-irq!\n");
		return -ENODEV;
	} else {
		ipa_res.bam_irq = resource_p->start;
	}

	IPADBG(":ipa_mem_base = 0x%x, ipa_mem_size = 0x%x\n",
	       ipa_res.ipa_mem_base, ipa_res.ipa_mem_size);
	IPADBG(":bam_mem_base = 0x%x, bam_mem_size = 0x%x\n",
	       ipa_res.bam_mem_base, ipa_res.bam_mem_size);
	IPADBG(":pipe_mem_start_ofst = 0x%x, pipe_mem_size = 0x%x\n",
	       ipa_res.ipa_pipe_mem_start_ofst, ipa_res.ipa_pipe_mem_size);

	IPADBG(":ipa_irq = %d\n", ipa_res.ipa_irq);
	IPADBG(":bam_irq = %d\n", ipa_res.bam_irq);

	/* stash the IPA dev ptr */
	ipa_dev = &pdev_p->dev;

	/* get IPA clocks */
	if (ipa_get_clks(ipa_dev) != 0)
		return -ENODEV;

	/* enable IPA clocks */
	ipa_enable_clks();

	/* Proceed to real initialization */
	result = ipa_init(&ipa_res);
	if (result)
		IPAERR("ipa_init failed\n");

	result = device_create_file(&pdev_p->dev,
			&dev_attr_aggregation_type);
	if (result)
		IPAERR("failed to create device file\n");

	result = device_create_file(&pdev_p->dev,
			&dev_attr_aggregation_byte_limit);
	if (result)
		IPAERR("failed to create device file\n");

	result = device_create_file(&pdev_p->dev,
			&dev_attr_aggregation_time_limit);
	if (result)
		IPAERR("failed to create device file\n");

	return result;
}

static struct platform_driver ipa_plat_drv = {
	.probe = ipa_plat_drv_probe,
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ipa_plat_drv_match,
	},
};

static int ipa_plat_drv_init(void)
{
	return platform_driver_register(&ipa_plat_drv);
}

struct ipa_context *ipa_get_ctx(void)
{
	return ipa_ctx;
}

static int __init ipa_module_init(void)
{
	int result = 0;

	IPADBG("IPA module init\n");
	ipa_debugfs_init();
	/* Register as a platform device driver */
	result = ipa_plat_drv_init();

	return result;
}

late_initcall(ipa_module_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("IPA HW device driver");

