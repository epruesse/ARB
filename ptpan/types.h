/*!
 * \file types.h
 *
 * Standard ptpan type definitions
 *
 * \date 03.05.2003
 * \author Chris Hodges <hodges@in.tum.de>
 */

#ifndef PTPAN_TYPES_H
#define PTPAN_TYPES_H

#ifndef VOID
#define VOID            void
#endif

typedef void *APTR; // 32-bit untyped pointer
typedef long long LLONG; // signed 64-bit quantity
typedef unsigned long long ULLONG; // unsigned 64-bit quantity
typedef long LONG; // signed 32-bit quantity   (or 64-bit at x64)
typedef unsigned long ULONG; // unsigned 32-bit quantity (or 64-bit at x64)
typedef int INT; // signed 32-bit
typedef unsigned int UINT; // unsigned 32-bit
typedef unsigned long LONGBITS; // 32 bits manipulated individually
typedef short WORD; // signed 16-bit quantity
typedef unsigned short UWORD; // unsigned 16-bit quantity
typedef unsigned short WORDBITS; // 16 bits manipulated individually
typedef signed char BYTE; // signed 8-bit quantity
typedef unsigned char UBYTE; // unsigned 8-bit quantity
typedef unsigned char BYTEBITS; // 8 bits manipulated individually
typedef char * STRPTR; // string pointer (NULL terminated)
typedef const char * CONST_STRPTR; // string pointer (NULL terminated)
typedef short BOOL;

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef NULL
#define NULL    0L
#endif

#endif  // PTPAN_TYPES_H
