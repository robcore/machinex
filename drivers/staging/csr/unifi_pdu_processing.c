/*
 * ---------------------------------------------------------------------------
 * FILE:     unifi_pdu_processing.c
 *
 * PURPOSE:
 *      This file provides the PDU handling functionality before it gets sent to unfi and after
 *      receiving a PDU from unifi
 *
 * Copyright (C) 2010 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */


#include <linux/types.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>

#include "csr_wifi_hip_unifi.h"
#include "csr_wifi_hip_conversions.h"
#include "csr_time.h"
#include "unifi_priv.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
#include <net/iw_handler.h>
#endif
#include <net/pkt_sched.h>

#ifdef CSR_SUPPORT_SME
static void _update_buffered_pkt_params_after_alignment(unifi_priv_t *priv, bulk_data_param_t *bulkdata,
                                                        tx_buffered_packets_t* buffered_pkt)
{

    struct sk_buff *skb ;
    CsrUint32 align_offset;

    if (priv == NULL || bulkdata == NULL || buffered_pkt == NULL){
        return;
    }
    skb = (struct sk_buff*)bulkdata->d[0].os_net_buf_ptr;
    align_offset = (CsrUint32)(long)(bulkdata->d[0].os_data_ptr) & (CSR_WIFI_ALIGN_BYTES-1);
    if(align_offset){
        skb_pull(skb,align_offset);
    }
    buffered_pkt->bulkdata.os_data_ptr = skb->data;
    buffered_pkt->bulkdata.data_length = skb->len;


}
#endif

void
unifi_frame_ma_packet_req(unifi_priv_t *priv, CSR_PRIORITY priority,
                          CSR_RATE TransmitRate, CSR_CLIENT_TAG hostTag,
                          CsrUint16 interfaceTag, CSR_TRANSMISSION_CONTROL transmissionControl,
                          CSR_PROCESS_ID leSenderProcessId, CsrUint8 *peerMacAddress,
                          CSR_SIGNAL *signal)
{

    CSR_MA_PACKET_REQUEST *req = &signal->u.MaPacketRequest;
    netInterface_priv_t *interfacePriv;
    CsrUint8 ba_session_idx = 0;
    ba_session_tx_struct *ba_session = NULL;
    CsrUint8 *ba_addr = NULL;

    interfacePriv = priv->interfacePriv[interfaceTag];

    UF_TRACE_MAC(priv, UDBG5, "In unifi_frame_ma_packet_req, Frame for Peer:", peerMacAddress);
    signal->SignalPrimitiveHeader.SignalId = CSR_MA_PACKET_REQUEST_ID;
    signal->SignalPrimitiveHeader.ReceiverProcessId = 0;
    signal->SignalPrimitiveHeader.SenderProcessId = leSenderProcessId;

    /* Fill the MA-PACKET.req */
    req->Priority = priority;
    unifi_trace(priv, UDBG3, "Tx Frame with Priority: 0x%x\n", req->Priority);

    /* A value of 0 is used for auto selection of rates. But for P2P GO case
     * for action frames the rate is governed by SME. Hence instead of 0,
     * the rate is filled in with the value passed here
     */
    req->TransmitRate = TransmitRate;

    /* packets from netdev then no confirm required but packets from
     * Nme/Sme eapol data frames requires the confirmation
     */
    req->TransmissionControl = transmissionControl;
    req->VirtualInterfaceIdentifier =
           uf_get_vif_identifier(interfacePriv->interfaceMode,interfaceTag);
    memcpy(req->Ra.x, peerMacAddress, ETH_ALEN);

    if (hostTag == 0xffffffff) {
        req->HostTag = interfacePriv->tag++;
        req->HostTag |= 0x40000000;
        unifi_trace(priv, UDBG3, "new host tag assigned = 0x%x\n", req->HostTag);
        interfacePriv->tag &= 0x0fffffff;
    } else {
        req->HostTag = hostTag;
        unifi_trace(priv, UDBG3, "host tag got from SME  = 0x%x\n", req->HostTag);
    }
    /* check if BA session exists for the peer MAC address on same tID */
    if(interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_AP ||
       interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_P2PGO){
        ba_addr = peerMacAddress;
    }else{
        ba_addr = interfacePriv->bssid.a;
    }
    for (ba_session_idx=0; ba_session_idx < MAX_SUPPORTED_BA_SESSIONS_TX; ba_session_idx++){
        ba_session = interfacePriv->ba_session_tx[ba_session_idx];
        if (ba_session){
           if ((!memcmp(ba_session->macAddress.a, ba_addr, ETH_ALEN)) && (ba_session->tID == priority)){
                req->TransmissionControl |= CSR_ALLOW_BA;
                break;
            }
        }
    }

    unifi_trace(priv, UDBG5, "leaving unifi_frame_ma_packet_req\n");
}

#ifdef CSR_SUPPORT_SME

#define TRANSMISSION_CONTROL_TRIGGER_MASK 0x0001
#define TRANSMISSION_CONTROL_ESOP_MASK 0x0002

static
int frame_and_send_queued_pdu(unifi_priv_t* priv,tx_buffered_packets_t* buffered_pkt,
            CsrWifiRouterCtrlStaInfo_t *staRecord,CsrBool moreData , CsrBool eosp)
{

    CSR_SIGNAL signal;
    bulk_data_param_t bulkdata;
    int result;
    CsrUint8 toDs, fromDs, macHeaderLengthInBytes = MAC_HEADER_SIZE;
    CsrUint8 *qc;
    CsrUint16 *fc = (CsrUint16*)(buffered_pkt->bulkdata.os_data_ptr);
    unsigned long lock_flags;
    unifi_trace(priv, UDBG3, "frame_and_send_queued_pdu with moreData: %d , EOSP: %d\n",moreData,eosp);
    unifi_frame_ma_packet_req(priv, buffered_pkt->priority, buffered_pkt->rate, buffered_pkt->hostTag,
               buffered_pkt->interfaceTag, buffered_pkt->transmissionControl,
               buffered_pkt->leSenderProcessId, buffered_pkt->peerMacAddress.a, &signal);
    bulkdata.d[0].os_data_ptr = buffered_pkt->bulkdata.os_data_ptr;
    bulkdata.d[0].data_length = buffered_pkt->bulkdata.data_length;
    bulkdata.d[0].os_net_buf_ptr = buffered_pkt->bulkdata.os_net_buf_ptr;
    bulkdata.d[0].net_buf_length = buffered_pkt->bulkdata.net_buf_length;
    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].data_length = 0;
    bulkdata.d[1].os_net_buf_ptr =0;
    bulkdata.d[1].net_buf_length =0;

    if(moreData) {
        *fc |= cpu_to_le16(IEEE802_11_FC_MOREDATA_MASK);
    } else {
        *fc &= cpu_to_le16(~IEEE802_11_FC_MOREDATA_MASK);
    }

    if((staRecord != NULL)&& (staRecord->wmmOrQosEnabled == TRUE))
    {
        unifi_trace(priv, UDBG3, "frame_and_send_queued_pdu WMM Enabled: %d \n",staRecord->wmmOrQosEnabled);

        toDs = (*fc & cpu_to_le16(IEEE802_11_FC_TO_DS_MASK))?1 : 0;
        fromDs = (*fc & cpu_to_le16(IEEE802_11_FC_FROM_DS_MASK))? 1: 0;

        switch(le16_to_cpu(*fc) & IEEE80211_FC_SUBTYPE_MASK)
        {
            case IEEE802_11_FC_TYPE_QOS_DATA & IEEE80211_FC_SUBTYPE_MASK:
            case IEEE802_11_FC_TYPE_QOS_NULL & IEEE80211_FC_SUBTYPE_MASK:
                /* If both are set then the Address4 exists (only for AP) */
                if (fromDs && toDs)
                {
                    /* 6 is the size of Address4 field */
                    macHeaderLengthInBytes += (QOS_CONTROL_HEADER_SIZE + 6);
                }
                else
                {
                    macHeaderLengthInBytes += QOS_CONTROL_HEADER_SIZE;
                }

                /* If order bit set then HT control field is the part of MAC header */
                if (*fc & cpu_to_le16(IEEE80211_FC_ORDER_MASK)) {
                    macHeaderLengthInBytes += HT_CONTROL_HEADER_SIZE;
                }
                break;
            default:
                if (fromDs && toDs)
                    macHeaderLengthInBytes += 6;
             break;
        }

        if (*fc & cpu_to_le16(IEEE80211_FC_ORDER_MASK)) {
            qc = (CsrUint8*)(buffered_pkt->bulkdata.os_data_ptr + (macHeaderLengthInBytes-6));
        } else {
            qc = (CsrUint8*)(buffered_pkt->bulkdata.os_data_ptr + (macHeaderLengthInBytes-2));
        }

        *qc = eosp ? *qc | (1 << 4) : *qc & (~(1 << 4));
    }
    result = ul_send_signal_unpacked(priv, &signal, &bulkdata);
    if(result){
        _update_buffered_pkt_params_after_alignment(priv, &bulkdata,buffered_pkt);
    }

 /* Decrement the packet counts queued in driver */
    if (result != -ENOSPC) {
        /* protect entire counter updation by disabling preemption */
        if (!priv->noOfPktQueuedInDriver) {
            unifi_error(priv, "packets queued in driver 0 still decrementing\n");
        } else {
            spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
            priv->noOfPktQueuedInDriver--;
            spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
        }
        /* Sta Record is available for all unicast (except genericMgt Frames) & in other case its NULL */
        if (staRecord) {
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            if (!staRecord->noOfPktQueued) {
                unifi_error(priv, "packets queued in driver per station is 0 still decrementing\n");
            } else {
                staRecord->noOfPktQueued--;
            }
            /* if the STA alive probe frame has failed then reset the saved host tag */
            if (result){
                if (staRecord->nullDataHostTag == buffered_pkt->hostTag){
                    staRecord->nullDataHostTag = INVALID_HOST_TAG;
                }
            }
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
        }

    }
    return result;
}
#ifdef CSR_SUPPORT_SME
static
void set_eosp_transmit_ctrl(unifi_priv_t *priv, struct list_head *txList)
{
    /* dequeue the tx data packets from the appropriate queue */
    tx_buffered_packets_t *tx_q_item = NULL;
    struct list_head *listHead;
    struct list_head *placeHolder;
    unsigned long lock_flags;


    unifi_trace(priv, UDBG5, "entering set_eosp_transmit_ctrl\n");
    /* check for list empty */
    if (list_empty(txList)) {
        unifi_warning(priv, "In set_eosp_transmit_ctrl, the list is empty\n");
        return;
    }

    /* return the last node , and modify it. */

    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    list_for_each_prev_safe(listHead, placeHolder, txList) {
        tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);
        tx_q_item->transmissionControl |= TRANSMISSION_CONTROL_ESOP_MASK;
        tx_q_item->transmissionControl = (tx_q_item->transmissionControl & ~(CSR_NO_CONFIRM_REQUIRED));
        unifi_trace(priv, UDBG1,
                "set_eosp_transmit_ctrl Transmission Control = 0x%x hostTag = 0x%x \n",tx_q_item->transmissionControl,tx_q_item->hostTag);
        unifi_trace(priv,UDBG3,"in set_eosp_transmit_ctrl no.of buffered frames %d\n",priv->noOfPktQueuedInDriver);
        break;
    }
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
    unifi_trace(priv, UDBG1,"List Empty %d\n",list_empty(txList));
    unifi_trace(priv, UDBG5, "leaving set_eosp_transmit_ctrl\n");
    return;
}

static
void send_vif_availibility_rsp(unifi_priv_t *priv,CSR_VIF_IDENTIFIER vif,CSR_RESULT_CODE resultCode)
{
    CSR_SIGNAL signal;
    CSR_MA_VIF_AVAILABILITY_RESPONSE *rsp;
    bulk_data_param_t *bulkdata = NULL;
    int r;

    memset(&signal,0,sizeof(CSR_SIGNAL));
    rsp = &signal.u.MaVifAvailabilityResponse;
    rsp->VirtualInterfaceIdentifier = vif;
    rsp->ResultCode = resultCode;
    signal.SignalPrimitiveHeader.SignalId = CSR_MA_VIF_AVAILABILITY_RESPONSE_ID;
    signal.SignalPrimitiveHeader.ReceiverProcessId = 0;
    signal.SignalPrimitiveHeader.SenderProcessId = priv->netdev_client->sender_id;

    /* Send the signal to UniFi */
    r = ul_send_signal_unpacked(priv, &signal, bulkdata);
    if(r) {
        unifi_error(priv,"Availibility response sending failed %x status %d\n",vif,r);
    }
}
#endif

static
void verify_and_accomodate_tx_packet(unifi_priv_t *priv)
{
    tx_buffered_packets_t *tx_q_item;
    unsigned long lock_flags;
    struct list_head *listHead, *list;
    struct list_head *placeHolder;
    CsrUint8 i, j,eospFramedeleted=0;
    CsrBool thresholdExcedeDueToBroadcast = TRUE;
    /* it will be made it interface Specific in the future when multi interfaces are supported ,
    right now interface 0 is considered */
    netInterface_priv_t *interfacePriv = priv->interfacePriv[0];
    CsrWifiRouterCtrlStaInfo_t *staInfo = NULL;

    unifi_trace(priv, UDBG3, "entering verify_and_accomodate_tx_packet\n");

    for(i = 0; i < UNIFI_MAX_CONNECTIONS; i++) {
        staInfo = interfacePriv->staInfo[i];
            if (staInfo && (staInfo->noOfPktQueued >= CSR_WIFI_DRIVER_MAX_PKT_QUEUING_THRESHOLD_PER_PEER)) {
            /* remove the first(oldest) packet from the all the access catogory, since data
             * packets for station record crossed the threshold limit (64 for AP supporting
             * 8 peers)
             */
            unifi_trace(priv,UDBG3,"number of station pkts queued=  %d for sta id = %d\n", staInfo->noOfPktQueued, staInfo->aid);
            for(j = 0; j < MAX_ACCESS_CATOGORY; j++) {
                list = &staInfo->dataPdu[j];
                spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                list_for_each_safe(listHead, placeHolder, list) {
                    tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);
                    list_del(listHead);
                    thresholdExcedeDueToBroadcast = FALSE;
                    unifi_net_data_free(priv, &tx_q_item->bulkdata);
                    kfree(tx_q_item);
                    tx_q_item = NULL;
                    if (!priv->noOfPktQueuedInDriver) {
                        unifi_error(priv, "packets queued in driver 0 still decrementing in %s\n", __FUNCTION__);
                    } else {
                        /* protection provided by spinlock */
                        priv->noOfPktQueuedInDriver--;

                    }
                    /* Sta Record is available for all unicast (except genericMgt Frames) & in other case its NULL */
                    if (!staInfo->noOfPktQueued) {
                        unifi_error(priv, "packets queued in driver per station is 0 still decrementing in %s\n", __FUNCTION__);
                    } else {
                        spin_lock(&priv->staRecord_lock);
                        staInfo->noOfPktQueued--;
                        spin_unlock(&priv->staRecord_lock);
                    }
                    break;
                }
                spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
            }
        }
    }
    if (thresholdExcedeDueToBroadcast &&  interfacePriv->noOfbroadcastPktQueued > CSR_WIFI_DRIVER_MINIMUM_BROADCAST_PKT_THRESHOLD ) {
        /* Remove the packets from genericMulticastOrBroadCastFrames queue
         * (the max packets in driver is reached due to broadcast/multicast frames)
         */
        spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
        list_for_each_safe(listHead, placeHolder, &interfacePriv->genericMulticastOrBroadCastFrames) {
            tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);
            if(eospFramedeleted){
                tx_q_item->transmissionControl |= TRANSMISSION_CONTROL_ESOP_MASK;
                tx_q_item->transmissionControl = (tx_q_item->transmissionControl & ~(CSR_NO_CONFIRM_REQUIRED));
                unifi_trace(priv, UDBG1,"updating eosp for next packet hostTag:= 0x%x ",tx_q_item->hostTag);
                eospFramedeleted =0;
                break;
            }

            if(tx_q_item->transmissionControl & TRANSMISSION_CONTROL_ESOP_MASK ){
               eospFramedeleted = 1;
            }
            unifi_trace(priv,UDBG1, "freeing of multicast packets ToC = 0x%x hostTag = 0x%x \n",tx_q_item->transmissionControl,tx_q_item->hostTag);
            list_del(listHead);
            unifi_net_data_free(priv, &tx_q_item->bulkdata);
            kfree(tx_q_item);
            priv->noOfPktQueuedInDriver--;
            spin_lock(&priv->staRecord_lock);
            interfacePriv->noOfbroadcastPktQueued--;
            spin_unlock(&priv->staRecord_lock);
            if(!eospFramedeleted){
                break;
            }
        }
        spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
    }
    unifi_trace(priv, UDBG3, "leaving verify_and_accomodate_tx_packet\n");
}

static
CsrResult enque_tx_data_pdu(unifi_priv_t *priv, bulk_data_param_t *bulkdata,
                            struct list_head *list, CSR_SIGNAL *signal,
                            CsrBool requeueOnSamePos)
{

    /* queue the tx data packets on to appropriate queue */
    CSR_MA_PACKET_REQUEST *req = &signal->u.MaPacketRequest;
    tx_buffered_packets_t *tx_q_item;
    unsigned long lock_flags;

    unifi_trace(priv, UDBG5, "entering enque_tx_data_pdu\n");
    if(!list) {
       unifi_error(priv,"List is not specified\n");
       return CSR_RESULT_FAILURE;
    }

    /* Removes aged packets & adds the incoming packet */
    if (priv->noOfPktQueuedInDriver >= CSR_WIFI_DRIVER_SUPPORT_FOR_MAX_PKT_QUEUEING) {
        unifi_trace(priv,UDBG3,"number of pkts queued=  %d \n", priv->noOfPktQueuedInDriver);
        verify_and_accomodate_tx_packet(priv);
    }



    tx_q_item = (tx_buffered_packets_t *)kmalloc(sizeof(tx_buffered_packets_t), GFP_ATOMIC);
    if (tx_q_item == NULL) {
        unifi_error(priv,
                "Failed to allocate %d bytes for tx packet record\n",
                sizeof(tx_buffered_packets_t));
        func_exit();
        return CSR_RESULT_FAILURE;
    }

    /* disable the preemption */
    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    INIT_LIST_HEAD(&tx_q_item->q);
    /* fill the tx_q structure members */
    tx_q_item->bulkdata.os_data_ptr = bulkdata->d[0].os_data_ptr;
    tx_q_item->bulkdata.data_length = bulkdata->d[0].data_length;
    tx_q_item->bulkdata.os_net_buf_ptr = bulkdata->d[0].os_net_buf_ptr;
    tx_q_item->bulkdata.net_buf_length = bulkdata->d[0].net_buf_length;
    tx_q_item->interfaceTag = req->VirtualInterfaceIdentifier & 0xff;
    tx_q_item->hostTag = req->HostTag;
    tx_q_item->leSenderProcessId = signal->SignalPrimitiveHeader.SenderProcessId;
    tx_q_item->transmissionControl = req->TransmissionControl;
    tx_q_item->priority = req->Priority;
    tx_q_item->rate = req->TransmitRate;
    memcpy(tx_q_item->peerMacAddress.a, req->Ra.x, ETH_ALEN);



    if (requeueOnSamePos) {
        list_add(&tx_q_item->q, list);
    } else {
        list_add_tail(&tx_q_item->q, list);
    }

    /* Count of packet queued in driver */
    priv->noOfPktQueuedInDriver++;
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
    unifi_trace(priv, UDBG5, "leaving enque_tx_data_pdu\n");
    return CSR_RESULT_SUCCESS;
}
static
CsrResult enque_direceted_ma_pkt_cfm_data_pdu(unifi_priv_t *priv, bulk_data_param_t *bulkdata,
                            struct list_head *list, CSR_SIGNAL *signal,
                            CsrBool requeueOnSamePos)
{

    /* queue the tx data packets on to appropriate queue */
    CSR_MA_PACKET_REQUEST *req = &signal->u.MaPacketRequest;
    tx_buffered_packets_t *tx_q_item;


    unifi_trace(priv, UDBG5, "entering enque_tx_data_pdu\n");
    if(!list  ) {
       unifi_error(priv,"List is not specified\n");
       return CSR_RESULT_FAILURE;
    }
    if(!requeueOnSamePos && !list->prev){
       unifi_error(priv,"List prev is NULL so don't requeu it\n");
       return CSR_RESULT_FAILURE;

    }



    tx_q_item = (tx_buffered_packets_t *)kmalloc(sizeof(tx_buffered_packets_t), GFP_ATOMIC);
    if (tx_q_item == NULL) {
        unifi_error(priv,
                "Failed to allocate %d bytes for tx packet record\n",
                sizeof(tx_buffered_packets_t));
        func_exit();
        return CSR_RESULT_FAILURE;
    }
    /* disable the preemption */
    INIT_LIST_HEAD(&tx_q_item->q);
    /* fill the tx_q structure members */
    tx_q_item->bulkdata.os_data_ptr = bulkdata->d[0].os_data_ptr;
    tx_q_item->bulkdata.data_length = bulkdata->d[0].data_length;
    tx_q_item->bulkdata.os_net_buf_ptr = bulkdata->d[0].os_net_buf_ptr;
    tx_q_item->bulkdata.net_buf_length = bulkdata->d[0].net_buf_length;
    tx_q_item->interfaceTag = req->VirtualInterfaceIdentifier & 0xff;
    tx_q_item->hostTag = req->HostTag;
    tx_q_item->leSenderProcessId = signal->SignalPrimitiveHeader.SenderProcessId;
    tx_q_item->transmissionControl = req->TransmissionControl;
    tx_q_item->priority = req->Priority;
    tx_q_item->rate = req->TransmitRate;
    memcpy(tx_q_item->peerMacAddress.a, req->Ra.x, ETH_ALEN);



    if (requeueOnSamePos) {
        list_add(&tx_q_item->q, list);
    } else {
        list_add_tail(&tx_q_item->q, list);
    }

    /* Count of packet queued in driver */
    priv->noOfPktQueuedInDriver++;
    unifi_trace(priv, UDBG5, "leaving enque_tx_data_pdu\n");
    return CSR_RESULT_SUCCESS;
}

