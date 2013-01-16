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
    
    shadow_thickness = 0;
    length_of_buttons = 0;
    height_of_buttons = 0;
    length_of_label_for_inputfield = 0;
    highlight = 0;

    helptext_for_next_button = NULL;
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

void AW_at::at(int x, int y){
    at_x(x);
    at_y(y);
}

void AW_at::at_x(int x){
    if (x_for_next_button > max_x_size) max_x_size = x_for_next_button;
    x_for_next_button = x;
    if (x_for_next_button > max_x_size) max_x_size = x_for_next_button;
}

void AW_at::at_y(int y){
    if (y_for_next_button + biggest_height_of_buttons > max_y_size)
        max_y_size = y_for_next_button + biggest_height_of_buttons;
    biggest_height_of_buttons = biggest_height_of_buttons + y_for_next_button - y;
    if (biggest_height_of_buttons<0) {
        biggest_height_of_buttons = 0;
        if (max_y_size < y)    max_y_size = y;
    }
    y_for_next_button = y;
}

void AW_at::at_shift(int x, int y){
    at(x + x_for_next_button, y + y_for_next_button);
}

void AW_at::at_newline(){
  if (do_auto_increment) {
        at_y(auto_increment_y + y_for_next_button);
    }
    else {
        if (do_auto_space) {
            at_y(y_for_next_button + auto_space_y + biggest_height_of_buttons);
        }
        else {
            GBK_terminate("neither auto_space nor auto_increment activated while using at_newline");
        }
    }
    at_x(x_for_newline);
}

bool AW_at::at_ifdef(const char *id) {
  GTK_NOT_IMPLEMENTED;
  return false;
}

void AW_at::at_set_to(bool attach_x_i, bool attach_y_i, int xoff, int yoff) {
    attach_any = attach_x_i || attach_y_i;
    attach_x   = attach_x_i;
    attach_y   = attach_y_i;

    to_position_exists = true;
    to_position_x      = xoff >= 0 ? x_for_next_button + xoff : max_x_size+xoff;
    to_position_y      = yoff >= 0 ? y_for_next_button + yoff : max_y_size+yoff;

    if (to_position_x > max_x_size) max_x_size = to_position_x;
    if (to_position_y > max_y_size) max_y_size = to_position_y;

    correct_for_at_center = 0;
}
void AW_at::unset_at_commands() {
    //prvt->callback   = NULL;
    //_d_callback = NULL;

    correct_for_at_center = 0;
    to_position_exists    = false;
    highlight             = false;

    freenull(helptext_for_next_button);
    freenull(label_for_inputfield);

    background_color = 0;
}

void AW_at::at_unset_to(){
    attach_x   = attach_y = to_position_exists = false;
    attach_any = attach_lx || attach_ly;
}

void AW_at::at_set_min_size(int xmin, int ymin){
    if (xmin > max_x_size) max_x_size = xmin; // this looks wrong, but its right!
    if (ymin > max_y_size) max_y_size = ymin;

    /*
    if (recalc_size_at_show != AW_KEEP_SIZE) {
        set_window_size(max_x_size+1000, max_y_size+1000);
    }
    */
}

void AW_at::auto_space(int x, int y){
    do_auto_space = true;
    auto_space_x  = x;
    auto_space_y  = y;
    
    do_auto_increment = false;
    
    x_for_newline = x_for_next_button;
    biggest_height_of_buttons = 0;
}

void AW_at::label_length(int length){
    length_of_label_for_inputfield = length;
}

void AW_at::button_length(int length) {
    length_of_buttons = length; 
}

int  AW_at::get_button_length() const {
    return length_of_buttons; 
}
void AW_at::get_at_position(int *x, int *y) const {
    *x = x_for_next_button; *y = y_for_next_button; 
}

int AW_at::get_at_xposition() const {
    return x_for_next_button; 
}

int AW_at::get_at_yposition() const {
    return y_for_next_button; 
}
