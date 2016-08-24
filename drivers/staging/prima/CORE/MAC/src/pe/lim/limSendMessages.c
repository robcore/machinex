/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 *
 * Airgo Networks, Inc proprietary. All rights reserved.
 * limSendMessages.c: Provides functions to send messages or Indications to HAL.
 * Author:    Sunit Bhatia
 * Date:       09/21/2006
 * History:-
 * Date        Modified by            Modification Information
 *
 * --------------------------------------------------------------------------
 *
 */
#include "limSendMessages.h"
#include "cfgApi.h"
#include "limTrace.h"
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM //FEATURE_WLAN_DIAG_SUPPORT
#include "vos_diag_core_log.h"
#endif //FEATURE_WLAN_DIAG_SUPPORT 
/* When beacon filtering is enabled, firmware will
 * analyze the selected beacons received during BMPS,
 * and monitor any changes in the IEs as listed below.
 * The format of the table is:
 *    - EID
 *    - Check for IE presence
 *    - Byte offset
 *    - Byte value
 *    - Bit Mask
 *    - Byte refrence
 */
static tBeaconFilterIe beaconFilterTable[] = {
   {SIR_MAC_DS_PARAM_SET_EID,    0, {0, 0, DS_PARAM_CHANNEL_MASK, 0}},
   {SIR_MAC_ERP_INFO_EID,        0, {0, 0, ERP_FILTER_MASK,       0}},
   {SIR_MAC_EDCA_PARAM_SET_EID,  0, {0, 0, EDCA_FILTER_MASK,      0}},
   {SIR_MAC_QOS_CAPABILITY_EID,  0, {0, 0, QOS_FILTER_MASK,       0}},
   {SIR_MAC_CHNL_SWITCH_ANN_EID, 1, {0, 0, 0,                     0}},
   {SIR_MAC_QUIET_EID,           1, {0, 0, 0,                     0}},
   {SIR_MAC_HT_INFO_EID,         0, {0, 0, HT_BYTE0_FILTER_MASK,  0}},  
   {SIR_MAC_HT_INFO_EID,         0, {2, 0, HT_BYTE2_FILTER_MASK,  0}}, 
   {SIR_MAC_HT_INFO_EID,         0, {5, 0, HT_BYTE5_FILTER_MASK,  0}}
#if defined WLAN_FEATURE_VOWIFI
   ,{SIR_MAC_PWR_CONSTRAINT_EID,  0, {0, 0, 0, 0}}
#endif
};

/**
 * limSendCFParams()
 *
 *FUNCTION:
 * This function is called to send CFP Parameters to WDA, when they are changed.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param pMac  pointer to Global Mac structure.
 * @param bssIdx Bss Index of the BSS to which STA is associated.
 * @param cfpCount CFP Count, if that is changed.
 * @param cfpPeriod CFP Period if that is changed.
 *
 * @return success if message send is ok, else false.
 */
tSirRetStatus limSendCFParams(tpAniSirGlobal pMac, tANI_U8 bssIdx, tANI_U8 cfpCount, tANI_U8 cfpPeriod)
{
    tpUpdateCFParams pCFParams = NULL;
    tSirRetStatus   retCode = eSIR_SUCCESS;
    tSirMsgQ msgQ;

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pCFParams,
          sizeof( tUpdateCFParams )))
      {
        limLog( pMac, LOGP,
            FL( "Unable to PAL allocate memory during Update CF Params\n" ));
        retCode = eSIR_MEM_ALLOC_FAILED;
        goto returnFailure;
      }
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pCFParams, sizeof(tUpdateCFParams));
    pCFParams->cfpCount = cfpCount;
    pCFParams->cfpPeriod = cfpPeriod;
    pCFParams->bssIdx     = bssIdx;

    msgQ.type = WDA_UPDATE_CF_IND;
    msgQ.reserved = 0;
    msgQ.bodyptr = pCFParams;
    msgQ.bodyval = 0;
    limLog( pMac, LOG3,
                FL( "Sending WDA_UPDATE_CF_IND..." ));
    MTRACE(macTraceMsgTx(pMac, NO_SESSION, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        palFreeMemory(pMac->hHdd, pCFParams);
        limLog( pMac, LOGP,
                    FL("Posting  WDA_UPDATE_CF_IND to WDA failed, reason=%X\n"),
                    retCode );
    }
returnFailure:
    return retCode;
}

