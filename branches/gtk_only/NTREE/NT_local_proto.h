/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef NT_LOCAL_PROTO_H
#define NT_LOCAL_PROTO_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* AP_consensus.cxx */
void AP_create_consensus_var(AW_root *aw_root, AW_default aw_def);
AW_window *AP_create_con_expert_window(AW_root *aw_root);
AW_window *AP_create_max_freq_window(AW_root *aw_root);

/* AP_pos_var_pars.cxx */
AW_window *AP_create_pos_var_pars_window(AW_root *root);

/* ColumnStat_2_gnuplot.cxx */
AW_window *NT_create_colstat_2_gnuplot_window(AW_root *root);

/* NT_branchAnalysis.cxx */

class AWT_canvas;

AW_window *NT_create_branch_analysis_window(AW_root *aw_root, AWT_canvas *ntw);

/* NT_cb.cxx */

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
class AWT_canvas;

void NT_delete_mark_all_cb(void *, AWT_canvas *ntw);
AW_window *NT_create_select_tree_window(AW_root *awr, const char *awar_tree);
void NT_select_bottom_tree(AW_window *aww, const char *awar_tree);
void NT_create_alignment_vars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main);
AW_window *NT_create_alignment_admin_window(AW_root *root, AW_window *aw_popmedown);
AW_window *NT_create_select_alignment_window(AW_root *awr);
void NT_system_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file);
void NT_system_in_xterm_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file);

/* NT_concatenate.cxx */
void NT_createConcatenationAwars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main);
AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root);
AW_window *NT_createConcatenationWindow(AW_root *aw_root);

/* NT_dbrepair.cxx */
GB_ERROR NT_repair_DB(GBDATA *gb_main);
void NT_rerepair_DB(AW_window *, AW_CL cl_gbmain, AW_CL dummy_1x);

/* NT_edconf.cxx */
void NT_activate_configMarkers_display(AWT_canvas *ntw);
void NT_popup_configuration_admin(AW_window *aw_main, AWT_canvas *ntw);
AW_window *NT_create_startEditorOnOldConfiguration_window(AW_root *awr);
void NT_start_editor_on_tree(AW_window *aww, int use_species_aside, AWT_canvas *ntw);
void NT_create_config_after_import(AWT_canvas *ntw, bool imported_from_scratch);

/* NT_extern.cxx */
void NT_start(const char *arb_ntree_args, bool restart_with_new_ARB_PID);
void NT_exit(AW_window *aws, AW_CL exitcode);
void NT_restart(AW_root *aw_root, const char *arb_ntree_args);
TreeNode *NT_get_tree_root_of_canvas(AWT_canvas *ntw);
int NT_get_canvas_id(AWT_canvas *ntw);
AWT_canvas *NT_create_main_window(AW_root *aw_root);

/* NT_import.cxx */
void NT_import_sequences(AW_window *aww, AWT_canvas *ntw);

/* NT_join.cxx */
AW_window *NT_create_species_join_window(AW_root *root);

/* NT_main.cxx */
GB_ERROR NT_format_all_alignments(GBDATA *gb_main);

/* NT_sort.cxx */
void NT_resort_data_by_phylogeny(AW_window *, AW_CL cl_ntw, AW_CL dummy_1x);
void NT_create_resort_awars(AW_root *awr, AW_default aw_def);
AW_window *NT_create_resort_window(AW_root *awr);

/* NT_taxonomy.cxx */
void NT_create_compare_taxonomy_awars(AW_root *aw_root, AW_default props);
AW_window *NT_create_compare_taxonomy_window(AW_root *aw_root, AWT_canvas *ntw);

/* NT_trackAliChanges.cxx */
void NT_create_trackAliChanges_Awars(AW_root *root, AW_default properties);
AW_window *NT_create_trackAliChanges_window(AW_root *root);

/* NT_userland_fixes.cxx */
void NT_repair_userland_problems(void);

/* NT_validManual.cxx */
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def);
AW_window *NT_create_searchManuallyNames_window(AW_root *aw_root);

/* NT_validNames.cxx */
void NT_deleteValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_importValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);
void NT_suggestValidNames(AW_window *, AW_CL dummy_1x, AW_CL dummy_2x);

/* ad_ext.cxx */
void NT_create_extendeds_vars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main);
AW_window *NT_create_extendeds_window(AW_root *aw_root);

/* ad_spec.cxx */
void NT_count_different_chars(AW_window *, AW_CL cl_gb_main, AW_CL dummy_1x);
void NT_create_sai_from_pfold(AW_window *aww, AW_CL ntw, AW_CL dummy_1x);

/* ad_transpro.cxx */
AW_window *NT_create_dna_2_pro_window(AW_root *root);
AW_window *NT_create_realign_dna_window(AW_root *root);
void NT_create_transpro_variables(AW_root *root, AW_default props);

/* ad_trees.cxx */
AW_window *NT_create_consense_window(AW_root *aw_root);
AW_window *NT_create_sort_tree_by_other_tree_window(AW_root *aw_root, AWT_canvas *ntw);
void NT_create_multifurcate_tree_awars(AW_root *aw_root, AW_default props);
AW_window *NT_create_multifurcate_tree_window(AW_root *aw_root, AWT_canvas *ntw);

#else
#error NT_local_proto.h included twice
#endif /* NT_LOCAL_PROTO_H */
