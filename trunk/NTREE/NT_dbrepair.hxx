// =============================================================== //
//                                                                 //
//   File      : NT_dbrepair.hxx                                   //
//   Purpose   : repair database bugs                              //
//   Time-stamp: <Wed Sep/03/2008 15:24 MET Coder@ReallySoft.de>   //
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
#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif



GB_ERROR NT_repair_DB(GBDATA *gb_main);
void NT_rerepair_DB(AW_window*, AW_CL cl_gbmain, AW_CL);

#else
#error NT_dbrepair.hxx included twice
#endif // NT_DBREPAIR_HXX
