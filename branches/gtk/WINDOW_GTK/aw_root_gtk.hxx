// ============================================================= //
//                                                               //
//   File      : aw_root_gtk.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 6, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#pragma once

#include <gdk/gdk.h>
#include "aw_base.hxx"
class AW_root;


/**
 * This class contains all gtk dependent attributes of aw_root.
 */
class AW_root_gtk {
private:
    friend class AW_root;

    GdkColormap* colormap; /**< Contains a color for each value in AW_base::AW_color_idx */
    AW_rgb  *color_table; /**< Contains pixel values that can be used to retrieve a color from the colormap  */
    AW_rgb foreground; /**<Pixel value of the foreground color */
    //FIXME background is currently determined on the fly in device_gtk
    //Pixel background; /**<Pixel value of the background color */

};

