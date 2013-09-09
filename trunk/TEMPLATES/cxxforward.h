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

// C++11 is enabled starting with gcc 4.7 in ../Makefile@USE_Cxx11
//
// Full support for C++11 is available starting with gcc 4.8.
// Use #ifdef Cxx11 to insert conditional sections using full C++11
#if (GCC_VERSION_CODE >= 408)
#define Cxx11 1

#if defined(DEBUG)
static_assert(true, "This fails to compile, if C++11 is available but unused");
#endif

#endif

// -------------------
//      constexpr

// allows static member initialisation in class definition:
#if (GCC_VERSION_CODE >= 407) // constexpr is supported starting with gcc 4.6. We use it starting with 4.7
# define CONSTEXPR constexpr
#else
# define CONSTEXPR const
#endif

// ------------------
//      override

// allows to protect overloading functions against signature changes of overload functions 
#if (GCC_VERSION_CODE >= 407) // override is supported starting with gcc 4.7
# define OVERRIDE override
#else
# define OVERRIDE
#endif

#else
#error cxxforward.h included twice
#endif // CXXFORWARD_H
