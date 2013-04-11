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
#include <gdk/gdkgc.h>
#include <gdk/gdkpixmap.h>
#include <gtk/gtkwidget.h>
#include "aw_xfont.hxx"
//TODO remove if xfont is gone
#include <gdk/gdkx.h>
#include <string>
#include <algorithm>

/*
 * Is called upon window resize
 * @param cl_common_gtk Pointer to the common_gtk instance that registered this callback.
 */
static void AW_window_resize_cb(AW_window *, AW_CL cl_common_gtk, AW_CL) {
    AW_common_gtk *common = (AW_common_gtk*)cl_common_gtk;
    gint width = common->get_window()->allocation.width;
    gint height = common->get_window()->allocation.height;
    common->set_screen_size(width, height);
}

AW_common_gtk::AW_common_gtk(GdkDisplay *display_in,
             GtkWidget    *window_in,
             AW_rgb*&   fcolors,
             AW_rgb*&   dcolors,
             long&      dcolors_count,
             AW_window *aw_window,
             AW_area    area_in)
    : AW_common(fcolors, dcolors, dcolors_count),
      display(display_in),
      window(window_in),
      aww(aw_window),
      area(area_in),
      pixelDepth(gdk_screen_get_system_visual(gdk_display_get_default_screen(display))->depth)
{
    aw_window->set_resize_callback(area, AW_window_resize_cb, (AW_CL)this);
}

GtkWidget *AW_common_gtk::get_drawing_target() {
    return aww->get_area(area)->get_area(); 
} 


AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this, aww->get_area(area)->get_area(), pixelDepth);
}

cairo_t *AW_common_gtk::get_CR(int gc) {
    GtkWidget *widget = aww->get_area(area)->get_area();
    const AW_GC *awgc = map_gc(gc);

    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
    if (!cr) {
        // Sometimes the window isn't there yet when ARB already 
        // tries to draw on it. That's not possible. Return 
        // NULL (drawing primitives will catch this)
        return NULL;
    }

    // set color
    AW_rgb col = awgc->get_fg_color();
    cairo_set_source_rgb(cr, 
                         (double)((col & 0xff0000)>>16) / 255,
                         (double)((col & 0xff00)>>8) / 255,
                         (double)(col & 0xff) / 255);

    // set line width
    cairo_set_line_width(cr, awgc->get_line_width());

    // set line style
    switch (awgc->get_line_style()) {
        case AW_SOLID:
            cairo_set_dash(cr, NULL, 0, 0);
            break;
        case AW_DOTTED:
            {
                double dash[] = {1.0};
                cairo_set_dash(cr, dash, 1, 0);
            }
            break;
        case AW_DASHED:
            {
                double dash[] = {8.0, 3.0};
                cairo_set_dash(cr, dash, 2, 0);
            }
            break;
        default:
          aw_assert(false);
    }

    // set rounded caps
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); 
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 

    // set function (aka operator)
    switch (awgc->get_function()) {
        case AW_XOR:
            cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
            break;
        case AW_COPY:
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER); 
            break;
        default:
            aw_assert(false);
    }
      
  
    return cr;
  
}

AW_GC_gtk::AW_GC_gtk(AW_common *aw_common, GtkWidget *drawable, int pixelDepth) 
  : AW_GC(aw_common) 
{
}
AW_GC_gtk::~AW_GC_gtk(){};


void AW_GC_gtk::wm_set_foreground_color(AW_rgb col){
}

void AW_GC_gtk::wm_set_function(AW_function mode_in){
}

void AW_GC_gtk::wm_set_lineattributes(short lwidth_in, AW_linestyle lstyle_in){
}

void AW_GC_gtk::replaceInString(const std::string& what,const std::string& with, std::string& text){
    size_t start = text.find(what);
    if (start != std::string::npos) {
        text.replace(start, what.length(), with);
    }
}

static const char* fonts[] = {
  "Times Roman", // 0
  "Times Italic", // 1
  "Times Bold", // 2
  "Times Bold Italic", // 3
  "AvantGarde Book", // 4
  "AvantGarde Book Oblique", // 5
  "AvantGarde Demi", // 6
  "AvantGarde Demi Oblique", // 7
  "Bookman Light", // 8
  "Bookman Light Italic", // 9
  "Bookman Demi", //10
  "Bookman Demi Italic", //11
  "Courier", //12
  "Courier Oblique", //13
  "Courier Bold", //14
  "Courier Bold Oblique", //15
  "Helvetica", //16
  "Helvetica Oblique", //17
  "Helvetica Bold", //18
  "Helvetica Bold Oblique", //19
  "Helvetica Narrow", //20
  "Helvetica Narrow Oblique", //21
  "Helvetica Narrow Bold", //22
  "Helvetica Narrow Bold Oblique", //23
  "New Century Schoolbook Roman", //24
  "New Century Schoolbook Italic", //25
  "New Century Schoolbook Bold", //26
  "New Century Schoolbook Bold Italic", //27
  "Palatino Roman", //28
  "Palatino Italic", //29
  "Palatino Bold", //30
  "Palatino Bold Italic", //31
  "Symbol", //32
  "Zapf Chancery Medium Italic", //33
  "Zapf Dingbats", //34
  "Lucida Medium" //35
};


void AW_GC_gtk::wm_set_font(const AW_font font_nr, const int font_size, int *found_size) {
    // translate xfig font number to some name
    if (font_nr < -1) { 
    } else if (font_nr == -1) {
      font_desc = pango_font_description_from_string("default");
    } else if (font_nr < sizeof(fonts)) {
      font_desc = pango_font_description_from_string(fonts[font_nr]);
    } else {
      font_desc = pango_font_description_from_string("default");
    }

    // set requested size 
    if (font_size) {
        pango_font_description_set_absolute_size(font_desc, font_size * PANGO_SCALE);
        if (found_size) *found_size = font_size;
    }
    else {
      if (found_size) *found_size = 8;
    }


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

int AW_GC_gtk::get_available_fontsizes(AW_font /*font_nr*/, int */*available_sizes*/) const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int AW_GC_gtk::get_actual_string_size(const char* str) const {
    GtkWidget *widget = get_common()->get_drawing_target();
    PangoLayout *pl = gtk_widget_create_pango_layout(widget, str);
    pango_layout_set_font_description(pl, font_desc);
    int w, h;
    pango_layout_get_pixel_size (pl, &w, &h);
    return w;
}
