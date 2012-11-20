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


AW_at::AW_at() {
    memset((char*)this, 0, sizeof(AW_at));

    length_of_buttons = 10;
    height_of_buttons = 0;
    shadow_thickness  = 2;
    widget_mask       = AWM_ALL;
    
    FIXME("Not sure if members initialized correctly");
    shadow_thickness = 0;
    length_of_buttons = 0;
    height_of_buttons = 0;
    length_of_label_for_inputfield = 0;
    highlight = 0;

    helptext_for_next_button = NULL;
    AW_active  widget_mask = 0; 
    background_color = 0;
    label_for_inputfield = NULL;
    x_for_next_button = 0;
    y_for_next_button = 0;
    max_x_size = 0;
    max_y_size = 0;

    to_position_x = 0;
    to_position_y = 0;
    to_position_exists = false;

    do_auto_space = false;
    auto_space_x = 0;
    auto_space_y = 0;

    do_auto_increment = false;
    auto_increment_x = 0;
    auto_increment_y = 0;

    biggest_height_of_buttons = 0;

    saved_xoff_for_label = 0;

    saved_x = 0;
    correct_for_at_center = 0;
    x_for_newline = 0;

    attach_x = false;           // attach right side to right form
    attach_y = false;
    attach_lx = false;          // attach left side to right form
    attach_ly = false;
    attach_any = false;

    
    
}
