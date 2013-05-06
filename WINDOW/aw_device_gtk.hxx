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
#include "aw_device_cairo.hxx"

/**
 * A device to draw/print on a GtkWidget.
 */
class AW_device_gtk : public AW_device_cairo, virtual Noncopyable {
private:
    GtkWidget* drawingArea;

protected:
    cairo_t *get_cr(int gc) OVERRIDE;
    PangoLayout *get_pl(int GC) OVERRIDE;

public:
    /**
     * @param drawingArea the device draws onto the background of this widget.
     */
    AW_device_gtk(AW_common *commoni, GtkWidget* drawingArea);

    AW_DEVICE_TYPE type() OVERRIDE;

    void flush() OVERRIDE;

    void move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, 
                     AW_pos dest_x, AW_pos dest_y) OVERRIDE;
};
