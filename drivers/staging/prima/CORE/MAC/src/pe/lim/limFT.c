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

#ifdef WLAN_FEATURE_VOWIFI_11R
/**=========================================================================
  
  \brief implementation for PE 11r VoWiFi FT Protocol 
  
   Copyright 2008 (c) Qualcomm, Incorporated.  All Rights Reserved.
   
   Qualcomm Confidential and Proprietary.
  
  ========================================================================*/

/* $Header$ */


/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include <limSendMessages.h>
#include <limTypes.h>
#include <limFT.h>
#include <limFTDefs.h>
#include <limUtils.h>
#include <limPropExtsUtils.h>
#include <limAssocUtils.h>
#include <limSession.h>
#include <limAdmitControl.h>
#include "wmmApsd.h"

#define LIM_FT_RIC_BA_SSN                       1
#define LIM_FT_RIC_BA_DIALOG_TOKEN_TID_0         248
#define LIM_FT_RIC_DESCRIPTOR_RESOURCE_TYPE_BA  1

/*--------------------------------------------------------------------------
  Initialize the FT variables. 
  ------------------------------------------------------------------------*/
void limFTOpen(tpAniSirGlobal pMac)
{
    pMac->ft.ftPEContext.pFTPreAuthReq = NULL;
    pMac->ft.ftPEContext.psavedsessionEntry = NULL;
}

/*--------------------------------------------------------------------------
  Cleanup FT variables. 
  ------------------------------------------------------------------------*/
void limFTCleanup(tpAniSirGlobal pMac)
{
    if (pMac->ft.ftPEContext.pFTPreAuthReq) 
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Freeing pFTPreAuthReq= %p\n", 
            __FUNCTION__, pMac->ft.ftPEContext.pFTPreAuthReq);) 
#endif
        vos_mem_free(pMac->ft.ftPEContext.pFTPreAuthReq);
        pMac->ft.ftPEContext.pFTPreAuthReq = NULL;
    }

    // This is the old session, should be deleted else where.
    // We should not be cleaning it here, just set it to NULL.
    if (pMac->ft.ftPEContext.psavedsessionEntry)
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Setting psavedsessionEntry= %p to NULL\n", 
            __FUNCTION__, pMac->ft.ftPEContext.psavedsessionEntry);) 
#endif
        pMac->ft.ftPEContext.psavedsessionEntry = NULL;
    }

    // This is the extra session we added as part of Auth resp
    // clean it up.
    if (pMac->ft.ftPEContext.pftSessionEntry)
    {
        if ((((tpPESession)(pMac->ft.ftPEContext.pftSessionEntry))->valid) &&
            (((tpPESession)(pMac->ft.ftPEContext.pftSessionEntry))->limSmeState == eLIM_SME_WT_REASSOC_STATE))
        {
            PELOGE(limLog( pMac, LOGE, "%s: Deleting Preauth Session %d\n", __func__, ((tpPESession)pMac->ft.ftPEContext.pftSessionEntry)->peSessionId);)
            peDeleteSession(pMac, pMac->ft.ftPEContext.pftSessionEntry);
        }
        pMac->ft.ftPEContext.pftSessionEntry = NULL;
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Setting psavedsessionEntry= %p to NULL\n", 
            __FUNCTION__, pMac->ft.ftPEContext.psavedsessionEntry);) 
#endif
    }

    if (pMac->ft.ftPEContext.pAddBssReq)
    {
        vos_mem_free(pMac->ft.ftPEContext.pAddBssReq);
        pMac->ft.ftPEContext.pAddBssReq = NULL;
    }

    if (pMac->ft.ftPEContext.pAddStaReq)
    {
        vos_mem_free(pMac->ft.ftPEContext.pAddStaReq);
        pMac->ft.ftPEContext.pAddStaReq = NULL;
    }

    pMac->ft.ftPEContext.ftPreAuthStatus = eSIR_SUCCESS; 

}

/*--------------------------------------------------------------------------
  Init FT variables. 
  ------------------------------------------------------------------------*/
void limFTInit(tpAniSirGlobal pMac)
{
    if (pMac->ft.ftPEContext.pFTPreAuthReq) 
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Freeing pFTPreAuthReq= %p\n", 
            __FUNCTION__, pMac->ft.ftPEContext.pFTPreAuthReq);) 
#endif
        vos_mem_free(pMac->ft.ftPEContext.pFTPreAuthReq);
        pMac->ft.ftPEContext.pFTPreAuthReq = NULL;
    }

    // This is the old session, should be deleted else where.
    // We should not be cleaning it here, just set it to NULL.
    if (pMac->ft.ftPEContext.psavedsessionEntry)
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Setting psavedsessionEntry= %p to NULL\n", 
            __FUNCTION__, pMac->ft.ftPEContext.psavedsessionEntry);) 
#endif
        pMac->ft.ftPEContext.psavedsessionEntry = NULL;
    }

    // This is the extra session we added as part of Auth resp
    // clean it up.
    if (pMac->ft.ftPEContext.pftSessionEntry)
    {
        /* Cannot delete sessions across associations */
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Deleting session = %p \n", 
            __FUNCTION__, pMac->ft.ftPEContext.pftSessionEntry);) 
#endif
        pMac->ft.ftPEContext.pftSessionEntry = NULL;
    }

    if (pMac->ft.ftPEContext.pAddBssReq)
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Freeing AddBssReq = %p \n", 
            __FUNCTION__, pMac->ft.ftPEContext.pAddBssReq);) 
#endif
        vos_mem_free(pMac->ft.ftPEContext.pAddBssReq);
        pMac->ft.ftPEContext.pAddBssReq = NULL;
    }


    if (pMac->ft.ftPEContext.pAddStaReq)
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: Freeing AddStaReq = %p \n", 
            __FUNCTION__, pMac->ft.ftPEContext.pAddStaReq);) 
#endif
        vos_mem_free(pMac->ft.ftPEContext.pAddStaReq);
        pMac->ft.ftPEContext.pAddStaReq = NULL;
    }

    pMac->ft.ftPEContext.ftPreAuthStatus = eSIR_SUCCESS; 

}

/*------------------------------------------------------------------
 * 
 * This is the handler after suspending the link.
 * We suspend the link and then now proceed to switch channel.
 *
 *------------------------------------------------------------------*/
void FTPreAuthSuspendLinkHandler(tpAniSirGlobal pMac, eHalStatus status, tANI_U32 *data)
{
    tpPESession psessionEntry;
    
    // The link is suspended of not ?
    if (status != eHAL_STATUS_SUCCESS) 
    {
        PELOGE(limLog( pMac, LOGE, "%s: Returning \n", __FUNCTION__);)
        // Post the FT Pre Auth Response to SME
        limPostFTPreAuthRsp(pMac, eSIR_FAILURE, NULL, 0, (tpPESession)data);

        return;
    }

    psessionEntry = (tpPESession)data;
    // Suspended, now move to a different channel.
    // Perform some sanity check before proceeding.
    if ((pMac->ft.ftPEContext.pFTPreAuthReq) && psessionEntry)
    {
        limChangeChannelWithCallback(pMac, 
            pMac->ft.ftPEContext.pFTPreAuthReq->preAuthchannelNum,
            limPerformFTPreAuth, NULL, psessionEntry);
        return;
    }

    // Else return error.
    limPostFTPreAuthRsp(pMac, eSIR_FAILURE, NULL, 0, psessionEntry);
}


/*--------------------------------------------------------------------------
  In this function, we process the FT Pre Auth Req.
  We receive Pre-Auth
  Suspend link
  Register a call back
  In the call back, we will need to accept frames from the new bssid
  Send out the auth req to new AP.
  Start timer and when the timer is done or if we receive the Auth response
  We change channel
  Resume link
  ------------------------------------------------------------------------*/
