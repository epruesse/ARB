/* 
 * File:   AW_device_print.cxx
 * Author: aboeckma
 * 
 * Created on November 13, 2012, 12:21 PM
 */

#include "aw_device_print.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "arb_msg.h"
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <cairo-script.h>

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

cairo_t *AW_device_print::get_cr(int gc) {
    get_common()->update_cr(prvt->cr, gc);
    return prvt->cr;
}

void AW_device_print::close() {
    cairo_destroy(prvt->cr);
    cairo_surface_destroy(prvt->surface);
}

GB_ERROR AW_device_print::open(const char* path) {
    AW_screen_area cliprect = get_cliprect();
    const char* ext = strrchr(path, '.');
    if (!ext) ext = "";

    if (!strcasecmp(ext, ".svg")) {
        prvt->surface = cairo_svg_surface_create(path, cliprect.r, cliprect.b);
    } else if (!strcasecmp(ext, ".pdf")) {
        prvt->surface = cairo_pdf_surface_create(path, cliprect.r, cliprect.b);
    } else if (!strcasecmp(ext, ".cairo-script")) {
        cairo_device_t *script_dev = cairo_script_create(path);
        cairo_script_set_mode(script_dev,  CAIRO_SCRIPT_MODE_ASCII);
        prvt->surface = cairo_script_surface_create(script_dev, CAIRO_CONTENT_COLOR,
                                                    cliprect.r, cliprect.b);
    } else {
        return "unrecognized file extension. Supported types are SVG and PDF.";
    }

    cairo_status_t state = cairo_surface_status(prvt->surface);
    if (state != CAIRO_STATUS_SUCCESS) {
        return GB_export_errorf("failed to print to file\n"
                                "filename: %s\n"
                                "error code: %s\n"
                                "Does the path exist? Do you have permission to write?",
                                path, cairo_status_to_string(state));
    }

    prvt->cr = cairo_create(prvt->surface);
    clear(AW_ALL_DEVICES);

    return NULL; // no error
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

