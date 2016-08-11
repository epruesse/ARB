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


#if defined(DEBUG)
// #define SHOW_CLIP_STACK_CHANGES
#endif // DEBUG

class AW_clip_scale_stack {
    // completely private, but accessible by AW_device
    friend class AW_device;

    AW_screen_area  clip_rect;
    AW_font_overlap font_overlap;

    AW::Vector offset;
    AW_pos scale;

    class AW_clip_scale_stack *next;
};

#if defined(SHOW_CLIP_STACK_CHANGES)
static const char *clipstatestr(AW_device *device) {
    static char buffer[1024];

    const AW_screen_area&  clip_rect = device->get_cliprect();
    const AW_font_overlap& fo        = device->get_font_overlap();
    const AW::Vector&      offset    = device->get_offset();

    sprintf(buffer,
            "clip_rect={t=%i, b=%i, l=%i, r=%i} "
            "font_overlap={t=%i, b=%i, l=%i, r=%i} "
            "scale=%f unscale=%f "
            "offset={x=%f y=%f}" ,
            clip_rect.t, clip_rect.b, clip_rect.l, clip_rect.r,
            fo.top, fo.bottom, fo.left, fo.right,
            device->get_scale(), device->get_unscale(),
            offset.x(), offset.y());

    return buffer;
}
#endif // SHOW_CLIP_STACK_CHANGES

const AW_screen_area& AW_device::get_area_size() const {
    return get_common()->get_screen();
}


void AW_device::pop_clip_scale() {
    if (!clip_scale_stack) {
        aw_assert(0); // Too many pop_clip_scale on that device
        return;
    }

#if defined(SHOW_CLIP_STACK_CHANGES)
    char *state_before_pop = ARB_strdup(clipstatestr(this));
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
        aw_assert(font_limits.width != 0); //division by zero
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

bool AW_device::generic_polygon(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
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

void AW_device::move_region(AW_pos /* src_x */, AW_pos /* src_y */, AW_pos /* width */, AW_pos /* height */,
                            AW_pos /* dest_x */, AW_pos /* dest_y */) {
    // empty default
}

static const AW_screen_area& get_universe() {
    // "unrestricted" area
    const int UMIN = INT_MIN/10;
    const int UMAX = INT_MAX/10;
    static AW_screen_area universe = { UMIN, UMAX, UMIN, UMAX };
    return universe;
}

void AW_device::reset() {
#if defined(SHOW_CLIP_STACK_CHANGES)
    printf("reset deletes clip_scale_stack!\n");
#endif // SHOW_CLIP_STACK_CHANGES
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

bool AW_device::generic_invisible(const AW::Position& pos, AW_bitset filteri) {
    return (filter & filteri) ? !is_outside_clip(transform(pos)) : false;
}

const AW_screen_area& AW_device::get_common_screen(const AW_common *common_) {
    return common_->get_screen();
}

bool AW_device::generic_box(int gc, const AW::Rectangle& rect, AW_bitset filteri) {
    int drawflag = 0;
    if (filteri & filter) {
        drawflag |= line_impl(gc, rect.upper_edge(), filteri);
        drawflag |= line_impl(gc, rect.lower_edge(), filteri);
        drawflag |= line_impl(gc, rect.left_edge(),  filteri);
        drawflag |= line_impl(gc, rect.right_edge(), filteri);
    }
    return drawflag;
}

void AW_device::clear(AW_bitset) {
    // nothing to do
}

void AW_device::clear_part(const AW::Rectangle&, AW_bitset) {
    // nothing to do
}

void AW_device::set_filter(AW_bitset filteri) {
    filter = filteri;
}


#if 0
//aw_clipable

inline AW_pos clip_in_range(AW_pos low, AW_pos val, AW_pos high) {
    if (val <= low) return low;
    if (val >= high) return high;
    return val;
}
#endif

void AW_zoomable::reset() {
    unscale = scale   = 1.0;
    offset  = AW::Vector(0, 0);
}

void AW_zoomable::zoom(AW_pos val) {
    scale   *= val;
    unscale  = 1.0/scale;
}