int limProcessFTPreAuthReq(tpAniSirGlobal pMac, tpSirMsgQ pMsg)
{
    int bufConsumed = FALSE;
    tpPESession psessionEntry;
    tANI_U8 sessionId;

    // Now we are starting fresh make sure all's cleanup.
    limFTInit(pMac);
    pMac->ft.ftPEContext.ftPreAuthStatus = eSIR_FAILURE;  // Can set it only after sending auth

    // We need information from the Pre-Auth Req. Lets save that
    pMac->ft.ftPEContext.pFTPreAuthReq = (tpSirFTPreAuthReq)pMsg->bodyptr;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOGE(limLog( pMac, LOGE, "%s: PE Auth ft_ies_length=%02x%02x%02x\n", __FUNCTION__,
        pMac->ft.ftPEContext.pFTPreAuthReq->ft_ies[0],
        pMac->ft.ftPEContext.pFTPreAuthReq->ft_ies[1],
        pMac->ft.ftPEContext.pFTPreAuthReq->ft_ies[2]);)
#endif

    // Get the current session entry
    psessionEntry = peFindSessionByBssid(pMac, 
        pMac->ft.ftPEContext.pFTPreAuthReq->currbssId, &sessionId);
    if (psessionEntry == NULL)
    {
        PELOGE(limLog( pMac, LOGE, "%s: Unable to find session for the following bssid\n", 
            __FUNCTION__);)
        limPrintMacAddr( pMac, pMac->ft.ftPEContext.pFTPreAuthReq->currbssId, LOGE );
        // Post the FT Pre Auth Response to SME
        limPostFTPreAuthRsp(pMac, eSIR_FAILURE, NULL, 0, NULL); 
        return TRUE;
    }

    // Dont need to suspend if APs are in same channel
    if (psessionEntry->currentOperChannel != pMac->ft.ftPEContext.pFTPreAuthReq->preAuthchannelNum) 
    {
        // Need to suspend link only if the channels are different
        PELOG2(limLog(pMac,LOG2,"%s: Performing pre-auth on different"
               " channel (session %p)\n", __FUNCTION__, psessionEntry);)
        limSuspendLink(pMac, eSIR_CHECK_ROAMING_SCAN, FTPreAuthSuspendLinkHandler, 
                       (tANI_U32 *)psessionEntry); 
    }
    else 
    {
        PELOG2(limLog(pMac,LOG2,"%s: Performing pre-auth on same"
               " channel (session %p)\n", __FUNCTION__, psessionEntry);)
        // We are in the same channel. Perform pre-auth
        limPerformFTPreAuth(pMac, eHAL_STATUS_SUCCESS, NULL, psessionEntry);
    }

    return bufConsumed;
}

/*------------------------------------------------------------------
 * Send the Auth1 
 * Receive back Auth2
 *------------------------------------------------------------------*/
void limPerformFTPreAuth(tpAniSirGlobal pMac, eHalStatus status, tANI_U32 *data, 
    tpPESession psessionEntry)
{
    tSirMacAuthFrameBody authFrame;

    if (psessionEntry->is11Rconnection)
    {
        // Only 11r assoc has FT IEs.
        if (pMac->ft.ftPEContext.pFTPreAuthReq->ft_ies == NULL) 
        {
            PELOGE(limLog( pMac, LOGE, "%s: FTIEs for Auth Req Seq 1 is absent\n");)
            return;
        }
    }
    if (status != eHAL_STATUS_SUCCESS) 
    {
        PELOGE(limLog( pMac, LOGE, "%s: Change channel not successful for FT pre-auth\n");)
        return;
    }
    pMac->ft.ftPEContext.psavedsessionEntry = psessionEntry;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOG2(limLog(pMac,LOG2,"Entered wait auth2 state for FT"
           " (old session %p)\n", 
           pMac->ft.ftPEContext.psavedsessionEntry);)
#endif


    if (psessionEntry->is11Rconnection)
    {
        // Now we are on the right channel and need to send out Auth1 and 
        // receive Auth2.
        authFrame.authAlgoNumber = eSIR_FT_AUTH; // Set the auth type to FT
    }
#if defined FEATURE_WLAN_CCX || defined FEATURE_WLAN_LFR
    else
    {
        // Will need to make isCCXconnection a enum may be for further
        // improvements to this to match this algorithm number
        authFrame.authAlgoNumber = eSIR_OPEN_SYSTEM; // For now if its CCX and 11r FT. 
    }
#endif
    authFrame.authTransactionSeqNumber = SIR_MAC_AUTH_FRAME_1;
    authFrame.authStatusCode = 0;

    // Start timer here to come back to operating channel.
    pMac->lim.limTimers.gLimFTPreAuthRspTimer.sessionId = psessionEntry->peSessionId;
    if(TX_SUCCESS !=  tx_timer_activate(&pMac->lim.limTimers.gLimFTPreAuthRspTimer))
    {
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "%s: FT Auth Rsp Timer Start Failed\n", __FUNCTION__);)
#endif
    }

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOGE(limLog( pMac, LOGE, "%s: FT Auth Rsp Timer Started\n", __FUNCTION__);)
#endif

    limSendAuthMgmtFrame(pMac, &authFrame,
        pMac->ft.ftPEContext.pFTPreAuthReq->preAuthbssId,
        LIM_NO_WEP_IN_FC, psessionEntry);

    return;
}


/*------------------------------------------------------------------
 *
 * Create the new Add Bss Req to the new AP.
 * This will be used when we are ready to FT to the new AP.
 * The newly created ft Session entry is passed to this function
 *
 *------------------------------------------------------------------*/
