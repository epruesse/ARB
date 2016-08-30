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

void AW_font_group::registerFont(AW_device *device, int gc, const char *chars) {
    aw_assert(gc <= AW_FONT_GROUP_MAX_GC);

    aw_assert(!font_registered(gc)); // font for gc is already registered

    const AW_GC *gcm = device->get_common()->map_gc(gc);

    if (!chars) { // use complete ASCII-range for limits
        gc_limits[gc] = gcm->get_font_limits();
    }
    else {
        aw_assert(chars[0]);
        AW_font_limits limits;
        for (int i = 0; chars[i]; ++i) {
            limits = AW_font_limits(limits, gcm->get_font_limits(chars[i]));
        }
        gc_limits[gc] = limits;
    }
    aw_assert(font_registered(gc));

    any_limits = AW_font_limits(any_limits, gc_limits[gc]);
    aw_assert(any_font_registered());
}

