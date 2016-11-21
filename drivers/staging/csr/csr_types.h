#ifndef CSR_TYPES_H__
#define CSR_TYPES_H__
/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2010
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/

#ifdef __KERNEL__
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/string.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#undef  FALSE
#define FALSE (0)

#undef  TRUE
#define TRUE (1)

/* Basic types */
typedef size_t CsrSize;         /* Return type of sizeof (ISO/IEC 9899:1990 7.1.6) */
typedef ptrdiff_t CsrPtrdiff;   /* Type of the result of subtracting two pointers (ISO/IEC 9899:1990 7.1.6) */
typedef uintptr_t CsrUintptr;   /* Unsigned integer large enough to hold any pointer (ISO/IEC 9899:1999 7.18.1.4) */
#ifdef __KERNEL__
typedef ptrdiff_t CsrIntptr;    /* intptr_t is not defined in kernel. Use the equivalent ptrdiff_t. */
#else
typedef intptr_t CsrIntptr;     /* Signed integer large enough to hold any pointer (ISO/IEC 9899:1999 7.18.1.4) */
#endif

/* Unsigned fixed width types */
typedef uint8_t CsrUint8;
typedef uint16_t CsrUint16;
typedef uint32_t CsrUint32;

/* Signed fixed width types */
typedef int8_t CsrInt8;
typedef int16_t CsrInt16;
typedef int32_t CsrInt32;

/* Boolean */
typedef CsrUint8 CsrBool;

/* String types */
typedef char CsrCharString;
typedef CsrUint8 CsrUtf8String;
typedef CsrUint16 CsrUtf16String;   /* 16-bit UTF16 strings */
typedef CsrUint32 CsrUint24;

/*
 * 64-bit integers
 *
 * Note: If a given compiler does not support 64-bit types, it is
 * OK to omit these definitions;  32-bit versions of the code using
 * these types may be available.  Consult the relevant documentation
 * or the customer support group for information on this.
 */
#define CSR_HAVE_64_BIT_INTEGERS
typedef uint64_t CsrUint64;
typedef int64_t CsrInt64;

/*
 * Floating point
 *
 * Note: If a given compiler does not support floating point, it is
 * OK to omit these definitions;  alternative versions of the code using
 * these types may be available.  Consult the relevant documentation
 * or the customer support group for information on this.
 */
#define CSR_HAVE_FLOATING_POINT
typedef float CsrFloat;
typedef double CsrDouble;

#ifdef __cplusplus
}
#endif

#endif
