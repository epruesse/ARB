// ==================================================================== //
//                                                                      //
//   File      : awt_sel_boxes.hxx                                      //
//   Purpose   :                                                        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AWT_SEL_BOXES_HXX
#define AWT_SEL_BOXES_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

class AP_filter;
struct adfiltercbstruct;

// -----------------------------------------
//      various database selection boxes

void awt_create_selection_list_on_alignments(GBDATA *gb_main, AW_window *aws, const char *varname, const char *comm);
void awt_create_selection_list_on_trees(GBDATA *gb_main, AW_window *aws, const char *varname);

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, bool popup);
void awt_edit_arbtcpdat_cb(AW_window *aww, AW_CL cl_gb_main);

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws, const char *varname);
void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws, const char *tablename, const char *varname);
AW_window *AWT_create_tables_admin_window(AW_root *aw_root, GBDATA *gb_main);

void *awt_create_selection_list_on_extendeds(GBDATA *gb_main, AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL) = 0, AW_CL filter_cd = 0,
                                             bool add_sel_species = false);
void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid);

void  awt_create_selection_list_on_configurations(GBDATA *gb_main, AW_window *aws, const char *varname);
char *awt_create_string_on_configurations(GBDATA *gb_main);

AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*), AW_window* (*create_popup)(AW_root *, AW_default));

// -------------------------------

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, AW_CL selid);
AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid);
void create_print_box_for_selection_lists(AW_window *aw_window, AW_CL selid);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

