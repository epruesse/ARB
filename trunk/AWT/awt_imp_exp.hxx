#ifndef awt_imp_exp_hxx_included
#define awt_imp_exp_hxx_included

GB_ERROR AWT_export_Newick_tree(GBDATA *gb_main, char *tree_name, bool use_NDS, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, char *path);
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, bool use_NDS, bool skip_folded, const char *path);

#endif
