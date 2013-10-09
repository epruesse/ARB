// ============================================================= //
//                                                               //
//   File      : AW_common_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_common_gtk.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string>
#include <algorithm>

struct AW_common_gtk::Pimpl {
    GtkWidget  *window;
    AW_window  *aww;
    AW_area    area;
    Pimpl() {}
};

AW_common_gtk::AW_common_gtk(GtkWidget *window,
                             AW_window *aw_window,
                             AW_area    area)
    : prvt(new Pimpl) 
{
    prvt->window  = window;
    prvt->aww     = aw_window;
    prvt->area    = area;
}

AW_common_gtk::~AW_common_gtk() {
    delete prvt;
}

GtkWidget  *AW_common_gtk::get_window() const { 
    return prvt->window; 
}

GtkWidget *AW_common_gtk::get_drawing_target() {
    return prvt->aww->get_area(prvt->area)->get_area();
} 


AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this);
}

void AW_common_gtk::update_cr(cairo_t* cr, int gc, bool use_grey) {
    const AW_GC *awgc = map_gc(gc);

    // set color
    AW_rgb col = awgc->get_fg_color();
    if (use_grey) {
        double a = awgc->get_grey_level();
        cairo_set_source_rgba(cr, col.r(), col.g(), col.b(), a);
    } 
    else {
        cairo_set_source_rgb(cr, col.r(), col.g(), col.b());
    } 

    // set line width
    cairo_set_line_width(cr, awgc->get_line_width());

    // set line style
    const double dotted[] = {1.0};
    const double dashed[] = {8.0, 3.0};
    switch (awgc->get_line_style()) {
        case AW_SOLID:
            cairo_set_dash(cr, NULL, 0, 0);
            break;
        case AW_DOTTED:
            cairo_set_dash(cr, dotted, 1, 0);
            break;
        case AW_DASHED:
            cairo_set_dash(cr, dashed, 2, 0);
            break;
        default:
          aw_assert(false);
    }
}

PangoFontDescription* AW_common_gtk::get_font(int gc) { 
    return map_gc(gc)->get_font(); 
}

AW_GC_gtk::AW_GC_gtk(AW_common *aw_common) 
    : AW_GC(aw_common),
      cr(NULL),
      font_desc(NULL)
{
}

AW_GC_gtk::~AW_GC_gtk(){};

void AW_GC_gtk::wm_set_font(const char* font_name) {
    if (font_desc) pango_font_description_free(font_desc);
    font_desc = pango_font_description_from_string(font_name);

    // now determine char sizes; get the font structure first:
    PangoFontMap *fontmap = pango_cairo_font_map_get_default();
    PangoContext *context = pango_font_map_create_context(fontmap);
    PangoFont *font = pango_font_map_load_font(fontmap, context, font_desc);
    
    // iterate through the ascii chars
    for (unsigned int j = AW_FONTINFO_CHAR_ASCII_MIN; j <= AW_FONTINFO_CHAR_ASCII_MAX; j++) {
        // make a char* and from that a gunichar which is a PangoGlyph
        char ascii[2]; ascii[0] = j; ascii[1]=0;
        PangoGlyph glyph = g_utf8_get_char(ascii);
        PangoRectangle rect;
        pango_font_get_glyph_extents(font, glyph , 0, &rect);
        pango_extents_to_pixels(&rect, 0);
        set_char_size(j, PANGO_ASCENT(rect), PANGO_DESCENT(rect), PANGO_RBEARING(rect));
    }
}


int AW_GC_gtk::get_actual_string_size(const char* str) const {
    GtkWidget *widget = get_common()->get_drawing_target();
    PangoLayout *pl = gtk_widget_create_pango_layout(widget, str);
    pango_layout_set_font_description(pl, font_desc);
    int w, h;
    pango_layout_get_pixel_size (pl, &w, &h);
    return w;
}
