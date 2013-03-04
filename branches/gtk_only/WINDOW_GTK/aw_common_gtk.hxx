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
    cairo_t *cr;
    PangoFontDescription *font_desc;

    virtual void wm_set_foreground_color(AW_rgb col) OVERRIDE;
    virtual void wm_set_function(AW_function mode) OVERRIDE;
    virtual void wm_set_lineattributes(short lwidth, AW_linestyle lstyle) OVERRIDE;
    virtual void wm_set_font(AW_font font_nr, int size, int *found_size) OVERRIDE;
 
    virtual int get_actual_string_size(const char *str) const OVERRIDE;

    /**
     * Searches for the first occurrence of 'what' in 'text' and replaces it with 'with'.
     */
    void replaceInString(const std::string& what,const std::string& with, std::string& text);
    
public:

    AW_rgb col_fg;
    AW_function mode;
    AW_linestyle lstyle;
    short lwidth;



    /**
     * Create a gc with specified pixel depth.
     * @note>: The gc can only be used to draw on surfaces that have the same pixel depth.
     */
    AW_GC_gtk(AW_common *common, GtkWidget *drawable,  int pixelDepth);
    ~AW_GC_gtk() OVERRIDE;

    // AW_GC interface (uses motif call)
    virtual int get_available_fontsizes(AW_font font_nr, int *available_sizes) const OVERRIDE;

    inline AW_common_gtk *get_common() const;

    cairo_t* get_cr() const {return cr; }
    PangoFontDescription* get_font() const { return font_desc; }
};



class AW_common_gtk: public AW_common { // derived from Noncopyable
    GdkDisplay *display;
    GtkWidget  *window;
    AW_window  *aww;
    AW_area    area;
    int pixelDepth; /** < Pixel depth of all drawing surfaces  */


public:
    AW_common_gtk(GdkDisplay *display_in,
                 GtkWidget    *window_in,
                 AW_rgb*&   fcolors,
                 AW_rgb*&   dcolors,
                 long&      dcolors_count,
                 AW_window *aww,
                 AW_area    area);

    virtual AW_GC *create_gc() OVERRIDE;

    const AW_GC_gtk *map_gc(int gc) const { return DOWNCAST(const AW_GC_gtk*, AW_common::map_gc(gc)); }
    AW_GC_gtk *map_mod_gc(int gc) { return DOWNCAST(AW_GC_gtk*, AW_common::map_mod_gc(gc)); }

    GdkDisplay *get_display() const { return display; }
    GtkWidget *get_window() const { return window; }
    GtkWidget *get_drawing_target(); 

    cairo_t *get_CR(int gc);
    PangoFontDescription* get_font(int gc) { return map_gc(gc)->get_font(); }

};

inline AW_common_gtk *AW_GC_gtk::get_common() const {
    return DOWNCAST(AW_common_gtk*, AW_GC::get_common());
}
