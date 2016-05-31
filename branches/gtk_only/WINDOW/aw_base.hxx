// ================================================================= //
//                                                                   //
//   File      : aw_base.hxx                                         //
//   Purpose   : forward declare some gui types                      //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#pragma once

#include <gdk/gdk.h>
class AW_root;
class AW_window;

typedef long AW_CL; // client data (casted from pointer or value) // @@@ [CB] eliminate later

typedef void (*AW_postcb_cb)(AW_window *);

struct          GBDATA;
typedef GBDATA *AW_default;

typedef double AW_pos;

struct AW_world {
    AW_pos t, b, l, r;
    void clear() { t = b = l = r = 0.0; }
};
struct AW_screen_area {
    int t, b, l, r;
    void clear() { t = b = l = r = 0; }
};

typedef AW_screen_area AW_borders; // not an area, but size spec at all borders

typedef int AW_font;

typedef long      AW_bitset;
typedef AW_bitset AW_active;     // bits to activate/inactivate buttons
typedef float     AW_grey_level; // <0 don't fill  0.0 white 1.0 black

enum AW_area {
    AW_INFO_AREA,
    AW_MIDDLE_AREA,
    AW_BOTTOM_AREA,
    AW_MAX_AREA
    // make sure matching labels exist in AW_area_management.cxx in AW_area_labels
};

enum GcChange {
    // value 0 is reserved (used internally in GC-manager)

    GC_COLOR_CHANGED = 1,       // -> normally a refresh should do
    GC_FONT_CHANGED,            // -> display needs a resize
    GC_COLOR_GROUP_USE_CHANGED, // -> might need extra calculations

    // Note: higher values should cover lower values,
    // i.e. a resize (GC_FONT_CHANGED) always does a refresh (GC_COLOR_CHANGED)
};

AW_default get_AW_ROOT_DEFAULT();
#define AW_ROOT_DEFAULT get_AW_ROOT_DEFAULT()
