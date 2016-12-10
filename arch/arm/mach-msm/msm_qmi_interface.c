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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/socket.h>
#include <linux/gfp.h>
#include <linux/qmi_encdec.h>

#include <mach/msm_qmi_interface.h>
#include <mach/msm_ipc_router.h>

#include "msm_qmi_interface_priv.h"

static LIST_HEAD(svc_event_nb_list);
static DEFINE_MUTEX(svc_event_nb_list_lock);

static void qmi_event_notify(unsigned event, void *priv)
{
	struct qmi_handle *handle = (struct qmi_handle *)priv;
	unsigned long flags;

	if (!handle)
		return;

	mutex_lock(&handle->handle_lock);
	if (handle->handle_reset) {
		mutex_unlock(&handle->handle_lock);
		return;
	}

	switch (event) {
	case MSM_IPC_ROUTER_READ_CB:
		spin_lock_irqsave(&handle->notify_lock, flags);
		handle->notify(handle, QMI_RECV_MSG, handle->notify_priv);
		spin_unlock_irqrestore(&handle->notify_lock, flags);
		break;

	default:
		break;
	}
	mutex_unlock(&handle->handle_lock);
}

struct qmi_handle *qmi_handle_create(
	void (*notify)(struct qmi_handle *handle,
		       enum qmi_event_type event, void *notify_priv),
	void *notify_priv)
{
	struct qmi_handle *temp_handle;
	struct msm_ipc_port *port_ptr;

	temp_handle = kzalloc(sizeof(struct qmi_handle), GFP_KERNEL);
	if (!temp_handle) {
		pr_err("%s: Failure allocating client handle\n", __func__);
		return NULL;
	}

	port_ptr = msm_ipc_router_create_port(qmi_event_notify,
					      (void *)temp_handle);
	if (!port_ptr) {
		pr_err("%s: IPC router port creation failed\n", __func__);
		kfree(temp_handle);
		return NULL;
	}

	temp_handle->src_port = port_ptr;
	temp_handle->next_txn_id = 1;
	INIT_LIST_HEAD(&temp_handle->txn_list);
	mutex_init(&temp_handle->handle_lock);
	spin_lock_init(&temp_handle->notify_lock);
	temp_handle->notify = notify;
	temp_handle->notify_priv = notify_priv;
	temp_handle->handle_reset = 0;
	init_waitqueue_head(&temp_handle->reset_waitq);
	return temp_handle;
}
EXPORT_SYMBOL(qmi_handle_create);

static void clean_txn_info(struct qmi_handle *handle)
{
	struct qmi_txn *txn_handle, *temp_txn_handle;

	list_for_each_entry_safe(txn_handle, temp_txn_handle,
				 &handle->txn_list, list) {
		if (txn_handle->type == QMI_ASYNC_TXN) {
			list_del(&txn_handle->list);
			kfree(txn_handle);
		} else if (txn_handle->type == QMI_SYNC_TXN) {
			wake_up(&txn_handle->wait_q);
		}
	}
}

int qmi_handle_destroy(struct qmi_handle *handle)
{
	int rc;

	if (!handle)
		return -EINVAL;

	mutex_lock(&handle->handle_lock);
	handle->handle_reset = 1;
	clean_txn_info(handle);
	mutex_unlock(&handle->handle_lock);

	rc = wait_event_interruptible(handle->reset_waitq,
				      list_empty(&handle->txn_list));

	/* TODO: Destroy client owned transaction */
	msm_ipc_router_close_port((struct msm_ipc_port *)(handle->src_port));
	kfree(handle->dest_info);
	kfree(handle);
	return 0;
}
EXPORT_SYMBOL(qmi_handle_destroy);

int qmi_register_ind_cb(struct qmi_handle *handle,
	void (*ind_cb)(struct qmi_handle *handle,
		       unsigned int msg_id, void *msg,
		       unsigned int msg_len, void *ind_cb_priv),
	void *ind_cb_priv)
{
	if (!handle)
		return -EINVAL;

	mutex_lock(&handle->handle_lock);
	if (handle->handle_reset) {
		mutex_unlock(&handle->handle_lock);
		return -ENETRESET;
	}

	handle->ind_cb = ind_cb;
	handle->ind_cb_priv = ind_cb_priv;
	mutex_unlock(&handle->handle_lock);
	return 0;
}
EXPORT_SYMBOL(qmi_register_ind_cb);

