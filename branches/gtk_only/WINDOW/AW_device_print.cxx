/* 
 * File:   AW_device_print.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:21 PM
 */

#include "aw_device_print.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include <cairo-pdf.h>

// ---------------------
//      Please note
//
// there is a unit test available that tests AW_device_print
// see ../../SL/TREEDISP/TreeDisplay.cxx@ENABLE_THIS_TEST 


struct AW_device_print::Pimpl {
    cairo_surface_t *surface;
    cairo_t *cr;
    Pimpl() : surface(NULL) {}
};

AW_device_print::AW_device_print(AW_common *common_)
    : AW_device_cairo(common_),
      prvt(new Pimpl())
{
}
AW_device_print::~AW_device_print() {
    delete prvt;
}

PangoLayout *AW_device_print::get_pl(int gc) {
    PangoLayout *pl = pango_cairo_create_layout(get_cr(gc));
    pango_layout_set_font_description(pl, get_common()->get_font(gc));
    return pl;
}

cairo_t *AW_device_print::get_cr(int gc) {
    get_common()->update_cr(prvt->cr, gc);
    return prvt->cr;
}

void AW_device_print::close() {
    cairo_destroy(prvt->cr);
    cairo_surface_destroy(prvt->surface);
}

GB_ERROR AW_device_print::open(char const* path) {
    AW_screen_area cliprect = get_cliprect();
    prvt->surface = cairo_pdf_surface_create(path, cliprect.r, cliprect.b);
    prvt->cr = cairo_create(prvt->surface);
    clear(AW_ALL_DEVICES);
    return NULL;
}

void AW_device_print::set_color_mode(bool /*mode*/) {
    // we won't do b&W mode here
    // if this is desired, there should be a color profile
    // for the entire tree that has b&w or grey colors
    // selected
}

AW_DEVICE_TYPE AW_device_print::type() {
    return AW_DEVICE_PRINTER;
}

