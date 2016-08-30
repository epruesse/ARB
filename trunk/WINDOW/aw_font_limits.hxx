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
    short height;
    short width;
    short min_width;

    void reset() {
        ascent    = descent = height = width = 0;
        min_width = SHRT_MAX;
    }

    void notify_ascent(short a) { ascent = std::max(a, ascent); }
    void notify_descent(short d) { descent = std::max(d, descent); }
    void notify_width(short w) {
        width     = std::max(w, width);
        min_width = std::min(w, min_width);
    }

    void notify_all(short a_ascent, short a_descent, short a_width) {
        notify_ascent (a_ascent);
        notify_descent(a_descent);
        notify_width  (a_width);
    }

    void calc_height() { height = ascent+descent+1; }

    bool is_monospaced() const { return width == min_width; }

    AW_font_limits() { reset(); }
    AW_font_limits(const AW_font_limits& lim1, const AW_font_limits& lim2)
        : ascent(std::max(lim1.ascent, lim2.ascent)),
          descent(std::max(lim1.descent, lim2.descent)),
          width(std::max(lim1.width, lim2.width)),
          min_width(std::min(lim1.min_width, lim2.min_width))
    {
        calc_height();
    }
};


#else
#error aw_font_limits.hxx included twice
#endif // AW_FONT_LIMITS_HXX