static void is_all_ac_deliver_enabled_and_moredata(CsrWifiRouterCtrlStaInfo_t *staRecord, CsrUint8 *allDeliveryEnabled, CsrUint8 *dataAvailable)
{
    CsrUint8 i;
    *allDeliveryEnabled = TRUE;
    for (i = 0 ;i < MAX_ACCESS_CATOGORY; i++) {
        if (!IS_DELIVERY_ENABLED(staRecord->powersaveMode[i])) {
            /* One is is not Delivery Enabled */
            *allDeliveryEnabled = FALSE;
            break;
        }
    }
    if (*allDeliveryEnabled) {
        *dataAvailable = (!list_empty(&staRecord->dataPdu[0]) || !list_empty(&staRecord->dataPdu[1])
                          ||!list_empty(&staRecord->dataPdu[2]) ||!list_empty(&staRecord->dataPdu[3])
                          ||!list_empty(&staRecord->mgtFrames));
    }
}

/*
 * ---------------------------------------------------------------------------
 *  uf_handle_tim_cfm
 *
 *
 *      This function updates tim status in host depending confirm status from firmware
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      cfm             CSR_MLME_SET_TIM_CONFIRM
 *      receiverProcessId SenderProcessID to fetch handle & timSet status
 *
 * ---------------------------------------------------------------------------
 */
void uf_handle_tim_cfm(unifi_priv_t *priv, CSR_MLME_SET_TIM_CONFIRM *cfm, CsrUint16 receiverProcessId)
{
    CsrUint8 handle = CSR_WIFI_GET_STATION_HANDLE_FROM_RECEIVER_ID(receiverProcessId);
    CsrUint8 timSetStatus = CSR_WIFI_GET_TIMSET_STATE_FROM_RECEIVER_ID(receiverProcessId);
    CsrUint16 interfaceTag = (cfm->VirtualInterfaceIdentifier & 0xff);
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CsrWifiRouterCtrlStaInfo_t *staRecord = NULL;
    /* This variable holds what TIM value we wanted to set in firmware */
    CsrUint16 timSetValue = 0;
    /* Irrespective of interface the count maintained */
    static CsrUint8 retryCount = 0;
    unsigned long lock_flags;
    unifi_trace(priv, UDBG3, "entering %s, handle = %x, timSetStatus = %x\n", __FUNCTION__, handle, timSetStatus);

    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_warning(priv, "bad interfaceTag = %x\n", interfaceTag);
        return;
    }

    if ((handle != CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE) && (handle >= UNIFI_MAX_CONNECTIONS)) {
        unifi_warning(priv, "bad station Handle = %x\n", handle);
        return;
    }

    if (handle != CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE) {
        spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
        if ((staRecord = ((CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[handle]))) == NULL) {
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
            unifi_warning(priv, "uf_handle_tim_cfm: station record is NULL  handle = %x\n", handle);
            return;
        }
       spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
    }
    switch(timSetStatus)
    {
        case CSR_WIFI_TIM_SETTING:
            timSetValue = CSR_WIFI_TIM_SET;
            break;
        case CSR_WIFI_TIM_RESETTING:
            timSetValue = CSR_WIFI_TIM_RESET;
            break;
        default:
            unifi_warning(priv, "timSet state is %x: Debug\n", timSetStatus);
            return;
    }

    /* check TIM confirm for success/failures */
    switch(cfm->ResultCode)
    {
        case CSR_RC_SUCCESS:
            if (handle != CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE) {
                /* Unicast frame & station record available */
                if (timSetStatus == staRecord->timSet) {
                    staRecord->timSet = timSetValue;
                    /* fh_cmd_q can also be full at some point of time!,
                     * resetting count as queue is cleaned by firmware at this point
                     */
                    retryCount = 0;
                    unifi_trace(priv, UDBG2, "tim (%s) successfully in firmware\n", (timSetValue)?"SET":"RESET");
                } else {
                    unifi_trace(priv, UDBG3, "receiver processID = %x, success: request & confirm states are not matching in TIM cfm: Debug status = %x, staRecord->timSet = %x, handle = %x\n",
                                 receiverProcessId, timSetStatus, staRecord->timSet, handle);
                }
            } else {
                /* fh_cmd_q can also be full at some point of time!,
                 * resetting count as queue is cleaned by firmware at this point
                 */
                retryCount = 0;
                unifi_trace(priv, UDBG3, "tim (%s) successfully for broadcast frame in firmware\n", (timSetValue)?"SET":"RESET");
            }
            break;
        case CSR_RC_INVALID_PARAMETERS:
        case CSR_RC_INSUFFICIENT_RESOURCE:
            /* check for max retry limit & send again
             * MAX_RETRY_LIMIT is not maintained for each set of transactions..Its generic
             * If failure crosses this Limit, we have to take a call to FIX
             */
            if (retryCount > UNIFI_MAX_RETRY_LIMIT) {
                CsrBool moreData = FALSE;
                retryCount = 0;
                /* Because of continuos traffic in fh_cmd_q the tim set request is failing (exceeding retry limit)
                 * but if we didn't synchronize our timSet varible state with firmware then it can cause below issues
                 * cond 1. We want to SET tim in firmware if its fails & max retry limit reached
                 *   -> If host set's the timSet to 1, we wont try to send(as max retry reached) update tim but
                 *   firmware is not updated with queue(TIM) status so it wont set TIM in beacon finally host start piling
                 *    up data & wont try to set tim in firmware (This can cause worser performance)
                 * cond 2. We want to reset tim in firmware it fails & reaches max retry limit
                 *   -> If host sets the timSet to Zero, it wont try to set a TIM request unless we wont have any packets
                 *   to be queued, so beacon unnecessarily advertizes the TIM
                 */

                if(staRecord) {
                    if(!staRecord->wmmOrQosEnabled) {
                        moreData = (!list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]) ||
                                !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]) ||
                                !list_empty(&staRecord->mgtFrames));
                    } else {
                        /* Peer is QSTA */
                        CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
                        /* Check if all AC's are Delivery Enabled */
                        is_all_ac_deliver_enabled_and_moredata(staRecord, &allDeliveryEnabled, &dataAvailable);
                        /*check for more data in non-delivery enabled queues*/
                        moreData = (uf_is_more_data_for_non_delivery_ac(staRecord) || (allDeliveryEnabled && dataAvailable));

                    }
                    /* To avoid cond 1 & 2, check internal Queues status, if we have more Data then set RESET the timSet(0),
                     *  so we are trying to be in sync with firmware & next packets before queuing atleast try to
                     *  set TIM in firmware otherwise it SET timSet(1)
                     */
                    if (moreData) {
                        staRecord->timSet = CSR_WIFI_TIM_RESET;
                    } else {
                        staRecord->timSet = CSR_WIFI_TIM_SET;
                    }
                } else {
                    /* Its a broadcast frames */
                    moreData = (!list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames) ||
                               !list_empty(&interfacePriv->genericMulticastOrBroadCastFrames));
                    if (moreData) {
                        update_tim(priv, 0, CSR_WIFI_TIM_SET, interfaceTag, 0xFFFFFFFF);
                    } else {
                        update_tim(priv, 0, CSR_WIFI_TIM_RESET, interfaceTag, 0xFFFFFFFF);
                    }
                }

                unifi_error(priv, "no of error's for TIM setting crossed the Limit: verify\n");
                return;
            }
            retryCount++;

            if (handle != CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE) {
                if (timSetStatus == staRecord->timSet) {
                    unifi_warning(priv, "tim request failed, retry for AID = %x\n", staRecord->aid);
                    update_tim(priv, staRecord->aid, timSetValue, interfaceTag, handle);
                } else {
                    unifi_trace(priv, UDBG1, "failure: request & confirm states are not matching in TIM cfm: Debug status = %x, staRecord->timSet = %x\n",
                                  timSetStatus, staRecord->timSet);
                }
            } else {
                unifi_warning(priv, "tim request failed, retry for broadcast frames\n");
                update_tim(priv, 0, timSetValue, interfaceTag, 0xFFFFFFFF);
            }
            break;
        default:
            unifi_warning(priv, "tim update request failed resultcode = %x\n", cfm->ResultCode);
    }
    unifi_trace(priv, UDBG2, "leaving %s\n", __FUNCTION__);
}

/*
 * ---------------------------------------------------------------------------
 *  update_tim
 *
 *
 *      This function updates tim status in firmware for AID[1 to UNIFI_MAX_CONNECTIONS] or
 *       AID[0] for broadcast/multicast packets.
 *
 *      NOTE: The LSB (least significant BYTE) of senderId while sending this MLME premitive
 *       has been modified(utilized) as below
 *
 *       SenderID in signal's SignalPrimitiveHeader is 2 byte the lowe byte bitmap is below
 *
 *       station handle(6 bits)      timSet Status (2 bits)
 *       ---------------------       ----------------------
 *       0  0  0  0  0  0        |       0  0
 *
 * timSet Status can be one of below:
 *
 * CSR_WIFI_TIM_RESET
 * CSR_WIFI_TIM_RESETTING
 * CSR_WIFI_TIM_SET
 * CSR_WIFI_TIM_SETTING
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      aid             can be 1 t0 UNIFI_MAX_CONNECTIONS & 0 means multicast/broadcast
 *      setTim          value SET(1) / RESET(0)
 *      interfaceTag    the interfaceID on which activity going on
 *      handle          from  (0 <= handle < UNIFI_MAX_CONNECTIONS)
 *
 * ---------------------------------------------------------------------------
 */
void update_tim(unifi_priv_t * priv, CsrUint16 aid, CsrUint8 setTim, CsrUint16 interfaceTag, CsrUint32 handle)
{
    CSR_SIGNAL signal;
    CsrInt32 r;
    CSR_MLME_SET_TIM_REQUEST *req = &signal.u.MlmeSetTimRequest;
    bulk_data_param_t *bulkdata = NULL;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CsrUint8 senderIdLsb = 0;
    CsrWifiRouterCtrlStaInfo_t *staRecord = NULL;
    CsrUint32 oldTimSetStatus = 0, timSetStatus = 0;

    unifi_trace(priv, UDBG5, "entering the update_tim routine\n");

    if (handle == 0xFFFFFFFF) {
        handle &= CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE;
    } else if ((handle != 0xFFFFFFFF) && (handle >= UNIFI_MAX_CONNECTIONS)) {
        unifi_warning(priv, "bad station Handle = %x\n", handle);
        return;
    }

    if (setTim) {
        timSetStatus =  CSR_WIFI_TIM_SETTING;
    } else {
        timSetStatus =  CSR_WIFI_TIM_RESETTING;
    }

    if (handle != CSR_WIFI_BROADCAST_OR_MULTICAST_HANDLE) {
        if ((staRecord = ((CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[handle]))) == NULL) {
            unifi_warning(priv, "station record is NULL in  update_tim: handle = %x :debug\n", handle);
            return;
        }
        /* In case of signal sending failed, revert back to old state */
        oldTimSetStatus = staRecord->timSet;
        staRecord->timSet = timSetStatus;
    }

    /* pack senderID LSB */
    senderIdLsb = CSR_WIFI_PACK_SENDER_ID_LSB_FOR_TIM_REQ(handle,  timSetStatus);

    /* initialize signal to zero */
    memset(&signal, 0, sizeof(CSR_SIGNAL));

    /* Frame the MLME-SET-TIM request */
    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_SET_TIM_REQUEST_ID;
    signal.SignalPrimitiveHeader.ReceiverProcessId = 0;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(((priv->netdev_client->sender_id & 0xff00) | senderIdLsb),
                   (u8*)&signal.SignalPrimitiveHeader.SenderProcessId);

    /* set The virtual interfaceIdentifier, aid, tim value */
    req->VirtualInterfaceIdentifier = uf_get_vif_identifier(interfacePriv->interfaceMode,interfaceTag);
    req->AssociationId = aid;
    req->TimValue = setTim;


    unifi_trace(priv, UDBG2, "update_tim:AID %x,senderIdLsb = 0x%x, handle = 0x%x, timSetStatus = %x, sender proceesID = %x \n",
                aid,senderIdLsb, handle, timSetStatus, signal.SignalPrimitiveHeader.SenderProcessId);

    /* Send the signal to UniFi */
    r = ul_send_signal_unpacked(priv, &signal, bulkdata);
    if (r) {
        /* No need to free bulk data, as TIM request doesn't carries any data */
        unifi_error(priv, "Error queueing CSR_MLME_SET_TIM_REQUEST signal\n");
        if (staRecord) {
            staRecord->timSet = oldTimSetStatus ;
        }
    }
    unifi_trace(priv, UDBG5, "leaving the update_tim routine\n");
}

static
void process_peer_active_transition(unifi_priv_t * priv,
                                    CsrWifiRouterCtrlStaInfo_t *staRecord,
                                    CsrUint16 interfaceTag)
{
    int r,i;
    CsrBool spaceAvail[4] = {TRUE,TRUE,TRUE,TRUE};
    tx_buffered_packets_t * buffered_pkt = NULL;
    unsigned long lock_flags;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];

    unifi_trace(priv, UDBG5, "entering process_peer_active_transition\n");

    if(IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag)) {
        /* giving more priority to multicast packets so delaying unicast packets*/
        unifi_trace(priv,UDBG2," multicast transmission is going on so resume unicast transmission after DTIM over\n");
        return;
    }
    while((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->mgtFrames))) {
        buffered_pkt->transmissionControl &=
                     ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);
        if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,0,FALSE)) == -ENOSPC) {
            unifi_trace(priv, UDBG2, "p_p_a_t:(ENOSPC) Mgt Frame queueing \n");
            /* Enqueue at the head of the queue */
            spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
            list_add(&buffered_pkt->q, &staRecord->mgtFrames);
            spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
            priv->pausedStaHandle[3]=(CsrUint8)(staRecord->assignedHandle);
            spaceAvail[3] = FALSE;
            break;
        } else {
            if(r){
                unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                /* the PDU failed where we can't do any thing so free the storage */
                unifi_net_data_free(priv, &buffered_pkt->bulkdata);
            }
            kfree(buffered_pkt);
        }
    }
    if (staRecord->txSuspend) {
        if(staRecord->timSet == CSR_WIFI_TIM_SET) {
            update_tim(priv,staRecord->aid,0,interfaceTag, staRecord->assignedHandle);
        }
        return;
    }
    for(i=3;i>=0;i--) {
        if(!spaceAvail[i])
            continue;
        unifi_trace(priv, UDBG6, "p_p_a_t:data pkt sending for AC %d \n",i);
        while((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->dataPdu[i]))) {
           buffered_pkt->transmissionControl &=
                      ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);
           if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,0,FALSE)) == -ENOSPC) {
               /* Clear the trigger bit transmission control*/
               /* Enqueue at the head of the queue */
               spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
               list_add(&buffered_pkt->q, &staRecord->dataPdu[i]);
               spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
               priv->pausedStaHandle[i]=(CsrUint8)(staRecord->assignedHandle);
               break;
           } else {
              if(r){
                  unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                  /* the PDU failed where we can't do any thing so free the storage */
                  unifi_net_data_free(priv, &buffered_pkt->bulkdata);
               }
              kfree(buffered_pkt);
           }
        }
    }
    if((staRecord->timSet  == CSR_WIFI_TIM_SET) || (staRecord->timSet == CSR_WIFI_TIM_SETTING)){
        unifi_trace(priv, UDBG3, "p_p_a_t:resetting tim .....\n");
        update_tim(priv,staRecord->aid,0,interfaceTag, staRecord->assignedHandle);
    }
    unifi_trace(priv, UDBG5, "leaving process_peer_active_transition\n");
}



