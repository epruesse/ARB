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

/* --------------------------------------------------------------------------------
 * The following function handle char*'s, which either own a heap copy or are NULL.
 *
 * freeset:  assigns a heap-copy to a variable (variable is automatically freed)
 * freedup:  similar to freeset, but strdup's the rhs-expression
 * reassign: similar to freeset, but rhs must be variable and will be set to NULL
 * nulldup:  like strdup, but pass-through NULL
 *
 * Note: freeset, freedup and reassign may safely use the changed variable in the rhs-expression!
 *
 * @@@ the complete section could go into a separate header,
 * but it makes no sense atm, cause we need GB_strdup for C compilation
 * (using a macro would evaluate 'str' in nulldup twice - which is not ok)
 *
 */

 
#define freeset(var, heapCopy) do { typeof(var) freesetvar = (heapCopy); free(var); (var) = freesetvar; } while(0)
// Note: expression 'heapCopy' may contain 'var' 

#ifdef __cplusplus

#ifndef _CPP_CSTRING
#include <cstring>
#endif
#ifndef _CPP_CSTDLIB
#include <cstdlib>
#endif

inline char *nulldup(const char *str)                           { return str ? strdup(str) : NULL; } // this does the same as GB_strdup
inline void freedup(char *& strvar, const char *no_heapcopy)    { char *tmp_copy = nulldup(no_heapcopy); free(strvar); strvar = tmp_copy; }
inline void reassign(char *& dstvar, char *& srcvar)            { freeset(dstvar, srcvar); srcvar = NULL; }

#else

#error please compile as C++

#define nulldup(str)        GB_strdup(str)
#define freedup(var,str)    freeset(var, nulldup(str))
#define reassign(dvar,svar) do { freeset(dvar, svar); (svar) = NULL; } while(0)

#endif

/* -------------------------------------------------------------------------------- */

#ifndef AD_K_PROT_H
#include <ad_k_prot.h>
#endif

#else
#error arbdb_base.h included twice
#endif // ARBDB_BASE_H
