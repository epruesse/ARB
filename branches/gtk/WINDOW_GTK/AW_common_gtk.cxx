// ============================================================= //
//                                                               //
//   File      : AW_common_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_common_gtk.hxx"
#include "aw_gtk_migration_helpers.hxx"

void AW_common_gtk::install_common_extends_cb(AW_window *aww, AW_area area) {
    GTK_NOT_IMPLEMENTED;
}

AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this);
}
