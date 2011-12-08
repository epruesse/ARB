/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef NT_CB_HXX
#define NT_CB_HXX

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* NT_cb.cxx */

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
class AWT_canvas;

void NT_delete_mark_all_cb(void *, AWT_canvas *ntw);
AW_window *NT_open_select_tree_window(AW_root *awr, char *awar_tree);
void NT_select_last_tree(AW_window *aww, char *awar_tree);
AW_window *NT_open_select_alignment_window(AW_root *awr);
void NT_system_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file);
void NT_system_in_xterm_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file);

/* NT_extern.cxx */
void nt_test_ascii_print(AW_window *aww);
void nt_changesecurity(AW_root *aw_root);
void export_nds_cb(AW_window *aww, AW_CL print_flag);
AW_window *create_nds_export_window(AW_root *root);
void create_export_nds_awars(AW_root *awr, AW_default def);
void nt_exit(AW_window *aws);
void NT_save_cb(AW_window *aww);
void NT_save_quick_cb(AW_window *aww);
void NT_save_quick_as_cb(AW_window *aww);
void NT_save_as_cb(AW_window *aww);
AW_window *NT_create_save_quick_as(AW_root *aw_root, char *base_name);
void NT_database_optimization(AW_window *aww);
AW_window *NT_create_database_optimization_window(AW_root *aw_root);
AW_window *NT_create_save_as(AW_root *aw_root, const char *base_name);
void NT_undo_cb(AW_window *, AW_CL undo_type, AW_CL ntw);
void NT_undo_info_cb(AW_window *, AW_CL undo_type);
AW_window *NT_create_tree_setting(AW_root *aw_root);
void NT_submit_mail(AW_window *aww, AW_CL cl_awar_base);
AW_window *NT_submit_bug(AW_root *aw_root, int bug_report);
void NT_focus_cb(AW_window *);
void NT_modify_cb(AW_window *aww, AW_CL cd1, AW_CL cd2);
void NT_primer_cb(void);
void NT_mark_degenerated_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_deep_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_long_branches(AW_window *aww, AW_CL ntwcl);
void NT_mark_duplicates(AW_window *aww, AW_CL ntwcl);
void NT_justify_branch_lenghs(AW_window *, AW_CL cl_ntw, AW_CL dummy_1x);
void NT_fix_database(AW_window *);
void NT_pseudo_species_to_organism(AW_window *, AW_CL ntwcl);
void NT_update_marked_counter(AW_window *aww, long count);
void NT_popup_species_window(AW_window *aww, AW_CL cl_gb_main, AW_CL dummy_1x);
void NT_alltree_remove_leafs(AW_window *, AW_CL cl_mode, AW_CL cl_gb_main);
GBT_TREE *nt_get_current_tree_root(void);
void nt_create_main_window(AW_root *aw_root);

#else
#error nt_cb.hxx included twice
#endif /* NT_CB_HXX */