static int qmi_encode_and_send_req(struct qmi_txn **ret_txn_handle,
	struct qmi_handle *handle, enum txn_type type,
	struct msg_desc *req_desc, void *req, unsigned int req_len,
	struct msg_desc *resp_desc, void *resp, unsigned int resp_len,
	void (*resp_cb)(struct qmi_handle *handle,
			unsigned int msg_id, void *msg,
			void *resp_cb_data),
	void *resp_cb_data)
{
	struct qmi_txn *txn_handle;
	int rc, encoded_req_len;
	void *encoded_req;

	if (!handle || !handle->dest_info ||
	    !req_desc || !req || !resp_desc || !resp)
		return -EINVAL;

	mutex_lock(&handle->handle_lock);
	if (handle->handle_reset) {
		mutex_unlock(&handle->handle_lock);
		return -ENETRESET;
	}

	/* Allocate Transaction Info */
	txn_handle = kzalloc(sizeof(struct qmi_txn), GFP_KERNEL);
	if (!txn_handle) {
		pr_err("%s: Failed to allocate txn handle\n", __func__);
		mutex_unlock(&handle->handle_lock);
		return -ENOMEM;
	}
	txn_handle->type = type;
	INIT_LIST_HEAD(&txn_handle->list);
	init_waitqueue_head(&txn_handle->wait_q);

	/* Cache the parameters passed & mark it as sync*/
	txn_handle->handle = handle;
	txn_handle->resp_desc = resp_desc;
	txn_handle->resp = resp;
	txn_handle->resp_len = resp_len;
	txn_handle->resp_received = 0;
	txn_handle->resp_cb = resp_cb;
	txn_handle->resp_cb_data = resp_cb_data;

	/* Encode the request msg */
	encoded_req_len = req_desc->max_msg_len + QMI_HEADER_SIZE;
	encoded_req = kmalloc(encoded_req_len, GFP_KERNEL);
	if (!encoded_req) {
		pr_err("%s: Failed to allocate req_msg_buf\n", __func__);
		rc = -ENOMEM;
		goto encode_and_send_req_err1;
	}
	rc = qmi_kernel_encode(req_desc,
		(void *)(encoded_req + QMI_HEADER_SIZE),
		req_desc->max_msg_len, req);
	if (rc < 0) {
		pr_err("%s: Encode Failure %d\n", __func__, rc);
		goto encode_and_send_req_err2;
	}
	encoded_req_len = rc;

	/* Encode the header & Add to the txn_list */
	if (!handle->next_txn_id)
		handle->next_txn_id++;
	txn_handle->txn_id = handle->next_txn_id++;
	encode_qmi_header(encoded_req, QMI_REQUEST_CONTROL_FLAG,
			  txn_handle->txn_id, req_desc->msg_id,
			  encoded_req_len);
	encoded_req_len += QMI_HEADER_SIZE;
	list_add_tail(&txn_handle->list, &handle->txn_list);

	/* Send the request */
	rc = msm_ipc_router_send_msg((struct msm_ipc_port *)(handle->src_port),
		(struct msm_ipc_addr *)handle->dest_info,
		encoded_req, encoded_req_len);
	if (rc < 0) {
		pr_err("%s: send_msg failed %d\n", __func__, rc);
		goto encode_and_send_req_err3;
	}
	mutex_unlock(&handle->handle_lock);

	kfree(encoded_req);
	if (ret_txn_handle)
		*ret_txn_handle = txn_handle;
	return 0;

encode_and_send_req_err3:
	list_del(&txn_handle->list);
encode_and_send_req_err2:
	kfree(encoded_req);
encode_and_send_req_err1:
	kfree(txn_handle);
	mutex_unlock(&handle->handle_lock);
	return rc;
}

