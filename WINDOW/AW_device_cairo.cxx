// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.hxx                               //
//   Purpose   : A device to print on a cairo surface            //
//                                                               //
//   Coded by Elmar Pruesse epruesse@mpi-bremen.de on May 5th 2013 //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <cairo.h>
#include "aw_position.hxx"
#include "aw_device.hxx"
#include "aw_device_cairo.hxx"

using namespace AW;

//#define TRACE_DRAWING
#ifdef TRACE_DRAWING
#define TRACE(format, ...)                              \
    printf("%2i %s" format "\n", gc, __func__, ##__VA_ARGS__);   \
    usleep(5000)
#define cairo_stroke for (int i=0; i<1000; i++) cairo_stroke
#define pango_cairo_show_layout for (int i=0; i<1000; i++) pango_cairo_show_layout
#define cairo_fill_preserve for (int i=0; i<1000; i++) cairo_fill_preserve

#else
#define TRACE(format, ...)
#endif

AW_device_cairo::AW_device_cairo(AW_common *awc) : AW_device(awc) {}
AW_device_cairo::~AW_device_cairo() {};

bool AW_device_cairo::line_impl(int gc, const LineVector& Line, AW_bitset filteri) 
{
    if (! (filteri & filter)) return false;
    LineVector transLine = transform(Line);
    LineVector clippedLine;
    if (!clip(transLine, clippedLine)) return false;
    clippedLine = transLine;

    aw_assert(clippedLine.valid());

    cairo_t *cr = get_cr(gc);
    if (!cr) return true;

    // In cairo, userspace coordinates are "between" pixels. To get
    // "1px wide" lines (rather than blurry 2px wide ones) 
    // we have to add 0.5 to x and y coords.
    // see also aw_zoomable.hxx@WORLD_vs_PIXEL
    
    AW_pos x      = AW_INT(clippedLine.start().xpos())+.5;
    AW_pos y      = AW_INT(clippedLine.start().ypos())+.5;
    AW_pos width  = AW_INT(clippedLine.line_vector().x());
    AW_pos height = AW_INT(clippedLine.line_vector().y());

    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x+width, y+height);
    cairo_stroke(cr);

    TRACE("%i %i %i %i", (int)x, (int)y, (int)width, (int)height);

    return true;
}

//this is static
bool AW_device_cairo::draw_string_on_screen(AW_device *device, int gc, const  char *str, 
                                            size_t /* opt_str_len */, size_t start, 
                                            size_t size, AW_pos x_, AW_pos y_, 
                                            AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, 
                                            AW_CL /*cduser*/)
{
    AW_pos X, Y;
    device->transform(x_, y_, X, Y);
    aw_assert(size <= strlen(str));

    AW_device_cairo *device_cairo = DOWNCAST(AW_device_cairo*, device);
    AW_common_gtk *common = device_cairo->get_common();

    cairo_t *cr = device_cairo->get_cr(gc);
    if (!cr) return true; 

#undef AW_USE_CLIPPING
#ifdef AW_USE_CLIPPING
    // using cairo for clipping is slow and ARB doesn't
    // currently need it -> disabled
    cairo_save(cr);
    AW_screen_area cliprect = device_cairo->get_cliprect();
    cairo_rectangle(cr, 
                    (int)cliprect.l, 
                    (int)cliprect.t, 
                    (int)(cliprect.r-cliprect.l+1), 
                    (int)(cliprect.b-cliprect.t+1));
    cairo_clip(cr);
#endif

    //#define AW_PANGO_SLOW                    
#ifdef AW_PANGO_SLOW
    PangoLayout *pl = device_cairo->get_common()->map_gc(gc)->get_pl(str+start, size);

    AW_pos base   = pango_layout_get_baseline(pl)  / PANGO_SCALE;
    AW_pos x      = AW_INT(X) + 0.5;
    AW_pos y      = AW_INT(Y - base - 1) + 0.5;

    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, pl);
#else 

    AW_pos x      = AW_INT(X);
    AW_pos y      = AW_INT(Y);

    cairo_glyph_t *glyph_string;
    int            glyph_string_len;
    common->map_gc(gc)->make_glyph_string(&glyph_string, &glyph_string_len,
                                          str+start, size);

    if (glyph_string_len > 0) {
        cairo_save(cr);
        cairo_translate(cr, x, y);
        cairo_show_glyphs(cr, glyph_string, glyph_string_len);
        cairo_restore(cr);
    }
#endif
    
#ifdef AW_USE_CLIPPING
    cairo_restore(cr);
#endif

    TRACE("%i %i %s", (int)x, (int)y, str+start);
    return true;
}


bool AW_device_cairo::text_impl(int gc, const char *str, const AW::Position& pos, 
                              AW_pos alignment, AW_bitset filteri, long opt_strlen) 
{
    return text_overlay(gc, str, opt_strlen, pos, alignment, filteri, 
                        (AW_CL)this, 0.0, 0.0, draw_string_on_screen);
}

bool AW_device_cairo::box_impl(int gc, bool filled, const Rectangle& rect, AW_bitset filteri) 
{
    if (! (filteri & filter)) return false;
    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (!box_clip(transRect, clippedRect)) return false;

    cairo_t *cr = get_cr(gc);
    if (!cr) return true;

    AW_pos x      = AW_INT(clippedRect.left())  - 0.5;
    AW_pos y      = AW_INT(clippedRect.top())   - 0.5;
    AW_pos width  = AW_INT(clippedRect.width()) + 1.0; //see aw_device.hxx@WORLD_vs_PIXEL
    AW_pos height = AW_INT(clippedRect.height())+ 1.0;

    cairo_rectangle(cr, x, y, width, height);

    if (filled) {
        get_common()->update_cr(cr, gc, true);
        cairo_fill_preserve(cr);
        get_common()->update_cr(cr, gc, false);
    }
    cairo_stroke(cr);

    TRACE("%i %i %i %i %s", (int)x, (int)y, (int)width, (int)height, filled?"filled":"");

    return true;
}

