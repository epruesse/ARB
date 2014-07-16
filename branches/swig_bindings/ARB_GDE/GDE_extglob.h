// ================================================================ //
//                                                                  //
//   File      : GDE_extglob.h                                      //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef GDE_EXTGLOB_H
#define GDE_EXTGLOB_H

enum GapCompression {
    COMPRESS_NONE            = 0,
    COMPRESS_VERTICAL_GAPS   = 1,
    COMPRESS_ALL             = 2,
    COMPRESS_NONINFO_COLUMNS = 3,
};

#define GDEMAXMENU 100

class AP_filter;

#ifndef GDE_PROTO_H
#include "GDE_proto.h"
#endif

extern Gmenu menu[GDEMAXMENU];
extern int   num_menus;
extern int   DataType;
extern int   FileFormat;
extern char  FileName[80];

// Months of the year
extern const char *GDEmonth[12];

// Tables for DNA/RNA <--> ASCII translation
extern int Default_RNA_Trans[];
extern int Default_DNA_Trans[];
extern int Default_NA_RTrans[];

// Character->color lookup table
extern int Default_NAColor_LKUP[];
extern int Default_PROColor_LKUP[];

#else
#error GDE_extglob.h included twice
#endif // GDE_EXTGLOB_H
