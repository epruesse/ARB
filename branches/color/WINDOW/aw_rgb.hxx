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

    static bool is_normal(const float& f) { return f >= 0.0 && f<=1.0; }
    static const float& normal(const float& f) { aw_assert(is_normal(f)); return f; }

protected:
    AW_rgb_normalized(void*, float red, float green, float blue) : // accepts any values
        R(red),
        G(green),
        B(blue)
    {}
public:
    AW_rgb_normalized(float red, float green, float blue) :
        R(normal(red)),
        G(normal(green)),
        B(normal(blue))
    {}
    explicit AW_rgb_normalized(const AW_rgb16& o) :
        R(normal(o.r()/AW_rgb16::MAX)),
        G(normal(o.g()/AW_rgb16::MAX)),
        B(normal(o.b()/AW_rgb16::MAX))
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

// ---------------------------
//      color calculation
//
// Note: avoids color overflows (eg. white cannot get any whiter, black cannot get any blacker)

class AW_rgb_diff : private AW_rgb_normalized {
public:
    AW_rgb_diff(float red, float green, float blue) : AW_rgb_normalized(NULL, red, green, blue) {}

    float r() const { return AW_rgb_normalized::r(); }
    float g() const { return AW_rgb_normalized::g(); }
    float b() const { return AW_rgb_normalized::b(); }
};

inline AW_rgb_diff operator-(const AW_rgb_normalized& c1, const AW_rgb_normalized& c2) {
    return AW_rgb_diff(c1.r()-c2.r(),
                       c1.g()-c2.g(),
                       c1.b()-c2.b());
}
inline AW_rgb_diff operator*(const AW_rgb_diff& d, const float& f) { return AW_rgb_diff(d.r()*f, d.g()*f, d.b()*f); }
inline AW_rgb_diff operator*(const float& f, const AW_rgb_diff& d) { return d*f; }
inline AW_rgb_diff operator-(const AW_rgb_diff& d) { return d*-1.0; }

inline float avoid_overflow(float f) { return f<0.0 ? 0.0 : (f>1.0 ? 1.0 : f); }

inline AW_rgb_normalized operator+(const AW_rgb_normalized& col, const AW_rgb_diff& off) {
    return AW_rgb_normalized(avoid_overflow(col.r()+off.r()),
                             avoid_overflow(col.g()+off.g()),
                             avoid_overflow(col.b()+off.b()));
}
inline AW_rgb_normalized operator-(const AW_rgb_normalized& col, const AW_rgb_diff& off) { return col + -off; }

// allow to calculate directly with AW_rgb16:
inline AW_rgb_diff operator-(const AW_rgb16& c1, const AW_rgb16& c2) { return AW_rgb_normalized(c1)-AW_rgb_normalized(c2); }
inline AW_rgb16 operator+(const AW_rgb16& col, const AW_rgb_diff& off) { return AW_rgb16(AW_rgb_normalized(col)+off); }
inline AW_rgb16 operator-(const AW_rgb16& col, const AW_rgb_diff& off) { return AW_rgb16(AW_rgb_normalized(col)-off); }


#if defined(ARB_MOTIF)
typedef unsigned long AW_rgb; // =XColor.pixel
#else // ARB_GTK
typedef AW_rgb16 AW_rgb;
#endif

#else
#error aw_rgb.hxx included twice
#endif // AW_RGB_HXX
