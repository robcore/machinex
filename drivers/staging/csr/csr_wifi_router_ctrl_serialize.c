/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2012
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/

/* Note: this is an auto-generated file. */

#include "csr_pmem.h"
#include "csr_msgconv.h"
#include "csr_unicode.h"


#include "csr_wifi_router_ctrl_prim.h"
#include "csr_wifi_router_ctrl_serialize.h"

void CsrWifiRouterCtrlPfree(void *ptr)
{
    CsrPmemFree(ptr);
}


CsrSize CsrWifiRouterCtrlConfigurePowerModeReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 8) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrWifiRouterCtrlLowPowerMode primitive->mode */
    bufferSize += 1; /* CsrBool primitive->wakeHost */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlConfigurePowerModeReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlConfigurePowerModeReq *primitive = (CsrWifiRouterCtrlConfigurePowerModeReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->mode);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->wakeHost);
    return(ptr);
}


void* CsrWifiRouterCtrlConfigurePowerModeReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlConfigurePowerModeReq *primitive = (CsrWifiRouterCtrlConfigurePowerModeReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlConfigurePowerModeReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->mode, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->wakeHost, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlHipReqSizeof(void *msg)
{
    CsrWifiRouterCtrlHipReq *primitive = (CsrWifiRouterCtrlHipReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 12) */
    bufferSize += 2;                            /* CsrUint16 primitive->mlmeCommandLength */
    bufferSize += primitive->mlmeCommandLength; /* CsrUint8 primitive->mlmeCommand */
    bufferSize += 2;                            /* CsrUint16 primitive->dataRef1Length */
    bufferSize += primitive->dataRef1Length;    /* CsrUint8 primitive->dataRef1 */
    bufferSize += 2;                            /* CsrUint16 primitive->dataRef2Length */
    bufferSize += primitive->dataRef2Length;    /* CsrUint8 primitive->dataRef2 */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlHipReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlHipReq *primitive = (CsrWifiRouterCtrlHipReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->mlmeCommandLength);
    if (primitive->mlmeCommandLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->mlmeCommand, ((CsrUint16) (primitive->mlmeCommandLength)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataRef1Length);
    if (primitive->dataRef1Length)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->dataRef1, ((CsrUint16) (primitive->dataRef1Length)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataRef2Length);
    if (primitive->dataRef2Length)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->dataRef2, ((CsrUint16) (primitive->dataRef2Length)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlHipReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlHipReq *primitive = (CsrWifiRouterCtrlHipReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlHipReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->mlmeCommandLength, buffer, &offset);
    if (primitive->mlmeCommandLength)
    {
        primitive->mlmeCommand = (CsrUint8 *)CsrPmemAlloc(primitive->mlmeCommandLength);
        CsrMemCpyDes(primitive->mlmeCommand, buffer, &offset, ((CsrUint16) (primitive->mlmeCommandLength)));
    }
    else
    {
        primitive->mlmeCommand = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataRef1Length, buffer, &offset);
    if (primitive->dataRef1Length)
    {
        primitive->dataRef1 = (CsrUint8 *)CsrPmemAlloc(primitive->dataRef1Length);
        CsrMemCpyDes(primitive->dataRef1, buffer, &offset, ((CsrUint16) (primitive->dataRef1Length)));
    }
    else
    {
        primitive->dataRef1 = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataRef2Length, buffer, &offset);
    if (primitive->dataRef2Length)
    {
        primitive->dataRef2 = (CsrUint8 *)CsrPmemAlloc(primitive->dataRef2Length);
        CsrMemCpyDes(primitive->dataRef2, buffer, &offset, ((CsrUint16) (primitive->dataRef2Length)));
    }
    else
    {
        primitive->dataRef2 = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlHipReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlHipReq *primitive = (CsrWifiRouterCtrlHipReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->mlmeCommand);
    CsrPmemFree(primitive->dataRef1);
    CsrPmemFree(primitive->dataRef2);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlMediaStatusReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 8) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 1; /* CsrWifiRouterCtrlMediaStatus primitive->mediaStatus */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlMediaStatusReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlMediaStatusReq *primitive = (CsrWifiRouterCtrlMediaStatusReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->mediaStatus);
    return(ptr);
}


void* CsrWifiRouterCtrlMediaStatusReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlMediaStatusReq *primitive = (CsrWifiRouterCtrlMediaStatusReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlMediaStatusReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->mediaStatus, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlMulticastAddressResSizeof(void *msg)
{
    CsrWifiRouterCtrlMulticastAddressRes *primitive = (CsrWifiRouterCtrlMulticastAddressRes *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 17) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->status */
    bufferSize += 1; /* CsrWifiRouterCtrlListAction primitive->action */
    bufferSize += 1; /* CsrUint8 primitive->getAddressesCount */
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->getAddressesCount; i1++)
        {
            bufferSize += 6; /* CsrUint8 primitive->getAddresses[i1].a[6] */
        }
    }
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlMulticastAddressResSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlMulticastAddressRes *primitive = (CsrWifiRouterCtrlMulticastAddressRes *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->action);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->getAddressesCount);
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->getAddressesCount; i1++)
        {
            CsrMemCpySer(ptr, len, (const void *) primitive->getAddresses[i1].a, ((CsrUint16) (6)));
        }
    }
    return(ptr);
}


void* CsrWifiRouterCtrlMulticastAddressResDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlMulticastAddressRes *primitive = (CsrWifiRouterCtrlMulticastAddressRes *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlMulticastAddressRes));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->action, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->getAddressesCount, buffer, &offset);
    primitive->getAddresses = NULL;
    if (primitive->getAddressesCount)
    {
        primitive->getAddresses = (CsrWifiMacAddress *)CsrPmemAlloc(sizeof(CsrWifiMacAddress) * primitive->getAddressesCount);
    }
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->getAddressesCount; i1++)
        {
            CsrMemCpyDes(primitive->getAddresses[i1].a, buffer, &offset, ((CsrUint16) (6)));
        }
    }

    return primitive;
}


