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

struct GargChoice {
    char *label;                // name for display in dialog box
    char *method;               // value (if null, return choice number)
};

struct GmenuItemArg {
    int         type;           // TEXT, SLIDER, CHOOSER, etc.
    int         ivalue;
    double      min;            // minimum range value
    double      max;            // maximum range value
    double      fvalue;         // default numeric value(or choice)
    int         numchoices;     // number of choices
    char       *textvalue;      // default text value
    int         textwidth;      // text width used for input field
    char       *label;          // description of arg function
    char       *symbol;         // internal symbol table mapping
    GargChoice *choice;         // choices
    // ARB BEGIN
    AW_active     active_mask;  // expert/novice
};

enum TypeInfo {
    BASIC_TYPEINFO    = 0, // has to be zero (default value; initialized with calloc)
    DETAILED_TYPEINFO = 1,
    UNKNOWN_TYPEINFO  = 2,
};

struct GfileFormat {
    int       save;             // how should file be saved
    int       format;           // what format is each field
    char     *symbol;           // internal symbol table mapping
    char     *name;             // file name
    TypeInfo  typeinfo;
};

class AW_window;

struct GmenuItem {
    int           numargs;        // number of agruments to cmnd
    int           numoutputs;     // number of outputs from cmnd
    int           numinputs;      // number of input files to cmnd
    char         *label;          // item name
    char         *method;         // commandline produced
    GfileFormat  *input;          // input definitions
    GfileFormat  *output;         // output definitions
    GmenuItemArg *arg;            // argument definitions
    char          meta;           // Meta character for function
    char          seqtype;        // A -> amino, N -> nucleotide, '-' -> no sequence, otherwise both
    char         *help;           // associated helpfile ("agde_*.hlp")
    // ARB BEGIN
    struct Gmenu *parent_menu;
    AW_window    *aws;            // opened window
    AW_active     active_mask;    // expert/novice
};

struct Gmenu {
    int        numitems;        // number of items in menu
    char      *label;           // menu heading
    GmenuItem *item;            // menu items
    // ARB BEGIN
    char       meta;            // Meta character for menu
    AW_active  active_mask;     // expert/novice
};

typedef unsigned char uchar;

extern struct gde_database_access {
    GDE_get_sequences_cb  get_sequences;
    gde_window_type       window_type;
    AW_CL                 client_data;
    GBDATA               *gb_main;
} db_access;


#else
#error GDE_menu.h included twice
#endif // GDE_MENU_H
