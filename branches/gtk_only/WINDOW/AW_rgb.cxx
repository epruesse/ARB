#include "aw_rgb.hxx"
#include <gdk/gdk.h>

AW_rgb16::AW_rgb16(const char* name) {
    GdkColor col;
    gdk_color_parse(name, &col);
    R = col.red;
    G = col.green;
    B = col.blue;
}

#if 0 // unused atm
char* AW_rgb::ascii() const {
    GdkColor col;
    col.red   = red;
    col.green = green;
    col.blue  = blue;
    return gdk_color_to_string(&col);
}
#endif

