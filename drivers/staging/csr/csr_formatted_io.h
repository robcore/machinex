#ifndef CSR_FORMATTED_IO_H__
#define CSR_FORMATTED_IO_H__
/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2010
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

CsrInt32 CsrSnprintf(CsrCharString *dest, CsrSize n, const CsrCharString *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