/**
 * limSendBeaconParams()
 *
 *FUNCTION:
 * This function is called to send beacnon interval, short preamble or other
 * parameters to WDA, which are changed and indication is received in beacon.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param pMac  pointer to Global Mac structure.
 * @param tpUpdateBeaconParams pointer to the structure,
                which contains the beacon parameters which are changed.
 *
 * @return success if message send is ok, else false.
 */
tSirRetStatus limSendBeaconParams(tpAniSirGlobal pMac, 
                                  tpUpdateBeaconParams pUpdatedBcnParams,
                                  tpPESession  psessionEntry )
{
    tpUpdateBeaconParams pBcnParams = NULL;
    tSirRetStatus   retCode = eSIR_SUCCESS;
    tSirMsgQ msgQ;

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pBcnParams, sizeof(*pBcnParams)))
    {
        limLog( pMac, LOGP,
            FL( "Unable to PAL allocate memory during Update Beacon Params\n" ));
        return eSIR_MEM_ALLOC_FAILED;
    }
    palCopyMemory( pMac->hHdd, (tANI_U8 *) pBcnParams,  pUpdatedBcnParams, sizeof(*pBcnParams));
    msgQ.type = WDA_UPDATE_BEACON_IND;
    msgQ.reserved = 0;
    msgQ.bodyptr = pBcnParams;
    msgQ.bodyval = 0;
    PELOG3(limLog( pMac, LOG3,
                FL( "Sending WDA_UPDATE_BEACON_IND, paramChangeBitmap in hex = %x" ),
                    pUpdatedBcnParams->paramChangeBitmap);)
    MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        palFreeMemory(pMac->hHdd, pBcnParams);
        limLog( pMac, LOGP,
                    FL("Posting  WDA_UPDATE_BEACON_IND to WDA failed, reason=%X\n"),
                    retCode );
    }
#ifdef WLAN_SOFTAP_FEATURE
    limSendBeaconInd(pMac, psessionEntry);
#endif
    return retCode;
}

/**
 * limSendSwitchChnlParams()
 *
 *FUNCTION:
 * This function is called to send Channel Switch Indication to WDA
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param pMac  pointer to Global Mac structure.
 * @param chnlNumber New Channel Number to be switched to.
 * @param secondaryChnlOffset  an enum for secondary channel offset.
 * @param localPowerConstraint 11h local power constraint value
 *
 * @return success if message send is ok, else false.
 */
#if !defined WLAN_FEATURE_VOWIFI  
tSirRetStatus limSendSwitchChnlParams(tpAniSirGlobal pMac,
                                      tANI_U8 chnlNumber,
                                      ePhyChanBondState secondaryChnlOffset,
                                      tANI_U8 localPwrConstraint, tANI_U8 peSessionId)
#else
tSirRetStatus limSendSwitchChnlParams(tpAniSirGlobal pMac,
                                      tANI_U8 chnlNumber,
                                      ePhyChanBondState secondaryChnlOffset,
                                      tPowerdBm maxTxPower, tANI_U8 peSessionId)
#endif
{
    tpSwitchChannelParams pChnlParams = NULL;
    tSirRetStatus   retCode = eSIR_SUCCESS;
    tSirMsgQ msgQ;
    tpPESession pSessionEntry;
    if((pSessionEntry = peFindSessionBySessionId(pMac , peSessionId)) == NULL)
    {
       limLog( pMac, LOGP,
             FL( "Unable to get Session for session Id %d\n" ), peSessionId);
       return eSIR_FAILURE;
    }
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pChnlParams,
          sizeof( tSwitchChannelParams )))
      {
          limLog( pMac, LOGP,
            FL( "Unable to PAL allocate memory during Switch Channel Params\n" ));
        retCode = eSIR_MEM_ALLOC_FAILED;
        goto returnFailure;
      }
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pChnlParams, sizeof(tSwitchChannelParams));
    pChnlParams->secondaryChannelOffset = secondaryChnlOffset;
    pChnlParams->channelNumber= chnlNumber;
