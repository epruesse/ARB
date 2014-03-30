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
GBT_TREE *nt_get_current_tree_root(void);
void nt_create_main_window(AW_root *aw_root);

#else
#error nt_cb.hxx included twice
#endif /* NT_CB_HXX */