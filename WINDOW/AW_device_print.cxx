/* 
 * File:   AW_device_print.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:21 PM
 */

#include "aw_device_print.hxx"
#include "aw_gtk_migration_helpers.hxx"

// ---------------------
//      Please note
//
// there is a unit test available that tests AW_device_print
// see ../../SL/TREEDISP/TreeDisplay.cxx@ENABLE_THIS_TEST 


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

bool AW_device_print::line_impl(int /*gc*/, const AW::LineVector& /*Line*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::text_impl(int /*gc*/, const char */*str*/, const AW::Position& /*pos*/, AW_pos /*alignment*/, AW_bitset /*filteri*/, long /*opt_strlen*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::box_impl(int /*gc*/, bool /*filled*/, const AW::Rectangle& /*rect*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::circle_impl(int /*gc*/, bool /*filled*/, const AW::Position& /*center*/, const AW::Vector& /*radius*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::arc_impl(int /*gc*/, bool /*filled*/, const AW::Position& /*center*/, const AW::Vector& /*radius*/, int /*start_degrees*/, int /*arc_degrees*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::filled_area_impl(int /*gc*/, int /*npos*/, const AW::Position */*pos*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

bool AW_device_print::invisible_impl(const AW::Position& /*pos*/, AW_bitset /*filteri*/) {
    GTK_NOT_IMPLEMENTED;
    return false;
}

AW_DEVICE_TYPE AW_device_print::type() {
    return AW_DEVICE_PRINTER;
}

