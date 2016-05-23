// ============================================================= //
//                                                               //
//   File      : aw_device_gtk.hxx                               //
//   Purpose   : A device to print on a gtk widget               //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de Aug 2012     //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#pragma once

#include "aw_common_impl.hxx"
#include "aw_device_cairo.hxx"

/**
 * A device to draw/print on a GtkWidget.
 */
class AW_device_gtk : public AW_device_cairo, virtual Noncopyable {
private:
    struct Pimpl;
    Pimpl *prvt;

protected:
    cairo_t *get_cr(int gc) OVERRIDE;
public:
    /**
     * @param drawingArea the device draws onto the background of this widget.
     */
    AW_device_gtk(AW_common *commoni, GtkWidget* drawingArea);
    ~AW_device_gtk();

    AW_DEVICE_TYPE type() OVERRIDE;

    void queue_draw() OVERRIDE;
    void queue_draw(const AW_screen_area&) OVERRIDE;

    void set_cr(cairo_t*);
};