#if defined WLAN_FEATURE_VOWIFI  
    pChnlParams->maxTxPower = maxTxPower;
    palCopyMemory( pMac->hHdd, pChnlParams->selfStaMacAddr, pSessionEntry->selfMacAddr, sizeof(tSirMacAddr) );
#else
    pChnlParams->localPowerConstraint = localPwrConstraint;
#endif
    palCopyMemory( pMac->hHdd, pChnlParams->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );
    pChnlParams->peSessionId = peSessionId;
    
    //we need to defer the message until we get the response back from WDA.
    SET_LIM_PROCESS_DEFD_MESGS(pMac, false);
    msgQ.type = WDA_CHNL_SWITCH_REQ;
    msgQ.reserved = 0;
    msgQ.bodyptr = pChnlParams;
    msgQ.bodyval = 0;
#if defined WLAN_FEATURE_VOWIFI  
    PELOG3(limLog( pMac, LOG3,
        FL( "Sending WDA_CHNL_SWITCH_REQ with SecondaryChnOffset - %d, ChannelNumber - %d, maxTxPower - %d"),
        pChnlParams->secondaryChannelOffset, pChnlParams->channelNumber, pChnlParams->maxTxPower);)
#else
    PELOG3(limLog( pMac, LOG3,
        FL( "Sending WDA_CHNL_SWITCH_REQ with SecondaryChnOffset - %d, ChannelNumber - %d, LocalPowerConstraint - %d"),
        pChnlParams->secondaryChannelOffset, pChnlParams->channelNumber, pChnlParams->localPowerConstraint);)
#endif
    MTRACE(macTraceMsgTx(pMac, peSessionId, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        palFreeMemory(pMac->hHdd, pChnlParams);
        limLog( pMac, LOGP,
                    FL("Posting  WDA_CHNL_SWITCH_REQ to WDA failed, reason=%X\n"),
                    retCode );
    }
returnFailure:
    return retCode;
}

/**
 * limSendEdcaParams()
 *
 *FUNCTION:
 * This function is called to send dynamically changing EDCA Parameters to WDA.
 *
 *LOGIC:
 *
 *ASSUMPTIONS:
 * NA
 *
 *NOTE:
 * NA
 *
 * @param pMac  pointer to Global Mac structure.
 * @param tpUpdatedEdcaParams pointer to the structure which contains
 *                                       dynamically changing EDCA parameters.
 * @param highPerformance  If the peer is Airgo (taurus) then switch to highPerformance is true.
 *
 * @return success if message send is ok, else false.
 */
tSirRetStatus limSendEdcaParams(tpAniSirGlobal pMac, tSirMacEdcaParamRecord *pUpdatedEdcaParams, tANI_U16 bssIdx, tANI_BOOLEAN highPerformance)
{
    tEdcaParams *pEdcaParams = NULL;
    tSirRetStatus   retCode = eSIR_SUCCESS;
    tSirMsgQ msgQ;
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pEdcaParams,
          sizeof(tEdcaParams)))
    {
        limLog( pMac, LOGP,
            FL( "Unable to PAL allocate memory during Update EDCA Params\n" ));
        retCode = eSIR_MEM_ALLOC_FAILED;
        return retCode;
    }
    pEdcaParams->bssIdx = bssIdx;
    pEdcaParams->acbe = pUpdatedEdcaParams[EDCA_AC_BE];
    pEdcaParams->acbk = pUpdatedEdcaParams[EDCA_AC_BK];
    pEdcaParams->acvi = pUpdatedEdcaParams[EDCA_AC_VI];
    pEdcaParams->acvo = pUpdatedEdcaParams[EDCA_AC_VO];
    pEdcaParams->highPerformance = highPerformance;
    msgQ.type = WDA_UPDATE_EDCA_PROFILE_IND;
    msgQ.reserved = 0;
    msgQ.bodyptr = pEdcaParams;
    msgQ.bodyval = 0;
    {
        tANI_U8 i;
        PELOG1(limLog( pMac, LOG1,FL("Sending WDA_UPDATE_EDCA_PROFILE_IND with EDCA Parameters:" ));)
        for(i=0; i<MAX_NUM_AC; i++)
        {
            PELOG1(limLog(pMac, LOG1, FL("AC[%d]:  AIFSN %d, ACM %d, CWmin %d, CWmax %d, TxOp %d \n"),  
                   i, pUpdatedEdcaParams[i].aci.aifsn, pUpdatedEdcaParams[i].aci.acm, 
                   pUpdatedEdcaParams[i].cw.min, pUpdatedEdcaParams[i].cw.max, pUpdatedEdcaParams[i].txoplimit);)
        }
    }
    MTRACE(macTraceMsgTx(pMac, NO_SESSION, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        palFreeMemory(pMac->hHdd, pEdcaParams);
        limLog( pMac, LOGP,
                    FL("Posting  WDA_UPDATE_EDCA_PROFILE_IND to WDA failed, reason=%X\n"),
                    retCode );
    }
    return retCode;
}

