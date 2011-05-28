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

void AW_device_size::init() {
    drawn = false;

    scaled_size.t = 0;
    scaled_size.b = 0;
    scaled_size.l = 0;
    scaled_size.r = 0;
}

AW_DEVICE_TYPE AW_device_size::type() { return AW_DEVICE_SIZE; }

void AW_device_size::privat_reset() {
    this->init();
}

inline void AW_device_size::dot_transformed(AW_pos X, AW_pos Y) {
    if (drawn) {
        scaled_size.l = min(scaled_size.l, X);
        scaled_size.r = max(scaled_size.r, X);
        scaled_size.t = min(scaled_size.t, Y);
        scaled_size.b = max(scaled_size.b, Y);
    }
    else {
        scaled_size.l = scaled_size.r = X;
        scaled_size.t = scaled_size.b = Y;
        drawn              = true;
    }
}

bool AW_device_size::invisible_impl(int /*gc*/, const AW::Position& pos, AW_bitset filteri) {
    if (filteri & filter) {
        dot(pos);
        return true;
    }
    return false;
}


bool AW_device_size::line_impl(int /*gc*/, const LineVector& Line, AW_bitset filteri) {
    if (filteri & filter) {
        dot(Line.start());
        dot(Line.head());
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

        dot_transformed(upperLeft);
        dot_transformed(upperLeft + Vector(l_width, l_ascent+l_descent));

#if defined(DEBUG)
        fprintf(stderr, "size device text_impl on '%s'\n", str);
#endif

        return true;
    }
    return false;
}

