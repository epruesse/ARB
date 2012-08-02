// ============================================================= //
//                                                               //
//   File      : AW_device.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_gtk_migration_helpers.hxx"
#include "aw_device.hxx"


const AW_screen_area& AW_device::get_area_size() const {
    GTK_NOT_IMPLEMENTED
}

void AW_device::reset() {
    GTK_NOT_IMPLEMENTED
}

void AW_device::set_filter(AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED
}

void AW_device_click::get_clicked_line(class AW_clicked_line *ptr) const {
    GTK_NOT_IMPLEMENTED
}

void AW_device_click::get_clicked_text(AW_clicked_text *ptr) const {
    GTK_NOT_IMPLEMENTED
}


FILE *AW_device_print::get_FILE() {
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_device_print::close() {
    GTK_NOT_IMPLEMENTED
}

GB_ERROR AW_device_print::open(char const* path) {
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_device_print::set_color_mode(bool mode) {
    GTK_NOT_IMPLEMENTED
}

AW_borders AW_device_size::get_unscaleable_overlap() const {
    GTK_NOT_IMPLEMENTED
}




//aw_styleable
//FIXME move to own class

const AW_font_limits& AW_stylable::get_font_limits(int gc, char c) const {
    GTK_NOT_IMPLEMENTED
}

int AW_stylable::get_string_size(int gc, const  char *string, long textlen) const {
    GTK_NOT_IMPLEMENTED
    return 0;
}
void AW_stylable::new_gc(int gc) {
    GTK_NOT_IMPLEMENTED
}

void AW_stylable::reset_style() {
    GTK_NOT_IMPLEMENTED
}

void AW_stylable::set_font(int gc, AW_font fontnr, int size, int *found_size) {
    GTK_NOT_IMPLEMENTED
}

void AW_stylable::set_foreground_color(int gc, AW_color_idx color) {
    GTK_NOT_IMPLEMENTED
}

void AW_stylable::set_grey_level(int gc, AW_grey_level grey_level) {
    GTK_NOT_IMPLEMENTED
}
void AW_stylable::set_line_attributes(int gc, short width, AW_linestyle style) {
    GTK_NOT_IMPLEMENTED
}

AW_common *AW_stylable::get_common() const {
    GTK_NOT_IMPLEMENTED
    return 0;
}


//aw_clipable
//FIXME move to own file

bool AW_clipable::clip(const AW::LineVector& line, AW::LineVector& clippedLine) {

    return false;
}
void AW_clipable::set_bottom_clip_border(int bottom, bool allow_oversize /*= false*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_clipable::set_left_clip_border(int left, bool allow_oversize/* = false*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_clipable::set_right_clip_border(int right, bool allow_oversize/* = false*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_clipable::set_top_clip_border(int top, bool allow_oversize/* = false*/) {
    GTK_NOT_IMPLEMENTED
}






void AW_zoomable::zoom(AW_pos scale) {
    GTK_NOT_IMPLEMENTED
}



