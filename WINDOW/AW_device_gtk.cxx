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
#include "aw_position.hxx"
#include "aw_device.hxx"
#include "gtk/gtkwidget.h"
#include "gtk/gtkstyle.h"
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
        AW_device(commoni),
        drawingArea(gtk_drawingArea)
{
    aw_assert(gtk_drawingArea != NULL);
    aw_assert(commoni != NULL);
    gtk_widget_set_app_paintable(gtk_drawingArea, true);
}

bool AW_device_gtk::line_impl(int gc, const LineVector& Line, AW_bitset) 
{
    LineVector transLine = transform(Line);
    LineVector clippedLine;
    if (!clip(transLine, clippedLine)) return false;
    
    aw_assert(clippedLine.valid());

    cairo_t *cr = get_common()->get_CR(gc);
    if (!cr) return false; // nothing to draw on

    // In cairo, userspace coordinates are "between" pixels. To get
    // "1px wide" lines (rather than blurry 2px wide ones) 
    // we have to add 0.5 to x and y coords.
    // see also aw_zoomable.hxx@WORLD_vs_PIXEL

    cairo_move_to(cr,
                  int(clippedLine.start().xpos())+0.5,
                  int(clippedLine.start().ypos())+0.5);
    cairo_line_to(cr,
                  int(clippedLine.head().xpos())+0.5,
                  int(clippedLine.head().ypos())+0.5);
    cairo_stroke(cr);
    
    AUTO_FLUSH(this);
    return true;
}

//this is static
bool AW_device_gtk::draw_string_on_screen(AW_device *device, int gc, const  char *str, 
                                          size_t /* opt_str_len */, size_t start, 
                                          size_t size, AW_pos x, AW_pos y, 
                                          AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, 
                                          AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));

    AW_device_gtk *device_gtk = DOWNCAST(AW_device_gtk*, device);
    cairo_t *cr = device_gtk->get_common()->get_CR(gc);
    if (!cr) return false; // nothing to draw on
    
    // always overlay text (for now)
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    PangoLayout *pl = gtk_widget_create_pango_layout(device_gtk->drawingArea, str+start);
    pango_layout_set_font_description(pl, device_gtk->get_common()->get_font(gc));
    
    double base = pango_layout_get_baseline(pl)  / PANGO_SCALE;

    cairo_move_to(cr, AW_INT(X), AW_INT(Y - base) - 1); 
    pango_cairo_show_layout(cr, pl);

    AUTO_FLUSH(device);
    return true;
}


bool AW_device_gtk::text_impl(int gc, const char *str, const AW::Position& pos, 
                              AW_pos alignment, AW_bitset filteri, long opt_strlen) 
{
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, 
                        (AW_CL)this, 0.0, 0.0, draw_string_on_screen);
}

bool AW_device_gtk::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) 
{
    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (!box_clip(transRect, clippedRect)) return false;
    cairo_t *cr = get_common()->get_CR(gc);
    if (!cr) return false; // nothing to draw on
    cairo_rectangle(cr,
                    clippedRect.left(),
                    clippedRect.top(),
                    clippedRect.width() + 1,
                    clippedRect.height() +1);
    if (filled) cairo_fill(cr);
    cairo_stroke(cr);
    return true;
}

bool AW_device_gtk::circle_impl(int gc, bool filled, const AW::Position& center, 
                                const AW::Vector& radius, AW_bitset filteri) 
{
    aw_assert(radius.x()>0 && radius.y()>0);
    return arc_impl(gc, filled, center, radius, 0, 360, filteri);
}

bool AW_device_gtk::arc_impl(int gc, bool filled, const AW::Position& center, 
                             const AW::Vector& radius, int start_degrees, 
                             int arc_degrees, AW_bitset filteri) {
    // degrees start at east side of unit circle,
    // but orientation is clockwise (because ARBs y-coordinate grows downwards)
    aw_assert(arc_degrees >= -360 && arc_degrees <= 360);

    Rectangle Box(center-radius, center+radius);
    Rectangle screen_box = transform(Box);
    if (is_outside_clip(screen_box)) return false;

    Position tcenter = transform(center);
    Vector   tradius = transform(radius);

    cairo_t *cr = get_common()->get_CR(gc);
    if (!cr) return false; // nothing to draw on

    cairo_save(cr); 
    cairo_translate(cr, tcenter.xpos(), tcenter.ypos());
    cairo_scale(cr, tradius.x(), tradius.y());
    cairo_set_line_width(cr, cairo_get_line_width(cr) / (tradius.x() + tradius.y())*2);
    cairo_arc(cr, 0., 0., 1.,  start_degrees * (M_PI / 180.), arc_degrees * (M_PI / 180.));
    if (filled) {
        cairo_fill(cr);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    AUTO_FLUSH(this);
    return true;
}

void AW_device_gtk::clear(AW_bitset filteri) 
{
    gdk_window_clear(drawingArea->window);
}

void AW_device_gtk::clear_part(const Rectangle& rect, AW_bitset filteri) 
{
    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (box_clip(transRect, clippedRect)) {
        gdk_window_clear_area(drawingArea->window,
                              AW_INT(clippedRect.left()),
                              AW_INT(clippedRect.top()),
                              AW_INT(clippedRect.width())+1, // see aw_zoomable.hxx@WORLD_vs_PIXEL
                              AW_INT(clippedRect.height())+1);
        //TODO the old motif code did not generate expose events on clear area
        AUTO_FLUSH(this);
    }
}


void AW_device_gtk::flush() 
{
    gdk_flush();
}

void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) 
{
    GdkRectangle rect;
    rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
    gdk_window_move_region(drawingArea->window, gdk_region_rectangle(&rect),
                           AW_INT(dest_x-src_x), AW_INT(dest_y-src_y));
    AUTO_FLUSH(this);
}
