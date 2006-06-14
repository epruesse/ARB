#ifndef __DEFINES_H
#define __DEFINES_H

#ifndef __STDIO_H
#include <stdio.h>
#endif

/* /-----------\ */
/* |  Defines  | */
/* \-----------/ */

#ifdef __cplusplus

    #define __PROTOTYPEN__ extern "C" {
    #define __PROTOENDE__  }

#else

    #define __PROTOTYPEN__
    #define __PROTOENDE__

#endif

#define outOfMemory()   def_outOfMemory(__FILE__, __LINE__)
#define assert(c)       do { static int assCnt; assCnt++; if (!(c)) def_assert(#c, __FILE__, __LINE__, assCnt); } while(0)

/* /---------\ */
/* |  Typen  | */
/* \---------/ */

typedef char          *str;
typedef const char    *cstr;

/* /----------------\ */
/* |  Hilfroutinen  | */
/* \----------------/ */

__PROTOTYPEN__

    void error           (cstr message);
    void errorf          (cstr format, ...) __attribute__((format(printf, 1, 2)));

    void warning         (cstr message);
    void warningf        (cstr format, ...) __attribute__((format(printf, 1, 2)));

    void def_outOfMemory (cstr source, int lineno);
    void def_assert      (cstr whatFailed, cstr source, int lineno, int cnt);

__PROTOENDE__


#endif