bool AW_device_cairo::polygon_impl(int gc, bool filled, int npos, const AW::Position *pos, AW_bitset filteri) {
    if (! (filteri & filter)) return false;
    
    cairo_t *cr = get_cr(gc);
    if (!cr) return true;

    // move to first point
    Position transPos = transform(pos[0]);
    cairo_move_to(cr, 
                  AW_INT(transPos.xpos())+0.5,
                  AW_INT(transPos.ypos())+0.5);
    
    // draw internal lines
    for (int i = 1; i < npos; i++) {
        transPos = transform(pos[i]);
        cairo_line_to(cr,
                      AW_INT(transPos.xpos())+0.5,
                      AW_INT(transPos.ypos())+0.5);
    }
    cairo_close_path(cr); // draw line to first point
    if (filled) {
        get_common()->update_cr(cr, gc, true);
        cairo_fill_preserve(cr);
        get_common()->update_cr(cr, gc, false);
    }
    cairo_stroke(cr);

    return true;
}

bool AW_device_cairo::circle_impl(int gc, bool filled, const AW::Position& center,
                                const AW::Vector& radius, AW_bitset filteri) 
{
    aw_assert(radius.x()>0 && radius.y()>0);
    return arc_impl(gc, filled, center, radius, 0, 360, filteri);
}

bool AW_device_cairo::arc_impl(int gc, bool filled, const AW::Position& center, 
                             const AW::Vector& radius, int start_degrees, 
                             int arc_degrees, AW_bitset filteri) 
{
    if (! (filteri & filter)) return false;
    // degrees start at east side of unit circle,
    // but orientation is clockwise (because ARBs y-coordinate grows downwards)
    aw_assert(arc_degrees >= -360 && arc_degrees <= 360);

    Rectangle Box(center-radius, center+radius);
    Rectangle screen_box = transform(Box);
    if (is_outside_clip(screen_box)) return false;

    Position tcenter = transform(center);
    Vector   tradius = transform(radius);

    cairo_t *cr = get_cr(gc);
    if (!cr) return true;

    cairo_save(cr); 
    cairo_translate(cr, AW_INT(tcenter.xpos())+.5, AW_INT(tcenter.ypos())+.5);
    cairo_scale(cr, tradius.x(), tradius.y());
    cairo_set_line_width(cr, cairo_get_line_width(cr) / (tradius.x() + tradius.y())*2);

    double angle1 = start_degrees * (M_PI / 180.);
    double angle2 = (start_degrees + arc_degrees) * (M_PI / 180.);

    // cairo_arc draws in direction of increasing angle
    // switch angle1/2 if we want to draw in the other direction as
    // indicated by negative arc_degrees
    cairo_new_path(cr);
    if (arc_degrees > 0) {
        cairo_arc(cr, 0., 0., 1., angle1, angle2);
    } else {
        cairo_arc(cr, 0., 0., 1., angle2, angle1);
    }

    if (filled) {
        get_common()->update_cr(cr, gc, true);
        cairo_fill_preserve(cr);
        get_common()->update_cr(cr, gc, false);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    return true;
}

bool AW_device_cairo::invisible_impl(const AW::Position& pos, AW_bitset filteri)
{
    return generic_invisible(pos, filteri); 
}

void AW_device_cairo::clear(AW_bitset filteri) 
{
    if (! (filteri & filter)) return;

    int gc=-1; // background
    cairo_t *cr = get_cr(gc);
    if (!cr) return;
    
    cairo_paint(cr);
    TRACE("");
}

void AW_device_cairo::clear_part(const Rectangle& rect, AW_bitset filteri) 
{
    if (! (filteri & filter)) return;

    Rectangle transRect = transform(rect);
    Rectangle clippedRect;
    if (box_clip(transRect, clippedRect)) {
        int gc = -1; // background

        cairo_t *cr = get_cr(gc);
        if (!cr) return;
    
        cairo_rectangle(cr,
                        clippedRect.left(),
                        clippedRect.top(),
                        clippedRect.width() + 1,
                        clippedRect.height() +1);
        
        cairo_fill(cr);
        TRACE("%i %i %i %i",  
              (int)clippedRect.left(),
              (int)clippedRect.top(),
              (int)clippedRect.width() + 1,
              (int)clippedRect.height() +1 );
    }
}

/**
 * Calculate the precise size of the string to be rendered 
 */
int AW_device_cairo::get_string_size(int gc, const char *str, long textlen) const {
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}

void AW_device_cairo::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height,
                                AW_pos dest_x, AW_pos dest_y) {
    int gc = -1;
    cairo_t *cr = get_cr(gc);
    if (!cr) return;

    cairo_save(cr);
    cairo_set_source_surface(cr, cairo_get_target(cr), dest_x - src_x, dest_y - src_y);
    cairo_rectangle (cr, dest_x, dest_y, width, height);
    cairo_fill (cr);
    cairo_restore(cr);
    /*
    GdkRectangle rect;
    rect.x = AW_INT(src_x), rect.y = AW_INT(src_y), rect.width=AW_INT(width), rect.height=AW_INT(height);
    gdk_window_move_region(gtk_widget_get_window(prvt->drawingArea), gdk_region_rectangle(&rect),
                           AW_INT(dest_x-src_x), AW_INT(dest_y-src_y));
    */
}
