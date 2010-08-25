// ==================================================================== //
//                                                                      //
//   File      : awt_item_sel_list.hxx                                  //
//   Purpose   : selection lists for items (ad_item_selector)           //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#ifndef AWT_ITEM_SEL_LIST_HXX
#define AWT_ITEM_SEL_LIST_HXX

#ifndef AW_SELECT_HXX
#include <aw_select.hxx>
#endif

#define PSEUDO_FIELD_ANY_FIELD  "[any field]"
#define PSEUDO_FIELD_ALL_FIELDS "[all fields]"

enum awt_selected_fields {
    AWT_SF_STANDARD = 0,
    AWT_SF_PSEUDO   = 1,
    AWT_SF_HIDDEN   = 2,
    // continue with 4, 8
    AWT_SF_ALL      = ((AWT_SF_HIDDEN<<1)-1),
};

#define AWT_NDS_FILTER    (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_PARS_FILTER   (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_STRING_FILTER (1<<GB_STRING)|(1<<GB_BITS)|(1<<GB_LINK)

struct ad_item_selector;

class AWT_itemfield_selection : public AW_DB_selection {
    long                    type_filter;
    awt_selected_fields     field_filter;
    const ad_item_selector *selector;

    bool shall_display_type(int key_type) { return type_filter & (1 << key_type); }

public:
    AWT_itemfield_selection(AW_window              *win_,
                            AW_selection_list      *sellist_,
                            GBDATA                 *gb_key_data,
                            long                    type_filter_,
                            awt_selected_fields     field_filter_,
                            const ad_item_selector *selector_);

    void fill();

    const ad_item_selector *get_selector() const { return selector; }
};

AWT_itemfield_selection *awt_create_selection_list_on_itemfields(GBDATA                 *gb_main,
                                                                 AW_window              *aws,
                                                                 const char             *varname,
                                                                 long                    type_filter,
                                                                 const char             *scan_xfig_label,
                                                                 const char             *rescan_xfig_label,
                                                                 const ad_item_selector *selector,
                                                                 size_t                  columns,
                                                                 size_t                  visible_rows,
                                                                 awt_selected_fields     field_filter       = AWT_SF_STANDARD,
                                                                 const char             *popup_button_label = NULL);


enum awt_rescan_mode {
    AWT_RS_SCAN_UNKNOWN_FIELDS  = 1, // scan database for unknown fields and add them
    AWT_RS_DELETE_UNUSED_FIELDS = 2, // delete all unused fields
    AWT_RS_SHOW_ALL             = 4, // unhide all hidden fields

    AWT_RS_UPDATE_FIELDS  = AWT_RS_SCAN_UNKNOWN_FIELDS|AWT_RS_DELETE_UNUSED_FIELDS
};

void awt_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode); /* rescan it */
void awt_gene_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);
void awt_experiment_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);

void awt_selection_list_scan_unknown_cb(AW_window *aww, GBDATA *gb_main, long bitfilter);
void awt_selection_list_delete_unused_cb(AW_window *aww, GBDATA *gb_main, long bitfilter);
void awt_selection_list_unhide_all_cb(AW_window *aww, GBDATA *gb_main, long bitfilter);
void awt_selection_list_update_cb(AW_window *aww, GBDATA *gb_main, long bitfilter);

void awt_gene_field_selection_list_scan_unknown_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_delete_unused_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_unhide_all_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_update_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);

void awt_experiment_field_selection_list_scan_unknown_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_delete_unused_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_unhide_all_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_update_cb(AW_window *dummy, GBDATA *gb_main, long bitfilter);

#else
#error AWT_item_sel_list.hxx included twice
#endif // AWT_ITEM_SEL_LIST_HXX

