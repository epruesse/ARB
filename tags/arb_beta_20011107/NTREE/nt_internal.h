
/* NT_edconf.cxx */
AW_window *NT_start_editor_on_old_configuration(AW_root *awr);
GB_ERROR NT_create_configuration(AW_window *, GBT_TREE **ptree, const char *conf_name, int use_species_aside);
void NT_start_editor_on_tree(AW_window *, GBT_TREE **ptree, int use_species_aside);
AW_window *NT_extract_configuration(AW_root *awr);

/* NT_join.cxx */

/* NT_sort.cxx */
char *NT_resort_data_base_by_tree(GBT_TREE *tree, GBDATA *gb_species_data);
GB_ERROR NT_resort_data_base(GBT_TREE *tree, char *key1, char *key2, char *key3);
void NT_resort_data_by_phylogeny(AW_window *dummy, GBT_TREE **ptree);
void NT_resort_data_by_costum(AW_window *aw);
void NT_build_resort_awars(AW_root *awr, AW_default aw_def);
AW_window *NT_build_resort_window(AW_root *awr);

/* NT_cb.cxx */
void NT_delete_mark_all_cb(void *dummy, AWT_canvas *ntw);
AW_window *NT_open_select_tree_window(AW_root *awr, char *awar_tree);
void NT_select_last_tree(AW_window *aww, char *awar_tree);
AW_window *NT_open_select_alignment_window(AW_root *awr);
void NT_system_cb(AW_window *aww, AW_CL command, AW_CL auto_help_file);
void NT_system_cb2(AW_window *aww, AW_CL command, AW_CL auto_help_file);

/* NT_main.cxx */

/* NT_extern.cxx */
void NT_show_message(AW_root *awr);

/* NT_ins_col.cxx */

/* NT_import.cxx */
void NT_import_sequences(AW_window *aww, int AW_CL, int AW_CL);

/* ad_spec.cxx */
AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_species_window(AW_root *aw_root);
AW_window *NT_create_organism_window(AW_root *aw_root);

/* ad_trees.cxx */

/* ad_ali.cxx */

/* ad_ext.cxx */

/* ad_transpro.cxx */

/* AP_consensus.cxx */
AW_window *AP_open_con_expert_window(AW_root *aw_root);
AW_window *AP_open_consensus_window(AW_root *aw_root);
AW_window *AP_open_max_freq_window(AW_root *aw_root);

/* AP_cprofile.cxx */
AW_window *AP_open_cprofile_window(AW_root *aw_root);

/* AP_pos_var_pars.cxx */
void AP_calc_pos_var_pars(AW_window *aww);
AW_window *AP_open_pos_var_pars_window(AW_root *root);

/* ETC_check_gcg.cxx */

/* AP_csp_2_gnuplot.cxx */
void AP_csp_2_gnuplot_cb(AW_window *aww, AW_CL cspcd);
AW_window *AP_open_csp_2_gnuplot_window(AW_root *root);
