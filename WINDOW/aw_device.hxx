#ifndef AW_DEVICE_HXX
#define AW_DEVICE_HXX

#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif
#ifndef _LIMITS_H
#include <limits.h>
#endif

#if defined(DEBUG) && defined(DEBUG_GRAPHICS)
// if you want flush() to be called after every motif command :
#define AUTO_FLUSH(device) (device)->flush()
#else
#define AUTO_FLUSH(device)
#endif

// #define AW_PIXELS_PER_MM 1.0001 // stupid and wrong

const AW_bitset AW_ALL_DEVICES = (AW_bitset)-1;
const AW_bitset AW_SCREEN      = 1;
const AW_bitset AW_CLICK       = 2;
const AW_bitset AW_CLICK_DRAG  = 4;
const AW_bitset AW_SIZE        = 8;
const AW_bitset AW_PRINTER     = 16; // print/xfig-export
const AW_bitset AW_PRINTER_EXT = 32; // (+Handles) use combined with AW_PRINTER only

typedef enum {
    AW_DEVICE_SCREEN  = 1,
    AW_DEVICE_CLICK   = 2,
    AW_DEVICE_SIZE    = 8,
    AW_DEVICE_PRINTER = 16
} AW_DEVICE_TYPE;

enum {
    AW_FIXED             = -1,
    AW_TIMES             = 0,
    AW_TIMES_ITALIC      = 1,
    AW_TIMES_BOLD        = 2,
    AW_TIMES_BOLD_ITALIC = 3,

    AW_COURIER              = 12,
    AW_COURIER_OBLIQUE      = 13,
    AW_COURIER_BOLD         = 14,
    AW_COURIER_BOLD_OBLIQUE = 15,

    AW_HELVETICA                     = 16,
    AW_HELVETICA_OBLIQUE             = 17,
    AW_HELVETICA_BOLD                = 18,
    AW_HELVETICA_BOLD_OBLIQUE        = 19,
    AW_HELVETICA_NARROW              = 20,
    AW_HELVETICA_NARROW_OBLIQUE      = 21,
    AW_HELVETICA_NARROW_BOLD         = 22,
    AW_HELVETICA_NARROW_BOLD_OBLIQUE = 23,

    AW_LUCIDA_SANS                 = 35,
    AW_LUCIDA_SANS_OBLIQUE         = 36,
    AW_LUCIDA_SANS_BOLD            = 37,
    AW_LUCIDA_SANS_BOLD_OBLIQUE    = 38,
    AW_LUCIDA_SANS_TYPEWRITER      = 39,
    AW_LUCIDA_SANS_TYPEWRITER_BOLD = 40,
    AW_SCREEN_MEDIUM               = 41,
    AW_SCREEN_BOLD                 = 42,
    AW_CLEAN_MEDIUM                = 43,
    AW_CLEAN_BOLD                  = 44,
    AW_TERMINAL_MEDIUM             = 45,
    AW_TERMINAL_BOLD               = 46,

    AW_NUM_FONTS = 47,

    AW_DEFAULT_NORMAL_FONT = AW_LUCIDA_SANS,
    AW_DEFAULT_BOLD_FONT   = AW_LUCIDA_SANS_BOLD,
    AW_DEFAULT_FIXED_FONT  = AW_LUCIDA_SANS_TYPEWRITER,

};      // AW_font

typedef enum {
    AW_cursor_insert,
    AW_cursor_overwrite
} AW_cursor_type;


// @@@ FIXME: elements of the following classes should go private!

class AW_clicked_element {
public:
    AW_CL client_data1;
    AW_CL client_data2;
    bool  exists;                                   // true if a drawn element was clicked, else false
};

class AW_clicked_line : public AW_clicked_element {
public:
    AW_pos x0, y0, x1, y1;  // @@@ make this a Rectangle
    AW_pos distance;        // min. distance to line
    AW_pos nearest_rel_pos; // 0 = at x0/y0, 1 = at x1/y1
};

class AW_clicked_text : public AW_clicked_element {
public:
    AW::Rectangle textArea;     // world coordinates of text
    AW_pos        alignment;
    AW_pos        rotation;
    AW_pos        distance;     // y-Distance to text, <0 -> above, >0 -> below
    AW_pos        dist2center;  // Distance to center of text
    int           cursor;       // which letter was selected, from 0 to strlen-1
    bool          exactHit;     // true -> real click on text (not only near text)
};

