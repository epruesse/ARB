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
#include "aw_device_gtk.hxx"
#include "aw_root.hxx"

GtkWidget *AW_area_management::get_form() const {
    return GTK_WIDGET(form);
}
GtkWidget *AW_area_management::get_area() const {
    return GTK_WIDGET(area);
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
    common = new AW_common_gtk(pDisplay, area, root->getColorTable(), aww->color_table, aww->color_table_size, aww, ar);
    screen_device = new AW_device_gtk(common, area);

}

AW_device_gtk *AW_area_management::get_screen_device() {

    return screen_device;
}


