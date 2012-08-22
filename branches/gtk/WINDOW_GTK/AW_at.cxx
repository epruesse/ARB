// ============================================================= //
//                                                               //
//   File      : AW_at.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 22, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_at.hxx"
#include "aw_window.hxx"

//FIXME init all members
AW_at::AW_at() {
    memset((char*)this, 0, sizeof(AW_at));

    length_of_buttons = 10;
    height_of_buttons = 0;
    shadow_thickness  = 2;
    widget_mask       = AWM_ALL;
}
