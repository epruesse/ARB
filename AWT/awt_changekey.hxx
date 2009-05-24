// ==================================================================== //
//                                                                      //
//   File      : AWT_changekey.hxx                                      //
//   Purpose   : changekey management                                   //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AWT_CHANGEKEY_HXX
#define AWT_CHANGEKEY_HXX

/***********************    FIELD INFORMATIONS  ************************/
AW_CL awt_create_selection_list_on_scandb(GBDATA                 *gb_main,
                                          AW_window              *aws,
                                          const char             *varname,
                                          long                    type_filter,
                                          const char             *scan_xfig_label,
                                          const char             *rescan_xfig_label,
                                          const ad_item_selector *selector,
                                          size_t                  columns,
                                          size_t                  visible_rows,
                                          bool                    popup_list_in_window        = false,
                                          bool                    add_all_fields_pseudo_field = false,
                                          bool                    include_hidden_fields       = false);
/* show fields of a species / extended / gene !!!
   type filter is a bitstring which controls what types are shown in
   the selection list: e.g 1<<GB_INT || 1 <<GB_STRING enables
   ints and strings */

enum awt_rescan_mode {
    AWT_RS_SCAN_UNKNOWN_FIELDS  = 1, // scan database for unknown fields and add them
    AWT_RS_DELETE_UNUSED_FIELDS = 2, // delete all unused fields
    AWT_RS_SHOW_ALL             = 4, // unhide all hidden fields

    AWT_RS_UPDATE_FIELDS  = AWT_RS_SCAN_UNKNOWN_FIELDS|AWT_RS_DELETE_UNUSED_FIELDS
} ;

void awt_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode); /* rescan it */
void awt_gene_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);
void awt_experiment_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);

void awt_selection_list_scan_unknown_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_delete_unused_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_unhide_all_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_update_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);

void awt_gene_field_selection_list_scan_unknown_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_delete_unused_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_unhide_all_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_update_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);

void awt_experiment_field_selection_list_scan_unknown_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_delete_unused_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_unhide_all_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_update_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);

#else
#error AWT_changekey.hxx included twice
#endif // AWT_CHANGEKEY_HXX

