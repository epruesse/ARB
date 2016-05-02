// =============================================================== //
//                                                                 //
//   File      : AW_clipable.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_window.hxx"
#include "aw_root.hxx"
#include "aw_common_xm.hxx"

#include <arb_msg.h>

using namespace AW;

inline AW_pos clip_in_range(AW_pos low, AW_pos val, AW_pos high) {
    if (val <= low) return low;
    if (val >= high) return high;
    return val;
}

bool AW_clipable::box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out) {
    // clip coordinates of a box

    aw_assert(x0 <= x1);
    aw_assert(y0 <= y1);

    if (x1<clip_rect.l || x0>clip_rect.r) return false;
    if (y1<clip_rect.t || y0>clip_rect.b) return false;

    if (completely_clipped()) return false;

    x0out = clip_in_range(clip_rect.l, x0, clip_rect.r);
    x1out = clip_in_range(clip_rect.l, x1, clip_rect.r);
    y0out = clip_in_range(clip_rect.t, y0, clip_rect.b);
    y1out = clip_in_range(clip_rect.t, y1, clip_rect.b);

    return true;
}

bool AW_clipable::box_clip(const Rectangle& rect, Rectangle& clippedRect) { // @@@ maybe return clippedRect as AW_screen_area
    if (completely_clipped()) return false;

    Rectangle clipRect(clip_rect, UPPER_LEFT_OUTLINE);
    if (rect.distinct_from(clipRect))
        return false;

    clippedRect = rect.intersect_with(clipRect);
    return true;
}


bool AW_clipable::clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out) {
    // clip coordinates of a line

    int    outcodeout;
    AW_pos x = 0;
    AW_pos y = 0;

    bool is_visible = false;    // indicates whether part of line is visible
    bool done       = false;    // true soon as line is completely inside or outside rectangle

    while (!done) {
        int outcode0 = compoutcode(x0, y0);
        int outcode1 = compoutcode(x1, y1);

        if ((outcode0 | outcode1) == 0) { // line is inside the rectangle
            x0out = x0; y0out = y0; // clipped coordinates of line
            x1out = x1; y1out = y1;

            done    = true;
            is_visible = true;
        }
        else if ((outcode0 & outcode1) != 0) { // line is outside the rectangle
            done = true;
        }
        else { // line overlaps with at least one rectangle border
            outcodeout = outcode0>0 ? outcode0 : outcode1;

            if ((outcodeout & 8) != 0) { // overlap at top
                x = x0+(x1-x0)*(clip_rect.t-y0)/(y1-y0);
                y = clip_rect.t;
            }
            else if ((outcodeout & 4) != 0) { // overlap at bottom
                x = x0+(x1-x0)*(clip_rect.b-y0)/(y1-y0);
                y = clip_rect.b;
            }
            else if ((outcodeout & 2) != 0) { // overlap at right side
                y = y0+(y1-y0)*(clip_rect.r-x0)/(x1-x0);
                x = clip_rect.r;
            }
            else if ((outcodeout & 1) != 0) {
                y = y0+(y1-y0)*(clip_rect.l-x0)/(x1-x0); // overlap at left side
                x = clip_rect.l;
            }

            // set corrected point and iterate :
            if (outcode0 > 0) {
                x0 = x;
                y0 = y;
            }
            else {
                x1 = x;
                y1 = y;
            }
        }
    }

    return is_visible;
}

bool AW_clipable::clip(const LineVector& line, LineVector& clippedLine) {
    AW_pos x0, y0, x1, y1;
    bool   drawflag = clip(line.start().xpos(), line.start().ypos(), line.head().xpos(), line.head().ypos(),
                           x0, y0, x1, y1);
    if (drawflag) clippedLine = LineVector(x0, y0, x1, y1);
    return drawflag;
}
void AW_clipable::set_bottom_clip_border(int bottom, bool allow_oversize) {
    clip_rect.b = bottom;
    if (!allow_oversize) {
        if (clip_rect.b > get_screen().b) clip_rect.b = get_screen().b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clipable::set_left_clip_border(int left, bool allow_oversize) {
    clip_rect.l = left;
    if (!allow_oversize) {
        if (clip_rect.l < get_screen().l) clip_rect.l = get_screen().l;
    }
    else {
        set_left_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clipable::set_right_clip_border(int right, bool allow_oversize) {
    clip_rect.r = right;
    if (!allow_oversize) {
        if (clip_rect.r > get_screen().r) clip_rect.r = get_screen().r;
    }
    else {
        set_right_font_overlap(true); // added to correct problem with last char skipped (added 21.6.02 --ralf)
    }
}

void AW_clipable::set_top_clip_border(int top, bool allow_oversize) {
    clip_rect.t = top;
    if (!allow_oversize) {
        if (clip_rect.t < get_screen().t) clip_rect.t = get_screen().t;
    }
    else {
        set_top_font_overlap(true); // added 21.6.02 --ralf
    }
}


int AW_clipable::reduceClipBorders(int top, int bottom, int left, int right) {
    // return 0 if no clipping area left
    if (top    > clip_rect.t) clip_rect.t = top;
    if (bottom < clip_rect.b) clip_rect.b = bottom;
    if (left   > clip_rect.l) clip_rect.l = left;
    if (right  < clip_rect.r) clip_rect.r = right;

    return !(clip_rect.b<clip_rect.t || clip_rect.r<clip_rect.l);
}

void AW_clipable::set_cliprect_oversize(const AW_screen_area& rect, bool allow_oversize) {
    clip_rect = rect;

    const AW_screen_area& screen = get_screen();
    if (!allow_oversize) {
        if (clip_rect.t < screen.t) clip_rect.t = screen.t;
        if (clip_rect.b > screen.b) clip_rect.b = screen.b;
        if (clip_rect.l < screen.l) clip_rect.l = screen.l;
        if (clip_rect.r > screen.r) clip_rect.r = screen.r;
    }

    set_font_overlap(false);

    if (allow_oversize) { // added 21.6.02 --ralf
        if (clip_rect.t < screen.t) set_top_font_overlap(true);
        if (clip_rect.b > screen.b) set_bottom_font_overlap(true);
        if (clip_rect.l < screen.l) set_left_font_overlap(true);
        if (clip_rect.r > screen.r) set_right_font_overlap(true);
    }
}

void AW_clipable::reduce_top_clip_border(int top) {
    if (top > clip_rect.t) clip_rect.t = top;
}
void AW_clipable::reduce_bottom_clip_border(int bottom) {
    if (bottom < clip_rect.b)     clip_rect.b = bottom;
}
void AW_clipable::reduce_left_clip_border(int left) {
    if (left > clip_rect.l)clip_rect.l = left;
}
void AW_clipable::reduce_right_clip_border(int right) {
    if (right < clip_rect.r)    clip_rect.r = right;
}

void AW_clipable::set_bottom_clip_margin(int bottom, bool allow_oversize) {
    clip_rect.b -= bottom;
    if (!allow_oversize) {
        if (clip_rect.b > get_screen().b) clip_rect.b = get_screen().b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}
bool AW_clipable::force_into_clipbox(const Position& pos, Position& forcedPos) {
    // force 'pos' inside 'clip_rect'
    if (completely_clipped()) return false;

    forcedPos.setx(clip_in_range(clip_rect.l, pos.xpos(), clip_rect.r));
    forcedPos.sety(clip_in_range(clip_rect.t, pos.ypos(), clip_rect.b));
    return true;
}

