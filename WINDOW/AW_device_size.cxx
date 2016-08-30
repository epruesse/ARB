/* 
 * File:   AW_device_size.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:25 PM
 */

#include "aw_device_size.hxx"
#include "aw_common.hxx"


inline int calc_overlap(AW_pos smaller, AW_pos bigger) {
    if (smaller<bigger) return AW_INT(bigger-smaller);
    return 0;
}


void AW_device_size::restart_tracking() {
    scaled.restart();
    unscaled.restart();
}

AW_borders AW_device_size::get_unscaleable_overlap() const {
    AW_borders unscalable_overlap;
    if (scaled.was_drawn() && unscaled.was_drawn()) {
        const AW_world& scaled_size   = scaled.get_size();
        const AW_world& unscaled_size = unscaled.get_size();

        unscalable_overlap.t = calc_overlap(unscaled_size.t, scaled_size.t);
        unscalable_overlap.l = calc_overlap(unscaled_size.l, scaled_size.l);
        unscalable_overlap.b = calc_overlap(scaled_size.b, unscaled_size.b);
        unscalable_overlap.r = calc_overlap(scaled_size.r, unscaled_size.r);
    }
    else {
        unscalable_overlap.clear();
    }
    return unscalable_overlap;
}

AW_DEVICE_TYPE AW_device_size::type() {
    return AW_DEVICE_SIZE;
}

bool AW_device_size::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset filteri) {
    dot(Line.start(), filteri);
    dot(Line.head(), filteri);
    return true;
}

bool AW_device_size::text_impl(int gc, const char *str, const AW::Position& pos,
                               AW_pos alignment, AW_bitset filteri, long opt_strlen) {

    AW::Position          transPos  = transform(pos);
    const AW_font_limits& font      = get_common()->map_gc(gc)->get_font_limits();
    AW_pos                l_ascent  = font.ascent;
    AW_pos                l_descent = font.descent;
    AW_pos                l_width   = get_common()->map_gc(gc)->get_string_size_fast(str, opt_strlen);

    AW::Position upperLeft(AW::x_alignment(transPos.xpos(), l_width, alignment),
                           transPos.ypos()-l_ascent);

    dot_transformed(upperLeft, filteri);
    dot_transformed(upperLeft + AW::Vector(l_width, l_ascent+l_descent), filteri);

    return true;
}

bool AW_device_size::invisible_impl(const AW::Position& pos, AW_bitset filteri) {
  dot(pos, filteri);
  return true;
}

inline void AW_device_size::dot_transformed(const AW::Position& pos, AW_bitset filteri) {
    if (filter == (AW_PRINTER|AW_PRINTER_EXT)) { // detect graphic size for print-scaling
        scaled.track(pos);
    }
    else {
        if (filteri&AW_SIZE) {
            aw_assert((filteri&AW_SIZE_UNSCALED) == 0);
            scaled.track(pos);
        }
        else {
            aw_assert((filteri&AW_SIZE) == 0);
            unscaled.track(pos);
        }
    }
}

void AW_device_size::specific_reset() {
    restart_tracking();
}

/**
 * Calculate an estimate of the size of the string to be rendered.
 * (Other devices calculate precise size, we overestimate)
 * See also AW_GC::get_string_size_fast
 */
int AW_device_size::get_string_size(int gc, const char *str, long textlen) const {
    return get_common()->map_gc(gc)->get_string_size_fast(str, textlen);
}
