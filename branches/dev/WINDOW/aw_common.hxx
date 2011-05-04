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

#ifndef AW_COMMON_HXX
#define AW_COMMON_HXX

#ifndef AW_DEVICE_HXX
#include "aw_device.hxx"
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif

#define AW_INT(x) (((x)>=0) ? (int) ((x)+.5) : (int)((x)-.5))

#define AW_FONTINFO_CHAR_ASCII_MIN 32
#define AW_FONTINFO_CHAR_ASCII_MAX 127

class AW_common;

class AW_GC : virtual Noncopyable {
    AW_common *common;

    // font
    AW_font_limits         font_limits;
    mutable AW_font_limits one_letter;
    short                  width_of_chars[256];
    short                  ascent_of_chars[256];
    short                  descent_of_chars[256];

    // colors
    short         color;
    unsigned long last_fg_color; // effective color (as modified by 'function')
    AW_pos        grey_level;
    AW_function   function;

    // lines
    short        line_width;
    AW_linestyle style;

    // font
    short   fontsize;
    AW_font fontnr;

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
          color(0),
          last_fg_color(0),
          grey_level(0),
          function(AW_COPY),
          line_width(1),
          style(AW_SOLID)
    {}
    virtual ~AW_GC() {}

    virtual void wm_set_foreground_color(unsigned long col)                          = 0;
    virtual void wm_set_function(AW_function mode)                                   = 0;
    virtual void wm_set_lineattributes(short lwidth, AW_linestyle lstyle)            = 0;
    virtual void wm_set_font(AW_font font_nr, int size, int *found_size)             = 0;
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

    // colors
    void set_foreground_color(unsigned long color); 
    short get_color() const { return color; }
    
    unsigned long get_last_fg_color() const { return last_fg_color; }

    AW_pos get_grey_level() const { return grey_level; }
    void set_fill(AW_grey_level grey_level); 
    void set_function(AW_function function);

    // lines
    short get_line_width() const { return line_width>0 ? line_width : 1; }
    void set_line_attributes(AW_pos width, AW_linestyle style);

    // font
    void set_font(AW_font font_nr, int size, int *found_size);
    short get_fontsize() const { return fontsize; }
    AW_font get_fontnr() const { return fontnr; }
};

class AW_common : virtual Noncopyable {
    unsigned long*& frame_colors;
    unsigned long*& data_colors;
    long&           data_colors_size;

    int     ngcs;
    AW_GC **gcs;

    AW_rectangle screen;

    virtual AW_GC *create_gc() = 0;

public:
    AW_common(unsigned long*&  fcolors,
              unsigned long*&  dcolors,
              long&            dcolors_count)
        : frame_colors(fcolors),
          data_colors(dcolors),
          data_colors_size(dcolors_count)
    {
        ngcs = 8;
        gcs  = (AW_GC **)malloc(sizeof(void *)*ngcs);
        memset((char *)gcs, 0, sizeof(void *)*ngcs);

        screen.t = 0;
        screen.b = -1;
        screen.l = 0;
        screen.r = -1;
    }
    virtual ~AW_common() {
        for (int i = 0; i<ngcs; ++i) delete gcs[i];
        free(gcs);
    }

    const AW_rectangle& get_screen() const { return screen; }
    void set_screen_size(unsigned int width, unsigned int height) {
        screen.t = 0;               // set clipping coordinates
        screen.b = height;
        screen.l = 0;
        screen.r = width;
    }
    void set_screen(const AW_rectangle& screen_) {
        // set clipping coordinates
        screen = screen_;
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
    const AW_GC *map_gc(int gc) const { aw_assert(gc_mapable(gc)); return gcs[gc]; }
    AW_GC *map_mod_gc(int gc) { aw_assert(gc_mapable(gc)); return gcs[gc]; }

    const AW_font_limits& get_font_limits(int gc, char c) const {
        // for one characters (c == 0 -> for all characters)
        return c
            ? map_gc(gc)->get_font_limits(c)
            : map_gc(gc)->get_font_limits();
    }
};

inline AW_pos x_alignment(AW_pos x_pos, AW_pos x_size, AW_pos alignment) { return x_pos - x_size*alignment; }


#else
#error aw_common.hxx included twice
#endif // AW_COMMON_HXX
