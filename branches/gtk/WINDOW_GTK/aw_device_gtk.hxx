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
class AW_device_gtk : public AW_device {

    /**
     * The drawing surface.
     * Everything is drawn onto this pixmap.
     * The pixmap is used as a background image in widgets.
     */
    GdkPixmap* pixmap;
    GtkWidget* drawingArea;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri);
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen);
    bool box_impl(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri);
    bool circle_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, AW_bitset filteri);
    bool arc_impl(int gc, bool filled, const AW::Position& center, const AW::Vector& radius, int start_degrees, int arc_degrees, AW_bitset filter);
    bool filled_area_impl(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
        return generic_filled_area(gc, npos, pos, filteri);
    }
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) { return generic_invisible(pos, filteri); }

    void specific_reset() {}

public:
    /**
     * @param drawingArea the device draws onto the background of this widget.
     */
    AW_device_gtk(AW_common *commoni, GtkWidget* drawingArea);


    AW_common_gtk *get_common() const { return DOWNCAST(AW_common_gtk*, AW_device::get_common()); }

    AW_DEVICE_TYPE type();

    void clear(AW_bitset filteri);
    void clear_part(const AW::Rectangle& rect, AW_bitset filteri);
    void clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2);

    void flush();
    void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y);

    /**
     * This is a helper callback which is used inside text_imlp().
     * In the old C code it was just a global function.
     * It is now static because it needs access to private members of aw_device_gtk
     */
    static bool draw_string_on_screen(AW_device *device, int gc, const  char *str, size_t /* opt_str_len */, size_t start, size_t size,
                                        AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL /*cduser*/);

};
