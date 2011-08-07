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

    AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_bound_item_selector);
    AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_bound_item_selector);
    AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_bound_item_selector);

    AW_window *NTX_create_query_window(AW_root *aw_root, AW_CL cl_gb_main);

    AW_window *NT_create_species_window(AW_root *aw_root, AW_CL cl_gb_main);
    AW_window *NTX_create_organism_window(AW_root *aw_root, AW_CL cl_gb_main);
    void       NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information);

    void NT_spec_create_field_items(AW_window *aws, GBDATA *gb_main);

    void NT_create_species_var(AW_root *aw_root, AW_default aw_def);

    void NT_unquery_all();
    void NT_query_update_list();

};

#else
#error dbui.h included twice
#endif // DBUI_H
