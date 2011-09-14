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

#define AW_INT(x) ((int)(((x)>=0.0) ? ((float)(x)+.5) : ((float)(x)-.5)))

// #define AW_PIXELS_PER_MM 1.0001 // stupid and wrong

#define DPI_SCREEN  80   // fixed
#define DPI_PRINTER 1200 // default resolution of xfig 3.2

const AW_bitset AW_SCREEN        = 1;
const AW_bitset AW_CLICK         = 2;
const AW_bitset AW_CLICK_DROP    = 4;
const AW_bitset AW_SIZE          = 8;
const AW_bitset AW_SIZE_UNSCALED = 16;  // for text and text-size dependant parts
const AW_bitset AW_PRINTER       = 32;  // print/xfig-export
const AW_bitset AW_PRINTER_EXT   = 64;  // (+Handles) use combined with AW_PRINTER only
const AW_bitset AW_PRINTER_CLIP  = 128; // print screen only

const AW_bitset AW_ALL_DEVICES          = (AW_bitset)-1; // @@@ allowed to used this ? 
const AW_bitset AW_ALL_DEVICES_SCALED   = (AW_ALL_DEVICES & ~AW_SIZE_UNSCALED);
const AW_bitset AW_ALL_DEVICES_UNSCALED = (AW_ALL_DEVICES & ~AW_SIZE);

enum AW_DEVICE_TYPE {
    AW_DEVICE_SCREEN  = AW_SCREEN,
    AW_DEVICE_CLICK   = AW_CLICK,
    AW_DEVICE_SIZE    = AW_SIZE,
    AW_DEVICE_PRINTER = AW_PRINTER, 
};

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
};

enum AW_cursor_type {
    AW_cursor_insert,
    AW_cursor_overwrite
};

class  AW_common;
class  AW_clip_scale_stack;
class  AW_device;
struct AW_world;

// @@@ FIXME: elements of the following classes should go private!

class AW_clicked_element {
public: // @@@ make private
    AW_CL client_data1;
    AW_CL client_data2;
    bool  exists;           // true if a drawn element was clicked, else false
    int   distance;         // distance in pixel to nearest line/text

    AW_pos nearest_rel_pos;        // 0 = at left(upper) small-side, 1 = at right(lower) small-side of textArea
    
protected:

    AW_clicked_element() : client_data1(0), client_data2(0), exists(false), distance(-1), nearest_rel_pos(0) {}
    virtual ~AW_clicked_element() {}

public:

    AW_CL cd1() const { return client_data1; }
    AW_CL cd2() const { return client_data2; }

    virtual AW::Position get_attach_point() const = 0;
    virtual bool is_text() const                  = 0;
    virtual int draw(AW_device *d, int gc) const  = 0;

    bool is_line() const { return !is_text(); }

    double get_rel_pos() const { return nearest_rel_pos; }
    void set_rel_pos(double rel) { aw_assert(rel >= 0.0 && rel <= 1.0); nearest_rel_pos = rel; }

    AW::LineVector get_connecting_line(const AW_clicked_element& other) const {
        return AW::LineVector(get_attach_point(), other.get_attach_point());
    }
};

class AW_clicked_line : public AW_clicked_element {
public:
    AW_pos x0, y0, x1, y1;  // @@@ make this a LineVector

    AW_clicked_line() : x0(0), y0(0), x1(0), y1(0) {}

    bool is_text() const { return false; }
    bool operator == (const AW_clicked_line& other) const { return nearlyEqual(get_line(), other.get_line()); }

    AW::Position get_attach_point() const {
        double nrp = get_rel_pos();
        return AW::Position(x0*(1-nrp)+x1*nrp,
                            y0*(1-nrp)+y1*nrp);
    }

    AW::LineVector get_line() const { return AW::LineVector(x0, y0, x1, y1); }
    int draw(AW_device *d, int gc) const;
};

class AW_clicked_text : public AW_clicked_element {
public:
    AW::Rectangle textArea;     // world coordinates of text
    int           cursor;       // which letter was selected, from 0 to strlen-1 [or -1 if not 'exactHit']
    bool          exactHit;     // true -> real click inside text bounding-box (not only near text) (@@@ redundant: exactHit == (distance == 0))

    AW_clicked_text() : cursor(-1), exactHit(false) {}

    bool is_text() const { return true; }
    bool operator == (const AW_clicked_text& other) const { return nearlyEqual(textArea, other.textArea); }