tSirRetStatus limFTPrepareAddBssReq( tpAniSirGlobal pMac, 
    tANI_U8 updateEntry, tpPESession pftSessionEntry, 
    tpSirBssDescription bssDescription )
{
    tpAddBssParams pAddBssParams = NULL;
    tANI_U8 i;
    tANI_U8 chanWidthSupp = 0;
    tSchBeaconStruct *pBeaconStruct;

    if(eHAL_STATUS_SUCCESS != palAllocateMemory(pMac->hHdd, 
                                                (void **)&pBeaconStruct, sizeof(tSchBeaconStruct)))
    {
        limLog(pMac, LOGE, FL("Unable to PAL allocate memory for creating ADD_BSS\n") );
        return eSIR_MEM_ALLOC_FAILED;
    }

    // Package SIR_HAL_ADD_BSS_REQ message parameters
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
        (void **) &pAddBssParams, sizeof( tAddBssParams )))
    {
        palFreeMemory(pMac->hHdd, pBeaconStruct);
        limLog( pMac, LOGP,
                FL( "Unable to PAL allocate memory for creating ADD_BSS\n" ));
        return (eSIR_MEM_ALLOC_FAILED);
    }
    
    palZeroMemory( pMac->hHdd, (tANI_U8 *) pAddBssParams, sizeof( tAddBssParams ));


    limExtractApCapabilities( pMac,
        (tANI_U8 *) bssDescription->ieFields,
        limGetIElenFromBssDescription( bssDescription ), pBeaconStruct );

    if (pMac->lim.gLimProtectionControl != WNI_CFG_FORCE_POLICY_PROTECTION_DISABLE)
        limDecideStaProtectionOnAssoc(pMac, pBeaconStruct, pftSessionEntry);

    palCopyMemory( pMac->hHdd, pAddBssParams->bssId, bssDescription->bssId,
        sizeof( tSirMacAddr ));

    // Fill in tAddBssParams selfMacAddr
    palCopyMemory( pMac->hHdd,  pAddBssParams->selfMacAddr, pftSessionEntry->selfMacAddr,
        sizeof( tSirMacAddr ));

    pAddBssParams->bssType = pftSessionEntry->bssType;//eSIR_INFRASTRUCTURE_MODE;
    pAddBssParams->operMode = BSS_OPERATIONAL_MODE_STA;

    pAddBssParams->beaconInterval = bssDescription->beaconInterval;
    
    pAddBssParams->dtimPeriod = pBeaconStruct->tim.dtimPeriod;
    pAddBssParams->updateBss = updateEntry;


    pAddBssParams->cfParamSet.cfpCount = pBeaconStruct->cfParamSet.cfpCount;
    pAddBssParams->cfParamSet.cfpPeriod = pBeaconStruct->cfParamSet.cfpPeriod;
    pAddBssParams->cfParamSet.cfpMaxDuration = pBeaconStruct->cfParamSet.cfpMaxDuration;
    pAddBssParams->cfParamSet.cfpDurRemaining = pBeaconStruct->cfParamSet.cfpDurRemaining;


    pAddBssParams->rateSet.numRates = pBeaconStruct->supportedRates.numRates;
    palCopyMemory( pMac->hHdd,  pAddBssParams->rateSet.rate,
                   pBeaconStruct->supportedRates.rate, pBeaconStruct->supportedRates.numRates );

    pAddBssParams->nwType = bssDescription->nwType;
    
    pAddBssParams->shortSlotTimeSupported = (tANI_U8)pBeaconStruct->capabilityInfo.shortSlotTime; 
    pAddBssParams->llaCoexist = (tANI_U8) pftSessionEntry->beaconParams.llaCoexist;
    pAddBssParams->llbCoexist = (tANI_U8) pftSessionEntry->beaconParams.llbCoexist;
    pAddBssParams->llgCoexist = (tANI_U8) pftSessionEntry->beaconParams.llgCoexist;
    pAddBssParams->ht20Coexist = (tANI_U8) pftSessionEntry->beaconParams.ht20Coexist;

    // Use the advertised capabilities from the received beacon/PR
    if (IS_DOT11_MODE_HT(pftSessionEntry->dot11mode) && ( pBeaconStruct->HTCaps.present ))
    {
        pAddBssParams->htCapable = pBeaconStruct->HTCaps.present;

        if ( pBeaconStruct->HTInfo.present )
        {
            pAddBssParams->htOperMode = (tSirMacHTOperatingMode)pBeaconStruct->HTInfo.opMode;
            pAddBssParams->dualCTSProtection = ( tANI_U8 ) pBeaconStruct->HTInfo.dualCTSProtection;

#ifdef WLAN_SOFTAP_FEATURE
            chanWidthSupp = limGetHTCapability( pMac, eHT_SUPPORTED_CHANNEL_WIDTH_SET, pftSessionEntry);
#else 
            chanWidthSupp = limGetHTCapability( pMac, eHT_SUPPORTED_CHANNEL_WIDTH_SET);
#endif
            if( (pBeaconStruct->HTCaps.supportedChannelWidthSet) &&
                (chanWidthSupp) )
            {
                pAddBssParams->txChannelWidthSet = ( tANI_U8 ) pBeaconStruct->HTInfo.recommendedTxWidthSet;
                pAddBssParams->currentExtChannel = pBeaconStruct->HTInfo.secondaryChannelOffset;
            }
            else
            {
                pAddBssParams->txChannelWidthSet = WNI_CFG_CHANNEL_BONDING_MODE_DISABLE;
                pAddBssParams->currentExtChannel = PHY_SINGLE_CHANNEL_CENTERED;
            }
            pAddBssParams->llnNonGFCoexist = (tANI_U8)pBeaconStruct->HTInfo.nonGFDevicesPresent;
            pAddBssParams->fLsigTXOPProtectionFullSupport = (tANI_U8)pBeaconStruct->HTInfo.lsigTXOPProtectionFullSupport;
            pAddBssParams->fRIFSMode = pBeaconStruct->HTInfo.rifsMode;
        }
    }

    pAddBssParams->currentOperChannel = bssDescription->channelId;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    limLog( pMac, LOGE, FL( "SIR_HAL_ADD_BSS_REQ with channel = %d..." ), 
        pAddBssParams->currentOperChannel);
#endif


    // Populate the STA-related parameters here
    // Note that the STA here refers to the AP
    {
        pAddBssParams->staContext.staType = STA_ENTRY_OTHER; // Identifying AP as an STA

        palCopyMemory( pMac->hHdd,  pAddBssParams->staContext.bssId,
                       bssDescription->bssId,
                       sizeof( tSirMacAddr ));
        pAddBssParams->staContext.listenInterval = bssDescription->beaconInterval;

        pAddBssParams->staContext.assocId = 0; // Is SMAC OK with this?
        pAddBssParams->staContext.uAPSD = 0;
        pAddBssParams->staContext.maxSPLen = 0;
        pAddBssParams->staContext.shortPreambleSupported = (tANI_U8)pBeaconStruct->capabilityInfo.shortPreamble;
        pAddBssParams->staContext.updateSta = updateEntry;
        pAddBssParams->staContext.encryptType = pftSessionEntry->encryptType;

        if (IS_DOT11_MODE_HT(pftSessionEntry->dot11mode) && ( pBeaconStruct->HTCaps.present ))
        {
            pAddBssParams->staContext.us32MaxAmpduDuration = 0;
            pAddBssParams->staContext.htCapable = 1;
            pAddBssParams->staContext.greenFieldCapable  = ( tANI_U8 ) pBeaconStruct->HTCaps.greenField;
            pAddBssParams->staContext.lsigTxopProtection = ( tANI_U8 ) pBeaconStruct->HTCaps.lsigTXOPProtection;
            if( (pBeaconStruct->HTCaps.supportedChannelWidthSet) &&
                (chanWidthSupp) )
            {
                pAddBssParams->staContext.txChannelWidthSet = ( tANI_U8 )pBeaconStruct->HTInfo.recommendedTxWidthSet;
            }
            else
            {
                pAddBssParams->staContext.txChannelWidthSet = WNI_CFG_CHANNEL_BONDING_MODE_DISABLE;
            }                                                           
            pAddBssParams->staContext.mimoPS             = (tSirMacHTMIMOPowerSaveState)pBeaconStruct->HTCaps.mimoPowerSave;
            pAddBssParams->staContext.delBASupport       = ( tANI_U8 ) pBeaconStruct->HTCaps.delayedBA;
            pAddBssParams->staContext.maxAmsduSize       = ( tANI_U8 ) pBeaconStruct->HTCaps.maximalAMSDUsize;
            pAddBssParams->staContext.maxAmpduDensity    =             pBeaconStruct->HTCaps.mpduDensity;
            pAddBssParams->staContext.fDsssCckMode40Mhz = (tANI_U8)pBeaconStruct->HTCaps.dsssCckMode40MHz;
            pAddBssParams->staContext.fShortGI20Mhz = (tANI_U8)pBeaconStruct->HTCaps.shortGI20MHz;
            pAddBssParams->staContext.fShortGI40Mhz = (tANI_U8)pBeaconStruct->HTCaps.shortGI40MHz;
            pAddBssParams->staContext.maxAmpduSize= pBeaconStruct->HTCaps.maxRxAMPDUFactor;
            
            if( pBeaconStruct->HTInfo.present )
                pAddBssParams->staContext.rifsMode = pBeaconStruct->HTInfo.rifsMode;
        }

        if ((pftSessionEntry->limWmeEnabled && pBeaconStruct->wmeEdcaPresent) ||
                (pftSessionEntry->limQosEnabled && pBeaconStruct->edcaPresent))
            pAddBssParams->staContext.wmmEnabled = 1;
        else 
            pAddBssParams->staContext.wmmEnabled = 0;

        //Update the rates
#ifdef WLAN_FEATURE_11AC
        limPopulateOwnRateSet(pMac, &pAddBssParams->staContext.supportedRates, 
                             pBeaconStruct->HTCaps.supportedMCSSet, 
                             false,pftSessionEntry,&pBeaconStruct->VHTCaps);
#else
        limPopulateOwnRateSet(pMac, &pAddBssParams->staContext.supportedRates, 
                                                    beaconStruct.HTCaps.supportedMCSSet, false,pftSessionEntry);
#endif
        limFillSupportedRatesInfo(pMac, NULL, &pAddBssParams->staContext.supportedRates,pftSessionEntry);

    }


    //Disable BA. It will be set as part of ADDBA negotiation.
    for( i = 0; i < STACFG_MAX_TC; i++ )
    {
        pAddBssParams->staContext.staTCParams[i].txUseBA    = eBA_DISABLE;
        pAddBssParams->staContext.staTCParams[i].rxUseBA    = eBA_DISABLE;
        pAddBssParams->staContext.staTCParams[i].txBApolicy = eBA_POLICY_IMMEDIATE;
        pAddBssParams->staContext.staTCParams[i].rxBApolicy = eBA_POLICY_IMMEDIATE;
    }

