// ============================================================= //
//                                                               //
//   File      : aw_common_impl.hxx                              //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de Aug 2012     //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#pragma once
#include "aw_common.hxx"
#include "aw_gtk_forward_declarations.hxx"
#include <string> 
#include <cairo.h>

class AW_GC_gtk;

class AW_common_gtk: public AW_common { // derived from Noncopyable
    struct Pimpl;
    Pimpl *prvt;
    friend class AW_GC_gtk;
public:
    AW_common_gtk(GtkWidget  *window_in,
                  AW_window  *aww,
                  AW_area     area);
    ~AW_common_gtk();

    AW_GC   *create_gc() OVERRIDE;

    const AW_GC_gtk *map_gc(int gc) const { return DOWNCAST(const AW_GC_gtk*, AW_common::map_gc(gc)); }
    AW_GC_gtk       *map_mod_gc(int gc) { return DOWNCAST(AW_GC_gtk*, AW_common::map_mod_gc(gc)); }

    GtkWidget  *get_window() const;

    void update_cr(cairo_t *cr, int gc, bool use_grey=false);
    PangoFontDescription* get_font(int gc);
};

class AW_GC_gtk : public AW_GC { // derived from Noncopyable
    struct Pimpl;
    Pimpl *prvt;

    void wm_set_font(const char* fontname, bool force_monospace=false) OVERRIDE;
    int get_actual_string_size(const char *str) const OVERRIDE;
public:
    AW_GC_gtk(AW_common *common);
    ~AW_GC_gtk() OVERRIDE;

    cairo_scaled_font_t* get_scaled_font() const;
    PangoLayout   *get_pl(const char*, int) const;
    void make_glyph_string(cairo_glyph_t**, int*, const char*, int) const;

};
