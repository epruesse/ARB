// =========================================================== //
//                                                             //
//   File      : awt_imp_exp.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_IMP_EXP_HXX
#define AWT_IMP_EXP_HXX


GB_ERROR AWT_export_Newick_tree(GBDATA *gb_main, char *tree_name, bool use_NDS, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, char *path);
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, bool use_NDS, bool skip_folded, const char *path);

#else
#error awt_imp_exp.hxx included twice
#endif // AWT_IMP_EXP_HXX
