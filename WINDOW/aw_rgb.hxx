#pragma once
#include <stdint.h>

#define AW_WINDOW_FG AW_rgb()

class AW_rgb {
private:
    uint16_t red;
    uint16_t green;
    uint16_t blue;
public:
    AW_rgb();
    AW_rgb(const char* name);
    bool operator==(const AW_rgb&) const;
    bool operator!=(const AW_rgb& o) const { return !operator==(o); }
    float r() const;
    float g() const;
    float b() const;
    char* string() const;
};
