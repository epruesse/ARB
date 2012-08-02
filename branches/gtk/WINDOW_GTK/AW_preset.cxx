// ============================================================= //
//                                                               //
//   File      : AW_preset.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_preset.hxx"
#include "aw_gtk_migration_helpers.hxx"


void AW_save_properties(AW_window *aw) {
    GTK_NOT_IMPLEMENTED
}


void AW_insert_common_property_menu_entries(AW_window_menu_modes *awmm) {
    GTK_NOT_IMPLEMENTED

}

void AW_insert_common_property_menu_entries(AW_window_simple_menu *awsm) {
    GTK_NOT_IMPLEMENTED
}


AW_gc_manager AW_manage_GC(AW_window *aww,
                           AW_device *device, int base_gc, int base_drag, AW_GCM_AREA area,
                           void (*changecb)(AW_window*, AW_CL, AW_CL), AW_CL  cd1, AW_CL cd2,
                           bool define_color_groups,
                           const char *default_background_color,
                           ...) {
    GTK_NOT_IMPLEMENTED
    return 0;
}


AW_window *AW_preset_window(AW_root *root) {
    GTK_NOT_IMPLEMENTED
    return 0;
}


AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager id) {
    GTK_NOT_IMPLEMENTED
    return 0;
}


AW_window *AW_create_gc_window_named(AW_root * aw_root, AW_gc_manager id_par, const char *wid, const char *windowname){
    GTK_NOT_IMPLEMENTED
    return 0;
}





