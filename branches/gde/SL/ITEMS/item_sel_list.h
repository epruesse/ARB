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
#ifndef _GLIBCXX_STRING
#include <string>
#endif

#define PSEUDO_FIELD_ANY_FIELD  "[any field]"
#define PSEUDO_FIELD_ALL_FIELDS "[all fields]"

enum SelectableFields {
    SF_STANDARD  = 0,  // normal fields
    SF_PSEUDO    = 1,  // pseudo-fields (see defines above)
    SF_HIDDEN    = 2,  // fields hidden by user
    SF_ALLOW_NEW = 4,  // allow on-the-fly creation of new fields

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
    long             type_filter;
    SelectableFields field_filter;
    ItemSelector&    selector;

    bool shall_display_type(int key_type) const { return type_filter & (1 << key_type); }

public:
    Itemfield_Selection(AW_selection_list *sellist_,
                        GBDATA            *gb_key_data,
                        long               type_filter_,
                        SelectableFields   field_filter_,
                        ItemSelector&      selector_);

    void fill() OVERRIDE;

    ItemSelector& get_selector() const { return selector; }
    long get_type_filter() const { return type_filter; }
};

class FieldSelDef {
    std::string       awar_name;
    GBDATA           *gb_main;
    ItemSelector&     selector;
    long              type_filter;
    SelectableFields  field_filter;

public:
    FieldSelDef(const char *awar_name_, GBDATA *gb_main_, ItemSelector& selector_, long type_filter_, SelectableFields field_filter_ = SF_STANDARD)
    /*! parameter collection for itemfield-selections
     * @param awar_name_           the name of the awar bound to the selection list
     * @param gb_main_             the database
     * @param selector_            describes the item type, for which fields are shown
     * @param type_filter_         is a bitstring which controls what types will be shown in the selection list (several filters are predefined: 'FIELD_FILTER_...', FIELD_UNFILTERED)
     * @param field_filter_        controls if pseudo-fields and/or hidden fields are added and whether new fields are allowed (defaults to SF_STANDARD)
     */
        : awar_name(awar_name_),
          gb_main(gb_main_),
          selector(selector_),
          type_filter(type_filter_),
          field_filter(field_filter_)
    {}
    FieldSelDef(const FieldSelDef& other)
        : awar_name(other.awar_name),
          gb_main(other.gb_main),
          selector(other.selector),
          type_filter(other.type_filter),
          field_filter(other.field_filter)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(FieldSelDef);

    const char *get_awarname() const { return awar_name.c_str(); }
    long get_type_filter() const { return type_filter; }
    bool new_fields_allowed() const { return field_filter & SF_ALLOW_NEW; }

    Itemfield_Selection *build_sel(AW_selection_list *from_sellist) const;

#if defined(ASSERTION_USED)
    FieldSelDef();
    bool matches4reuse(const FieldSelDef& other);
#endif
};

enum FailIfField {
    FIF_NAME_SELECTED     = 1, // fail if 'name' field selected
    FIF_NO_FIELD_SELECTED = 2, // fail if NO_FIELD_SELECTED

    // use-cases:
    FIF_STANDARD   = FIF_NAME_SELECTED|FIF_NO_FIELD_SELECTED,
    FIF_ALLOW_NONE = FIF_STANDARD^FIF_NO_FIELD_SELECTED,
};

Itemfield_Selection *create_itemfield_selection_list(  AW_window *aws, const FieldSelDef& selDef, const char *at);
void                 create_itemfield_selection_button(AW_window *aws, const FieldSelDef& selDef, const char *at);
const char          *prepare_and_get_selected_itemfield(AW_root *awr, const char *awar_name, GBDATA *gb_main, const ItemSelector& itemtype, const char *description = "target", FailIfField failIf = FIF_STANDARD);

enum RescanMode {
    RESCAN_REFRESH  = 1, // scan database for unregistered/unused fields and update the field list
    RESCAN_SHOW_ALL = 2, // unhide all hidden fields
};

// @@@ generalize (use BoundItemSel)

void species_field_selection_list_rescan(GBDATA *gb_main, RescanMode mode);
void gene_field_selection_list_rescan   (GBDATA *gb_main, RescanMode mode);

void species_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main);
void species_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main);

void gene_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main);
void gene_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main);

void experiment_field_selection_list_unhide_all_cb(AW_window*, GBDATA *gb_main);
void experiment_field_selection_list_update_cb    (AW_window*, GBDATA *gb_main);

#else
#error item_sel_list.h included twice
#endif // ITEM_SEL_LIST_H

