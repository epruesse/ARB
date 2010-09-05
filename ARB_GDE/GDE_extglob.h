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
extern int num_menus;
extern GBDATA *GLOBAL_gb_main;

/* global.h */
extern int DataType;
extern int FileFormat,first_select;
/*int Dirty,OldEditMode,EditMode = INSERT, EditDir = RIGHT;*/
extern int DisplayAttr,OVERWRITE;
extern int SCALE;
extern int BlockInput;
#ifdef SeeAlloc
int TotalCalloc = 0;
int TotalRealloc = 0;
#endif
extern char FileName[80];
extern char current_dir[1024];

/*
*       Months of the year
*/
extern const char *GDEmonth[12];
/*
*       Tables for DNA/RNA <--> ASCII translation
*/

extern int Default_RNA_Trans[];
extern int Default_DNA_Trans[];
extern int Default_NA_RTrans[];


/*
*       RGB values for the simple palette
*/


/*
*       Character->color lookup table
*/

extern int Default_NAColor_LKUP[];
extern int Default_PROColor_LKUP[];

extern const char *vert_mito[512];
extern const char *mycoplasma[512];
extern const char *universal[512];
extern const char *yeast[512];

extern const char *three_to_one[23];
#if 0

extern unsigned char grey0;
extern unsigned char grey1;
extern unsigned char grey2;
extern unsigned char grey3;
extern unsigned char grey4;
extern unsigned char grey5;
extern unsigned char grey6;
extern unsigned char grey7;

extern unsigned char **greys;
extern char **grey_pm; /*Pixmap instead of char !?!*/
#endif

#else
#error GDE_extglob.h included twice
#endif // GDE_EXTGLOB_H