#if defined WLAN_FEATURE_VOWIFI  
    pAddBssParams->maxTxPower = pftSessionEntry->maxTxPower;
#endif

    pAddBssParams->status = eHAL_STATUS_SUCCESS;
    pAddBssParams->respReqd = true;

    pAddBssParams->staContext.sessionId = pftSessionEntry->peSessionId;
    pAddBssParams->sessionId = pftSessionEntry->peSessionId;
    
    // Set a new state for MLME

    pftSessionEntry->limMlmState = eLIM_MLM_WT_ADD_BSS_RSP_FT_REASSOC_STATE;

    pAddBssParams->halPersona=(tANI_U8)pftSessionEntry->pePersona; //pass on the session persona to hal
    
    pMac->ft.ftPEContext.pAddBssReq = pAddBssParams;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    limLog( pMac, LOGE, FL( "Saving SIR_HAL_ADD_BSS_REQ for pre-auth ap..." ));
#endif

    palFreeMemory(pMac->hHdd, pBeaconStruct);
    return 0;
}

/*------------------------------------------------------------------
 *
 * Setup the new session for the pre-auth AP. 
 * Return the newly created session entry.
 *
 *------------------------------------------------------------------*/
tpPESession limFillFTSession(tpAniSirGlobal pMac,
    tpSirBssDescription  pbssDescription, tpPESession psessionEntry)
{
    tpPESession      pftSessionEntry;
    tANI_U8          currentBssUapsd;
    tANI_U8          sessionId;
    tPowerdBm        localPowerConstraint;
    tPowerdBm        regMax;
    tSchBeaconStruct *pBeaconStruct;

    if(eHAL_STATUS_SUCCESS != palAllocateMemory(pMac->hHdd, 
                                                (void **)&pBeaconStruct, sizeof(tSchBeaconStruct)))
    {
        limLog(pMac, LOGE, FL("Unable to PAL allocate memory for creating limFillFTSession\n") );
        return NULL;
    }


    if((pftSessionEntry = peCreateSession(pMac, pbssDescription->bssId,
        &sessionId, pMac->lim.maxStation)) == NULL)
    {
        limLog(pMac, LOGE, FL("Session Can not be created for pre-auth 11R AP\n"));
        palFreeMemory(pMac->hHdd, pBeaconStruct);
        return NULL;
    }
        
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG || defined FEATURE_WLAN_CCX || defined(FEATURE_WLAN_LFR)
    limPrintMacAddr(pMac, pbssDescription->bssId, LOGE);
#endif

    /* Store PE session Id in session Table */
    pftSessionEntry->peSessionId = sessionId;

    pftSessionEntry->dot11mode = psessionEntry->dot11mode;
    pftSessionEntry->htCapability = psessionEntry->htCapability;

    pftSessionEntry->limWmeEnabled = psessionEntry->limWmeEnabled;
    pftSessionEntry->limQosEnabled = psessionEntry->limQosEnabled;
    pftSessionEntry->limWsmEnabled = psessionEntry->limWsmEnabled;
    pftSessionEntry->lim11hEnable = psessionEntry->lim11hEnable;

    // Fields to be filled later
    pftSessionEntry->pLimJoinReq = NULL; 
    pftSessionEntry->smeSessionId = 0; 
    pftSessionEntry->transactionId = 0; 

    limExtractApCapabilities( pMac,
                            (tANI_U8 *) pbssDescription->ieFields,
                            limGetIElenFromBssDescription( pbssDescription ),
                            pBeaconStruct );

    pftSessionEntry->rateSet.numRates = pBeaconStruct->supportedRates.numRates;
    palCopyMemory( pMac->hHdd,  pftSessionEntry->rateSet.rate,
        pBeaconStruct->supportedRates.rate, pBeaconStruct->supportedRates.numRates );

    pftSessionEntry->extRateSet.numRates = pBeaconStruct->extendedRates.numRates;
    palCopyMemory(pMac->hHdd, pftSessionEntry->extRateSet.rate, 
        pBeaconStruct->extendedRates.rate, pftSessionEntry->extRateSet.numRates);


    pftSessionEntry->ssId.length = pBeaconStruct->ssId.length;
    palCopyMemory( pMac->hHdd, pftSessionEntry->ssId.ssId, pBeaconStruct->ssId.ssId,
        pftSessionEntry->ssId.length);


    // Self Mac
    sirCopyMacAddr(pftSessionEntry->selfMacAddr, psessionEntry->selfMacAddr);
    sirCopyMacAddr(pftSessionEntry->limReAssocbssId, pbssDescription->bssId);
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG || defined FEATURE_WLAN_CCX || defined(FEATURE_WLAN_LFR)
    limPrintMacAddr(pMac, pftSessionEntry->limReAssocbssId, LOGE);
#endif

    /* Store beaconInterval */
    pftSessionEntry->beaconParams.beaconInterval = pbssDescription->beaconInterval;
    pftSessionEntry->bssType = psessionEntry->bssType;

    pftSessionEntry->statypeForBss = STA_ENTRY_PEER;
    pftSessionEntry->nwType = pbssDescription->nwType;

    /* Copy The channel Id to the session Table */
    pftSessionEntry->limReassocChannelId = pbssDescription->channelId;
    pftSessionEntry->currentOperChannel = pbssDescription->channelId;
            
            
    if (pftSessionEntry->bssType == eSIR_INFRASTRUCTURE_MODE)
    {
        pftSessionEntry->limSystemRole = eLIM_STA_ROLE;
    }
    else if(pftSessionEntry->bssType == eSIR_BTAMP_AP_MODE)
    {
        pftSessionEntry->limSystemRole = eLIM_BT_AMP_STA_ROLE;
    }
    else
    {   
        /* Throw an error and return and make sure to delete the session.*/
        limLog(pMac, LOGE, FL("Invalid bss type\n"));
    }    
                       
    pftSessionEntry->limCurrentBssCaps = pbssDescription->capabilityInfo;
    pftSessionEntry->limReassocBssCaps = pbssDescription->capabilityInfo;

    regMax = cfgGetRegulatoryMaxTransmitPower( pMac, pftSessionEntry->currentOperChannel ); 
    localPowerConstraint = regMax;
    limExtractApCapability( pMac, (tANI_U8 *) pbssDescription->ieFields, 
        limGetIElenFromBssDescription(pbssDescription),
        &pftSessionEntry->limCurrentBssQosCaps,
        &pftSessionEntry->limCurrentBssPropCap,
        &currentBssUapsd , &localPowerConstraint);

    pftSessionEntry->limReassocBssQosCaps =
        pftSessionEntry->limCurrentBssQosCaps;
    pftSessionEntry->limReassocBssPropCap =
        pftSessionEntry->limCurrentBssPropCap;


    pftSessionEntry->maxTxPower = VOS_MIN( regMax , (localPowerConstraint) );

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    limLog( pMac, LOGE, "%s: Regulatory max = %d, local power constraint = %d, max tx = %d", 
        __FUNCTION__, regMax, localPowerConstraint, pftSessionEntry->maxTxPower );
#endif

    pftSessionEntry->limRFBand = limGetRFBand(pftSessionEntry->currentOperChannel);

    pftSessionEntry->limPrevSmeState = pftSessionEntry->limSmeState;
    pftSessionEntry->limSmeState = eLIM_SME_WT_REASSOC_STATE;

    pftSessionEntry->encryptType = psessionEntry->encryptType;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOGE(limLog(pMac,LOGE,"%s:created session (%p) with id = %d\n", 
       __FUNCTION__, pftSessionEntry, pftSessionEntry->peSessionId);)
#endif

    palFreeMemory(pMac->hHdd, pBeaconStruct);
    return pftSessionEntry;
}

/*------------------------------------------------------------------
 *
 * Setup the session and the add bss req for the pre-auth AP. 
 *
 *------------------------------------------------------------------*/
