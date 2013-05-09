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
#include "aw_xfont.hxx"
//TODO remove if xfont is gone
#include <string>
#include <algorithm>

/*
 * Is called upon window resize
 * @param cl_common_gtk Pointer to the common_gtk instance that registered this callback.
 */
static void AW_window_resize_cb(AW_window *, AW_CL cl_common_gtk, AW_CL) {
    AW_common_gtk *common = (AW_common_gtk*)cl_common_gtk;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(common->get_window()), &alloc);
    common->set_screen_size(alloc.width, alloc.height);
}

struct AW_common_gtk::Pimpl {
    GdkDisplay *display;
    GtkWidget  *window;
    AW_window  *aww;
    AW_area    area;

    Pimpl() {}
};

AW_common_gtk::AW_common_gtk(GdkDisplay *display,
             GtkWidget *window,
             AW_rgb*&   fcolors,
             AW_rgb*&   dcolors,
             long&      dcolors_count,
             AW_window *aw_window,
             AW_area    area)
    : AW_common(fcolors, dcolors, dcolors_count),
      prvt(new Pimpl) 
{
    prvt->display = display;
    prvt->window  = window;
    prvt->aww     = aw_window;
    prvt->area    = area;

    aw_window->set_resize_callback(area, AW_window_resize_cb, (AW_CL)this);
}

AW_common_gtk::~AW_common_gtk() {
    delete prvt;
}

GdkDisplay *AW_common_gtk::get_display() const { 
    return prvt->display; 
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
    double r = (double)((col & 0xff0000)>>16) / 255;
    double g = (double)((col & 0xff00)>>8) / 255;
    double b = (double)(col & 0xff) / 255;
    if (use_grey) {
        double a = awgc->get_grey_level();
        cairo_set_source_rgba(cr, r, g, b, a);
    } 
    else {
        cairo_set_source_rgb(cr, r, g, b);
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

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE); 
}

PangoFontDescription* AW_common_gtk::get_font(int gc) { 
    return map_gc(gc)->get_font(); 
}

AW_GC_gtk::AW_GC_gtk(AW_common *aw_common) 
    : AW_GC(aw_common) 
{
}

AW_GC_gtk::~AW_GC_gtk(){};


void AW_GC_gtk::wm_set_foreground_color(AW_rgb /*col*/){
}

void AW_GC_gtk::wm_set_function(AW_function /*mode_in*/){
}

void AW_GC_gtk::wm_set_lineattributes(short /*lwidth_in*/, AW_linestyle /*lstyle_in*/){
}

void AW_GC_gtk::wm_set_grey_level(AW_grey_level /*l*/) {
}

void AW_GC_gtk::replaceInString(const std::string& what,const std::string& with, std::string& text){
    size_t start = text.find(what);
    if (start != std::string::npos) {
        text.replace(start, what.length(), with);
    }
}

void AW_GC_gtk::wm_set_font(const char* font_name) {
    font_desc = pango_font_description_from_string(font_name);

    // begin acrobatics to get the glyph sizes
    // get the actual widget we're drawing on, 
    GtkWidget *widget = get_common()->get_drawing_target();
    // get its context,
    PangoContext *context = gtk_widget_get_pango_context(widget);
    // and the fontmap thereof
    PangoFontMap *fontmap = pango_context_get_font_map(context);//pango_cairo_font_map_get_default();
    // use all of those to finally get the probably used font
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
