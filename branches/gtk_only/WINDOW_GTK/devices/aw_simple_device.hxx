/* 
 * File:   AW_simple_device.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:23 PM
 */

#include "aw_device.hxx"

#pragma once

class AW_simple_device : public AW_device {
    bool box_impl(int gc, bool /*filled*/, const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE {
        return generic_box(gc, false, rect, filteri);
    }
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) OVERRIDE {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) OVERRIDE {
        return generic_circle(gc, filled, center, radius, filteri);
    }
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) OVERRIDE {
        return generic_arc(gc, filled, center, radius, start_degrees, arc_degrees, filteri);
    }
public:
    AW_simple_device(AW_common *common_) : AW_device(common_) {}
};
