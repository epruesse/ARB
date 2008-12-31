// ================================================================ //
//                                                                  //
//   File      : arbdb_base.h                                       //
//   Purpose   : most minimal ARBDB interface                       //
//               provide functions/types needed for arb_assert.h    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2008   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARBDB_BASE_H
#define ARBDB_BASE_H

#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

typedef const char *GB_CSTR;    /* this is read-only! */
typedef const char *GB_ERROR;   /* memory management is controlled by the ARBDB lib */
typedef char       *GB_BUFFER;  /* points to a piece of mem (writeable, but don't free!)*/
typedef const char *GB_CBUFFER; /* points to a piece of mem (readable only)*/

typedef enum { GB_FALSE = 0 , GB_TRUE = 1 } GB_BOOL;

#ifndef P_
#define P_(s) s
#endif

#ifndef AD_K_PROT_H
#include <ad_k_prot.h>
#endif

#else
#error arbdb_base.h included twice
#endif // ARBDB_BASE_H
