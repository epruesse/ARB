#ifndef P_
#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif
#endif


/* NT_cb.cxx */
void NT_delete_mark_all_cb P_((void *dummy, AWT_canvas *ntw));
AW_window *NT_open_select_tree_window P_((AW_root *awr,char *awar_name));
AW_window *NT_open_select_alignment_window P_((AW_root *awr));
void NT_system_cb P_((AW_window *aww, AW_CL command, AW_CL auto_help_file));
void NT_system_cb2 P_((AW_window *aww, AW_CL command, AW_CL auto_help_file));

/* NT_extern.cxx */
void nt_changesecurity P_((AW_root *aw_root));
void export_nds_cb P_((AW_window *aww));
AW_window *create_nds_export_window P_((AW_root *root));
void create_export_nds_awars P_((AW_root *awr, AW_default def));
void create_all_awars P_((AW_root *awr, AW_default def));
void create_probe_menu_items P_((AW_window *awm));
void create_phylo_menu_items P_((AW_window *awm));
void nt_exit P_((AW_window *aw_window));
void NT_save_as_cb P_((AW_window *aww));
void NT_save_cb P_((AW_window *aww));
AW_window *NT_create_save_as P_((AW_root *aw_root,const  char *base_name));
AW_window *NT_create_tree_setting P_((AW_root *aw_root));
void NT_focus_cb P_((AW_window *aww));
void NT_modify_cb P_((AW_window *aww, AW_CL cd1, AW_CL cd2));
AW_window *create_nt_main_window P_((AW_root *awr,AW_CL clone));
AW_window *NT_preset_tree_window P_((AW_root *root));
void NT_select_last_tree(AW_window *aww,char *awar_tree);

#undef P_