void uf_process_ma_pkt_cfm_for_ap(unifi_priv_t *priv,CsrUint16 interfaceTag, const CSR_MA_PACKET_CONFIRM *pkt_cfm)
{
    netInterface_priv_t *interfacePriv;
    CsrUint8 i;
    CsrWifiRouterCtrlStaInfo_t *staRecord = NULL;
    struct list_head *listHeadMaPktreq,*listHeadStaQueue;
    struct list_head *placeHolderMaPktreq,*placeHolderStaQueue;
    unsigned long lock_flags;
    unsigned long lock_flags1;
    maPktReqList_t *maPktreq = NULL;
    tx_buffered_packets_t *tx_q_item = NULL;
    bulk_data_param_t bulkdata;
    CsrBool entryFound = FALSE;
    interfacePriv = priv->interfacePriv[interfaceTag];


    if(pkt_cfm->HostTag == interfacePriv->multicastPduHostTag) {
         unifi_trace(priv,UDBG2,"CFM for marked Multicast Tag = %x\n",interfacePriv->multicastPduHostTag);
         interfacePriv->multicastPduHostTag = 0xffffffff;
         resume_suspended_uapsd(priv,interfaceTag);
         resume_unicast_buffered_frames(priv,interfaceTag);
         if(list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames) &&
              list_empty(&interfacePriv->genericMulticastOrBroadCastFrames)) {
            unifi_trace(priv,UDBG1,"Resetting multicastTIM");
            update_tim(priv,0,0,interfaceTag, 0xFFFFFFFF);
        }
        return;
    }

    /* Check if a copy of the same frame (identified by host tag) is queued in driver */
    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    list_for_each_safe(listHeadMaPktreq, placeHolderMaPktreq, &interfacePriv->directedMaPktReq) {
        maPktreq = list_entry(listHeadMaPktreq, maPktReqList_t, q);
        if(maPktreq->hostTag == pkt_cfm->HostTag){
            entryFound = TRUE;
            break;
        }
    }
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);

    if(entryFound){

        /* Monitor the time difference between the MA-PACKET.req and MA-PACKET.cfm */
        unsigned long timeout;
        timeout = (long)jiffies - (long)maPktreq->jiffeTime;

        /* convert into milliseconds */
        timeout = jiffies_to_msecs(timeout);
        unifi_trace(priv, UDBG3, "Jiffies Time: Host Tag(%x) --> Req(%u) Cfm(%u) Diff (in ms): %u\n",maPktreq->hostTag,maPktreq->jiffeTime, jiffies, timeout);

        if( (timeout/1000) > 1)
        {
             unifi_trace(priv, UDBG1, "Confirm time > 2 Seconds: time = %u Status = %x\n", (timeout/1000), pkt_cfm->TransmissionStatus);
        }

       if( CSR_TX_LIFETIME == pkt_cfm->TransmissionStatus  ||
           CSR_TX_BLOCK_ACK_TIMEOUT== pkt_cfm->TransmissionStatus ||
           CSR_TX_FAIL_TRANSMISSION_VIF_INTERRUPTED== pkt_cfm->TransmissionStatus ||
           CSR_TX_REJECTED_PEER_STATION_SLEEPING== pkt_cfm->TransmissionStatus ||
           CSR_TX_REJECTED_DTIM_STARTED== pkt_cfm->TransmissionStatus ){

         CsrWifiRouterCtrlStaInfo_t *staRecord = interfacePriv->staInfo[maPktreq->staHandler];
         unifi_TrafficQueue priority_q;
         struct list_head *list;
         CsrResult result;
         CSR_MA_PACKET_REQUEST *req = &maPktreq->signal.u.MaPacketRequest;
         CsrUint16 ii=0;
         CsrBool locationFound = FALSE;
         CsrUint8 *sigbuffer;

         sigbuffer = (CsrUint8*)&maPktreq->signal;
         if(req->Priority == CSR_MANAGEMENT){
             list = &staRecord->mgtFrames;
             unifi_trace(priv,UDBG5,"mgmt list priority %d\n",req->Priority);
         }
         else{
             priority_q= unifi_frame_priority_to_queue(req->Priority);
             list = &staRecord->dataPdu[priority_q];
             unifi_trace(priv,UDBG5,"data list priority %d\n",req->Priority);
         }

         spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
         list_for_each_safe(listHeadStaQueue, placeHolderStaQueue, list){
             tx_q_item = list_entry(listHeadStaQueue, tx_buffered_packets_t, q);
           COMPARE_HOST_TAG_TO_ENQUEUE(tx_q_item->hostTag ,maPktreq->hostTag)


         }
         if(sigbuffer[SIZEOF_SIGNAL_HEADER + 1]){
            skb_pull(maPktreq->skb,sigbuffer[SIZEOF_SIGNAL_HEADER + 1]);
         }

         /* enqueue the failed packet sta queue*/
         bulkdata.d[0].os_net_buf_ptr= (unsigned char*)maPktreq->skb;
         bulkdata.d[0].os_data_ptr = maPktreq->skb->data;
         bulkdata.d[0].data_length = bulkdata.d[0].net_buf_length = maPktreq->skb->len;
         bulkdata.d[1].os_data_ptr = NULL;
         bulkdata.d[1].os_net_buf_ptr = NULL;
         bulkdata.d[1].data_length = bulkdata.d[0].net_buf_length = 0;
         unifi_trace(priv,UDBG4,"Cfm Fail for HosTag = %x with status %d so requeue it\n",maPktreq->hostTag,pkt_cfm->TransmissionStatus );
         req->TransmissionControl = 0;

         if(!locationFound){

             if(list_empty(list)){
                result = enque_direceted_ma_pkt_cfm_data_pdu(priv, &bulkdata, list,&maPktreq->signal,1);
             }
             else{
                  unifi_trace(priv,UDBG4,"did not find location so add to end of list \n");
                  result = enque_direceted_ma_pkt_cfm_data_pdu(priv, &bulkdata, list,&maPktreq->signal,0);
             }


         }

         else {
            if(ii > 1){
                 unifi_trace(priv,UDBG4,"find the location in the middle of list \n");
                 result = enque_direceted_ma_pkt_cfm_data_pdu(priv, &bulkdata, listHeadStaQueue,&maPktreq->signal,0);

            }
            else{
                unifi_trace(priv,UDBG4," add at begining of list \n");
                result = enque_direceted_ma_pkt_cfm_data_pdu(priv, &bulkdata, list,&maPktreq->signal,1);
            }
         }

         spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);

         /* Increment the counter */
         spin_lock_irqsave(&priv->staRecord_lock,lock_flags1);
         staRecord->noOfPktQueued++;
         spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags1);




         /* after enqueuing update the TIM */
         if(CSR_RESULT_SUCCESS == result){
             if(CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE == staRecord->currentPeerState) {
                if(staRecord->timSet == CSR_WIFI_TIM_RESET || staRecord->timSet == CSR_WIFI_TIM_RESETTING) {
                    if(!staRecord->wmmOrQosEnabled) {
                        unifi_trace(priv, UDBG3, "uf_process_ma_pkt_cfm_for_ap :tim set due to unicast pkt & peer in powersave\n");
                        update_tim(priv,staRecord->aid,1,interfaceTag, staRecord->assignedHandle);
                    }
                    else {
                        /* Check for non delivery enable(i.e trigger enable), all delivery enable & legacy AC for TIM update in firmware */
                        CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
                        /* Check if all AC's are Delivery Enabled */
                        is_all_ac_deliver_enabled_and_moredata(staRecord, &allDeliveryEnabled, &dataAvailable);
                        if (uf_is_more_data_for_non_delivery_ac(staRecord) || (allDeliveryEnabled && dataAvailable)) {
                            update_tim(priv,staRecord->aid,1,interfaceTag, staRecord->assignedHandle);
                        }
                    }
                }
             }
         }
         else{
            dev_kfree_skb(maPktreq->skb);
         }
       }
      else
      {
        CsrWifiRouterCtrlStaInfo_t *staRecord = interfacePriv->staInfo[maPktreq->staHandler];
        if (CSR_TX_RETRY_LIMIT == pkt_cfm->TransmissionStatus ||
            CSR_TX_NO_BSS == pkt_cfm->TransmissionStatus)
        {
            if (staRecord->timSet == CSR_WIFI_TIM_RESET || staRecord->timSet == CSR_WIFI_TIM_RESETTING)
            {
                unifi_trace(priv, UDBG2, "CFM failed with Retry Limit or No BSS --> update TIM\n");
                update_tim(priv, staRecord->aid, 1, interfaceTag, staRecord->assignedHandle);
            }
        }
        else if (CSR_TX_SUCCESSFUL == pkt_cfm->TransmissionStatus)
        {
            staRecord->activity_flag = TRUE;
        }
        unifi_trace(priv, UDBG5, "CFM for HosTag = %x Status = %d, Free SKB reference\n",
                    maPktreq->hostTag,
                    pkt_cfm->TransmissionStatus );

        dev_kfree_skb(maPktreq->skb);

      }
      list_del(listHeadMaPktreq);
      kfree(maPktreq);

    }else{
        /* Check if it is a Confirm for null data frame used
         * for probing station activity
         */
        for(i =0; i < UNIFI_MAX_CONNECTIONS; i++) {
            staRecord = (CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[i]);
            if (staRecord && (staRecord->nullDataHostTag == pkt_cfm->HostTag)) {

                unifi_trace(priv, UDBG1, "CFM for Inactive probe Null frame (tag = %x, status = %d)\n",
                                        pkt_cfm->HostTag,
                                        pkt_cfm->TransmissionStatus
                                        );
                staRecord->nullDataHostTag = INVALID_HOST_TAG;

                if(pkt_cfm->TransmissionStatus == CSR_TX_RETRY_LIMIT){
                    CsrTime now;
                    CsrTime inactive_time;

                    unifi_trace(priv, UDBG1, "Nulldata to probe STA ALIVE Failed with retry limit\n");
                    /* Recheck if there is some activity after null data is sent.
                    *
                    * If still there is no activity then send a disconnected indication
                    * to SME to delete the station record.
                    */
                    if (staRecord->activity_flag){
                        return;
                    }
                    now = CsrTimeGet(NULL);

                    if (staRecord->lastActivity > now)
                    {
                        /* simple timer wrap (for 1 wrap) */
                        inactive_time = CsrTimeAdd((CsrTime)CsrTimeSub(CSR_SCHED_TIME_MAX, staRecord->lastActivity),
                                                   now);
                    }
                    else
                    {
                        inactive_time = (CsrTime)CsrTimeSub(now, staRecord->lastActivity);
                    }

                    if (inactive_time >= STA_INACTIVE_TIMEOUT_VAL)
                    {
                        struct list_head send_cfm_list;
                        CsrUint8 j;

                        /* The SME/NME may be waiting for confirmation for requested frames to this station.
                         * Though this is --VERY UNLIKELY-- in case of station in active mode. But still as a
                         * a defensive check, it loops through buffered frames for this station and if confirmation
                         * is requested, send auto confirmation with failure status. Also flush the frames so
                         * that these are not processed again in PEER_DEL_REQ handler.
                         */
                        INIT_LIST_HEAD(&send_cfm_list);

                        uf_prepare_send_cfm_list_for_queued_pkts(priv,
                                                                 &send_cfm_list,
                                                                 &(staRecord->mgtFrames));

                        uf_flush_list(priv, &(staRecord->mgtFrames));

                        for(j = 0; j < MAX_ACCESS_CATOGORY; j++){
                            uf_prepare_send_cfm_list_for_queued_pkts(priv,
                                                                     &send_cfm_list,
                                                                     &(staRecord->dataPdu[j]));

                            uf_flush_list(priv,&(staRecord->dataPdu[j]));
                        }

                        send_auto_ma_packet_confirm(priv, staRecord->interfacePriv, &send_cfm_list);



                        unifi_warning(priv, "uf_process_ma_pkt_cfm_for_ap: Router Disconnected IND Peer (%x-%x-%x-%x-%x-%x)\n",
                                                 staRecord->peerMacAddress.a[0],
                                                 staRecord->peerMacAddress.a[1],
                                                 staRecord->peerMacAddress.a[2],
                                                 staRecord->peerMacAddress.a[3],
                                                 staRecord->peerMacAddress.a[4],
                                                 staRecord->peerMacAddress.a[5]);

                        CsrWifiRouterCtrlConnectedIndSend(priv->CSR_WIFI_SME_IFACEQUEUE,
                                                          0,
                                                          staRecord->interfacePriv->InterfaceTag,
                                                          staRecord->peerMacAddress,
                                                          CSR_WIFI_ROUTER_CTRL_PEER_DISCONNECTED);
                    }

                }
                else if (pkt_cfm->TransmissionStatus == CSR_TX_SUCCESSFUL)
                {
                     staRecord->activity_flag = TRUE;
                }
            }
        }
    }
}

#endif
CsrUint16 uf_get_vif_identifier (CsrWifiRouterCtrlMode mode, CsrUint16 tag)
{
    switch(mode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_STA:
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PCLI:
            return (0x02<<8|tag);

        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
            return (0x03<<8|tag);

        case CSR_WIFI_ROUTER_CTRL_MODE_IBSS:
            return (0x01<<8|tag);

        case CSR_WIFI_ROUTER_CTRL_MODE_MONITOR:
            return (0x04<<8|tag);
        case CSR_WIFI_ROUTER_CTRL_MODE_AMP:
            return (0x05<<8|tag);
        default:
            return tag;
    }
}

#ifdef CSR_SUPPORT_SME

/*
 * ---------------------------------------------------------------------------
 *  update_macheader
 *
 *
 *      These functions updates mac header for intra BSS packet
 *      routing.
 *      NOTE: This function always has to be called in rx context which
 *      is in bh thread context since GFP_KERNEL is used. In soft IRQ/ Interrupt
 *      context shouldn't be used
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      skb             Socket buffer containing data packet to transmit
 *      newSkb          Socket buffer containing data packet + Mac header if no sufficient headroom in skb
 *      priority        to append QOS control header in Mac header
 *      bulkdata        if newSkb allocated then bulkdata updated to send to unifi
 *      interfaceTag    the interfaceID on which activity going on
 *      macHeaderLengthInBytes no. of bytes of mac header in received frame
 *      qosDestination  used to append Qos control field
 *
 *  Returns:
 *      Zero on success or -1 on error.
 * ---------------------------------------------------------------------------
 */

static int update_macheader(unifi_priv_t *priv, struct sk_buff *skb,
                            struct sk_buff *newSkb, CSR_PRIORITY *priority,
                            bulk_data_param_t *bulkdata, CsrUint16 interfaceTag,
                            CsrUint8 macHeaderLengthInBytes,
                            CsrUint8 qosDestination)
{

    CsrUint16 *fc = NULL;
    CsrUint8 direction = 0, toDs, fromDs;
    CsrUint8 *bufPtr = NULL;
    CsrUint8 sa[ETH_ALEN], da[ETH_ALEN];
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    int headroom;
    CsrUint8 macHeaderBuf[IEEE802_11_DATA_FRAME_MAC_HEADER_SIZE] = {0};

    unifi_trace(priv, UDBG5, "entering the update_macheader function\n");

    /* temporary buffer for the Mac header storage */
    memcpy(macHeaderBuf, skb->data, macHeaderLengthInBytes);

    /* remove the Macheader from the skb */
    skb_pull(skb, macHeaderLengthInBytes);

    /* get the skb headroom for skb_push check */
    headroom = skb_headroom(skb);

    /*  pointer to frame control field */
    fc = (CsrUint16*) macHeaderBuf;

    toDs = (*fc & cpu_to_le16(IEEE802_11_FC_TO_DS_MASK))?1 : 0;
    fromDs = (*fc & cpu_to_le16(IEEE802_11_FC_FROM_DS_MASK))? 1: 0;
    unifi_trace(priv, UDBG5, "In update_macheader function, fromDs = %x, toDs = %x\n", fromDs, toDs);
    direction = ((fromDs | (toDs << 1)) & 0x3);

    /* Address1 or 3 from the macheader */
    memcpy(da, macHeaderBuf+4+toDs*12, ETH_ALEN);
    /* Address2, 3 or 4 from the mac header */
    memcpy(sa, macHeaderBuf+10+fromDs*(6+toDs*8), ETH_ALEN);

    unifi_trace(priv, UDBG3, "update_macheader:direction = %x\n", direction);
    /* update the toDs, fromDs & address fields in Mac header */
    switch(direction)
    {
        case 2:
            /* toDs = 1 & fromDs = 0 , toAp when frames received from peer
             * while sending this packet to Destination the Mac header changed
             * as fromDs = 1 & toDs = 0, fromAp
             */
            *fc &= cpu_to_le16(~IEEE802_11_FC_TO_DS_MASK);
            *fc |= cpu_to_le16(IEEE802_11_FC_FROM_DS_MASK);
            /* Address1: MAC address of the actual destination (4 = 2+2) */
            memcpy(macHeaderBuf + 4, da, ETH_ALEN);
            /* Address2: The MAC address of the AP (10 = 2+2+6) */
            memcpy(macHeaderBuf + 10, &interfacePriv->bssid, ETH_ALEN);
            /* Address3: MAC address of the actual source from mac header (16 = 2+2+6+6) */
            memcpy(macHeaderBuf + 16, sa, ETH_ALEN);
            break;
        case 3:
            unifi_trace(priv, UDBG3, "when both the toDs & fromDS set, NOT SUPPORTED\n");
            break;
        default:
            unifi_trace(priv, UDBG3, "problem in decoding packet in update_macheader \n");
            return -1;
    }

    /* frameType is Data always, Validation is done before calling this function */

    /* check for the souce station type */
    switch(le16_to_cpu(*fc) & IEEE80211_FC_SUBTYPE_MASK)
    {
        case IEEE802_11_FC_TYPE_QOS_DATA & IEEE80211_FC_SUBTYPE_MASK:
            /* No need to modify the qos control field */
            if (!qosDestination) {

                /* If source Sta is QOS enabled & if this bit set, then HTC is supported by
                 * peer station & htc field present in macHeader
                 */
                if (*fc & cpu_to_le16(IEEE80211_FC_ORDER_MASK)) {
                    /* HT control field present in Mac header
                     * 6 = sizeof(qosControl) + sizeof(htc)
                     */
                    macHeaderLengthInBytes -= 6;
                } else {
                    macHeaderLengthInBytes -= 2;
                }
                /* Destination STA is non qos so change subtype to DATA */
                *fc &= cpu_to_le16(~IEEE80211_FC_SUBTYPE_MASK);
                *fc |= cpu_to_le16(IEEE802_11_FC_TYPE_DATA);
                /* remove the qos control field & HTC(if present). new macHeaderLengthInBytes is less than old
                 * macHeaderLengthInBytes so no need to verify skb headroom
                 */
                if (headroom < macHeaderLengthInBytes) {
                    unifi_trace(priv, UDBG1, " sufficient headroom not there to push updated mac header \n");
                    return -1;
                }
                bufPtr = (CsrUint8 *) skb_push(skb, macHeaderLengthInBytes);

                /*  update bulk data os_data_ptr */
                bulkdata->d[0].os_data_ptr = skb->data;
                bulkdata->d[0].os_net_buf_ptr = (unsigned char*)skb;
                bulkdata->d[0].data_length = skb->len;

            } else {
                /* pointing to QOS control field */
                CsrUint8 qc;
                if (*fc & cpu_to_le16(IEEE80211_FC_ORDER_MASK)) {
                    qc = *((CsrUint8*)(macHeaderBuf + (macHeaderLengthInBytes - 4 - 2)));
                } else {
                    qc = *((CsrUint8*)(macHeaderBuf + (macHeaderLengthInBytes - 2)));
                }

                if ((qc & IEEE802_11_QC_TID_MASK) > 7) {
                    *priority = 7;
                } else {
                    *priority = qc & IEEE802_11_QC_TID_MASK;
                }

                unifi_trace(priv, UDBG1, "Incoming packet priority from QSTA is %x\n", *priority);

                if (headroom < macHeaderLengthInBytes) {
                    unifi_trace(priv, UDBG3, " sufficient headroom not there to push updated mac header \n");
                    return -1;
                }
                bufPtr = (CsrUint8 *) skb_push(skb, macHeaderLengthInBytes);
            }
            break;
        default:
            {
                bulk_data_param_t data_ptrs;
                CsrResult csrResult;
                unifi_trace(priv, UDBG5, "normal Data packet, NO QOS \n");

                *priority = CSR_CONTENTION;
                if (qosDestination) {
                    CsrUint8 qc = 0;
                    unifi_trace(priv, UDBG3, "destination is QOS station \n");
                    /* prepare the qos control field */

                    qc |= CSR_QOS_UP0;

                    /* no Amsdu is in ap buffer so eosp is left 0 */

                    if (da[0] & 0x1) {
                        /* multicast/broadcast frames, no acknowledgement needed */
                        qc |= 1 << 5;
                    }

                    /* update new Mac header Length with 2 = sizeof(qos control) */
                    macHeaderLengthInBytes += 2;

                    /* received DATA frame but destiantion is QOS station so update subtype to QOS*/
                    *fc &= cpu_to_le16(~IEEE80211_FC_SUBTYPE_MASK);
                    *fc |= cpu_to_le16(IEEE802_11_FC_TYPE_QOS_DATA);

                    /* appendQosControlOffset = macHeaderLengthInBytes - 2, since source sta is not QOS */
                    macHeaderBuf[macHeaderLengthInBytes - 2] = qc;
                    /* txopLimit is 0 */
                    macHeaderBuf[macHeaderLengthInBytes - 1] = 0;
                    if (headroom < macHeaderLengthInBytes) {
                        csrResult = unifi_net_data_malloc(priv, &data_ptrs.d[0], skb->len + macHeaderLengthInBytes);

                        if (csrResult != CSR_RESULT_SUCCESS) {
                            unifi_error(priv, " failed to allocate request_data. in update_macheader func\n");
                            return -1;
                        }
                        newSkb = (struct sk_buff *)(data_ptrs.d[0].os_net_buf_ptr);
                        newSkb->len = skb->len + macHeaderLengthInBytes;

                        memcpy((void*)data_ptrs.d[0].os_data_ptr + macHeaderLengthInBytes,
                                skb->data, skb->len);

                        bulkdata->d[0].os_data_ptr = newSkb->data;
                        bulkdata->d[0].os_net_buf_ptr = (unsigned char*)newSkb;
                        bulkdata->d[0].data_length = newSkb->len;

                        bufPtr = (CsrUint8*)data_ptrs.d[0].os_data_ptr;

                        /* The old skb will not be used again */
                        kfree_skb(skb);
                    } else {
                        /* skb headroom is sufficient to append Macheader */
                        bufPtr = (CsrUint8*)skb_push(skb, macHeaderLengthInBytes);
                        bulkdata->d[0].os_data_ptr = skb->data;
                        bulkdata->d[0].os_net_buf_ptr = (unsigned char*)skb;
                        bulkdata->d[0].data_length = skb->len;
                    }
                } else {
                    unifi_trace(priv, UDBG3, "destination is not a QSTA\n");
                    if (headroom < macHeaderLengthInBytes) {
                        csrResult = unifi_net_data_malloc(priv, &data_ptrs.d[0], skb->len + macHeaderLengthInBytes);

                        if (csrResult != CSR_RESULT_SUCCESS) {
                            unifi_error(priv, " failed to allocate request_data. in update_macheader func\n");
                            return -1;
                        }
                        newSkb = (struct sk_buff *)(data_ptrs.d[0].os_net_buf_ptr);
                        newSkb->len = skb->len + macHeaderLengthInBytes;

                        memcpy((void*)data_ptrs.d[0].os_data_ptr + macHeaderLengthInBytes,
                                skb->data, skb->len);

                        bulkdata->d[0].os_data_ptr = newSkb->data;
                        bulkdata->d[0].os_net_buf_ptr = (unsigned char*)newSkb;
                        bulkdata->d[0].data_length = newSkb->len;

                        bufPtr = (CsrUint8*)data_ptrs.d[0].os_data_ptr;

                        /* The old skb will not be used again */
                        kfree_skb(skb);
                    } else {
                        /* skb headroom is sufficient to append Macheader */
                        bufPtr = (CsrUint8*)skb_push(skb, macHeaderLengthInBytes);
                        bulkdata->d[0].os_data_ptr = skb->data;
                        bulkdata->d[0].os_net_buf_ptr = (unsigned char*)skb;
                        bulkdata->d[0].data_length = skb->len;
                    }
                }
            }
    }

    /* prepare the complete skb, by pushing the MAC header to the begining of the skb->data */
    unifi_trace(priv, UDBG5, "updated Mac Header: %d \n",macHeaderLengthInBytes);
    memcpy(bufPtr, macHeaderBuf, macHeaderLengthInBytes);

    unifi_trace(priv, UDBG5, "leaving the update_macheader function\n");
    return 0;
}
/*
 * ---------------------------------------------------------------------------
 *  uf_ap_process_data_pdu
 *
 *
 *      Takes care of intra BSS admission control & routing packets within BSS
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      skb             Socket buffer containing data packet to transmit
 *      ehdr            ethernet header to fetch priority of packet
 *      srcStaInfo      source stations record for connection verification
 *      packed_signal
 *      signal_len
 *      signal          MA-PACKET.indication signal
 *      bulkdata        if newSkb allocated then bulkdata updated to send to unifi
 *      macHeaderLengthInBytes no. of bytes of mac header in received frame
 *
 *  Returns:
 *      Zero on success(ap processing complete) or -1 if packet also have to be sent to NETDEV.
 * ---------------------------------------------------------------------------
 */