/**
 * limSetActiveEdcaParams()
 *
 * FUNCTION:
 *  This function is called to set the most up-to-date EDCA parameters
 *  given the default local EDCA parameters.  The rules are as following:
 *  - If ACM bit is set for all ACs, then downgrade everything to Best Effort.
 *  - If ACM is not set for any AC, then PE will use the default EDCA
 *    parameters as advertised by AP.
 *  - If ACM is set in any of the ACs, PE will use the EDCA parameters
 *    from the next best AC for which ACM is not enabled.
 *
 * @param pMac  pointer to Global Mac structure.
 * @param plocalEdcaParams pointer to the local EDCA parameters
 * @ param psessionEntry point to the session entry
 * @return none
 */
 void limSetActiveEdcaParams(tpAniSirGlobal pMac, tSirMacEdcaParamRecord *plocalEdcaParams, tpPESession psessionEntry)
{
    tANI_U8   ac, newAc, i;
    tANI_U8   acAdmitted;
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM //FEATURE_WLAN_DIAG_SUPPORT
    vos_log_qos_edca_pkt_type *log_ptr = NULL;
#endif //FEATURE_WLAN_DIAG_SUPPORT 
    // Initialize gLimEdcaParamsActive[] to be same as localEdcaParams
    psessionEntry->gLimEdcaParamsActive[EDCA_AC_BE] = plocalEdcaParams[EDCA_AC_BE];
    psessionEntry->gLimEdcaParamsActive[EDCA_AC_BK] = plocalEdcaParams[EDCA_AC_BK];
    psessionEntry->gLimEdcaParamsActive[EDCA_AC_VI] = plocalEdcaParams[EDCA_AC_VI];
    psessionEntry->gLimEdcaParamsActive[EDCA_AC_VO] = plocalEdcaParams[EDCA_AC_VO];
    /* An AC requires downgrade if the ACM bit is set, and the AC has not
     * yet been admitted in uplink or bi-directions.
     * If an AC requires downgrade, it will downgrade to the next beset AC
     * for which ACM is not enabled.
     *
     * - There's no need to downgrade AC_BE since it IS the lowest AC. Hence
     *   start the for loop with AC_BK.
     * - If ACM bit is set for an AC, initially downgrade it to AC_BE. Then
     *   traverse thru the AC list. If we do find the next best AC which is
     *   better than AC_BE, then use that one. For example, if ACM bits are set
     *   such that: BE_ACM=1, BK_ACM=1, VI_ACM=1, VO_ACM=0
     *   then all AC will be downgraded to AC_BE.
     */
    limLog(pMac, LOG1, FL("adAdmitMask[UPLINK] = 0x%x \n"),  pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_UPLINK] );
    limLog(pMac, LOG1, FL("adAdmitMask[DOWNLINK] = 0x%x \n"),  pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_DNLINK] );
    for (ac = EDCA_AC_BK; ac <= EDCA_AC_VO; ac++)
    {
        acAdmitted = ( (pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_UPLINK] & (1 << ac)) >> ac );
        limLog(pMac, LOG1, FL("For AC[%d]: acm=%d,  acAdmit=%d \n"), ac, plocalEdcaParams[ac].aci.acm, acAdmitted);
        if ( (plocalEdcaParams[ac].aci.acm == 1) && (acAdmitted == 0) )
        {
            limLog(pMac, LOG1, FL("We need to downgrade AC %d!! "), ac);
            newAc = EDCA_AC_BE;
            for (i=ac-1; i>0; i--)
            {
                if (plocalEdcaParams[i].aci.acm == 0)
                {
                    newAc = i;
                    break;
                }
            }
            limLog(pMac, LOGW, FL("Downgrading AC %d ---> AC %d "), ac, newAc);
            psessionEntry->gLimEdcaParamsActive[ac] = plocalEdcaParams[newAc];
        }
    }