void limFTSetupAuthSession(tpAniSirGlobal pMac, tpPESession psessionEntry)
{
    tpPESession pftSessionEntry;

    // Prepare the session right now with as much as possible.
    pftSessionEntry = limFillFTSession(pMac, pMac->ft.ftPEContext.pFTPreAuthReq->pbssDescription, psessionEntry);

    if (pftSessionEntry)
    {
        pftSessionEntry->is11Rconnection = psessionEntry->is11Rconnection;
#ifdef FEATURE_WLAN_CCX
        pftSessionEntry->isCCXconnection = psessionEntry->isCCXconnection;
#endif
#if defined WLAN_FEATURE_VOWIFI_11R || defined FEATURE_WLAN_CCX || defined(FEATURE_WLAN_LFR)
        pftSessionEntry->isFastTransitionEnabled = psessionEntry->isFastTransitionEnabled;
#endif

#ifdef FEATURE_WLAN_LFR
        pftSessionEntry->isFastRoamIniFeatureEnabled = psessionEntry->isFastRoamIniFeatureEnabled; 
#endif
        limFTPrepareAddBssReq( pMac, FALSE, pftSessionEntry, 
            pMac->ft.ftPEContext.pFTPreAuthReq->pbssDescription );
        pMac->ft.ftPEContext.pftSessionEntry = pftSessionEntry;
    }
}

/*------------------------------------------------------------------
 * Resume Link Call Back 
 *------------------------------------------------------------------*/
void limFTProcessPreAuthResult(tpAniSirGlobal pMac, eHalStatus status, tANI_U32 *data)
{
    tpPESession psessionEntry;

    psessionEntry = (tpPESession)data;

    if (pMac->ft.ftPEContext.ftPreAuthStatus == eSIR_SUCCESS)
    {
        limFTSetupAuthSession(pMac, psessionEntry);
    }

    // Post the FT Pre Auth Response to SME
    limPostFTPreAuthRsp(pMac, pMac->ft.ftPEContext.ftPreAuthStatus,
        pMac->ft.ftPEContext.saved_auth_rsp,
        pMac->ft.ftPEContext.saved_auth_rsp_length, psessionEntry);

}

/*------------------------------------------------------------------
 * Resume Link Call Back 
 *------------------------------------------------------------------*/
void limPerformPostFTPreAuthAndChannelChange(tpAniSirGlobal pMac, eHalStatus status, tANI_U32 *data, 
    tpPESession psessionEntry)
{
    //Set the resume channel to Any valid channel (invalid). 
    //This will instruct HAL to set it to any previous valid channel.
    peSetResumeChannel(pMac, 0, 0);
    limResumeLink(pMac, limFTProcessPreAuthResult, (tANI_U32 *)psessionEntry);
}

tSirRetStatus limCreateRICBlockAckIE(tpAniSirGlobal pMac, tANI_U8 tid, tCfgTrafficClass *pTrafficClass, 
                                                                    tANI_U8 *ric_ies, tANI_U32 *ieLength)
{
    tDot11fIERICDataDesc ricIe;
    tDot11fFfBAStartingSequenceControl baSsnControl;
    tDot11fFfAddBAParameterSet baParamSet;
    tDot11fFfBATimeout  baTimeout;

    vos_mem_zero(&ricIe, sizeof(tDot11fIERICDataDesc));
    vos_mem_zero(&baSsnControl, sizeof(tDot11fFfBAStartingSequenceControl));
    vos_mem_zero(&baParamSet, sizeof(tDot11fFfAddBAParameterSet));
    vos_mem_zero(&baTimeout, sizeof(tDot11fFfBATimeout));

    ricIe.present = 1;
    ricIe.RICData.present = 1;
    ricIe.RICData.resourceDescCount = 1;
    ricIe.RICData.Identifier = LIM_FT_RIC_BA_DIALOG_TOKEN_TID_0 + tid;
    ricIe.RICDescriptor.present = 1;
    ricIe.RICDescriptor.resourceType = LIM_FT_RIC_DESCRIPTOR_RESOURCE_TYPE_BA;
    baParamSet.tid = tid;
    baParamSet.policy = pTrafficClass->fTxBApolicy;  // Immediate Block Ack
    baParamSet.bufferSize = pTrafficClass->txBufSize;
    vos_mem_copy((v_VOID_t *)&baTimeout, (v_VOID_t *)&pTrafficClass->tuTxBAWaitTimeout, sizeof(baTimeout));
    baSsnControl.fragNumber = 0;
    baSsnControl.ssn = LIM_FT_RIC_BA_SSN;

    dot11fPackFfAddBAParameterSet(pMac, &baParamSet, &ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData]);
    //vos_mem_copy(&ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData], &baParamSet, sizeof(tDot11fFfAddBAParameterSet));
    ricIe.RICDescriptor.num_variableData += sizeof(tDot11fFfAddBAParameterSet);

    dot11fPackFfBATimeout(pMac, &baTimeout, &ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData]);
    //vos_mem_copy(&ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData], &baTimeout, sizeof(tDot11fFfBATimeout));
    ricIe.RICDescriptor.num_variableData += sizeof(tDot11fFfBATimeout);

    dot11fPackFfBAStartingSequenceControl(pMac, &baSsnControl, &ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData]);
    //vos_mem_copy(&ricIe.RICDescriptor.variableData[ricIe.RICDescriptor.num_variableData], &baSsnControl, sizeof(tDot11fFfBAStartingSequenceControl));
    ricIe.RICDescriptor.num_variableData += sizeof(tDot11fFfBAStartingSequenceControl);

    return (tSirRetStatus) dot11fPackIeRICDataDesc(pMac, &ricIe, ric_ies, sizeof(tDot11fIERICDataDesc), ieLength);
}

tSirRetStatus limFTFillRICBlockAckInfo(tpAniSirGlobal pMac, tANI_U8 *ric_ies, tANI_U32 *ric_ies_length)
{
    tANI_U8 tid = 0;
    tpDphHashNode pSta;
    tANI_U16 numBA = 0, aid = 0;
    tpPESession psessionEntry = pMac->ft.ftPEContext.psavedsessionEntry;
    tANI_U32 offset = 0, ieLength = 0;
    tSirRetStatus status = eSIR_SUCCESS;
    
    // First, extract the DPH entry
    pSta = dphLookupHashEntry( pMac, pMac->ft.ftPEContext.pFTPreAuthReq->currbssId, &aid, &psessionEntry->dph.dphHashTable);
    if( NULL == pSta )
    {
        PELOGE(limLog( pMac, LOGE,
            FL( "STA context not found for saved session's BSSID %02x:%02x:%02x:%02x:%02x:%02x\n" ),
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[0], 
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[1], 
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[2], 
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[3], 
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[4], 
            pMac->ft.ftPEContext.pFTPreAuthReq->currbssId[5] );)
        return eSIR_FAILURE;
    }

    for (tid = 0; tid < STACFG_MAX_TC; tid++)
    {
        if (pSta->tcCfg[tid].fUseBATx)
        {
            status = limCreateRICBlockAckIE(pMac, tid, &pSta->tcCfg[tid], ric_ies + offset, &ieLength);
            if (eSIR_SUCCESS == status)
            {
                offset += ieLength;
                *ric_ies_length += ieLength;
                numBA++;
            }
            else
            {
                PELOGE(limLog(pMac, LOGE, FL("BA RIC IE creation for TID %d failed with status %d"), tid, status);)
            }
        }
    }

    PELOGE(limLog(pMac, LOGE, FL("Number of BA RIC IEs created = %d: Total length = %d\n"), numBA, *ric_ies_length);)
    return status;
}

/*------------------------------------------------------------------
 *
 *  Will post pre auth response to SME.
 *
 *------------------------------------------------------------------*/