int
uf_ap_process_data_pdu(unifi_priv_t *priv, struct sk_buff *skb,
                       struct ethhdr *ehdr, CsrWifiRouterCtrlStaInfo_t * srcStaInfo,
                       const CSR_SIGNAL *signal,
                       bulk_data_param_t *bulkdata,
                       CsrUint8 macHeaderLengthInBytes)
{
    const CSR_MA_PACKET_INDICATION *ind = &(signal->u.MaPacketIndication);
    CsrUint16 interfaceTag = (ind->VirtualInterfaceIdentifier & 0x00ff);
    struct sk_buff *newSkb = NULL;
    /* pointer to skb or private skb created using skb_copy() */
    struct sk_buff *skbPtr = skb;
    CsrBool sendToNetdev = FALSE;
    CsrBool qosDestination = FALSE;
    CSR_PRIORITY priority = CSR_CONTENTION;
    CsrWifiRouterCtrlStaInfo_t *dstStaInfo = NULL;
    netInterface_priv_t *interfacePriv;

    unifi_trace(priv, UDBG5, "entering  uf_ap_process_data_pdu %d\n",macHeaderLengthInBytes);
    /* InterfaceTag validation from MA_PACKET.indication */
    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_trace(priv, UDBG1, "Interface Tag is Invalid in uf_ap_process_data_pdu\n");
        unifi_net_data_free(priv, &bulkdata->d[0]);
        return 0;
    }
    interfacePriv = priv->interfacePriv[interfaceTag];

    if((interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_P2PGO) &&
       (interfacePriv->intraBssEnabled == FALSE)) {
        unifi_trace(priv, UDBG2, "uf_ap_process_data_pdu:P2P GO intrabssEnabled?= %d\n", interfacePriv->intraBssEnabled);

        /*In P2P GO case, if intraBSS distribution Disabled then don't do IntraBSS routing */
        /* If destination in our BSS then drop otherwise give packet to netdev */
        dstStaInfo = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, ehdr->h_dest, interfaceTag);
        if (dstStaInfo) {
            unifi_net_data_free(priv, &bulkdata->d[0]);
            return 0;
        }
        /* May be associated P2PCLI trying to send the packets on backbone (Netdev) */
        return -1;
    }

    if(!memcmp(ehdr->h_dest, interfacePriv->bssid.a, ETH_ALEN)) {
        /* This packet will be given to the TCP/IP stack since this packet is for us(AP)
         * No routing needed */
        unifi_trace(priv, UDBG4, "destination address is csr_ap\n");
        return -1;
    }

    /* fetch the destination record from staion record database */
    dstStaInfo = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, ehdr->h_dest, interfaceTag);

    /* AP mode processing, & if packet is unicast */
    if(!dstStaInfo) {
        if (!(ehdr->h_dest[0] & 0x1)) {
            /* destination not in station record & its a unicast packet, so pass the packet to network stack */
            unifi_trace(priv, UDBG3, "unicast frame & destination record not exist, send to netdev proto = %x\n", htons(skb->protocol));
            return -1;
        } else {
            /* packet is multicast/broadcast */
            /* copy the skb to skbPtr, send skb to netdev & skbPtr to multicast/broad cast list */
            unifi_trace(priv, UDBG5, "skb_copy, in  uf_ap_process_data_pdu, protocol = %x\n", htons(skb->protocol));
            skbPtr = skb_copy(skb, GFP_KERNEL);
            if(skbPtr == NULL) {
                /* We don't have memory to don't send the frame in BSS*/
                unifi_notice(priv, "broacast/multicast frame can't be sent in BSS No memeory: proto = %x\n", htons(skb->protocol));
                return -1;
            }
            sendToNetdev = TRUE;
        }
    } else {

        /* validate the Peer & Destination Station record */
        if (uf_process_station_records_for_sending_data(priv, interfaceTag, srcStaInfo, dstStaInfo)) {
            unifi_notice(priv, "uf_ap_process_data_pdu: station record validation failed \n");
            interfacePriv->stats.rx_errors++;
            unifi_net_data_free(priv, &bulkdata->d[0]);
            return 0;
        }
    }

    /* BroadCast packet received and it's been sent as non QOS packets.
     * Since WMM spec not mandates broadcast/multicast to be sent as QOS data only,
     * if all Peers are QSTA
     */
    if(sendToNetdev) {
       /* BroadCast packet and it's been sent as non QOS packets */
        qosDestination = FALSE;
    } else if(dstStaInfo && (dstStaInfo->wmmOrQosEnabled == TRUE)) {
          qosDestination = TRUE;
    }

    unifi_trace(priv, UDBG3, "uf_ap_process_data_pdu QoS destination  = %s\n", (qosDestination)? "TRUE": "FALSE");

    /* packet is allowed to send to unifi, update the Mac header */
    if (update_macheader(priv, skbPtr, newSkb, &priority, bulkdata, interfaceTag, macHeaderLengthInBytes, qosDestination)) {
        interfacePriv->stats.rx_errors++;
        unifi_notice(priv, "(Packet Drop) failed to update the Mac header in uf_ap_process_data_pdu\n");
        if (sendToNetdev) {
            /*  Free's the skb_copy(skbPtr) data since packet processing failed */
            bulkdata->d[0].os_data_ptr = skbPtr->data;
            bulkdata->d[0].os_net_buf_ptr = (unsigned char*)skbPtr;
            bulkdata->d[0].data_length = skbPtr->len;
            unifi_net_data_free(priv, &bulkdata->d[0]);
        }
        return -1;
    }

    unifi_trace(priv, UDBG3, "Mac Header updated...calling uf_process_ma_packet_req \n");

    /* Packet is ready to send to unifi ,transmissionControl = 0x0004, confirmation is not needed for data packets */
    if (uf_process_ma_packet_req(priv,  ehdr->h_dest, 0xffffffff, interfaceTag, CSR_NO_CONFIRM_REQUIRED, (CSR_RATE)0,priority, priv->netdev_client->sender_id, bulkdata)) {
        if (sendToNetdev) {
            unifi_trace(priv, UDBG1, "In uf_ap_process_data_pdu, (Packet Drop) uf_process_ma_packet_req failed. freeing skb_copy data (original data sent to Netdev)\n");
            /*  Free's the skb_copy(skbPtr) data since packet processing failed */
            bulkdata->d[0].os_data_ptr = skbPtr->data;
            bulkdata->d[0].os_net_buf_ptr = (unsigned char*)skbPtr;
            bulkdata->d[0].data_length = skbPtr->len;
            unifi_net_data_free(priv, &bulkdata->d[0]);
        } else {
            /* This free's the skb data */
            unifi_trace(priv, UDBG1, "In uf_ap_process_data_pdu, (Packet Drop). Unicast data so freeing original skb \n");
            unifi_net_data_free(priv, &bulkdata->d[0]);
        }
    }
    unifi_trace(priv, UDBG5, "leaving  uf_ap_process_data_pdu\n");

    if (sendToNetdev) {
        /* The packet is multicast/broadcast, so after AP processing packet has to
         * be sent to netdev, if peer port state is open
        */
        unifi_trace(priv, UDBG4, "Packet will be routed to NetDev\n");
        return -1;
    }
    /* Ap handled the packet & its a unicast packet, no need to send to netdev */
    return 0;
}

#endif

CsrResult uf_process_ma_packet_req(unifi_priv_t *priv,
                                   CsrUint8 *peerMacAddress,
                                   CSR_CLIENT_TAG hostTag,
                                   CsrUint16 interfaceTag,
                                   CSR_TRANSMISSION_CONTROL transmissionControl,
                                   CSR_RATE TransmitRate,
                                   CSR_PRIORITY priority,
                                   CSR_PROCESS_ID leSenderProcessId,
                                   bulk_data_param_t *bulkdata)
{
    CsrResult status = CSR_RESULT_SUCCESS;
    CSR_SIGNAL signal;
    int result;
#ifdef CSR_SUPPORT_SME
   CsrWifiRouterCtrlStaInfo_t *staRecord = NULL;
    const CsrUint8 *macHdrLocation =  bulkdata->d[0].os_data_ptr;
    CsrWifiPacketType pktType;
    int frameType = 0;
    CsrBool queuePacketDozing = FALSE;
    CsrUint32 priority_q;
    CsrUint16 frmCtrl;
    struct list_head * list = NULL; /* List to which buffered PDUs are to be enqueued*/
    CsrBool setBcTim=FALSE;
    netInterface_priv_t *interfacePriv;
    CsrBool requeueOnSamePos = FALSE;
    CsrUint32 handle = 0xFFFFFFFF;
    unsigned long lock_flags;

    UF_TRACE_MAC(priv, UDBG5, "entering uf_process_ma_packet_req, peer: ", peerMacAddress);

    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_error(priv, "interfaceTag >= CSR_WIFI_NUM_INTERFACES, interfacetag = %d\n", interfaceTag);
        return CSR_RESULT_FAILURE;
    }
    interfacePriv = priv->interfacePriv[interfaceTag];


    /* fetch the station record for corresponding peer mac address */
    if ((staRecord = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, peerMacAddress, interfaceTag))) {
        handle = staRecord->assignedHandle;
    }

    /* Frame ma-packet.req, this is saved/transmitted depend on queue state */
    unifi_frame_ma_packet_req(priv, priority, TransmitRate, hostTag,
                              interfaceTag, transmissionControl, leSenderProcessId,
                              peerMacAddress, &signal);

   /* Since it's common path between STA & AP mode, in case of STA packet
     * need not to be queued but in AP case we have to queue PDU's in
     * different scenarios
     */
    switch(interfacePriv->interfaceMode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
            /* For this mode processing done below */
            break;
        default:
            /* In case of STA/IBSS/P2PCLI/AMP, no checks needed send the packet down & return */
            unifi_trace(priv, UDBG5, "In %s, interface mode is %x \n", __FUNCTION__, interfacePriv->interfaceMode);
            if (interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_NONE) {
                unifi_warning(priv, "In %s, interface mode NONE \n", __FUNCTION__);
            }
            if ((result = ul_send_signal_unpacked(priv, &signal, bulkdata))) {
                status = CSR_RESULT_FAILURE;
            }
            return status;
    }

    /* -----Only AP/P2pGO mode handling falls below----- */

    /* convert priority to queue */
    priority_q = unifi_frame_priority_to_queue((CSR_PRIORITY) priority);

    /* check the powersave status of the peer */
    if (staRecord && (staRecord->currentPeerState ==
                     CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)) {
        /* Peer is dozing & packet have to be delivered, so buffer the packet &
         * update the TIM
         */
        queuePacketDozing = TRUE;
    }

    /* find the type of frame unicast or mulicast/broadcast */
    if (*peerMacAddress & 0x1) {
        /* Multicast/broadCast data are always triggered by vif_availability.ind
         * at the DTIM
         */
        pktType = CSR_WIFI_MULTICAST_PDU;
    } else {
        pktType = CSR_WIFI_UNICAST_PDU;
    }

    /* Fetch the frame control field from mac header & check for frame type */
    frmCtrl = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(macHdrLocation);

    /* Processing done according to Frame/Packet type */
    frameType =  ((frmCtrl & 0x000c) >> FRAME_CONTROL_TYPE_FIELD_OFFSET);
    switch(frameType)
    {
        case IEEE802_11_FRAMETYPE_MANAGEMENT:

            switch(pktType)
            {
                case CSR_WIFI_UNICAST_PDU:
                    unifi_trace(priv, UDBG5, "management unicast PDU in uf_process_ma_packet_req \n");
                    /* push the packet in to the queue with appropriate mgt list */
                    if (!staRecord) {
                        /* push the packet to the unifi if list is empty (if packet lost how to re-enque) */
                        if (list_empty(&interfacePriv->genericMgtFrames)) {
#ifdef CSR_SUPPORT_SME
                            if(!(IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag))) {
#endif

                            unifi_trace(priv, UDBG3, "genericMgtFrames list is empty uf_process_ma_packet_req \n");
                            result = ul_send_signal_unpacked(priv, &signal, bulkdata);
                            /*  reque only on ENOSPC */
                            if(result == -ENOSPC) {
                                /* requeue the failed packet to genericMgtFrame with same position */
                                unifi_trace(priv, UDBG1, "(ENOSPC) Sending genericMgtFrames Failed so buffering\n");
                                list = &interfacePriv->genericMgtFrames;
                                requeueOnSamePos = TRUE;
                            }
#ifdef CSR_SUPPORT_SME
                            }else{
                                list = &interfacePriv->genericMgtFrames;
                                unifi_trace(priv, UDBG3, "genericMgtFrames queue empty and dtim started\n hosttag is 0x%x,\n",signal.u.MaPacketRequest.HostTag);
                                update_eosp_to_head_of_broadcast_list_head(priv,interfaceTag);
                           }
#endif
                        } else {
                            /* Queue the packet to genericMgtFrame of unifi_priv_t data structure */
                            list = &interfacePriv->genericMgtFrames;
                            unifi_trace(priv, UDBG2, "genericMgtFrames queue not empty\n");
                        }
                    } else {
                        /* check peer power state */
                        if (queuePacketDozing || !list_empty(&staRecord->mgtFrames) || IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag)) {
                            /* peer is in dozing mode, so queue packet in mgt frame list of station record */
                           /*if multicast traffic is going on, buffer the unicast packets*/
                            list = &staRecord->mgtFrames;

                            unifi_trace(priv, UDBG1, "staRecord->MgtFrames list empty? = %s, handle = %d, queuePacketDozing = %d\n",
                                        (list_empty(&staRecord->mgtFrames))? "YES": "NO", staRecord->assignedHandle, queuePacketDozing);
                            if(IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag)){
                                update_eosp_to_head_of_broadcast_list_head(priv,interfaceTag);
                            }

                        } else {
                            unifi_trace(priv, UDBG5, "staRecord->mgtFrames list is empty uf_process_ma_packet_req \n");
                            result = ul_send_signal_unpacked(priv, &signal, bulkdata);
                            if(result == -ENOSPC) {
                                /* requeue the failed packet to staRecord->mgtFrames with same position */
                                list = &staRecord->mgtFrames;
                                requeueOnSamePos = TRUE;
                                unifi_trace(priv, UDBG1, "(ENOSPC) Sending MgtFrames Failed handle = %d so buffering\n",staRecord->assignedHandle);
                                priv->pausedStaHandle[0]=(CsrUint8)(staRecord->assignedHandle);
                            } else if (result) {
                                status = CSR_RESULT_FAILURE;
                            }
                        }
                    }
                    break;
                case CSR_WIFI_MULTICAST_PDU:
                    unifi_trace(priv, UDBG5, "management multicast/broadcast PDU in uf_process_ma_packet_req 'QUEUE it' \n");
                    /* Queue the packet to genericMulticastOrBroadCastMgtFrames of unifi_priv_t data structure
                     * will be sent when we receive VIF AVAILABILITY from firmware as part of DTIM
                     */

                    list = &interfacePriv->genericMulticastOrBroadCastMgtFrames;
                    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                    interfacePriv->noOfbroadcastPktQueued++;
                    spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                    if((interfacePriv->interfaceMode != CSR_WIFI_ROUTER_CTRL_MODE_IBSS) &&
                            (list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames))) {
                        setBcTim=TRUE;
                    }
                    break;
                default:
                    unifi_error(priv, "condition never meets: packet type unrecognized\n");
            }
            break;
        case IEEE802_11_FRAMETYPE_DATA:
            switch(pktType)
            {
                case CSR_WIFI_UNICAST_PDU:
                    unifi_trace(priv, UDBG5, "data unicast PDU in uf_process_ma_packet_req \n");
                    /* check peer power state, list status & peer port status */
                    if(!staRecord) {
                        unifi_error(priv, "In %s unicast but staRecord = NULL\n", __FUNCTION__);
                        return CSR_RESULT_FAILURE;
                    } else if (queuePacketDozing || isRouterBufferEnabled(priv,priority_q)|| !list_empty(&staRecord->dataPdu[priority_q]) || IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag)) {
                        /* peer is in dozing mode, so queue packet in mgt frame list of station record */
                        /* if multicast traffic is going on, buffet the unicast packets */
                        unifi_trace(priv, UDBG2, "Enqueued to staRecord->dataPdu[%d] queuePacketDozing=%d,\
                                Buffering enabled = %d \n", priority_q,queuePacketDozing,isRouterBufferEnabled(priv,priority_q));
                        signal.u.MaPacketRequest.TransmissionControl &= ~(CSR_NO_CONFIRM_REQUIRED);
                        list = &staRecord->dataPdu[priority_q];
                    } else {
                        unifi_trace(priv, UDBG5, "staRecord->dataPdu[%d] list is empty uf_process_ma_packet_req \n", priority_q);
                        signal.u.MaPacketRequest.TransmissionControl &= ~(CSR_NO_CONFIRM_REQUIRED);
                        /* Pdu allowed to send to unifi */
                        result = ul_send_signal_unpacked(priv, &signal, bulkdata);
                        if(result == -ENOSPC) {
                            /* requeue the failed packet to staRecord->dataPdu[priority_q] with same position */
                            unifi_trace(priv, UDBG1, "(ENOSPC) Sending Unicast DataPDU to queue %d Failed so buffering\n",priority_q);
                            requeueOnSamePos = TRUE;
                            list = &staRecord->dataPdu[priority_q];
                            priv->pausedStaHandle[priority_q]=(CsrUint8)(staRecord->assignedHandle);
                            if(!isRouterBufferEnabled(priv,priority_q)) {
                                unifi_error(priv,"Buffering Not enabled for queue %d \n",priority_q);
                            }
                        } else if (result) {
                            status = CSR_RESULT_FAILURE;
                        }
                    }
                    break;
                case CSR_WIFI_MULTICAST_PDU:
                    unifi_trace(priv, UDBG5, "data multicast/broadcast PDU in uf_process_ma_packet_req \n");
                    /* Queue the packet to genericMulticastOrBroadCastFrames list of unifi_priv_t data structure
                     * will be sent when we receive VIF AVAILABILITY from firmware as part of DTIM
                     */
                    list = &interfacePriv->genericMulticastOrBroadCastFrames;
                    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                    interfacePriv->noOfbroadcastPktQueued++;
                    spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                    if(list_empty(&interfacePriv->genericMulticastOrBroadCastFrames)) {
                        setBcTim = TRUE;
                    }
                    break;
                default:
                    unifi_error(priv, "condition never meets: packet type un recognized\n");
            }
            break;
        default:
            unifi_error(priv, "unrecognized frame type\n");
    }
    if(list) {
        status = enque_tx_data_pdu(priv, bulkdata,list, &signal,requeueOnSamePos);
        /* Record no. of packet queued for each peer */
        if (staRecord && (pktType == CSR_WIFI_UNICAST_PDU) && (!status)) {
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            staRecord->noOfPktQueued++;
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
        }
    }
    if(setBcTim) {
        unifi_trace(priv, UDBG3, "tim set due to broadcast pkt\n");
        update_tim(priv,0,1,interfaceTag, handle);
    } else if(staRecord && staRecord->currentPeerState ==
                            CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE) {
        if(staRecord->timSet == CSR_WIFI_TIM_RESET || staRecord->timSet == CSR_WIFI_TIM_RESETTING) {
            if(!staRecord->wmmOrQosEnabled) {
                if(!list_empty(&staRecord->mgtFrames) ||
                   !list_empty(&staRecord->dataPdu[3]) ||
                   !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION])) {
                    unifi_trace(priv, UDBG3, "tim set due to unicast pkt & peer in powersave\n");
                    update_tim(priv,staRecord->aid,1,interfaceTag, handle);
                }
            } else {
                /* Check for non delivery enable(i.e trigger enable), all delivery enable & legacy AC for TIM update in firmware */
                CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
                /* Check if all AC's are Delivery Enabled */
                is_all_ac_deliver_enabled_and_moredata(staRecord, &allDeliveryEnabled, &dataAvailable);
                if (uf_is_more_data_for_non_delivery_ac(staRecord) || (allDeliveryEnabled && dataAvailable)) {
                    update_tim(priv,staRecord->aid,1,interfaceTag, handle);
                }
            }
        }
    }

    if((list) && (pktType == CSR_WIFI_UNICAST_PDU && !queuePacketDozing) && !(isRouterBufferEnabled(priv,priority_q)) && !(IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag))) {
        unifi_trace(priv, UDBG2, "buffering cleared for queue = %d So resending buffered frames\n",priority_q);
        uf_send_buffered_frames(priv, priority_q);
    }
    unifi_trace(priv, UDBG5, "leaving uf_process_ma_packet_req \n");
    return status;
