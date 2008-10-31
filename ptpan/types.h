/************************************************************************
 Standard type definitions
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 03.06.03
 ************************************************************************/

#ifndef TYPES_H
#define TYPES_H
#ifndef VOID
#define VOID            void
#endif

typedef void           *APTR;       /* 32-bit untyped pointer */
typedef long long       LLONG;      /* signed 64-bit quantity */
typedef unsigned long   ULLONG;     /* unsigned 64-bit quantity */
typedef long            LONG;       /* signed 32-bit quantity */
typedef unsigned long   ULONG;      /* unsigned 32-bit quantity */
typedef unsigned long   LONGBITS;   /* 32 bits manipulated individually */
typedef short           WORD;       /* signed 16-bit quantity */
typedef unsigned short  UWORD;      /* unsigned 16-bit quantity */
typedef unsigned short  WORDBITS;   /* 16 bits manipulated individually */
typedef signed char     BYTE;       /* signed 8-bit quantity */
typedef unsigned char   UBYTE;      /* unsigned 8-bit quantity */
typedef unsigned char   BYTEBITS;   /* 8 bits manipulated individually */
typedef char           *STRPTR;     /* string pointer (NULL terminated) */
typedef short           BOOL;

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
#ifndef NULL
#define NULL            0L
#endif
#endif  /* TYPES_H */
