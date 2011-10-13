// =============================================================== //
//                                                                 //
//   File      : ptcommon.h                                        //
//   Purpose   : code common to ptserver and ptpan                 //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PTCOMMON_H
#define PTCOMMON_H

#ifndef ARB_CORE_H
#include <arb_core.h>
#endif

enum PT_Servertype {
    PTSERVER,
    PTPAN,
    PTUNKNOWN, 
};

PT_Servertype servertype_of_uptodate_index(const char *dbname, GB_ERROR& error);
const char *readable_Servertype(PT_Servertype type);

#else
#error ptcommon.h included twice
#endif // PTCOMMON_H
