//  ==================================================================== //
//                                                                       //
//    File      : bugex.h                                                //
//    Purpose   : Debugging code                                         //
//    Time-stamp: <Fri Apr/16/2004 12:28 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in April 2003            //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

// Description :
//
// when using the macros XXX_DUMP_VAL and XXX_DUMP_STR the program prints
// out information about the specified variables.
//
// This information is prefixed by 'FILE:LINENO: '
//
// The intention is to use the program as compile-command in emacs and follow
// the program flow with next-error / previous-error
//
// Note : When finished with debugging you should remove the #include "bugex.h"
// because it adds unneeded code to EVERY object file!

// --------------------------------------------------------------------------------
// This section is used by Ralf
// If you want to use the DUMP-Macros copy this section and personalize it!

#if defined(DEVEL_RALF)

#define BUGEX_DUMPS
#define BUGEX_MAX_STRING_PRINT 40
#define RALF_DUMP_VAL(var)  ALL_DUMP_VAL(var)
#define RALF_DUMP_STR(var)  ALL_DUMP_STR(var)
#define RALF_DUMP_PTR(var)  ALL_DUMP_PTR(var)
#define RALF_DUMP_MARK()    ALL_DUMP_MARK()

#else

#define RALF_DUMP_VAL(var)
#define RALF_DUMP_STR(var)
#define RALF_DUMP_MARK()

#endif

// --------------------------------------------------------------------------------
// This section is used by Yadhu

#if defined(DEVEL_YADHU)

#define BUGEX_DUMPS
#define BUGEX_MAX_STRING_PRINT 40
#define YADHU_DUMP_VAL(var)  ALL_DUMP_VAL(var)
#define YADHU_DUMP_STR(var)  ALL_DUMP_STR(var)
#define YADHU_DUMP_PTR(var)  ALL_DUMP_PTR(var)
#define YADHU_DUMP_MARK()    ALL_DUMP_MARK()

#else

#define YADHU_DUMP_VAL(var)
#define YADHU_DUMP_STR(var)
#define YADHU_DUMP_MARK()

#endif

// --------------------------------------------------------------------------------
// Definition of needed macros/functions :

#if defined(BUGEX_DUMPS)

#ifndef	_CTYPE_H
#include <ctype.h>
#endif

// Do NOT use the following macros!!!
#define ALL_DUMP_VAL(var) bugex_dump_value(&var, #var, sizeof(var), __FILE__, __LINE__)
#define ALL_DUMP_STR(var) bugex_dump_string(&var, #var, __FILE__, __LINE__)
#define ALL_DUMP_PTR(var) bugex_dump_pointer(&var, #var, __FILE__, __LINE__)
#define ALL_DUMP_MARK()   bugex_dump_mark(__FILE__, __LINE__)

static void bugex_dump_mark(const char *file, size_t lineno) {
    fprintf(stderr, "%s:%u: ------------------------------ MARK\n", file, lineno);
    fflush(stderr);
}

static void bugex_printString(const char *str, size_t len) {
    while (len--) {
        unsigned char c = (unsigned char)*str++;
        if (isprint(c)) {
            fputc(c, stderr);
        }
        else {
            switch (c) {
                case '\n': fputs("\\n", stderr); break;
                case '\t': fputs("\\t", stderr); break;
                default : fprintf(stderr, "\\%i", (int)c); break;
            }
        }
    }
}

static void bugex_dump_pointer(void *somePtr, const char *someName, const char *file, size_t lineno) {
    fprintf(stderr, "%s:%u: %s: %p -> %p\n", file, lineno, someName, somePtr, *(void**)somePtr);
    fflush(stderr);
}

static void bugex_dump_string(void *strPtr, const char *strName, const char *file, size_t lineno) {
    const char *s = *((const char **)strPtr);

    fprintf(stderr, "%s:%u: ", file, lineno);

    if (s == 0) {               // NULL pointer
        fprintf(stderr, "%s is NULL (&=%p)\n", strName, strPtr);
    }
    else {
        size_t len = strlen(s);

        fprintf(stderr, "%s = \"", strName);
        if (len <= BUGEX_MAX_STRING_PRINT) { // short strings
            bugex_printString(s, len);
        }
        else {
            bugex_printString(s, BUGEX_MAX_STRING_PRINT/2);
            fprintf(stderr, "\" .. \"");
            bugex_printString(s+len-BUGEX_MAX_STRING_PRINT/2, BUGEX_MAX_STRING_PRINT/2);
        }
        fprintf(stderr, "\" (len=%u, &=%p -> %p)\n", len, strPtr, s);
    }
    fflush(stderr);
}


#define BUGEX_VALSIZES 4
static unsigned short bugex_valsize[] = { sizeof(char), sizeof(short), sizeof(int), sizeof(long), 0 };
enum bugex_valtype { BUGEX_CHAR, BUGEX_SHORT, BUGEX_INT, BUGEX_LONG, BUGEX_UNKNOWN };

static void bugex_dump_value(void *valuePtr, const char *valueName, size_t size, const char *file, size_t lineno) {
    int                vs;
    enum bugex_valtype type = BUGEX_UNKNOWN;

    if (size == bugex_valsize[BUGEX_INT]) {
        type = BUGEX_INT; // prefer int
    }
    else {
        for (vs = 0; vs < BUGEX_VALSIZES; ++vs) {
            if (size == bugex_valsize[vs]) {
                type = (enum bugex_valtype)vs;
            }
        }
    }

    fprintf(stderr, "%s:%u: ", file, lineno);

    switch (type) {
        case BUGEX_CHAR: {
            unsigned char c = *(unsigned char*)valuePtr;
            int           i = (int)(signed char)c;

            if (i<0) {
                fprintf(stderr, "(char)  %s = '%c' = %i = %u (&=%p)", valueName, c, i, (unsigned int)c, valuePtr);
            }
            else {
                fprintf(stderr, "(char)  %s = '%c' = %i (&=%p)", valueName, c, i, valuePtr);
            }
            break;
        }
        case BUGEX_SHORT: {
            unsigned short s  = *(unsigned short *)valuePtr;
            signed short   ss = (signed short)s;
            if (ss<0) {
                fprintf(stderr, "(short) %s = %i = %u (&=%p)", valueName, (int)ss, (unsigned)s, valuePtr);
            }
            else {
                fprintf(stderr, "(short) %s = %i (&=%p)", valueName, (int)ss, valuePtr);
            }
            break;
        }
        case BUGEX_INT: {
            unsigned int u = *(unsigned int *)valuePtr;
            int          i = (int)u;

            if (i<0) {
                fprintf(stderr, "(int)   %s = %i = %u (&=%p)", valueName, i, u, valuePtr);
            }
            else {
                fprintf(stderr, "(int)   %s = %i (&=%p)", valueName, i, valuePtr);
            }
            break;
        }
        case BUGEX_LONG: {
            unsigned long l = *(unsigned long *)valuePtr;
            fprintf(stderr, "(long)  %s = %li = %lu (&=%p)", valueName, (signed long)l, (unsigned long)l, valuePtr);
            break;
        }
        default :  {
            fprintf(stderr, "value '%s' has unknown size (use cast operator!)", valueName);
            break;
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

#endif // BUGEX_DUMPS
