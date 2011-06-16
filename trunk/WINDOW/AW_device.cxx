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
#include "aw_commn.hxx"
#include "aw_root.hxx"

#include <arb_msg.h>

using namespace AW;

void AW_clip::set_cliprect(AW_rectangle *rect, bool allow_oversize) {
    clip_rect = *rect;                  // coordinates : (0,0) = top-left-corner
    
    const AW_rectangle& screen = common->get_screen();
    if (!allow_oversize) {
        if (clip_rect.t < screen.t) clip_rect.t = screen.t;
        if (clip_rect.b > screen.b) clip_rect.b = screen.b;
        if (clip_rect.l < screen.l) clip_rect.l = screen.l;
        if (clip_rect.r > screen.r) clip_rect.r = screen.r;
    }

    top_font_overlap    = 0;
    bottom_font_overlap = 0;
    left_font_overlap   = 0;
    right_font_overlap  = 0;

    if (allow_oversize) { // added 21.6.02 --ralf
        if (clip_rect.t < screen.t) set_top_font_overlap(true);
        if (clip_rect.b > screen.b) set_bottom_font_overlap(true);
        if (clip_rect.l < screen.l) set_left_font_overlap(true);
        if (clip_rect.r > screen.r) set_right_font_overlap(true);
    }
}

int AW_clip::reduceClipBorders(int top, int bottom, int left, int right) {
    // return 0 if no clipping area left
    if (top    > clip_rect.t) clip_rect.t = top;
    if (bottom < clip_rect.b) clip_rect.b = bottom;
    if (left   > clip_rect.l) clip_rect.l = left;
    if (right  < clip_rect.r) clip_rect.r = right;

    return !(clip_rect.b<clip_rect.t || clip_rect.r<clip_rect.l);
}

void AW_clip::reduce_top_clip_border(int top) {
    if (top > clip_rect.t) clip_rect.t = top;
}

