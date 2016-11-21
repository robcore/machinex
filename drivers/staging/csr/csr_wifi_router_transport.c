/** @file router_transport.c
 *
 *
 * Copyright (C) Cambridge Silicon Radio Ltd 2006-2010. All rights reserved.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 ****************************************************************************/

#include "unifi_priv.h"

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_msgconv.h"

#include "sme_userspace.h"

#include "csr_wifi_hostio_prim.h"
#include "csr_wifi_router_lib.h"
#include "csr_wifi_router_sef.h"
#include "csr_wifi_router_converter_init.h"
#include "csr_wifi_router_ctrl_lib.h"
#include "csr_wifi_router_ctrl_sef.h"
#include "csr_wifi_router_ctrl_converter_init.h"
#include "csr_wifi_sme_prim.h"
#include "csr_wifi_sme_sef.h"
#include "csr_wifi_sme_converter_init.h"
#ifdef CSR_SUPPORT_WEXT
#ifdef CSR_SUPPORT_WEXT_AP
#include "csr_wifi_nme_ap_prim.h"
#include "csr_wifi_nme_ap_sef.h"
#include "csr_wifi_nme_ap_converter_init.h"
#endif
#endif

static unifi_priv_t *drvpriv = NULL;
void CsrWifiRouterTransportInit(unifi_priv_t *priv)
{
    unifi_trace(priv, UDBG1, "CsrWifiRouterTransportInit: \n");

    drvpriv = priv;
    (void)CsrMsgConvInit();
    CsrWifiRouterConverterInit();
    CsrWifiRouterCtrlConverterInit();
    CsrWifiSmeConverterInit();
#ifdef CSR_SUPPORT_WEXT
#ifdef CSR_SUPPORT_WEXT_AP
    CsrWifiNmeApConverterInit();
#endif
#endif
}

void CsrWifiRouterTransportDeinit(unifi_priv_t *priv)
{
    unifi_trace(priv, UDBG1, "CsrWifiRouterTransportDeinit: \n");
    if (priv == drvpriv)
    {
        CsrMsgConvDeinit();
        drvpriv = NULL;
    }
}

void CsrWifiRouterTransportRecv(unifi_priv_t *priv, CsrUint8* buffer, CsrSize bufferLength)
{
    CsrMsgConvMsgEntry* msgEntry;
    CsrUint16 primType;
    CsrSchedQid src;
    CsrSchedQid dest;
    CsrUint16 msgType;
    CsrSize offset = 0;
    CsrWifiFsmEvent* msg;

    /* Decode the prim and message type */
    CsrUint16Des(&primType, buffer, &offset);
    CsrUint16Des(&src, buffer, &offset);
    CsrUint16Des(&dest, buffer, &offset);
    CsrUint16Des(&msgType, buffer, &offset);
    offset -= 2; /* Adjust as the Deserialise Function will read this as well */

    unifi_trace(priv, UDBG4, "CsrWifiRouterTransportRecv: primType=0x%.4X, msgType=0x%.4X, bufferLength=%d\n",
                primType, msgType, bufferLength);

    /* Special handling for HOSTIO messages.... */
    if (primType == CSR_WIFI_HOSTIO_PRIM)
    {
        CsrWifiRouterCtrlHipReq req = {{CSR_WIFI_ROUTER_CTRL_HIP_REQ, CSR_WIFI_ROUTER_CTRL_PRIM, dest, src, NULL}, 0, NULL, 0, NULL, 0, NULL};

        req.mlmeCommandLength = bufferLength;
        req.mlmeCommand = buffer;

        offset += 8;/* Skip the id, src, dest and slot number */
        CsrUint16Des(&req.dataRef1Length, buffer, &offset);
        offset += 2; /* Skip the slot number */
        CsrUint16Des(&req.dataRef2Length, buffer, &offset);

        if (req.dataRef1Length)
        {
            CsrUint16 dr1Offset = (bufferLength - req.dataRef2Length) - req.dataRef1Length;
            req.dataRef1 = &buffer[dr1Offset];
        }

        if (req.dataRef2Length)
        {
            CsrUint16 dr2Offset = bufferLength - req.dataRef2Length;
            req.dataRef2 = &buffer[dr2Offset];
        }

        /* Copy the hip data but strip off the prim type */
        req.mlmeCommandLength -= (req.dataRef1Length + req.dataRef2Length + 6);
        req.mlmeCommand = &buffer[6];

        CsrWifiRouterCtrlHipReqHandler(priv, &req.common);
        return;
    }

    msgEntry = CsrMsgConvFindEntry(primType, msgType);
    if (!msgEntry)
    {
        unifi_error(priv, "CsrWifiRouterTransportDeserialiseAndSend can not process the message. primType=0x%.4X, msgType=0x%.4X\n",
                    primType, msgType);
        dump(buffer, bufferLength);
        return;
    }

    msg = (CsrWifiFsmEvent*)(msgEntry->deserFunc)(&buffer[offset], bufferLength - offset);

    msg->primtype = primType;
    msg->type = msgType;
    msg->source = src;
    msg->destination = dest;

    switch(primType)
    {
    case CSR_WIFI_ROUTER_CTRL_PRIM:
        CsrWifiRouterCtrlDownstreamStateHandlers[msg->type - CSR_WIFI_ROUTER_CTRL_PRIM_DOWNSTREAM_LOWEST](priv, msg);
        CsrWifiRouterCtrlFreeDownstreamMessageContents(CSR_WIFI_ROUTER_CTRL_PRIM, msg);
        break;
    case CSR_WIFI_ROUTER_PRIM:
        CsrWifiRouterDownstreamStateHandlers[msg->type - CSR_WIFI_ROUTER_PRIM_DOWNSTREAM_LOWEST](priv, msg);
        CsrWifiRouterFreeDownstreamMessageContents(CSR_WIFI_ROUTER_PRIM, msg);
        break;
        case CSR_WIFI_SME_PRIM:
            CsrWifiSmeUpstreamStateHandlers[msg->type - CSR_WIFI_SME_PRIM_UPSTREAM_LOWEST](priv, msg);
            CsrWifiSmeFreeUpstreamMessageContents(CSR_WIFI_SME_PRIM, msg);
            break;
#ifdef CSR_SUPPORT_WEXT
#ifdef CSR_SUPPORT_WEXT_AP
        case CSR_WIFI_NME_AP_PRIM:
            CsrWifiNmeApUpstreamStateHandlers(priv, msg);
            CsrWifiNmeApFreeUpstreamMessageContents(CSR_WIFI_NME_AP_PRIM, msg);
            break;
#endif
#endif
        default:
            unifi_error(priv, "CsrWifiRouterTransportDeserialiseAndSend unhandled prim type 0x%.4X\n", primType);
            break;
    }
    CsrPmemFree(msg);
}

