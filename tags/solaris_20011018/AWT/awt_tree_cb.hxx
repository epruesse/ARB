#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* AWT_tree_cb.cxx */
void nt_mode_event P_((AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode));
void NT_mark_all_cb P_((void *dummy, AWT_canvas *ntw));
void NT_unmark_all_cb P_((void *dummy, AWT_canvas *ntw));
void NT_invert_mark_all_cb P_((void *dummy, AWT_canvas *ntw));
void NT_count_mark_all_cb P_((void *dummy, AWT_canvas *ntw));
void NT_mark_tree_cb P_((void *dummy, AWT_canvas *ntw));
void NT_group_tree_cb P_((void *dummy, AWT_canvas *ntw));
void NT_group_not_marked_cb P_((void *dummy, AWT_canvas *ntw));
void NT_group_terminal_cb P_((void *dummy, AWT_canvas *ntw));
void NT_resort_tree_cb P_((void *dummy, AWT_canvas *ntw, int type));
void NT_ungroup_all_cb P_((void *dummy, AWT_canvas *ntw));
void NT_reset_lzoom_cb P_((void *dummy, AWT_canvas *ntw));
void NT_reset_pzoom_cb P_((void *dummy, AWT_canvas *ntw));
void NT_unmark_all_tree_cb P_((void *dummy, AWT_canvas *ntw));
void NT_set_tree_style P_((void *dummy, AWT_canvas *ntw, AP_tree_sort type));
void NT_remove_leafs P_((void *dummy, AWT_canvas *ntw, long mode));
void NT_remove_bootstrap P_((void *dummy, AWT_canvas *ntw));
void NT_jump_cb P_((AW_window *dummy, AWT_canvas *ntw, AW_CL auto_expand_groups));
void NT_jump_cb_auto P_((AW_window *dummy, AWT_canvas *ntw));
void NT_reload_tree_event P_((AW_root *awr, AWT_canvas *ntw, GB_BOOL set_delete_cbs));

#ifdef __cplusplus
}
#endif

#undef P_
