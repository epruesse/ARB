/* 
 * File:   AW_device_print.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:21 PM
 */

#include "aw_device_print.hxx"
#include "aw_gtk_migration_helpers.hxx"

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