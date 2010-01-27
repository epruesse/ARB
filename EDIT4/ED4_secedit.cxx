// =============================================================== //
//                                                                 //
//   File      : ED4_secedit.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_secedit.hxx"

#include <secedit_extern.hxx>
#include <arbdb.h>
#include <awt_canvas.hxx>

void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL) {
    static AW_window *aw_sec = 0;

    if (!aw_sec) { // do not open window twice
        aw_sec = SEC_create_main_window(aw->get_root(), (GBDATA*)cl_gbmain);
        if (!aw_sec) {
            GB_ERROR err = GB_await_error();
            aw_message(GBS_global_string("Couldn't start secondary structure editor.\nReason: %s", err));
            return;
        }
    }
    aw_sec->activate();
}

