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

GB_ERROR AWT_export_Newick_tree(GBDATA *gb_main, char *tree_name, bool use_NDS, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, char *path);
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, bool use_NDS, bool skip_folded, const char *path);

#else
#error TreeWrite.h included twice
#endif // TREEWRITE_H
