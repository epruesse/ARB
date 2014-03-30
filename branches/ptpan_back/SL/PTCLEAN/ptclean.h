// ================================================================= //
//                                                                   //
//   File      : ptclean.h                                           //
//   Purpose   : prepare db for ptserver/ptpan                       //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2011   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef PTCLEAN_H
#define PTCLEAN_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef PTCOMMON_H
#include <ptcommon.h>
#endif

GB_ERROR prepare_ptserver_database(GBDATA *gb_main, PT_Servertype type);

#else
#error ptclean.h included twice
#endif // PTCLEAN_H