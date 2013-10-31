/* 
 * File:   AW_device_size.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:25 PM
 */

#pragma once

#include "aw_simple_device.hxx"


class AW_device_size : public AW_simple_device {
    AW_size_tracker scaled;   // all zoomable parts (e.g. tree skeleton)
    AW_size_tracker unscaled; // all unzoomable parts (e.g. text at tree-tips or group-brackets)

    void dot_transformed(const AW::Position& pos, AW_bitset filteri);
    void dot_transformed(AW_pos X, AW_pos Y, AW_bitset filteri) { dot_transformed(AW::Position(X, Y), filteri); }

    void dot(const AW::Position& p, AW_bitset filteri) { dot_transformed(transform(p), filteri); }
    void dot(AW_pos x, AW_pos y, AW_bitset filteri) { dot(AW::Position(x, y), filteri); }

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) OVERRIDE;

    void specific_reset() OVERRIDE;
    
public:
    AW_device_size(AW_common *common_) : AW_simple_device(common_) {}

    void restart_tracking();

    AW_DEVICE_TYPE type() OVERRIDE;

    // all get_size_information...() return screen coordinates

    void get_size_information(AW_world *ptr) const __ATTR__DEPRECATED_TODO("whole AW_world is deprecated") {
        *ptr = scaled.get_size();
    }
    AW::Rectangle get_size_information() const { return scaled.get_size_as_Rectangle(); }
    AW::Rectangle get_size_information_unscaled() const { return unscaled.get_size_as_Rectangle(); }
    AW::Rectangle get_size_information_inclusive_text() const {
        return get_size_information().bounding_box(get_size_information_unscaled());
    }
    int  get_string_size(int gc, const  char *string, long textlen) const OVERRIDE;
    AW_borders get_unscaleable_overlap() const;
};

