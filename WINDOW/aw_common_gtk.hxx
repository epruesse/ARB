// ============================================================= //
//                                                               //
//   File      : aw_common_gtk.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
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

    virtual AW_GC   *create_gc() OVERRIDE;

    const AW_GC_gtk *map_gc(int gc) const { return DOWNCAST(const AW_GC_gtk*, AW_common::map_gc(gc)); }
    AW_GC_gtk       *map_mod_gc(int gc) { return DOWNCAST(AW_GC_gtk*, AW_common::map_mod_gc(gc)); }

    GtkWidget  *get_window() const;
    GtkWidget  *get_drawing_target(); 

    void update_cr(cairo_t *cr, int gc, bool use_grey=false);
    PangoFontDescription* get_font(int gc);
};

class AW_GC_gtk : public AW_GC { // derived from Noncopyable
    struct Pimpl;
    Pimpl *prvt;

    virtual void wm_set_font(const char* fontname) OVERRIDE;
    virtual int get_actual_string_size(const char *str) const OVERRIDE;
public:
    AW_GC_gtk(AW_common *common);
    ~AW_GC_gtk() OVERRIDE;

    PangoLayout *get_pl(const char*, int) const;

};
