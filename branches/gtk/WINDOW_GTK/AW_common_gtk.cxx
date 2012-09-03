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
#include <gdk/gdkgc.h>
#include <gdk/gdkpixmap.h>
void AW_common_gtk::install_common_extends_cb(AW_window *aww, AW_area area) {
    GTK_NOT_IMPLEMENTED;
}

AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this);
}

void AW_GC::set_font(const AW_font font_nr, const int size, int *found_size) {
    GTK_NOT_IMPLEMENTED;
}

//FIXME initialize gc
AW_GC_gtk::AW_GC_gtk(AW_common *common) : AW_GC(common) {

    //FIXME hack
    //It is not possible to create a gc without a drawable.
    //The gc can only be used to draw on drawables which use the same colormap and depth
    GdkPixmap *temp = gdk_pixmap_new(0, 3000, 3000, 24); //FIXME memory leak
    gc = gdk_gc_new(temp);

}
AW_GC_gtk::~AW_GC_gtk(){};

void AW_GC_gtk::wm_set_foreground_color(AW_rgb col){

    GdkColor color;
    color.pixel = col;
    gdk_gc_set_foreground(gc, &color);
}
void AW_GC_gtk::wm_set_function(AW_function mode){
    GTK_NOT_IMPLEMENTED;
}
void AW_GC_gtk::wm_set_lineattributes(short lwidth, AW_linestyle lstyle){
    GdkLineStyle lineStyle;

    switch(lstyle) {
        case AW_SOLID:
            lineStyle = GDK_LINE_SOLID;
            break;
        case AW_DASHED:
        case AW_DOTTED:
            //FIXME gdk does not support dotted lines?? cant imagine that
            lineStyle = GDK_LINE_ON_OFF_DASH;
            break;
        default:
            lineStyle = GDK_LINE_SOLID;
            break;
    }

    gdk_gc_set_line_attributes(gc, lwidth, lineStyle, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
}
void AW_GC_gtk::wm_set_font(AW_font font_nr, int size, int *found_size){
    GTK_NOT_IMPLEMENTED;
}

int AW_GC_gtk::get_available_fontsizes(AW_font font_nr, int *available_sizes) const {
    //FIXME font stuff
    GTK_NOT_IMPLEMENTED;
    return 0;
}
