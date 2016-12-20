/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/usb/msm_hsusb.h>
#include <mach/usb_bam.h>
#include <mach/sps.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <mach/msm_smsm.h>

#define USB_SUMMING_THRESHOLD 512
#define CONNECTIONS_NUM	4

static struct sps_bam_props usb_props;
static struct sps_pipe *sps_pipes[CONNECTIONS_NUM][2];
static struct sps_connect sps_connections[CONNECTIONS_NUM][2];
static struct sps_mem_buffer data_mem_buf[CONNECTIONS_NUM][2];
static struct sps_mem_buffer desc_mem_buf[CONNECTIONS_NUM][2];
static struct platform_device *usb_bam_pdev;
static struct workqueue_struct *usb_bam_wq;
static u32 h_bam;
static spinlock_t usb_bam_lock;

struct usb_bam_event_info {
	struct sps_register_event event;
	int (*callback)(void *);
	void *param;
	struct work_struct event_w;
};

struct usb_bam_connect_info {
	u8 idx;
	u32 *src_pipe;
	u32 *dst_pipe;
	struct usb_bam_event_info wake_event;
	bool enabled;
};

enum usb_bam_sm {
	USB_BAM_SM_INIT = 0,
	USB_BAM_SM_PLUG_NOTIFIED,
	USB_BAM_SM_PLUG_ACKED,
	USB_BAM_SM_UNPLUG_NOTIFIED,
};

struct usb_bam_peer_handhskae_info {
	enum usb_bam_sm state;
	bool client_ready;
	bool ack_received;
	int pending_work;
	struct usb_bam_event_info reset_event;
};

static struct usb_bam_connect_info usb_bam_connections[CONNECTIONS_NUM];
static struct usb_bam_pipe_connect ***msm_usb_bam_connections_info;
static struct usb_bam_pipe_connect *bam_connection_arr;
struct usb_bam_peer_handhskae_info peer_handhskae_info;

