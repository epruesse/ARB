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

    size_information.t = 0;
    size_information.b = 0;
    size_information.l = 0;
    size_information.r = 0;
}

AW_DEVICE_TYPE AW_device_size::type() { return AW_DEVICE_SIZE; }

void AW_device_size::privat_reset() {
    this->init();
}

inline void AW_device_size::dot_transformed(AW_pos X, AW_pos Y) {
    if (drawn) {
        size_information.l = min(size_information.l, X);
        size_information.r = max(size_information.r, X);
        size_information.t = min(size_information.t, Y);
        size_information.b = max(size_information.b, Y);
    }
    else {
        size_information.l = size_information.r = X;
        size_information.t = size_information.b = Y;
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

        return true;
    }
    return false;
}



void AW_device_size::get_size_information(AW_world *ptr) {
    *ptr = size_information;
}