bool AW_getBestClick(AW_clicked_line *cl, AW_clicked_text *ct, AW_CL *cd1, AW_CL *cd2);

class AW_zoomable {
    AW::Vector offset;
    AW_pos     scale;
    AW_pos     unscale;         // = 1.0/scale

public:
    AW_zoomable() { this->reset(); };
    virtual ~AW_zoomable() {}

    void zoom(AW_pos scale);

    AW_pos get_scale() { return scale; };
    AW_pos get_unscale() { return unscale; };
    AW::Vector get_offset() const { return offset; }

    void rotate(AW_pos angle);

    void set_offset(const AW::Vector& off) { offset = off*scale; }
    void shift(const AW::Vector& doff) { offset += doff*scale; }

    void reset();

    double transform_size(const double& size) const { return size*scale; }
    double rtransform_size(const double& size) const { return size*unscale; }

    // transforming a Vector only scales the vector (a Vector has no position!)
    AW::Vector transform (const AW::Vector& vec) const { return vec*scale; }
    AW::Vector rtransform(const AW::Vector& vec) const { return vec*unscale; }

    // transform a Position
    AW::Position transform (const AW::Position& pos) const { return transform(AW::Vector(pos+offset)).endpoint(); }
    AW::Position rtransform(const AW::Position& pos) const { return rtransform(AW::Vector(pos)).endpoint()-offset; }
#if defined(WARN_TODO) && 0
#warning fix transformations
    // @@@ I think this calculation is wrong, cause offset is already scaled
    //     (same applies to old-style transform/rtransform below)
#endif

    AW::LineVector transform (const AW::LineVector& lvec) const { return AW::LineVector(transform(lvec.start()), transform(lvec.line_vector())); }
    AW::LineVector rtransform(const AW::LineVector& lvec) const { return AW::LineVector(rtransform(lvec.start()), rtransform(lvec.line_vector())); }

    AW::Rectangle transform (const AW::Rectangle& rect) const { return AW::Rectangle(transform(static_cast<const AW::LineVector&>(rect))); }
    AW::Rectangle rtransform(const AW::Rectangle& rect) const { return AW::Rectangle(rtransform(static_cast<const AW::LineVector&>(rect))); }
    
    // old style functions, not preferred:
    void transform(int x, int y, int& xout, int& yout) const {
        xout = int((x+offset.x())*scale);
        yout = int((y+offset.y())*scale);
    }
    void transform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = (x+offset.x())*scale;
        yout = (y+offset.y())*scale;
    }

    void rtransform(int x, int y, int& xout, int& yout) const {
        xout = int(x*unscale - offset.x());
        yout = int(y*unscale - offset.y());
    }
    void rtransform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = x*unscale - offset.x();
        yout = y*unscale - offset.y();
    }
};

class AW_clipable {
    const AW_rectangle& common_screen;
    const AW_rectangle& get_screen() const { return common_screen; }

protected:
    int compoutcode(AW_pos xx, AW_pos yy) {
        /* calculate outcode for clipping the current line */
        /* order - top,bottom,right,left */
        int code = 0;
        if (clip_rect.b - yy < 0)       code = 4;
        else if (yy - clip_rect.t < 0)  code = 8;
        if (clip_rect.r - xx < 0)       code |= 2;
        else if (xx - clip_rect.l < 0)  code |= 1;
        return (code);
    };
    
public:

    // ****** read only section
    AW_rectangle clip_rect;     // holds the clipping rectangle coordinates
    int top_font_overlap;
    int bottom_font_overlap;
    int left_font_overlap;
    int right_font_overlap;

    // ****** real public

    int clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    int clip(const AW::LineVector& line, AW::LineVector& clippedLine);

    int box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    int box_clip(const AW::Rectangle& rect, AW::Rectangle& clippedRect);
    int force_into_clipbox(const AW::Position& pos, AW::Position& forcedPos);

    void set_top_clip_border(int top, bool allow_oversize = false);
    void set_bottom_clip_border(int bottom, bool allow_oversize = false); // absolute
    void set_bottom_clip_margin(int bottom, bool allow_oversize = false); // relative
    void set_left_clip_border(int left, bool allow_oversize = false);
    void set_right_clip_border(int right, bool allow_oversize = false);
    void set_cliprect(AW_rectangle *rect, bool allow_oversize = false);
    void set_clipall() {
        AW_rectangle rect;
        rect.t = rect.b = rect.l = rect.r = 0;
        set_cliprect(&rect);     // clip all -> nothing drawn afterwards
    }


