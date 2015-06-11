#pragma once

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

#include "aw_stylable.hxx"
#include "aw_clipable.hxx"
#include "aw_zoomable.hxx"

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

    AW_NUM_FONTS      = 63,
    AW_NUM_FONTS_XFIG = 35, // immutable

    AW_DEFAULT_NORMAL_FONT = AW_LUCIDA_SANS,
    AW_DEFAULT_BOLD_FONT   = AW_LUCIDA_SANS_BOLD,
    AW_DEFAULT_FIXED_FONT  = AW_LUCIDA_SANS_TYPEWRITER,
};

//enum AW_cursor_type {
//    AW_cursor_insert,
//    AW_cursor_overwrite
//};

class AW_device;
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

class AW_clip_scale_stack;
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

    void reset(); // pops all clip_scales

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
    virtual bool box_impl(int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri)                              = 0;
    virtual bool polygon_impl(int gc, AW::FillStyle filled, int npos, const AW::Position *pos, AW_bitset filteri)                  = 0;

    virtual bool circle_impl(int gc, AW::FillStyle filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri)                                  = 0;
    virtual bool arc_impl(int gc, AW::FillStyle filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri) = 0;

    virtual bool invisible_impl(const AW::Position& pos, AW_bitset filteri) = 0;

    virtual void specific_reset() = 0;

protected:

    // * second level functions
    // generic implementations which may be used by primary functions of derived classes 
    
    bool generic_box(int gc, const AW::Rectangle& rect, AW_bitset filteri);
    bool generic_circle(int gc, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) {
        return generic_box(gc, AW::Rectangle(center-radius, center+radius), filteri);
    }
    bool generic_polygon(int gc, int npos, const AW::Position *pos, AW_bitset filteri);
    bool generic_invisible(const AW::Position& pos, AW_bitset filteri);

public:
    // * third level functions (never virtual/overloaded by derived classes)

    bool line(int gc, const AW::LineVector& Line, 
              AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return line_impl(gc, Line, filteri);
    }
    bool line(int gc, const AW::Position& pos1, const AW::Position& pos2, 
              AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return line_impl(gc, AW::LineVector(pos1, pos2), filteri);
    }
    bool line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, 
              AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return line_impl(gc, AW::LineVector(x0, y0, x1, y1), filteri);
    }

    bool text(int         gc,
              const char *string,
              AW_pos      x,
              AW_pos      y,
              AW_pos      alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
              AW_bitset   filteri    = AW_ALL_DEVICES_UNSCALED,
              long        opt_strlen = 0) {
        if (!(filteri & filter)) return false;
        return text_impl(gc, string, AW::Position(x, y), alignment, filteri, opt_strlen);
    }
    bool text(int                  gc,
              const char          *string,
              const AW::Position&  pos,
              AW_pos               alignment  = 0.0, // 0.0 alignment left 0.5 centered 1.0 right justified
              AW_bitset            filteri    = AW_ALL_DEVICES_UNSCALED,
              long                 opt_strlen = 0) {
        if (!(filteri & filter)) return false;
        return text_impl(gc, string, pos, alignment, filteri, opt_strlen);
    }

    bool invisible(const AW::Position& pos, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return invisible_impl(pos, filteri);
    }

    bool box(int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return box_impl(gc, filled, rect, filteri);
    }
    bool box(int gc, AW::FillStyle filled, const AW::Position& pos, const AW::Vector& size, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return box_impl(gc, filled, AW::Rectangle(pos, size), filteri);
    }
    bool box(int gc, AW::FillStyle filled, AW_pos x0, AW_pos y0, AW_pos width, AW_pos height, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return box_impl(gc, filled, AW::Rectangle(AW::Position(x0, y0), AW::Vector(width, height)), filteri);
    }

    bool circle(int gc, AW::FillStyle filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return circle_impl(gc, filled, center, radius, filteri);
    }
    bool circle(int gc, AW::FillStyle filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        if (!(filteri & filter)) return false;
        return circle_impl(gc, filled, AW::Position(x0, y0), AW::Vector(xradius, yradius), filteri);
    }
    bool circle(int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        // draw ellipse into Rectangle
        if (!(filteri & filter)) return false;
        return circle_impl(gc, filled, rect.centroid(), AW::Vector(rect.width()/2, rect.height()/2), filteri);
    }

    // draw arcs (Note: passed degrees are nagative compared to unit circle!)
    bool arc(int gc, AW::FillStyle filled, AW_pos x0, AW_pos y0, AW_pos xradius, AW_pos yradius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        if (!(filteri & filter)) return false;
        return arc_impl(gc, filled, AW::Position(x0, y0), AW::Vector(xradius, yradius), start_degrees, arc_degrees, filteri);
    }
    bool arc(int gc, AW::FillStyle filled, const AW::Position& pos, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filteri = AW_ALL_DEVICES_SCALED) {
        if (!(filteri & filter)) return false;
        return arc_impl(gc, filled, pos, radius, start_degrees, arc_degrees, filteri);
    }

    bool polygon(int gc, AW::FillStyle filled, int npoints, const AW_pos *points, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        if (!(filteri & filter)) return false;
        AW::Position *pos = new AW::Position[npoints];
        for (int n = 0; n<npoints; ++n) {
            pos[n].setx(points[n*2]);
            pos[n].sety(points[n*2+1]);
        }
        bool result = polygon_impl(gc, filled, npoints, pos, filteri);
        delete [] pos;
        return result;
    }
    bool polygon(int gc, AW::FillStyle filled, int npos, const AW::Position *pos, AW_bitset filteri = AW_ALL_DEVICES_SCALED)  {
        if (!(filteri & filter)) return false;
        return polygon_impl(gc, filled, npos, pos, filteri);
    }

    // reduces any string (or virtual string) to its actual drawn size and calls the function f with the result
    bool text_overlay(int gc, const char *opt_string, long opt_strlen,   // either string or strlen != 0
                      const AW::Position& pos, AW_pos alignment, AW_bitset filteri, AW_CL cduser, 
                      AW_pos opt_ascent, AW_pos opt_descent,  // optional height (if == 0 take font height)
                      TextOverlayCallback toc);


    // ********* X11 Device only ********
    virtual void queue_draw() {}
    virtual void queue_draw(const AW_screen_area&) {}

    virtual void clear(AW_bitset filteri);
    virtual void clear_part(const AW::Rectangle& rect, AW_bitset filteri);

    void clear_part(AW_pos x, AW_pos y, AW_pos width, AW_pos height, AW_bitset filteri) {
        if (!(filteri & filter)) return;
        clear_part(AW::Rectangle(AW::Position(x, y), AW::Vector(width, height)), filteri);
    }

    virtual void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y);

    void flush() {};
};


inline void AW_click_cd::link() {
    previous            = my_device->click_cd;
    my_device->click_cd = this;
}
inline AW_click_cd::~AW_click_cd() { my_device->click_cd = previous; }
inline void AW_click_cd::disable() { my_device->click_cd = NULL; }
inline void AW_click_cd::enable() { my_device->click_cd = this; }



class AW_size_tracker {
    bool     drawn;
    AW_world size;

    void extend(AW_pos& low, AW_pos val, AW_pos& high) {
        low = std::min(low, val);
        high = std::max(high, val);
    }
public:
    AW_size_tracker() { restart(); }

    void restart() { drawn = false; size.clear(); }
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

