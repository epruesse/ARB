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

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

class AP_filter;
struct adfiltercbstruct;

/**************************************************************************
 *********************   Various Database Selection Boxes    ***********
 ***************************************************************************/
/***********************    Alignments  ************************/
void awt_create_selection_list_on_ad(GBDATA *gb_main,AW_window *aws,const char *varname,const char *comm);
/* Create selection lists on alignments, if comm is set, then
   pars the alignment type, show only those that are parsed */


/***********************    Trees       ************************/
/* Selection list for trees */
void awt_create_selection_list_on_trees(GBDATA *gb_main,AW_window *aws, const char *varname);

/*********************** Pt-Servers ******************************/
/* Selection list for pt-servers */

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, bool popup);
void awt_refresh_all_pt_server_selection_lists();

/***********************    Tables Fields       ************************/

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws,const char *varname);

void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws,const char *tablename, const char *varname);
// if tablename == 0 take fields from species table !!!!

AW_window *AWT_create_tables_admin_window(AW_root *aw_root,GBDATA *gb_main);

/***********************    SAIS        ************************/
void *awt_create_selection_list_on_extendeds(GBDATA *gb_main,AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL) = 0, AW_CL filter_cd = 0,
                                             bool add_sel_species= false);
/* Selection list for all extendeds !!!!!!!!! */
/* if filter_proc is set then show only those items on which
   filter_proc returns a strdup(string) */

void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid);
/* update the selection box defined by awt_create_selection_list_on_extendeds
   usefull only when filterproc is defined (changes to the SAI entries automatically
   calls awt_create_selection_list_on_extendeds_update*/

/***********************    CONFIGURATIONS      ************************/

void  awt_create_selection_list_on_configurations(GBDATA *gb_main,AW_window *aws, const char *varname);
char *awt_create_string_on_configurations(GBDATA *gb_main);

/***********************    FILES IN GENERAL        ************************/

AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*), // set callback ..
                               AW_window* (*create_popup)(AW_root *, AW_default)); // .. or create_popup  (both together not allowed)

/***********************    species fields     ************************/

void AWT_popup_select_species_field_window(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

