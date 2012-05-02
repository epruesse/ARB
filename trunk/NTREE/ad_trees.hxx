// ============================================================ //
//                                                              //
//   File      : ad_trees.hxx                                   //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef AD_TREES_HXX
#define AD_TREES_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

#define AWAR_TREE_NAME "tmp/ad_tree/tree_name"

void create_trees_var(AW_root *aw_root, AW_default aw_def);
void popup_tree_admin_window(AW_root *aw_root, AW_CL cl_popmedown);
void popup_tree_admin_window(AW_window *aws, AW_CL cl_popmedown);

#else
#error ad_trees.hxx included twice
#endif // AD_TREES_HXX
