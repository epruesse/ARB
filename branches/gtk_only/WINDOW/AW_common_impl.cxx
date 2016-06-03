// ============================================================= //
//                                                               //
//   File      : AW_common_gtk.cxx                               //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de Aug 2012     //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_common_impl.hxx"
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
struct layout_cache : virtual Noncopyable {
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
    
    PangoLayout* get(const char* text, int size) {
        for (layout_vec::iterator it = content.begin(); 
             it != content.end(); ++it) {
            if (!*it) continue;
            if (size != -1 && size != (int)strlen(text)) continue;
            if (!strcmp(pango_layout_get_text(*it), text)) {
                return *it;
            }
        }
        // not found, replace
        if (!*cur) {
            *cur = pango_layout_new(context);
            pango_layout_set_font_description(*cur, font_desc);
        }
        pango_layout_set_text(*cur, text, size);
        PangoLayout *rval = *cur;
        if (++cur == content.end()) cur = content.begin();

        return rval;
    }
};


struct AW_common_gtk::Pimpl : virtual ::Noncopyable {
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

AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this);
}

void AW_common_gtk::update_cr(cairo_t* cr, int gc, bool use_grey) {
    const AW_GC_gtk *awgc = map_gc(gc);

    // set antialias
    cairo_set_antialias(cr, make_cairo_antialias(get_default_aa()));

    // set color
    AW_rgb_normalized col(awgc->get_fg_color());
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
    const double dotted[] = {2.0}; // 1px dots merge into line using aa
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

    // set font
    if (awgc->get_scaled_font()) { // have a font
        cairo_set_scaled_font(cr, awgc->get_scaled_font());
    }
}

const int ASCII_CHAR_COUNT = AW_FONTINFO_CHAR_ASCII_MAX - AW_FONTINFO_CHAR_ASCII_MIN;

struct AW_GC_gtk::Pimpl : virtual ::Noncopyable {
    layout_cache                cache;
    PangoGlyphInfo              ascii_glyphs[ASCII_CHAR_COUNT + 1];
    cairo_scaled_font_t        *scaled_font;

    std::vector<char>           last_string;
    std::vector<cairo_glyph_t>  glyph_store; 
    int                         last_string_width;


    Pimpl() 
        : cache(200),
          scaled_font(NULL),
          last_string_width(0)
    {}

    ~Pimpl() {
        set_scaled_font(NULL);
    }

