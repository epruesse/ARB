#ifndef awt_imp_exp_hxx_included
#define awt_imp_exp_hxx_included

GB_ERROR AWT_export_Newick_tree(GBDATA *gb_main, char *tree_name, AW_BOOL use_NDS, AW_BOOL save_branchlengths, AW_BOOL save_bootstraps, AW_BOOL save_groupnames, AW_BOOL pretty, char *path);
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, AW_BOOL use_NDS, AW_BOOL skip_folded, const char *path);

#endif
