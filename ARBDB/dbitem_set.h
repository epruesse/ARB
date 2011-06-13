// ============================================================ //
//                                                              //
//   File      : dbitem_set.h                                   //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef DBITEM_SET_H
#define DBITEM_SET_H

#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef ARBDB_BASE_H
#include "arbdb_base.h"
#endif

typedef std::set<GBDATA*>         DBItemSet;
typedef DBItemSet::const_iterator DBItemSetIter;

#else
#error dbitem_set.h included twice
#endif // DBITEM_SET_H
