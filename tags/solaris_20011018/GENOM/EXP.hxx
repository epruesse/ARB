//  ==================================================================== //
//                                                                       //
//    File      : EXP.hxx                                                //
//    Purpose   :                                                        //
//    Time-stamp: <Thu Sep/06/2001 23:18 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef EXP_HXX
#define EXP_HXX

class AW_window_menu_modes;

// --------------------------------------------------------------------------------
// awars:
void EXP_create_awars(AW_root *aw_root, AW_default aw_def);
inline GBDATA* EXP_get_experiment_data(GBDATA *gb_species) { return GB_search(gb_species, "experiment_data", GB_CREATE_CONTAINER); }

// --------------------------------------------------------------------------------
// submenu:
void EXP_create_experiments_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE);

// --------------------------------------------------------------------------------
// windows:
AW_window *EXP_create_experiment_query_window(AW_root *aw_root);

// --------------------------------------------------------------------------------
// database access:
GBDATA *EXP_get_current_experiment(GBDATA *gb_main, AW_root *aw_root);

GBDATA* EXP_find_experiment(GBDATA *gb_species, const char *name);
GBDATA* EXP_find_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name);

GBDATA* EXP_first_experiment_rel_experiment_data(GBDATA *gb_experiment_data);
GBDATA* EXP_next_experiment(GBDATA *gb_experiment);

GBDATA* EXT_create_experiment_rel_experiment_data(GBDATA *gb_experiment_data, const char *name);

#else
#error EXP.hxx included twice
#endif // EXP_HXX

