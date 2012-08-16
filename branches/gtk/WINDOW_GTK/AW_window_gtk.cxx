// ============================================================= //
//                                                               //
//   File      : AW_window_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 13, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_window_gtk.hxx"
#include "aw_window.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "gdk/gdkscreen.h"
#include "gtk/gtkwidget.h"
#include "aw_common_gtk.hxx"

void AW_area_management::set_expose_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {

    GTK_NOT_IMPLEMENTED;

//    // insert expose callback for draw_area
//    if (!expose_cb) {
//        XtAddCallback(area, XmNexposeCallback, (XtCallbackProc) AW_exposeCB,
//                (XtPointer) this);
//    }
//    expose_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, expose_cb);
}


GtkWidget *AW_area_management::get_form() const {
    return GTK_WIDGET(form);
}
GtkWidget *AW_area_management::get_area() const {
    return GTK_WIDGET(area);
}

bool AW_area_management::is_expose_callback(AW_window * /* aww */, void (*f)(AW_window*, AW_CL, AW_CL)) {
    return expose_cb && expose_cb->contains(f);
}

void AW_area_management::run_expose_callback() {
    if (expose_cb) expose_cb->run_callback();
}

AW_area_management::AW_area_management(AW_root* awr, GtkWidget* form,
        GtkWidget* area) : form(form), area(area) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_area_management::is_input_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_area_management::is_double_click_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_area_management::is_motion_callback(AW_window* aww,
        void (*f)(AW_window*, AW_CL, AW_CL)) {
    GTK_NOT_IMPLEMENTED;
}

void AW_area_management::create_devices(AW_window *aww, AW_area ar) {
    AW_root *root = aww->get_root();
    GdkScreen* screen = gtk_widget_get_screen (area);
    GdkDisplay* pDisplay = gdk_screen_get_display(screen);
    //FIXME font stuff
    //FIXME parameter global colortable is wrong.
    common = new AW_common_gtk(pDisplay, area, aww->color_table, aww->color_table, aww->color_table_size, aww, ar);

}

