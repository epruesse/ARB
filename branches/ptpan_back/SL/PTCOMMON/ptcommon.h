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

enum Servertype { // @@@ DRY vs PTCLEAN when merged
    PTSERVER,
    PTPAN
};

bool index_exists_for(Servertype type); 

#else
#error ptcommon.h included twice
#endif // PTCOMMON_H
