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
#include <vector>

#if (CAIRO_ANTIALIAS_DEFAULT == AW_AA_DEFAULT) &&           \
    (CAIRO_ANTIALIAS_NONE     == AW_AA_NONE) &&             \
    (CAIRO_ANTIALIAS_GRAY     == AW_AA_GRAY) &&             \
    (CAIRO_ANTIALIAS_SUBPIXEL == AW_AA_SUBPIXEL) &&         \
    (CAIRO_ANTIALIAS_FAST     == AW_AA_FAST) &&             \
    (CAIRO_ANTIALIAS_GOOD     == AW_AA_GOOD) &&             \
    (CAIRO_ANTIALIAS_BEST     == AW_AA_BEST)
#define make_cairo_antialias(aa) ((cairo_antialias_t)(aa))
#else 
// just in case... shouldn't happen though
cairo_antialias_t make_cairo_antialias(AW_antialias aa) {
    switch(aa) {
    default:
    case AW_AA_DEFAULT:
        return CAIRO_ANTIALIAS_DEFAULT;
    case AW_AA_NONE:
        return CAIRO_ANTIALIAS_NONE;
    case AW_AA_GRAY:
        return CAIRO_ANTIALIAS_GRAY;
    case AW_AA_SUBPIXEL:
        return CAIRO_ANTIALIAS_SUBPIXEL;
    case AW_AA_FAST:
        return CAIRO_ANTIALIAS_FAST;
    case AW_AA_GOOD:
        return CAIRO_ANTIALIAS_GOOD;
    case AW_AA_BEST:
        return CAIRO_ANTIALIAS_BEST;
    }
}
#endif


/* primitive implementation ahead */
struct layout_cache {
    typedef std::vector<PangoLayout*> layout_vec;
    layout_vec content;
    layout_vec::iterator cur;
    PangoContext *context;
    PangoFontDescription *font_desc;
    
    layout_cache(size_t size)
        : content(size, NULL) ,
          cur(content.begin()),
          context(NULL),
          font_desc(NULL)
    {}
    ~layout_cache() {
        set_font(NULL);
    }

    void set_context(PangoContext *context_) {
        context = context_;
        clear();
    }

    /* takes ownership of font */
    void set_font(PangoFontDescription *desc) {
        if (font_desc) {
            pango_font_description_free(font_desc);
        }

        font_desc = desc; 
        for (layout_vec::iterator it = content.begin(); 
             it != content.end(); ++it) {
            if (*it) {
                pango_layout_set_font_description(*it, font_desc);                
            }
        }
    }
    
    void clear() {
        for (layout_vec::iterator it = content.begin(); 
             it != content.end(); ++it) {
            if (*it) {
                g_object_unref(*it);
                *it = NULL;
            }
        }
    }
    
    PangoLayout* get(const char* text) {
        for (layout_vec::iterator it = content.begin(); 
             it != content.end(); ++it) {
            if (!*it) continue;
            if (!strcmp(pango_layout_get_text(*it), text)) {
                return *it;
            }
        }
        // not found, replace
        if (!*cur) {
            *cur = pango_layout_new(context);
            pango_layout_set_font_description(*cur, font_desc);
        }
        pango_layout_set_text(*cur, text, -1);
        PangoLayout *rval = *cur;
        if (++cur == content.end()) cur = content.begin();

        return rval;
    }
};


struct AW_common_gtk::Pimpl {
    GtkWidget  *window;
    AW_window  *aww;
    AW_area    area;

    PangoFontMap *fontmap;
    PangoContext *context;
    
    Pimpl() 
        : fontmap(pango_cairo_font_map_get_default()),
          context(pango_font_map_create_context(fontmap)) 
    {}
    ~Pimpl() {
        g_object_unref(context);
        g_object_unref(fontmap);
    }
};

AW_common_gtk::AW_common_gtk(GtkWidget *window,
                             AW_window *aw_window,
                             AW_area    area)
    : prvt(new Pimpl) 
{
    prvt->window  = window;
    prvt->aww     = aw_window;
    prvt->area    = area;

    // we want to always have at least background and one foreground gc
    new_gc(-1); 
    new_gc(0);
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

    // set antialias
    cairo_set_antialias(cr, make_cairo_antialias(get_default_aa()));

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

struct AW_GC_gtk::Pimpl {
    layout_cache cache;
    Pimpl() : cache(200) {}
};

AW_GC_gtk::AW_GC_gtk(AW_common *aw_common) 
    : AW_GC(aw_common),
      prvt(new Pimpl)
{
    AW_common_gtk* common = DOWNCAST(AW_common_gtk*, get_common());
    prvt->cache.set_context(common->prvt->context);
}

AW_GC_gtk::~AW_GC_gtk(){
    delete prvt;
};

void AW_GC_gtk::wm_set_font(const char* font_name) {
    AW_common_gtk* common = DOWNCAST(AW_common_gtk*, get_common());

    PangoFontDescription * desc = pango_font_description_from_string(font_name);
    prvt->cache.set_font(desc);

    // now determine char sizes; get the font structure first:
    PangoFont *font = pango_font_map_load_font(common->prvt->fontmap, common->prvt->context, desc);
    
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

    g_object_unref(font);
}

/**
 * Gets the current PangoLayout, updated with the supplied string if necessary.
 * We keep the PangoLayout around to make subsequents calls with the same
 * text faster.
 */
PangoLayout* AW_GC_gtk::get_pl(const char* str) const {
    return prvt->cache.get(str);
}

int AW_GC_gtk::get_actual_string_size(const char* str) const {
    PangoLayout *pl = get_pl(str); // update with string if necessary
   
    int w, h;
    pango_layout_get_pixel_size (pl, &w, &h);
    return w;
}
