#ifndef DEFINES_H
#define DEFINES_H

#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

// ----------------
//      Defines

#define outOfMemory()   def_outOfMemory(__FILE__, __LINE__)
#define assert(c)       do { static int assCnt; assCnt++; if (!(c)) def_assert(#c, __FILE__, __LINE__, assCnt); } while (0)

// --------------
//      Typen

typedef char          *str;
typedef const char    *cstr;

// ---------------------
//      Hilfroutinen

#ifdef __cplusplus
extern "C" {
#endif

    void error           (cstr message);
    void errorf          (cstr format, ...) __ATTR__FORMAT(1);

    void warning         (cstr message);
    void warningf        (cstr format, ...) __ATTR__FORMAT(1);

    void def_outOfMemory (cstr source, int lineno);
    void def_assert      (cstr whatFailed, cstr source, int lineno, int cnt);

#ifdef __cplusplus
}
#endif


#endif