void limPostFTPreAuthRsp(tpAniSirGlobal pMac, eHalStatus status,
    tANI_U8 *auth_rsp, tANI_U16  auth_rsp_length,
    tpPESession psessionEntry)
{
    tpSirFTPreAuthRsp pFTPreAuthRsp;
    tSirMsgQ          mmhMsg;
    tANI_U16 rspLen = sizeof(tSirFTPreAuthRsp);   
    tSirRetStatus   sirStatus = eSIR_SUCCESS;

    pFTPreAuthRsp = (tpSirFTPreAuthRsp)vos_mem_malloc(rspLen);
    if(NULL == pFTPreAuthRsp)
    {
       PELOGE(limLog( pMac, LOGE, "Failed to allocate memory\n");)
       VOS_ASSERT(pFTPreAuthRsp != NULL);
       return;
    }
    vos_mem_zero( pFTPreAuthRsp, rspLen);
#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOGE(limLog( pMac, LOGE, "%s: Auth Rsp = %p\n", pFTPreAuthRsp);)
#endif
         
    palZeroMemory(pMac, (tANI_U8*)pFTPreAuthRsp, rspLen);
    pFTPreAuthRsp->messageType = eWNI_SME_FT_PRE_AUTH_RSP;
    pFTPreAuthRsp->length = (tANI_U16) rspLen;
    pFTPreAuthRsp->status = status;
    if (psessionEntry)
        pFTPreAuthRsp->smeSessionId = psessionEntry->smeSessionId;

    // The bssid of the AP we are sending Auth1 to.
    if (pMac->ft.ftPEContext.pFTPreAuthReq)
        sirCopyMacAddr(pFTPreAuthRsp->preAuthbssId, 
            pMac->ft.ftPEContext.pFTPreAuthReq->preAuthbssId);
    
    // Attach the auth response now back to SME
    pFTPreAuthRsp->ft_ies_length = 0;
    if ((auth_rsp != NULL) && (auth_rsp_length < MAX_FTIE_SIZE))
    {
        // Only 11r assoc has FT IEs.
        vos_mem_copy(pFTPreAuthRsp->ft_ies, auth_rsp, auth_rsp_length); 
        pFTPreAuthRsp->ft_ies_length = auth_rsp_length;
    }
    
#ifdef WLAN_FEATURE_VOWIFI_11R
    if ((psessionEntry) && (psessionEntry->is11Rconnection))
    {
        /* Fill in the Block Ack RIC IEs in the preAuthRsp */
        sirStatus = limFTFillRICBlockAckInfo(pMac, pFTPreAuthRsp->ric_ies, 
                                         (tANI_U32 *)&pFTPreAuthRsp->ric_ies_length);
        if (eSIR_SUCCESS != sirStatus)
        {
            PELOGE(limLog(pMac, LOGE, FL("Fill RIC BA Info failed with status %d"), sirStatus);)
        }
    }
#endif
    
    mmhMsg.type = pFTPreAuthRsp->messageType;
    mmhMsg.bodyptr = pFTPreAuthRsp;
    mmhMsg.bodyval = 0;

#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    PELOGE(limLog( pMac, LOGE, "Posted Auth Rsp to SME\n");)
#endif
    PELOGE(limLog( pMac, LOGE, "Posted Auth Rsp to SME with status of %d\n", status);)
    limSysProcessMmhMsgApi(pMac, &mmhMsg,  ePROT);
}

/*------------------------------------------------------------------
 *
 * Send the FT Pre Auth Response to SME when ever we have a status
 * ready to be sent to SME
 *
 * SME will be the one to send it up to the supplicant to receive 
 * FTIEs which will be required for Reassoc Req.
 *
 *------------------------------------------------------------------*/
void limHandleFTPreAuthRsp(tpAniSirGlobal pMac, eHalStatus status,
    tANI_U8 *auth_rsp, tANI_U16  auth_rsp_length,
    tpPESession psessionEntry)
{

    // Save the status of pre-auth
    pMac->ft.ftPEContext.ftPreAuthStatus = status; 

    // Save the auth rsp, so we can send it to 
    // SME once we resume link. 
    pMac->ft.ftPEContext.saved_auth_rsp_length = 0; 
    if ((auth_rsp != NULL) && (auth_rsp_length < MAX_FTIE_SIZE))
    {
        vos_mem_copy(pMac->ft.ftPEContext.saved_auth_rsp,
            auth_rsp, auth_rsp_length); 
        pMac->ft.ftPEContext.saved_auth_rsp_length = auth_rsp_length;
    }

    if (psessionEntry->currentOperChannel != 
        pMac->ft.ftPEContext.pFTPreAuthReq->preAuthchannelNum) 
    {
        // Need to move to the original AP channel
        limChangeChannelWithCallback(pMac, psessionEntry->currentOperChannel, 
                limPerformPostFTPreAuthAndChannelChange, NULL, psessionEntry);
    }
    else 
    {
#ifdef WLAN_FEATURE_VOWIFI_11R_DEBUG
        PELOGE(limLog( pMac, LOGE, "Pre auth on same channel as connected AP channel %d\n", 
            pMac->ft.ftPEContext.pFTPreAuthReq->preAuthchannelNum);)
#endif
        limFTProcessPreAuthResult(pMac, status, (tANI_U32 *)psessionEntry);
    }
}

/*------------------------------------------------------------------
 *
 *  This function handles the 11R Reassoc Req from SME
 *
 *------------------------------------------------------------------*/
void limProcessMlmFTReassocReq(tpAniSirGlobal pMac, tANI_U32 *pMsgBuf,
    tpPESession psessionEntry)
{
    tANI_U8 smeSessionId = 0;
    tANI_U16 transactionId = 0;
    tANI_U8 chanNum = 0; 
    tLimMlmReassocReq  *pMlmReassocReq;
    tANI_U16 caps;
    tANI_U32 val;
    tSirMsgQ msgQ;
    tSirRetStatus retCode;
    tANI_U32 teleBcnEn = 0;

    chanNum = psessionEntry->currentOperChannel; 
    limGetSessionInfo(pMac,(tANI_U8*)pMsgBuf, &smeSessionId, &transactionId);
    psessionEntry->smeSessionId = smeSessionId;
    psessionEntry->transactionId = transactionId;



    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&pMlmReassocReq, 
        sizeof(tLimMlmReassocReq)))
    {
        // Log error
        limLog(pMac, LOGE, FL("call to palAllocateMemory failed for mlmReassocReq\n"));
        return;
    }

    palCopyMemory( pMac->hHdd, pMlmReassocReq->peerMacAddr,
                  psessionEntry->bssId,
                  sizeof(tSirMacAddr));

    if (wlan_cfgGetInt(pMac, WNI_CFG_REASSOCIATION_FAILURE_TIMEOUT,
                  (tANI_U32 *) &pMlmReassocReq->reassocFailureTimeout)
                           != eSIR_SUCCESS)
    {
        /**
         * Could not get ReassocFailureTimeout value
         * from CFG. Log error.
         */
        limLog(pMac, LOGE, FL("could not retrieve ReassocFailureTimeout value\n"));
        return;
    }

    if (cfgGetCapabilityInfo(pMac, &caps,psessionEntry) != eSIR_SUCCESS)
    {
        /**
         * Could not get Capabilities value
         * from CFG. Log error.
         */
        limLog(pMac, LOGE, FL("could not retrieve Capabilities value\n"));
        return;
    }
    pMlmReassocReq->capabilityInfo = caps;
    
    /* Update PE sessionId*/
    pMlmReassocReq->sessionId = psessionEntry->peSessionId;

    /* If telescopic beaconing is enabled, set listen interval to WNI_CFG_TELE_BCN_MAX_LI */
    if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_WAKEUP_EN, &teleBcnEn) != 
       eSIR_SUCCESS) 
       limLog(pMac, LOGP, FL("Couldn't get WNI_CFG_TELE_BCN_WAKEUP_EN\n"));

    if(teleBcnEn)
    {
       if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_MAX_LI, &val) != eSIR_SUCCESS)
          /**
            * Could not get ListenInterval value
            * from CFG. Log error.
          */
          limLog(pMac, LOGE, FL("could not retrieve ListenInterval\n"));
          return;
    }
    else
    {
    if (wlan_cfgGetInt(pMac, WNI_CFG_LISTEN_INTERVAL, &val) != eSIR_SUCCESS)
      {
         /**
            * Could not get ListenInterval value
            * from CFG. Log error.
            */
         limLog(pMac, LOGE, FL("could not retrieve ListenInterval\n"));
         return;
      }
    }
    if (limSetLinkState(pMac, eSIR_LINK_PREASSOC_STATE, psessionEntry->bssId,
                        psessionEntry->selfMacAddr, NULL, NULL) != eSIR_SUCCESS) 
    {
            return;
    }

    if (limSetLinkState(pMac, eSIR_LINK_PREASSOC_STATE, psessionEntry->bssId,
                        psessionEntry->selfMacAddr, NULL, NULL) != eSIR_SUCCESS)
    {
            return;
    }

    if (limSetLinkState(pMac, eSIR_LINK_POSTASSOC_STATE, psessionEntry->bssId,
                        psessionEntry->selfMacAddr, NULL, NULL) != eSIR_SUCCESS)
    {
            return;
    }

    pMlmReassocReq->listenInterval = (tANI_U16) val;

    psessionEntry->pLimMlmReassocReq = pMlmReassocReq;


    //we need to defer the message until we get the response back from HAL.
    SET_LIM_PROCESS_DEFD_MESGS(pMac, false);
 
    msgQ.type = SIR_HAL_ADD_BSS_REQ;
    msgQ.reserved = 0;
    msgQ.bodyptr = pMac->ft.ftPEContext.pAddBssReq;
    msgQ.bodyval = 0;


