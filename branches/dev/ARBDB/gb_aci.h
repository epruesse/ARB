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
#ifndef ARBDBT_H
#include "arbdbt.h"
#endif

#define GBL_MAX_ARGUMENTS 500

struct GBL {
    char *str;
};

class GBL_reference {
    GBDATA     *gb_ref;            // the database entry on which the command is applied (may be species, gene, experiment, group and maybe more)
    const char *default_tree_name; // if we have a default tree, its name is specified here (NULL otherwise)

public:
    GBL_reference(GBDATA *gbd, const char *treeName)
        : gb_ref(gbd), default_tree_name(treeName) {}

    GBDATA *get_main() const { return GB_get_root(gb_ref); }
    GBDATA *get_ref() const { return gb_ref; }
    const char *get_tree_name() const { return default_tree_name; }
};

struct GBL_command_arguments : public GBL_reference {
    GBL_command_arguments(GBDATA *gbd, const char *treeName) : GBL_reference(gbd, treeName) {}
    
    const char *command; // the name of the current command

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
