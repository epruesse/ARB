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
    if (!(filteri & filter)) return false;
    dot(Line.start(), filteri);
    dot(Line.head(), filteri);
    return true;
}

bool AW_device_size::text_impl(int gc, const char *str, const AW::Position& pos,
                               AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (!(filteri & filter)) return false;

    AW::Position          transPos    = transform(pos);
    const AW_font_limits& font_limits = get_common()->map_gc(gc)->get_font_limits();
    AW_pos                l_ascent    = font_limits.ascent;
    AW_pos                l_descent   = font_limits.descent;
    AW_pos                l_width     = get_string_size(gc, str, opt_strlen);

    AW::Position upperLeft(AW::x_alignment(transPos.xpos(), l_width, alignment),
                           transPos.ypos()-l_ascent);

    dot_transformed(upperLeft, filteri);
    dot_transformed(upperLeft + AW::Vector(l_width, l_ascent+l_descent), filteri);

    return true;
}

bool AW_device_size::invisible_impl(const AW::Position& pos, AW_bitset filteri) {
    if (!(filteri & filter)) return false;
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
