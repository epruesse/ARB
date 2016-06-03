// ============================================================ //
//                                                              //
//   File      : aw_rgb.hxx                                     //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AW_RGB_HXX
#define AW_RGB_HXX

#include <stdint.h>
#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif

#if defined(ARB_GTK)
#define AW_WINDOW_FG AW_rgb()
#endif

class AW_rgb16 {
    uint16_t R, G, B;

public:
    static CONSTEXPR float MAX = 65535.0;

    AW_rgb16() : R(0), G(0), B(0) {}
    explicit AW_rgb16(const char* name);

    bool operator == (const AW_rgb16& o) const { return R==o.R && G==o.G && B == o.B; }
    bool operator != (const AW_rgb16& o) const { return !operator==(o); }

    uint16_t r() const { return R; }
    uint16_t g() const { return G; }
    uint16_t b() const { return B; }
};

class AW_rgb_normalized {
    float R, G, B;
public:
    explicit AW_rgb_normalized(const AW_rgb16& o) :
        R(o.r()/AW_rgb16::MAX),
        G(o.g()/AW_rgb16::MAX),
        B(o.b()/AW_rgb16::MAX)
    {}

    float r() const { return R; }
    float g() const { return G; }
    float b() const { return B; }
};

#if defined(ARB_MOTIF)
typedef unsigned long AW_rgb; // =XColor.pixel
#else // ARB_GTK
typedef AW_rgb16 AW_rgb;
#endif

#else
#error aw_rgb.hxx included twice
#endif // AW_RGB_HXX