#else
#ifdef CSR_NATIVE_LINUX
    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_error(priv, "interfaceTag >= CSR_WIFI_NUM_INTERFACES, interfacetag = %d\n", interfaceTag);
        return CSR_RESULT_FAILURE;
    }
    /* Frame ma-packet.req, this is saved/transmitted depend on queue state */
    unifi_frame_ma_packet_req(priv, priority, TransmitRate, hostTag, interfaceTag,
            transmissionControl, leSenderProcessId,
            peerMacAddress, &signal);
    result = ul_send_signal_unpacked(priv, &signal, bulkdata);
    if (result) {
        return CSR_RESULT_FAILURE;
    }
#endif
    return status;
#endif
}

#ifdef CSR_SUPPORT_SME
CsrInt8 uf_get_protection_bit_from_interfacemode(unifi_priv_t *priv, CsrUint16 interfaceTag, const CsrUint8 *daddr)
{
    CsrInt8 protection = 0;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];

    switch(interfacePriv->interfaceMode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_STA:
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PCLI:
        case CSR_WIFI_ROUTER_CTRL_MODE_AMP:
        case CSR_WIFI_ROUTER_CTRL_MODE_IBSS:
            protection = interfacePriv->protect;
            break;
        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
            {
                CsrWifiRouterCtrlStaInfo_t *dstStaInfo = NULL;
                if (daddr[0] & 0x1) {
                    unifi_trace(priv, UDBG3, "broadcast/multicast packet in send_ma_pkt_request\n");
                    /* In this mode, the protect member of priv structure has an information of how
                     * AP/P2PGO has started, & the member updated in set mode request for AP/P2PGO
                     */
                    protection = interfacePriv->protect;
                } else {
                    /* fetch the destination record from staion record database */
                    dstStaInfo = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, daddr, interfaceTag);
                    if (!dstStaInfo) {
                        unifi_trace(priv, UDBG3, "peer not found in station record in send_ma_pkt_request\n");
                        return -1;
                    }
                    protection = dstStaInfo->protection;
                }
            }
            break;
        default:
            unifi_trace(priv, UDBG2, "mode unknown in send_ma_pkt_request\n");
    }
    return protection;
}
#endif
#ifdef CSR_SUPPORT_SME
CsrUint8 send_multicast_frames(unifi_priv_t *priv, CsrUint16 interfaceTag)
{
    int r;
    tx_buffered_packets_t * buffered_pkt = NULL;
    CsrBool moreData = FALSE;
    CsrUint8 pduSent =0;
    unsigned long lock_flags;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CsrUint32 hostTag = 0xffffffff;

    func_enter();
    if(!isRouterBufferEnabled(priv,UNIFI_TRAFFIC_Q_VO)) {
        while((interfacePriv->dtimActive)&& (buffered_pkt=dequeue_tx_data_pdu(priv,&interfacePriv->genericMulticastOrBroadCastMgtFrames))) {
            buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK);
            moreData = (buffered_pkt->transmissionControl & TRANSMISSION_CONTROL_ESOP_MASK)?FALSE:TRUE;


            unifi_trace(priv,UDBG2,"DTIM Occurred for interface:sending Mgt packet %d\n",interfaceTag);

            if((r=frame_and_send_queued_pdu(priv,buffered_pkt,NULL,moreData,FALSE)) == -ENOSPC) {
               unifi_trace(priv,UDBG1,"frame_and_send_queued_pdu failed with ENOSPC for host tag = %x\n", buffered_pkt->hostTag);
               /* Enqueue at the head of the queue */
               spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
               list_add(&buffered_pkt->q, &interfacePriv->genericMulticastOrBroadCastMgtFrames);
               spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
               break;
            } else {
                unifi_trace(priv,UDBG1,"send_multicast_frames: Send genericMulticastOrBroadCastMgtFrames (%x, %x)\n",
                                        buffered_pkt->hostTag,
                                        r);
                if(r) {
                   unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                }
                if(!moreData) {

                    interfacePriv->dtimActive = FALSE;
                    if(!r) {
                        hostTag = buffered_pkt->hostTag;
                        pduSent++;
                    } else {
                        send_vif_availibility_rsp(priv,uf_get_vif_identifier(interfacePriv->interfaceMode,interfaceTag),CSR_RC_UNSPECIFIED_FAILURE);
                    }
                }
                /* Buffered frame sent successfully */
                spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                interfacePriv->noOfbroadcastPktQueued--;
                spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                kfree(buffered_pkt);
           }

        }
    }
    if(!isRouterBufferEnabled(priv,UNIFI_TRAFFIC_Q_CONTENTION)) {
        while((interfacePriv->dtimActive)&& (buffered_pkt=dequeue_tx_data_pdu(priv,&interfacePriv->genericMulticastOrBroadCastFrames))) {
            buffered_pkt->transmissionControl |= TRANSMISSION_CONTROL_TRIGGER_MASK;
            moreData = (buffered_pkt->transmissionControl & TRANSMISSION_CONTROL_ESOP_MASK)?FALSE:TRUE;


            if((r=frame_and_send_queued_pdu(priv,buffered_pkt,NULL,moreData,FALSE)) == -ENOSPC) {
                /* Clear the trigger bit transmission control*/
                buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK);
                /* Enqueue at the head of the queue */
                spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                list_add(&buffered_pkt->q, &interfacePriv->genericMulticastOrBroadCastFrames);
                spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                break;
            } else {
                if(r) {
                    unifi_trace(priv,UDBG1,"send_multicast_frames: Send genericMulticastOrBroadCastFrame failed (%x, %x)\n",
                                            buffered_pkt->hostTag,
                                            r);
                    unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                }
                if(!moreData) {
                    interfacePriv->dtimActive = FALSE;
                    if(!r) {
                        pduSent ++;
                        hostTag = buffered_pkt->hostTag;
                    } else {
                        send_vif_availibility_rsp(priv,uf_get_vif_identifier(interfacePriv->interfaceMode,interfaceTag),CSR_RC_UNSPECIFIED_FAILURE);
                    }
                }
                /* Buffered frame sent successfully */
                spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                interfacePriv->noOfbroadcastPktQueued--;
                spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                kfree(buffered_pkt);
            }
        }
    }
    if((interfacePriv->dtimActive == FALSE)) {
        /* Record the host Tag*/
        unifi_trace(priv,UDBG2,"send_multicast_frames: Recorded hostTag of EOSP packet: = 0x%x\n",hostTag);
        interfacePriv->multicastPduHostTag = hostTag;
    }
    return pduSent;
}
#endif
void uf_process_ma_vif_availibility_ind(unifi_priv_t *priv,CsrUint8 *sigdata,
                                        CsrUint32 siglen)
{
#ifdef CSR_SUPPORT_SME
    CSR_SIGNAL signal;
    CSR_MA_VIF_AVAILABILITY_INDICATION *ind;
    int r;
    CsrUint16 interfaceTag;
    CsrUint8 pduSent =0;
    CSR_RESULT_CODE resultCode = CSR_RC_SUCCESS;
    netInterface_priv_t *interfacePriv;

    func_enter();
    unifi_trace(priv, UDBG3,
            "uf_process_ma_vif_availibility_ind: Process signal 0x%.4X\n",
            *((CsrUint16*)sigdata));

    r = read_unpack_signal(sigdata, &signal);
    if (r) {
        unifi_error(priv,
                    "uf_process_ma_vif_availibility_ind: Received unknown signal 0x%.4X.\n",
                    CSR_GET_UINT16_FROM_LITTLE_ENDIAN(sigdata));
        func_exit();
        return;
    }
    ind = &signal.u.MaVifAvailabilityIndication;
    interfaceTag=ind->VirtualInterfaceIdentifier & 0xff;

    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_error(priv, "in vif_availability_ind interfaceTag is wrong\n");
        return;
    }

    interfacePriv = priv->interfacePriv[interfaceTag];

    if(ind->Multicast) {
        if(list_empty(&interfacePriv->genericMulticastOrBroadCastFrames) &&
            list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames)) {
            /* This condition can occur because of a potential race where the
               TIM is not yet reset as host is waiting for confirm but it is sent
               by firmware and DTIM occurs*/
            unifi_notice(priv,"ma_vif_availibility_ind recevied for multicast but queues are empty%d\n",interfaceTag);
            send_vif_availibility_rsp(priv,ind->VirtualInterfaceIdentifier,CSR_RC_NO_BUFFERED_BROADCAST_MULTICAST_FRAMES);
            interfacePriv->dtimActive = FALSE;
            if(interfacePriv->multicastPduHostTag == 0xffffffff) {
                unifi_notice(priv,"ma_vif_availibility_ind recevied for multicast but queues are empty%d\n",interfaceTag);
                /* This may be an extra request in very rare race conditions but it is fine as it would atleast remove the potential lock up */
                update_tim(priv,0,0,interfaceTag, 0xFFFFFFFF);
            }
            return;
        }
        if(interfacePriv->dtimActive) {
            unifi_trace(priv,UDBG2,"DTIM Occurred for already active DTIM interface %d\n",interfaceTag);
            return;
        } else {
            unifi_trace(priv,UDBG2,"DTIM Occurred for interface %d\n",interfaceTag);
            if(list_empty(&interfacePriv->genericMulticastOrBroadCastFrames)) {
                set_eosp_transmit_ctrl(priv,&interfacePriv->genericMulticastOrBroadCastMgtFrames);
            } else {
                set_eosp_transmit_ctrl(priv,&interfacePriv->genericMulticastOrBroadCastFrames);
            }
        }
        interfacePriv->dtimActive = TRUE;
        pduSent = send_multicast_frames(priv,interfaceTag);
    }
    else {
        unifi_error(priv,"Interface switching is not supported %d\n",interfaceTag);
        resultCode = CSR_RC_NOT_SUPPORTED;
        send_vif_availibility_rsp(priv,ind->VirtualInterfaceIdentifier,CSR_RC_NOT_SUPPORTED);
    }
#endif
}
#ifdef CSR_SUPPORT_SME

#define  GET_ACTIVE_INTERFACE_TAG(priv) 0


void uf_continue_uapsd(unifi_priv_t *priv, CsrWifiRouterCtrlStaInfo_t * staInfo)
{

    CsrInt8 i;

    func_enter();

    if(((staInfo->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED)||
         (staInfo->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_DELIVERY_ONLY_ENABLE))
        &&(!list_empty(&staInfo->mgtFrames))){

        unifi_trace(priv, UDBG5, "uf_continue_uapsd : U-APSD ACTIVE and sending buffered mgt frames\n");
        uf_send_buffered_data_from_delivery_ac(priv, staInfo, UNIFI_TRAFFIC_Q_VO, &staInfo->mgtFrames);

        /*This may happen because U-APSD was completed
         with previous AC transfer*/

        if(staInfo->uapsdActive == FALSE) {
           return;
        }
    }

    for(i=3;i>=0;i--) {

        if(((staInfo->powersaveMode[i]== CSR_WIFI_AC_DELIVERY_ONLY_ENABLE)
             ||(staInfo->powersaveMode[i] == CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED))
             &&(!list_empty(&staInfo->dataPdu[i]))) {
            unifi_trace(priv, UDBG5, "uf_continue_uapsd : U-APSD ACTIVE and sending buffered  data frames\n");
            uf_send_buffered_data_from_delivery_ac(priv, staInfo, i, &staInfo->dataPdu[i]);
        }

        /*This may happen because U-APSD was completed
          with previous AC transfer*/
        if (staInfo->uapsdActive == FALSE) {
            return;
        }
    }

    if (staInfo->uapsdActive && !uf_is_more_data_for_delivery_ac(priv, staInfo, TRUE)) {
        /* If last packet not able to transfer due to ENOSPC & buffer management algorithm
         * would have removed last packet. Then we wont update staInfo->UapsdActive = FALSE (suppose
         * to update as we dont have packet to transfer at this USP) because above if loop fails as list is empty &
         * update of UAPSD activity done in uf_send_buffered_data_from_delivery_ac().
         * In this situation we send QOS null & mean time update UapsdActive to FALSE here
         */
        staInfo->uapsdActive = FALSE;
        uf_send_qos_null(priv, GET_ACTIVE_INTERFACE_TAG(priv), staInfo->peerMacAddress.a, CSR_QOS_UP0 , staInfo);
    }
    func_exit();
}


void uf_send_buffered_data_from_delivery_ac(unifi_priv_t *priv,
                                            CsrWifiRouterCtrlStaInfo_t * staInfo,
                                            CsrUint8 queue,
                                            struct list_head *txList)
{

    CsrUint16 interfaceTag = GET_ACTIVE_INTERFACE_TAG(priv);
    tx_buffered_packets_t * buffered_pkt = NULL;
    unsigned long lock_flags;
    CsrBool eosp=FALSE;
    CsrInt8 r =0;
    CsrBool moreData = FALSE;

    CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
    netInterface_priv_t *interfacePriv;
    interfacePriv = priv->interfacePriv[interfaceTag];
    func_enter();

    /*Check U-APSD conditions if not met return from here*/
    if((staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)&&
        (staInfo->uapsdActive == TRUE)&&
        (!IS_DELIVERY_AND_TRIGGER_ENABLED(staInfo->powersaveMode[queue]))){

        unifi_trace(priv,UDBG4,"uf_send_buffered_data_from_queue : U-APSD active. %d :Queue NOT delivery enbaled.return %\n",queue);

        return;
     }

    while(!isRouterBufferEnabled(priv,queue) &&
                    ((buffered_pkt=dequeue_tx_data_pdu(priv, txList))!=NULL)){
        if((IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag))){
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            staInfo->uapsdSuspended = TRUE;
            staInfo->uapsdActive = FALSE;
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
            /* re-queueing the packet as DTIM started */
            spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
            list_add(&buffered_pkt->q,txList);
            spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
            unifi_trace(priv, UDBG3, "%s: DTIM Active while UAPSD in progress for staId: 0x%x\n",__FUNCTION__,staInfo->aid);
            break;
        }

        buffered_pkt->transmissionControl &=
                 ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);


        if((staInfo->wmmOrQosEnabled == TRUE)&&(staInfo->uapsdActive == TRUE)){

            moreData = uf_is_more_data_for_delivery_ac(priv,staInfo,TRUE);

             buffered_pkt->transmissionControl = TRANSMISSION_CONTROL_TRIGGER_MASK;

            if(staInfo->noOfSpFramesSent == (staInfo->maxSpLength-1)){
                moreData = FALSE;
            }

            if(moreData == FALSE){
                 eosp = TRUE;
                 staInfo->uapsdActive = FALSE;
                 staInfo->noOfSpFramesSent = FALSE;
                 buffered_pkt->transmissionControl =
                          (TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);

                /* Check if all AC's are Delivery Enabled */
                is_all_ac_deliver_enabled_and_moredata(staInfo, &allDeliveryEnabled, &dataAvailable);
                if ((allDeliveryEnabled && !dataAvailable)) {
                    update_tim(priv,staInfo->aid,0,interfaceTag, staInfo->assignedHandle);
                }
                /* check the moer data for non delivery ac and update accordingly */
                else if(uf_is_more_data_for_non_delivery_ac(staInfo) ) {
                    update_tim(priv,staInfo->aid,1,interfaceTag, staInfo->assignedHandle);
                }
                else if(!uf_is_more_data_for_non_delivery_ac(staInfo) ){
                     unifi_trace(priv, UDBG3, "more data = NULL, set tim to 0 in uf_send_buffered_data_from_delivery_ac\n");
                     update_tim(priv,staInfo->aid,0,interfaceTag, staInfo->assignedHandle);
                }

             }
        }
        else
        {
          /*Non QoS and non U-APSD.*/
            eosp = FALSE;
            moreData = FALSE;
            unifi_warning(priv,"uf_send_buffered_data_from_delivery_ac :non U-APSD !!! \n");
        }

        unifi_trace(priv,UDBG2,"uf_send_buffered_data_from_delivery_ac : MoreData:%d, EOSP:%d\n",moreData,eosp);

        if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staInfo,moreData,eosp)) == -ENOSPC) {
           /* Enqueue at the head of the queue */
           spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
           list_add(&buffered_pkt->q,txList);
           spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
           priv->pausedStaHandle[queue]=(CsrUint8)(staInfo->assignedHandle);
           unifi_notice (priv," U-APSD: PDU sending failed .. no space for queue %d \n",queue);
           /*Break the loop for this queue.Try for next available Delivery enabled
           Queue*/
           break;
        } else {
            if(r){
                /* the PDU failed where we can't do any thing so free the storage */
                unifi_net_data_free(priv, &buffered_pkt->bulkdata);
            }

            kfree(buffered_pkt);
            if(staInfo->uapsdActive == TRUE){
                    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                    staInfo->noOfSpFramesSent = staInfo->noOfSpFramesSent + 1;
                    if(staInfo->noOfSpFramesSent == staInfo->maxSpLength){
                        staInfo->uapsdActive = FALSE;
                        staInfo->noOfSpFramesSent = FALSE;
                        spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                        break;
                    }
                spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
            }
        }
    }

   func_exit();

}

