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

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif

#define NOT4PERL                                    // function definitions starting with NOT4PERL are not included into the ARB-perl-interface

typedef const char *GB_CSTR;                        // read-only
typedef char       *GB_BUFFER;                      // points to a piece of mem (writeable, but don't free!)*/
typedef const char *GB_CBUFFER;                     // points to a piece of mem (readable only)*/

struct GBDATA;
struct GB_HASH;

typedef int GBQUARK;

typedef float GBT_LEN;
struct        GBT_TREE;

typedef unsigned int       GB_UINT4;                // 4 byte! @@@ use uint32_t ?
typedef const unsigned int GB_CUINT4;

typedef unsigned long GB_ULONG;
typedef const float   GB_CFLOAT;


enum GB_CB_TYPE {
    GB_CB_NONE        = 0,
    GB_CB_DELETE      = 1,
    GB_CB_CHANGED     = 2, // element or son of element changed
    GB_CB_SON_CREATED = 4, // new son created
    GB_CB_ALL         = 7
};

typedef void (*GB_CB)(GBDATA *, int *clientdata, GB_CB_TYPE gbtype);

enum GB_alignment_type {
    GB_AT_UNKNOWN,
    GB_AT_RNA,      // Nucleotide sequence (U)
    GB_AT_DNA,      // Nucleotide sequence (T)
    GB_AT_AA,       // AminoAcid
};

#else
#error arbdb_base.h included twice
#endif // ARBDB_BASE_H