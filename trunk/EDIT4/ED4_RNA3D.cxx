#include <stdio.h>

#include <aw_root.hxx>
#include <arbdb.h>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>

#include "../RNA3D/RNA3D_Main.hxx"
#include "ed4_RNA3D.hxx"

void ED4_RNA3D_Start(AW_window *aw, AW_CL, AW_CL)
{    
#if defined(ARB_OPENGL)
    RNA3D_StartApplication(aw->get_root());
#endif // ARB_OPENGL
}