static void CsrWifiRouterTransportSerialiseAndSend(CsrUint16 primType, void* msg)
{
    CsrWifiFsmEvent* evt = (CsrWifiFsmEvent*)msg;
    CsrMsgConvMsgEntry* msgEntry;
    CsrSize msgSize;
    CsrSize encodeBufferLen = 0;
    CsrSize offset = 0;
    CsrUint8* encodeBuffer;

    unifi_trace(drvpriv, UDBG4, "CsrWifiRouterTransportSerialiseAndSend: primType=0x%.4X, msgType=0x%.4X\n",
                primType, evt->type);

    msgEntry = CsrMsgConvFindEntry(primType, evt->type);
    if (!msgEntry)
    {
        unifi_error(drvpriv, "CsrWifiRouterTransportSerialiseAndSend can not process the message. primType=0x%.4X, msgType=0x%.4X\n",
                    primType, evt->type);
        return;
    }

    msgSize = 6 + (msgEntry->sizeofFunc)((void*)msg);

    encodeBuffer = CsrPmemAlloc(msgSize);

    /* Encode PrimType */
    CsrUint16Ser(encodeBuffer, &encodeBufferLen, primType);
    CsrUint16Ser(encodeBuffer, &encodeBufferLen, evt->source);
    CsrUint16Ser(encodeBuffer, &encodeBufferLen, evt->destination);

    (void)(msgEntry->serFunc)(&encodeBuffer[encodeBufferLen], &offset, msg);
    encodeBufferLen += offset;

    uf_sme_queue_message(drvpriv, encodeBuffer, encodeBufferLen);

    /* Do not use msgEntry->freeFunc because the memory is owned by the driver */
    CsrPmemFree(msg);
}

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_INCLUDE_FILE_NAME_AND_LINE_NUMBER)
void CsrSchedMessagePutStringLog(CsrSchedQid q, CsrUint16 mi, void *mv, CsrUint32 line, CsrCharString *file)
#else
void CsrSchedMessagePut(CsrSchedQid q, CsrUint16 mi, void *mv)
#endif
{
    CsrWifiFsmEvent* evt = (CsrWifiFsmEvent*)mv;
    evt->destination = q;
    CsrWifiRouterTransportSerialiseAndSend(mi, mv);
}

