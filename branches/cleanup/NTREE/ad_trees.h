// ============================================================ //
//                                                              //
//   File      : ad_trees.h                                     //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef AD_TREES_H
#define AD_TREES_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

#define AWAR_TREE_NAME "tmp/ad_tree/tree_name"

void create_trees_var(AW_root *aw_root, AW_default aw_def);
void popup_tree_admin_window(AW_root *aw_root);
void popup_tree_admin_window(AW_window *aws);

#else
#error ad_trees.h included twice
#endif // AD_TREES_H
