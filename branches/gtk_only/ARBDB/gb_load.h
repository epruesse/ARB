// =============================================================== //
//                                                                 //
//   File      : gb_load.h                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_LOAD_H
#define GB_LOAD_H

#define GB_MAIN_ARRAY_SIZE 4096

#define A_TO_I(c) if (c>'9') c-='A'-10; else c-='0'; // @@@ design+name are crap

enum gb_scan_quicks_types {
    GB_SCAN_NO_QUICK,
    GB_SCAN_NEW_QUICK,
    GB_SCAN_OLD_QUICK
};

struct gb_scandir {
    int                  highest_quick_index;
    int                  newest_quick_index;
    unsigned long        date_of_quick_file;
    gb_scan_quicks_types type;  // xxx.arb.quick? or xxx.a??
};


#else
#error gb_load.h included twice
#endif // GB_LOAD_H
