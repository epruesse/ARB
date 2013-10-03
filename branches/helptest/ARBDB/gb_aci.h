// =============================================================== //
//                                                                 //
//   File      : gb_aci.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_ACI_H
#define GB_ACI_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#define GBL_MAX_ARGUMENTS 500

struct GBL {
    char *str;
};

struct GBL_command_arguments {
    GBDATA     *gb_ref;                             // the database entry on which the command is applied (may be species, gene, experiment, group and maybe more)
    const char *default_tree_name;                  // if we have a default tree, its name is specified here (NULL otherwise)
    const char *command;                            // the name of the current command

    // input streams:
    int        cinput;
    const GBL *vinput;

    // parameter "streams":
    int        cparam;
    const GBL *vparam;

    // output streams:
    int  *coutput;
    GBL **voutput;
};

typedef GB_ERROR (*GBL_COMMAND)(GBL_command_arguments *args);

struct GBL_command_table {
    const char *command_identifier;
    GBL_COMMAND function;
};

#else
#error gb_aci.h included twice
#endif // GB_ACI_H
