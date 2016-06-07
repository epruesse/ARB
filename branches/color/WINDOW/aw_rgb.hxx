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

#ifndef _STDINT_H
#include <stdint.h>
#endif
#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

#if defined(ARB_GTK)
#define AW_WINDOW_FG AW_rgb()
#endif

class AW_rgb_normalized;

class AW_rgb16 {
    uint16_t R, G, B;

    void rescale(uint16_t my_max);
public:
    static CONSTEXPR float MAX = 65535.0;

    AW_rgb16() : R(0), G(0), B(0) {}
    AW_rgb16(uint16_t red, uint16_t green, uint16_t blue) : R(red), G(green), B(blue) {}
    explicit AW_rgb16(const char *colorname);
    explicit inline AW_rgb16(const AW_rgb_normalized& o);

    bool operator == (const AW_rgb16& o) const { return R==o.R && G==o.G && B == o.B; }
    bool operator != (const AW_rgb16& o) const { return !operator==(o); }

    uint16_t r() const { return R; }
    uint16_t g() const { return G; }
    uint16_t b() const { return B; }

    const char *ascii() const;
};

class AW_rgb_normalized {
    float R, G, B;
public:
    AW_rgb_normalized(float red, float green, float blue) : R(red), G(green), B(blue) {}
    explicit AW_rgb_normalized(const AW_rgb16& o) :
        R(o.r()/AW_rgb16::MAX),
        G(o.g()/AW_rgb16::MAX),
        B(o.b()/AW_rgb16::MAX)
    {}
#if defined(Cxx11)
    explicit AW_rgb_normalized(const char *colorname) : AW_rgb_normalized(AW_rgb16(colorname)) {}
#else // !defined(Cxx11)
    explicit AW_rgb_normalized(const char *colorname) { *this = AW_rgb_normalized(AW_rgb16(colorname)); }
#endif

    float r() const { return R; }
    float g() const { return G; }
    float b() const { return B; }
};

inline AW_rgb16::AW_rgb16(const AW_rgb_normalized& o) : R(o.r()*MAX), G(o.g()*MAX), B(o.b()*MAX) {}

#if defined(ARB_MOTIF)
typedef unsigned long AW_rgb; // =XColor.pixel
#else // ARB_GTK
typedef AW_rgb16 AW_rgb;
#endif

#else
#error aw_rgb.hxx included twice
#endif // AW_RGB_HXX
