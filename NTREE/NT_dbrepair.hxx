// =============================================================== //
//                                                                 //
//   File      : NT_dbrepair.hxx                                   //
//   Purpose   : repair database bugs                              //
//   Time-stamp: <Wed May/28/2008 17:47 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2008       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_DBREPAIR_HXX
#define NT_DBREPAIR_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif


GB_ERROR NT_repair_DB(GBDATA *gb_main);

#else
#error NT_dbrepair.hxx included twice
#endif // NT_DBREPAIR_HXX
