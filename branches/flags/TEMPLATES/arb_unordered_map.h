// ================================================================ //
//                                                                  //
//   File      : arb_unordered_map.h                                //
//   Purpose   : compatibility wrapper for unordered_map            //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_UNORDERED_MAP_H
#define ARB_UNORDERED_MAP_H

// this header can be eliminated when arb EXPECTS Cxx11

#ifdef DARWIN
// fallback to std:map on DARWIN
// workaround compilation problems on Maverick (osx)
# include <map>
# define arb_unordered_map  std::map
#else
# ifdef Cxx11
#  include <unordered_map>
#  define arb_unordered_map std::unordered_map
# else
#  include <tr1/unordered_map>
#  define arb_unordered_map std::tr1::unordered_map
# endif
#endif

#else
#error arb_unordered_map.h included twice
#endif // ARB_UNORDERED_MAP_H
