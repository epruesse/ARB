// =============================================================== //
//                                                                 //
//   File      : AW_size.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_common.hxx"

#include <algorithm>

using namespace std;
using namespace AW;

// *****************************************************************************************
  //                                    size_device
  // *****************************************************************************************

void AW_device_size::clear() {
    scaled.clear();
    unscaled.clear();
}

AW_DEVICE_TYPE AW_device_size::type() { return AW_DEVICE_SIZE; }

void AW_device_size::privat_reset() {
    clear();
}

inline int calc_overlap(AW_pos smaller, AW_pos bigger) {
    if (smaller<bigger) return AW_INT(bigger-smaller);
    return 0;
}

const AW_borders& AW_device_size::get_unscaleable_overlap() const {
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

bool AW_device_size::invisible_impl(int /*gc*/, const AW::Position& pos, AW_bitset filteri) {
    if (filteri & filter) {
        dot(pos, filteri);
        return true;
    }
    return false;
}


bool AW_device_size::line_impl(int /*gc*/, const LineVector& Line, AW_bitset filteri) {
    if (filteri & filter) {
        dot(Line.start(), filteri);
        dot(Line.head(), filteri);
        return true;
    }
    return false;
}

bool AW_device_size::text_impl(int gc, const char *str, const Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (filteri & filter) {
        Position              transPos    = transform(pos);
        const AW_font_limits& font_limits = get_common()->map_gc(gc)->get_font_limits();
        AW_pos                l_ascent    = font_limits.ascent;
        AW_pos                l_descent   = font_limits.descent;
        AW_pos                l_width     = get_string_size(gc, str, opt_strlen);

        Position upperLeft(x_alignment(transPos.xpos(), l_width, alignment),
                           transPos.ypos()-l_ascent);

        dot_transformed(upperLeft, filteri);
        dot_transformed(upperLeft + Vector(l_width, l_ascent+l_descent), filteri);

        return true;
    }
    return false;
}

