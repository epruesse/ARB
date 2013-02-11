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

#if (GCC_VERSION_CODE >= 408)
#define Cxx11 1 // use Cxx11 to insert conditional sections using full C++11
#endif

// -------------------
//      constexpr

// allows static member initialisation in class definition:
#if (GCC_VERSION_CODE >= 406) // override is supported starting with gcc 4.6
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
