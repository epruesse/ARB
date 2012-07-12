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

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ITEMS_H
#include <items.h>
#endif

class AW_window_menu_modes;

// --------------------------------------------------------------------------------
// awars:

#define AWAR_EXPERIMENT_NAME "tmp/exp/name"
#define AWAR_PROTEOM_NAME    "tmp/exp/proteom_name"
#define AWAR_PROTEIN_NAME    "tmp/exp/protein_name"

void EXP_create_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main);

// --------------------------------------------------------------------------------
// submenu:
void EXP_create_experiments_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool submenu);

// --------------------------------------------------------------------------------
// windows:
AW_window *EXP_create_experiment_query_window(AW_root *aw_root, AW_CL cl_gb_main);

ItemSelector& EXP_get_selector(); // return EXP_item_selector

#else
#error EXP.hxx included twice
#endif // EXP_HXX

