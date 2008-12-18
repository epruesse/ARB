/* ================================================================ */
/*                                                                  */
/*   File      : adExperiment.c                                     */
/*   Purpose   : DB-access on experiments                           */
/*   Time-stamp: <Wed Dec/17/2008 23:50 MET Coder@ReallySoft.de>    */
/*                                                                  */
/*   Coded by Ralf Westram (coder@reallysoft.de) in December 2008   */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
 /*                                                                  */
 /* ================================================================ */

#include "arbdb.h"

GBDATA* EXP_get_experiment_data(GBDATA *gb_species) {
    return GB_search(gb_species, "experiment_data", GB_CREATE_CONTAINER);
}

GBDATA* EXP_find_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name) {
    GBDATA *gb_name = GB_find_string(gb_experiment_data, "name", name, GB_IGNORE_CASE, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing experiment
    return 0;
}
GBDATA* EXP_find_experiment(GBDATA *gb_species, const char *name) {
    // find existing experiment
    return EXP_find_experiment_rel_experiment_data(EXP_get_experiment_data(gb_species), name);
}


GBDATA* EXP_first_experiment_rel_experiment_data(GBDATA *gb_experiment_data) {
    return GB_entry(gb_experiment_data, "experiment");
}

GBDATA* EXP_next_experiment(GBDATA *gb_experiment) {
    gb_assert(GB_has_key(gb_experiment, "experiment"));
    return GB_nextEntry(gb_experiment);
}


GBDATA* EXP_create_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name) {
    /* Search for a experiment, when experiment does not exist create it */
    GBDATA *gb_name = GB_find_string(gb_experiment_data, "name", name, GB_IGNORE_CASE, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing experiment

    GBDATA *gb_experiment = GB_create_container(gb_experiment_data, "experiment");
    gb_name = GB_create(gb_experiment, "name", GB_STRING);
    GB_write_string(gb_name, name);

    return gb_experiment;
}



