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

// @@@ rename the functions below

namespace DBUI {

    AW_window *create_fields_reorder_window(AW_root *root, AW_CL cl_bound_item_selector);
    AW_window *create_field_delete_window(AW_root *root, AW_CL cl_bound_item_selector);
    AW_window *create_field_create_window(AW_root *root, AW_CL cl_bound_item_selector);

    AW_window *create_species_query_window(AW_root *aw_root, AW_CL cl_gb_main);

    AW_window *create_species_info_window(AW_root *aw_root, AW_CL cl_gb_main);
    AW_window *create_organism_info_window(AW_root *aw_root, AW_CL cl_gb_main);
    void       detach_info_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information);

    void insert_field_admin_menuitems(AW_window *aws, GBDATA *gb_main);

    void create_dbui_awars(AW_root *aw_root, AW_default aw_def);

    void unquery_all();
    void query_update_list();

};

#else
#error dbui.h included twice
#endif // DBUI_H