void uf_send_buffered_data_from_ac(unifi_priv_t *priv,
                                   CsrWifiRouterCtrlStaInfo_t * staInfo,
                                   CsrUint8 queue,
                                   struct list_head *txList)
{
    tx_buffered_packets_t * buffered_pkt = NULL;
    unsigned long lock_flags;
    CsrBool eosp=FALSE;
    CsrBool moreData = FALSE;
    CsrInt8 r =0;

    func_enter();

    unifi_trace(priv,UDBG2,"uf_send_buffered_data_from_ac :\n");

    while(!isRouterBufferEnabled(priv,queue) &&
                    ((buffered_pkt=dequeue_tx_data_pdu(priv, txList))!=NULL)){

        buffered_pkt->transmissionControl &=
                 ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);

        unifi_trace(priv,UDBG3,"uf_send_buffered_data_from_ac : MoreData:%d, EOSP:%d\n",moreData,eosp);

        if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staInfo,moreData,eosp)) == -ENOSPC) {
           /* Enqueue at the head of the queue */
           spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
           list_add(&buffered_pkt->q,txList);
           spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
           if(staInfo != NULL){
              priv->pausedStaHandle[queue]=(CsrUint8)(staInfo->assignedHandle);
           }
           unifi_trace(priv,UDBG3," uf_send_buffered_data_from_ac: PDU sending failed .. no space for queue %d \n",queue);
           } else {
            if(r){
                /* the PDU failed where we can't do any thing so free the storage */
                unifi_net_data_free(priv, &buffered_pkt->bulkdata);
            }
            kfree(buffered_pkt);
      }
  }

  func_exit();

}

void uf_send_buffered_frames(unifi_priv_t *priv,unifi_TrafficQueue q)
{
    CsrUint16 interfaceTag = GET_ACTIVE_INTERFACE_TAG(priv);
    CsrUint32 startIndex=0,endIndex=0;
    CsrWifiRouterCtrlStaInfo_t * staInfo = NULL;
    CsrUint8 queue;
    CsrBool moreData = FALSE;

    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];

    if(!((interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_AP) ||
        (interfacePriv->interfaceMode == CSR_WIFI_ROUTER_CTRL_MODE_P2PGO)))
        return;
    func_enter();

    queue = (q<=3)?q:0;

    if(interfacePriv->dtimActive) {
        /* this function updates dtimActive*/
        send_multicast_frames(priv,interfaceTag);
        if(!interfacePriv->dtimActive) {
            moreData = (!list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames) ||
             !list_empty(&interfacePriv->genericMulticastOrBroadCastFrames));
            if(!moreData) {
                update_tim(priv,0,0,interfaceTag, 0XFFFFFFFF);
            }
        } else {
            moreData = (!list_empty(&interfacePriv->genericMulticastOrBroadCastMgtFrames) ||
                        !list_empty(&interfacePriv->genericMulticastOrBroadCastFrames));
           if(!moreData) {
               /* This should never happen but if it happens, we need a way out */
               unifi_error(priv,"ERROR: No More Data but DTIM is active sending Response\n");
               send_vif_availibility_rsp(priv,uf_get_vif_identifier(interfacePriv->interfaceMode,interfaceTag),CSR_RC_NO_BUFFERED_BROADCAST_MULTICAST_FRAMES);
               interfacePriv->dtimActive = FALSE;
           }
        }
        func_exit();
        return;
    }
    if(priv->pausedStaHandle[queue] > 7) {
        priv->pausedStaHandle[queue] = 0;
    }

    if(queue == UNIFI_TRAFFIC_Q_VO) {


        unifi_trace(priv,UDBG2,"uf_send_buffered_frames : trying mgt from queue=%d\n",queue);
        for(startIndex= 0; startIndex < UNIFI_MAX_CONNECTIONS;startIndex++) {
            staInfo =  CsrWifiRouterCtrlGetStationRecordFromHandle(priv,startIndex,interfaceTag);
            if(!staInfo ) {
                continue;
            } else if((staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                       &&(staInfo->uapsdActive == FALSE) ) {
                continue;
            }

            if((staInfo != NULL)&&(staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE)
                               &&(staInfo->uapsdActive == FALSE)){
                            /*Non-UAPSD case push the management frames out*/
               if(!list_empty(&staInfo->mgtFrames)){
                    uf_send_buffered_data_from_ac(priv,staInfo, UNIFI_TRAFFIC_Q_VO, &staInfo->mgtFrames);
                }
            }
            else if((staInfo != NULL)&&(staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                               &&(staInfo->uapsdActive == TRUE)&&(IS_DELIVERY_AND_TRIGGER_ENABLED(staInfo->powersaveMode[UNIFI_TRAFFIC_Q_VO]))){


                if(!list_empty(&staInfo->mgtFrames)){
                    /*UNIFI_TRAFFIC_Q_VO is delivery enabled push the managment frames out*/
                    uf_send_buffered_data_from_delivery_ac(priv, staInfo, UNIFI_TRAFFIC_Q_VO, &staInfo->mgtFrames);

                }
            }

            if(isRouterBufferEnabled(priv,queue)) {
                unifi_notice(priv,"uf_send_buffered_frames : No space Left for queue = %d\n",queue);
                break;
            }
        }


        /*push generic management frames out*/

        if(!list_empty(&interfacePriv->genericMgtFrames)){

        unifi_trace(priv,UDBG2,"uf_send_buffered_frames : trying generic mgt from queue=%d\n",queue);
        uf_send_buffered_data_from_ac(priv,staInfo, UNIFI_TRAFFIC_Q_VO, &interfacePriv->genericMgtFrames);

        }

    }


    unifi_trace(priv,UDBG2,"uf_send_buffered_frames : Resume called for Queue=%d\n",queue);
    unifi_trace(priv,UDBG2,"uf_send_buffered_frames : start=%d end=%d\n",startIndex,endIndex);

    startIndex = priv->pausedStaHandle[queue];
    endIndex = (startIndex + UNIFI_MAX_CONNECTIONS -1) % UNIFI_MAX_CONNECTIONS;

    while(startIndex != endIndex) {
        staInfo =  CsrWifiRouterCtrlGetStationRecordFromHandle(priv,startIndex,interfaceTag);
        if(!staInfo) {
            startIndex ++;
            if(startIndex >= UNIFI_MAX_CONNECTIONS){
                startIndex = 0;
            }
            continue;
        } else if((staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                   &&(staInfo->uapsdActive == FALSE)){
            startIndex ++;
            if(startIndex >= UNIFI_MAX_CONNECTIONS){
                startIndex = 0;
            }
            continue;
        }
        /* Peer is active or U-APSD is active so send PDUs to the peer */
        unifi_trace(priv,UDBG2,"uf_send_buffered_frames : trying data from queue=%d\n",queue);


        if((staInfo != NULL)&&(staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE)
                           &&(staInfo->uapsdActive == FALSE)){

           if(!list_empty(&staInfo->dataPdu[queue])){

          /*Non-UAPSD case push the AC frames out*/
            uf_send_buffered_data_from_ac(priv, staInfo, queue, (&staInfo->dataPdu[queue]));
           }
        }
        else  if((staInfo != NULL)&&(staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                               &&(staInfo->uapsdActive == TRUE)&&(IS_DELIVERY_AND_TRIGGER_ENABLED(staInfo->powersaveMode[queue]))){
            if(!list_empty(&staInfo->dataPdu[queue])){
            uf_send_buffered_data_from_delivery_ac(priv, staInfo, queue, &staInfo->dataPdu[queue]);
            }
        }

        startIndex ++;
        if(startIndex >= UNIFI_MAX_CONNECTIONS){
           startIndex = 0;
        }
    }
    if(isRouterBufferEnabled(priv,queue)) {
        priv->pausedStaHandle[queue] = endIndex;
    } else {
        priv->pausedStaHandle[queue] = 0;
    }

   /*U-APSD might have stopped because of pause.So restart it if U-APSD
   was active with any of the station*/
    for(startIndex= 0; startIndex < UNIFI_MAX_CONNECTIONS;startIndex++) {
        staInfo =  CsrWifiRouterCtrlGetStationRecordFromHandle(priv,startIndex,interfaceTag);
        if(!staInfo ) {
            continue;
        } else if((staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                   &&(staInfo->uapsdActive == TRUE)) {

            /*U-APSD Still active, it means trigger frame is received,so continue U-APSD by
            sending data from remaining delivery enabled queues*/
            uf_continue_uapsd(priv,staInfo);
        }
    }
    func_exit();
}

CsrBool uf_is_more_data_for_delivery_ac(unifi_priv_t *priv,CsrWifiRouterCtrlStaInfo_t *staRecord,CsrBool mgtCheck)
{
    CsrUint8 i;

    for(i=0;i<=3;i++)
    {
     if(((staRecord->powersaveMode[i]==CSR_WIFI_AC_DELIVERY_ONLY_ENABLE)
            ||(staRecord->powersaveMode[i]==CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED))
            &&(!list_empty(&staRecord->dataPdu[i]))){
          unifi_trace(priv,UDBG2,"uf_is_more_data_for_delivery_ac: Data Available \n");
         return TRUE;
        }
    }
    if((mgtCheck == TRUE)&&(IS_DELIVERY_AND_TRIGGER_ENABLED(staRecord->powersaveMode[UNIFI_TRAFFIC_Q_VO]))
        &&(!list_empty(&staRecord->mgtFrames))){

        unifi_trace(priv,UDBG2,"uf_is_more_data_for_delivery_ac: Management Data Available \n");

        return TRUE;
    }

    unifi_trace(priv,UDBG2,"uf_is_more_data_for_delivery_ac: Data NOT Available \n");
    return FALSE;
}

CsrBool uf_is_more_data_for_non_delivery_ac(CsrWifiRouterCtrlStaInfo_t *staRecord)
{
    CsrUint8 i;

    for(i=0;i<=3;i++)
    {
        if(((staRecord->powersaveMode[i]==CSR_WIFI_AC_TRIGGER_ONLY_ENABLED)
                ||(staRecord->powersaveMode[i]==CSR_WIFI_AC_LEGACY_POWER_SAVE))
                &&(!list_empty(&staRecord->dataPdu[i]))){

         return TRUE;
        }
    }

    if(((staRecord->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_TRIGGER_ONLY_ENABLED)
            ||(staRecord->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_LEGACY_POWER_SAVE))
            &&(!list_empty(&staRecord->mgtFrames))){

     return TRUE;
    }



    return FALSE;
}


int uf_process_station_records_for_sending_data(unifi_priv_t *priv,CsrUint16 interfaceTag,
                                                 CsrWifiRouterCtrlStaInfo_t *srcStaInfo,
                                                 CsrWifiRouterCtrlStaInfo_t *dstStaInfo)
{
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];

    unifi_trace(priv, UDBG5, "entering uf_process_station_records_for_sending_data\n");

    if (srcStaInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_DISCONNECTED) {
        unifi_error(priv, "Peer State not connected AID = %x, handle = %x, control port state = %x\n",
                    srcStaInfo->aid, srcStaInfo->assignedHandle, srcStaInfo->peerControlledPort->port_action);
        return -1;
    }
    switch (interfacePriv->interfaceMode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
            unifi_trace(priv, UDBG5, "mode is AP/P2PGO\n");
            break;
        default:
            unifi_warning(priv, "mode is nor AP neither P2PGO, packet cant be xmit\n");
            return -1;
    }

    switch(dstStaInfo->peerControlledPort->port_action)
    {
        case CSR_WIFI_ROUTER_CTRL_PORT_ACTION_8021X_PORT_CLOSED_DISCARD:
        case CSR_WIFI_ROUTER_CTRL_PORT_ACTION_8021X_PORT_CLOSED_BLOCK:
            unifi_trace(priv, UDBG5, "destination port is closed/blocked, discarding the packet\n");
            return -1;
        default:
            unifi_trace(priv, UDBG5, "destination port state is open\n");
    }

    /* port state is open, destination station record is valid, Power save state is
     * validated in uf_process_ma_packet_req function
     */
    unifi_trace(priv, UDBG5, "leaving uf_process_station_records_for_sending_data\n");
    return 0;
}

void uf_process_wmm_deliver_ac_uapsd(unifi_priv_t * priv,
                                     CsrWifiRouterCtrlStaInfo_t * srcStaInfo,
                                     CsrUint16 qosControl,
                                     CsrUint16 interfaceTag)
{

    CSR_PRIORITY priority;
    CsrInt8 i;
    unifi_TrafficQueue priority_q;
    unsigned long lock_flags;

    func_enter();

    /* start the U-APSD operation only if it not active*/
    if(srcStaInfo->uapsdActive == FALSE){
        /*if recceived Frames trigger Frame and Devlivery enabled AC has data
         then transmit from High priorty delivery enabled AC*/


        priority = (CSR_PRIORITY)(qosControl & IEEE802_11_QC_TID_MASK);

        priority_q = unifi_frame_priority_to_queue((CSR_PRIORITY) priority);

      if((srcStaInfo->powersaveMode[priority_q]==CSR_WIFI_AC_TRIGGER_ONLY_ENABLED)
          ||(srcStaInfo->powersaveMode[priority_q]==CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED)){

          unifi_trace(priv, UDBG3, "uf_process_wmm_deliver_ac_uapsd starting U-APSD operations\n");

          /*Received Frame is trigger frame*/
        unifi_trace(priv, UDBG5, "uf_process_wmm_deliver_ac_uapsd : Received Frame is trigger frame %d\n",priority_q);

        if(((srcStaInfo->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED)||
             (srcStaInfo->powersaveMode[UNIFI_TRAFFIC_Q_VO]==CSR_WIFI_AC_DELIVERY_ONLY_ENABLE))
            &&(!list_empty(&srcStaInfo->mgtFrames))){

            /*Trigger frame received and Data available in Delivery enabled AC
            or in Management queue when UNIFI_TRAFFIC_Q_VO is delivery enabled*/
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            srcStaInfo->uapsdActive = TRUE;
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);

            unifi_trace(priv, UDBG5, "uf_process_wmm_deliver_ac_uapsd : U-APSD ACTIVE and sending buffered mgt frames\n");

           /* uf_send_buffered_frames(priv, priority_q); */
            uf_send_buffered_data_from_delivery_ac(priv, srcStaInfo, UNIFI_TRAFFIC_Q_VO, &srcStaInfo->mgtFrames);


           /*This may happen because U-APSD was completed
            with previous AC transfer*/

           if(srcStaInfo->uapsdActive == FALSE){
              return;
           }

         }


        for(i=3;i>=0;i--){

            if(((srcStaInfo->powersaveMode[i]==CSR_WIFI_AC_DELIVERY_ONLY_ENABLE)
                ||(srcStaInfo->powersaveMode[i]==CSR_WIFI_AC_TRIGGER_AND_DELIVERY_ENABLED))
                &&(!list_empty(&srcStaInfo->dataPdu[i]))){


                 /*Trigger frame received and Data available in Delivery enabled AC
                 or in Management queue when UNIFI_TRAFFIC_Q_VO is delivery enabled*/
                 spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
                 srcStaInfo->uapsdActive = TRUE;
                 spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);

                 unifi_trace(priv, UDBG5, "uf_process_wmm_deliver_ac_uapsd : U-APSD ACTIVE and sending buffered  data frames\n");

                 uf_send_buffered_data_from_delivery_ac(priv, srcStaInfo, i, &srcStaInfo->dataPdu[i]);

                 /*This may happen because U-APSD was completed
                  with previous AC transfer*/

                 if(srcStaInfo->uapsdActive == FALSE){
                    return;
                 }
            }

         }
         if(srcStaInfo->uapsdActive == FALSE && !(uf_is_more_data_for_delivery_ac(priv,srcStaInfo,TRUE))){
             unifi_trace(priv, UDBG3, "uf_process_wmm_deliver_ac_uapsd : No buffer frames so sending QOS Null in response of trigger frame\n");
             uf_send_qos_null(priv,interfaceTag,srcStaInfo->peerMacAddress.a,priority,srcStaInfo);
         }

      }

  }

  func_exit();

}
void uf_send_qos_null(unifi_priv_t * priv,CsrUint16 interfaceTag, const CsrUint8 *da,CSR_PRIORITY priority,CsrWifiRouterCtrlStaInfo_t * srcStaInfo)
{
    bulk_data_param_t bulkdata;
    CsrResult csrResult;
    struct sk_buff *skb, *newSkb = NULL;
    CsrWifiMacAddress peerAddress;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CSR_TRANSMISSION_CONTROL transmissionControl = (TRANSMISSION_CONTROL_ESOP_MASK | TRANSMISSION_CONTROL_TRIGGER_MASK);
    int r;
    CSR_SIGNAL signal;
    CsrUint32 priority_q;
    CSR_RATE transmitRate = 0;


    func_enter();
    /* Send a Null Frame to Peer,
     * 32= size of mac header  */
    csrResult = unifi_net_data_malloc(priv, &bulkdata.d[0], MAC_HEADER_SIZE + QOS_CONTROL_HEADER_SIZE);

    if (csrResult != CSR_RESULT_SUCCESS) {
        unifi_error(priv, " failed to allocate request_data. in uf_send_qos_null func\n");
        return ;
    }
    skb = (struct sk_buff *)(bulkdata.d[0].os_net_buf_ptr);
    skb->len = 0;
    bulkdata.d[0].os_data_ptr = skb->data;
    bulkdata.d[0].os_net_buf_ptr = (unsigned char*)skb;
    bulkdata.d[0].net_buf_length = bulkdata.d[0].data_length = skb->len;
    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].os_net_buf_ptr = NULL;
    bulkdata.d[1].net_buf_length = bulkdata.d[1].data_length = 0;

    /* For null frames protection bit should not be set in MAC header, so passing value 0 below for protection field */

    if (prepare_and_add_macheader(priv, skb, newSkb, priority, &bulkdata, interfaceTag, da, interfacePriv->bssid.a, 0)) {
        unifi_error(priv, "failed to create MAC header\n");
        unifi_net_data_free(priv, &bulkdata.d[0]);
        return;
    }
    memcpy(peerAddress.a, ((CsrUint8 *) bulkdata.d[0].os_data_ptr) + 4, ETH_ALEN);
    /* convert priority to queue */
    priority_q = unifi_frame_priority_to_queue((CSR_PRIORITY) priority);

    /* Frame ma-packet.req, this is saved/transmitted depend on queue state
     * send the null frame at data rate of 1 Mb/s for AP or 6 Mb/s for P2PGO
     */
    switch (interfacePriv->interfaceMode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
            transmitRate = 2;
            break;
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
            transmitRate = 12;
            break;
        default:
            transmitRate = 0;
    }
    unifi_frame_ma_packet_req(priv, priority, transmitRate, 0xffffffff, interfaceTag,
                              transmissionControl, priv->netdev_client->sender_id,
                              peerAddress.a, &signal);

    r = ul_send_signal_unpacked(priv, &signal, &bulkdata);
    if(r) {
        unifi_error(priv, "failed to send QOS data null packet result: %d\n",r);
        unifi_net_data_free(priv, &bulkdata.d[0]);
    }

    func_exit();
    return;

}
void uf_send_nulldata(unifi_priv_t * priv,CsrUint16 interfaceTag, const CsrUint8 *da,CSR_PRIORITY priority,CsrWifiRouterCtrlStaInfo_t * srcStaInfo)
{
    bulk_data_param_t bulkdata;
    CsrResult csrResult;
    struct sk_buff *skb, *newSkb = NULL;
    CsrWifiMacAddress peerAddress;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CSR_TRANSMISSION_CONTROL transmissionControl = 0;
    int r;
    CSR_SIGNAL signal;
    CsrUint32 priority_q;
    CSR_RATE transmitRate = 0;
    CSR_MA_PACKET_REQUEST *req = &signal.u.MaPacketRequest;
    unsigned long lock_flags;

    func_enter();
    /* Send a Null Frame to Peer, size = 24 for MAC header */
    csrResult = unifi_net_data_malloc(priv, &bulkdata.d[0], MAC_HEADER_SIZE);

    if (csrResult != CSR_RESULT_SUCCESS) {
        unifi_error(priv, "uf_send_nulldata: Failed to allocate memory for NULL frame\n");
        return ;
    }
    skb = (struct sk_buff *)(bulkdata.d[0].os_net_buf_ptr);
    skb->len = 0;
    bulkdata.d[0].os_data_ptr = skb->data;
    bulkdata.d[0].os_net_buf_ptr = (unsigned char*)skb;
    bulkdata.d[0].net_buf_length = bulkdata.d[0].data_length = skb->len;
    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].os_net_buf_ptr = NULL;
    bulkdata.d[1].net_buf_length = bulkdata.d[1].data_length = 0;

    /* For null frames protection bit should not be set in MAC header, so passing value 0 below for protection field */
    if (prepare_and_add_macheader(priv, skb, newSkb, priority, &bulkdata, interfaceTag, da, interfacePriv->bssid.a, 0)) {
        unifi_error(priv, "uf_send_nulldata: Failed to create MAC header\n");
        unifi_net_data_free(priv, &bulkdata.d[0]);
        return;
    }
    memcpy(peerAddress.a, ((CsrUint8 *) bulkdata.d[0].os_data_ptr) + 4, ETH_ALEN);
    /* convert priority to queue */
    priority_q = unifi_frame_priority_to_queue((CSR_PRIORITY) priority);
    transmissionControl &= ~(CSR_NO_CONFIRM_REQUIRED);

    /* Frame ma-packet.req, this is saved/transmitted depend on queue state
     * send the null frame at data rate of 1 Mb/s for AP or 6 Mb/s for P2PGO
     */
    switch (interfacePriv->interfaceMode)
    {
        case CSR_WIFI_ROUTER_CTRL_MODE_AP:
            transmitRate = 2;
            break;
        case CSR_WIFI_ROUTER_CTRL_MODE_P2PGO:
            transmitRate = 12;
            break;
        default:
            transmitRate = 0;
    }
    unifi_frame_ma_packet_req(priv, priority, transmitRate, INVALID_HOST_TAG, interfaceTag,
                              transmissionControl, priv->netdev_client->sender_id,
                              peerAddress.a, &signal);

    /* Save host tag to check the status on reception of MA packet confirm */
    srcStaInfo->nullDataHostTag = req->HostTag;
    unifi_trace(priv, UDBG1, "uf_send_nulldata: STA AID = %d hostTag = %x\n", srcStaInfo->aid, req->HostTag);

    r = ul_send_signal_unpacked(priv, &signal, &bulkdata);

    if(r == -ENOSPC) {
        unifi_trace(priv, UDBG1, "uf_send_nulldata: ENOSPC Requeue the Null frame\n");
        enque_tx_data_pdu(priv, &bulkdata, &srcStaInfo->dataPdu[priority_q], &signal, 1);
        spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
        srcStaInfo->noOfPktQueued++;
        spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);


    }
    if(r && r != -ENOSPC){
        unifi_error(priv, "uf_send_nulldata: Failed to send Null frame Error = %d\n",r);
        unifi_net_data_free(priv, &bulkdata.d[0]);
        srcStaInfo->nullDataHostTag = INVALID_HOST_TAG;
    }

    func_exit();
    return;
}

