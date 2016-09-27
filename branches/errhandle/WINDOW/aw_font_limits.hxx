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

    void reset() { *this = AW_font_limits(); }

    bool was_notified() const { return min_width != SHRT_MAX; } // true when notify... called

    void notify(short ascent_, short descent_, short width_) {
        ascent    = std::max(ascent_, ascent);
        descent   = std::max(descent_, descent);
        width     = std::max(width_, width);
        min_width = std::min(width_, min_width);
    }

    short get_height() const { return ascent+descent+1; }

    bool is_monospaced() const { aw_assert(was_notified()); return width == min_width; }

    AW_font_limits() :
        ascent(0),
        descent(0),
        width(0),
        min_width(SHRT_MAX)
    {}
    AW_font_limits(const AW_font_limits& lim1, const AW_font_limits& lim2) : // works with notified and initial AW_font_limits
        ascent(std::max(lim1.ascent, lim2.ascent)),
        descent(std::max(lim1.descent, lim2.descent)),
        width(std::max(lim1.width, lim2.width)),
        min_width(std::min(lim1.min_width, lim2.min_width))
    {
        aw_assert(correlated(lim1.was_notified() || lim2.was_notified(), was_notified()));
    }
};


#else
#error aw_font_limits.hxx included twice
#endif // AW_FONT_LIMITS_HXX
