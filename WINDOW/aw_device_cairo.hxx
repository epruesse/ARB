// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.hxx                               //
//   Purpose   : A device to print on a cairo surface            //
//                                                               //
//   Coded by Elmar Pruesse epruesse@mpi-bremen.de on May 5th 2013 //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#pragma once

#include "aw_common_impl.hxx"

typedef struct _cairo cairo_t;
typedef struct _PangoLayout pango_layout_t;

/**
 * A device to draw on a cairo_surface_t
 */
class AW_device_cairo : public AW_device, virtual Noncopyable {
private:
    bool line_impl       (int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl       (int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool box_impl        (int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE;
    bool circle_impl     (int gc, AW::FillStyle filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) OVERRIDE;
    bool arc_impl        (int gc, AW::FillStyle filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filter) OVERRIDE;
    bool polygon_impl    (int gc, AW::FillStyle filled, int npos, const AW::Position *pos, AW_bitset filteri) OVERRIDE;
    bool invisible_impl  (const AW::Position& pos, AW_bitset filteri) OVERRIDE;

    void specific_reset() OVERRIDE {}

protected:
    virtual cairo_t *get_cr(int gc) = 0;

public:
    AW_device_cairo(AW_common* awc);
    ~AW_device_cairo();

    AW_common_gtk *get_common() const { return DOWNCAST(AW_common_gtk*, AW_device::get_common()); }

    void clear(AW_bitset filteri) OVERRIDE;
    void clear_part(const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE;

    /**
     * This is a helper callback which is used inside text_imlp().
     * In the old C code it was just a global function.
     * It is now static because it needs access to private members of aw_device_gtk
     */
    static bool draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                                        AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/);

    int  get_string_size(int gc, const  char *string, long textlen) const OVERRIDE;

    void move_region(AW_pos, AW_pos, AW_pos, AW_pos, AW_pos, AW_pos) OVERRIDE;
};


