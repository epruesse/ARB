#include <stdio.h>

#include <aw_root.hxx>
#include <arbdb.h>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_canvas.hxx>

#include <RNA3D_main.hxx>

void ED4_RNA3D_start(AW_window *aw, AW_CL, AW_CL)
{
    static AW_window *aw_RNA3D = 0;

    if (!aw_RNA3D) { // do not open window twice
        aw_RNA3D = createRNA3D_Window(aw->get_root());
        if (!aw_RNA3D) {
            GB_ERROR err = GB_get_error();
            aw_message(GBS_global_string("Couldn't start 3D structure of ribosomal RNA.\nReason: %s", err));
            return;
        }
    }
    aw_RNA3D->show();
}