    /** set scaled font for this GC
     * manages the reference count for the pango scaled font
     * structure (old font unreferenced, new one referenced if 
     * not NULL
     */
    void set_scaled_font(cairo_scaled_font_t *font) {
        if (scaled_font) {
            cairo_scaled_font_destroy(scaled_font);
        }
        scaled_font = font;
        if (scaled_font) {
            cairo_scaled_font_reference(scaled_font);
        }
    }
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

void AW_GC_gtk::wm_set_font(const char* font_name, bool force_monospace) {
    AW_common_gtk* common = DOWNCAST(AW_common_gtk*, get_common());

    PangoFontDescription * desc = pango_font_description_from_string(font_name);
    prvt->cache.set_font(desc);

    char ascii_chars[ASCII_CHAR_COUNT+1];
    for (unsigned char i = 0; i < ASCII_CHAR_COUNT; i++) {
        ascii_chars[i] = AW_FONTINFO_CHAR_ASCII_MIN + i;
    }
    ascii_chars[ASCII_CHAR_COUNT] = 0;

    PangoLayout *pl = pango_layout_new(common->prvt->context);
    pango_layout_set_font_description(pl, desc);
    pango_layout_set_text(pl, ascii_chars, ASCII_CHAR_COUNT);

    PangoLayoutLine  *line     = pango_layout_get_line_readonly(pl, 0);
    PangoGlyphItem   *item     = (PangoGlyphItem*) line->runs->data;
    PangoGlyphString *glyphstr = item->glyphs;

    PangoFont *font = pango_font_map_load_font(common->prvt->fontmap, common->prvt->context, desc);
    prvt->set_scaled_font(pango_cairo_font_get_scaled_font((PangoCairoFont*) font));

    PangoGlyphItemIter iter;
    for (bool have_cluster = pango_glyph_item_iter_init_start(&iter, item, ascii_chars);
         have_cluster; have_cluster = pango_glyph_item_iter_next_cluster(&iter)) {
        // check that 1 char converted to 1 cluster and 1 glyph
        // (paranoia, should be the case for ascii)
        if (iter.start_char + 1 == iter.end_char &&
            iter.start_index + 1 == iter.end_index &&
            iter.start_glyph + 1 == iter.end_glyph)  {
            PangoGlyphInfo     &info = glyphstr->glyphs[iter.start_glyph];

            PangoRectangle      rect;
            pango_font_get_glyph_extents(font, info.glyph, 0, &rect);
            
            set_char_size(ascii_chars[iter.start_index], 
                          PANGO_PIXELS(PANGO_ASCENT(rect)),
                          PANGO_PIXELS(PANGO_DESCENT(rect)),
                          PANGO_PIXELS(info.geometry.width));
            prvt->ascii_glyphs[iter.start_index] = info;
        } 
        else {
            set_char_size(ascii_chars[iter.start_index], 0, 0, 0);
            prvt->ascii_glyphs[iter.start_index].glyph = 0;
        }
    }

    g_object_unref(font);
}

cairo_scaled_font_t* AW_GC_gtk::get_scaled_font() const {
    return prvt->scaled_font;
}

/**
 * Gets the current PangoLayout, updated with the supplied string if necessary.
 * We keep the PangoLayout around to make subsequents calls with the same
 * text faster.
 */
PangoLayout* AW_GC_gtk::get_pl(const char* str, int len) const {
    return prvt->cache.get(str, len);
}

void AW_GC_gtk::make_glyph_string(cairo_glyph_t **gstr, int *glen,
                                            const char* str, int len) const {
    bool is_ascii = true;
    prvt->glyph_store.reserve(len);
    prvt->last_string.reserve(len);
    double x = 0, y = 0;

    *glen = 0; // default = fail

    // try building glyph string from 
    for (int i=0; i<len; i++) {
        if (str[i] < AW_FONTINFO_CHAR_ASCII_MIN || 
            str[i] > AW_FONTINFO_CHAR_ASCII_MAX ||
            prvt->ascii_glyphs[str[i]-AW_FONTINFO_CHAR_ASCII_MIN].glyph == 0) {
            is_ascii = false;
            break;
        }
        PangoGlyphInfo &ginfo = prvt->ascii_glyphs[str[i] - AW_FONTINFO_CHAR_ASCII_MIN];
        
        prvt->glyph_store[i].index = ginfo.glyph;
        prvt->glyph_store[i].x = PANGO_PIXELS(x + ginfo.geometry.x_offset);
        prvt->glyph_store[i].y = PANGO_PIXELS(y + ginfo.geometry.y_offset);

        x += ginfo.geometry.width;
    }

    if (is_ascii) {
        prvt->last_string_width = x;
        strncpy(prvt->last_string.data(), str, len);
        
        *gstr = prvt->glyph_store.data();
        *glen  = len;
    } 
    else {
         cairo_status_t err;
         cairo_glyph_t *glyphs = prvt->glyph_store.data();
         int glyphs_len = len;
         err = cairo_scaled_font_text_to_glyphs(prvt->scaled_font, 0, 0, str, len,
                                                &glyphs, &glyphs_len, NULL, NULL, NULL);
         if (err == CAIRO_STATUS_SUCCESS) {
             if (prvt->glyph_store.data() == glyphs) {
                 *gstr = prvt->glyph_store.data();
                 *glen  = glyphs_len;
             } 
             else {
                cairo_glyph_free(glyphs);
             }
         }
    }

    return;
}


int AW_GC_gtk::get_actual_string_size(const char* str) const {
    aw_warn_if_reached();
    return -666;
    /*
      PangoLayout *pl = get_pl(str, -1); // update with string if necessary

      int w, h;
      pango_layout_get_pixel_size (pl, &w, &h);
    return w;
    */
}
