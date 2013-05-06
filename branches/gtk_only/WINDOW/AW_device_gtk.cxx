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

AW_DEVICE_TYPE AW_device_gtk::type() { return AW_DEVICE_SCREEN; }

AW_device_gtk::AW_device_gtk(AW_common *commoni, GtkWidget *gtk_drawingArea) :
        AW_device_cairo(commoni),
        drawingArea(gtk_drawingArea)
{
    aw_assert(gtk_drawingArea != NULL);
    aw_assert(commoni != NULL);
    gtk_widget_set_app_paintable(gtk_drawingArea, true);
}

cairo_t* AW_device_gtk::get_cr(int gc) {
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(drawingArea));
    if (!cr) {
        // Sometimes the window isn't there yet when ARB already 
        // tries to draw on it. That's not possible. Return 
        // NULL (drawing primitives will catch this)
        return NULL;
    }
    get_common()->update_cr(cr, gc);
    return cr;
}

PangoLayout* AW_device_gtk::get_pl(int gc) {
    PangoLayout *pl = gtk_widget_create_pango_layout(drawingArea, NULL);
    pango_layout_set_font_description(pl, get_common()->get_font(gc));
    return pl;
}



void AW_device_gtk::flush() 
{
    //gdk_flush();
}


void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) 
{
    GdkRectangle rect;
    rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
#if GTK_MAJOR_VERSION >2
#else
    gdk_window_move_region(gtk_widget_get_window(drawingArea), gdk_region_rectangle(&rect),
                           AW_INT(dest_x-src_x), AW_INT(dest_y-src_y));
#endif
    AUTO_FLUSH(this);
}