//log: LOG_WLAN_QOS_EDCA_C
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM //FEATURE_WLAN_DIAG_SUPPORT
    WLAN_VOS_DIAG_LOG_ALLOC(log_ptr, vos_log_qos_edca_pkt_type, LOG_WLAN_QOS_EDCA_C);
    if(log_ptr)
    {
       log_ptr->aci_be = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BE].aci.aci;
       log_ptr->cw_be  = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BE].cw.max << 4 |
          psessionEntry->gLimEdcaParamsActive[EDCA_AC_BE].cw.min;
       log_ptr->txoplimit_be = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BE].txoplimit;
       log_ptr->aci_bk = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BK].aci.aci;
       log_ptr->cw_bk  = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BK].cw.max << 4 |
          psessionEntry->gLimEdcaParamsActive[EDCA_AC_BK].cw.min;
       log_ptr->txoplimit_bk = psessionEntry->gLimEdcaParamsActive[EDCA_AC_BK].txoplimit;
       log_ptr->aci_vi = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VI].aci.aci;
       log_ptr->cw_vi  = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VI].cw.max << 4 |
          psessionEntry->gLimEdcaParamsActive[EDCA_AC_VI].cw.min;
       log_ptr->txoplimit_vi = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VI].txoplimit;
       log_ptr->aci_vo = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VO].aci.aci;
       log_ptr->cw_vo  = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VO].cw.max << 4 |
          psessionEntry->gLimEdcaParamsActive[EDCA_AC_VO].cw.min;
       log_ptr->txoplimit_vo = psessionEntry->gLimEdcaParamsActive[EDCA_AC_VO].txoplimit;
    }
    WLAN_VOS_DIAG_LOG_REPORT(log_ptr);
#endif //FEATURE_WLAN_DIAG_SUPPORT
    
    return;
 }

/** ---------------------------------------------------------
\fn      limSetLinkState
\brief   LIM sends a message to WDA to set the link state
\param   tpAniSirGlobal  pMac
\param   tSirLinkState      state
\return  None
  -----------------------------------------------------------*/
 //Original code with out anu's change