int qmi_send_req_wait(struct qmi_handle *handle,
		      struct msg_desc *req_desc,
		      void *req, unsigned int req_len,
		      struct msg_desc *resp_desc,
		      void *resp, unsigned int resp_len,
		      unsigned long timeout_ms)
{
	struct qmi_txn *txn_handle = NULL;
	int rc;

	/* Encode and send the request */
	rc = qmi_encode_and_send_req(&txn_handle, handle, QMI_SYNC_TXN,
				     req_desc, req, req_len,
				     resp_desc, resp, resp_len,
				     NULL, NULL);
	if (rc < 0) {
		pr_err("%s: Error encode & send req: %d\n", __func__, rc);
		return rc;
	}

	/* Wait for the response */
	if (!timeout_ms) {
		rc = wait_event_interruptible(txn_handle->wait_q,
					(txn_handle->resp_received ||
					 handle->handle_reset));
	} else {
		rc = wait_event_interruptible_timeout(txn_handle->wait_q,
					(txn_handle->resp_received ||
					 handle->handle_reset),
					msecs_to_jiffies(timeout_ms));
		if (rc == 0)
			rc = -ETIMEDOUT;
	}

	mutex_lock(&handle->handle_lock);
	if (!txn_handle->resp_received) {
		pr_err("%s: Response Wait Error %d\n", __func__, rc);
		if (handle->handle_reset)
			rc = -ENETRESET;
		if (rc >= 0)
			rc = -EFAULT;
		goto send_req_wait_err;
	}
	rc = 0;

send_req_wait_err:
	list_del(&txn_handle->list);
	kfree(txn_handle);
	mutex_unlock(&handle->handle_lock);
	wake_up(&handle->reset_waitq);
	return rc;
}
EXPORT_SYMBOL(qmi_send_req_wait);

int qmi_send_req_nowait(struct qmi_handle *handle,
			struct msg_desc *req_desc,
			void *req, unsigned int req_len,
			struct msg_desc *resp_desc,
			void *resp, unsigned int resp_len,
			void (*resp_cb)(struct qmi_handle *handle,
					unsigned int msg_id, void *msg,
					void *resp_cb_data),
			void *resp_cb_data)
{
	return qmi_encode_and_send_req(NULL, handle, QMI_ASYNC_TXN,
				       req_desc, req, req_len,
				       resp_desc, resp, resp_len,
				       resp_cb, resp_cb_data);
}
EXPORT_SYMBOL(qmi_send_req_nowait);

static struct qmi_txn *find_txn_handle(struct qmi_handle *handle,
				       uint16_t txn_id)
{
	struct qmi_txn *txn_handle;

	list_for_each_entry(txn_handle, &handle->txn_list, list) {
		if (txn_handle->txn_id == txn_id)
			return txn_handle;
	}
	return NULL;
}

static int handle_qmi_response(struct qmi_handle *handle,
			       unsigned char *resp_msg, uint16_t txn_id,
			       uint16_t msg_id, uint16_t msg_len)
{
	struct qmi_txn *txn_handle;
	int rc;

	/* Find the transaction handle */
	txn_handle = find_txn_handle(handle, txn_id);
	if (!txn_handle) {
		pr_err("%s Response received for non-existent txn_id %d\n",
			__func__, txn_id);
		return -EINVAL;
	}

	/* Decode the message */
	rc = qmi_kernel_decode(txn_handle->resp_desc, txn_handle->resp,
			       (void *)(resp_msg + QMI_HEADER_SIZE), msg_len);
	if (rc < 0) {
		pr_err("%s: Response Decode Failure <%d: %d: %d> rc: %d\n",
			__func__, txn_id, msg_id, msg_len, rc);
		wake_up(&txn_handle->wait_q);
		if (txn_handle->type == QMI_ASYNC_TXN) {
			list_del(&txn_handle->list);
			kfree(txn_handle);
		}
		return rc;
	}

	/* Handle async or sync resp */
	switch (txn_handle->type) {
	case QMI_SYNC_TXN:
		txn_handle->resp_received = 1;
		wake_up(&txn_handle->wait_q);
		rc = 0;
		break;

	case QMI_ASYNC_TXN:
		if (txn_handle->resp_cb)
			txn_handle->resp_cb(txn_handle->handle, msg_id,
					    txn_handle->resp,
					    txn_handle->resp_cb_data);
		list_del(&txn_handle->list);
		kfree(txn_handle);
		rc = 0;
		break;

	default:
		pr_err("%s: Unrecognized transaction type\n", __func__);
		return -EFAULT;
	}
	return rc;
}

static int handle_qmi_indication(struct qmi_handle *handle, void *msg,
				 unsigned int msg_id, unsigned int msg_len)
{
	if (handle->ind_cb)
		handle->ind_cb(handle, msg_id, msg,
				msg_len, handle->ind_cb_priv);
	return 0;
}

