/* 
 * File:   AW_simple_device.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:23 PM
 */

#include "aw_device_impl.hxx"

#pragma once

class AW_simple_device : public AW_device {
    bool box_impl(int gc, bool /*filled*/, const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE {
        return generic_box(gc, rect, filteri);
    }
    bool polygon_impl(int gc, bool /*filled*/, int npos, const AW::Position *pos, AW_bitset filteri) OVERRIDE {
        return generic_polygon(gc, npos, pos, filteri);
    }
    bool circle_impl(int gc, bool /*filled*/, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) OVERRIDE {
        return generic_circle(gc, center, radius, filteri);
    }
    bool arc_impl(int gc, bool /*filled*/, const AW::Position& center, const AW::Vector& radius, int /*start_degrees*/, int /*arc_degrees*/, AW_bitset filteri) OVERRIDE {
        return generic_circle(gc, center, radius, filteri);
    }
public:
    AW_simple_device(AW_common *common_) : AW_device(common_) {}
};
