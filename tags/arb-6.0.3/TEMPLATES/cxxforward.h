// ================================================================ //
//                                                                  //
//   File      : cxxforward.h                                       //
//   Purpose   : macros for forward usage of C++11 features         //
//               w/o loosing compatibility to C++03 compilers       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef CXXFORWARD_H
#define CXXFORWARD_H

#ifndef GCCVER_H
#include "gccver.h"
#endif

#if defined(__cplusplus)
# if (GCC_VERSION_CODE >= 407)
#  if (__cplusplus == 199711L)
#  else
#   if (__cplusplus == 201103L)
#    define ARB_ENABLE_Cxx11_FEATURES
#   else
#    error Unknown C++ standard defined in __cplusplus
#   endif
#  endif
# endif
#else
# warning C compilation includes cxxforward.h
#endif


#ifdef ARB_ENABLE_Cxx11_FEATURES

// C++11 is enabled starting with gcc 4.7 in ../Makefile@USE_Cxx11
//
// Full support for C++11 is available starting with gcc 4.8.
// Use #ifdef Cxx11 to insert conditional sections using full C++11
# if (GCC_VERSION_CODE >= 408)
#  define Cxx11 1
# endif

// allows static member initialisation in class definition:
# define CONSTEXPR        constexpr
# define CONSTEXPR_RETURN constexpr

// allows to protect overloading functions against signature changes of overload functions:
# define OVERRIDE override

#else
// backward (non C++11) compatibility defines:
# define CONSTEXPR        const
# define CONSTEXPR_RETURN
# define OVERRIDE

#endif

#else
#error cxxforward.h included twice
#endif // CXXFORWARD_H
