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

#define NOT4PERL                                    // function definitions starting with NOT4PERL are not included into the ARB-perl-interface

typedef const char *GB_CSTR;                        // read-only
typedef const char *GB_ERROR;                       // memory managed by ARBDB
typedef char       *GB_BUFFER;                      // points to a piece of mem (writeable, but don't free!)*/
typedef const char *GB_CBUFFER;                     // points to a piece of mem (readable only)*/

struct GBDATA;
struct GB_HASH;

// @@@ TODO: GB_BOOL/GB_TRUE/GB_FALSE should be replaced by bool/true/false in code 
typedef bool GB_BOOL;
const bool   GB_FALSE = false;
const bool   GB_TRUE  = true;

enum GB_CB_TYPE {
    GB_CB_NONE        = 0,
    GB_CB_DELETE      = 1,
    GB_CB_CHANGED     = 2, // element or son of element changed
    GB_CB_SON_CREATED = 4, // new son created
    GB_CB_ALL         = 7
};

#ifndef DUPSTR_H
#include <dupstr.h>
#endif
#ifndef AD_K_PROT_H
#include <ad_k_prot.h>
#endif

#else
#error arbdb_base.h included twice
#endif // ARBDB_BASE_H