void CsrWifiRouterCtrlMulticastAddressResSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlMulticastAddressRes *primitive = (CsrWifiRouterCtrlMulticastAddressRes *) voidPrimitivePointer;
    CsrPmemFree(primitive->getAddresses);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlPortConfigureReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 18) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrWifiRouterCtrlPortAction primitive->uncontrolledPortAction */
    bufferSize += 2; /* CsrWifiRouterCtrlPortAction primitive->controlledPortAction */
    bufferSize += 6; /* CsrUint8 primitive->macAddress.a[6] */
    bufferSize += 1; /* CsrBool primitive->setProtection */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPortConfigureReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPortConfigureReq *primitive = (CsrWifiRouterCtrlPortConfigureReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->uncontrolledPortAction);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->controlledPortAction);
    CsrMemCpySer(ptr, len, (const void *) primitive->macAddress.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->setProtection);
    return(ptr);
}


void* CsrWifiRouterCtrlPortConfigureReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPortConfigureReq *primitive = (CsrWifiRouterCtrlPortConfigureReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPortConfigureReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->uncontrolledPortAction, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->controlledPortAction, buffer, &offset);
    CsrMemCpyDes(primitive->macAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->setProtection, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlQosControlReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrWifiRouterCtrlQoSControl primitive->control */
    bufferSize += 1; /* CsrWifiRouterCtrlQueueConfigMask primitive->queueConfig */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlQosControlReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlQosControlReq *primitive = (CsrWifiRouterCtrlQosControlReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->control);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->queueConfig);
    return(ptr);
}


void* CsrWifiRouterCtrlQosControlReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlQosControlReq *primitive = (CsrWifiRouterCtrlQosControlReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlQosControlReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->control, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->queueConfig, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlSuspendResSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlSuspendResSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlSuspendRes *primitive = (CsrWifiRouterCtrlSuspendRes *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlSuspendResDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlSuspendRes *primitive = (CsrWifiRouterCtrlSuspendRes *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlSuspendRes));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlTclasAddReqSizeof(void *msg)
{
    CsrWifiRouterCtrlTclasAddReq *primitive = (CsrWifiRouterCtrlTclasAddReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2;                      /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                      /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;                      /* CsrUint16 primitive->tclasLength */
    bufferSize += primitive->tclasLength; /* CsrUint8 primitive->tclas */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTclasAddReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTclasAddReq *primitive = (CsrWifiRouterCtrlTclasAddReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->tclasLength);
    if (primitive->tclasLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->tclas, ((CsrUint16) (primitive->tclasLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlTclasAddReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTclasAddReq *primitive = (CsrWifiRouterCtrlTclasAddReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTclasAddReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->tclasLength, buffer, &offset);
    if (primitive->tclasLength)
    {
        primitive->tclas = (CsrUint8 *)CsrPmemAlloc(primitive->tclasLength);
        CsrMemCpyDes(primitive->tclas, buffer, &offset, ((CsrUint16) (primitive->tclasLength)));
    }
    else
    {
        primitive->tclas = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlTclasAddReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlTclasAddReq *primitive = (CsrWifiRouterCtrlTclasAddReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->tclas);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlResumeResSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlResumeResSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlResumeRes *primitive = (CsrWifiRouterCtrlResumeRes *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlResumeResDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlResumeRes *primitive = (CsrWifiRouterCtrlResumeRes *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlResumeRes));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlTclasDelReqSizeof(void *msg)
{
    CsrWifiRouterCtrlTclasDelReq *primitive = (CsrWifiRouterCtrlTclasDelReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2;                      /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                      /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;                      /* CsrUint16 primitive->tclasLength */
    bufferSize += primitive->tclasLength; /* CsrUint8 primitive->tclas */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTclasDelReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTclasDelReq *primitive = (CsrWifiRouterCtrlTclasDelReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->tclasLength);
    if (primitive->tclasLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->tclas, ((CsrUint16) (primitive->tclasLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlTclasDelReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTclasDelReq *primitive = (CsrWifiRouterCtrlTclasDelReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTclasDelReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->tclasLength, buffer, &offset);
    if (primitive->tclasLength)
    {
        primitive->tclas = (CsrUint8 *)CsrPmemAlloc(primitive->tclasLength);
        CsrMemCpyDes(primitive->tclas, buffer, &offset, ((CsrUint16) (primitive->tclasLength)));
    }
    else
    {
        primitive->tclas = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlTclasDelReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlTclasDelReq *primitive = (CsrWifiRouterCtrlTclasDelReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->tclas);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlTrafficClassificationReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 1; /* CsrWifiRouterCtrlTrafficType primitive->trafficType */
    bufferSize += 2; /* CsrUint16 primitive->period */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTrafficClassificationReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTrafficClassificationReq *primitive = (CsrWifiRouterCtrlTrafficClassificationReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->trafficType);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->period);
    return(ptr);
}


void* CsrWifiRouterCtrlTrafficClassificationReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTrafficClassificationReq *primitive = (CsrWifiRouterCtrlTrafficClassificationReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTrafficClassificationReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->trafficType, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->period, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlTrafficConfigReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 24) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrWifiRouterCtrlTrafficConfigType primitive->trafficConfigType */
    bufferSize += 2; /* CsrUint16 primitive->config.packetFilter */
    bufferSize += 4; /* CsrUint32 primitive->config.customFilter.etherType */
    bufferSize += 1; /* CsrUint8 primitive->config.customFilter.ipType */
    bufferSize += 4; /* CsrUint32 primitive->config.customFilter.udpSourcePort */
    bufferSize += 4; /* CsrUint32 primitive->config.customFilter.udpDestPort */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTrafficConfigReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTrafficConfigReq *primitive = (CsrWifiRouterCtrlTrafficConfigReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->trafficConfigType);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->config.packetFilter);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->config.customFilter.etherType);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->config.customFilter.ipType);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->config.customFilter.udpSourcePort);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->config.customFilter.udpDestPort);
    return(ptr);
}


void* CsrWifiRouterCtrlTrafficConfigReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTrafficConfigReq *primitive = (CsrWifiRouterCtrlTrafficConfigReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTrafficConfigReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->trafficConfigType, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->config.packetFilter, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->config.customFilter.etherType, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->config.customFilter.ipType, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->config.customFilter.udpSourcePort, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->config.customFilter.udpDestPort, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlWifiOnReqSizeof(void *msg)
{
    CsrWifiRouterCtrlWifiOnReq *primitive = (CsrWifiRouterCtrlWifiOnReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2;                     /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 4;                     /* CsrUint32 primitive->dataLength */
    bufferSize += primitive->dataLength; /* CsrUint8 primitive->data */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWifiOnReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWifiOnReq *primitive = (CsrWifiRouterCtrlWifiOnReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->dataLength);
    if (primitive->dataLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->data, ((CsrUint16) (primitive->dataLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlWifiOnReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWifiOnReq *primitive = (CsrWifiRouterCtrlWifiOnReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWifiOnReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->dataLength, buffer, &offset);
    if (primitive->dataLength)
    {
        primitive->data = (CsrUint8 *)CsrPmemAlloc(primitive->dataLength);
        CsrMemCpyDes(primitive->data, buffer, &offset, ((CsrUint16) (primitive->dataLength)));
    }
    else
    {
        primitive->data = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlWifiOnReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWifiOnReq *primitive = (CsrWifiRouterCtrlWifiOnReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->data);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlWifiOnResSizeof(void *msg)
{
    CsrWifiRouterCtrlWifiOnRes *primitive = (CsrWifiRouterCtrlWifiOnRes *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 30) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->status */
    bufferSize += 2; /* CsrUint16 primitive->numInterfaceAddress */
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < 2; i1++)
        {
            bufferSize += 6;                                                                            /* CsrUint8 primitive->stationMacAddress[i1].a[6] */
        }
    }
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->smeVersions.firmwarePatch */
    bufferSize += (primitive->smeVersions.smeBuild?CsrStrLen(primitive->smeVersions.smeBuild) : 0) + 1; /* CsrCharString* primitive->smeVersions.smeBuild (0 byte len + 1 for NULL Term) */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->smeVersions.smeHip */
    bufferSize += 1;                                                                                    /* CsrBool primitive->scheduledInterrupt */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWifiOnResSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWifiOnRes *primitive = (CsrWifiRouterCtrlWifiOnRes *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->numInterfaceAddress);
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < 2; i1++)
        {
            CsrMemCpySer(ptr, len, (const void *) primitive->stationMacAddress[i1].a, ((CsrUint16) (6)));
        }
    }
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->smeVersions.firmwarePatch);
    CsrCharStringSer(ptr, len, primitive->smeVersions.smeBuild);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->smeVersions.smeHip);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->scheduledInterrupt);
    return(ptr);
}


void* CsrWifiRouterCtrlWifiOnResDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWifiOnRes *primitive = (CsrWifiRouterCtrlWifiOnRes *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWifiOnRes));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->numInterfaceAddress, buffer, &offset);
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < 2; i1++)
        {
            CsrMemCpyDes(primitive->stationMacAddress[i1].a, buffer, &offset, ((CsrUint16) (6)));
        }
    }
    CsrUint32Des((CsrUint32 *) &primitive->smeVersions.firmwarePatch, buffer, &offset);
    CsrCharStringDes(&primitive->smeVersions.smeBuild, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->smeVersions.smeHip, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->scheduledInterrupt, buffer, &offset);

    return primitive;
}


void CsrWifiRouterCtrlWifiOnResSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWifiOnRes *primitive = (CsrWifiRouterCtrlWifiOnRes *) voidPrimitivePointer;
    CsrPmemFree(primitive->smeVersions.smeBuild);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlM4TransmitReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlM4TransmitReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlM4TransmitReq *primitive = (CsrWifiRouterCtrlM4TransmitReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    return(ptr);
}


void* CsrWifiRouterCtrlM4TransmitReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlM4TransmitReq *primitive = (CsrWifiRouterCtrlM4TransmitReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlM4TransmitReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlModeSetReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 16) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 1; /* CsrWifiRouterCtrlMode primitive->mode */
    bufferSize += 6; /* CsrUint8 primitive->bssid.a[6] */
    bufferSize += 1; /* CsrBool primitive->protection */
    bufferSize += 1; /* CsrBool primitive->intraBssDistEnabled */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlModeSetReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlModeSetReq *primitive = (CsrWifiRouterCtrlModeSetReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->mode);
    CsrMemCpySer(ptr, len, (const void *) primitive->bssid.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->protection);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->intraBssDistEnabled);
    return(ptr);
}


void* CsrWifiRouterCtrlModeSetReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlModeSetReq *primitive = (CsrWifiRouterCtrlModeSetReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlModeSetReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->mode, buffer, &offset);
    CsrMemCpyDes(primitive->bssid.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->protection, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->intraBssDistEnabled, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerAddReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 21) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 2; /* CsrUint16 primitive->associationId */
    bufferSize += 1; /* CsrBool primitive->staInfo.wmmOrQosEnabled */
    bufferSize += 2; /* CsrWifiRouterCtrlPowersaveTypeMask primitive->staInfo.powersaveMode */
    bufferSize += 1; /* CsrUint8 primitive->staInfo.maxSpLength */
    bufferSize += 2; /* CsrUint16 primitive->staInfo.listenIntervalInTus */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerAddReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerAddReq *primitive = (CsrWifiRouterCtrlPeerAddReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->associationId);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->staInfo.wmmOrQosEnabled);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->staInfo.powersaveMode);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->staInfo.maxSpLength);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->staInfo.listenIntervalInTus);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerAddReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerAddReq *primitive = (CsrWifiRouterCtrlPeerAddReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerAddReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint16Des((CsrUint16 *) &primitive->associationId, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->staInfo.wmmOrQosEnabled, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->staInfo.powersaveMode, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->staInfo.maxSpLength, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->staInfo.listenIntervalInTus, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerDelReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 11) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 4; /* CsrWifiRouterCtrlPeerRecordHandle primitive->peerRecordHandle */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerDelReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerDelReq *primitive = (CsrWifiRouterCtrlPeerDelReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->peerRecordHandle);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerDelReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerDelReq *primitive = (CsrWifiRouterCtrlPeerDelReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerDelReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->peerRecordHandle, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerUpdateReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 13) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 4; /* CsrWifiRouterCtrlPeerRecordHandle primitive->peerRecordHandle */
    bufferSize += 2; /* CsrWifiRouterCtrlPowersaveTypeMask primitive->powersaveMode */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerUpdateReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerUpdateReq *primitive = (CsrWifiRouterCtrlPeerUpdateReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->peerRecordHandle);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->powersaveMode);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerUpdateReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerUpdateReq *primitive = (CsrWifiRouterCtrlPeerUpdateReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerUpdateReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->peerRecordHandle, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->powersaveMode, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlBlockAckEnableReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 21) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 6; /* CsrUint8 primitive->macAddress.a[6] */
    bufferSize += 1; /* CsrWifiRouterCtrlTrafficStreamId primitive->trafficStreamID */
    bufferSize += 1; /* CsrWifiRouterCtrlBlockAckRole primitive->role */
    bufferSize += 2; /* CsrUint16 primitive->bufferSize */
    bufferSize += 2; /* CsrUint16 primitive->timeout */
    bufferSize += 2; /* CsrUint16 primitive->ssn */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlBlockAckEnableReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlBlockAckEnableReq *primitive = (CsrWifiRouterCtrlBlockAckEnableReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrMemCpySer(ptr, len, (const void *) primitive->macAddress.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->trafficStreamID);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->role);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->bufferSize);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->timeout);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->ssn);
    return(ptr);
}