    void set_top_font_overlap(int val=1);
    void set_bottom_font_overlap(int val=1);
    void set_left_font_overlap(int val=1);
    void set_right_font_overlap(int val=1);

    // like set_xxx_clip_border but make window only smaller:

    void reduce_top_clip_border(int top);
    void reduce_bottom_clip_border(int bottom);
    void reduce_left_clip_border(int left);
    void reduce_right_clip_border(int right);

    int reduceClipBorders(int top, int bottom, int left, int right);

    AW_clipable(const AW_rectangle& screen)
        : common_screen(screen),
          top_font_overlap(0),
          bottom_font_overlap(0),
          left_font_overlap(0),
          right_font_overlap(0)
    {
        clip_rect.clear();
    }

    virtual ~AW_clipable() {}
};

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
        : ascent(std::max(lim1.ascent, lim2.ascent))
        , descent(std::max(lim1.descent, lim2.descent))
        , width(std::max(lim1.width, lim2.width))
    {
        calc_height();
    }
};

// -----------------------------------------------
//      Graphic context (linestyle, width ...)

typedef enum {
    AW_SOLID,
    AW_DOTTED
} AW_linestyle;


typedef enum {
    AW_COPY,
    AW_XOR
} AW_function;

class AW_common;

class AW_stylable : virtual Noncopyable {
    AW_common *common;
public:
    AW_stylable(AW_common *common_) : common(common_) {}
    virtual ~AW_stylable() {};
    
    AW_common *get_common() const { return common; }

    void new_gc(int gc);
    void set_fill(int gc, AW_grey_level grey_level); 
    void set_font(int gc, AW_font fontnr, int size, int *found_size);
    void set_line_attributes(int gc, AW_pos width, AW_linestyle style);
    void set_function(int gc, AW_function function);
    void set_foreground_color(int gc, AW_color color); // lines ....
    int  get_string_size(int gc, const  char *string, long textlen) const; // get the size of the string

    const AW_font_limits& get_font_limits(int gc, char c) const; // for one characters (c == 0 -> for all characters)

    int get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes);
};

class  AW_clip_scale_stack;
struct AW_world;
class  AW_device;

class AW_click_cd : virtual Noncopyable {
    AW_CL              cd1;
    AW_CL              cd2;
    AW_device         *my_device;
    const AW_click_cd *previous;

    void link();

public:
    AW_click_cd(AW_device *device, AW_CL CD1, AW_CL CD2)
        : cd1(CD1),
          cd2(CD2),
          my_device(device)
    { link(); }
    AW_click_cd(AW_device *device, AW_CL CD1)
        : cd1(CD1),
          cd2(0),
          my_device(device)
    { link(); }
    ~AW_click_cd();

    void disable();
    void enable();

    AW_CL get_cd1() const { return cd1; }
    AW_CL get_cd2() const { return cd2; }

    void set_cd1(AW_CL cd) { cd1 = cd; }
    void set_cd2(AW_CL cd) { cd2 = cd; }
};

typedef int (*TextOverlayCallback)(AW_device *device, int gc, const char *opt_string, size_t opt_string_len, size_t start, size_t size, AW_pos x, AW_pos y, AW_pos opt_ascent, AW_pos opt_descent, AW_CL cduser);

class AW_device : public AW_zoomable, public AW_stylable, public AW_clipable {
    AW_device(const AW_device& other);
    AW_device& operator=(const AW_device& other);

protected:
    AW_clip_scale_stack *clip_scale_stack;
    virtual         void  privat_reset();

    const AW_click_cd *click_cd;
    friend class       AW_click_cd;

    AW_bitset filter;

    static const AW_rectangle& get_common_screen(const AW_common *common_);
    
public:
    AW_device(class AW_common *common_)
        : AW_stylable(common_),
          AW_clipable(get_common_screen(common_)),
          clip_scale_stack(NULL),
          click_cd(NULL), 
          filter(AW_ALL_DEVICES) 
    {}
    virtual ~AW_device() {}

    const AW_click_cd *get_click_cd() const { return click_cd; }
    AW_bitset get_filter() const { return filter; }

    void reset();

    void          get_area_size(AW_rectangle *rect); // read the frame size
    void          get_area_size(AW_world *rect); // read the frame size
    AW::Rectangle get_area_size();

    void set_filter(AW_bitset filteri);   // set the main filter mask

