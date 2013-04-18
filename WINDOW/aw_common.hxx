// =========================================================== //
//                                                             //
//   File      : aw_common.hxx                                 //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2011   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#pragma once

#ifndef DOWNCAST_H
#include <downcast.h>
#endif
#ifndef ARB_DEFS_H
#include <arb_defs.h>
#endif

#include "aw_device_impl.hxx"
#include <map>


#define AW_FONTINFO_CHAR_ASCII_MIN 32
#define AW_FONTINFO_CHAR_ASCII_MAX 127

#define GC_DEFAULT_LINE_WIDTH 1

class AW_common;

class AW_GC_config { // variable part of AW_GC
protected:
    AW_function   function;
    AW_grey_level grey_level;
    short         line_width;
    AW_linestyle  style;

public:
    AW_GC_config()
        : function(AW_COPY),
          grey_level(0),
          line_width(GC_DEFAULT_LINE_WIDTH),
          style(AW_SOLID)
    {}

    AW_function get_function() const { return function; }

    // fill style
    AW_grey_level get_grey_level() const { return grey_level; }
    void set_grey_level(AW_grey_level grey_level_);

    // lines
    short get_line_width() const { return line_width; }
    AW_linestyle get_line_style() const { return style; }

    bool operator == (const AW_GC_config& other) const {
        return
            function == other.function &&
            grey_level == other.grey_level &&
            line_width == other.line_width &&
            style == other.style;
    }
};


/**
 * Most attributes of graphics operations are stored in Graphic Contexts
 *  (GCs). These include line width, line style, plane mask,
 *  foreground, background, tile, stipple, clipping region,
 *  end style, join style, and so on. Graphics operations
 *  (for example, drawing lines) use these values to determine
 *  the actual drawing operation.
 */
class AW_GC : public AW_GC_config, virtual Noncopyable {
    AW_common    *common;
    AW_GC_config *default_config; // NULL means "no special default"

    // foreground color
    AW_rgb color;
    AW_rgb last_fg_color; // effective color (as modified by 'function')

    // font
    AW_font_limits         font_limits;
    mutable AW_font_limits one_letter;

    short width_of_chars[256];
    short ascent_of_chars[256];
    short descent_of_chars[256];

    short   fontsize;
    AW_font fontnr;

    void init_char_widths() {
        memset(width_of_chars, 0, ARRAY_ELEMS(width_of_chars)*sizeof(*width_of_chars));
        memset(ascent_of_chars, 0, ARRAY_ELEMS(ascent_of_chars)*sizeof(*ascent_of_chars));
        memset(descent_of_chars, 0, ARRAY_ELEMS(descent_of_chars)*sizeof(*descent_of_chars));
    }

    void set_effective_color();

    virtual void wm_set_foreground_color(AW_rgb col)                      = 0;
    virtual void wm_set_function(AW_function mode)                        = 0;
    virtual void wm_set_lineattributes(short lwidth, AW_linestyle lstyle) = 0;
    virtual void wm_set_font(AW_font font_nr, int size, int *found_size)  = 0;
    virtual int get_actual_string_size(const char */*str*/) const {return 0;};

protected:

    void set_char_size(int i, int ascent, int descent, int width) {
        ascent_of_chars[i]  = ascent;
        descent_of_chars[i] = descent;
        width_of_chars[i]   = width;
        font_limits.notify_all(ascent, descent, width);
    }
    void set_no_char_size(int i) {
        ascent_of_chars[i]  = 0;
        descent_of_chars[i] = 0;
        width_of_chars[i]   = 0;
    }

public:
    AW_GC(AW_common *common_)
        : common(common_),
          default_config(NULL),
          color(0),
          last_fg_color(0),
          fontsize(-1), 
          fontnr(-1) 
    {
        init_char_widths();
    }
    virtual ~AW_GC() { delete default_config; }

    virtual int get_available_fontsizes(AW_font font_nr, int *available_sizes) const = 0;

    AW_common *get_common() const { return common; }

