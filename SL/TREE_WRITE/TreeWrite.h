// ============================================================ //
//                                                              //
//   File      : TreeWrite.h                                    //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef TREEWRITE_H
#define TREEWRITE_H

GB_ERROR TREE_write_Newick(GBDATA *gb_main, char *tree_name, bool use_NDS, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, char *path);
GB_ERROR TREE_write_XML(GBDATA *gb_main, const char *db_name, const char *tree_name, bool use_NDS, bool skip_folded, const char *path);

#else
#error TreeWrite.h included twice
#endif // TREEWRITE_H
