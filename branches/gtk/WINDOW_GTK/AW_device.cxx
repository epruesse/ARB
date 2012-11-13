// ============================================================= //
//                                                               //
//   File      : AW_device.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_gtk_migration_helpers.hxx"
#include "aw_device.hxx"
#include "aw_base.hxx"
#include "aw_common.hxx"
#include "aw_position.hxx"



class AW_clip_scale_stack {
    // completely private, but accessible by AW_device
    friend class AW_device;

    AW_screen_area  clip_rect;
    AW_font_overlap font_overlap;

    AW::Vector offset;
    AW_pos scale;

    class AW_clip_scale_stack *next;
};



const AW_screen_area& AW_device::get_area_size() const {
    return get_common()->get_screen();
}


void AW_device::pop_clip_scale() {
    if (!clip_scale_stack) {
        aw_assert(0); // Too many pop_clip_scale on that device
        return;
    }

#if defined(SHOW_CLIP_STACK_CHANGES)
    char *state_before_pop = strdup(clipstatestr(this));
#endif // SHOW_CLIP_STACK_CHANGES

    AW_zoomable::reset();
    set_offset(clip_scale_stack->offset); // needs to be called before zoom()
    zoom(clip_scale_stack->scale);
    set_cliprect(clip_scale_stack->clip_rect);
    set_font_overlap(clip_scale_stack->font_overlap);

    aw_assert(get_scale() == clip_scale_stack->scale);

    AW_clip_scale_stack *oldstack = clip_scale_stack;
    clip_scale_stack              = clip_scale_stack->next;
    delete oldstack;

#if defined(SHOW_CLIP_STACK_CHANGES)
    printf(" pop_clip_scale: %s\n", state_before_pop);
    printf("    [after pop]: %s\n\n", clipstatestr(this));
    free(state_before_pop);
#endif // SHOW_CLIP_STACK_CHANGES
}

void AW_device::push_clip_scale() {
    AW_clip_scale_stack *stack = new AW_clip_scale_stack;

    stack->next      = clip_scale_stack;
    clip_scale_stack = stack;

    stack->scale        = get_scale();
    stack->offset       = get_offset();
    stack->font_overlap = get_font_overlap();
    stack->clip_rect    = get_cliprect();

#if defined(SHOW_CLIP_STACK_CHANGES)
    printf("push_clip_scale: %s\n", clipstatestr(this));
#endif // SHOW_CLIP_STACK_CHANGES
}


bool AW_device::text_overlay(int gc, const char *opt_str, long opt_len,  // either string or strlen != 0
                             const AW::Position& pos, AW_pos alignment, AW_bitset filteri, AW_CL cduser,
                             AW_pos opt_ascent, AW_pos opt_descent,             // optional height (if == 0 take font height)
                             TextOverlayCallback toc)
{
    const AW_GC           *gcm         = get_common()->map_gc(gc);
    const AW_font_limits&  font_limits = gcm->get_font_limits();

    long   textlen;
    int    xi;
    int    h;
    int    start;
    int    l;
    AW_pos X0, Y0;              // Transformed pos

    bool inside_clipping_left  = true; // clipping at the left edge of the screen is different from clipping right of the left edge.
    bool inside_clipping_right = true;

    // es gibt 4 clipping Moeglichkeiten:
    // 1. man will fuer den Fall clippen, dass man vom linken display-Rand aus druckt   => clipping rechts vom 1. Buchstaben
    // 2. man will fuer den Fall clippen, dass man mitten im Bildschirm ist             => clipping links vom 1. Buchstaben
    // 3. man will fuer den Fall clippen, dass man mitten im Bildschirm ist             => clipping links vom letzten Buchstaben
    // 4. man will fuer den Fall clippen, dass man bis zum rechten display-Rand druckt  => clipping rechts vom letzten Buchstaben

    if (!(filter & filteri)) return 0;

    const AW_screen_area& screen   = get_common()->get_screen();
    const AW_screen_area& clipRect = get_cliprect();
    //quick hack


    if (allow_left_font_overlap() || screen.l == clipRect.l) inside_clipping_left = false;
    if (allow_right_font_overlap() || clipRect.r == screen.r) inside_clipping_right = false;

    transform(pos.xpos(), pos.ypos(), X0, Y0);

    if (allow_top_font_overlap() || clipRect.t == 0) {             // check clip border inside screen
        if (Y0+font_limits.descent < clipRect.t) return 0; // draw outside screen
    }
    else {
        if (Y0-font_limits.ascent < clipRect.t) return 0;  // don't cross the clip border
    }

    if (allow_bottom_font_overlap() || clipRect.b == screen.b) {   // check clip border inside screen
        if (Y0-font_limits.ascent > clipRect.b) return 0;  // draw outside screen
    }
    else {
        if (Y0+font_limits.descent> clipRect.b) return 0;  // don't cross the clip border
    }

    if (!opt_len) {
        opt_len = textlen = strlen(opt_str);
    }
    else {
        textlen = opt_len;
    }

    aw_assert(opt_len == textlen);


#if defined(DEBUG)
    int opt_str_len = int(strlen(opt_str));
    aw_assert(opt_str_len >= textlen);
#endif

    if (alignment) {
        AW_pos width = get_string_size(gc, opt_str, textlen);
        X0 = X0-alignment*width;
    }
    xi = AW_INT(X0);
    if (X0 > clipRect.r) return 0; // right of screen

    l = (int)clipRect.l;
    if (xi + textlen*font_limits.width < l) return 0; // left of screen

    start = 0;

    // now clip left side
    if (xi < l) {
        if (font_limits.is_monospaced()) {
            h = (l - xi)/font_limits.width;
            if (inside_clipping_left) {
                if ((l-xi)%font_limits.width  >0) h += 1;
            }
            if (h >= textlen) return 0;
            start    = h;
            xi      += h*font_limits.width;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }
        else { // proportional font
            int c = 0;
            for (h=0; xi < l; h++) {
                if (!(c = opt_str[h])) return 0;
                xi += gcm->get_width_of_char(c);
            }
            if (!inside_clipping_left) {
                h-=1;
                xi -= gcm->get_width_of_char(c);
            }
            start    = h;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }
    }

    // now clip right side
    if (font_limits.is_monospaced()) {
        h = ((int)clipRect.r - xi) / font_limits.width;
        if (h < textlen) {
            if (inside_clipping_right) {
                textlen = h;
            }
            else {
                textlen = h+1;
            }
        }

        if (textlen < 0) return 0;
        aw_assert(int(strlen(opt_str)) >= textlen);
    }
    else { // proportional font
        l = (int)clipRect.r - xi;
        for (h = start; l >= 0 && textlen > 0;  h++, textlen--) { // was textlen >= 0
            l -= gcm->get_width_of_char(opt_str[h]);
        }
        textlen = h - start;
        if (l <= 0 && inside_clipping_right && textlen  > 0) {
            textlen -= 1;
        }

        if (textlen < 0) return 0;
        aw_assert(int(strlen(opt_str)) >= textlen);
    }
    X0 = (AW_pos)xi;

    AW_pos corrx, corry;
    rtransform(X0, Y0, corrx, corry);

    aw_assert(opt_len >= textlen);
    aw_assert(textlen >= 0 && int(strlen(opt_str)) >= textlen);

    return toc(this, gc, opt_str, opt_len, start, (size_t)textlen, corrx, corry, opt_ascent, opt_descent, cduser);
}