static int connect_pipe(u8 conn_idx, enum usb_bam_pipe_dir pipe_dir,
						u32 *usb_pipe_idx)
{
	int ret;
	struct sps_pipe **pipe = &sps_pipes[conn_idx][pipe_dir];
	struct sps_connect *connection =
		&sps_connections[conn_idx][pipe_dir];
	struct msm_usb_bam_platform_data *pdata =
		usb_bam_pdev->dev.platform_data;
	struct usb_bam_pipe_connect *pipe_connection =
				&msm_usb_bam_connections_info
				[pdata->usb_active_bam][conn_idx][pipe_dir];

	*pipe = sps_alloc_endpoint();
	if (*pipe == NULL) {
		pr_err("%s: sps_alloc_endpoint failed\n", __func__);
		return -ENOMEM;
	}

	ret = sps_get_config(*pipe, connection);
	if (ret) {
		pr_err("%s: tx get config failed %d\n", __func__, ret);
		goto get_config_failed;
	}

	ret = sps_phy2h(pipe_connection->src_phy_addr, &(connection->source));
	if (ret) {
		pr_err("%s: sps_phy2h failed (src BAM) %d\n", __func__, ret);
		goto get_config_failed;
	}

	connection->src_pipe_index = pipe_connection->src_pipe_index;
	ret = sps_phy2h(pipe_connection->dst_phy_addr,
					&(connection->destination));
	if (ret) {
		pr_err("%s: sps_phy2h failed (dst BAM) %d\n", __func__, ret);
		goto get_config_failed;
	}
	connection->dest_pipe_index = pipe_connection->dst_pipe_index;

	if (pipe_dir == USB_TO_PEER_PERIPHERAL) {
		connection->mode = SPS_MODE_SRC;
		*usb_pipe_idx = connection->src_pipe_index;
	} else {
		connection->mode = SPS_MODE_DEST;
		*usb_pipe_idx = connection->dest_pipe_index;
	}

	/* If BAM is using dedicated SPS pipe memory, get it */
	if (pipe_connection->mem_type == SPS_PIPE_MEM) {
		pr_debug("%s: USB BAM using SPS pipe memory\n", __func__);
		ret = sps_setup_bam2bam_fifo(
				&data_mem_buf[conn_idx][pipe_dir],
				pipe_connection->data_fifo_base_offset,
				pipe_connection->data_fifo_size, 1);
		if (ret) {
			pr_err("%s: data fifo setup failure %d\n", __func__,
				ret);
			goto fifo_setup_error;
		}

		ret = sps_setup_bam2bam_fifo(
				&desc_mem_buf[conn_idx][pipe_dir],
				pipe_connection->desc_fifo_base_offset,
				pipe_connection->desc_fifo_size, 1);
		if (ret) {
			pr_err("%s: desc. fifo setup failure %d\n", __func__,
				ret);
			goto fifo_setup_error;
		}
	} else if (pipe_connection->mem_type == USB_PRIVATE_MEM) {
		pr_debug("%s: USB BAM using private memory\n", __func__);
		/* BAM is using dedicated USB private memory, map it */
		data_mem_buf[conn_idx][pipe_dir].phys_base =
			pipe_connection->data_fifo_base_offset +
				pdata->usb_base_address;
		data_mem_buf[conn_idx][pipe_dir].size =
			pipe_connection->data_fifo_size;
		data_mem_buf[conn_idx][pipe_dir].base =
			ioremap(data_mem_buf[conn_idx][pipe_dir].phys_base,
				data_mem_buf[conn_idx][pipe_dir].size);
		memset(data_mem_buf[conn_idx][pipe_dir].base, 0,
			data_mem_buf[conn_idx][pipe_dir].size);

		desc_mem_buf[conn_idx][pipe_dir].phys_base =
			pipe_connection->desc_fifo_base_offset +
				pdata->usb_base_address;
		desc_mem_buf[conn_idx][pipe_dir].size =
			pipe_connection->desc_fifo_size;
		desc_mem_buf[conn_idx][pipe_dir].base =
			ioremap(desc_mem_buf[conn_idx][pipe_dir].phys_base,
				desc_mem_buf[conn_idx][pipe_dir].size);
		memset(desc_mem_buf[conn_idx][pipe_dir].base, 0,
			desc_mem_buf[conn_idx][pipe_dir].size);
	} else {
		pr_debug("%s: USB BAM using system memory\n", __func__);
		/* BAM would use system memory, allocate FIFOs */
		data_mem_buf[conn_idx][pipe_dir].size =
					pipe_connection->data_fifo_size;
		data_mem_buf[conn_idx][pipe_dir].base =
			dma_alloc_coherent(&usb_bam_pdev->dev,
				    pipe_connection->data_fifo_size,
				    &data_mem_buf[conn_idx][pipe_dir].phys_base,
				    0);
		memset(data_mem_buf[conn_idx][pipe_dir].base, 0,
					pipe_connection->data_fifo_size);

		desc_mem_buf[conn_idx][pipe_dir].size =
					pipe_connection->desc_fifo_size;
		desc_mem_buf[conn_idx][pipe_dir].base =
			dma_alloc_coherent(&usb_bam_pdev->dev,
				    pipe_connection->desc_fifo_size,
				    &desc_mem_buf[conn_idx][pipe_dir].phys_base,
				    0);
		memset(desc_mem_buf[conn_idx][pipe_dir].base, 0,
					pipe_connection->desc_fifo_size);
	}

	connection->data = data_mem_buf[conn_idx][pipe_dir];
	connection->desc = desc_mem_buf[conn_idx][pipe_dir];
	connection->event_thresh = 16;
	connection->options = SPS_O_AUTO_ENABLE;

	ret = sps_connect(*pipe, connection);
	if (ret < 0) {
		pr_err("%s: tx connect error %d\n", __func__, ret);
		goto error;
	}
	return 0;

error:
	sps_disconnect(*pipe);
fifo_setup_error:
get_config_failed:
	sps_free_endpoint(*pipe);
	return ret;
}


