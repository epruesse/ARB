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
#include "aw_base.hxx"
#include "aw_common.hxx"
#include "aw_position.hxx"

bool AW_getBestClick(AW_clicked_line */*cl*/, AW_clicked_text */*ct*/, AW_CL */*cd1*/, AW_CL */*cd2*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}


const AW_screen_area& AW_device::get_area_size() const {
    return get_common()->get_screen();
}


void AW_device::pop_clip_scale() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::push_clip_scale() {
    GTK_NOT_IMPLEMENTED;
}

bool AW_device::text_overlay(int /*gc*/, const char */*opt_string*/, long /*opt_strlen*/,   // either string or strlen != 0
                      const AW::Position& /*pos*/, AW_pos /*alignment*/, AW_bitset /*filteri*/, AW_CL /*cduser*/,
                      AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/,  // optional height (if == 0 take font height)
                      TextOverlayCallback /*toc*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}



bool AW_device::generic_filled_area(int gc, int npos, const AW::Position *pos, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::move_region(AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::fast() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::slow() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::flush() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::reset() {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::clear(AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_device::generic_invisible(const AW::Position& pos, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
    return false;
}
const AW_screen_area&  AW_device::get_common_screen(const AW_common *common_) {
    GTK_NOT_IMPLEMENTED;
}


bool AW_device::generic_box(int gc, bool filled, const AW::Rectangle& rect, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

void AW_device::clear_part(const AW::Rectangle& rect, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
}


void AW_device::set_filter(AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_device_click::get_clicked_line(class AW_clicked_line */*ptr*/) const {
    GTK_NOT_IMPLEMENTED;
}

void AW_device_click::get_clicked_text(AW_clicked_text */*ptr*/) const {
    GTK_NOT_IMPLEMENTED;
}


FILE *AW_device_print::get_FILE() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}




void AW_device_print::close() {
    GTK_NOT_IMPLEMENTED;
}

GB_ERROR AW_device_print::open(char const* /*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_device_print::set_color_mode(bool /*mode*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_borders AW_device_size::get_unscaleable_overlap() const {
    GTK_NOT_IMPLEMENTED;
    return AW_borders();
}

void AW_device_size::specific_reset() {
    GTK_NOT_IMPLEMENTED;
}

AW_DEVICE_TYPE AW_device_size::type() {
    GTK_NOT_IMPLEMENTED;
    return (AW_DEVICE_TYPE)0;
}

bool AW_device_size::line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_size::text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_size::invisible_impl(const AW::Position& pos, AW_bitset filteri) {
    GTK_NOT_IMPLEMENTED;
    return false;
}


void specific_reset() {
    GTK_NOT_IMPLEMENTED;
}






//aw_styleable
//FIXME move to own class

const AW_font_limits& AW_stylable::get_font_limits(int /*gc*/, char /*c*/) const {
    GTK_NOT_IMPLEMENTED;
}

int AW_stylable::get_string_size(int /*gc*/, const  char */*string*/, long /*textlen*/) const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
void AW_stylable::new_gc(int gc) {
    get_common()->new_gc(gc);
}
void AW_stylable::set_function(int gc, AW_function function) {
    GTK_NOT_IMPLEMENTED;
}


void AW_stylable::reset_style() {
    GTK_NOT_IMPLEMENTED;
}

void AW_stylable::set_font(int /*gc*/, AW_font /*fontnr*/, int /*size*/, int */*found_size*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_stylable::set_foreground_color(int gc, AW_color_idx color) {
    get_common()->map_mod_gc(gc)->set_fg_color(get_common()->get_color(color));
}

void AW_stylable::set_grey_level(int /*gc*/, AW_grey_level /*grey_level*/) {
    GTK_NOT_IMPLEMENTED;
}
void AW_stylable::set_line_attributes(int gc, short width, AW_linestyle style) {
    get_common()->map_mod_gc(gc)->set_line_attributes(width, style);
}


//aw_clipable
//FIXME move to own file
bool AW_clipable::clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out) {
    // clip coordinates of a line

    int    outcodeout;
    AW_pos x = 0;
    AW_pos y = 0;

    bool is_visible = false;    // indicates whether part of line is visible
    bool done       = false;    // true soon as line is completely inside or outside rectangle

    while (!done) {
        int outcode0 = compoutcode(x0, y0);
        int outcode1 = compoutcode(x1, y1);

        if ((outcode0 | outcode1) == 0) { // line is inside the rectangle
            x0out = x0; y0out = y0; // clipped coordinates of line
            x1out = x1; y1out = y1;

            done    = true;
            is_visible = true;
        }
        else if ((outcode0 & outcode1) != 0) { // line is outside the rectangle
            done = true;
        }
        else { // line overlaps with at least one rectangle border
            outcodeout = outcode0>0 ? outcode0 : outcode1;

            if ((outcodeout & 8) != 0) { // overlap at top
                x = x0+(x1-x0)*(clip_rect.t-y0)/(y1-y0);
                y = clip_rect.t;
            }
            else if ((outcodeout & 4) != 0) { // overlap at bottom
                x = x0+(x1-x0)*(clip_rect.b-y0)/(y1-y0);
                y = clip_rect.b;
            }
            else if ((outcodeout & 2) != 0) { // overlap at right side
                y = y0+(y1-y0)*(clip_rect.r-x0)/(x1-x0);
                x = clip_rect.r;
            }
            else if ((outcodeout & 1) != 0) {
                y = y0+(y1-y0)*(clip_rect.l-x0)/(x1-x0); // overlap at left side
                x = clip_rect.l;
            }

            // set corrected point and iterate :
            if (outcode0 > 0) {
                x0 = x;
                y0 = y;
            }
            else {
                x1 = x;
                y1 = y;
            }
        }
    }

    return is_visible;
}

bool AW_clipable::clip(const AW::LineVector& line, AW::LineVector& clippedLine) {
    AW_pos x0, y0, x1, y1;
    bool   drawflag = clip(line.start().xpos(), line.start().ypos(), line.head().xpos(), line.head().ypos(),
                           x0, y0, x1, y1);
    if (drawflag) clippedLine = AW::LineVector(x0, y0, x1, y1);
    return drawflag;
}
void AW_clipable::set_bottom_clip_border(int /*bottom*/, bool /*allow_oversize*/ /*= false*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_clipable::set_left_clip_border(int /*left*/, bool /*allow_oversize*/ /* = false*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_clipable::set_right_clip_border(int /*right*/, bool /*allow_oversize*/ /* = false*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_clipable::set_top_clip_border(int /*top*/, bool /*allow_oversize*/ /* = false*/) {
    GTK_NOT_IMPLEMENTED;
}


int AW_clipable::reduceClipBorders(int /*top*/, int /*bottom*/, int /*left*/, int /*right*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}



void AW_zoomable::reset() {
    unscale = scale   = 1.0;
    offset  = AW::Vector(0, 0);
}

void AW_zoomable::zoom(AW_pos /*scale*/) {
    GTK_NOT_IMPLEMENTED;
}



