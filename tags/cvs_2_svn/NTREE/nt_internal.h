/*
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

/* hide __attribute__'s for non-gcc compilers: */
#ifndef __GNUC__
# ifndef __attribute__
#  define __attribute__(x)
# endif
#endif


/* AP_consensus.cxx */
AW_window *AP_open_con_expert_window(AW_root *aw_root);
AW_window *AP_open_consensus_window(AW_root *aw_root);
AW_window *AP_open_max_freq_window(AW_root *aw_root);

/* AP_conservProfile2Gnuplot.cxx */
void AP_conservProfile2Gnuplot_callback(AW_window *aww);
AW_window *AP_openConservationPorfileWindow(AW_root *root);

/* AP_cprofile.cxx */
AW_window *AP_open_cprofile_window(AW_root *aw_root);

/* AP_csp_2_gnuplot.cxx */
void AP_csp_2_gnuplot_cb(AW_window *aww, AW_CL cspcd, AW_CL cl_mode);
AW_window *AP_open_csp_2_gnuplot_window(AW_root *root);

/* AP_pos_var_pars.cxx */
void AP_calc_pos_var_pars(AW_window *aww);
AW_window *AP_open_pos_var_pars_window(AW_root *root);

/* NT_MAUS.cxx */
void NT_create_MAUS_awars(AW_root *aw_root, AW_default aw_def, AW_default gb_def);
AW_window *NT_create_MAUS_window(AW_root *aw_root, AW_CL dummy_1x);

/* NT_cb.cxx */
void NT_delete_mark_all_cb(void *dummy, AWT_canvas *ntw);
AW_window *NT_open_select_tree_window(AW_root *awr, char *awar_tree);
void NT_select_last_tree(AW_window *aww, char *awar_tree);
AW_window *NT_open_select_alignment_window(AW_root *awr);
void NT_system_cb(AW_window *aww, AW_CL command, AW_CL auto_help_file);
void NT_system_cb2(AW_window *aww, AW_CL command, AW_CL auto_help_file);

/* NT_concatenate.cxx */
void NT_createConcatenationAwars(AW_root *aw_root, AW_default aw_def);
AW_window *NT_createConcatenationWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesAndConcatenateWindow(AW_root *aw_root, AW_CL cl_ntw);

/* NT_dbrepair.cxx */
GB_ERROR NT_repair_DB(GBDATA *gb_main);

/* NT_edconf.cxx */
AW_window *NT_start_editor_on_old_configuration(AW_root *awr);
void NT_start_editor_on_tree(AW_window *, GBT_TREE **ptree, int use_species_aside);
GB_ERROR NT_create_configuration(AW_window *, GBT_TREE **ptree, const char *conf_name, int use_species_aside);
void NT_configuration_admin(AW_window *aw_main, AW_CL cl_GBT_TREE_ptr, AW_CL dummy_1x);
AW_window *NT_extract_configuration(AW_root *awr);
GB_ERROR NT_create_configuration_cb(AW_window *aww, AW_CL cl_GBT_TREE_ptr, AW_CL cl_use_species_aside);

/* NT_extern.cxx */
void NT_save_cb(AW_window *aww);
void NT_save_quick_cb(AW_window *aww);
void NT_save_quick_as_cb(AW_window *aww);
AW_window *NT_create_save_quick_as(AW_root *aw_root, char *base_name);
void NT_database_optimization(AW_window *aww);
AW_window *NT_create_database_optimization_window(AW_root *aw_root);
void NT_save_as_cb(AW_window *aww);
AW_window *NT_create_save_as(AW_root *aw_root, const char *base_name);
void NT_undo_cb(AW_window *, AW_CL undo_type, AW_CL ntw);
void NT_undo_info_cb(AW_window *, AW_CL undo_type);
AW_window *NT_create_tree_setting(AW_root *aw_root);
void NT_submit_mail(AW_window *aww, AW_CL cl_awar_base);
AW_window *NT_submit_bug(AW_root *aw_root, int bug_report);
void NT_focus_cb(AW_window *aww);
void NT_modify_cb(AW_window *aww, AW_CL cd1, AW_CL cd2);
void NT_primer_cb(void);
void NT_set_compression(AW_window *, AW_CL dis_compr, AW_CL dummy_1x);
void NT_mark_degenerated_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_deep_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_long_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_duplicates(AW_window *aww, AW_CL ntwcl);
void NT_justify_branch_lenghs(AW_window *, AW_CL cl_ntw, AW_CL dummy_1x);
void NT_fix_database(AW_window *);
void NT_pseudo_species_to_organism(AW_window *, AW_CL ntwcl);
void NT_test_input_mask(AW_root *root);
void NT_update_marked_counter(AW_window *aww, long count);
void NT_popup_species_window(AW_window *aww, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_rename_test(AW_window *, AW_CL cl_gb_main, AW_CL dummy_1x);
void NT_test_AWT(AW_window *aww);
void NT_dump_gcs(AW_window *aww, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_alltree_remove_leafs(AW_window *, AW_CL cl_mode, AW_CL cl_gb_main);

/* NT_import.cxx */
void NT_import_sequences(AW_window *aww, AW_CL dummy_1x, AW_CL dummy_2x);

/* NT_main.cxx */
GB_ERROR NT_format_all_alignments(GBDATA *gb_main);

/* NT_sort.cxx */
char *NT_resort_data_base_by_tree(GBT_TREE *tree, GBDATA *gb_species_data);
GB_ERROR NT_resort_data_base(GBT_TREE *tree, char *key1, char *key2, char *key3);
void NT_resort_data_by_phylogeny(AW_window *dummy, GBT_TREE **ptree);
void NT_resort_data_by_costum(AW_window *aw);
void NT_build_resort_awars(AW_root *awr, AW_default aw_def);
AW_window *NT_build_resort_window(AW_root *awr);

/* NT_trackAliChanges.cxx */
void NT_create_trackAliChanges_Awars(AW_root *root, AW_default properties);
AW_window *NT_create_trackAliChanges_window(AW_root *root);

/* NT_validManual.cxx */
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def);
AW_window *NT_searchManuallyNames(AW_root *aw_root);

/* NT_validNames.cxx */
void NT_deleteValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_importValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_suggestValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);

/* ad_ali.cxx */
void NT_create_alignment_vars(AW_root *aw_root, AW_default aw_def);
AW_window *NT_create_alignment_window(AW_root *root, AW_CL popmedown);

/* ad_ext.cxx */
void NT_create_extendeds_var(AW_root *aw_root, AW_default aw_def);
AW_window *NT_create_extendeds_window(AW_root *aw_root);

/* ad_spec.cxx */
AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_item_selector);
void NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information);
AW_window *NT_create_species_window(AW_root *aw_root);
AW_window *NT_create_organism_window(AW_root *aw_root);

/* ad_transpro.cxx */
AW_window *NT_create_dna_2_pro_window(AW_root *root);
AW_window *NT_create_realign_dna_window(AW_root *root);
void NT_create_transpro_variables(AW_root *root, AW_default db1);