    void push_clip_scale();     // push clipping area and scale
    void pop_clip_scale();     // pop them

    virtual AW_DEVICE_TYPE type() = 0;

    bool ready_to_draw(int gc); // unused atm

    // * functions below return 1 if any pixel is drawn, 0 otherwise
    // * primary functions (always virtual)

private:
    virtual int line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri)                                                  = 0;
    virtual int text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) = 0;
    virtual int box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri)                                       = 0;
    virtual int filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri)                                    = 0;

    virtual int circle_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri)                                  = 0;
    virtual int arc_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri) = 0;

protected:
    virtual bool invisible_impl(int gc, AW_pos x, AW_pos y, AW_bitset filteri); // returns true if x/y is outside viewport (or if it would now be drawn undrawn)

    // * second level functions (maybe non virtual)

protected:
    int generic_box(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    int generic_circle(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri) {
        AW::Vector toCorner(xradius, yradius);
        return generic_box(gc, filled, AW::Rectangle(center+toCorner, center-toCorner), filteri);
    }
    int generic_arc(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int /*start_degrees*/, int /*arc_degrees*/, AW_bitset filteri) {
        return generic_circle(gc, filled, center, xradius, yradius, filteri);
    }
    int generic_filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri);

public:
    // * third level functions (never virtual)

    int line(int gc, const AW::LineVector& Line, AW_bitset filteri = AW_ALL_DEVICES) {
        return line_impl(gc, Line, filteri);
    }
    int line(int gc, const AW::Position& pos1, const AW::Position& pos2, AW_bitset filteri = AW_ALL_DEVICES) {
        return line(gc, AW::LineVector(pos1, pos2), filteri);
    }
    int line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri = AW_ALL_DEVICES) {
        return line(gc, AW::LineVector(x0, y0, x1, y1), filteri);
    }

    int text(int         gc,
             const char *string,
             AW_pos      x,
             AW_pos      y,
             AW_pos      alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
             AW_bitset   filteri    = AW_ALL_DEVICES,
             long        opt_strlen = 0) {
        return text_impl(gc, string, AW::Position(x, y), alignment, filteri, opt_strlen);
    }
    int text(int                  gc,
             const char          *string,
             const AW::Position&  pos,
             AW_pos               alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
             AW_bitset            filteri    = AW_ALL_DEVICES,
             long                 opt_strlen = 0) {
        return text_impl(gc, string, pos, alignment, filteri, opt_strlen);
    }

    bool invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri = AW_ALL_DEVICES) {
        return invisible_impl(gc, x, y, filteri);
    }
    bool invisible(int gc, AW::Position pos, AW_bitset filteri = AW_ALL_DEVICES) {
        return invisible_impl(gc, pos.xpos(), pos.ypos(), filteri);
    }

    int box(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES) {
        return box_impl(gc, filled, rect, filteri);
    }
    int box(int gc, bool filled, const AW::Position& pos, const AW::Vector& size, AW_bitset filteri = AW_ALL_DEVICES) {
        return box(gc, filled, AW::Rectangle(pos, size), filteri);
    }
    int box(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri = AW_ALL_DEVICES) {
        return box(gc, filled, AW::Position(x0, y0), AW::Vector(width, height), filteri);
    }

    int circle(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri = AW_ALL_DEVICES) {
        return circle_impl(gc, filled, center, xradius, yradius, filteri);
    }
    int circle(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, AW_bitset filteri = AW_ALL_DEVICES)  {
        return circle_impl(gc, filled, AW::Position(x0, y0), xradius, yradius, filteri);
    }
    int circle(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES) {
        // draw ellipse into Rectangle
        return circle_impl(gc, filled, rect.centroid(), rect.width()/2, rect.height()/2, filteri);
    }

    int arc(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES)  {
        return arc_impl(gc, filled, AW::Position(x0, y0), xradius, yradius, start_degrees, arc_degrees, filteri);
    }
    int arc(int gc, bool filled, const AW::Position& pos, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES) {
        return arc_impl(gc, filled, pos, xradius, yradius, start_degrees, arc_degrees, filteri);
    }

    int filled_area(int gc, int npoints, const AW_pos *points, AW_bitset filteri = AW_ALL_DEVICES)  {
        AW::Position pos[npoints];
        for (int n = 0; n<npoints; ++n) {
            pos[n].setx(points[n*2]);
            pos[n].sety(points[n*2+1]);
        }
        return filled_area_impl(gc, npoints, pos, filteri);
    }
    int filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri = AW_ALL_DEVICES)  {
        return filled_area_impl(gc, npos, pos, filteri);
    }

    // reduces any string (or virtual string) to its actual drawn size and calls the function f with the result
    int text_overlay(int gc, const char *opt_string, long opt_strlen,   // either string or strlen != 0
                     const AW::Position& pos, AW_pos alignment, AW_bitset filteri, AW_CL cduser, 
                     AW_pos opt_ascent, AW_pos opt_descent,  // optional height (if == 0 take font height)
                     TextOverlayCallback toc);


    // ********* X11 Device only ********
    virtual void    clear(AW_bitset filteri);
    virtual void    clear_part(AW_pos x, AW_pos y, AW_pos width, AW_pos height, AW_bitset filteri);

    void clear_part(const AW::Rectangle&rect, AW_bitset filteri) { clear_part(rect.xpos(), rect.ypos(), rect.width(), rect.height(), filteri); }

    virtual void    clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri);
    virtual void    move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y);
    virtual void    fast();                                         // e.g. zoom linewidth off
    virtual void    slow();
    virtual void    flush();                                        // empty X11 buffers
};


