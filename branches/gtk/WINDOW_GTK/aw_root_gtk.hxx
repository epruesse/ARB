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
class AW_root;


/**
 * This class contains all gtk dependent attributes of aw_root.
 */
class AW_root_gtk {
private:
    friend class AW_root;

    GdkColormap* colormap; /**< Contains a color for each value in AW_base::AW_color_idx */

};

