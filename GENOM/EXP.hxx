//  ==================================================================== //
//                                                                       //
//    File      : EXP.hxx                                                //
//    Purpose   :                                                        //
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

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

class AW_window_menu_modes;

// --------------------------------------------------------------------------------
// awars:

#define AWAR_EXPERIMENT_NAME "tmp/exp/name"
#define AWAR_PROTEOM_NAME    "tmp/exp/proteom_name"
#define AWAR_PROTEIN_NAME    "tmp/exp/protein_name"

void EXP_create_awars(AW_root *aw_root, AW_default aw_def);
GBDATA *EXP_get_current_experiment(GBDATA *gb_main, AW_root *aw_root); // get AWAR_EXPERIMENT_NAME experiment

// --------------------------------------------------------------------------------
// submenu:
void EXP_create_experiments_submenu(AW_window_menu_modes *awm, bool submenu);

// --------------------------------------------------------------------------------
// windows:
AW_window *EXP_create_experiment_query_window(AW_root *aw_root);

#else
#error EXP.hxx included twice
#endif // EXP_HXX