static int disconnect_pipe(u8 connection_idx, enum usb_bam_pipe_dir pipe_dir,
						u32 *usb_pipe_idx)
{
	struct msm_usb_bam_platform_data *pdata =
				usb_bam_pdev->dev.platform_data;
	struct usb_bam_pipe_connect *pipe_connection =
			&msm_usb_bam_connections_info
			[pdata->usb_active_bam][connection_idx][pipe_dir];
	struct sps_pipe *pipe = sps_pipes[connection_idx][pipe_dir];
	struct sps_connect *connection =
		&sps_connections[connection_idx][pipe_dir];

	sps_disconnect(pipe);
	sps_free_endpoint(pipe);

	if (pipe_connection->mem_type == SYSTEM_MEM) {
		pr_debug("%s: Freeing system memory used by PIPE\n", __func__);
		dma_free_coherent(&usb_bam_pdev->dev, connection->data.size,
			  connection->data.base, connection->data.phys_base);
		dma_free_coherent(&usb_bam_pdev->dev, connection->desc.size,
			  connection->desc.base, connection->desc.phys_base);
	}

	connection->options &= ~SPS_O_AUTO_ENABLE;
	return 0;
}

int usb_bam_connect(u8 idx, u32 *src_pipe_idx, u32 *dst_pipe_idx)
{
	struct usb_bam_connect_info *connection = &usb_bam_connections[idx];
	int ret;

	if (!usb_bam_pdev) {
		pr_err("%s: usb_bam device not found\n", __func__);
		return -ENODEV;
	}

	if (idx >= CONNECTIONS_NUM) {
		pr_err("%s: Invalid connection index\n",
			__func__);
		return -EINVAL;
	}

	if (connection->enabled) {
		pr_debug("%s: connection %d was already established\n",
			__func__, idx);
		return 0;
	}
	connection->src_pipe = src_pipe_idx;
	connection->dst_pipe = dst_pipe_idx;
	connection->idx = idx;

	if (src_pipe_idx) {
		/* open USB -> Peripheral pipe */
		ret = connect_pipe(connection->idx, USB_TO_PEER_PERIPHERAL,
			connection->src_pipe);
		if (ret) {
			pr_err("%s: src pipe connection failure\n", __func__);
			return ret;
		}
	}
	if (dst_pipe_idx) {
		/* open Peripheral -> USB pipe */
		ret = connect_pipe(connection->idx, PEER_PERIPHERAL_TO_USB,
			connection->dst_pipe);
		if (ret) {
			pr_err("%s: dst pipe connection failure\n", __func__);
			return ret;
		}
	}
	connection->enabled = 1;

	return 0;
}

int usb_bam_client_ready(bool ready)
{
	spin_lock(&usb_bam_lock);
	if (peer_handhskae_info.client_ready == ready) {
		pr_debug("%s: client state is already %d\n",
			__func__, ready);
		spin_unlock(&usb_bam_lock);
		return 0;
	}

	peer_handhskae_info.client_ready = ready;

	spin_unlock(&usb_bam_lock);
	if (!queue_work(usb_bam_wq, &peer_handhskae_info.reset_event.event_w)) {
		spin_lock(&usb_bam_lock);
		peer_handhskae_info.pending_work++;
		spin_unlock(&usb_bam_lock);
	}

	return 0;
}

static void usb_bam_work(struct work_struct *w)
{
	struct usb_bam_event_info *event_info =
		container_of(w, struct usb_bam_event_info, event_w);

	event_info->callback(event_info->param);
}

static void usb_bam_wake_cb(struct sps_event_notify *notify)
{
	struct usb_bam_event_info *wake_event_info =
		(struct usb_bam_event_info *)notify->user;

	queue_work(usb_bam_wq, &wake_event_info->event_w);
}