    const AW_font_limits& get_font_limits() const { return font_limits; }
    const AW_font_limits& get_font_limits(char c) const {
        aw_assert(c); // you want to use the version w/o parameter
        one_letter.ascent  = ascent_of_chars[safeCharIndex(c)];
        one_letter.descent = descent_of_chars[safeCharIndex(c)];
        one_letter.width   = width_of_chars[safeCharIndex(c)];
        one_letter.calc_height();
        return one_letter;
    }

    short get_width_of_char(char c) const { return width_of_chars[safeCharIndex(c)]; }
    short get_ascent_of_char(char c) const { return ascent_of_chars[safeCharIndex(c)]; }
    short get_descent_of_char(char c) const { return descent_of_chars[safeCharIndex(c)]; }

    int get_string_size(const char *str, long textlen) const;

    // foreground color
    AW_rgb get_fg_color() const { return color; }
    AW_rgb get_last_fg_color() const { return last_fg_color; }
    void set_fg_color(AW_rgb col) { color = col; set_effective_color(); }

    void set_function(AW_function mode) {
        if (function != mode) {
            wm_set_function(mode);
            function = mode;
            set_effective_color();
        }
    }

    // lines
    void set_line_attributes(short new_width, AW_linestyle new_style) {
        aw_assert(new_width >= 1);
        if (new_style != style || new_width != line_width) {
            line_width = new_width;
            style      = new_style;
            wm_set_lineattributes(line_width, style);
        }
    }

    // font
    void set_font(AW_font font_nr, int size, int *found_size);
    short get_fontsize() const { return fontsize; }
    AW_font get_fontnr() const { return fontnr; }

    void establish_default() {
        aw_assert(!default_config); // can't establish twice
        default_config = new AW_GC_config(*this);
        aw_assert(!(*default_config == AW_GC_config())); // you are trying to store the general default
    }
    void apply_config(const AW_GC_config& conf) {
        set_line_attributes(conf.get_line_width(), conf.get_line_style());
        set_grey_level(conf.get_grey_level());
        set_function(conf.get_function());
    }
    void reset() { apply_config(default_config ? *default_config : AW_GC_config()); }
};



class AW_common {
    AW_rgb*& frame_colors;
    AW_rgb*& data_colors;
    long&    data_colors_size;

    std::map<int, AW_GC*> gcmap;

    AW_screen_area screen;

    virtual AW_GC *create_gc() = 0;

public:
    AW_common(AW_rgb*& fcolors, AW_rgb*& dcolors, long& dcolors_count);
    virtual ~AW_common() {}
    //TODO no idea why this method exists
    void reset_style();

    const AW_screen_area& get_screen() const { return screen; }
    void set_screen_size(unsigned int width, unsigned int height);
    void set_screen(const AW_screen_area& screen);

    AW_rgb get_color(AW_color_idx color) const {
        return color >= AW_STD_COLOR_IDX_MAX ? data_colors[color] : frame_colors[color];
    }
    AW_rgb get_data_color(int i) const;
    int find_data_color_idx(AW_rgb color) const;
    int get_data_color_size() const;

    AW_rgb get_XOR_color() const;

    /**
     * Create a new gc with the specified number.
     */
    void new_gc(int gc);

    /**
     * Determine whether a gc with that number exists.
     */
    bool gc_mapable(int gc) const;

    /**
     * Get GC object by numeric id (readonly/const)
     */
    const AW_GC *map_gc(int gc) const;

    /**
     * Get GC object by numeric id (writable)
     */
    AW_GC *map_mod_gc(int gc);

    const AW_font_limits& get_font_limits(int gc, char c) const;
};

inline AW_pos x_alignment(AW_pos x_pos, AW_pos x_size, AW_pos alignment) { return x_pos - x_size*alignment; }

inline void AW_GC::set_effective_color() {
    AW_rgb col = color^(function == AW_XOR ? common->get_XOR_color(): AW_rgb(0));
    if (col != last_fg_color) {
        last_fg_color = col;
        wm_set_foreground_color(col);
    }
}
