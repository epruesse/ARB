#include <stdio.h>

#include <aw_root.hxx>
#include <arbdb.h>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>

#include "../RNA3D/RNA3D_Interface.hxx"
#include "ed4_RNA3D.hxx"

void ED4_RNA3D_Start(AW_window *aw, AW_CL, AW_CL)
{
    static AW_window *aw_3D = 0;

    if (!aw_3D) { // do not open window twice
        aw_3D = createRNA3DMainWindow(aw->get_root());
        if (!aw_3D) {
            GB_ERROR err = GB_get_error();
            aw_message(GBS_global_string("Couldn't start Ribosomal RNA 3D Structure Tool.\nReason: %s", err));
            return;
        }
    }
    aw_3D->show();
}

