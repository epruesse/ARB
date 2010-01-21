#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "pars_debug.hxx"

void display_out(double * /* liste */, int /* anz */, double /* prev */, double /* pars_start */, int /* rek_deep */) {
    // dummy Funktion zum darstellen
    return;
}

void display_clear(double (* /* funktion */)(double, double *, int), double * /* parm_liste */, int /* param_anz */, int /* pars_start */, int /* max_rek_deep */) {
    // dummy funktion zum display clearen;
    return;
}
