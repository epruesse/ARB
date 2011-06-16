#ifndef AW_COMMN_HXX
#define AW_COMMN_HXX

#ifndef AW_DEVICE_HXX
#include "aw_device.hxx"
#endif

#ifndef X_H
#include <X11/X.h>
#endif
#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif

#define AW_INT(x) (((x)>=0) ? (int) ((x)+.5) : (int)((x)-.5))

class AW_common;

class AW_GC_Xm : virtual Noncopyable {
    mutable AW_font_limits one_letter;
    AW_font_limits         all_letters;

public:
    GC                   gc;
    AW_common           *common;
    XFontStruct          curfont;
    short                width_of_chars[256];
    short                ascent_of_chars[256];
    short                descent_of_chars[256];
    short                line_width;
    AW_linestyle         style;
    short                color;
    unsigned long        last_fg_color;
    unsigned long        last_bg_color;

    short   fontsize;
    AW_font fontnr;

    AW_function function;
    AW_pos      grey_level;

    AW_GC_Xm(AW_common *common);
    ~AW_GC_Xm();

    const AW_font_limits& get_font_limits() const { return all_letters; }
    const AW_font_limits& get_font_limits(char c) const {
        aw_assert(c); // you want to use the version w/o parameter
        one_letter.ascent  = ascent_of_chars[safeCharIndex(c)];
        one_letter.descent = descent_of_chars[safeCharIndex(c)];
        one_letter.width   = width_of_chars[safeCharIndex(c)];
        one_letter.calc_height();
        return one_letter;
    }

    void set_fill(AW_grey_level grey_level); // <0 don't fill  0.0 white 1.0 black
    void set_font(AW_font font_nr, int size, int *found_size);
    void set_lineattributes(AW_pos width, AW_linestyle style);
    void set_function(AW_function function);
    void set_foreground_color(unsigned long color);
    void set_background_color(unsigned long color);

    int get_available_fontsizes(AW_font font_nr, int *available_sizes) const;
    int get_string_size(const char *str, long textlen) const;
};


class AW_common {
    Display *display;
    XID      window_id;
    
    unsigned long*& frame_colors;
    unsigned long*& data_colors;
    long&           data_colors_size;

    int        ngcs;
    AW_GC_Xm **gcs;

    AW_rectangle screen;

public:
    AW_common(Display         *display_in,
              XID              window_id_in,
              unsigned long*&  fcolors,
              unsigned long*&  dcolors,
              long&            dcolors_count);

    void install_common_extends_cb(AW_window *aww, AW_area area); // call early

    Display *get_display() const { return display; }
    XID get_window_id() const { return window_id; }

    const AW_rectangle& get_screen() const { return screen; }
    void set_screen_size(unsigned int width, unsigned int height) {
        screen.t = 0;               // set clipping coordinates
        screen.b = height;
        screen.l = 0;
        screen.r = width;
    }

    unsigned long get_color(AW_color color) const {
        unsigned long col;
        if (color>=AW_DATA_BG) {
            col = data_colors[color];
        }
        else {
            col = frame_colors[color];
        }
        return col;
    }
    unsigned long get_data_color(int i) const { return data_colors[i]; }
    int find_data_color_idx(unsigned long color) const;
    int get_data_color_size() const { return data_colors_size; }

    unsigned long get_XOR_color() const {
        return data_colors ? data_colors[AW_DATA_BG] : frame_colors[AW_WINDOW_BG];
    }
    void new_gc(int gc);
    
    bool gc_mapable(int gc) const { return gc<ngcs && gcs[gc]; }
    const AW_GC_Xm *map_gc(int gc) const { aw_assert(gc_mapable(gc)); return gcs[gc]; }
    AW_GC_Xm *map_mod_gc(int gc) { aw_assert(gc_mapable(gc)); return gcs[gc]; }

    GC get_GC(int gc) const { return map_gc(gc)->gc; }
    const XFontStruct *get_xfont(int gc) const { return &map_gc(gc)->curfont; }
    const AW_font_limits& get_font_limits(int gc, char c) const {
        // for one characters (c == 0 -> for all characters)
        return c
            ? map_gc(gc)->get_font_limits(c)
            : map_gc(gc)->get_font_limits();
    }
};

inline AW_pos x_alignment(AW_pos x_pos, AW_pos x_size, AW_pos alignment) { return x_pos - x_size*alignment; }

#else
#error aw_commn.hxx included twice
#endif
