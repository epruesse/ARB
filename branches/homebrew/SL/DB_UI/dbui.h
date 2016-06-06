// ============================================================ //
//                                                              //
//   File      : dbui.h                                         //
//   Purpose   : Encapsulate query and info user interface      //
//               (atm only for species)                         //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef DBUI_H
#define DBUI_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ITEMS_H
#include <items.h>
#endif

// @@@ rename the functions below

class AW_window_simple;
class AW_window_simple_menu;

namespace DBUI {

    AW_window *create_fields_reorder_window(AW_root *root, BoundItemSel *bound_selector);
    AW_window *create_field_delete_window(AW_root *root, BoundItemSel *bound_selector);
    AW_window *create_field_create_window(AW_root *root, BoundItemSel *bound_selector);

    AW_window *create_species_query_window(AW_root *aw_root, GBDATA *gb_main);

    void popup_species_info_window(AW_root *aw_root, GBDATA *gb_main);
    void popup_organism_info_window(AW_root *aw_root, GBDATA *gb_main);

    void insert_field_admin_menuitems(AW_window *aws, GBDATA *gb_main);

    void create_dbui_awars(AW_root *aw_root);

    void init_info_window(AW_root *aw_root, AW_window_simple_menu *aws, const ItemSelector& itemType, int detach_id);

    void unquery_all();
    void query_update_list();
};

#else
#error dbui.h included twice
#endif // DBUI_H