    AW::Position get_attach_point() const {
        return textArea.centroid(); // @@@ use center atm
    }
    int draw(AW_device *d, int gc) const;
};

const AW_clicked_element *AW_getBestClick(const AW_clicked_line *cl, const AW_clicked_text *ct);

// --------------------------------------------------
// general note on world- vs. pixel-positions:(WORLD_vs_PIXEL)
// 
// A position is interpreted as the center of the corresponding pixel
// (pixel refers to screen; printer pixel are 15 times smaller!)
// 
// Hence, when scaling factor is 1.0, then
// - any position inside [-0.5, 0.5[ will fall into the pixel 0, any inside [0.5, 1.5[ into pixel 1.
// - a line from 0.0 to 2.0 will paint THREE pixels (0, 1 and 2). A line from -0.5 to 2.499 will to the same
// - clipping to area [0, 100] should in fact clip to [-0.5, 100.5[ (@@@ check this)

class AW_zoomable {
    AW::Vector offset;
    AW_pos     scale;
    AW_pos     unscale;         // = 1.0/scale

public:
    AW_zoomable() { this->reset(); };
    virtual ~AW_zoomable() {}

    void zoom(AW_pos scale);

    AW_pos get_scale() const { return scale; };
    AW_pos get_unscale() const { return unscale; };
    AW::Vector get_offset() const { return offset; }

    void rotate(AW_pos angle);

    void set_offset(const AW::Vector& off) { offset = off*scale; }
    void shift(const AW::Vector& doff) { offset += doff*scale; }

    void reset();

    double transform_size(const double& size) const { return size*scale; }
    double rtransform_size(const double& size) const { return size*unscale; }

    double rtransform_pixelsize(int pixelsize) const {
        // return world-size needed to draw line/box with length/size == 'pixelsize'
        return (pixelsize-1)*unscale;
    }

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
    void transform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = (x+offset.x())*scale;
        yout = (y+offset.y())*scale;
    }
    void rtransform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = x*unscale - offset.x();
        yout = y*unscale - offset.y();
    }
};

struct AW_font_overlap { bool top, bottom, left, right; };

class AW_clipable {
    const AW_screen_area& common_screen;
    const AW_screen_area& get_screen() const { return common_screen; }

    AW_screen_area  clip_rect;    // holds the clipping rectangle coordinates
    AW_font_overlap font_overlap;

    void set_cliprect_oversize(const AW_screen_area& rect, bool allow_oversize);
protected:
    int compoutcode(AW_pos xx, AW_pos yy) const {
        // calculate outcode for clipping the current line
        // order - top,bottom,right,left
        int code = 0;
        if (clip_rect.b - yy < 0)       code = 4;
        else if (yy - clip_rect.t < 0)  code = 8;
        if (clip_rect.r - xx < 0)       code |= 2;
        else if (xx - clip_rect.l < 0)  code |= 1;
        return (code);
    };

    void set_cliprect(const AW_screen_area& rect) { clip_rect = rect; }

public:
    
    AW_clipable(const AW_screen_area& screen)
        : common_screen(screen)
    {
        clip_rect.clear();
        set_font_overlap(false);
    }
    virtual ~AW_clipable() {}

    bool is_below_clip(double ypos) const { return ypos > clip_rect.b; }
    bool is_above_clip(double ypos) const { return ypos < clip_rect.t; }
    bool is_leftof_clip(double xpos) const { return xpos < clip_rect.l; }
    bool is_rightof_clip(double xpos) const { return xpos > clip_rect.r; }

    bool is_outside_clip(AW::Position pos) const {
        return
            is_below_clip(pos.ypos()) || is_above_clip(pos.ypos()) ||
            is_leftof_clip(pos.xpos()) || is_rightof_clip(pos.xpos());
    }
    bool is_outside_clip(AW::Rectangle rect) const {
        return !rect.overlaps_with(AW::Rectangle(get_cliprect(), AW::INCLUSIVE_OUTLINE));
    }

    bool clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    bool clip(const AW::LineVector& line, AW::LineVector& clippedLine);

    bool box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    bool box_clip(const AW::Rectangle& rect, AW::Rectangle& clippedRect);
    bool force_into_clipbox(const AW::Position& pos, AW::Position& forcedPos);

