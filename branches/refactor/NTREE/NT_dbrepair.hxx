// =============================================================== //
//                                                                 //
//   File      : NT_dbrepair.hxx                                   //
//   Purpose   : repair database bugs                              //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2008       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_DBREPAIR_HXX
#define NT_DBREPAIR_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

class AW_window;

GB_ERROR NT_repair_DB(GBDATA *gb_main);
void     NT_rerepair_DB(AW_window*, AW_CL cl_gbmain, AW_CL);

#else
#error NT_dbrepair.hxx included twice
#endif // NT_DBREPAIR_HXX