bool AW_device::generic_filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
    bool drawflag = false;
    if (filteri & filter) {
        int p = npos-1;
        for (int n = 0; n<npos; ++n) {
            drawflag |= line(gc, pos[p], pos[n], filteri);
            p = n;
        }
    }
    return drawflag;
}
void AW_device::move_region(AW_pos /*src_x*/, AW_pos /*src_y*/, AW_pos /*width*/, AW_pos /*height*/
                          , AW_pos /*dest_x*/, AW_pos /*dest_y*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::fast() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::slow() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::flush() {
    GTK_NOT_IMPLEMENTED;
}

static const AW_screen_area& get_universe() {
    // "unrestricted" area
    const int UMIN = INT_MIN/10;
    const int UMAX = INT_MAX/10;
    static AW_screen_area universe = { UMIN, UMAX, UMIN, UMAX };
    return universe;
}

void AW_device::reset() {
    while (clip_scale_stack) {
        pop_clip_scale();
    }
    if (type() == AW_DEVICE_SIZE) {
        set_cliprect(get_universe());
    }
    else {
        set_cliprect(get_area_size());
    }
    AW_zoomable::reset();
    specific_reset();
}

void AW_device::clear(AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_device::generic_invisible(const AW::Position& pos, AW_bitset filteri) {
    return (filter & filteri) ? !is_outside_clip(transform(pos)) : false;
}


const AW_screen_area&  AW_device::get_common_screen(const AW_common *common_) {
    return common_->get_screen();
}


bool AW_device::generic_box(int gc, bool /*filled*/, const AW::Rectangle& rect, AW_bitset filteri) {
    // Note: 'filled' is not supported on this device
    int drawflag = 0;
    if (filteri & filter) {
        drawflag |= line_impl(gc, rect.upper_edge(), filteri);
        drawflag |= line_impl(gc, rect.lower_edge(), filteri);
        drawflag |= line_impl(gc, rect.left_edge(),  filteri);
        drawflag |= line_impl(gc, rect.right_edge(), filteri);
    }
    return drawflag;
}

void AW_device::clear_part(const AW::Rectangle& rect, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}


void AW_device::set_filter(AW_bitset filteri) {
    filter = filteri;
}





FILE *AW_device_print::get_FILE() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}




void AW_device_print::close() {
    GTK_NOT_IMPLEMENTED;
}

GB_ERROR AW_device_print::open(char const* /*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_device_print::set_color_mode(bool /*mode*/) {
    GTK_NOT_IMPLEMENTED;
}



void AW_device_size::clear() {
    scaled.clear();
    unscaled.clear();
}

inline int calc_overlap(AW_pos smaller, AW_pos bigger) {
    if (smaller<bigger) return AW_INT(bigger-smaller);
    return 0;
}

