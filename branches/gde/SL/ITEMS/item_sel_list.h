// ==================================================================== //
//                                                                      //
//   File      : item_sel_list.h                                        //
//   Purpose   : selection lists for items (ItemSelector)               //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#ifndef ITEM_SEL_LIST_H
#define ITEM_SEL_LIST_H

#ifndef AW_SELECT_HXX
#include <aw_select.hxx>
#endif
#ifndef ITEMS_H
#include "items.h"
#endif

#define PSEUDO_FIELD_ANY_FIELD  "[any field]"
#define PSEUDO_FIELD_ALL_FIELDS "[all fields]"

enum SelectedFields {
    SF_STANDARD = 0,
    SF_PSEUDO   = 1,
    SF_HIDDEN   = 2,
    // continue with 4, 8
    SF_ALL      = ((SF_HIDDEN<<1)-1),
};


CONSTEXPR long FIELD_FILTER_STRING_WRITEABLE = (1<<GB_STRING);
CONSTEXPR long FIELD_FILTER_INT_WRITEABLE    = (1<<GB_STRING) | (1<<GB_INT);
CONSTEXPR long FIELD_FILTER_BYTE_WRITEABLE   = (1<<GB_STRING) | (1<<GB_INT) | (1<<GB_BYTE) | (1<<GB_FLOAT);
CONSTEXPR long FIELD_FILTER_FLOAT_WRITEABLE  = (1<<GB_STRING) | (1<<GB_FLOAT);

CONSTEXPR long FIELD_FILTER_STRING_READABLE = (1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_STRING)|(1<<GB_BITS)|(1<<GB_LINK); // as supported by GB_read_as_string()

CONSTEXPR long FIELD_UNFILTERED = -1L;

CONSTEXPR long FIELD_FILTER_NDS  = FIELD_FILTER_STRING_READABLE; // @@@ used too wide-spread (use FIELD_FILTER_STRING_READABLE instead)
CONSTEXPR long FIELD_FILTER_PARS = FIELD_FILTER_STRING_READABLE; // @@@ used only once (use FIELD_FILTER_STRING_READABLE instead)


class Itemfield_Selection : public AW_DB_selection { // derived from a Noncopyable
    long            type_filter;
    SelectedFields  field_filter;
    ItemSelector&   selector;

    bool shall_display_type(int key_type) const { return type_filter & (1 << key_type); }

public:
    Itemfield_Selection(AW_selection_list *sellist_,
                        GBDATA            *gb_key_data,
                        long               type_filter_,
                        SelectedFields     field_filter_,
                        ItemSelector&      selector_);

    void fill() OVERRIDE;

    ItemSelector& get_selector() const { return selector; }
};

Itemfield_Selection *create_selection_list_on_itemfields(GBDATA         *gb_main,
                                                         AW_window      *aws,
                                                         const char     *varname,
                                                         bool            fallback2default,
                                                         ItemSelector&   selector,
                                                         long            type_filter,
                                                         SelectedFields  field_filter,
                                                         const char     *scan_xfig_label,
                                                         size_t          columns,
                                                         size_t          visible_rows,
                                                         const char     *popup_button_label);


enum RescanMode {
    RESCAN_REFRESH  = 1, // scan database for unregistered/unused fields and update the field list
    RESCAN_SHOW_ALL = 2, // unhide all hidden fields
};

// @@@ generalize (use BoundItemSel)

void species_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, RescanMode mode);
void gene_field_selection_list_rescan   (GBDATA *gb_main, long bitfilter, RescanMode mode);

void species_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main, long bitfilter);
void species_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main, long bitfilter);

void gene_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main, long bitfilter);
void gene_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main, long bitfilter);

void experiment_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main, long bitfilter);
void experiment_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main, long bitfilter);

#else
#error item_sel_list.h included twice
#endif // ITEM_SEL_LIST_H

