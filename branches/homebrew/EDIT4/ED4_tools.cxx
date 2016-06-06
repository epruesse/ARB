// =============================================================== //
//                                                                 //
//   File      : ED4_tools.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"

void ED4_set_clipping_rectangle(AW_screen_area *rect) {
    // The following can't be replaced by set_cliprect (set_cliprect clears font overlaps):

    current_device()->set_top_clip_border(rect->t);
    current_device()->set_bottom_clip_border(rect->b);
    current_device()->set_left_clip_border(rect->l);
    current_device()->set_right_clip_border(rect->r);
}

