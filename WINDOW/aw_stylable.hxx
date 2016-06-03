/* 
 * File:   AW_stylable.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:28 PM
 */

#pragma once


#include "aw_font_limits.hxx"
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#include "aw_base.hxx"
#include "aw_rgb.hxx"
class AW_common;


enum AW_linestyle {
    AW_SOLID,
    AW_DASHED,
    AW_DOTTED, 
};

enum AW_function {
    AW_COPY,
    AW_XOR
};



class AW_stylable : virtual Noncopyable {
    AW_common *common;
public:
    AW_stylable(AW_common *common_) : common(common_) {}
    virtual ~AW_stylable() {};
    
    AW_common *get_common() const { return common; }

    void new_gc(int gc);
    void set_grey_level(int gc, AW_grey_level grey_level); 
    void set_font(int gc, const char* fontname, bool force_monospace=false);
    /**
     *
     * Set width and linestyle of the selected graphic context.
     * @param gc index of the gc to modify. Aw_common holds a list of all available graphic contexts.
     */
    void set_line_attributes(int gc, short width, AW_linestyle style);
    void set_function(int gc, AW_function function);
    void establish_default(int gc);
    void set_foreground_color(int gc, AW_rgb);
    void set_foreground_color(int gc, const char *color) { set_foreground_color(gc, AW_rgb(color)); }
    virtual int  get_string_size(int gc, const  char *string, long textlen) const = 0;

    const AW_font_limits& get_font_limits(int gc, char c) const; // for one characters (c == 0 -> for all characters)

    void reset_style();
};
