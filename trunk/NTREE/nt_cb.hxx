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


/* NT_cb.cxx */
void NT_delete_mark_all_cb(void *dummy, AWT_canvas *ntw);
AW_window *NT_open_select_tree_window(AW_root *awr, char *awar_tree);
void NT_select_last_tree(AW_window *aww, char *awar_tree);
AW_window *NT_open_select_alignment_window(AW_root *awr);
void NT_system_cb(AW_window *aww, AW_CL command, AW_CL auto_help_file);
void NT_system_cb2(AW_window *aww, AW_CL command, AW_CL auto_help_file);

/* NT_extern.cxx */
void nt_test_ascii_print(AW_window *aww);
void nt_changesecurity(AW_root *aw_root);
void export_nds_cb(AW_window *aww, AW_CL print_flag);
AW_window *create_nds_export_window(AW_root *root);
void create_export_nds_awars(AW_root *awr, AW_default def);
void create_all_awars(AW_root *awr, AW_default def);
void nt_exit(AW_window *);
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
void NT_popup_species_window(AW_window *aww, AW_CL dummy_1x, AW_CL dummy_1x);
void NT_rename_test(AW_window *, AW_CL cl_gb_main, AW_CL dummy_1x);
void NT_test_AWT(AW_window *aww);
void NT_dump_gcs(AW_window *aww, AW_CL dummy_1x, AW_CL dummy_1x);
void NT_alltree_remove_leafs(AW_window *, AW_CL cl_mode, AW_CL cl_gb_main);
AW_window *create_nt_main_window(AW_root *awr, AW_CL clone);