void* CsrWifiRouterCtrlBlockAckEnableReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlBlockAckEnableReq *primitive = (CsrWifiRouterCtrlBlockAckEnableReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlBlockAckEnableReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrMemCpyDes(primitive->macAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->trafficStreamID, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->role, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->bufferSize, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->timeout, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->ssn, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlBlockAckDisableReqSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 15) */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 6; /* CsrUint8 primitive->macAddress.a[6] */
    bufferSize += 1; /* CsrWifiRouterCtrlTrafficStreamId primitive->trafficStreamID */
    bufferSize += 1; /* CsrWifiRouterCtrlBlockAckRole primitive->role */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlBlockAckDisableReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlBlockAckDisableReq *primitive = (CsrWifiRouterCtrlBlockAckDisableReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrMemCpySer(ptr, len, (const void *) primitive->macAddress.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->trafficStreamID);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->role);
    return(ptr);
}


void* CsrWifiRouterCtrlBlockAckDisableReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlBlockAckDisableReq *primitive = (CsrWifiRouterCtrlBlockAckDisableReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlBlockAckDisableReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrMemCpyDes(primitive->macAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->trafficStreamID, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->role, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlWapiRxPktReqSizeof(void *msg)
{
    CsrWifiRouterCtrlWapiRxPktReq *primitive = (CsrWifiRouterCtrlWapiRxPktReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 11) */
    bufferSize += 2;                       /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                       /* CsrUint16 primitive->signalLength */
    bufferSize += primitive->signalLength; /* CsrUint8 primitive->signal */
    bufferSize += 2;                       /* CsrUint16 primitive->dataLength */
    bufferSize += primitive->dataLength;   /* CsrUint8 primitive->data */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWapiRxPktReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWapiRxPktReq *primitive = (CsrWifiRouterCtrlWapiRxPktReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->signalLength);
    if (primitive->signalLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->signal, ((CsrUint16) (primitive->signalLength)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataLength);
    if (primitive->dataLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->data, ((CsrUint16) (primitive->dataLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlWapiRxPktReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWapiRxPktReq *primitive = (CsrWifiRouterCtrlWapiRxPktReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWapiRxPktReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->signalLength, buffer, &offset);
    if (primitive->signalLength)
    {
        primitive->signal = (CsrUint8 *)CsrPmemAlloc(primitive->signalLength);
        CsrMemCpyDes(primitive->signal, buffer, &offset, ((CsrUint16) (primitive->signalLength)));
    }
    else
    {
        primitive->signal = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataLength, buffer, &offset);
    if (primitive->dataLength)
    {
        primitive->data = (CsrUint8 *)CsrPmemAlloc(primitive->dataLength);
        CsrMemCpyDes(primitive->data, buffer, &offset, ((CsrUint16) (primitive->dataLength)));
    }
    else
    {
        primitive->data = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlWapiRxPktReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWapiRxPktReq *primitive = (CsrWifiRouterCtrlWapiRxPktReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->signal);
    CsrPmemFree(primitive->data);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlWapiUnicastTxPktReqSizeof(void *msg)
{
    CsrWifiRouterCtrlWapiUnicastTxPktReq *primitive = (CsrWifiRouterCtrlWapiUnicastTxPktReq *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 8) */
    bufferSize += 2;                     /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                     /* CsrUint16 primitive->dataLength */
    bufferSize += primitive->dataLength; /* CsrUint8 primitive->data */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWapiUnicastTxPktReqSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWapiUnicastTxPktReq *primitive = (CsrWifiRouterCtrlWapiUnicastTxPktReq *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataLength);
    if (primitive->dataLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->data, ((CsrUint16) (primitive->dataLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlWapiUnicastTxPktReqDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWapiUnicastTxPktReq *primitive = (CsrWifiRouterCtrlWapiUnicastTxPktReq *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWapiUnicastTxPktReq));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->dataLength, buffer, &offset);
    if (primitive->dataLength)
    {
        primitive->data = (CsrUint8 *)CsrPmemAlloc(primitive->dataLength);
        CsrMemCpyDes(primitive->data, buffer, &offset, ((CsrUint16) (primitive->dataLength)));
    }
    else
    {
        primitive->data = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlWapiUnicastTxPktReqSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWapiUnicastTxPktReq *primitive = (CsrWifiRouterCtrlWapiUnicastTxPktReq *) voidPrimitivePointer;
    CsrPmemFree(primitive->data);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlHipIndSizeof(void *msg)
{
    CsrWifiRouterCtrlHipInd *primitive = (CsrWifiRouterCtrlHipInd *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 12) */
    bufferSize += 2;                            /* CsrUint16 primitive->mlmeCommandLength */
    bufferSize += primitive->mlmeCommandLength; /* CsrUint8 primitive->mlmeCommand */
    bufferSize += 2;                            /* CsrUint16 primitive->dataRef1Length */
    bufferSize += primitive->dataRef1Length;    /* CsrUint8 primitive->dataRef1 */
    bufferSize += 2;                            /* CsrUint16 primitive->dataRef2Length */
    bufferSize += primitive->dataRef2Length;    /* CsrUint8 primitive->dataRef2 */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlHipIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlHipInd *primitive = (CsrWifiRouterCtrlHipInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->mlmeCommandLength);
    if (primitive->mlmeCommandLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->mlmeCommand, ((CsrUint16) (primitive->mlmeCommandLength)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataRef1Length);
    if (primitive->dataRef1Length)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->dataRef1, ((CsrUint16) (primitive->dataRef1Length)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataRef2Length);
    if (primitive->dataRef2Length)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->dataRef2, ((CsrUint16) (primitive->dataRef2Length)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlHipIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlHipInd *primitive = (CsrWifiRouterCtrlHipInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlHipInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->mlmeCommandLength, buffer, &offset);
    if (primitive->mlmeCommandLength)
    {
        primitive->mlmeCommand = (CsrUint8 *)CsrPmemAlloc(primitive->mlmeCommandLength);
        CsrMemCpyDes(primitive->mlmeCommand, buffer, &offset, ((CsrUint16) (primitive->mlmeCommandLength)));
    }
    else
    {
        primitive->mlmeCommand = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataRef1Length, buffer, &offset);
    if (primitive->dataRef1Length)
    {
        primitive->dataRef1 = (CsrUint8 *)CsrPmemAlloc(primitive->dataRef1Length);
        CsrMemCpyDes(primitive->dataRef1, buffer, &offset, ((CsrUint16) (primitive->dataRef1Length)));
    }
    else
    {
        primitive->dataRef1 = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataRef2Length, buffer, &offset);
    if (primitive->dataRef2Length)
    {
        primitive->dataRef2 = (CsrUint8 *)CsrPmemAlloc(primitive->dataRef2Length);
        CsrMemCpyDes(primitive->dataRef2, buffer, &offset, ((CsrUint16) (primitive->dataRef2Length)));
    }
    else
    {
        primitive->dataRef2 = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlHipIndSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlHipInd *primitive = (CsrWifiRouterCtrlHipInd *) voidPrimitivePointer;
    CsrPmemFree(primitive->mlmeCommand);
    CsrPmemFree(primitive->dataRef1);
    CsrPmemFree(primitive->dataRef2);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlMulticastAddressIndSizeof(void *msg)
{
    CsrWifiRouterCtrlMulticastAddressInd *primitive = (CsrWifiRouterCtrlMulticastAddressInd *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 15) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 1; /* CsrWifiRouterCtrlListAction primitive->action */
    bufferSize += 1; /* CsrUint8 primitive->setAddressesCount */
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->setAddressesCount; i1++)
        {
            bufferSize += 6; /* CsrUint8 primitive->setAddresses[i1].a[6] */
        }
    }
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlMulticastAddressIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlMulticastAddressInd *primitive = (CsrWifiRouterCtrlMulticastAddressInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->action);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->setAddressesCount);
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->setAddressesCount; i1++)
        {
            CsrMemCpySer(ptr, len, (const void *) primitive->setAddresses[i1].a, ((CsrUint16) (6)));
        }
    }
    return(ptr);
}


void* CsrWifiRouterCtrlMulticastAddressIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlMulticastAddressInd *primitive = (CsrWifiRouterCtrlMulticastAddressInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlMulticastAddressInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->action, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->setAddressesCount, buffer, &offset);
    primitive->setAddresses = NULL;
    if (primitive->setAddressesCount)
    {
        primitive->setAddresses = (CsrWifiMacAddress *)CsrPmemAlloc(sizeof(CsrWifiMacAddress) * primitive->setAddressesCount);
    }
    {
        CsrUint16 i1;
        for (i1 = 0; i1 < primitive->setAddressesCount; i1++)
        {
            CsrMemCpyDes(primitive->setAddresses[i1].a, buffer, &offset, ((CsrUint16) (6)));
        }
    }

    return primitive;
}


void CsrWifiRouterCtrlMulticastAddressIndSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlMulticastAddressInd *primitive = (CsrWifiRouterCtrlMulticastAddressInd *) voidPrimitivePointer;
    CsrPmemFree(primitive->setAddresses);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlPortConfigureCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 15) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    bufferSize += 6; /* CsrUint8 primitive->macAddress.a[6] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPortConfigureCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPortConfigureCfm *primitive = (CsrWifiRouterCtrlPortConfigureCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    CsrMemCpySer(ptr, len, (const void *) primitive->macAddress.a, ((CsrUint16) (6)));
    return(ptr);
}


void* CsrWifiRouterCtrlPortConfigureCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPortConfigureCfm *primitive = (CsrWifiRouterCtrlPortConfigureCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPortConfigureCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);
    CsrMemCpyDes(primitive->macAddress.a, buffer, &offset, ((CsrUint16) (6)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlSuspendIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 1; /* CsrBool primitive->hardSuspend */
    bufferSize += 1; /* CsrBool primitive->d3Suspend */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlSuspendIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlSuspendInd *primitive = (CsrWifiRouterCtrlSuspendInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->hardSuspend);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->d3Suspend);
    return(ptr);
}


void* CsrWifiRouterCtrlSuspendIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlSuspendInd *primitive = (CsrWifiRouterCtrlSuspendInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlSuspendInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->hardSuspend, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->d3Suspend, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlTclasAddCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTclasAddCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTclasAddCfm *primitive = (CsrWifiRouterCtrlTclasAddCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlTclasAddCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTclasAddCfm *primitive = (CsrWifiRouterCtrlTclasAddCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTclasAddCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlRawSdioDeinitialiseCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->result */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlRawSdioDeinitialiseCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlRawSdioDeinitialiseCfm *primitive = (CsrWifiRouterCtrlRawSdioDeinitialiseCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->result);
    return(ptr);
}


void* CsrWifiRouterCtrlRawSdioDeinitialiseCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlRawSdioDeinitialiseCfm *primitive = (CsrWifiRouterCtrlRawSdioDeinitialiseCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlRawSdioDeinitialiseCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->result, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlRawSdioInitialiseCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 39) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->result */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioByteRead primitive->byteRead */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioByteWrite primitive->byteWrite */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioFirmwareDownload primitive->firmwareDownload */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioReset primitive->reset */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioCoreDumpPrepare primitive->coreDumpPrepare */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioByteBlockRead primitive->byteBlockRead */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioGpRead16 primitive->gpRead16 */
    bufferSize += 4; /* CsrWifiRouterCtrlRawSdioGpWrite16 primitive->gpWrite16 */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlRawSdioInitialiseCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlRawSdioInitialiseCfm *primitive = (CsrWifiRouterCtrlRawSdioInitialiseCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->result);
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->byteRead */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->byteWrite */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->firmwareDownload */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->reset */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->coreDumpPrepare */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->byteBlockRead */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->gpRead16 */
    CsrUint32Ser(ptr, len, 0); /* Special for Function Pointers... primitive->gpWrite16 */
    return(ptr);
}


void* CsrWifiRouterCtrlRawSdioInitialiseCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlRawSdioInitialiseCfm *primitive = (CsrWifiRouterCtrlRawSdioInitialiseCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlRawSdioInitialiseCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->result, buffer, &offset);
    primitive->byteRead = NULL;         /* Special for Function Pointers... */
    offset += 4;
    primitive->byteWrite = NULL;        /* Special for Function Pointers... */
    offset += 4;
    primitive->firmwareDownload = NULL; /* Special for Function Pointers... */
    offset += 4;
    primitive->reset = NULL;            /* Special for Function Pointers... */
    offset += 4;
    primitive->coreDumpPrepare = NULL;  /* Special for Function Pointers... */
    offset += 4;
    primitive->byteBlockRead = NULL;    /* Special for Function Pointers... */
    offset += 4;
    primitive->gpRead16 = NULL;         /* Special for Function Pointers... */
    offset += 4;
    primitive->gpWrite16 = NULL;        /* Special for Function Pointers... */
    offset += 4;

    return primitive;
}


CsrSize CsrWifiRouterCtrlTclasDelCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTclasDelCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTclasDelCfm *primitive = (CsrWifiRouterCtrlTclasDelCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlTclasDelCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTclasDelCfm *primitive = (CsrWifiRouterCtrlTclasDelCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTclasDelCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlTrafficProtocolIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 17) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrWifiRouterCtrlTrafficPacketType primitive->packetType */
    bufferSize += 2; /* CsrWifiRouterCtrlProtocolDirection primitive->direction */
    bufferSize += 6; /* CsrUint8 primitive->srcAddress.a[6] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTrafficProtocolIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTrafficProtocolInd *primitive = (CsrWifiRouterCtrlTrafficProtocolInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->packetType);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->direction);
    CsrMemCpySer(ptr, len, (const void *) primitive->srcAddress.a, ((CsrUint16) (6)));
    return(ptr);
}


void* CsrWifiRouterCtrlTrafficProtocolIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTrafficProtocolInd *primitive = (CsrWifiRouterCtrlTrafficProtocolInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTrafficProtocolInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->packetType, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->direction, buffer, &offset);
    CsrMemCpyDes(primitive->srcAddress.a, buffer, &offset, ((CsrUint16) (6)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlTrafficSampleIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 38) */
    bufferSize += 2;  /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;  /* CsrUint16 primitive->interfaceTag */
    bufferSize += 4;  /* CsrUint32 primitive->stats.rxMeanRate */
    bufferSize += 4;  /* CsrUint32 primitive->stats.rxFramesNum */
    bufferSize += 4;  /* CsrUint32 primitive->stats.txFramesNum */
    bufferSize += 4;  /* CsrUint32 primitive->stats.rxBytesCount */
    bufferSize += 4;  /* CsrUint32 primitive->stats.txBytesCount */
    bufferSize += 11; /* CsrUint8 primitive->stats.intervals[11] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlTrafficSampleIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlTrafficSampleInd *primitive = (CsrWifiRouterCtrlTrafficSampleInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->stats.rxMeanRate);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->stats.rxFramesNum);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->stats.txFramesNum);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->stats.rxBytesCount);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->stats.txBytesCount);
    CsrMemCpySer(ptr, len, (const void *) primitive->stats.intervals, ((CsrUint16) (11)));
    return(ptr);
}


void* CsrWifiRouterCtrlTrafficSampleIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlTrafficSampleInd *primitive = (CsrWifiRouterCtrlTrafficSampleInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlTrafficSampleInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->stats.rxMeanRate, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->stats.rxFramesNum, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->stats.txFramesNum, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->stats.rxBytesCount, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->stats.txBytesCount, buffer, &offset);
    CsrMemCpyDes(primitive->stats.intervals, buffer, &offset, ((CsrUint16) (11)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlWifiOnIndSizeof(void *msg)
{
    CsrWifiRouterCtrlWifiOnInd *primitive = (CsrWifiRouterCtrlWifiOnInd *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 27) */
    bufferSize += 2;                                                                                    /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;                                                                                    /* CsrResult primitive->status */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->versions.chipId */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->versions.chipVersion */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->versions.firmwareBuild */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->versions.firmwareHip */
    bufferSize += (primitive->versions.routerBuild?CsrStrLen(primitive->versions.routerBuild) : 0) + 1; /* CsrCharString* primitive->versions.routerBuild (0 byte len + 1 for NULL Term) */
    bufferSize += 4;                                                                                    /* CsrUint32 primitive->versions.routerHip */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWifiOnIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWifiOnInd *primitive = (CsrWifiRouterCtrlWifiOnInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->versions.chipId);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->versions.chipVersion);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->versions.firmwareBuild);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->versions.firmwareHip);
    CsrCharStringSer(ptr, len, primitive->versions.routerBuild);
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->versions.routerHip);
    return(ptr);
}


void* CsrWifiRouterCtrlWifiOnIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWifiOnInd *primitive = (CsrWifiRouterCtrlWifiOnInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWifiOnInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->versions.chipId, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->versions.chipVersion, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->versions.firmwareBuild, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->versions.firmwareHip, buffer, &offset);
    CsrCharStringDes(&primitive->versions.routerBuild, buffer, &offset);
    CsrUint32Des((CsrUint32 *) &primitive->versions.routerHip, buffer, &offset);

    return primitive;
}


void CsrWifiRouterCtrlWifiOnIndSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWifiOnInd *primitive = (CsrWifiRouterCtrlWifiOnInd *) voidPrimitivePointer;
    CsrPmemFree(primitive->versions.routerBuild);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlWifiOnCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 7) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWifiOnCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWifiOnCfm *primitive = (CsrWifiRouterCtrlWifiOnCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlWifiOnCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWifiOnCfm *primitive = (CsrWifiRouterCtrlWifiOnCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWifiOnCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlM4ReadyToSendIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 13) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlM4ReadyToSendIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlM4ReadyToSendInd *primitive = (CsrWifiRouterCtrlM4ReadyToSendInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    return(ptr);
}


void* CsrWifiRouterCtrlM4ReadyToSendIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlM4ReadyToSendInd *primitive = (CsrWifiRouterCtrlM4ReadyToSendInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlM4ReadyToSendInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlM4TransmittedIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 15) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlM4TransmittedIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlM4TransmittedInd *primitive = (CsrWifiRouterCtrlM4TransmittedInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlM4TransmittedIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlM4TransmittedInd *primitive = (CsrWifiRouterCtrlM4TransmittedInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlM4TransmittedInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlMicFailureIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 14) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 1; /* CsrBool primitive->unicastPdu */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlMicFailureIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlMicFailureInd *primitive = (CsrWifiRouterCtrlMicFailureInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->unicastPdu);
    return(ptr);
}


void* CsrWifiRouterCtrlMicFailureIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlMicFailureInd *primitive = (CsrWifiRouterCtrlMicFailureInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlMicFailureInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->unicastPdu, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlConnectedIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 14) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 1; /* CsrWifiRouterCtrlPeerStatus primitive->peerStatus */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlConnectedIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlConnectedInd *primitive = (CsrWifiRouterCtrlConnectedInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->peerStatus);
    return(ptr);
}


void* CsrWifiRouterCtrlConnectedIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlConnectedInd *primitive = (CsrWifiRouterCtrlConnectedInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlConnectedInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint8Des((CsrUint8 *) &primitive->peerStatus, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerAddCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 19) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 4; /* CsrWifiRouterCtrlPeerRecordHandle primitive->peerRecordHandle */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerAddCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerAddCfm *primitive = (CsrWifiRouterCtrlPeerAddCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint32Ser(ptr, len, (CsrUint32) primitive->peerRecordHandle);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerAddCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerAddCfm *primitive = (CsrWifiRouterCtrlPeerAddCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerAddCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint32Des((CsrUint32 *) &primitive->peerRecordHandle, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerDelCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerDelCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerDelCfm *primitive = (CsrWifiRouterCtrlPeerDelCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerDelCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerDelCfm *primitive = (CsrWifiRouterCtrlPeerDelCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerDelCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlUnexpectedFrameIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 13) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlUnexpectedFrameIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlUnexpectedFrameInd *primitive = (CsrWifiRouterCtrlUnexpectedFrameInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    return(ptr);
}


void* CsrWifiRouterCtrlUnexpectedFrameIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlUnexpectedFrameInd *primitive = (CsrWifiRouterCtrlUnexpectedFrameInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlUnexpectedFrameInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlPeerUpdateCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlPeerUpdateCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlPeerUpdateCfm *primitive = (CsrWifiRouterCtrlPeerUpdateCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlPeerUpdateCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlPeerUpdateCfm *primitive = (CsrWifiRouterCtrlPeerUpdateCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlPeerUpdateCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlCapabilitiesCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->commandQueueSize */
    bufferSize += 2; /* CsrUint16 primitive->trafficQueueSize */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlCapabilitiesCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlCapabilitiesCfm *primitive = (CsrWifiRouterCtrlCapabilitiesCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->commandQueueSize);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->trafficQueueSize);
    return(ptr);
}


void* CsrWifiRouterCtrlCapabilitiesCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlCapabilitiesCfm *primitive = (CsrWifiRouterCtrlCapabilitiesCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlCapabilitiesCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->commandQueueSize, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->trafficQueueSize, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlBlockAckEnableCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlBlockAckEnableCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlBlockAckEnableCfm *primitive = (CsrWifiRouterCtrlBlockAckEnableCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlBlockAckEnableCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlBlockAckEnableCfm *primitive = (CsrWifiRouterCtrlBlockAckEnableCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlBlockAckEnableCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlBlockAckDisableCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 9) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlBlockAckDisableCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlBlockAckDisableCfm *primitive = (CsrWifiRouterCtrlBlockAckDisableCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlBlockAckDisableCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlBlockAckDisableCfm *primitive = (CsrWifiRouterCtrlBlockAckDisableCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlBlockAckDisableCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlBlockAckErrorIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 16) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 1; /* CsrWifiRouterCtrlTrafficStreamId primitive->trafficStreamID */
    bufferSize += 6; /* CsrUint8 primitive->peerMacAddress.a[6] */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlBlockAckErrorIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlBlockAckErrorInd *primitive = (CsrWifiRouterCtrlBlockAckErrorInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->trafficStreamID);
    CsrMemCpySer(ptr, len, (const void *) primitive->peerMacAddress.a, ((CsrUint16) (6)));
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlBlockAckErrorIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlBlockAckErrorInd *primitive = (CsrWifiRouterCtrlBlockAckErrorInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlBlockAckErrorInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->trafficStreamID, buffer, &offset);
    CsrMemCpyDes(primitive->peerMacAddress.a, buffer, &offset, ((CsrUint16) (6)));
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlStaInactiveIndSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 13) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 6; /* CsrUint8 primitive->staAddress.a[6] */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlStaInactiveIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlStaInactiveInd *primitive = (CsrWifiRouterCtrlStaInactiveInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrMemCpySer(ptr, len, (const void *) primitive->staAddress.a, ((CsrUint16) (6)));
    return(ptr);
}


void* CsrWifiRouterCtrlStaInactiveIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlStaInactiveInd *primitive = (CsrWifiRouterCtrlStaInactiveInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlStaInactiveInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrMemCpyDes(primitive->staAddress.a, buffer, &offset, ((CsrUint16) (6)));

    return primitive;
}


CsrSize CsrWifiRouterCtrlWapiRxMicCheckIndSizeof(void *msg)
{
    CsrWifiRouterCtrlWapiRxMicCheckInd *primitive = (CsrWifiRouterCtrlWapiRxMicCheckInd *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 13) */
    bufferSize += 2;                       /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;                       /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                       /* CsrUint16 primitive->signalLength */
    bufferSize += primitive->signalLength; /* CsrUint8 primitive->signal */
    bufferSize += 2;                       /* CsrUint16 primitive->dataLength */
    bufferSize += primitive->dataLength;   /* CsrUint8 primitive->data */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWapiRxMicCheckIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWapiRxMicCheckInd *primitive = (CsrWifiRouterCtrlWapiRxMicCheckInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->signalLength);
    if (primitive->signalLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->signal, ((CsrUint16) (primitive->signalLength)));
    }
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataLength);
    if (primitive->dataLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->data, ((CsrUint16) (primitive->dataLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlWapiRxMicCheckIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWapiRxMicCheckInd *primitive = (CsrWifiRouterCtrlWapiRxMicCheckInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWapiRxMicCheckInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->signalLength, buffer, &offset);
    if (primitive->signalLength)
    {
        primitive->signal = (CsrUint8 *)CsrPmemAlloc(primitive->signalLength);
        CsrMemCpyDes(primitive->signal, buffer, &offset, ((CsrUint16) (primitive->signalLength)));
    }
    else
    {
        primitive->signal = NULL;
    }
    CsrUint16Des((CsrUint16 *) &primitive->dataLength, buffer, &offset);
    if (primitive->dataLength)
    {
        primitive->data = (CsrUint8 *)CsrPmemAlloc(primitive->dataLength);
        CsrMemCpyDes(primitive->data, buffer, &offset, ((CsrUint16) (primitive->dataLength)));
    }
    else
    {
        primitive->data = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlWapiRxMicCheckIndSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWapiRxMicCheckInd *primitive = (CsrWifiRouterCtrlWapiRxMicCheckInd *) voidPrimitivePointer;
    CsrPmemFree(primitive->signal);
    CsrPmemFree(primitive->data);
    CsrPmemFree(primitive);
}


CsrSize CsrWifiRouterCtrlModeSetCfmSizeof(void *msg)
{
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2; /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2; /* CsrUint16 primitive->interfaceTag */
    bufferSize += 1; /* CsrWifiRouterCtrlMode primitive->mode */
    bufferSize += 2; /* CsrResult primitive->status */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlModeSetCfmSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlModeSetCfm *primitive = (CsrWifiRouterCtrlModeSetCfm *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint8Ser(ptr, len, (CsrUint8) primitive->mode);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->status);
    return(ptr);
}


void* CsrWifiRouterCtrlModeSetCfmDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlModeSetCfm *primitive = (CsrWifiRouterCtrlModeSetCfm *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlModeSetCfm));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint8Des((CsrUint8 *) &primitive->mode, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->status, buffer, &offset);

    return primitive;
}


CsrSize CsrWifiRouterCtrlWapiUnicastTxEncryptIndSizeof(void *msg)
{
    CsrWifiRouterCtrlWapiUnicastTxEncryptInd *primitive = (CsrWifiRouterCtrlWapiUnicastTxEncryptInd *) msg;
    CsrSize bufferSize = 2;

    /* Calculate the Size of the Serialised Data. Could be more efficient (Try 10) */
    bufferSize += 2;                     /* CsrWifiRouterCtrlRequestorInfo primitive->clientData */
    bufferSize += 2;                     /* CsrUint16 primitive->interfaceTag */
    bufferSize += 2;                     /* CsrUint16 primitive->dataLength */
    bufferSize += primitive->dataLength; /* CsrUint8 primitive->data */
    return bufferSize;
}


CsrUint8* CsrWifiRouterCtrlWapiUnicastTxEncryptIndSer(CsrUint8 *ptr, CsrSize *len, void *msg)
{
    CsrWifiRouterCtrlWapiUnicastTxEncryptInd *primitive = (CsrWifiRouterCtrlWapiUnicastTxEncryptInd *)msg;
    *len = 0;
    CsrUint16Ser(ptr, len, primitive->common.type);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->clientData);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->interfaceTag);
    CsrUint16Ser(ptr, len, (CsrUint16) primitive->dataLength);
    if (primitive->dataLength)
    {
        CsrMemCpySer(ptr, len, (const void *) primitive->data, ((CsrUint16) (primitive->dataLength)));
    }
    return(ptr);
}


void* CsrWifiRouterCtrlWapiUnicastTxEncryptIndDes(CsrUint8 *buffer, CsrSize length)
{
    CsrWifiRouterCtrlWapiUnicastTxEncryptInd *primitive = (CsrWifiRouterCtrlWapiUnicastTxEncryptInd *) CsrPmemAlloc(sizeof(CsrWifiRouterCtrlWapiUnicastTxEncryptInd));
    CsrSize offset;
    offset = 0;

    CsrUint16Des(&primitive->common.type, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->clientData, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->interfaceTag, buffer, &offset);
    CsrUint16Des((CsrUint16 *) &primitive->dataLength, buffer, &offset);
    if (primitive->dataLength)
    {
        primitive->data = (CsrUint8 *)CsrPmemAlloc(primitive->dataLength);
        CsrMemCpyDes(primitive->data, buffer, &offset, ((CsrUint16) (primitive->dataLength)));
    }
    else
    {
        primitive->data = NULL;
    }

    return primitive;
}


void CsrWifiRouterCtrlWapiUnicastTxEncryptIndSerFree(void *voidPrimitivePointer)
{
    CsrWifiRouterCtrlWapiUnicastTxEncryptInd *primitive = (CsrWifiRouterCtrlWapiUnicastTxEncryptInd *) voidPrimitivePointer;
    CsrPmemFree(primitive->data);
    CsrPmemFree(primitive);
}


