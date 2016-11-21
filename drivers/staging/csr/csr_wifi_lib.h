/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2011
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/
#ifndef CSR_WIFI_LIB_H__
#define CSR_WIFI_LIB_H__

#include "csr_types.h"
#include "csr_wifi_fsm_event.h"


#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*
 *  CsrWifiFsmEventInit
 *
 *  DESCRIPTION
 *      Macro to initialise the members of a CsrWifiFsmEvent.
 *----------------------------------------------------------------------------*/
#define CsrWifiFsmEventInit(evt, p_primtype, p_msgtype, p_dst, p_src) \
    (evt)->primtype = p_primtype; \
    (evt)->type = p_msgtype; \
    (evt)->destination = p_dst; \
    (evt)->source = p_src


/*----------------------------------------------------------------------------*
 *  CsrWifiEvent_struct
 *
 *  DESCRIPTION
 *      Generic message creator.
 *      Allocates and fills in a message with the signature CsrWifiEvent
 *
 *----------------------------------------------------------------------------*/
CsrWifiFsmEvent* CsrWifiEvent_struct(CsrUint16 primtype, CsrUint16 msgtype, CsrSchedQid dst, CsrSchedQid src);

typedef struct
{
    CsrWifiFsmEvent common;
    CsrUint8        value;
} CsrWifiEventCsrUint8;

/*----------------------------------------------------------------------------*
 *  CsrWifiEventCsrUint8_struct
 *
 *  DESCRIPTION
 *      Generic message creator.
 *      Allocates and fills in a message with the signature CsrWifiEventCsrUint8
 *
 *----------------------------------------------------------------------------*/
CsrWifiEventCsrUint8* CsrWifiEventCsrUint8_struct(CsrUint16 primtype, CsrUint16 msgtype, CsrSchedQid dst, CsrSchedQid src, CsrUint8 value);

typedef struct
{
    CsrWifiFsmEvent common;
    CsrUint16       value;
} CsrWifiEventCsrUint16;

/*----------------------------------------------------------------------------*
 *  CsrWifiEventCsrUint16_struct
 *
 *  DESCRIPTION
 *      Generic message creator.
 *      Allocates and fills in a message with the signature CsrWifiEventCsrUint16
 *
 *----------------------------------------------------------------------------*/
CsrWifiEventCsrUint16* CsrWifiEventCsrUint16_struct(CsrUint16 primtype, CsrUint16 msgtype, CsrSchedQid dst, CsrSchedQid src, CsrUint16 value);

typedef struct
{
    CsrWifiFsmEvent common;
    CsrUint32       value;
} CsrWifiEventCsrUint32;

/*----------------------------------------------------------------------------*
 *  CsrWifiEventCsrUint32_struct
 *
 *  DESCRIPTION
 *      Generic message creator.
 *      Allocates and fills in a message with the signature CsrWifiEventCsrUint32
 *
 *----------------------------------------------------------------------------*/
CsrWifiEventCsrUint32* CsrWifiEventCsrUint32_struct(CsrUint16 primtype, CsrUint16 msgtype, CsrSchedQid dst, CsrSchedQid src, CsrUint32 value);

typedef struct
{
    CsrWifiFsmEvent common;
    CsrUint16       value16;
    CsrUint8        value8;
} CsrWifiEventCsrUint16CsrUint8;

/*----------------------------------------------------------------------------*
 *  CsrWifiEventCsrUint16CsrUint8_struct
 *
 *  DESCRIPTION
 *      Generic message creator.
 *      Allocates and fills in a message with the signature CsrWifiEventCsrUint16CsrUint8
 *
 *----------------------------------------------------------------------------*/
CsrWifiEventCsrUint16CsrUint8* CsrWifiEventCsrUint16CsrUint8_struct(CsrUint16 primtype, CsrUint16 msgtype, CsrSchedQid dst, CsrSchedQid src, CsrUint16 value16, CsrUint8 value8);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_LIB_H__ */
