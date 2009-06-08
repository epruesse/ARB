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

#define AWAR_TREE_NAME "tmp/ad_tree/tree_name"

void       create_trees_var(AW_root *aw_root, AW_default aw_def);
AW_window *create_trees_window(AW_root *aw_root);

#else
#error ad_trees.hxx included twice
#endif // AD_TREES_HXX
