// =============================================================== //
//                                                                 //
//   File      : ED4_RNA3D.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_RNA3D.hxx"
#include "../RNA3D/RNA3D_Main.hxx"
#include <awt_canvas.hxx>

void ED4_RNA3D_Start(AW_window *aw, AW_CL, AW_CL)
{
#if defined(ARB_OPENGL)
    RNA3D_StartApplication(aw->get_root());
#else
    aw = aw; // avoid warning
#endif // ARB_OPENGL
}

