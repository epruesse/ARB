// ================================================================ //
//                                                                  //
//   File      : gccver.h                                           //
//   Purpose   : central place for gcc-version-dependent defs       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef GCCVER_H
#define GCCVER_H

#ifndef __GNUC__
# error You have to use the gnu compiler!
#endif

#define GCC_VERSION_CODE    ((__GNUC__ * 100) + __GNUC_MINOR__)
#define GCC_PATCHLEVEL_CODE ((GCC_VERSION_CODE * 100) + __GNUC_PATCHLEVEL__)

// check required gcc version:
#if (GCC_VERSION_CODE >= 403) // gcc 4.3 or newer
# define GCC_VERSION_OK
#else
# if (GCC_VERSION_CODE >= 402) // gcc 4.2 is ok for clang
#  ifdef __clang__
#   define GCC_VERSION_OK
#  endif
# endif
#endif

#ifndef GCC_VERSION_OK
# error Wrong compiler version (need at least gcc 4.3 or clang 4.2)
#endif

#else
#error gccver.h included twice
#endif // GCCVER_H
