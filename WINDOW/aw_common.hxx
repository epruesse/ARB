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

#include "aw_rgb.hxx"

#include "aw_device_impl.hxx"
#include <map>


#define AW_FONTINFO_CHAR_ASCII_MIN 32
#define AW_FONTINFO_CHAR_ASCII_MAX 127

#define GC_DEFAULT_LINE_WIDTH 1

class AW_GC;

enum AW_antialias {
    AW_AA_DEFAULT,    // whatever is the device default
    AW_AA_NONE,       
    AW_AA_GRAY,
    AW_AA_SUBPIXEL,
    AW_AA_FAST,       // hints, let the device choose
    AW_AA_GOOD,
    AW_AA_BEST
};

class AW_common : virtual Noncopyable {
    struct Pimpl;
    Pimpl *prvt;

    virtual AW_GC *create_gc() = 0;
protected:
    AW_antialias default_aa_setting;

public:
    AW_common();
    virtual ~AW_common();

    void reset_style();

    const AW_screen_area& get_screen() const;
    void set_screen_size(unsigned int width, unsigned int height);
    void set_screen(const AW_screen_area& screen);

    void         new_gc(int gc);
    bool         gc_mapable(int gc) const;
    const AW_GC *map_gc(int gc) const;
    AW_GC       *map_mod_gc(int gc);

    void set_default_aa(AW_antialias);
    AW_antialias get_default_aa() const;

    const AW_font_limits& get_font_limits(int gc, char c) const __ATTR__DEPRECATED();
};

/**
 * Most attributes of graphics operations are stored in Graphic Contexts
 *  (GCs). These include line width, line style, plane mask,
 *  foreground, background, tile, stipple, clipping region,
 *  end style, join style, and so on. Graphics operations
 *  (for example, drawing lines) use these values to determine
 *  the actual drawing operation.
 */
class AW_GC : virtual Noncopyable {
private:
    struct Pimpl;
    Pimpl *prvt;

    virtual void wm_set_font(const char* font_name, bool force_monospace=false) = 0;
    virtual int get_actual_string_size(const char */*str*/) const {return 0;};

protected:
    struct config {
        AW_function   function;
        AW_grey_level grey_level;
        short         line_width;
        AW_linestyle  style;
        AW_rgb        color;
        config(); 
        bool operator==(const config& o) const;
    };

    config conf;
    config default_conf;

    void set_char_size(int i, int ascent, int descent, int width);
    void set_no_char_size(int i);

public:
    AW_GC(AW_common *common_);
    virtual ~AW_GC();

    void          set_function(AW_function mode);
    AW_function   get_function() const { return conf.function; }
    void          set_grey_level(AW_grey_level grey_level_);
    AW_grey_level get_grey_level() const { return conf.grey_level; }
    void          set_line_attributes(short new_width, AW_linestyle new_style);
    short         get_line_width() const { return conf.line_width; }
    AW_linestyle  get_line_style() const { return conf.style; }
    void          set_fg_color(AW_rgb col);
    AW_rgb        get_fg_color() const { return conf.color; }

    void          set_font(const char* fontname, bool force_monospace=false);

    AW_common    *get_common() const;

    bool operator == (const AW_GC& other) const { return conf == other.conf; }

    const AW_font_limits& get_font_limits() const;
    const AW_font_limits& get_font_limits(char c) const;

    short get_width_of_char(char c) const;
    short get_ascent_of_char(char c) const;
    short get_descent_of_char(char c) const;

    int get_string_size(const char *str, long textlen) const;
    int get_string_size_fast(const char *str, long textlen) const;

    void establish_default();
    void reset();
};
