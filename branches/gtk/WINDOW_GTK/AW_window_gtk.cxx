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
        GtkWidget* widget) {
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
