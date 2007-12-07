// ==================================================================== //
//                                                                      //
//   File      : awt_sel_boxes.hxx                                      //
//   Purpose   :                                                        //
//   Time-stamp: <Fri Apr/20/2007 17:57 MET Coder@ReallySoft.de>        //
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

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, AW_BOOL popup);
void awt_refresh_all_pt_server_selection_lists();

/***********************    Tables Fields       ************************/

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws,const char *varname);

void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws,const char *tablename, const char *varname);
// if tablename == 0 take fields from species table !!!!

AW_window *AWT_create_tables_admin_window(AW_root *aw_root,GBDATA *gb_main);

/***********************    SAIS        ************************/
void *awt_create_selection_list_on_extendeds(GBDATA *gb_main,AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL) = 0, AW_CL filter_cd = 0,
                                             AW_BOOL add_sel_species= AW_FALSE);
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

/***********************    FILTERS     ************************/
AW_CL   awt_create_select_filter(AW_root *aw_root,GBDATA *gb_main, const char *def_name);
/* Create a data structure for filters (needed for awt_create_select_filter_win */
/* Create a filter selection box, this box needs 3 AWARS:
   1. "$def_name"
   2. "$def_name:/name=/filter"
   3. "$def_name:/name=/alignment"
   and some internal awars
*/
void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main,AW_root *awr, const char *awar_name);

AW_window *awt_create_select_filter_win(AW_root *aw_root,AW_CL res_of_create_select_filter);
/* not just the box, but the whole window */

class AP_filter;
AP_filter *awt_get_filter(AW_root *aw_root,AW_CL res_of_create_select_filter);

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix);

#define AWT_NDS_FILTER (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_PARS_FILTER (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_STRING_FILTER (1<<GB_STRING)|(1<<GB_BITS)|(1<<GB_LINK)

/***********************    species fields     ************************/

void AWT_popup_select_species_field_window(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main);

#else
#error awt_sel_boxes.hxx included twice
#endif // AWT_SEL_BOXES_HXX

