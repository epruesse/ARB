// =============================================================== //
//                                                                 //
//   File      : adExperiment.cxx                                  //
//   Purpose   : DB-access on experiments                          //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2008  //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include "gb_local.h"

GBDATA* EXP_get_experiment_data(GBDATA *gb_species) {
    return GB_search(gb_species, "experiment_data", GB_CREATE_CONTAINER);
}

GBDATA* EXP_find_experiment_rel_exp_data(GBDATA *gb_experiment_data, const char *name) {
    return GBT_find_item_rel_item_data(gb_experiment_data, "name", name);
}
GBDATA* EXP_find_experiment(GBDATA *gb_species, const char *name) {
    // search an experiment
    // Note: If you know the experiment exists, use EXP_expect_experiment!
    return EXP_find_experiment_rel_exp_data(EXP_get_experiment_data(gb_species), name);
}
GBDATA* EXP_first_experiment_rel_exp_data(GBDATA *gb_experiment_data) {
    return GB_entry(gb_experiment_data, "experiment");
}

GBDATA* EXP_next_experiment(GBDATA *gb_experiment) {
    gb_assert(GB_has_key(gb_experiment, "experiment"));
    return GB_nextEntry(gb_experiment);
}


GBDATA* EXP_find_or_create_experiment_rel_exp_data(GBDATA *gb_experiment_data, const char *name) {
    // Search for a experiment, when experiment does not exist create it
    return GBT_find_or_create_item_rel_item_data(gb_experiment_data, "experiment", "name", name, false);
}

