// ============================================================== //
//                                                                //
//   File      : dbconn.h                                         //
//   Purpose   : Connector to running ARB                         //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2011   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef DBCONN_H
#define DBCONN_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

GBDATA *runningDatabase();

#else
#error dbconn.h included twice
#endif // DBCONN_H
