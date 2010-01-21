// =============================================================== //
//                                                                 //
//   File      : GDE_menu.h                                        //
//   Purpose   :                                                   //
//                                                                 //
// =============================================================== //

#ifndef GDE_MENU_H
#define GDE_MENU_H

#ifndef GDE_HXX
#include "gde.hxx"
#endif
#ifndef GDE_DEF_H
#include "GDE_def.h"
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define gde_assert(bed) arb_assert(bed)

typedef struct GargChoicetype
{
    char *label;                /* name for display in dialog box */
    char *method;               /* value (if null, return choice number) */
} GargChoice;

typedef struct GmenuItemArgtype
{
    int         optional;       /* is this optional? */
    int         type;           /* TEXT, SLIDER, CHOOSER, etc. */
    int         ivalue;
    double      min;            /* minimum range value */
    double      max;            /* maximum range value */
    double      fvalue;         /* default numeric value(or choice) */
    int         numchoices;     /* number of choices */
    char       *textvalue;      /* default text value */
    int         textwidth;      /* text width used for input field */
    char       *label;          /* description of arg function */
    char       *symbol;         /* internal symbol table mapping */
    char       *method;         /* commandline interpretation */
    GargChoice *choice;         /* choices */
    /* ARB BEGIN */
} GmenuItemArg;

typedef struct GfileFormattype
{
    int   save;                 /* how should file be saved */
    int   overwrite;            /* how should file be loaded */
    int   format;               /* what format is each field */
    int   maskable;             /* Can a write through mask be used? */
    int   select;               /* what type of selection */
    char *symbol;               /* internal symbol table mapping */
    char *name;                 /* file name */
} GfileFormat;

class AW_window;

typedef struct GmenuItemtype
{
    int               numargs;  /* number of agruments to cmnd */
    int               numoutputs; /* number of outputs from cmnd */
    int               numinputs; /* number of input files to cmnd */
    char             *label;    /* item name */
    char             *method;   /* commandline produced */
    GfileFormat      *input;    /* input definitions */
    GfileFormat      *output;   /* output definitions */
    GmenuItemArg     *arg;      /* argument definitions */
    char              meta;     /* Meta character for function */
    char              seqtype;  /* A -> amino, N -> nucleotide, '-' -> no sequence, otherwise both */
    char             *help;     /* commandline help */
    /* ARB BEGIN */
    struct Gmenutype *parent_menu;
    AW_window        *aws;      /* opened window */
} GmenuItem;

typedef struct Gmenutype
{
    int        numitems;        /* number of items in menu */
    char      *label;           /* menu heading */
    GmenuItem *item;            /* menu items */
    /* ARB BEGIN */
    char       meta;            /* Meta character for menu */
} Gmenu;

typedef unsigned char uchar;

extern struct choose_get_sequence_struct {
    char *(*get_sequences)(void *THIS, GBDATA **&the_species, uchar **&the_names, uchar **&the_sequences, long &numberspecies, long &maxalignlen);
    gde_cgss_window_type wt;
    void *THIS;
} gde_cgss;


#else
#error GDE_menu.h included twice
#endif // GDE_MENU_H
