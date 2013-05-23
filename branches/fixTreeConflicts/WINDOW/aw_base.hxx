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

typedef long int AW_CL; // client data (casted from pointer or value)

typedef void (*AW_CB)(AW_window*, AW_CL, AW_CL);
typedef void (*AW_CB0)(AW_window*);
typedef void (*AW_CB1)(AW_window*, AW_CL);
typedef void (*AW_CB2)(AW_window*, AW_CL, AW_CL);
typedef AW_window *(*AW_Window_Creator)(AW_root*, AW_CL);

typedef void (*AW_postcb_cb)(AW_window *);

class           GBDATA;
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

typedef const char *AWAR;

#define AW_NO_COLOR (-1U)
typedef guint32 AW_rgb;

enum AW_area {
    AW_INFO_AREA,
    AW_MIDDLE_AREA,
    AW_BOTTOM_AREA,
    AW_MAX_AREA
};

enum AW_color_idx {
    AW_WINDOW_BG,
    AW_WINDOW_FG,
    AW_WINDOW_C1,
    AW_WINDOW_C2,
    AW_WINDOW_C3,
    AW_WINDOW_DRAG, // unused
    AW_DATA_BG,
    AW_STD_COLOR_IDX_MAX
};


AW_default get_AW_ROOT_DEFAULT();
#define AW_ROOT_DEFAULT get_AW_ROOT_DEFAULT()