CsrBool uf_check_broadcast_bssid(unifi_priv_t *priv, const bulk_data_param_t *bulkdata)
{
    CsrUint8 *bssid = NULL;
    static const CsrWifiMacAddress broadcast_address = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    CsrUint8 toDs, fromDs;

    toDs = (((bulkdata->d[0].os_data_ptr)[1]) & 0x01) ? 1 : 0;
    fromDs =(((bulkdata->d[0].os_data_ptr)[1]) & 0x02) ? 1 : 0;

     if (toDs && fromDs)
    {
        unifi_trace(priv, UDBG6, "Address 4 present, Don't try to find BSSID\n");
        bssid = NULL;
    }
    else if((toDs == 0) && (fromDs ==0))
    {
        /* BSSID is Address 3 */
        bssid = (CsrUint8 *) (bulkdata->d[0].os_data_ptr + 4 + (2 * ETH_ALEN));
    }
    else if(toDs)
    {
        /* BSSID is Address 1 */
        bssid = (CsrUint8 *) (bulkdata->d[0].os_data_ptr + 4);
    }
    else if(fromDs)
    {
        /* BSSID is Address 2 */
        bssid = (CsrUint8 *) (bulkdata->d[0].os_data_ptr + 4 + ETH_ALEN);
    }

    if (memcmp(broadcast_address.a, bssid, ETH_ALEN)== 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


CsrBool uf_process_pm_bit_for_peer(unifi_priv_t * priv, CsrWifiRouterCtrlStaInfo_t * srcStaInfo,
                                CsrUint8 pmBit,CsrUint16 interfaceTag)
{
    CsrBool moreData = FALSE;
    CsrBool powerSaveChanged = FALSE;
    unsigned long lock_flags;

    unifi_trace(priv, UDBG3, "entering uf_process_pm_bit_for_peer\n");
    if (pmBit) {
        priv->allPeerDozing |= (0x01 << (srcStaInfo->assignedHandle));
    } else {
        priv->allPeerDozing &= ~(0x01 << (srcStaInfo->assignedHandle));
    }
    if(pmBit) {
        if(srcStaInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE) {

            /* disable the preemption */
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            srcStaInfo->currentPeerState =CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE;
            powerSaveChanged = TRUE;
            /* enable the preemption */
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
        } else {
            return powerSaveChanged;
        }
    } else {
        if(srcStaInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE) {
            /* disable the preemption */
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            srcStaInfo->currentPeerState = CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE;
            powerSaveChanged = TRUE;
            /* enable the preemption */
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
        }else {
            return powerSaveChanged;
        }
    }


    if(srcStaInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE) {
        unifi_trace(priv,UDBG3, "Peer with AID = %d is active now\n",srcStaInfo->aid);
        process_peer_active_transition(priv,srcStaInfo,interfaceTag);
    } else {
        unifi_trace(priv,UDBG3, "Peer with AID = %d is in PS Now\n",srcStaInfo->aid);
        /* Set TIM if needed */
        if(!srcStaInfo->wmmOrQosEnabled) {
            moreData = (!list_empty(&srcStaInfo->mgtFrames) ||
                        !list_empty(&srcStaInfo->dataPdu[UNIFI_TRAFFIC_Q_VO])||
                        !list_empty(&srcStaInfo->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]));
            if(moreData && (srcStaInfo->timSet == CSR_WIFI_TIM_RESET)) {
                unifi_trace(priv, UDBG3, "This condition should not occur\n");
                update_tim(priv,srcStaInfo->aid,1,interfaceTag, srcStaInfo->assignedHandle);
            }
        } else {
            CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
            unifi_trace(priv, UDBG5, "Qos in AP Mode\n");
            /* Check if all AC's are Delivery Enabled */
            is_all_ac_deliver_enabled_and_moredata(srcStaInfo, &allDeliveryEnabled, &dataAvailable);
            /*check for more data in non-delivery enabled queues*/
            moreData = (uf_is_more_data_for_non_delivery_ac(srcStaInfo) || (allDeliveryEnabled && dataAvailable));

            if(moreData && (srcStaInfo->timSet == CSR_WIFI_TIM_RESET)) {
                update_tim(priv,srcStaInfo->aid,1,interfaceTag, srcStaInfo->assignedHandle);
            }
        }
    }
    unifi_trace(priv, UDBG3, "leaving uf_process_pm_bit_for_peer\n");
    return powerSaveChanged;
}



void uf_process_ps_poll(unifi_priv_t *priv,CsrUint8* sa,CsrUint8* da,CsrUint8 pmBit,CsrUint16 interfaceTag)
{
    CsrWifiRouterCtrlStaInfo_t *staRecord =
    CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, sa, interfaceTag);
    tx_buffered_packets_t * buffered_pkt = NULL;
    CsrWifiMacAddress peerMacAddress;
    unsigned long lock_flags;
    CsrInt8 r =0;
    CsrBool moreData = FALSE;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];

    unifi_trace(priv, UDBG3, "entering uf_process_ps_poll\n");
    if(!staRecord) {
        memcpy(peerMacAddress.a,sa,ETH_ALEN);
        unifi_trace(priv, UDBG3, "In uf_process_ps_poll, sta record not found:unexpected frame addr = %x:%x:%x:%x:%x:%x\n",
                sa[0], sa[1],sa[2], sa[3], sa[4],sa[5]);
        CsrWifiRouterCtrlUnexpectedFrameIndSend(priv->CSR_WIFI_SME_IFACEQUEUE,0,interfaceTag,peerMacAddress);
        return;
    }

    uf_process_pm_bit_for_peer(priv,staRecord,pmBit,interfaceTag);

    /* Update station last activity time */
    staRecord->activity_flag = TRUE;

    /* This should not change the PM bit as PS-POLL has PM bit always set */
    if(!pmBit) {
        unifi_notice (priv," PM bit reset in PS-POLL\n");
        return;
    }

    if(IS_DTIM_ACTIVE(interfacePriv->dtimActive,interfacePriv->multicastPduHostTag)) {
        /* giving more priority to multicast packets so dropping ps-poll*/
        unifi_notice (priv," multicast transmission is going on so don't take action on PS-POLL\n");
        return;
    }

    if(!staRecord->wmmOrQosEnabled) {
        if((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->mgtFrames))) {
            buffered_pkt->transmissionControl |= TRANSMISSION_CONTROL_TRIGGER_MASK;
            moreData = (!list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]) ||
                        !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]) ||
                        !list_empty(&staRecord->mgtFrames));

            buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
            if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                /* Clear the trigger bit transmission control*/
                buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                /* Enqueue at the head of the queue */
                spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                list_add(&buffered_pkt->q, &staRecord->mgtFrames);
                spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
                priv->pausedStaHandle[3]=(CsrUint8)(staRecord->assignedHandle);
            } else {
                if(r){
                    unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                    /* the PDU failed where we can't do any thing so free the storage */
                    unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                }
                kfree(buffered_pkt);
            }
        } else if((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]))) {
            buffered_pkt->transmissionControl |= TRANSMISSION_CONTROL_TRIGGER_MASK;
            moreData = (!list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]) ||
                        !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]));

            buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
            if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                /* Clear the trigger bit transmission control*/
                buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                /* Enqueue at the head of the queue */
                spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                list_add(&buffered_pkt->q, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]);
                spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                priv->pausedStaHandle[3]=(CsrUint8)(staRecord->assignedHandle);
                unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
            } else {
                if(r){
                    unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                    /* the PDU failed where we can't do any thing so free the storage */
                    unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                }
                kfree(buffered_pkt);
            }
        } else  if((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]))) {
            buffered_pkt->transmissionControl |= TRANSMISSION_CONTROL_TRIGGER_MASK;
            moreData = !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]);

            buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
            if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                /* Clear the trigger bit transmission control*/
                buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                /* Enqueue at the head of the queue */
                spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                list_add(&buffered_pkt->q, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]);
                spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                priv->pausedStaHandle[0]=(CsrUint8)(staRecord->assignedHandle);
                unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
            } else {
                if(r){
                    unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                    /* the PDU failed where we can't do any thing so free the storage */
                    unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                }
                kfree(buffered_pkt);
            }
        } else {
         /* Actually since we have sent an ACK, there
         * there is no need to send a NULL frame*/
        }
        moreData = (!list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]) ||
           !list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_CONTENTION]) ||
            !list_empty(&staRecord->mgtFrames));
        if(!moreData && (staRecord->timSet == CSR_WIFI_TIM_SET)) {
            unifi_trace(priv, UDBG3, "more data = NULL, set tim to 0 in uf_process_ps_poll\n");
            update_tim(priv,staRecord->aid,0,interfaceTag, staRecord->assignedHandle);
        }
    } else {

        CsrUint8 allDeliveryEnabled = 0, dataAvailable = 0;
        unifi_trace(priv, UDBG3,"Qos Support station.Processing PS-Poll\n");

        /*Send Data From Management Frames*/
        /* Priority orders for delivering the buffered packets are
         * 1. UNIFI_TRAFFIC_Q_VO, if its non delivery enabled
         * 2. management frames
         * 3. Other access catagory frames which are non deliver enable
         */

        /* Check if all AC's are Delivery Enabled */
        is_all_ac_deliver_enabled_and_moredata(staRecord, &allDeliveryEnabled, &dataAvailable);

        if (allDeliveryEnabled) {
            unifi_trace(priv, UDBG3, "uf_process_ps_poll: All ACs are delivery enable so Sending QOS Null in response of Ps-poll\n");
            uf_send_qos_null(priv,interfaceTag,sa,CSR_QOS_UP0,staRecord);
            return;
        }

        if ((!IS_DELIVERY_ENABLED(staRecord->powersaveMode[UNIFI_TRAFFIC_Q_VO])) &&
                (!list_empty(&staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]) || !list_empty(&staRecord->mgtFrames))) {
            /* UNIFI_TRAFFIC_Q_VO is non delivery enabled, & check for packets are there to send from this AC */
            if((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]))) {
                moreData = uf_is_more_data_for_non_delivery_ac(staRecord);
                buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);

                /* Last parameter is EOSP & its false always for PS-POLL processing */
                if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                    /* Clear the trigger bit transmission control*/
                    buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                    /* Enqueue at the head of the queue */
                    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                    list_add(&buffered_pkt->q, &staRecord->dataPdu[UNIFI_TRAFFIC_Q_VO]);
                    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                    priv->pausedStaHandle[0]=(CsrUint8)(staRecord->assignedHandle);
                    unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
                } else {
                    if(r){
                        unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                        /* the PDU failed where we can't do any thing so free the storage */
                        unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                    }
                    kfree(buffered_pkt);
                }
            } else if ((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->mgtFrames))) {
                    /* We dont have packets in non delivery enabled UNIFI_TRAFFIC_Q_VO, So we are looking in management
                     * queue of the station record
                     */
                    moreData = uf_is_more_data_for_non_delivery_ac(staRecord);
                    buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);

                    /* Last parameter is EOSP & its false always for PS-POLL processing */
                    if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                        /* Clear the trigger bit transmission control*/
                        buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                        /* Enqueue at the head of the queue */
                        spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                        list_add(&buffered_pkt->q, &staRecord->mgtFrames);
                        spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                        priv->pausedStaHandle[0]=(CsrUint8)(staRecord->assignedHandle);
                        unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
                    } else {
                        if(r){
                            unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                            /* the PDU failed where we can't do any thing so free the storage */
                            unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                        }
                        kfree(buffered_pkt);
                    }
                }
        } else {
            CsrInt8 i;
            /* We dont have buffered packet in UNIFI_TRAFFIC_Q_VO & mangement frame queue (1 & 2 failed), So proceed with 3 condition
             * UNIFI_TRAFFIC_Q_VO is taken care so start with i index = 2
             */
            for(i= 2; i>=0; i--) {
                if (!IS_DELIVERY_ENABLED(staRecord->powersaveMode[i])) {
                    /* Send One packet, if queue is NULL then continue */
                    if((buffered_pkt=dequeue_tx_data_pdu(priv, &staRecord->dataPdu[i]))) {
                        moreData = uf_is_more_data_for_non_delivery_ac(staRecord);

                        buffered_pkt->transmissionControl |= (TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);

                        /* Last parameter is EOSP & its false always for PS-POLL processing */
                        if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staRecord,moreData,FALSE)) == -ENOSPC) {
                            /* Clear the trigger bit transmission control*/
                            buffered_pkt->transmissionControl &= ~(TRANSMISSION_CONTROL_TRIGGER_MASK | TRANSMISSION_CONTROL_ESOP_MASK);
                            /* Enqueue at the head of the queue */
                            spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                            list_add(&buffered_pkt->q, &staRecord->dataPdu[i]);
                            spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                            priv->pausedStaHandle[0]=(CsrUint8)(staRecord->assignedHandle);
                            unifi_trace(priv, UDBG1, "(ENOSPC) PS-POLL received : PDU sending failed \n");
                        } else {
                            if(r) {
                                unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                                /* the PDU failed where we can't do any thing so free the storage */
                                unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                            }
                            kfree(buffered_pkt);
                        }
                        break;
                    }
                }
            }
        }
        /* Check if all AC's are Delivery Enabled */
        is_all_ac_deliver_enabled_and_moredata(staRecord, &allDeliveryEnabled, &dataAvailable);
        /*check for more data in non-delivery enabled queues*/
        moreData = (uf_is_more_data_for_non_delivery_ac(staRecord) || (allDeliveryEnabled && dataAvailable));
        if(!moreData && (staRecord->timSet == CSR_WIFI_TIM_SET)) {
            unifi_trace(priv, UDBG3, "more data = NULL, set tim to 0 in uf_process_ps_poll\n");
            update_tim(priv,staRecord->aid,0,interfaceTag, staRecord->assignedHandle);
        }
    }

        unifi_trace(priv, UDBG3, "leaving uf_process_ps_poll\n");
}



void add_to_send_cfm_list(unifi_priv_t * priv,
                          tx_buffered_packets_t *tx_q_item,
                          struct list_head *frames_need_cfm_list)
{
    tx_buffered_packets_t *send_cfm_list_item = NULL;

    send_cfm_list_item = (tx_buffered_packets_t *) kmalloc(sizeof(tx_buffered_packets_t), GFP_ATOMIC);

    if(send_cfm_list_item == NULL){
        unifi_warning(priv, "%s: Failed to allocate memory for new list item \n");
        return;
    }

    INIT_LIST_HEAD(&send_cfm_list_item->q);

    send_cfm_list_item->hostTag = tx_q_item->hostTag;
    send_cfm_list_item->interfaceTag = tx_q_item->interfaceTag;
    send_cfm_list_item->transmissionControl = tx_q_item->transmissionControl;
    send_cfm_list_item->leSenderProcessId = tx_q_item->leSenderProcessId;
    send_cfm_list_item->rate = tx_q_item->rate;
    memcpy(send_cfm_list_item->peerMacAddress.a, tx_q_item->peerMacAddress.a, ETH_ALEN);
    send_cfm_list_item->priority = tx_q_item->priority;

    list_add_tail(&send_cfm_list_item->q, frames_need_cfm_list);
}

void uf_prepare_send_cfm_list_for_queued_pkts(unifi_priv_t * priv,
                                                 struct list_head *frames_need_cfm_list,
                                                 struct list_head * list)
{
    tx_buffered_packets_t *tx_q_item = NULL;
    struct list_head *listHead;
    struct list_head *placeHolder;
    unsigned long lock_flags;

    func_enter();

    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);

    /* Search through the list and if confirmation required for any frames,
    add it to the send_cfm list */
    list_for_each_safe(listHead, placeHolder, list) {
        tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);

        if(!tx_q_item) {
            unifi_error(priv, "Entry should exist, otherwise it is a (BUG)\n");
            continue;
        }

        /* check if confirmation is requested and if the sender ID
        is not netdevice client then save the entry in the list for need cfms */
        if (!(tx_q_item->transmissionControl & CSR_NO_CONFIRM_REQUIRED) &&
            (tx_q_item->leSenderProcessId != priv->netdev_client->sender_id)){
             unifi_trace(priv, UDBG1, "%s: SenderProcessID=%x host tag=%x transmission control=%x\n",
                __FUNCTION__,
                tx_q_item->leSenderProcessId,
                tx_q_item->hostTag,
                tx_q_item->transmissionControl);

             add_to_send_cfm_list(priv, tx_q_item, frames_need_cfm_list);
        }
    }

    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);

    func_exit();
}



