//  ==================================================================== //
//                                                                       //
//    File      : EXP_db.cxx                                             //
//    Purpose   : database access for experiments                        //
//    Time-stamp: <Thu Mar/11/2004 13:43 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <cstdio>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include "EXP.hxx"

using namespace std;

GBDATA* EXP_find_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name) {
    GBDATA *gb_name = GB_find(gb_experiment_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing experiment
    return 0;
}
GBDATA* EXP_find_experiment(GBDATA *gb_species, const char *name) {
    // find existing experiment
    return EXP_find_experiment_rel_experiment_data(EXP_get_experiment_data(gb_species), name);
}


GBDATA* EXP_first_experiment_rel_experiment_data(GBDATA *gb_experiment_data) {
    return GB_find(gb_experiment_data, "experiment", 0, down_level);
}

GBDATA* EXP_next_experiment(GBDATA *gb_experiment) {
    return GB_find(gb_experiment, "experiment", 0, this_level|search_next);
}


GBDATA* EXT_create_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name) {
    /* Search for a experiment, when experiment does not exist create it */
    GBDATA *gb_name = GB_find(gb_experiment_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing experiment

    GBDATA *gb_experiment = GB_create_container(gb_experiment_data, "experiment");
    gb_name = GB_create(gb_experiment, "name", GB_STRING);
    GB_write_string(gb_name, name);

    return gb_experiment;
}