inline void AW_click_cd::link() {
    previous            = my_device->click_cd;
    my_device->click_cd = this;
}
inline AW_click_cd::~AW_click_cd() { my_device->click_cd = previous; }
inline void AW_click_cd::disable() { my_device->click_cd = NULL; }
inline void AW_click_cd::enable() { my_device->click_cd = this; }

class AW_device_print : public AW_device { // derived from a Noncopyable
    FILE *out;
    bool  color_mode;

    int line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    int text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    int box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    int circle_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri);
    int arc_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri) {
        return generic_arc(gc, filled, center, xradius, yradius, start_degrees, arc_degrees, filteri);
    }
    int filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri);
public:
    AW_device_print(AW_common *common_)
        : AW_device(common_),
          out(0),
          color_mode(false)
    {}

    void init() {}
    GB_ERROR open(const char *path) __ATTR__USERESULT;
    void close();

    FILE *get_FILE() { return out; }

    // AW_device interface:
    AW_DEVICE_TYPE type();

    int find_color_idx(unsigned long color);
    void set_color_mode(bool mode);

};

class AW_simple_device : public AW_device {
    int box_impl(int gc, bool /*filled*/, const AW::Rectangle& rect, AW_bitset filteri) {
        return generic_box(gc, false, rect, filteri);
    }
    int filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    int circle_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, AW_bitset filteri) {
        return generic_circle(gc, filled, center, xradius, yradius, filteri);
    }
    int arc_impl(int gc, bool filled, const AW::Position& center, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri) {
        return generic_arc(gc, filled, center, xradius, yradius, start_degrees, arc_degrees, filteri);
    }
public:
    AW_simple_device(AW_common *common_) : AW_device(common_) {}
};
    
class AW_device_size : public AW_simple_device {
    bool     drawn;
    AW_world size_information;
    void     privat_reset();

    void dot_transformed(AW_pos X, AW_pos Y);
    void dot_transformed(const AW::Position& p) { dot_transformed(p.xpos(), p.ypos()); }

    void dot(AW_pos x, AW_pos y) { dot(AW::Position(x, y)); }
    void dot(const AW::Position& p) { dot_transformed(transform(p)); }

    int line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    int text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool invisible_impl(int gc, AW_pos x, AW_pos y, AW_bitset filteri);

public:
    AW_device_size(AW_common *common_) : AW_simple_device(common_) {}

    void           init();
    AW_DEVICE_TYPE type();

    void get_size_information(AW_world *ptr);
};

class AW_device_click : public AW_simple_device {
    AW_pos          mouse_x, mouse_y;
    AW_pos          max_distance_line;
    AW_pos          max_distance_text;

    int line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    int text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);

public:
    AW_clicked_line opt_line;
    AW_clicked_text opt_text;

    AW_device_click(AW_common *common_) : AW_simple_device(common_) {}

    AW_DEVICE_TYPE type();

    void init(AW_pos mousex, AW_pos mousey, AW_pos max_distance_liniei, AW_pos max_distance_texti, AW_pos radi, AW_bitset filteri);

    void get_clicked_line(class AW_clicked_line *ptr);
    void get_clicked_text(class AW_clicked_text *ptr);
};


#else
#error aw_device.hxx included twice
#endif
