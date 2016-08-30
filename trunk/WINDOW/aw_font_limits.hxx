// ============================================================== //
//                                                                //
//   File: aw_font_limits.hxx                                     //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef AW_FONT_LIMITS_HXX
#define AW_FONT_LIMITS_HXX

#ifndef _GLIBCXX_CLIMITS
#include <climits>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

struct AW_font_limits {
    short ascent;
    short descent;
    short width;
    short min_width;

    void reset() {
        ascent    = descent = width = 0;
        min_width = SHRT_MAX;
    }

    void notify(short ascent_, short descent_, short width_) {
        ascent    = std::max(ascent_, ascent);
        descent   = std::max(descent_, descent);
        width     = std::max(width_, width);
        min_width = std::min(width_, min_width);
    }

    short get_height() const { return ascent+descent+1; }

    bool is_monospaced() const { return width == min_width; }

    AW_font_limits() { reset(); }
    AW_font_limits(const AW_font_limits& lim1, const AW_font_limits& lim2) :
        ascent(std::max(lim1.ascent, lim2.ascent)),
        descent(std::max(lim1.descent, lim2.descent)),
        width(std::max(lim1.width, lim2.width)),
        min_width(std::min(lim1.min_width, lim2.min_width))
    {}
};


#else
#error aw_font_limits.hxx included twice
#endif // AW_FONT_LIMITS_HXX
