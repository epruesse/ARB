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
#define Cxx11 // use Cxx11 to insert conditional sections using full C++11
#endif

// -------------------
//      constexpr

// allows static member initialisation in class definition:
#if defined(Cxx11)
# define CONSTEXPR constexpr
#else // !defined(Cxx11)
# define CONSTEXPR const
#endif

#else
#error cxxforward.h included twice
#endif // CXXFORWARD_H
