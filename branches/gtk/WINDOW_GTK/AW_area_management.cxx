// ============================================================= //
//                                                               //
//   File      : AW_area_management.cxx                          //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 13, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "AW_area_management.hxx"
#include "aw_window.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "gdk/gdkscreen.h"
#include "gtk/gtkwidget.h"
#include "aw_common_gtk.hxx"
#include "aw_device_gtk.hxx"
#include "aw_root.hxx"

class AW_area_management::Pimpl {
public:
    GtkWidget *form; /**< the managing widget */
    GtkWidget *area; /**< the drawing area */

    AW_common_gtk *common;

    AW_device_gtk   *screen_device;
    AW_device_size  *size_device;
    AW_device_print *print_device;
    AW_device_click *click_device;

    AW_cb_struct *resize_cb; /**<A list of callback functions that are called whenever this area is resized. */
    AW_cb_struct *double_click_cb;

    long click_time;
    
    Pimpl() : 
        form(NULL),
        area(NULL),
        common(NULL),
        screen_device(NULL),
        size_device(NULL),
        print_device(NULL),
        click_device(NULL),
        resize_cb(NULL),
        double_click_cb(NULL){}   
};


GtkWidget *AW_area_management::get_form() const {
    return GTK_WIDGET(prvt->form);
}
GtkWidget *AW_area_management::get_area() const {
    return GTK_WIDGET(prvt->area);
}
AW_common_gtk *AW_area_management::get_common() const {
    return prvt->common; 
}


/**
 * Handles the drawing areas expose callback.
 */
static gboolean draw_area_expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer area_management)
{
    AW_area_management *aram = (AW_area_management *) area_management;
    aram->run_resize_callback();
}


void AW_area_management::set_resize_callback(AW_window *aww, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    // insert resize callback for draw_area

    //FIXME do this when handling the expose_cb.
    if (!prvt->resize_cb) {//connect the gtk signal upon first call

        //note: we use the expose callback because there is no resize callback in gtk.
        g_signal_connect (prvt->area, "expose_event",
                          G_CALLBACK (draw_area_expose_cb), (gpointer) this);

    }
    prvt->resize_cb = new AW_cb_struct(aww, f, cd1, cd2, 0, prvt->resize_cb);
}


inline void AW_area_management::run_resize_callback() {
    if (prvt->resize_cb) prvt->resize_cb->run_callback();
}

AW_area_management::AW_area_management(AW_root* awr, GtkWidget* form,
        GtkWidget* area) {
    
    prvt = new AW_area_management::Pimpl();
    prvt->form = form;
    prvt->area = area;
    GTK_PARTLY_IMPLEMENTED;
    
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
    GdkScreen* screen = gtk_widget_get_screen (prvt->area);
    GdkDisplay* pDisplay = gdk_screen_get_display(screen);
    //FIXME font stuff
    //FIXME parameter global colortable is wrong.
    prvt->common = new AW_common_gtk(pDisplay, prvt->area, root->getColorTable(), aww->color_table, aww->color_table_size, aww, ar);
    prvt->screen_device = new AW_device_gtk(prvt->common, prvt->area);

}

AW_device_gtk *AW_area_management::get_screen_device() {

    return prvt->screen_device;
}


AW_cb_struct *AW_area_management::get_double_click_cb() { return prvt->double_click_cb; }
long AW_area_management::get_click_time() const { return prvt->click_time; }
void AW_area_management::set_click_time(long click_time) { prvt->click_time = click_time; }