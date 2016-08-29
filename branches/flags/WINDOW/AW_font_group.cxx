// ==================================================================== //
//                                                                      //
//   File      : AW_font_group.cxx                                      //
//   Purpose   : Bundles a group of fonts and provides overall maximas  //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "aw_font_group.hxx"
#include "aw_common.hxx"

AW_font_group::AW_font_group() {
    unregisterAll();
}

void AW_font_group::unregisterAll()
{
    max_width   = 0;
    max_ascent  = 0;
    max_descent = 0;
    max_height  = 0;

    memset(&max_letter_limits[0], 0, sizeof(max_letter_limits));
}

inline void set_max(int val, int& max) { if (val>max) max = val; }

void AW_font_group::registerFont(AW_device *device, int gc, const char *chars) {
    aw_assert(gc <= AW_FONT_GROUP_MAX_GC);

    const AW_GC *gcm = device->get_common()->map_gc(gc);

    if (!chars) {
        // use complete ASCII-range for limits
        max_letter_limits[gc] = gcm->get_font_limits();
    }
    else {
        aw_assert(chars[0]);
        AW_font_limits limits = gcm->get_font_limits(chars[0]);
        for (int i = 1; chars[i]; ++i) {
            limits = AW_font_limits(limits, gcm->get_font_limits(chars[i]));
        }
        max_letter_limits[gc] = limits;
    }

    aw_assert(font_registered(gc)); 

    set_max(get_width(gc), max_width);
    set_max(get_ascent(gc), max_ascent);
    set_max(get_descent(gc), max_descent);
    set_max(get_height(gc), max_height);
}

