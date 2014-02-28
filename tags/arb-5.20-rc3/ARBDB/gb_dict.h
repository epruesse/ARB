// =============================================================== //
//                                                                 //
//   File      : gb_dict.h                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_DICT_H
#define GB_DICT_H

typedef int GB_NINT;                                // Network byte order int

struct GB_DICTIONARY {
    int            words;
    int            textlen;
    unsigned char *text;
    GB_NINT       *offsets;
    GB_NINT       *resort;
};

#else
#error gb_dict.h included twice
#endif // GB_DICT_H
