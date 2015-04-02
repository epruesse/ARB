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

typedef int GBQUARK; // -1 = any quark, 0 = no quark, >0 explicit quark

typedef float GBT_LEN;

class TreeNode;
class TreeRoot;

typedef unsigned int       GB_UINT4;                // 4 byte! @@@ use uint32_t ?
typedef const unsigned int GB_CUINT4;

typedef unsigned long GB_ULONG;
typedef const float   GB_CFLOAT;


enum GB_CB_TYPE {
    GB_CB_NONE        = 0,
    GB_CB_DELETE      = 1,
    GB_CB_CHANGED     = 2, // element or son of element changed
    GB_CB_SON_CREATED = 4, // new son created

    // convenience defs:
    GB_CB_ALL                    = GB_CB_DELETE|GB_CB_CHANGED|GB_CB_SON_CREATED,
    GB_CB_ALL_BUT_DELETE         = GB_CB_ALL&~GB_CB_DELETE,
    GB_CB_CHANGED_OR_DELETED     = GB_CB_CHANGED|GB_CB_DELETE,
    GB_CB_CHANGED_OR_SON_CREATED = GB_CB_CHANGED|GB_CB_SON_CREATED,
};

typedef void (*GB_CB)(GBDATA *, int *clientdata, GB_CB_TYPE gbtype);

enum GB_alignment_type {
    GB_AT_UNKNOWN,
    GB_AT_RNA,      // Nucleotide sequence (U)
    GB_AT_DNA,      // Nucleotide sequence (T)
    GB_AT_AA,       // AminoAcid
};

enum NewickFormat { // bit-values
    nSIMPLE  = 0,
    nLENGTH  = 1,
    nGROUP   = 2,
    nREMARK  = 4,
    nWRAP    = 8,

    nALL = nLENGTH|nGROUP|nREMARK,
};


#else
#error arbdb_base.h included twice
#endif // ARBDB_BASE_H
