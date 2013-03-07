/* 
 * File:   AW_stylable.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:28 PM
 */

#include "aw_stylable.hxx"
#include "aw_common.hxx"

const AW_font_limits& AW_stylable::get_font_limits(int gc, char c) const {
    return get_common()->get_font_limits(gc, c);
}

int AW_stylable::get_string_size(int gc, const char *str, long textlen) const {
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}

int AW_stylable::get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes) {
    return get_common()->map_gc(gc)->get_available_fontsizes(font_nr, available_sizes);
}

void AW_stylable::establish_default(int gc) {
    get_common()->map_mod_gc(gc)->establish_default();
}

void AW_stylable::new_gc(int gc) {
    get_common()->new_gc(gc);
}

void AW_stylable::set_function(int gc, AW_function function) {
    get_common()->map_mod_gc(gc)->set_function(function);
}

void AW_stylable::reset_style() {
    get_common()->reset_style();
}

void AW_stylable::set_font(int gc, AW_font font_nr, int size, int *found_size) {
    get_common()->map_mod_gc(gc)->set_font(font_nr, size, found_size);
}

void AW_stylable::set_foreground_color(int gc, AW_color_idx color) {
    get_common()->map_mod_gc(gc)->set_fg_color(get_common()->get_color(color));
}

void AW_stylable::set_grey_level(int gc, AW_grey_level grey_level) {
    // <0 = don't fill, 0.0 = white, 1.0 = black
    get_common()->map_mod_gc(gc)->set_grey_level(grey_level);
}

void AW_stylable::set_line_attributes(int gc, short width, AW_linestyle style) {
    get_common()->map_mod_gc(gc)->set_line_attributes(width, style);
}