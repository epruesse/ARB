// =============================================================== //
//                                                                 //
//   File      : AW_size.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_commn.hxx"

#include <algorithm>

using namespace std;

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

inline void AW_device_size::dot(AW_pos x, AW_pos y) {
    AW_pos X, Y;
    transform(x, y, X, Y);
    dot_transformed(X, Y);
}

bool AW_device_size::invisible_impl(int gc, AW_pos x, AW_pos y, AW_bitset filteri) {
    if (filteri & filter) dot(x, y);
    return AW_device::invisible_impl(gc, x, y, filteri);
}


int AW_device_size::line_impl(int /*gc*/, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri) {
    if (filteri & filter) {
        dot(x0, y0);
        dot(x1, y1);
        return true;
    }
    return false;
}

int AW_device_size::text_impl(int gc, const char *str, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (filteri & filter) {
        XFontStruct *xfs = &(common->gcs[gc]->curfont);

        AW_pos X0, Y0;          // Transformed pos
        this->transform(x, y, X0, Y0);

        AW_pos l_ascent  = xfs->max_bounds.ascent;
        AW_pos l_descent = xfs->max_bounds.descent;
        AW_pos l_width   = get_string_size(gc, str, opt_strlen);
        X0               = common->x_alignment(X0, l_width, alignment);

        dot_transformed(X0, Y0-l_ascent);
        dot_transformed(X0+l_width, Y0+l_descent);
        return 1;
    }
    return 0;
}



void AW_device_size::get_size_information(AW_world *ptr) {
    *ptr = size_information;
}

