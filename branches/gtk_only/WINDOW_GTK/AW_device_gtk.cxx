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

bool AW_device_gtk::line_impl(int gc, const LineVector& Line, AW_bitset) {
    bool drawflag = false;

    LineVector transLine = transform(Line);
    LineVector clippedLine;
    if (!clip(transLine, clippedLine)) return false;
    
    aw_assert(clippedLine.valid());

    cairo_t *cr = get_common()->get_CR(gc);
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

bool AW_device_gtk::draw_string_on_screen(AW_device *device, int gc, const  char *str, 
                                          size_t /* opt_str_len */, size_t /*start*/, 
                                          size_t size, AW_pos x, AW_pos y, 
                                          AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, 
                                          AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x, y, X, Y);
    aw_assert(size <= strlen(str));



    AW_device_gtk *device_gtk = DOWNCAST(AW_device_gtk*, device);
    cairo_t *cr = device_gtk->get_common()->get_CR(gc);
    PangoLayout *pl = gtk_widget_create_pango_layout(device_gtk->drawingArea, str);
    pango_layout_set_font_description(pl, device_gtk->get_common()->get_font(gc));
    
    int w, h, base;
    pango_layout_get_pixel_size (pl, &w, &h);
    base = pango_layout_get_baseline(pl)  / PANGO_SCALE;

    FIXME("use ascent/descent to calculate this (if it's wrong...)");
    cairo_move_to(cr, AW_INT(X), AW_INT(Y) - base - 1); 
    pango_cairo_show_layout(cr, pl);
    
    if (str[0]=='B')
      printf("D %i-%i %i-%i %s\n", AW_INT(X), AW_INT(X)+w, AW_INT(Y) - base -1, AW_INT(Y) - base + h -1, str);
    //printf("draw string at %i,%i: '%s'\n", AW_INT(X), AW_INT(Y) - h, str);

    //cairo_rectangle(cr, AW_INT(X), AW_INT(Y) - base -1, w,  h); 
    //cairo_fill(cr);

    return true;
}


bool AW_device_gtk::text_impl(int gc, const char *str, const AW::Position& pos, 
                              AW_pos alignment, AW_bitset filteri, long opt_strlen) {

    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, 
                        (AW_CL)this, 0.0, 0.0, draw_string_on_screen);
}

bool AW_device_gtk::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) {
    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (!box_clip(transRect, clippedRect)) return false;
    cairo_t *cr = get_common()->get_CR(gc);
    cairo_rectangle(cr,
                    clippedRect.left(),
                    clippedRect.top(),
                    clippedRect.width() + 1,
                    clippedRect.height() +1);
    if (filled) cairo_fill(cr);
    return true;
}

bool AW_device_gtk::circle_impl(int gc, bool filled, const AW::Position& center, 
                                const AW::Vector& radius, AW_bitset filteri) {
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

    int             width  = AW_INT(screen_box.width());
    int             height = AW_INT(screen_box.height());
    const Position& ulc    = screen_box.upper_left_corner();
    int             xl     = AW_INT(ulc.xpos());
    int             yl     = AW_INT(ulc.ypos());
    
    // ARB -> X
    start_degrees = -start_degrees;
    arc_degrees   = -arc_degrees;

    while (start_degrees<0) start_degrees += 360;
            
    //cairo_t = get_common()->get_CR(gc);
    // gdk_draw_arc(GDK_DRAWABLE(drawingArea->window), get_common()->get_GC(gc), filled, xl, yl, width, height, 64*start_degrees, 64*arc_degrees );
    AUTO_FLUSH(this);
    return true;
}

void AW_device_gtk::clear(AW_bitset filteri) {
    gdk_window_clear(drawingArea->window);
}

void AW_device_gtk::clear_part(const Rectangle& rect, AW_bitset filteri) {
    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (box_clip(transRect, clippedRect)) {
        gdk_window_clear_area(drawingArea->window,
                              AW_INT(clippedRect.left()),
                              AW_INT(clippedRect.top()),
                              AW_INT(clippedRect.width())+1, // see aw_device.hxx@WORLD_vs_PIXEL
                              AW_INT(clippedRect.height())+1);
        //TODO the old motif code did not generate expose events on clear area
        AUTO_FLUSH(this);
    }
}


void AW_device_gtk::flush() {
    gdk_flush();
}

void AW_device_gtk::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) {
    GdkRectangle rect;
    rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
    gdk_window_move_region(drawingArea->window, gdk_region_rectangle(&rect), AW_INT(dest_x), AW_INT(dest_y));
    AUTO_FLUSH(this);
}