    void set_top_clip_border(int top, bool allow_oversize = false);
    void set_bottom_clip_border(int bottom, bool allow_oversize = false); // absolute
    void set_bottom_clip_margin(int bottom, bool allow_oversize = false); // relative
    void set_left_clip_border(int left, bool allow_oversize = false);
    void set_right_clip_border(int right, bool allow_oversize = false);
    const AW_screen_area& get_cliprect() const { return clip_rect; }

    void set_clipall() {
        // clip all -> nothing drawn afterwards
        AW_screen_area rect = { 0, -1, 0, -1};
        set_cliprect_oversize(rect, false);
    }

    bool completely_clipped() const { return clip_rect.l>clip_rect.r || clip_rect.t>clip_rect.b; }

    bool allow_top_font_overlap() const { return font_overlap.top; }
    bool allow_bottom_font_overlap() const { return font_overlap.bottom; }
    bool allow_left_font_overlap() const { return font_overlap.left; }
    bool allow_right_font_overlap() const { return font_overlap.right; }
    const AW_font_overlap& get_font_overlap() const { return font_overlap; }
    
    void set_top_font_overlap(bool allow) { font_overlap.top = allow; }
    void set_bottom_font_overlap(bool allow) { font_overlap.bottom = allow; }
    void set_left_font_overlap(bool allow) { font_overlap.left = allow; }
    void set_right_font_overlap(bool allow) { font_overlap.right = allow; }

    void set_vertical_font_overlap(bool allow) { font_overlap.top = font_overlap.bottom = allow; }
    void set_horizontal_font_overlap(bool allow) { font_overlap.left = font_overlap.right = allow; }
    void set_font_overlap(bool allow) { set_vertical_font_overlap(allow); set_horizontal_font_overlap(allow); }
    void set_font_overlap(const AW_font_overlap& fo) { font_overlap = fo; }

    // like set_xxx_clip_border but make window only smaller:

    void reduce_top_clip_border(int top);
    void reduce_bottom_clip_border(int bottom);
    void reduce_left_clip_border(int left);
    void reduce_right_clip_border(int right);

    int reduceClipBorders(int top, int bottom, int left, int right);
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
        : ascent(std::max(lim1.ascent, lim2.ascent)),
          descent(std::max(lim1.descent, lim2.descent)),
          width(std::max(lim1.width, lim2.width)),
          min_width(std::min(lim1.min_width, lim2.min_width))
    {
        calc_height();
    }
};

// -----------------------------------------------
//      Graphic context (linestyle, width ...)

enum AW_linestyle {
    AW_SOLID,
    AW_DASHED,
    AW_DOTTED, 
};


enum AW_function {
    AW_COPY,
    AW_XOR
};

class AW_stylable : virtual Noncopyable {
    AW_common *common;
public:
    AW_stylable(AW_common *common_) : common(common_) {}
    virtual ~AW_stylable() {};
    
    AW_common *get_common() const { return common; }

    void new_gc(int gc);
    void set_grey_level(int gc, AW_grey_level grey_level); 
    void set_font(int gc, AW_font fontnr, int size, int *found_size);
    void set_line_attributes(int gc, short width, AW_linestyle style);
    void set_function(int gc, AW_function function);
    void establish_default(int gc);
    void set_foreground_color(int gc, AW_color_idx color); // lines ....
    int  get_string_size(int gc, const  char *string, long textlen) const; // get the size of the string

    const AW_font_limits& get_font_limits(int gc, char c) const; // for one characters (c == 0 -> for all characters)

    int get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes);
    
    void reset_style();
};

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

typedef bool (*TextOverlayCallback)(AW_device *device, int gc, const char *opt_string, size_t opt_string_len, size_t start, size_t size, AW_pos x, AW_pos y, AW_pos opt_ascent, AW_pos opt_descent, AW_CL cduser);

class AW_device : public AW_zoomable, public AW_stylable, public AW_clipable {
    AW_device(const AW_device& other);
    AW_device& operator=(const AW_device& other);

protected:
    AW_clip_scale_stack *clip_scale_stack;

    const AW_click_cd *click_cd;
    friend class       AW_click_cd;

    AW_bitset filter;

    static const AW_screen_area& get_common_screen(const AW_common *common_);
    
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

    const AW_screen_area& get_area_size() const;
    AW::Rectangle get_rtransformed_cliprect() const { return rtransform(AW::Rectangle(get_cliprect(), AW::INCLUSIVE_OUTLINE)); }

    void set_filter(AW_bitset filteri);   // set the main filter mask

