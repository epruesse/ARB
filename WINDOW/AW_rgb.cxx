#include "aw_rgb.hxx"
#include <gdk/gdk.h>

AW_rgb::AW_rgb() 
    : red(0), green(0), blue(0)
{}

AW_rgb::AW_rgb(const char* name) {
    GdkColor col;
    gdk_color_parse(name, &col);
    red   = col.red;
    green = col.green;
    blue  = col.blue;
}

char* AW_rgb::string() const {
    GdkColor col;
    col.red   = red;
    col.green = green;
    col.blue  = blue;
    return gdk_color_to_string(&col);
}

bool AW_rgb::operator==(const AW_rgb& o) const {
    return red==o.red && green==o.green && blue == o.blue;
}

float AW_rgb::r() const {
    return red/65535.0;
}

float AW_rgb::g() const {
    return green/65535.0;
}

float AW_rgb::b() const {
    return blue/65535.0;
}