static void usb_bam_sm_work(struct work_struct *w)
{
	pr_debug("%s: current state: %d\n", __func__,
		peer_handhskae_info.state);

	spin_lock(&usb_bam_lock);

	switch (peer_handhskae_info.state) {
	case USB_BAM_SM_INIT:
		if (peer_handhskae_info.client_ready) {
			spin_unlock(&usb_bam_lock);
			smsm_change_state(SMSM_APPS_STATE, 0,
				SMSM_USB_PLUG_UNPLUG);
			spin_lock(&usb_bam_lock);
			peer_handhskae_info.state = USB_BAM_SM_PLUG_NOTIFIED;
		}
		break;
	case USB_BAM_SM_PLUG_NOTIFIED:
		if (peer_handhskae_info.ack_received) {
			peer_handhskae_info.state = USB_BAM_SM_PLUG_ACKED;
			peer_handhskae_info.ack_received = 0;
		}
		break;
	case USB_BAM_SM_PLUG_ACKED:
		if (!peer_handhskae_info.client_ready) {
			spin_unlock(&usb_bam_lock);
			smsm_change_state(SMSM_APPS_STATE,
				SMSM_USB_PLUG_UNPLUG, 0);
			spin_lock(&usb_bam_lock);
			peer_handhskae_info.state = USB_BAM_SM_UNPLUG_NOTIFIED;
		}
		break;
	case USB_BAM_SM_UNPLUG_NOTIFIED:
		if (peer_handhskae_info.ack_received) {
			spin_unlock(&usb_bam_lock);
			peer_handhskae_info.reset_event.
				callback(peer_handhskae_info.reset_event.param);
			spin_lock(&usb_bam_lock);
			peer_handhskae_info.state = USB_BAM_SM_INIT;
			peer_handhskae_info.ack_received = 0;
		}
		break;
	}

	if (peer_handhskae_info.pending_work) {
		peer_handhskae_info.pending_work--;
		spin_unlock(&usb_bam_lock);
		queue_work(usb_bam_wq,
			&peer_handhskae_info.reset_event.event_w);
		spin_lock(&usb_bam_lock);
	}
	spin_unlock(&usb_bam_lock);
}

static void usb_bam_ack_toggle_cb(void *priv, uint32_t old_state,
	uint32_t new_state)
{
	static int last_processed_state;
	int current_state;

	spin_lock(&usb_bam_lock);

	current_state = new_state & SMSM_USB_PLUG_UNPLUG;

	if (current_state == last_processed_state) {
		spin_unlock(&usb_bam_lock);
		return;
	}

	last_processed_state = current_state;
	peer_handhskae_info.ack_received = true;

	spin_unlock(&usb_bam_lock);
	if (!queue_work(usb_bam_wq, &peer_handhskae_info.reset_event.event_w)) {
		spin_lock(&usb_bam_lock);
		peer_handhskae_info.pending_work++;
		spin_unlock(&usb_bam_lock);
	}
}