int qmi_recv_msg(struct qmi_handle *handle)
{
	unsigned int recv_msg_len;
	unsigned char *recv_msg = NULL;
	struct msm_ipc_addr src_addr;
	unsigned char cntl_flag;
	uint16_t txn_id, msg_id, msg_len;
	int rc;

	if (!handle)
		return -EINVAL;

	mutex_lock(&handle->handle_lock);
	if (handle->handle_reset) {
		mutex_unlock(&handle->handle_lock);
		return -ENETRESET;
	}

	/* Read the messages */
	rc = msm_ipc_router_read_msg((struct msm_ipc_port *)(handle->src_port),
				     &src_addr, &recv_msg, &recv_msg_len);
	if (rc < 0) {
		pr_err("%s: Read failed %d\n", __func__, rc);
		mutex_unlock(&handle->handle_lock);
		return rc;
	}

	/* Decode the header & Handle the req, resp, indication message */
	decode_qmi_header(recv_msg, &cntl_flag, &txn_id, &msg_id, &msg_len);

	switch (cntl_flag) {
	case QMI_RESPONSE_CONTROL_FLAG:
		rc = handle_qmi_response(handle, recv_msg,
					 txn_id, msg_id, msg_len);
		break;

	case QMI_INDICATION_CONTROL_FLAG:
		rc = handle_qmi_indication(handle, recv_msg, msg_id, msg_len);
		break;

	default:
		rc = -EFAULT;
		pr_err("%s: Unsupported message type %d\n",
			__func__, cntl_flag);
		break;
	}
	kfree(recv_msg);
	mutex_unlock(&handle->handle_lock);
	return rc;
}
EXPORT_SYMBOL(qmi_recv_msg);

int qmi_connect_to_service(struct qmi_handle *handle,
			   uint32_t service_id, uint32_t instance_id)
{
	struct msm_ipc_port_name svc_name;
	struct msm_ipc_server_info svc_info;
	struct msm_ipc_addr *svc_dest_addr;
	int rc;

	if (!handle)
		return -EINVAL;

	svc_dest_addr = kzalloc(sizeof(struct msm_ipc_addr),
				GFP_KERNEL);
	if (!svc_dest_addr) {
		pr_err("%s: Failure allocating memory\n", __func__);
		return -ENOMEM;
	}

	svc_name.service = service_id;
	svc_name.instance = instance_id;

	rc = msm_ipc_router_lookup_server_name(&svc_name, &svc_info, 1, 0xFF);
	if (rc <= 0) {
		pr_err("%s: Server not found\n", __func__);
		return -ENODEV;
	}
	svc_dest_addr->addrtype = MSM_IPC_ADDR_ID;
	svc_dest_addr->addr.port_addr.node_id = svc_info.node_id;
	svc_dest_addr->addr.port_addr.port_id = svc_info.port_id;
	mutex_lock(&handle->handle_lock);
	if (handle->handle_reset) {
		mutex_unlock(&handle->handle_lock);
		return -ENETRESET;
	}
	handle->dest_info = svc_dest_addr;
	mutex_unlock(&handle->handle_lock);

	return 0;
}
EXPORT_SYMBOL(qmi_connect_to_service);

static struct svc_event_nb *find_svc_event_nb_by_name(const char *name)
{
	struct svc_event_nb *temp;

	list_for_each_entry(temp, &svc_event_nb_list, list) {
		if (!strncmp(name, temp->pdriver_name,
			     sizeof(temp->pdriver_name)))
			return temp;
	}
	return NULL;
}

static int qmi_svc_event_probe(struct platform_device *pdev)
{
	struct svc_event_nb *temp;
	unsigned long flags;

	mutex_lock(&svc_event_nb_list_lock);
	temp = find_svc_event_nb_by_name(pdev->name);
	if (!temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		return -EINVAL;
	}

	spin_lock_irqsave(&temp->nb_lock, flags);
	temp->svc_avail = 1;
	raw_notifier_call_chain(&temp->svc_event_rcvr_list,
				QMI_SERVER_ARRIVE, NULL);
	spin_unlock_irqrestore(&temp->nb_lock, flags);
	mutex_unlock(&svc_event_nb_list_lock);
	return 0;
}

