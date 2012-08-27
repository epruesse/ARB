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
#include <gdk/gdk.h>
#include "aw_common.hxx"
#include <gtk/gtkwidget.h>

class AW_common_gtk;

/**
 * All drawing operations in GDK take a graphics context (GC) argument.
 * A graphics context encapsulates information about
 * the way things are drawn, such as the foreground color or line width.
 * By using graphics contexts, the number of arguments to each drawing
 * call is greatly reduced, and communication overhead is minimized, since
 * identical arguments do not need to be passed repeatedly.
 */
class AW_GC_gtk : public AW_GC { // derived from Noncopyable
    GdkGC*      gc;
    //FIXME font stuff
    //XFontStruct curfont;

    virtual void wm_set_foreground_color(AW_rgb col);
    virtual void wm_set_function(AW_function mode);
    virtual void wm_set_lineattributes(short lwidth, AW_linestyle lstyle);
    virtual void wm_set_font(AW_font font_nr, int size, int *found_size);

public:

    AW_GC_gtk(AW_common *common);
    ~AW_GC_gtk();

    // AW_GC interface (uses motif call)
    virtual int get_available_fontsizes(AW_font font_nr, int *available_sizes) const;

    inline AW_common_gtk *get_common() const;

    GdkGC* get_gc() const { return gc; }
    //FIXME font stuff
    //const XFontStruct *get_xfont() const { return &curfont; }
};



class AW_common_gtk: public AW_common { // derived from Noncopyable
    GdkDisplay *display;
    GtkWidget  *window;

    void install_common_extends_cb(AW_window *aww, AW_area area);

public:
    AW_common_gtk(GdkDisplay *display_in,
                 GtkWidget    *window_in,
                 AW_rgb*&   fcolors,
                 AW_rgb*&   dcolors,
                 long&      dcolors_count,
                 AW_window *aww,
                 AW_area    area)
        : AW_common(fcolors, dcolors, dcolors_count),
          display(display_in),
          window(window_in)
    {
        install_common_extends_cb(aww, area);
    }

    virtual AW_GC *create_gc();

    const AW_GC_gtk *map_gc(int gc) const { return DOWNCAST(const AW_GC_gtk*, AW_common::map_gc(gc)); }
    AW_GC_gtk *map_mod_gc(int gc) { return DOWNCAST(AW_GC_gtk*, AW_common::map_mod_gc(gc)); }

    GdkDisplay *get_display() const { return display; }
    GtkWidget *get_window_id() const { return window; } //FIXME rename to get_window

    GdkGC *get_GC(int gc) const { return map_gc(gc)->get_gc(); }
    //FIXME font stuff
    //const XFontStruct *get_xfont(int gc) const { return map_gc(gc)->get_xfont(); }
};

inline AW_common_gtk *AW_GC_gtk::get_common() const {
    return DOWNCAST(AW_common_gtk*, AW_GC::get_common());
}