int usb_bam_register_wake_cb(u8 idx,
	int (*callback)(void *user), void* param)
{
	struct sps_pipe *pipe = sps_pipes[idx][PEER_PERIPHERAL_TO_USB];
	struct sps_connect *sps_connection =
		&sps_connections[idx][PEER_PERIPHERAL_TO_USB];
	struct usb_bam_connect_info *connection = &usb_bam_connections[idx];
	struct usb_bam_event_info *wake_event_info =
		&connection->wake_event;
	int ret;

	wake_event_info->param = param;
	wake_event_info->callback = callback;
	wake_event_info->event.mode = SPS_TRIGGER_CALLBACK;
	wake_event_info->event.xfer_done = NULL;
	wake_event_info->event.callback = callback ? usb_bam_wake_cb : NULL;
	wake_event_info->event.user = wake_event_info;
	wake_event_info->event.options = SPS_O_WAKEUP;
	ret = sps_register_event(pipe, &wake_event_info->event);
	if (ret) {
		pr_err("%s: sps_register_event() failed %d\n", __func__, ret);
		return ret;
	}

	sps_connection->options = callback ?
		(SPS_O_AUTO_ENABLE | SPS_O_WAKEUP | SPS_O_WAKEUP_IS_ONESHOT) :
			SPS_O_AUTO_ENABLE;
	ret = sps_set_config(pipe, sps_connection);
	if (ret) {
		pr_err("%s: sps_set_config() failed %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int usb_bam_register_peer_reset_cb(u8 idx,
	 int (*callback)(void *), void *param)
{
	u32 ret = 0;

	if (callback) {
		peer_handhskae_info.reset_event.param = param;
		peer_handhskae_info.reset_event.callback = callback;

		ret = smsm_state_cb_register(SMSM_MODEM_STATE,
			SMSM_USB_PLUG_UNPLUG, usb_bam_ack_toggle_cb, NULL);
		if (ret) {
			pr_err("%s: failed to register SMSM callback\n",
				__func__);
		} else {
			if (smsm_get_state(SMSM_MODEM_STATE) &
				SMSM_USB_PLUG_UNPLUG)
				usb_bam_ack_toggle_cb(NULL, 0,
					SMSM_USB_PLUG_UNPLUG);
		}
	} else {
		peer_handhskae_info.reset_event.param = NULL;
		peer_handhskae_info.reset_event.callback = NULL;
		smsm_state_cb_deregister(SMSM_MODEM_STATE,
			SMSM_USB_PLUG_UNPLUG, usb_bam_ack_toggle_cb, NULL);
	}

	return ret;
}

int usb_bam_disconnect_pipe(u8 idx)
{
	struct usb_bam_connect_info *connection = &usb_bam_connections[idx];
	int ret;

	if (idx >= CONNECTIONS_NUM) {
		pr_err("%s: Invalid connection index\n",
			__func__);
		return -EINVAL;
	}

	if (!connection->enabled) {
		pr_debug("%s: connection %d isn't enabled\n",
			__func__, idx);
		return 0;
	}

	if (connection->src_pipe) {
		/* close USB -> Peripheral pipe */
		ret = disconnect_pipe(connection->idx, USB_TO_PEER_PERIPHERAL,
						   connection->src_pipe);
		if (ret) {
			pr_err("%s: src pipe connection failure\n", __func__);
			return ret;
		}

	}
	if (connection->dst_pipe) {
		/* close Peripheral -> USB pipe */
		ret = disconnect_pipe(connection->idx, PEER_PERIPHERAL_TO_USB,
			connection->dst_pipe);
		if (ret) {
			pr_err("%s: dst pipe connection failure\n", __func__);
			return ret;
		}
	}

	connection->src_pipe = 0;
	connection->dst_pipe = 0;
	connection->enabled = 0;

	return 0;
}

int usb_bam_reset(void)
{
	struct usb_bam_connect_info *connection;
	int i;
	int ret = 0, ret_int;
	bool reconnect[CONNECTIONS_NUM];
	u32 *reconnect_src_pipe[CONNECTIONS_NUM];
	u32 *reconnect_dst_pipe[CONNECTIONS_NUM];

	/* Disconnect all pipes */
	for (i = 0; i < CONNECTIONS_NUM; i++) {
		connection = &usb_bam_connections[i];
		reconnect[i] = connection->src_enabled ||
			connection->dst_enabled;
		reconnect_src_pipe[i] = connection->src_pipe;
		reconnect_dst_pipe[i] = connection->dst_pipe;

		ret_int = usb_bam_disconnect_pipe(i);
		if (ret_int) {
			pr_err("%s: failure to connect pipe %d\n",
				__func__, i);
			ret = ret_int;
			continue;
		}
	}

	/* Reset USB/HSIC BAM */
	if (sps_device_reset(h_bam))
		pr_err("%s: BAM reset failed\n", __func__);

	/* Reconnect all pipes */
	for (i = 0; i < CONNECTIONS_NUM; i++) {
		connection = &usb_bam_connections[i];
		if (reconnect[i]) {
			ret_int = usb_bam_connect(i, reconnect_src_pipe[i],
				reconnect_dst_pipe[i]);
			if (ret_int) {
				pr_err("%s: failure to reconnect pipe %d\n",
					__func__, i);
				ret = ret_int;
				continue;
			}
		}
	}

	return ret;
}

static int update_connections_info(struct device_node *node, int bam,
	int conn_num, int dir, enum usb_pipe_mem_type mem_type)
{
	u32 rc;
	char *key = NULL;
	uint32_t val = 0;

	struct usb_bam_pipe_connect *pipe_connection;

	pipe_connection = &msm_usb_bam_connections_info[bam][conn_num][dir];

	pipe_connection->mem_type = mem_type;

	key = "qcom,src-bam-physical-address";
	rc = of_property_read_u32(node, key, &val);
	if (rc)
		goto err;
	pipe_connection->src_phy_addr = val;

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
	pr_err("%s: Error in name %s key %s\n", __func__,
		node->full_name, key);
	return -EFAULT;
}

static int usb_bam_update_conn_array_index(struct platform_device *pdev,
		void *buff, int bam_max, int conn_max, int pipe_dirs)
{
	int bam_num, conn_num;
	struct usb_bam_pipe_connect *bam_connection_arr = buff;

	msm_usb_bam_connections_info = devm_kzalloc(&pdev->dev,
		bam_max * sizeof(struct usb_bam_pipe_connect **),
		GFP_KERNEL);

	if (!msm_usb_bam_connections_info)
		return -ENOMEM;

	for (bam_num = 0; bam_num < bam_max; bam_num++) {
		msm_usb_bam_connections_info[bam_num] =
			devm_kzalloc(&pdev->dev, conn_max *
			sizeof(struct usb_bam_pipe_connect *),
			GFP_KERNEL);
		if (!msm_usb_bam_connections_info[bam_num])
			return -ENOMEM;

		for (conn_num = 0; conn_num < conn_max; conn_num++)
			msm_usb_bam_connections_info[bam_num][conn_num] =
				bam_connection_arr +
				(bam_num * conn_max * pipe_dirs) +
				(conn_num * pipe_dirs);
	}

	return 0;
}

static struct msm_usb_bam_platform_data *usb_bam_dt_to_pdata(
	struct platform_device *pdev)
{
	struct msm_usb_bam_platform_data *pdata;
	struct device_node *node = pdev->dev.of_node;
	int conn_num, bam;
	u8 dir;
	u8 ncolumns = 2;
	int bam_amount, rc = 0;
	u32 pipe_entry = 0;
	char *key = NULL;
	enum usb_pipe_mem_type mem_type;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("unable to allocate platform data\n");
		return NULL;
	}

	rc = of_property_read_u32(node, "qcom,usb-active-bam",
		&pdata->usb_active_bam);
	if (rc) {
		pr_err("Invalid usb active bam property\n");
		return NULL;
	}

	rc = of_property_read_u32(node, "qcom,usb-total-bam-num",
		&pdata->total_bam_num);
	if (rc) {
		pr_err("Invalid usb total bam num property\n");
		return NULL;
	}

	rc = of_property_read_u32(node, "qcom,usb-bam-num-pipes",
		&pdata->usb_bam_num_pipes);
	if (rc) {
		pr_err("Invalid usb bam num pipes property\n");
		return NULL;
	}

	rc = of_property_read_u32(node, "qcom,usb-base-address",
		&pdata->usb_base_address);
	if (rc) {
		pr_err("Invalid usb base address property\n");
		return NULL;
	}

	pdata->ignore_core_reset_ack = of_property_read_bool(node,
					"qcom,ignore-core-reset-ack");

	for_each_child_of_node(pdev->dev.of_node, node)
		pipe_entry++;

	/*
	 * we need to know the number of connection, so we will know
	 * how much memory to allocate
	 */
	conn_num = pipe_entry / 2;
	bam_amount = pdata->total_bam_num;

	if (conn_num <= 0 || conn_num >= pdata->usb_bam_num_pipes)
		goto err;


	/* alloc msm_usb_bam_connections_info */
	bam_connection_arr = devm_kzalloc(&pdev->dev, bam_amount *
		conn_num * ncolumns *
		sizeof(struct usb_bam_pipe_connect), GFP_KERNEL);

	if (!bam_connection_arr)
		goto err;

	rc = usb_bam_update_conn_array_index(pdev, bam_connection_arr,
					bam_amount, conn_num, ncolumns);
	if (rc)
		goto err;

	/* retrieve device tree parameters */
	for_each_child_of_node(pdev->dev.of_node, node) {
		const char *str;

		key = "qcom,usb-bam-type";
		rc = of_property_read_u32(node, key, &bam);
		if (rc)
			goto err;

		key = "qcom,usb-bam-mem-type";
		rc = of_property_read_u32(node, key, &mem_type);
		if (rc)
			goto err;

		rc = of_property_read_string(node, "label", &str);
		if (rc) {
			pr_err("Cannot read string\n");
			goto err;
		}

		if (strstr(str, "usb-to-peri"))
			dir = USB_TO_PEER_PERIPHERAL;
		else if (strstr(str, "peri-to-usb"))
			dir = PEER_PERIPHERAL_TO_USB;
		else
			goto err;

		/* Check if connection type is suported */
		if (!strcmp(str, "usb-to-peri-qdss-dwc3") ||
			!strcmp(str, "peri-to-usb-qdss-dwc3") ||
			!strcmp(str, "usb-to-peri-qdss-hsusb") ||
			!strcmp(str, "peri-to-usb-qdss-hsusb"))
				conn_num = 0;
		else
			goto err;

		rc = update_connections_info(node, bam, conn_num,
						dir, mem_type);
		if (rc)
			goto err;
	}

	pdata->connections = &msm_usb_bam_connections_info[0][0][0];

	return pdata;
err:
	pr_err("%s: failed\n", __func__);
	return NULL;
}

static char *bam_enable_strings[3] = {
	[SSUSB_BAM] = "ssusb",
	[HSUSB_BAM] = "hsusb",
	[HSIC_BAM]  = "hsic",
};

static int usb_bam_init(void)
{
	int ret;
	void *usb_virt_addr;
	struct msm_usb_bam_platform_data *pdata =
		usb_bam_pdev->dev.platform_data;
	struct resource *res;
	int irq;

	res = platform_get_resource_byname(usb_bam_pdev, IORESOURCE_MEM,
				bam_enable_strings[pdata->usb_active_bam]);
	if (!res) {
		dev_err(&usb_bam_pdev->dev, "Unable to get memory resource\n");
		return -ENODEV;
	}

	irq = platform_get_irq_byname(usb_bam_pdev,
				bam_enable_strings[pdata->usb_active_bam]);
	if (irq < 0) {
		dev_err(&usb_bam_pdev->dev, "Unable to get IRQ resource\n");
		return irq;
	}

	usb_virt_addr = ioremap(res->start, resource_size(res));
	if (!usb_virt_addr) {
		pr_err("%s: ioremap failed\n", __func__);
		return -ENOMEM;
	}
	usb_props.phys_addr = res->start;
	usb_props.virt_addr = usb_virt_addr;
	usb_props.virt_size = resource_size(res);
	usb_props.irq = irq;
	usb_props.summing_threshold = USB_SUMMING_THRESHOLD;
	usb_props.event_threshold = 512;
	usb_props.num_pipes = pdata->usb_bam_num_pipes;
	/*
	 * HSUSB and HSIC Cores don't support RESET ACK signal to BAMs
	 * Hence, let BAM to ignore acknowledge from USB while resetting PIPE
	 */
	if (pdata->ignore_core_reset_ack && pdata->usb_active_bam != SSUSB_BAM)
		usb_props.options = SPS_BAM_NO_EXT_P_RST;

	ret = sps_register_bam_device(&usb_props, &h_bam);
	if (ret < 0) {
		pr_err("%s: register bam error %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}

static ssize_t
usb_bam_show_enable(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	struct msm_usb_bam_platform_data *pdata = dev->platform_data;

	if (!pdata)
		return 0;
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 bam_enable_strings[pdata->usb_active_bam]);
}

static ssize_t usb_bam_store_enable(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct msm_usb_bam_platform_data *pdata = dev->platform_data;
	char str[10], *pstr;
	int ret, i;

	if (!pdata) {
		dev_err(dev, "no usb_bam pdata found\n");
		return -ENODEV;
	}

	strlcpy(str, buf, sizeof(str));
	pstr = strim(str);

	for (i = 0; i < ARRAY_SIZE(bam_enable_strings); i++) {
		if (!strncmp(pstr, bam_enable_strings[i], sizeof(str)))
			pdata->usb_active_bam = i;
	}

	dev_dbg(dev, "active_bam=%s\n",
		bam_enable_strings[pdata->usb_active_bam]);

	ret = usb_bam_init();
	if (ret) {
		dev_err(dev, "failed to initialize usb bam\n");
		return ret;
	}

	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUSR, usb_bam_show_enable,
		   usb_bam_store_enable);

static int usb_bam_probe(struct platform_device *pdev)
{
	int ret, i;
	struct msm_usb_bam_platform_data *pdata;

	dev_dbg(&pdev->dev, "usb_bam_probe\n");

	for (i = 0; i < CONNECTIONS_NUM; i++) {
		usb_bam_connections[i].enabled = 0;
		INIT_WORK(&usb_bam_connections[i].wake_event.event_w,
			usb_bam_work);
	}

	spin_lock_init(&usb_bam_lock);
	INIT_WORK(&peer_handhskae_info.reset_event.event_w, usb_bam_sm_work);

	if (pdev->dev.of_node) {
		dev_dbg(&pdev->dev, "device tree enabled\n");
		pdata = usb_bam_dt_to_pdata(pdev);
		if (!pdata)
			return -ENOMEM;
		pdev->dev.platform_data = pdata;
	} else if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "missing platform_data\n");
		return -ENODEV;
	} else {
		pdata = pdev->dev.platform_data;
		ret = usb_bam_update_conn_array_index(pdev, pdata->connections,
				MAX_BAMS, CONNECTIONS_NUM, 2);
		if (ret) {
			pr_err("usb_bam_update_conn_array_index failed\n");
			return ret;
		}
	}
	usb_bam_pdev = pdev;

	ret = device_create_file(&pdev->dev, &dev_attr_enable);
	if (ret)
		dev_err(&pdev->dev, "failed to create device file\n");

	usb_bam_wq = alloc_workqueue("usb_bam_wq",
		WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!usb_bam_wq) {
		pr_err("unable to create workqueue usb_bam_wq\n");
		return -ENOMEM;
	}

	return ret;
}

void get_bam2bam_connection_info(u8 conn_idx, enum usb_bam_pipe_dir pipe_dir,
	u32 *usb_bam_handle, u32 *usb_bam_pipe_idx, u32 *peer_pipe_idx,
	struct sps_mem_buffer *desc_fifo, struct sps_mem_buffer *data_fifo)
{
	struct sps_connect *connection =
		&sps_connections[conn_idx][pipe_dir];


	if (pipe_dir == USB_TO_PEER_PERIPHERAL) {
		*usb_bam_handle = connection->source;
		*usb_bam_pipe_idx = connection->src_pipe_index;
		*peer_pipe_idx = connection->dest_pipe_index;
	} else {
		*usb_bam_handle = connection->destination;
		*usb_bam_pipe_idx = connection->dest_pipe_index;
		*peer_pipe_idx = connection->src_pipe_index;
	}
	if (data_fifo)
		memcpy(data_fifo, &data_mem_buf[conn_idx][pipe_dir],
			sizeof(struct sps_mem_buffer));
	if (desc_fifo)
		memcpy(desc_fifo, &desc_mem_buf[conn_idx][pipe_dir],
			sizeof(struct sps_mem_buffer));
}
EXPORT_SYMBOL(get_bam2bam_connection_info);

static int usb_bam_remove(struct platform_device *pdev)
{
	destroy_workqueue(usb_bam_wq);

	return 0;
}

static const struct of_device_id usb_bam_dt_match[] = {
	{ .compatible = "qcom,usb-bam-msm",
	},
	{}
};
MODULE_DEVICE_TABLE(of, usb_bam_dt_match);

static struct platform_driver usb_bam_driver = {
	.probe = usb_bam_probe,
	.remove = usb_bam_remove,
	.driver		= {
		.name	= "usb_bam",
		.of_match_table = usb_bam_dt_match,
	},
};

static int __init init(void)
{
	return platform_driver_register(&usb_bam_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	platform_driver_unregister(&usb_bam_driver);
}
module_exit(cleanup);

MODULE_DESCRIPTION("MSM USB BAM DRIVER");
MODULE_LICENSE("GPL v2");
