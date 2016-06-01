// ==================================================================== //
//                                                                      //
//   File      : attributes.h                                           //
//   Purpose   : declare attribute macros                               //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in June 2005             //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#ifndef GCCVER_H
#include "gccver.h"
#endif

// ------------------------------------------------------------
// short description of attributes defined:
//
// __ATTR__FORMAT(p)   used for printf-like functions. 'p' is the position of the format string, args follow directly
// __ATTR__VFORMAT(p)  used for vprintf-like functions. 'p' is the position of the format string, args are NOT checked
// __ATTR__SENTINEL    used for function that expect a parameter list terminated by NULL
// __ATTR__NORETURN    used for functions which do NEVER return
// __ATTR__DEPRECATED  used for deprecated functions (useful for redesign)
// __ATTR__USERESULT   warn if result of function is unused
//
// __ATTR__FORMAT_MEMBER(p)     same as __ATTR__FORMAT for member functions
// __ATTR__VFORMAT_MEMBER(p)    same as __ATTR__VFORMAT for member functions
//
// ------------------------------------------------------------

#if (GCC_PATCHLEVEL_CODE >= 40502) // gcc 4.5.2 and higher
# define __ATTR__DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif

#if (GCC_VERSION_CODE >= 409) // gcc 4.9.x or higher
# define __ATTR__DONT_SANITIZE __attribute__((no_sanitize_address))
#endif

// ------------------------------------------------------------
// helper macro to declare attributes for function-pointers

#define FUNCTION_TYPE_ATTR(x) x

// ------------------------------------------------------------
// valid for any gcc above 4.3

#ifndef __ATTR__DEPRECATED
# define __ATTR__DEPRECATED(reason) __attribute__((deprecated))
#endif

#define __ATTR__NORETURN  __attribute__((noreturn))
#define __ATTR__SENTINEL  __attribute__((sentinel))
#define __ATTR__USERESULT __attribute__((warn_unused_result))

#define __ATTR__FORMAT(pos)         __attribute__((format(__printf__, pos, (pos)+1)))
#define __ATTR__VFORMAT(pos)        __attribute__((format(__printf__, pos, 0)))
#define __ATTR__FORMAT_MEMBER(pos)  __attribute__((format(__printf__, (pos)+1, (pos)+2)))
#define __ATTR__VFORMAT_MEMBER(pos) __attribute__((format(__printf__, (pos)+1, 0)))
// when used for member functions, start with pos+1 (pos = 1 seems to be the this-pointer!?)

// ------------------------------------------------------------
//  temporary disable

// #undef __ATTR__DEPRECATED
// #undef __ATTR__USERESULT
// #undef __ATTR__SENTINEL

// ------------------------------------------------------------
// now define undefined attributes empty :

#ifndef __ATTR__SENTINEL
# define __ATTR__SENTINEL
#endif
#ifndef __ATTR__USERESULT
# define __ATTR__USERESULT
#endif
#ifndef __ATTR__DEPRECATED
# define __ATTR__DEPRECATED(reason)
#endif
#ifndef __ATTR__DONT_SANITIZE
# define __ATTR__DONT_SANITIZE
#endif

// ------------------------------------------------------------
// quickly disable attributes

#if defined(WARN_TODO)
#define __ATTR__USERESULT_TODO           __ATTR__USERESULT
#define __ATTR__DEPRECATED_TODO(reason)  __ATTR__DEPRECATED(reason)
#else
#define __ATTR__USERESULT_TODO
#define __ATTR__DEPRECATED_TODO(reason)
#endif

// no warning now, but it's planned to get deprecated
// -> just works as annotation and placeholder for __ATTR__DEPRECATED or __ATTR__DEPRECATED_TODO
#define __ATTR__DEPRECATED_LATER(reason)

// ------------------------------------------------------------
// casting result to void does not help when a function is
// declared with __ATTR__USERESULT. Use

#ifdef __cplusplus
template <typename T> void IGNORE_RESULT(const T&) {}
#endif

#else
#error attributes.h included twice
#endif // ATTRIBUTES_H