    void push_clip_scale();     // push clipping area and scale
    void pop_clip_scale();     // pop them

    virtual AW_DEVICE_TYPE type() = 0;

    bool ready_to_draw(int gc); // unused atm

private:
    // * functions below return 1 if any pixel is drawn, 0 otherwise
    // * primary functions (always virtual; pure virtual in all devices used as baseclass)

    virtual bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri)                                                  = 0;
    virtual bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) = 0;
    virtual bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri)                                       = 0;
    virtual bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri)                                    = 0;

    virtual bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri)                                  = 0;
    virtual bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) = 0;

    virtual bool invisible_impl(const AW::Position& pos, AW_bitset filteri) = 0;

    virtual void specific_reset() = 0;

protected:

    // * second level functions
    // generic implementations which may be used by primary functions of derived classes 
    
    bool generic_box(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    bool generic_circle(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
        return generic_box(gc, filled, AW::Rectangle(center-radius, center+radius), filteri);
    }
    bool generic_arc(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int /*start_degrees*/, int /*arc_degrees*/, AW_bitset filteri) {
        return generic_circle(gc, filled, center, radius, filteri);
    }
    bool generic_filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri);
    bool generic_invisible(const AW::Position& pos, AW_bitset filteri);

public:
    // * third level functions (never virtual/overloaded by derived classes)

    bool line(int gc, const AW::LineVector& Line, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return line_impl(gc, Line, filteri);
    }
    bool line(int gc, const AW::Position& pos1, const AW::Position& pos2, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return line_impl(gc, AW::LineVector(pos1, pos2), filteri);
    }
    bool line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return line_impl(gc, AW::LineVector(x0, y0, x1, y1), filteri);
    }

    bool text(int         gc,
              const char *string,
              AW_pos      x,
              AW_pos      y,
              AW_pos      alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
              AW_bitset   filteri    = AW_ALL_DEVICES_UNSCALED,
              long        opt_strlen = 0) {
        return text_impl(gc, string, AW::Position(x, y), alignment, filteri, opt_strlen);
    }
    bool text(int                  gc,
              const char          *string,
              const AW::Position&  pos,
              AW_pos               alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
              AW_bitset            filteri    = AW_ALL_DEVICES_UNSCALED,
              long                 opt_strlen = 0) {
        return text_impl(gc, string, pos, alignment, filteri, opt_strlen);
    }

    bool invisible(const AW::Position& pos, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return invisible_impl(pos, filteri);
    }

    bool box(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return box_impl(gc, filled, rect, filteri);
    }
    bool box(int gc, bool filled, const AW::Position& pos, const AW::Vector& size, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return box_impl(gc, filled, AW::Rectangle(pos, size), filteri);
    }
    bool box(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return box_impl(gc, filled, AW::Rectangle(AW::Position(x0, y0), AW::Vector(width, height)), filteri);
    }

    bool circle(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return circle_impl(gc, filled, center, radius, filteri);
    }
    bool circle(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        return circle_impl(gc, filled, AW::Position(x0, y0), AW::Vector(xradius, yradius), filteri);
    }
    bool circle(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        // draw ellipse into Rectangle
        return circle_impl(gc, filled, rect.centroid(), AW::Vector(rect.width()/2, rect.height()/2), filteri);
    }

    // draw arcs (Note: passed degrees are nagative compared to unit circle!)
    bool arc(int gc, bool filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        return arc_impl(gc, filled, AW::Position(x0, y0), AW::Vector(xradius, yradius), start_degrees, arc_degrees, filteri);
    }
    bool arc(int gc, bool filled, const AW::Position& pos, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        return arc_impl(gc, filled, pos, radius, start_degrees, arc_degrees, filteri);
    }

    // @@@ rename to 'polygone' and pass 'filled' parameter
    bool filled_area(int gc, int npoints, const AW_pos *points, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        AW::Position pos[npoints];
        for (int n = 0; n<npoints; ++n) {
            pos[n].setx(points[n*2]);
            pos[n].sety(points[n*2+1]);
        }
        return filled_area_impl(gc, npoints, pos, filteri);
    }
    bool filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        return filled_area_impl(gc, npos, pos, filteri);
    }

    // reduces any string (or virtual string) to its actual drawn size and calls the function f with the result
    bool text_overlay(int gc, const char *opt_string, long opt_strlen,   // either string or strlen != 0
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

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri);
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri);
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri);
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri);

    void specific_reset() {}

