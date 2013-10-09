// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_gtk_migration_helpers.hxx"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "aw_position.hxx"
#include "aw_device.hxx"
#include "aw_device_gtk.hxx"

#if defined(WARN_TODO)
#warning change implementation of draw functions (read more)
// * filter has to be checked early (in AW_device)
//   (partially done. the filter variable is needed for 
//    click mapping as well, so we can't get rid of it 
//    easily.)
// * functions shall use Position/LineVector/Rectangle only
#endif

using namespace AW;

struct AW_device_gtk::Pimpl {
    cairo_t *cr; // owned
    PangoLayout *pl; // owned
    GtkWidget* drawingArea;

    Pimpl(GtkWidget* w) : cr(NULL), pl(NULL), drawingArea(w) {}
    ~Pimpl() {
        if (cr) cairo_destroy(cr);
        if (pl) g_object_unref(pl);
    }
private:
    Pimpl(const Pimpl&);
    Pimpl& operator=(const Pimpl&);
};

AW_device_gtk::AW_device_gtk(AW_common *commoni, GtkWidget *gtk_drawingArea) :
        AW_device_cairo(commoni),
        prvt(new Pimpl(gtk_drawingArea))
{
    aw_assert(gtk_drawingArea != NULL);
    aw_assert(commoni != NULL);
    gtk_widget_set_app_paintable(gtk_drawingArea, true);
}

AW_device_gtk::~AW_device_gtk() {
    delete prvt;
}

cairo_t* AW_device_gtk::get_cr(int gc) {
    // destroy old cr
    if (prvt->cr)
        cairo_destroy(prvt->cr);
    // create new one
    prvt->cr = gdk_cairo_create(gtk_widget_get_window(prvt->drawingArea));
    // update with settings from gc
    get_common()->update_cr(prvt->cr, gc, false);
    return prvt->cr;
}

PangoLayout* AW_device_gtk::get_pl(int gc) {
    // free old pl
    if (prvt->pl)
        g_object_unref(prvt->pl);
    // create new one
    prvt->pl = gtk_widget_create_pango_layout(prvt->drawingArea, NULL);
    // set font
    pango_layout_set_font_description(prvt->pl, get_common()->get_font(gc));
    return prvt->pl;
}

void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) 
{
    GdkRectangle rect;
    rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
    gdk_window_move_region(gtk_widget_get_window(prvt->drawingArea), gdk_region_rectangle(&rect),
                           AW_INT(dest_x-src_x), AW_INT(dest_y-src_y));
}

AW_DEVICE_TYPE AW_device_gtk::type() { 
    return AW_DEVICE_SCREEN; 
}

void AW_device_gtk::queue_draw() {
    gtk_widget_queue_draw(prvt->drawingArea);
}

void AW_device_gtk::queue_draw(const AW_screen_area& r) {
    gtk_widget_queue_draw_area(prvt->drawingArea, r.l, r.t, r.r-r.l, r.b-r.t);
}