AW_borders AW_device_size::get_unscaleable_overlap() const {
    AW_borders unscalable_overlap;
    if (scaled.was_drawn() && unscaled.was_drawn()) {
        const AW_world& scaled_size   = scaled.get_size();
        const AW_world& unscaled_size = unscaled.get_size();

        unscalable_overlap.t = calc_overlap(unscaled_size.t, scaled_size.t);
        unscalable_overlap.l = calc_overlap(unscaled_size.l, scaled_size.l);
        unscalable_overlap.b = calc_overlap(scaled_size.b, unscaled_size.b);
        unscalable_overlap.r = calc_overlap(scaled_size.r, unscaled_size.r);
    }
    else {
        unscalable_overlap.clear();
    }
    return unscalable_overlap;
}

AW_DEVICE_TYPE AW_device_size::type() {
    return AW_DEVICE_SIZE;
}

bool AW_device_size::line_impl(int /*gc*/, const AW::LineVector& Line, AW_bitset filteri) {
    if (filteri & filter) {
        dot(Line.start(), filteri);
        dot(Line.head(), filteri);
        return true;
    }
    return false;
}

bool AW_device_size::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    if (filteri & filter) {
        AW::Position          transPos    = transform(pos);
        const AW_font_limits& font_limits = get_common()->map_gc(gc)->get_font_limits();
        AW_pos                l_ascent    = font_limits.ascent;
        AW_pos                l_descent   = font_limits.descent;
        AW_pos                l_width     = get_string_size(gc, str, opt_strlen);

        AW::Position upperLeft(x_alignment(transPos.xpos(), l_width, alignment),
                               transPos.ypos()-l_ascent);

        dot_transformed(upperLeft, filteri);
        dot_transformed(upperLeft + AW::Vector(l_width, l_ascent+l_descent), filteri);

        return true;
    }
    return false;
}

bool AW_device_size::invisible_impl(const AW::Position& pos, AW_bitset filteri) {
    if (filteri & filter) {
        dot(pos, filteri);
        return true;
    }
    return false;
}

inline void AW_device_size::dot_transformed(const AW::Position& pos, AW_bitset filteri) {
    if (filter == (AW_PRINTER|AW_PRINTER_EXT)) { // detect graphic size for print-scaling
        scaled.track(pos);
    }
    else {
        if (filteri&AW_SIZE) {
            aw_assert((filteri&AW_SIZE_UNSCALED) == 0);
            scaled.track(pos);
        }
        else {
            aw_assert((filteri&AW_SIZE) == 0);
            unscaled.track(pos);
        }
    }
}

void AW_device_size::specific_reset() {
    clear();
}



//aw_styleable
//FIXME move to own class

const AW_font_limits& AW_stylable::get_font_limits(int gc, char c) const {
    return get_common()->get_font_limits(gc, c);
}

int AW_stylable::get_string_size(int gc, const char *str, long textlen) const {
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}

int AW_stylable::get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes) {
    return get_common()->map_gc(gc)->get_available_fontsizes(font_nr, available_sizes);
}

void AW_stylable::establish_default(int gc) {
    get_common()->map_mod_gc(gc)->establish_default();
}

void AW_stylable::new_gc(int gc) {
    get_common()->new_gc(gc);
}

void AW_stylable::set_function(int gc, AW_function function) {
    get_common()->map_mod_gc(gc)->set_function(function);
}

void AW_stylable::reset_style() {
    get_common()->reset_style();
}

void AW_stylable::set_font(int gc, AW_font font_nr, int size, int *found_size) {
    get_common()->map_mod_gc(gc)->set_font(font_nr, size, found_size);
}

void AW_stylable::set_foreground_color(int gc, AW_color_idx color) {
    get_common()->map_mod_gc(gc)->set_fg_color(get_common()->get_color(color));
}

void AW_stylable::set_grey_level(int gc, AW_grey_level grey_level) {
    // <0 = don't fill, 0.0 = white, 1.0 = black
    get_common()->map_mod_gc(gc)->set_grey_level(grey_level);
}

void AW_stylable::set_line_attributes(int gc, short width, AW_linestyle style) {
    get_common()->map_mod_gc(gc)->set_line_attributes(width, style);
}


//aw_clipable

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

bool AW_clipable::box_clip(const AW::Rectangle& rect, AW::Rectangle& clippedRect) { // @@@ maybe return clippedRect as AW_screen_area
    if (completely_clipped()) return false;

    AW::Rectangle clipRect(clip_rect, AW::UPPER_LEFT_OUTLINE);
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

bool AW_clipable::clip(const AW::LineVector& line, AW::LineVector& clippedLine) {
    AW_pos x0, y0, x1, y1;
    bool   drawflag = clip(line.start().xpos(), line.start().ypos(), line.head().xpos(), line.head().ypos(),
                           x0, y0, x1, y1);
    if (drawflag) clippedLine = AW::LineVector(x0, y0, x1, y1);
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



void AW_zoomable::reset() {
    unscale = scale   = 1.0;
    offset  = AW::Vector(0, 0);
}

void AW_zoomable::zoom(AW_pos val) {
    scale   *= val;
    unscale  = 1.0/scale;
}