public:
    AW_device_print(AW_common *common_)
        : AW_device(common_),
          out(0),
          color_mode(false)
    {}

    GB_ERROR open(const char *path) __ATTR__USERESULT;
    void close();

    FILE *get_FILE() { return out; }

    // AW_device interface:
    AW_DEVICE_TYPE type();

    int find_color_idx(AW_rgb color);
    void set_color_mode(bool mode);

};

class AW_simple_device : public AW_device {
    bool box_impl(int gc, bool /*filled*/, const AW::Rectangle& rect, AW_bitset filteri) {
        return generic_box(gc, false, rect, filteri);
    }
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
        return generic_circle(gc, filled, center, radius, filteri);
    }
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) {
        return generic_arc(gc, filled, center, radius, start_degrees, arc_degrees, filteri);
    }
public:
    AW_simple_device(AW_common *common_) : AW_device(common_) {}
};

class AW_size_tracker {
    bool     drawn;
    AW_world size;

    void extend(AW_pos& low, AW_pos val, AW_pos& high) {
        low = std::min(low, val);
        high = std::max(high, val);
    }
public:
    AW_size_tracker() { clear(); }

    void clear() { drawn = false; size.clear(); }
    void track(const AW::Position& pos) {
        if (drawn) {
            extend(size.l, pos.xpos(), size.r);
            extend(size.t, pos.ypos(), size.b);
        }
        else {
            size.l = size.r = pos.xpos();
            size.t = size.b = pos.ypos();
            drawn  = true;
        }
    }

    bool was_drawn() const { return drawn; }
    const AW_world& get_size() const { return size; }
    AW::Rectangle get_size_as_Rectangle() const {
        return AW::Rectangle(AW::Position(size.l, size.t), AW::Position(size.r, size.b));
    }
};

class AW_device_size : public AW_simple_device {
    AW_size_tracker scaled;   // all zoomable parts (e.g. tree skeleton)
    AW_size_tracker unscaled; // all unzoomable parts (e.g. text at tree-tips or group-brackets)

    void dot_transformed(const AW::Position& pos, AW_bitset filteri);
    void dot_transformed(AW_pos X, AW_pos Y, AW_bitset filteri) { dot_transformed(AW::Position(X, Y), filteri); }

    void dot(const AW::Position& p, AW_bitset filteri) { dot_transformed(transform(p), filteri); }
    void dot(AW_pos x, AW_pos y, AW_bitset filteri) { dot(AW::Position(x, y), filteri); }

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri);

    void specific_reset();
    
public:
    AW_device_size(AW_common *common_) : AW_simple_device(common_) {}

    void clear();

    AW_DEVICE_TYPE type();

    // all get_size_information...() return screen coordinates

    void get_size_information(AW_world *ptr) const __ATTR__DEPRECATED_TODO("whole AW_world is deprecated") {
        *ptr = scaled.get_size();
    }
    AW::Rectangle get_size_information() const { return scaled.get_size_as_Rectangle(); }
    AW::Rectangle get_size_information_unscaled() const { return unscaled.get_size_as_Rectangle(); }
    AW::Rectangle get_size_information_inclusive_text() const {
        return get_size_information().bounding_box(get_size_information_unscaled());
    }

    AW_borders get_unscaleable_overlap() const;
};

#define AWT_CATCH    30         // max-pixel distance to graphical element (to accept a click or command)
#define AWT_NO_CATCH -1

class AW_device_click : public AW_simple_device {
    AW_pos mouse_x, mouse_y; // @@@ is 'int' sufficant ? 
    int    max_distance_line;
    int    max_distance_text;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) { return generic_invisible(pos, filteri); }

    void specific_reset() {}
    
public:
    AW_clicked_line opt_line; // @@@ make private
    AW_clicked_text opt_text;

    AW_device_click(AW_common *common_)
        : AW_simple_device(common_)
        {
            init_click(0, 0, AWT_NO_CATCH, AW_ALL_DEVICES);
        }

    AW_DEVICE_TYPE type();

    void init_click(AW_pos mousex, AW_pos mousey, int max_distance, AW_bitset filteri);

    void get_clicked_line(class AW_clicked_line *ptr) const; // @@@ make real accessors returning const&
    void get_clicked_text(class AW_clicked_text *ptr) const;
};


#else
#error aw_device.hxx included twice
#endif