#if defined WLAN_FEATURE_VOWIFI_11R_DEBUG
    limLog( pMac, LOGE, FL( "Sending SIR_HAL_ADD_BSS_REQ..." ));
#endif
    MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msgQ.type));

    retCode = wdaPostCtrlMsg( pMac, &msgQ );
    if( eSIR_SUCCESS != retCode) 
    {
        vos_mem_free(pMac->ft.ftPEContext.pAddBssReq);
        limLog( pMac, LOGE, FL("Posting ADD_BSS_REQ to HAL failed, reason=%X\n"),
                retCode );
    }
    // Dont need this anymore
    pMac->ft.ftPEContext.pAddBssReq = NULL;
    return;
}

/*------------------------------------------------------------------
 *
 * This function is called if preauth response is not received from the AP
 * within this timeout while FT in progress
 *
 *------------------------------------------------------------------*/
void limProcessFTPreauthRspTimeout(tpAniSirGlobal pMac)
{
    tpPESession psessionEntry;

    // We have failed pre auth. We need to resume link and get back on
    // home channel.

    if((psessionEntry = peFindSessionBySessionId(pMac, pMac->lim.limTimers.gLimFTPreAuthRspTimer.sessionId))== NULL) 
    {
        limLog(pMac, LOGE, FL("Session Does not exist for given sessionID\n"));
        return;
    }

    // Ok, so attempted at Pre-Auth and failed. If we are off channel. We need
    // to get back.
    limHandleFTPreAuthRsp(pMac, eSIR_FAILURE, NULL, 0, psessionEntry);
}


/*------------------------------------------------------------------
 *
 * This function is called to process the update key request from SME
 *
 *------------------------------------------------------------------*/
tANI_BOOLEAN limProcessFTUpdateKey(tpAniSirGlobal pMac, tANI_U32 *pMsgBuf )
{
   tAddBssParams * pAddBssParams;
   tSirFTUpdateKeyInfo * pKeyInfo;
   tANI_U32 val = 0;

   /* Sanity Check */
   if( pMac == NULL || pMsgBuf == NULL )
   {
      return TRUE;
   }

   pAddBssParams = pMac->ft.ftPEContext.pAddBssReq;
   pKeyInfo = (tSirFTUpdateKeyInfo *)pMsgBuf;

   /* Store the key information in the ADD BSS parameters */
   pAddBssParams->extSetStaKeyParamValid = 1;
   pAddBssParams->extSetStaKeyParam.encType = pKeyInfo->keyMaterial.edType;
   palCopyMemory( pMac->hHdd, (tANI_U8 *) &pAddBssParams->extSetStaKeyParam.key,
                  (tANI_U8 *) &pKeyInfo->keyMaterial.key, sizeof( tSirKeys ));
  if(eSIR_SUCCESS != wlan_cfgGetInt(pMac, WNI_CFG_SINGLE_TID_RC, &val))
  {
     limLog( pMac, LOGP, FL( "Unable to read WNI_CFG_SINGLE_TID_RC\n" ));
  }

  pAddBssParams->extSetStaKeyParam.singleTidRc = val;    

  return TRUE;   
}

tSirRetStatus
limProcessFTAggrQosReq(tpAniSirGlobal pMac, tANI_U32 *pMsgBuf )
{
    tSirMsgQ msg;
    tSirAggrQosReq * aggrQosReq = (tSirAggrQosReq *)pMsgBuf;
    tpAggrAddTsParams pAggrAddTsParam;
    tpPESession  psessionEntry = NULL;
    tpLimTspecInfo   tspecInfo;
    tANI_U8          ac; 
    tpDphHashNode    pSta;
    tANI_U16         aid;
    tANI_U8 sessionId;
    int i;

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                (void **)&pAggrAddTsParam,
                sizeof(tAggrAddTsParams)))
    {
        PELOGE(limLog(pMac, LOGE, FL("palAllocateMemory() failed\n"));)
        return eSIR_MEM_ALLOC_FAILED;
    }

    psessionEntry = peFindSessionByBssid(pMac, aggrQosReq->bssId, &sessionId);

    if (psessionEntry == NULL) {
        PELOGE(limLog(pMac, LOGE, FL("psession Entry Null for sessionId = %d\n"), aggrQosReq->sessionId);)
        return eSIR_FAILURE;
    }

    pSta = dphLookupHashEntry(pMac, aggrQosReq->bssId, &aid, &psessionEntry->dph.dphHashTable);
    if (pSta == NULL)
    {
        PELOGE(limLog(pMac, LOGE, FL("Station context not found - ignoring AddTsRsp\n"));)
        return eSIR_FAILURE;
    }

    palZeroMemory( pMac->hHdd, (tANI_U8 *)pAggrAddTsParam,
            sizeof(tAggrAddTsParams));
    pAggrAddTsParam->staIdx = psessionEntry->staId;
    // Fill in the sessionId specific to PE
    pAggrAddTsParam->sessionId = sessionId;
    pAggrAddTsParam->tspecIdx = aggrQosReq->aggrInfo.tspecIdx;

    for( i = 0; i < HAL_QOS_NUM_AC_MAX; i++ )
    {
        if (aggrQosReq->aggrInfo.tspecIdx & (1<<i)) 
        {
            tSirMacTspecIE *pTspec = &aggrQosReq->aggrInfo.aggrAddTsInfo[i].tspec;
            /* Since AddTS response was successful, check for the PSB flag
             * and directional flag inside the TS Info field. 
             * An AC is trigger enabled AC if the PSB subfield is set to 1  
             * in the uplink direction.
             * An AC is delivery enabled AC if the PSB subfield is set to 1 
             * in the downlink direction.
             * An AC is trigger and delivery enabled AC if the PSB subfield  
             * is set to 1 in the bi-direction field.
             */
            if (pTspec->tsinfo.traffic.psb == 1)
            {
                limSetTspecUapsdMask(pMac, &pTspec->tsinfo, SET_UAPSD_MASK);
            }
            else
            { 
                limSetTspecUapsdMask(pMac, &pTspec->tsinfo, CLEAR_UAPSD_MASK);
            }
            /* ADDTS success, so AC is now admitted. We shall now use the default
             * EDCA parameters as advertised by AP and send the updated EDCA params
             * to HAL. 
             */
            ac = upToAc(pTspec->tsinfo.traffic.userPrio);
            if(pTspec->tsinfo.traffic.direction == SIR_MAC_DIRECTION_UPLINK)
            {
                pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_UPLINK] |= (1 << ac);
            }
            else if(pTspec->tsinfo.traffic.direction == SIR_MAC_DIRECTION_DNLINK)
            {
                pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_DNLINK] |= (1 << ac);
            }
            else if(pTspec->tsinfo.traffic.direction == SIR_MAC_DIRECTION_BIDIR)
            {
                pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_UPLINK] |= (1 << ac);
                pMac->lim.gAcAdmitMask[SIR_MAC_DIRECTION_DNLINK] |= (1 << ac);
            }

            limSetActiveEdcaParams(pMac, psessionEntry->gLimEdcaParams, psessionEntry);

            if (pSta->aniPeer == eANI_BOOLEAN_TRUE) 
            {
                limSendEdcaParams(pMac, psessionEntry->gLimEdcaParamsActive, pSta->bssId, eANI_BOOLEAN_TRUE);
            }
            else 
            {
                limSendEdcaParams(pMac, psessionEntry->gLimEdcaParamsActive, pSta->bssId, eANI_BOOLEAN_FALSE);
            }

            if(eSIR_SUCCESS != limTspecAdd(pMac, pSta->staAddr, pSta->assocId, pTspec,  0, &tspecInfo))
            {
                PELOGE(limLog(pMac, LOGE, FL("Adding entry in lim Tspec Table failed \n"));)
                pMac->lim.gLimAddtsSent = false;
                return eSIR_FAILURE; //Error handling. send the response with error status. need to send DelTS to tear down the TSPEC status.
            }

            // Copy the TSPEC paramters
        pAggrAddTsParam->tspec[i] = aggrQosReq->aggrInfo.aggrAddTsInfo[i].tspec;
    }
    }

    msg.type = WDA_AGGR_QOS_REQ;
    msg.bodyptr = pAggrAddTsParam;
    msg.bodyval = 0;

    /* We need to defer any incoming messages until we get a
     * WDA_AGGR_QOS_RSP from HAL.
     */
    SET_LIM_PROCESS_DEFD_MESGS(pMac, false);
    MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msg.type));

    if(eSIR_SUCCESS != wdaPostCtrlMsg(pMac, &msg))
    {
       PELOGW(limLog(pMac, LOGW, FL("wdaPostCtrlMsg() failed\n"));)
       SET_LIM_PROCESS_DEFD_MESGS(pMac, true);
       palFreeMemory(pMac->hHdd, (tANI_U8*)pAggrAddTsParam);
       return eSIR_FAILURE;
    }

    return eSIR_SUCCESS;
}

