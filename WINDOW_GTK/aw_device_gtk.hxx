// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.hxx                               //
//   Purpose   : A device to print on a gtk widget               //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#pragma once

#include "aw_common_gtk.hxx"

/**
 * A device to draw/print on a GtkWidget.
 */
class AW_device_gtk : public AW_device, virtual Noncopyable {

    GtkWidget* drawingArea;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE;
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri) OVERRIDE;
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filter) OVERRIDE;
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) OVERRIDE {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) OVERRIDE { return generic_invisible(pos, filteri); }

    void specific_reset() OVERRIDE {}

public:
    /**
     * @param drawingArea the device draws onto the background of this widget.
     */
    AW_device_gtk(AW_common *commoni, GtkWidget* drawingArea);


    AW_common_gtk *get_common() const { return DOWNCAST(AW_common_gtk*, AW_device::get_common()); }

    AW_DEVICE_TYPE type() OVERRIDE;

    void clear(AW_bitset filteri) OVERRIDE;
    void clear_part(const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE;

    void flush() OVERRIDE;
    void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) OVERRIDE;

    /**
     * This is a helper callback which is used inside text_imlp().
     * In the old C code it was just a global function.
     * It is now static because it needs access to private members of aw_device_gtk
     */
    static bool draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                                        AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/);

};
