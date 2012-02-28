// =============================================================== //
//                                                                 //
//   File      : AW_device.cxx                                     //
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

int AW_clipable::reduceClipBorders(int top, int bottom, int left, int right) {
    // return 0 if no clipping area left
    if (top    > clip_rect.t) clip_rect.t = top;
    if (bottom < clip_rect.b) clip_rect.b = bottom;
    if (left   > clip_rect.l) clip_rect.l = left;
    if (right  < clip_rect.r) clip_rect.r = right;

    return !(clip_rect.b<clip_rect.t || clip_rect.r<clip_rect.l);
}

void AW_clipable::reduce_top_clip_border(int top) {
    if (top > clip_rect.t) clip_rect.t = top;
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

void AW_clipable::reduce_bottom_clip_border(int bottom) {
    if (bottom < clip_rect.b)     clip_rect.b = bottom;
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

void AW_clipable::set_bottom_clip_margin(int bottom, bool allow_oversize) {
    clip_rect.b -= bottom;
    if (!allow_oversize) {
        if (clip_rect.b > get_screen().b) clip_rect.b = get_screen().b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}
void AW_clipable::reduce_left_clip_border(int left) {
    if (left > clip_rect.l)clip_rect.l = left;
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

void AW_clipable::reduce_right_clip_border(int right) {
    if (right < clip_rect.r)    clip_rect.r = right;
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

    Rectangle clipRect(clip_rect, FAULTY_OLD_CONVERSION); // @@@ fix
    if (rect.distinct_from(clipRect))
        return false;

    clippedRect = rect.intersect_with(clipRect);
    return true;
}

bool AW_clipable::force_into_clipbox(const Position& pos, Position& forcedPos) {
    // force 'pos' inside 'clip_rect'
    if (completely_clipped()) return false;

    forcedPos.setx(clip_in_range(clip_rect.l, pos.xpos(), clip_rect.r));
    forcedPos.sety(clip_in_range(clip_rect.t, pos.ypos(), clip_rect.b));
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

void AW_zoomable::zoom(AW_pos val) {
    scale   *= val;
    unscale  = 1.0/scale;
}

void AW_zoomable::reset() {
    unscale = scale   = 1.0;
    offset  = Vector(0, 0);
}

// -----------------
//      AW_GC_Xm

const int GC_DEFAULT_LINE_STYLE = LineSolid;
const int GC_DEFAULT_CAP_STYLE  = CapProjecting;
const int GC_JOIN_STYLE         = JoinMiter;

AW_GC_Xm::AW_GC_Xm(AW_common *common_)
    : AW_GC(common_)
{
    XGCValues val;

    val.line_width = GC_DEFAULT_LINE_WIDTH;
    val.line_style = GC_DEFAULT_LINE_STYLE;
    val.cap_style  = GC_DEFAULT_CAP_STYLE;
    val.join_style = GC_JOIN_STYLE;

    unsigned long value_mask = GCLineWidth|GCLineStyle|GCCapStyle|GCJoinStyle;

    gc = XCreateGC(get_common()->get_display(), get_common()->get_window_id(), value_mask, &val);
    wm_set_function(get_function());
}
AW_GC_Xm::~AW_GC_Xm() {
    if (gc) XFreeGC(get_common()->get_display(), gc);
}
void AW_GC_Xm::wm_set_lineattributes(short lwidth, AW_linestyle lstyle) {
    Display            *display = get_common()->get_display();
    aw_assert(lwidth>0);

    switch (lstyle) {
        case AW_SOLID:
            XSetLineAttributes(display, gc, lwidth, LineSolid, GC_DEFAULT_CAP_STYLE, GC_JOIN_STYLE);
            break;

        case AW_DOTTED:
        case AW_DASHED: {
            static char dashes[] = { 5, 2 };
            static char dots[]   = { 1, 1 };
            XSetDashes(display, gc, 0, lstyle == AW_DOTTED ? dots : dashes, 2);
            XSetLineAttributes(display, gc, lwidth, LineOnOffDash, CapButt, GC_JOIN_STYLE);
            break;
        }
    }
}
void AW_GC_Xm::wm_set_function(AW_function mode) {
    switch (mode) {
        case AW_XOR:
            XSetFunction(get_common()->get_display(), gc, GXxor);
            break;
        case AW_COPY:
            XSetFunction(get_common()->get_display(), gc, GXcopy);
            break;
    }
}
void AW_GC_Xm::wm_set_foreground_color(AW_rgb col) {
    XSetForeground(get_common()->get_display(), gc, col);
}

const AW_font_limits& AW_stylable::get_font_limits(int gc, char c) const {
    return get_common()->get_font_limits(gc, c);
}

int AW_GC::get_string_size(const char *str, long textlen) const {
    // calculate display size of 'str'
    // 'str' and/or 'textlen' may be 0
    // 'str'     == 0 -> calculate max width of any text with length 'textlen'
    // 'textlen' == 0 -> calls strlen when needed
    // both 0 -> return 0

    int width = 0;
    if (font_limits.is_monospaced() || !str) {
        if (!textlen && str) textlen = strlen(str);
        width = textlen * font_limits.width;
    }
    else {
        for (int c = *(str++); c; c = *(str++)) width += width_of_chars[c];
    }
    return width;
}

AW_GC *AW_common_Xm::create_gc() {
    return new AW_GC_Xm(this); 
}

void AW_GC_set::add_gc(int gi, AW_GC *agc) {
    if (gi >= count) {
        int new_count = gi+10;
        gcs           = (AW_GC **)realloc((char *)gcs, sizeof(*gcs)*new_count);
        memset(&gcs[count], 0, sizeof(*gcs)*(new_count-count));
        count         = new_count;
    }
    if (gcs[gi]) delete gcs[gi];
    gcs[gi] = agc;
}

int AW_stylable::get_string_size(int gc, const char *str, long textlen) const {
    return get_common()->map_gc(gc)->get_string_size(str, textlen);
}
void AW_stylable::new_gc(int gc) { get_common()->new_gc(gc); }
void AW_stylable::set_grey_level(int gc, AW_grey_level grey_level) {
    // <0 = don't fill, 0.0 = white, 1.0 = black
    get_common()->map_mod_gc(gc)->set_grey_level(grey_level);
}
void AW_stylable::set_font(int gc, AW_font font_nr, int size, int *found_size) {
    // if found_size != 0 -> return value for used font size
    get_common()->map_mod_gc(gc)->set_font(font_nr, size, found_size);
}
int AW_stylable::get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes) {
    return get_common()->map_gc(gc)->get_available_fontsizes(font_nr, available_sizes);
}
void AW_stylable::set_line_attributes(int gc, short width, AW_linestyle style) {
    get_common()->map_mod_gc(gc)->set_line_attributes(width, style);
}
void AW_stylable::set_function(int gc, AW_function function) {
    get_common()->map_mod_gc(gc)->set_function(function);
}
void AW_stylable::set_foreground_color(int gc, AW_color_idx color) {
    get_common()->map_mod_gc(gc)->set_fg_color(get_common()->get_color(color));
}
void AW_stylable::establish_default(int gc) {
    get_common()->map_mod_gc(gc)->establish_default();
}
void AW_stylable::reset_style() {
    get_common()->reset_style();
}

static void AW_get_common_extends_cb(AW_window */*aww*/, AW_CL cl_common_xm, AW_CL) {
    AW_common_Xm *common = (AW_common_Xm*)cl_common_xm;
    Window        root;
    unsigned int  width, height;
    unsigned int  depth, borderwidth; // unused
    int           x_offset, y_offset; // unused

    XGetGeometry(common->get_display(), common->get_window_id(),
                 &root,
                 &x_offset, 
                 &y_offset, 
                 &width,
                 &height,
                 &borderwidth,  // border width
                 &depth);       // depth of display

    common->set_screen_size(width, height);
}

void AW_common_Xm::install_common_extends_cb(AW_window *aww, AW_area area) {
    aww->set_resize_callback(area, AW_get_common_extends_cb, (AW_CL)this);
    AW_get_common_extends_cb(aww, (AW_CL)this, 0);
}

#if defined(DEBUG)
// #define SHOW_CLIP_STACK_CHANGES
#endif // DEBUG

class AW_clip_scale_stack {
    // completely private, but accessible by AW_device
    friend class AW_device;

    AW_screen_area  clip_rect;
    AW_font_overlap font_overlap;

    Vector offset;
    AW_pos scale;

    class AW_clip_scale_stack *next;
};

#if defined(SHOW_CLIP_STACK_CHANGES)
static const char *clipstatestr(AW_device *device) {
    static char buffer[1024];

    const AW_screen_area&  clip_rect = device->get_cliprect();
    const AW_font_overlap& fo        = device->get_font_overlap();
    const Vector&          offset    = device->get_offset();

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

// --------------------------------------------------------------------------------

static const AW_screen_area& get_universe() {
    // "unrestricted" area
    const int UMIN = INT_MIN/10;
    const int UMAX = INT_MAX/10;
    static AW_screen_area universe = { UMIN, UMAX, UMIN, UMAX };
    return universe;
}

const AW_screen_area& AW_device::get_area_size() const {
    return get_common()->get_screen();
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

bool AW_device::generic_invisible(const AW::Position& pos, AW_bitset filteri) {
    return (filter & filteri) ? !is_outside_clip(transform(pos)) : false;
}

bool AW_device::ready_to_draw(int gc) {
    return get_common()->gc_mapable(gc);
}

bool AW_device::generic_box(int gc, bool IF_DEBUG(filled), const Rectangle& rect, AW_bitset filteri) {
    aw_assert(!filled); // not supported
    int drawflag = 0;
    if (filteri & filter) {
        drawflag |= line_impl(gc, rect.upper_edge(), filteri);
        drawflag |= line_impl(gc, rect.lower_edge(), filteri);
        drawflag |= line_impl(gc, rect.left_edge(),  filteri);
        drawflag |= line_impl(gc, rect.right_edge(), filteri);
    }
    return drawflag;
}

bool AW_device::generic_filled_area(int gc, int npos, const Position *pos, AW_bitset filteri) {
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

#if defined(WARN_TODO)
#warning make functions below pure virtual
#endif


void AW_device::clear(AW_bitset /* filteri */) {}
void AW_device::clear_part(AW_pos /* x */, AW_pos /* y */, AW_pos /* width */, AW_pos /* height */, AW_bitset /* filteri */) {}
void AW_device::clear_text(int /* gc */, const char * /* string */, AW_pos /* x */, AW_pos /* y */, AW_pos /* alignment */, AW_bitset /* filteri */) {}
void AW_device::move_region(AW_pos /* src_x */, AW_pos /* src_y */, AW_pos /* width */, AW_pos /* height */, AW_pos /* dest_x */, AW_pos /* dest_y */) {}
void AW_device::fast() {}
void AW_device::slow() {}
void AW_device::flush() {}

bool AW_device::text_overlay(int gc, const char *opt_str, long opt_len,  // either string or strlen != 0
                             const Position& pos, AW_pos alignment, AW_bitset filteri, AW_CL cduser, 
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

    if (allow_left_font_overlap() || screen.l == clipRect.l) inside_clipping_left = false;
    if (allow_right_font_overlap() || clipRect.r == screen.r) inside_clipping_right = false;

    transform(pos.xpos(), pos.ypos(), X0, Y0);

    if (allow_top_font_overlap() || clipRect.t == 0) {             // check clip border inside screen
        if (Y0+font_limits.descent < clipRect.t) return 0; // draw outside screen
    }
    else {
        if (Y0-font_limits.ascent < clipRect.t) return 0;  // don't cross the clip border
    }

    if (allow_bottom_font_overlap() || clipRect.b == screen.b) {   // check clip border inside screen drucken
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

void AW_device::set_filter(AW_bitset filteri) { filter = filteri; }

const AW_screen_area& AW_device::get_common_screen(const AW_common *common_) {
    return common_->get_screen();
}


