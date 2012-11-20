/* 
 * File:   AW_device_print.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:21 PM
 */

#pragma once

#include "aw_device.hxx"

/* xfig print device */
class AW_device_print : public AW_device { // derived from a Noncopyable
    FILE *out;
    bool  color_mode;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri);
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri);
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri);
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri);

    void specific_reset() {}

public:
    AW_device_print(AW_common *common_)
        : AW_device(common_),
          out(0),
          color_mode(false)
    {}

    GB_ERROR open(const char *path) __ATTR__USERESULT;
    void close();

    FILE *get_FILE();

    // AW_device interface:
    AW_DEVICE_TYPE type();

    int find_color_idx(AW_rgb color);
    void set_color_mode(bool mode);

};