#if 0
tSirRetStatus limSetLinkState(tpAniSirGlobal pMac, tSirLinkState state,tSirMacAddr bssId)
{
    tSirMsgQ msg;
    tSirRetStatus retCode;
    msg.type = WDA_SET_LINK_STATE;
    msg.bodyval = (tANI_U32) state;
    msg.bodyptr = NULL;
    MTRACE(macTraceMsgTx(pMac, 0, msg.type));
    retCode = wdaPostCtrlMsg(pMac, &msg);
    if (retCode != eSIR_SUCCESS)
        limLog(pMac, LOGP, FL("Posting link state %d failed, reason = %x \n"), retCode);
    return retCode;
}
#endif //0
tSirRetStatus limSetLinkState(tpAniSirGlobal pMac, tSirLinkState state,tSirMacAddr bssId, 
                              tSirMacAddr selfMacAddr, tpSetLinkStateCallback callback, 
                              void *callbackArg) 
{
    tSirMsgQ msgQ;
    tSirRetStatus retCode;
    tpLinkStateParams pLinkStateParams = NULL;
    // Allocate memory.
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pLinkStateParams,
          sizeof(tLinkStateParams)))
    {
        limLog( pMac, LOGP,
        FL( "Unable to PAL allocate memory while sending Set Link State\n" ));
        retCode = eSIR_SME_RESOURCES_UNAVAILABLE;
        return retCode;
    }
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pLinkStateParams, sizeof(tLinkStateParams));
    pLinkStateParams->state        = state;
    pLinkStateParams->callback     = callback;
    pLinkStateParams->callbackArg  = callbackArg;
     
    /* Copy Mac address */
    sirCopyMacAddr(pLinkStateParams->bssid,bssId);
    sirCopyMacAddr(pLinkStateParams->selfMacAddr, selfMacAddr);

    msgQ.type = WDA_SET_LINK_STATE;
    msgQ.reserved = 0;
    msgQ.bodyptr = pLinkStateParams;
    msgQ.bodyval = 0;
    
    MTRACE(macTraceMsgTx(pMac, NO_SESSION, msgQ.type));

    retCode = (tANI_U32)wdaPostCtrlMsg(pMac, &msgQ);
    if (retCode != eSIR_SUCCESS)
    {
        palFreeMemory(pMac, (void*)pLinkStateParams);
        limLog(pMac, LOGP, FL("Posting link state %d failed, reason = %x \n"), retCode);
    }
    return retCode;
}
#ifdef WLAN_FEATURE_VOWIFI_11R
extern tSirRetStatus limSetLinkStateFT(tpAniSirGlobal pMac, tSirLinkState 
state,tSirMacAddr bssId, tSirMacAddr selfMacAddr, int ft, tpPESession psessionEntry)
{
    tSirMsgQ msgQ;
    tSirRetStatus retCode;
    tpLinkStateParams pLinkStateParams = NULL;
    // Allocate memory.
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pLinkStateParams,
          sizeof(tLinkStateParams)))
    {
        limLog( pMac, LOGP,
        FL( "Unable to PAL allocate memory while sending Set Link State\n" ));
        retCode = eSIR_SME_RESOURCES_UNAVAILABLE;
        return retCode;
    }
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pLinkStateParams, sizeof(tLinkStateParams));
    pLinkStateParams->state = state;
    /* Copy Mac address */
    sirCopyMacAddr(pLinkStateParams->bssid,bssId);
    sirCopyMacAddr(pLinkStateParams->selfMacAddr, selfMacAddr);
    pLinkStateParams->ft = 1;
    pLinkStateParams->session = psessionEntry;

    msgQ.type = WDA_SET_LINK_STATE;
    msgQ.reserved = 0;
    msgQ.bodyptr = pLinkStateParams;
    msgQ.bodyval = 0;
    
    MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msgQ.type));

    retCode = (tANI_U32)wdaPostCtrlMsg(pMac, &msgQ);
    if (retCode != eSIR_SUCCESS)
    {
        palFreeMemory(pMac, (void*)pLinkStateParams);
        limLog(pMac, LOGP, FL("Posting link state %d failed, reason = %x \n"), retCode);
    }
    return retCode;
}
#endif

/** ---------------------------------------------------------
\fn      limSendSetTxPowerReq
\brief   LIM sends a WDA_SET_TX_POWER_REQ message to WDA 
\param   tpAniSirGlobal      pMac
\param   tpSirSetTxPowerReq  request message
\return  None
  -----------------------------------------------------------*/