static int qmi_svc_event_remove(struct platform_device *pdev)
{
	struct svc_event_nb *temp;
	unsigned long flags;

	mutex_lock(&svc_event_nb_list_lock);
	temp = find_svc_event_nb_by_name(pdev->name);
	if (!temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		return -EINVAL;
	}

	spin_lock_irqsave(&temp->nb_lock, flags);
	temp->svc_avail = 0;
	raw_notifier_call_chain(&temp->svc_event_rcvr_list,
				QMI_SERVER_EXIT, NULL);
	spin_unlock_irqrestore(&temp->nb_lock, flags);
	mutex_unlock(&svc_event_nb_list_lock);
	return 0;
}

static struct svc_event_nb *find_svc_event_nb(uint32_t service_id,
					      uint32_t instance_id)
{
	struct svc_event_nb *temp;

	list_for_each_entry(temp, &svc_event_nb_list, list) {
		if (temp->service_id == service_id &&
		    temp->instance_id == instance_id)
			return temp;
	}
	return NULL;
}

static struct svc_event_nb *find_and_add_svc_event_nb(uint32_t service_id,
						      uint32_t instance_id)
{
	struct svc_event_nb *temp;
	int ret;

	mutex_lock(&svc_event_nb_list_lock);
	temp = find_svc_event_nb(service_id, instance_id);
	if (temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		return temp;
	}

	temp = kzalloc(sizeof(struct svc_event_nb), GFP_KERNEL);
	if (!temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		pr_err("%s: Failed to alloc notifier block\n", __func__);
		return temp;
	}

	spin_lock_init(&temp->nb_lock);
	temp->service_id = service_id;
	temp->instance_id = instance_id;
	INIT_LIST_HEAD(&temp->list);
	temp->svc_driver.probe = qmi_svc_event_probe;
	temp->svc_driver.remove = qmi_svc_event_remove;
	scnprintf(temp->pdriver_name, sizeof(temp->pdriver_name),
		  "QMI%08x:%08x", service_id, instance_id);
	temp->svc_driver.driver.name = temp->pdriver_name;
	RAW_INIT_NOTIFIER_HEAD(&temp->svc_event_rcvr_list);

	list_add_tail(&temp->list, &svc_event_nb_list);
	mutex_unlock(&svc_event_nb_list_lock);

	ret = platform_driver_register(&temp->svc_driver);
	if (ret < 0) {
		pr_err("%s: Failed pdriver register\n", __func__);
		mutex_lock(&svc_event_nb_list_lock);
		list_del(&temp->list);
		mutex_unlock(&svc_event_nb_list_lock);
		kfree(temp);
		temp = NULL;
	}

	return temp;
}

int qmi_svc_event_notifier_register(uint32_t service_id,
				    uint32_t instance_id,
				    struct notifier_block *nb)
{
	struct svc_event_nb *temp;
	unsigned long flags;
	int ret;

	temp = find_and_add_svc_event_nb(service_id, instance_id);
	if (!temp)
		return -EFAULT;

	mutex_lock(&svc_event_nb_list_lock);
	temp = find_svc_event_nb(service_id, instance_id);
	if (!temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		return -EFAULT;
	}
	spin_lock_irqsave(&temp->nb_lock, flags);
	if (temp->svc_avail)
		nb->notifier_call(nb, QMI_SERVER_ARRIVE, NULL);

	ret = raw_notifier_chain_register(&temp->svc_event_rcvr_list, nb);
	spin_unlock_irqrestore(&temp->nb_lock, flags);
	mutex_unlock(&svc_event_nb_list_lock);

	return ret;
}
EXPORT_SYMBOL(qmi_svc_event_notifier_register);

int qmi_svc_event_notifier_unregister(uint32_t service_id,
				      uint32_t instance_id,
				      struct notifier_block *nb)
{
	int ret;
	struct svc_event_nb *temp;
	unsigned long flags;

	mutex_lock(&svc_event_nb_list_lock);
	temp = find_svc_event_nb(service_id, instance_id);
	if (!temp) {
		mutex_unlock(&svc_event_nb_list_lock);
		return -EINVAL;
	}

	spin_lock_irqsave(&temp->nb_lock, flags);
	ret = raw_notifier_chain_unregister(&temp->svc_event_rcvr_list, nb);
	spin_unlock_irqrestore(&temp->nb_lock, flags);
	mutex_unlock(&svc_event_nb_list_lock);

	return ret;
}
EXPORT_SYMBOL(qmi_svc_event_notifier_unregister);

MODULE_DESCRIPTION("MSM QMI Interface");
MODULE_LICENSE("GPL v2");
