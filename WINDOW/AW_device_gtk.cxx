// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.cxx                               //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de Aug 2012     //
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

// workaround for older GTK 2 versions
#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 20
#define gtk_widget_get_realized(widget) GTK_WIDGET_REALIZED(widget)
#endif

using namespace AW;

struct AW_device_gtk::Pimpl {
    cairo_t *cr; // owned
    int last_gc;
    GtkWidget* drawingArea;
    

    Pimpl(GtkWidget* w) : cr(NULL), last_gc(-1), drawingArea(w) {}
    ~Pimpl() {
        if (cr) cairo_destroy(cr);
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
    //gtk_widget_set_double_buffered(gtk_drawingArea, false);
}

AW_device_gtk::~AW_device_gtk() {
    delete prvt;
}

void AW_device_gtk::set_cr(cairo_t* cr) {
    if (prvt->cr)
        cairo_destroy(prvt->cr);
    prvt->cr = cr;
    prvt->last_gc = -2;
}

cairo_t* AW_device_gtk::get_cr(int gc) {
    //set_cr(0);
    // if we have no cr, make a new one
    if (!prvt->cr) {
        if (!gtk_widget_get_realized(prvt->drawingArea)) return NULL;
        set_cr(gdk_cairo_create(gtk_widget_get_window(prvt->drawingArea)));
    }

    /*
      // doesn't work ... gc may have changed in the mean time
    if (gc != prvt->last_gc) {
        // update with settings from gc
        get_common()->update_cr(prvt->cr, gc, false);
        prvt->last_gc = gc;
    }
    */
    get_common()->update_cr(prvt->cr, gc, false);

    return prvt->cr;
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