tSirRetStatus limSendSetTxPowerReq(tpAniSirGlobal pMac,  tpSirSetTxPowerReq pTxPowerReq)
{
    tSirRetStatus  retCode = eSIR_SUCCESS;
    tSirMsgQ       msgQ;
    if (NULL == pTxPowerReq)
        return retCode;
    msgQ.type = WDA_SET_TX_POWER_REQ;
    msgQ.reserved = 0;
    msgQ.bodyptr = pTxPowerReq;
    msgQ.bodyval = 0;
    PELOGW(limLog(pMac, LOGW, FL( "Sending WDA_SET_TX_POWER_REQ to WDA"));)
    MTRACE(macTraceMsgTx(pMac, NO_SESSION, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        limLog( pMac, LOGP, FL("Posting WDA_SET_TX_POWER_REQ to WDA failed, reason=%X"), retCode );
        if (NULL != pTxPowerReq)
        {
            palFreeMemory( pMac->hHdd, (tANI_U8 *) pTxPowerReq);
        }
        return retCode;
    }
    return retCode;
}
/** ---------------------------------------------------------
\fn      limSendGetTxPowerReq
\brief   LIM sends a WDA_GET_TX_POWER_REQ message to WDA
\param   tpAniSirGlobal      pMac
\param   tpSirGetTxPowerReq  request message
\return  None
  -----------------------------------------------------------*/
tSirRetStatus limSendGetTxPowerReq(tpAniSirGlobal pMac,  tpSirGetTxPowerReq pTxPowerReq)
{
    tSirRetStatus  retCode = eSIR_SUCCESS;
    tSirMsgQ       msgQ;
    if (NULL == pTxPowerReq)
        return retCode;
    msgQ.type = WDA_GET_TX_POWER_REQ;
    msgQ.reserved = 0;
    msgQ.bodyptr = pTxPowerReq;
    msgQ.bodyval = 0;
    PELOGW(limLog(pMac, LOGW, FL( "Sending WDA_GET_TX_POWER_REQ to WDA"));)
    MTRACE(macTraceMsgTx(pMac, NO_SESSION, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        limLog( pMac, LOGP, FL("Posting WDA_GET_TX_POWER_REQ to WDA failed, reason=%X"), retCode );
        if (NULL != pTxPowerReq)
        {
            palFreeMemory( pMac->hHdd, (tANI_U8 *) pTxPowerReq);
        }
        return retCode;
    }
    return retCode;
}
/** ---------------------------------------------------------
\fn      limSendBeaconFilterInfo
\brief   LIM sends beacon filtering info to WDA
\param   tpAniSirGlobal  pMac
\return  None
  -----------------------------------------------------------*/
tSirRetStatus limSendBeaconFilterInfo(tpAniSirGlobal pMac)
{
    tpBeaconFilterMsg  pBeaconFilterMsg = NULL;
    tSirRetStatus      retCode = eSIR_SUCCESS;
    tSirMsgQ           msgQ;
    tANI_U8            *ptr;
    tANI_U32           i;
    tANI_U32           msgSize;
    tpBeaconFilterIe   pIe;
    tpPESession psessionEntry = &pMac->lim.gpSession[0];  //TBD-RAJESH get the sessionEntry from the caller

    msgSize = sizeof(tBeaconFilterMsg) + sizeof(beaconFilterTable);
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
          (void **) &pBeaconFilterMsg, msgSize) )
    {
        limLog( pMac, LOGP, FL("Fail to allocate memory for beaconFiilterMsg \n"));
        retCode = eSIR_MEM_ALLOC_FAILED;
        return retCode;
    }
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pBeaconFilterMsg, msgSize);
    // Fill in capability Info and mask
    //TBD-RAJESH get the BSS capability from session.
    //Don't send this message if no active Infra session is found.
    pBeaconFilterMsg->capabilityInfo = psessionEntry->limCurrentBssCaps;
    pBeaconFilterMsg->capabilityMask = CAPABILITY_FILTER_MASK;
    pBeaconFilterMsg->beaconInterval = (tANI_U16) psessionEntry->beaconParams.beaconInterval;
    // Fill in number of IEs in beaconFilterTable
    pBeaconFilterMsg->ieNum = (tANI_U16) (sizeof(beaconFilterTable) / sizeof(tBeaconFilterIe));
    //Fill message with info contained in the beaconFilterTable
    ptr = (tANI_U8 *)pBeaconFilterMsg + sizeof(tBeaconFilterMsg);
    for(i=0; i < (pBeaconFilterMsg->ieNum); i++)
    {
        pIe = (tpBeaconFilterIe) ptr;
        pIe->elementId = beaconFilterTable[i].elementId;
        pIe->checkIePresence = beaconFilterTable[i].checkIePresence;
        pIe->byte.offset = beaconFilterTable[i].byte.offset;
        pIe->byte.value =  beaconFilterTable[i].byte.value;
        pIe->byte.bitMask =  beaconFilterTable[i].byte.bitMask;
        pIe->byte.ref =  beaconFilterTable[i].byte.ref; 
        ptr += sizeof(tBeaconFilterIe);
    }
    msgQ.type = WDA_BEACON_FILTER_IND;
    msgQ.reserved = 0;
    msgQ.bodyptr = pBeaconFilterMsg;
    msgQ.bodyval = 0;
    limLog( pMac, LOG3, FL( "Sending WDA_BEACON_FILTER_IND..." ));
    MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msgQ.type));
    if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
    {
        palFreeMemory(pMac->hHdd, pBeaconFilterMsg);
        limLog( pMac, LOGP,
            FL("Posting  WDA_BEACON_FILTER_IND to WDA failed, reason=%X\n"),
            retCode );
        return retCode;
    }
    return retCode;
}