void uf_flush_list(unifi_priv_t * priv, struct list_head * list)
{
    tx_buffered_packets_t *tx_q_item;
    struct list_head *listHead;
    struct list_head *placeHolder;
    unsigned long lock_flags;

    unifi_trace(priv, UDBG5, "entering the uf_flush_list \n");

    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    /* go through list, delete & free memory */
    list_for_each_safe(listHead, placeHolder, list) {
        tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);

        if(!tx_q_item) {
            unifi_error(priv, "entry should exists, otherwise crashes (bug)\n");
        }
        unifi_trace(priv, UDBG5,
                "proccess_tx:  in uf_flush_list peerMacAddress=%02X%02X%02X%02X%02X%02X senderProcessId=%x\n",
                tx_q_item->peerMacAddress.a[0], tx_q_item->peerMacAddress.a[1],
                tx_q_item->peerMacAddress.a[2], tx_q_item->peerMacAddress.a[3],
                tx_q_item->peerMacAddress.a[4], tx_q_item->peerMacAddress.a[5],
                tx_q_item->leSenderProcessId);

        list_del(listHead);
        /* free the allocated memory */
        unifi_net_data_free(priv, &tx_q_item->bulkdata);
        kfree(tx_q_item);
        tx_q_item = NULL;
        if (!priv->noOfPktQueuedInDriver) {
            unifi_error(priv, "packets queued in driver 0 still decrementing in %s\n", __FUNCTION__);
        } else {
            priv->noOfPktQueuedInDriver--;
        }
    }
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
}
void uf_flush_maPktlist(unifi_priv_t * priv, struct list_head * list)
{
    struct list_head *listHeadMaPktreq,*placeHolderMaPktreq;
    maPktReqList_t *maPktreq;
    unsigned long lock_flags;

    unifi_trace(priv, UDBG5, "entering the uf_flush_maPktlist \n");

    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    /* go through list, delete & free memory */
    list_for_each_safe(listHeadMaPktreq, placeHolderMaPktreq, list) {
        maPktreq = list_entry(listHeadMaPktreq, maPktReqList_t, q);

        if(!maPktreq) {
            unifi_error(priv, "entry should exists, otherwise crashes (bug)\n");
        }
        /* free the allocated memory */
        dev_kfree_skb(maPktreq->skb);
        list_del(listHeadMaPktreq);
        kfree(maPktreq);
        maPktreq = NULL;

    }
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
}
tx_buffered_packets_t *dequeue_tx_data_pdu(unifi_priv_t *priv, struct list_head *txList)
{
    /* dequeue the tx data packets from the appropriate queue */
    tx_buffered_packets_t *tx_q_item = NULL;
    struct list_head *listHead;
    struct list_head *placeHolder;
    unsigned long lock_flags;

    unifi_trace(priv, UDBG5, "entering dequeue_tx_data_pdu\n");
    /* check for list empty */
    if (list_empty(txList)) {
        unifi_trace(priv, UDBG5, "In dequeue_tx_data_pdu, the list is empty\n");
        return NULL;
    }

    /* Verification, if packet count is negetive */
    if (priv->noOfPktQueuedInDriver == 0xFFFF) {
        unifi_warning(priv, "no packet available in queue: debug");
        return NULL;
    }

    /* return first node after header, & delete from the list  && atleast one item exist */
    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    list_for_each_safe(listHead, placeHolder, txList) {
        tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);
        list_del(listHead);
        break;
    }
    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);

    if (tx_q_item) {
        unifi_trace(priv, UDBG5,
                "proccess_tx:  In dequeue_tx_data_pdu peerMacAddress=%02X%02X%02X%02X%02X%02X senderProcessId=%x\n",
                tx_q_item->peerMacAddress.a[0], tx_q_item->peerMacAddress.a[1],
                tx_q_item->peerMacAddress.a[2], tx_q_item->peerMacAddress.a[3],
                tx_q_item->peerMacAddress.a[4], tx_q_item->peerMacAddress.a[5],
                tx_q_item->leSenderProcessId);
    }

    unifi_trace(priv, UDBG5, "leaving dequeue_tx_data_pdu\n");
    return tx_q_item;
}
/* generic function to get the station record handler */
CsrWifiRouterCtrlStaInfo_t *CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(unifi_priv_t *priv,
        const CsrUint8 *peerMacAddress,
        CsrUint16 interfaceTag)
{
    CsrUint8 i;
    netInterface_priv_t *interfacePriv;
    unsigned long lock_flags;

    if (interfaceTag >= CSR_WIFI_NUM_INTERFACES) {
        unifi_error(priv, "interfaceTag is not proper, interfaceTag = %d\n", interfaceTag);
        return NULL;
    }

    interfacePriv = priv->interfacePriv[interfaceTag];

    /* disable the preemption untill station record is fetched */
    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);

    for (i = 0; i < UNIFI_MAX_CONNECTIONS; i++) {
        if (interfacePriv->staInfo[i]!= NULL) {
            if (!memcmp(((CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[i]))->peerMacAddress.a, peerMacAddress, ETH_ALEN)) {
                /* enable the preemption as station record is fetched */
                spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
                unifi_trace(priv, UDBG5, "peer entry found in station record\n");
                return ((CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[i]));
            }
        }
    }
    /* enable the preemption as station record is fetched */
    spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
    unifi_trace(priv, UDBG5, "peer entry not found in station record\n");
    return NULL;
}
/* generic function to get the station record handler from the handle */
CsrWifiRouterCtrlStaInfo_t * CsrWifiRouterCtrlGetStationRecordFromHandle(unifi_priv_t *priv,
                                                                 CsrUint32 handle,
                                                                 CsrUint16 interfaceTag)
{
    netInterface_priv_t *interfacePriv;

    if ((handle >= UNIFI_MAX_CONNECTIONS) || (interfaceTag >= CSR_WIFI_NUM_INTERFACES)) {
        unifi_error(priv, "handle/interfaceTag is not proper, handle = %d, interfaceTag = %d\n", handle, interfaceTag);
        return NULL;
    }
    interfacePriv = priv->interfacePriv[interfaceTag];
    return ((CsrWifiRouterCtrlStaInfo_t *) (interfacePriv->staInfo[handle]));
}

/* Function to do inactivity */
void uf_check_inactivity(unifi_priv_t *priv, CsrUint16 interfaceTag, CsrTime currentTime)
{
    CsrUint32 i;
    CsrWifiRouterCtrlStaInfo_t *staInfo;
    CsrTime elapsedTime;    /* Time in microseconds */
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CsrWifiMacAddress peerMacAddress;
    unsigned long lock_flags;

    if (interfacePriv == NULL) {
        unifi_trace(priv, UDBG3, "uf_check_inactivity: Interface priv is NULL \n");
        return;
    }

    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
    /* Go through the list of stations to check for inactivity */
    for(i = 0; i < UNIFI_MAX_CONNECTIONS; i++) {
        staInfo =  CsrWifiRouterCtrlGetStationRecordFromHandle(priv, i, interfaceTag);
        if(!staInfo ) {
            continue;
        }

        unifi_trace(priv, UDBG3, "Running Inactivity handler Time %xus station's last activity %xus\n",
                currentTime, staInfo->lastActivity);


        elapsedTime = (currentTime >= staInfo->lastActivity)?
                (currentTime - staInfo->lastActivity):
                (~((CsrUint32)0) - staInfo->lastActivity + currentTime);
        spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);

        if (elapsedTime > MAX_INACTIVITY_INTERVAL) {
            memcpy((CsrUint8*)&peerMacAddress, (CsrUint8*)&staInfo->peerMacAddress, sizeof(CsrWifiMacAddress));

            /* Indicate inactivity for the station */
            unifi_trace(priv, UDBG3, "Station %x:%x:%x:%x:%x:%x inactive since %xus\n sending Inactive Ind\n",
                        peerMacAddress.a[0], peerMacAddress.a[1],
                        peerMacAddress.a[2], peerMacAddress.a[3],
                        peerMacAddress.a[4], peerMacAddress.a[5],
                        elapsedTime);

            CsrWifiRouterCtrlStaInactiveIndSend(priv->CSR_WIFI_SME_IFACEQUEUE, 0, interfaceTag, peerMacAddress);
        }
    }

    interfacePriv->last_inactivity_check = currentTime;
}

/* Function to update activity of a station */
void uf_update_sta_activity(unifi_priv_t *priv, CsrUint16 interfaceTag, const CsrUint8 *peerMacAddress)
{
    CsrTime elapsedTime, currentTime;    /* Time in microseconds */
    CsrTime timeHi;         /* Not used - Time in microseconds */
    CsrWifiRouterCtrlStaInfo_t *staInfo;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    unsigned long lock_flags;

    if (interfacePriv == NULL) {
        unifi_trace(priv, UDBG3, "uf_check_inactivity: Interface priv is NULL \n");
        return;
    }

    currentTime = CsrTimeGet(&timeHi);


    staInfo = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, peerMacAddress, interfaceTag);

    if (staInfo == NULL) {
        unifi_trace(priv, UDBG4, "Sta does not exist yet");
        return;
    }

    spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
    /* Update activity */
    staInfo->lastActivity = currentTime;

    /* See if inactivity handler needs to be run
     * Here it is theoretically possible that the counter may have wrapped around. But
     * since we just want to know when to run the inactivity handler it does not really matter.
     * Especially since this is data path it makes sense in keeping it simple and avoiding
     * 64 bit handling */
    elapsedTime = (currentTime >= interfacePriv->last_inactivity_check)?
                    (currentTime - interfacePriv->last_inactivity_check):
                    (~((CsrUint32)0) - interfacePriv->last_inactivity_check + currentTime);

    spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);

    /* Check if it is time to run the inactivity handler */
    if (elapsedTime > INACTIVITY_CHECK_INTERVAL) {
        uf_check_inactivity(priv, interfaceTag, currentTime);
    }
}
void resume_unicast_buffered_frames(unifi_priv_t *priv, CsrUint16 interfaceTag)
{

   netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
   CsrUint8 i;
   int j;
   tx_buffered_packets_t * buffered_pkt = NULL;
   CsrBool hipslotFree[4] = {TRUE,TRUE,TRUE,TRUE};
   int r;
   unsigned long lock_flags;

   func_enter();
   while(!isRouterBufferEnabled(priv,3) &&
                            ((buffered_pkt=dequeue_tx_data_pdu(priv,&interfacePriv->genericMgtFrames))!=NULL)) {
        buffered_pkt->transmissionControl &=
                     ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);
        if((r=frame_and_send_queued_pdu(priv,buffered_pkt,NULL,0,FALSE)) == -ENOSPC) {
            /* Enqueue at the head of the queue */
            spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
            list_add(&buffered_pkt->q, &interfacePriv->genericMgtFrames);
            spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
            hipslotFree[3]=FALSE;
            break;
        }else {
            if(r){
                unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                /* the PDU failed where we can't do any thing so free the storage */
                unifi_net_data_free(priv, &buffered_pkt->bulkdata);
            }
            kfree(buffered_pkt);
        }
   }
   for(i = 0; i < UNIFI_MAX_CONNECTIONS; i++) {
        CsrWifiRouterCtrlStaInfo_t *staInfo = interfacePriv->staInfo[i];
        if(!hipslotFree[0] && !hipslotFree[1] && !hipslotFree[2] && !hipslotFree[3]) {
            unifi_trace(priv, UDBG3, "(ENOSPC) in resume_unicast_buffered_frames:: hip slots are full \n");
            break;
        }
        if (staInfo && (staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_ACTIVE)) {
          while((( TRUE == hipslotFree[3] ) && (buffered_pkt=dequeue_tx_data_pdu(priv, &staInfo->mgtFrames)))) {
              buffered_pkt->transmissionControl &=
                           ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);
              if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staInfo,0,FALSE)) == -ENOSPC) {
                  unifi_trace(priv, UDBG3, "(ENOSPC) in resume_unicast_buffered_frames:: hip slots are full for voice queue\n");
                  /* Enqueue at the head of the queue */
                  spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                  list_add(&buffered_pkt->q, &staInfo->mgtFrames);
                  spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                  priv->pausedStaHandle[3]=(CsrUint8)(staInfo->assignedHandle);
                  hipslotFree[3] = FALSE;
                  break;
              } else {
                  if(r){
                      unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                      /* the PDU failed where we can't do any thing so free the storage */
                      unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                  }
                  kfree(buffered_pkt);
              }
          }

          for(j=3;j>=0;j--) {
              if(!hipslotFree[j])
                  continue;

              while((buffered_pkt=dequeue_tx_data_pdu(priv, &staInfo->dataPdu[j]))) {
                 buffered_pkt->transmissionControl &=
                            ~(TRANSMISSION_CONTROL_TRIGGER_MASK|TRANSMISSION_CONTROL_ESOP_MASK);
                 if((r=frame_and_send_queued_pdu(priv,buffered_pkt,staInfo,0,FALSE)) == -ENOSPC) {
                     /* Enqueue at the head of the queue */
                     spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
                     list_add(&buffered_pkt->q, &staInfo->dataPdu[j]);
                     spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
                     priv->pausedStaHandle[j]=(CsrUint8)(staInfo->assignedHandle);
                     hipslotFree[j]=FALSE;
                     break;
                 } else {
                    if(r){
                        unifi_trace (priv, UDBG1, " HIP validation failure : PDU sending failed \n");
                        /* the PDU failed where we can't do any thing so free the storage */
                        unifi_net_data_free(priv, &buffered_pkt->bulkdata);
                     }
                    kfree(buffered_pkt);
                 }
              }
          }
       }
    }
    func_exit();
}
void update_eosp_to_head_of_broadcast_list_head(unifi_priv_t *priv,CsrUint16 interfaceTag)
{

    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    unsigned long lock_flags;
    struct list_head *listHead;
    struct list_head *placeHolder;
    tx_buffered_packets_t *tx_q_item;

    func_enter();
    if (interfacePriv->noOfbroadcastPktQueued) {

        /* Update the EOSP to the HEAD of b/c list
         * beacuse we have received any mgmt packet so it should not hold for long time
         * peer may time out.
         */
        spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
        list_for_each_safe(listHead, placeHolder, &interfacePriv->genericMulticastOrBroadCastFrames) {
            tx_q_item = list_entry(listHead, tx_buffered_packets_t, q);
            tx_q_item->transmissionControl |= TRANSMISSION_CONTROL_ESOP_MASK;
            tx_q_item->transmissionControl = (tx_q_item->transmissionControl & ~(CSR_NO_CONFIRM_REQUIRED));
            unifi_trace(priv, UDBG1,"updating eosp for list Head hostTag:= 0x%x ",tx_q_item->hostTag);
            break;
        }
        spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
    }
    func_exit();
}
void resume_suspended_uapsd(unifi_priv_t* priv,CsrUint16 interfaceTag)
{

   CsrUint8 startIndex;
   CsrWifiRouterCtrlStaInfo_t * staInfo = NULL;
   unsigned long lock_flags;
   /*U-APSD might have stopped because of multicast. So restart it if U-APSD
   was active with any of the station*/
    for(startIndex= 0; startIndex < UNIFI_MAX_CONNECTIONS;startIndex++) {
        staInfo =  CsrWifiRouterCtrlGetStationRecordFromHandle(priv,startIndex,interfaceTag);
        if(!staInfo ) {
            continue;
        } else if((staInfo->currentPeerState == CSR_WIFI_ROUTER_CTRL_PEER_CONNECTED_POWER_SAVE)
                   &&(staInfo->uapsdSuspended == TRUE)) {

            /*U-APSD Still active, it means trigger frame is received,so continue U-APSD by
            sending data from remaining delivery enabled queues*/
            spin_lock_irqsave(&priv->staRecord_lock,lock_flags);
            staInfo->uapsdActive = TRUE;
            staInfo->uapsdSuspended = FALSE;
            spin_unlock_irqrestore(&priv->staRecord_lock,lock_flags);
            uf_continue_uapsd(priv,staInfo);
        }
    }

}
void uf_store_directed_ma_packet_referenece(unifi_priv_t *priv, bulk_data_param_t *bulkdata,
                                            CSR_SIGNAL *sigptr, CsrUint32 alignOffset)
{

    maPktReqList_t *maPktreq = NULL;
    CSR_MA_PACKET_REQUEST *req = &sigptr->u.MaPacketRequest;
    CsrWifiRouterCtrlStaInfo_t *staRecord = NULL;
    CsrUint16 frmCtrl,interfaceTag = 0;
    const CsrUint8* macHdrLocation;
    struct sk_buff *skb ;
    unsigned long lock_flags;
    netInterface_priv_t *interfacePriv = priv->interfacePriv[interfaceTag];
    CsrUint8 *sigbuffer;
    CsrUint8 frameType = 0;
    func_enter();

    if(bulkdata == NULL || (0 == bulkdata->d[0].data_length )){
      unifi_trace (priv, UDBG3, "uf_store_directed_ma_packet_referenece:bulk data NULL \n");
      func_exit();
      return;
    }
    macHdrLocation = bulkdata->d[0].os_data_ptr;
    skb = (struct sk_buff*)bulkdata->d[0].os_net_buf_ptr;
    /* fectch the frame control value from mac header */
    frmCtrl = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(macHdrLocation);

    /* Processing done according to Frame/Packet type */
    frameType =  ((frmCtrl & 0x000c) >> FRAME_CONTROL_TYPE_FIELD_OFFSET);

    if( (((frmCtrl & 0xff) == IEEE802_11_FC_TYPE_QOS_NULL) ||
        ((frmCtrl & 0xff) == IEEE802_11_FC_TYPE_NULL ) ) ||
        ( IEEE802_11_FRAMETYPE_MANAGEMENT== frameType)){

        /* if packet is NULL or Qos Null no need of retransmit so dont queue it*/
        unifi_trace (priv, UDBG3, "uf_store_directed_ma_packet_referenece: NULL data Pkt or mgmt\n");
        func_exit();
        return;
    }

    /* fetch the station record for corresponding peer mac address */
    if ((staRecord = CsrWifiRouterCtrlGetStationRecordFromPeerMacAddress(priv, req->Ra.x, interfaceTag))) {
        maPktreq = (maPktReqList_t*)kmalloc(sizeof(maPktReqList_t),GFP_ATOMIC);
        if(maPktreq == NULL){
            unifi_error(priv,
                "uf_store_directed_ma_packet_referenece :: Failed to allocate %d byter for maPktreq\n",
                sizeof(maPktReqList_t));
            func_exit();
            return;
        }
    }

    /* staRecord not present that means packet is multicast or generic mgmt so no need to queue it */
    else{
        unifi_trace (priv, UDBG3, "uf_store_directed_ma_packet_referenece: multicast pkt \n");
        func_exit();
        return ;
    }

    /* disbale preemption */
    spin_lock_irqsave(&priv->tx_q_lock,lock_flags);
    INIT_LIST_HEAD(&maPktreq->q);
    maPktreq->staHandler = staRecord->assignedHandle;
    memcpy(&maPktreq->signal,sigptr,sizeof(CSR_SIGNAL_PRIMITIVE_HEADER) + sizeof(CSR_MA_PACKET_REQUEST));
    sigbuffer = (CsrUint8*)&maPktreq->signal;
    sigbuffer[SIZEOF_SIGNAL_HEADER + 1] = alignOffset;
    maPktreq->skb = skb_get(skb);
    maPktreq->hostTag = req->HostTag;
    maPktreq->jiffeTime = jiffies;
    list_add_tail(&maPktreq->q,&interfacePriv->directedMaPktReq);

    spin_unlock_irqrestore(&priv->tx_q_lock,lock_flags);
    func_exit();

}

#endif
