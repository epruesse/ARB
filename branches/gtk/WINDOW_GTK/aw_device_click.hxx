/* 
 * File:   AW_device_click.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 11:34 AM
 */

#pragma once

#include "aw_device.hxx"

class AW_device_click : public AW_simple_device {
    AW_pos          mouse_x, mouse_y;
    AW_pos          max_distance_line;
    AW_pos          max_distance_text;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) { return generic_invisible(pos, filteri); }

    void specific_reset() {}
    
public:
    AW_clicked_line opt_line;
    AW_clicked_text opt_text;

    AW_device_click(AW_common *common_);

    AW_DEVICE_TYPE type();

    void init(AW_pos mousex, AW_pos mousey, AW_pos max_distance_liniei, AW_pos max_distance_texti, AW_pos radi, AW_bitset filteri);

    void get_clicked_line(class AW_clicked_line *ptr) const;
    void get_clicked_text(class AW_clicked_text *ptr) const;
};
