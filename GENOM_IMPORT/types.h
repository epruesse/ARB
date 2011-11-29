// ================================================================ //
//                                                                  //
//   File      : types.h                                            //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef TYPES_H
#define TYPES_H

#ifndef DEFS_H
#include "defs.h"
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

using std::vector;  // @@@ do not use using decls in header w/o namespace
using std::map;
using std::set;

typedef set<string>         stringSet;
typedef map<string, string> stringMap;
typedef vector<string>      stringVector;

DEFINE_ITERATORS(string);
DEFINE_ITERATORS(stringSet);
DEFINE_ITERATORS(stringMap);
DEFINE_ITERATORS(stringVector);

#else
#error types.h included twice
#endif // TYPES_H

