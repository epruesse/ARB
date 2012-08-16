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


//FIXME initialize gc
AW_GC_gtk::AW_GC_gtk(AW_common *common) : AW_GC(common) {}
AW_GC_gtk::~AW_GC_gtk(){};

void AW_GC_gtk::wm_set_foreground_color(AW_rgb col){
    GTK_NOT_IMPLEMENTED;
}
void AW_GC_gtk::wm_set_function(AW_function mode){
    GTK_NOT_IMPLEMENTED;
}
void AW_GC_gtk::wm_set_lineattributes(short lwidth, AW_linestyle lstyle){
    GTK_NOT_IMPLEMENTED;
}
void AW_GC_gtk::wm_set_font(AW_font font_nr, int size, int *found_size){
    GTK_NOT_IMPLEMENTED;
}

int AW_GC_gtk::get_available_fontsizes(AW_font font_nr, int *available_sizes) const {
    //FIXME font stuff
    GTK_NOT_IMPLEMENTED;
    return 0;
}
