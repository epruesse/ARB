#include <stdio.h>

#include <aw_root.hxx>
#include <arbdb.h>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>

#include <secedit_extern.hxx>
#include "ed4_secedit.hxx"

void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL) {
    static AW_window *aw_sec = 0;

    if (!aw_sec) { // do not open window twice
        aw_sec = SEC_create_main_window(aw->get_root(), (GBDATA*)cl_gbmain);
        if (!aw_sec) {
            GB_ERROR err = GB_get_error();
            aw_message(GBS_global_string("Couldn't start secondary structure editor.\nReason: %s", err));
            return;
        }
    }
    aw_sec->show();
}