void AW_clip::set_top_clip_border(int top, bool allow_oversize) {
    clip_rect.t = top;
    if (!allow_oversize) {
        if (clip_rect.t < common->get_screen().t) clip_rect.t = common->get_screen().t;
    }
    else {
        set_top_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::reduce_bottom_clip_border(int bottom) {
    if (bottom < clip_rect.b)     clip_rect.b = bottom;
}

void AW_clip::set_bottom_clip_border(int bottom, bool allow_oversize) {
    clip_rect.b = bottom;
    if (!allow_oversize) {
        if (clip_rect.b > common->get_screen().b) clip_rect.b = common->get_screen().b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::set_bottom_clip_margin(int bottom, bool allow_oversize) {
    clip_rect.b -= bottom;
    if (!allow_oversize) {
        if (clip_rect.b > common->get_screen().b) clip_rect.b = common->get_screen().b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}
void AW_clip::reduce_left_clip_border(int left) {
    if (left > clip_rect.l)clip_rect.l = left;
}
void AW_clip::set_left_clip_border(int left, bool allow_oversize) {
    clip_rect.l = left;
    if (!allow_oversize) {
        if (clip_rect.l < common->get_screen().l) clip_rect.l = common->get_screen().l;
    }
    else {
        set_left_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::reduce_right_clip_border(int right) {
    if (right < clip_rect.r)    clip_rect.r = right;
}

void AW_clip::set_right_clip_border(int right, bool allow_oversize) {
    clip_rect.r = right;
    if (!allow_oversize) {
        if (clip_rect.r > common->get_screen().r) clip_rect.r = common->get_screen().r;
    }
    else {
        set_right_font_overlap(true); // added to correct problem with last char skipped (added 21.6.02 --ralf)
    }
}

void AW_clip::set_top_font_overlap(int val) {
    top_font_overlap = val;
}
void AW_clip::set_bottom_font_overlap(int val) {
    bottom_font_overlap = val;
}
void AW_clip::set_left_font_overlap(int val) {
    left_font_overlap = val;
}
void AW_clip::set_right_font_overlap(int val) {
    right_font_overlap = val;
}

AW_clip::AW_clip() {
    memset((char *)this, 0, sizeof(*this));
}

inline AW_pos clip_in_range(AW_pos low, AW_pos val, AW_pos high) {
    if (val <= low) return low;
    if (val >= high) return high;
    return val;
}

int AW_clip::box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out) {
    // clip coordinates of a box

    if (x1<clip_rect.l || x0>clip_rect.r) return 0;
    if (y1<clip_rect.t || y0>clip_rect.b) return 0;

    // @@@ refactor into method
    if (clip_rect.l>clip_rect.r) return 0;
    if (clip_rect.t>clip_rect.b) return 0;

    x0out = clip_in_range(clip_rect.l, x0, clip_rect.r);
    x1out = clip_in_range(clip_rect.l, x1, clip_rect.r);
    y0out = clip_in_range(clip_rect.t, y0, clip_rect.b);
    y1out = clip_in_range(clip_rect.t, y1, clip_rect.b);

    return 1;
}

int AW_clip::box_clip(const Rectangle& rect, Rectangle& clippedRect) {
    if (rect.distinct_from(clip_rect))
        return 0;
    
    // @@@ refactor into method
    if (clip_rect.l>clip_rect.r) return 0;
    if (clip_rect.t>clip_rect.b) return 0;

    clippedRect = rect.intersect_with(clip_rect);
    return 1;
}

int AW_clip::force_into_clipbox(const Position& pos, Position& forcedPos) {
    // force 'pos' inside 'clip_rect'

    // @@@ refactor into method
    if (clip_rect.l>clip_rect.r) return 0;
    if (clip_rect.t>clip_rect.b) return 0;

    forcedPos.setx(clip_in_range(clip_rect.l, pos.xpos(), clip_rect.r));
    forcedPos.sety(clip_in_range(clip_rect.t, pos.ypos(), clip_rect.b));
    return 1;
}

int AW_clip::clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out) {
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

int AW_clip::clip(const LineVector& line, LineVector& clippedLine) {
    AW_pos x0, y0, x1, y1;
    if (clip(line.start().xpos(), line.start().ypos(),
             line.head().xpos(), line.head().ypos(),
             x0, y0, x1, y1)) {
        clippedLine = LineVector(x0, y0, x1, y1);
        return 1;
    }
    return 0;
}

void AW_matrix::zoom(AW_pos val) {
    scale   *= val;
    unscale  = 1.0/scale;
}

void AW_matrix::reset() {
    unscale = scale   = 1.0;
    offset  = Vector(0, 0);
}

// -----------------
//      AW_GC_Xm

AW_GC_Xm::AW_GC_Xm(AW_common *commoni) {
    common     = commoni;
    line_width = 1;
    style      = AW_SOLID;
    function   = AW_COPY;
    color      = 0;
    grey_level = 0;

    XGCValues val;
    val.line_width = 1;
    unsigned long value_mask = GCLineWidth;

    gc = XCreateGC(common->get_display(), common->get_window_id(), value_mask, &val);
}


AW_GC_Xm::~AW_GC_Xm() {
    if (gc) XFreeGC(common->get_display(), gc);
}


void AW_GC_Xm::set_fill(AW_grey_level grey_leveli) {    // <0 don't fill  0.0 white 1.0 black
    grey_level = grey_leveli;
}


void AW_GC_Xm::set_lineattributes(AW_pos width, AW_linestyle stylei) {
    int lwidth = AW_INT(width);
    if (stylei == style && line_width == lwidth) return;

    switch (style) {
        case AW_SOLID:
            XSetLineAttributes(common->get_display(), gc, lwidth, LineSolid, CapButt, JoinBevel);
            break;
        case AW_DOTTED:
            XSetLineAttributes(common->get_display(), gc, lwidth, LineOnOffDash, CapButt, JoinBevel);
            break;
        default:
            break;
    }
    line_width = lwidth;
    style = style;
}


void AW_GC_Xm::set_function(AW_function mode)
{
    if (function != mode) {
        switch (mode) {
            case AW_XOR:
                XSetFunction(common->get_display(), gc, GXxor);
                break;
            case AW_COPY:
                XSetFunction(common->get_display(), gc, GXcopy);
                break;
        }
        function = mode;
        set_foreground_color(color);
    }
}

void AW_GC_Xm::set_foreground_color(unsigned long col) {
    color = (short)col;
    if (function == AW_XOR) col ^= common->get_XOR_color();
    XSetForeground(common->get_display(), gc, col);
    last_fg_color =  col;
}

void AW_GC_Xm::set_background_color(unsigned long colori) {
    XSetBackground(common->get_display(), gc, colori);
    last_bg_color = colori;
}

const AW_font_limits& AW_gc::get_font_limits(int gc, char c) const {
    return common->get_font_limits(gc, c);
}

// --------------
//      AW_gc

int AW_GC_Xm::get_string_size(const char *str, long textlen) const {
    // calculate display size of 'str'
    if (!textlen) {
        if (!str) return 0;
        textlen = strlen(str);
    }

    long l_width;
    if (curfont.max_bounds.width == curfont.min_bounds.width || !str) { // monospaced font
        l_width = textlen * curfont.max_bounds.width;
    }
    else { // non-monospaced font
        l_width = 0;
        for (int c = *(str++); c; c = *(str++)) {
            l_width += width_of_chars[c];
        }
    }
    return (int)l_width;
}

void AW_common::new_gc(int gc) {
    if (gc >= ngcs) {
        gcs = (AW_GC_Xm **)realloc((char *)gcs, sizeof(*gcs)*(gc+10));
        memset(&gcs[ngcs], 0, sizeof(*gcs) * (gc-ngcs+10));
        ngcs = gc+10;
    }
    if (gcs[gc]) delete gcs[gc];
    gcs[gc] = new AW_GC_Xm(this);
}

int AW_gc::get_string_size(int gc, const char *str, long textlen) const {
    return common->map_gc(gc)->get_string_size(str, textlen);
}
void AW_gc::new_gc(int gc) { common->new_gc(gc); }
void AW_gc::set_fill(int gc, AW_grey_level grey_level) { common->map_mod_gc(gc)->set_fill(grey_level); }
void AW_gc::set_font(int gc, AW_font font_nr, int size, int *found_size) {
    // if found_size != 0 -> return value for used font size
    common->map_mod_gc(gc)->set_font(font_nr, size, found_size);
}
int AW_gc::get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes) {
    return common->map_gc(gc)->get_available_fontsizes(font_nr, available_sizes);
}
void AW_gc::set_line_attributes(int gc, AW_pos width, AW_linestyle style) {
    common->map_mod_gc(gc)->set_lineattributes(width, style);
}
void AW_gc::set_function(int gc, AW_function function) {
    common->map_mod_gc(gc)->set_function(function);
}
void AW_gc::set_foreground_color(int gc, AW_color color) {
    common->map_mod_gc(gc)->set_foreground_color(common->get_color(color));
}
void AW_gc::set_background_color(int gc, AW_color color) {
    common->map_mod_gc(gc)->set_background_color(common->get_color(color));
}

void AW_get_common_extends_cb(AW_window */*aww*/, AW_CL cl_common, AW_CL) {
    AW_common    *common = (AW_common*)cl_common;
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

AW_common::AW_common(Display         *display_in,
                     XID              window_id_in,
                     unsigned long*&  fcolors,
                     unsigned long*&  dcolors,
                     long&            dcolors_count)
    : display(display_in),
      window_id(window_id_in),
      frame_colors(fcolors),
      data_colors(dcolors),
      data_colors_size(dcolors_count)
{
    ngcs = 8;
    gcs  = (AW_GC_Xm **)malloc(sizeof(void *)*ngcs);
    memset((char *)gcs, 0, sizeof(void *)*ngcs);

    screen.t = 0; 
    screen.b = -1;
    screen.l = 0;
    screen.r = -1;
}

void AW_common::install_common_extends_cb(AW_window *aww, AW_area area) {
    aww->set_resize_callback(area, AW_get_common_extends_cb, (AW_CL)this);
    AW_get_common_extends_cb(aww, (AW_CL)this, 0);
}

#if defined(DEBUG)
// #define SHOW_CLIP_STACK_CHANGES
#endif // DEBUG

class AW_clip_scale_stack {
    // completely private, but accessible by AW_device
    friend class AW_device;

    AW_rectangle clip_rect;

    int top_font_overlap;
    int bottom_font_overlap;
    int left_font_overlap;
    int right_font_overlap;

    Vector offset;
    AW_pos     scale;

    class AW_clip_scale_stack *next;
};

#if defined(SHOW_CLIP_STACK_CHANGES)
static const char *clipstatestr(AW_device *device) {
    static char   buffer[1024];
    AW_rectangle& clip_rect = device->clip_rect;

    sprintf(buffer, "clip_rect={t=%i, b=%i, l=%i, r=%i}",
            clip_rect.t, clip_rect.b, clip_rect.l, clip_rect.r);

    return buffer;
}
#endif // SHOW_CLIP_STACK_CHANGES



void AW_device::push_clip_scale()
{
    AW_clip_scale_stack *stack = new AW_clip_scale_stack;

    stack->next      = clip_scale_stack;
    clip_scale_stack = stack;

    stack->scale  = get_scale();
    stack->offset = get_offset();

    stack->top_font_overlap    = top_font_overlap;
    stack->bottom_font_overlap = bottom_font_overlap;
    stack->left_font_overlap   = left_font_overlap;
    stack->right_font_overlap  = right_font_overlap;

    stack->clip_rect = clip_rect;

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

    zoom(clip_scale_stack->scale);
    set_offset(clip_scale_stack->offset);

    clip_rect = clip_scale_stack->clip_rect;

    top_font_overlap    = clip_scale_stack->top_font_overlap;
    bottom_font_overlap = clip_scale_stack->bottom_font_overlap;
    left_font_overlap   = clip_scale_stack->left_font_overlap;
    right_font_overlap  = clip_scale_stack->right_font_overlap;

    AW_clip_scale_stack *oldstack = clip_scale_stack;
    clip_scale_stack              = clip_scale_stack->next;
    delete oldstack;

#if defined(SHOW_CLIP_STACK_CHANGES)
    printf("pop_clip_scale: %s -> %s\n", state_before_pop, clipstatestr(this));
    free(state_before_pop);
#endif // SHOW_CLIP_STACK_CHANGES
}

// --------------------------------------------------------------------------------

void AW_device::get_area_size(AW_rectangle *rect) {     // get the extends from the class AW_device
    *rect = common->get_screen();
}

void AW_device::get_area_size(AW_world *rect) { // get the extends from the class AW_device
    const AW_rectangle& screen = common->get_screen();
    
    rect->t = screen.t;
    rect->b = screen.b;
    rect->l = screen.l;
    rect->r = screen.r;
}

Rectangle AW_device::get_area_size() {
    return Rectangle(common->get_screen());
}

void AW_device::privat_reset() {}

void AW_device::reset() {
    while (clip_scale_stack) {
        pop_clip_scale();
    }
    get_area_size(&clip_rect);
    AW_matrix::reset();
    privat_reset();
}

AW_device::AW_device(class AW_common *commoni) : AW_gc() {
    common           = commoni;
    clip_scale_stack = 0;
    filter           = AW_ALL_DEVICES;
    click_cd         = 0;
}

AW_gc::AW_gc() : AW_clip() {}

bool AW_device::invisible_impl(int /*gc*/, AW_pos x, AW_pos y, AW_bitset filteri) {
    if (filteri & filter) {
        AW_pos X, Y;            // Transformed pos
        transform(x, y, X, Y);
        return ! (X<clip_rect.l || X>clip_rect.r ||
                  Y<clip_rect.t || Y>clip_rect.b);
    }
    return true;
}

bool AW_device::ready_to_draw(int gc) {
    return common->gc_mapable(gc);
}

int AW_device::generic_box(int gc, bool IF_DEBUG(filled), const Rectangle& rect, AW_bitset filteri) {
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

int AW_device::generic_filled_area(int gc, int npos, const Position *pos, AW_bitset filteri) {
    int erg = 0;
    if (filteri & filter) {
        int p = npos-1;
        for (int n = 0; n<npos; ++n) {
            erg |= line(gc, pos[p], pos[n], filteri);
            p    = n;
        }
    }
    return erg;
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

int AW_device::cursor(int gc, AW_pos x0, AW_pos y0, AW_cursor_type cur_type, AW_bitset filteri) {
    const AW_GC_Xm    *gcm = common->map_gc(gc);
    const XFontStruct *xfs = &gcm->curfont;
    AW_pos             x1, x2, y1, y2;
    AW_pos             X0, Y0;  // Transformed pos

    //  cursor insert         cursor overwrite
    //     (X0,Y0)
    //       /\                       .
    //      /  \                      .
    //      ----
    // (X1,Y1)(X2,Y2)

    if (filteri & filter) {
        if (cur_type == AW_cursor_insert) {
            transform(x0, y0, X0, Y0);

            if (X0 > clip_rect.r) return 0;
            if (X0 < clip_rect.l) return 0;
            if (Y0+(AW_pos)(xfs->max_bounds.descent) < clip_rect.t) return 0;
            if (Y0-(AW_pos)(xfs->max_bounds.ascent) > clip_rect.b) return 0;

            x1 = x0-4;
            y1 = y0+4;
            x2 = x0+4;
            y2 = y0+4;

            line(gc, x1, y1, x0, y0, filteri);
            line(gc, x2, y2, x0, y0, filteri);
            line(gc, x1, y1, x2, y2, filteri);
        }
    }
    return 1;
}

int AW_device::text_overlay(int gc, const char *opt_str, long opt_len,  // either string or strlen != 0
                            const Position& pos, AW_pos alignment, AW_bitset filteri, AW_CL cduser, 
                            AW_pos opt_ascent, AW_pos opt_descent,             // optional height (if == 0 take font height)
                            TextOverlayCallback toc)
{
    long               textlen;
    const AW_GC_Xm    *gcm           = common->map_gc(gc);
    const XFontStruct *xfs           = &gcm->curfont;
    const short       *size_per_char = gcm->width_of_chars;
    int                xi, yi;
    int                h;
    int                start;
    int                l;
    int                c             = 0;
    AW_pos             X0, Y0;  // Transformed pos

    bool inside_clipping_left  = true; // clipping at the left edge of the screen is different from clipping right of the left edge.
    bool inside_clipping_right = true;

    // es gibt 4 clipping Moeglichkeiten:
    // 1. man will fuer den Fall clippen, dass man vom linken display-Rand aus druckt   => clipping rechts vom 1. Buchstaben
    // 2. man will fuer den Fall clippen, dass man mitten im Bildschirm ist             => clipping links vom 1. Buchstaben
    // 3. man will fuer den Fall clippen, dass man mitten im Bildschirm ist             => clipping links vom letzten Buchstaben
    // 4. man will fuer den Fall clippen, dass man bis zum rechten display-Rand druckt  => clipping rechts vom letzten Buchstaben

    if (!(filter & filteri)) return 0;

    const AW_rectangle& screen = common->get_screen();

    if (left_font_overlap || screen.l == clip_rect.l) { // was : clip_rect.l == 0
        inside_clipping_left = false;
    }

    if (right_font_overlap || clip_rect.r == screen.r) { // was : clip_rect.r == screen.r

        inside_clipping_right = false;
    }

    transform(pos.xpos(), pos.ypos(), X0, Y0);


    if (top_font_overlap || clip_rect.t == 0) {                                                 // check clip border inside screen
        if (Y0+(AW_pos)(xfs->max_bounds.descent) < clip_rect.t) return 0; // draw outside screen
    }
    else {
        if (Y0-(AW_pos)(xfs->max_bounds.ascent) < clip_rect.t) return 0; // don't cross the clip border
    }

    if (bottom_font_overlap || clip_rect.b == screen.b) {                               // check clip border inside screen drucken
        if (Y0-(AW_pos)(xfs->max_bounds.ascent) > clip_rect.b) return 0;             // draw outside screen
    }
    else {
        if (Y0+(AW_pos)(xfs->max_bounds.descent)> clip_rect.b) return 0;             // don't cross the clip border
    }

    if (!opt_len) {
        opt_len = textlen = strlen(opt_str);
    }
    else {
        textlen = opt_len;
    }

    aw_assert(opt_len == textlen);
    aw_assert(int(strlen(opt_str)) >= textlen);

    if (alignment) {
        AW_pos width = get_string_size(gc, opt_str, textlen);
        X0 = X0-alignment*width;
    }
    xi = AW_INT(X0);
    yi = AW_INT(Y0);
    if (X0 > clip_rect.r) return 0;                           // right of screen

    l = (int)clip_rect.l;
    if (xi + textlen*xfs->max_bounds.width < l) return 0;               // left of screen

    start = 0;
    if (xi < l) {                                                               // now clip left side
        if (xfs->max_bounds.width == xfs->min_bounds.width) {           //  monospaced font
            h = (l - xi)/xfs->max_bounds.width;
            if (inside_clipping_left) {
                if ((l-xi)%xfs->max_bounds.width  >0) h += 1;
            }
            if (h >= textlen) return 0;
            start    = h;
            xi      += h*xfs->max_bounds.width;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }
        else {                                                         // non-monospaced font
            for (h=0; xi < l; h++) {
                if (!(c = opt_str[h])) return 0;
                xi += size_per_char[c];
            }
            if (!inside_clipping_left) {
                h-=1;
                xi -= size_per_char[c];
            }
            start    = h;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }
    }

    // now clipp right side
    if (xfs->max_bounds.width == xfs->min_bounds.width) {                       // monospaced font
        h = ((int)clip_rect.r - xi) / xfs->max_bounds.width;
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
    else {                                                                      // non-monospaced font
        l = (int)clip_rect.r - xi;
        for (h = start; l >= 0 && textlen > 0;  h++, textlen--) { // was textlen >= 0
            l -= size_per_char[safeCharIndex(opt_str[h])];
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