void
limFTSendAggrQosRsp(tpAniSirGlobal pMac, tANI_U8 rspReqd,
                    tpAggrAddTsParams aggrQosRsp, tANI_U8 smesessionId)
{
    tpSirAggrQosRsp  rsp;
    int i = 0;

    if (! rspReqd)
    {
        return;
    }

    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **)&rsp,
                sizeof(tSirAggrQosRsp)))
    {
        limLog(pMac, LOGP, FL("palAllocateMemory failed for tSirAggrQosRsp"));
        return;
    }

    palZeroMemory( pMac->hHdd, (tANI_U8 *) rsp, sizeof(*rsp));
    rsp->messageType = eWNI_SME_FT_AGGR_QOS_RSP;
    rsp->sessionId = smesessionId;
    rsp->length = sizeof(*rsp);
    rsp->aggrInfo.tspecIdx = aggrQosRsp->tspecIdx;

    for( i = 0; i < SIR_QOS_NUM_AC_MAX; i++ )
    {
        if( (1 << i) & aggrQosRsp->tspecIdx )
        {
            rsp->aggrInfo.aggrRsp[i].status = aggrQosRsp->status[i];
            rsp->aggrInfo.aggrRsp[i].tspec = aggrQosRsp->tspec[i];
        }
    }

    limSendSmeAggrQosRsp(pMac, rsp, smesessionId);
    return;
}


void limProcessFTAggrQoSRsp(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
    tpAggrAddTsParams pAggrQosRspMsg = NULL;
    //tpAggrQosParams  pAggrQosRspMsg = NULL;
    tAddTsParams     addTsParam = {0};
    tpDphHashNode  pSta = NULL;
    tANI_U16  assocId =0;
    tSirMacAddr  peerMacAddr;
    tANI_U8   rspReqd = 1;
    tpPESession  psessionEntry = NULL;
    int i = 0;

    PELOG1(limLog(pMac, LOG1, FL(" Received AGGR_QOS_RSP from HAL\n"));)

    /* Need to process all the deferred messages enqueued since sending the
       SIR_HAL_AGGR_ADD_TS_REQ */
    SET_LIM_PROCESS_DEFD_MESGS(pMac, true);

    pAggrQosRspMsg = (tpAggrAddTsParams) (limMsg->bodyptr);
    if (NULL == pAggrQosRspMsg)
    {
        PELOGE(limLog(pMac, LOGE, FL("NULL pAggrQosRspMsg"));)
        return;
    }

    psessionEntry = peFindSessionBySessionId(pMac, pAggrQosRspMsg->sessionId);
    if (NULL == psessionEntry)
    {
        // Cant find session entry
        PELOGE(limLog(pMac, LOGE, FL("Cant find session entry for %s\n"), __FUNCTION__);)
        if( pAggrQosRspMsg != NULL )
        {
            palFreeMemory( pMac->hHdd, (void *)pAggrQosRspMsg );
        }
        return;
    }

    for( i = 0; i < HAL_QOS_NUM_AC_MAX; i++ )
    {
        if((((1 << i) & pAggrQosRspMsg->tspecIdx)) &&
                (pAggrQosRspMsg->status[i] != eHAL_STATUS_SUCCESS))
        {
            /* send DELTS to the station */
            sirCopyMacAddr(peerMacAddr,psessionEntry->bssId);

            addTsParam.staIdx = pAggrQosRspMsg->staIdx;
            addTsParam.sessionId = pAggrQosRspMsg->sessionId;
            addTsParam.tspec = pAggrQosRspMsg->tspec[i];
            addTsParam.tspecIdx = pAggrQosRspMsg->tspecIdx;

            limSendDeltsReqActionFrame(pMac, peerMacAddr, rspReqd,
                    &addTsParam.tspec.tsinfo,
                    &addTsParam.tspec, psessionEntry);

            pSta = dphLookupAssocId(pMac, addTsParam.staIdx, &assocId,
                    &psessionEntry->dph.dphHashTable);
            if (pSta != NULL)
            {
                limAdmitControlDeleteTS(pMac, assocId, &addTsParam.tspec.tsinfo,
                        NULL, (tANI_U8 *)&addTsParam.tspecIdx);
            }
        }
    }

    /* Send the Aggr QoS response to SME */

    limFTSendAggrQosRsp(pMac, rspReqd, pAggrQosRspMsg,
            psessionEntry->smeSessionId);
    if( pAggrQosRspMsg != NULL )
    {
        palFreeMemory( pMac->hHdd, (void *)pAggrQosRspMsg );
    }
    return;
}


/*--------------------------------------------------------------------------
         Determines if a session with ccx or 11r assoc is present.
        If present it will return TRUE else FALSE
  ------------------------------------------------------------------------*/
int limisFastTransitionRequired(tpAniSirGlobal pMac, int sessionId)
{
    if(pMac->lim.gpSession[sessionId].valid == TRUE)
    {
        // If ccx or 11r connection is found we need to return TRUE
        if((pMac->lim.gpSession[sessionId].bssType == eSIR_INFRASTRUCTURE_MODE) &&
           (((pMac->lim.gpSession[sessionId].is11Rconnection) 
#ifdef FEATURE_WLAN_CCX
           || (pMac->lim.gpSession[sessionId].isCCXconnection)
#endif
#ifdef FEATURE_WLAN_LFR
           || (pMac->lim.gpSession[sessionId].isFastRoamIniFeatureEnabled)
#endif
           )&& 
            pMac->lim.gpSession[sessionId].isFastTransitionEnabled))
        {
            // Make sure we have 11r/CCX and FT enabled only then we need
            // the values to be altered from cfg for FW RSSI Period alteration.
            return TRUE;
        }
    }

    return FALSE;
}

#endif /* WLAN_FEATURE_VOWIFI_11R */
