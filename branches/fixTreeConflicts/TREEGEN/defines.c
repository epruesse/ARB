#include "defines.h"
#include <stdlib.h>
#include <stdarg.h>

#define MAXERRLEN 500

void error(cstr message)
{
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}
void warning(cstr message)
{
    fprintf(stderr, "Warning: %s\n", message);
}
void errorf(cstr format, ...)
{
    char    errBuf[MAXERRLEN];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(errBuf, format, argptr);
    va_end(argptr);

    error(errBuf);
}
void warningf(cstr format, ...)
{
    char    warnBuf[MAXERRLEN];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(warnBuf, format, argptr);
    va_end(argptr);

    warning(warnBuf);
}
void def_outOfMemory(cstr source, int lineno)
{
    errorf("Out of memory (in %s, #%i)", source, lineno);
}
void def_assert(cstr whatFailed, cstr source, int lineno, int cnt)
{
    errorf("Assertion (%s) failed in %s (Line: %i ; Pass: %i)\n",
           whatFailed, source, lineno, cnt);
}
