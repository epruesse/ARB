// =============================================================== //
//                                                                 //
//   File      : ED4_RNA3D.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "../RNA3D/RNA3D_Main.hxx"
#include "ed4_RNA3D.hxx"
#include <awt_canvas.hxx>

#if defined(ARB_OPENGL)

void ED4_RNA3D_Start(AW_window *aw, AW_CL cl_gb_main, AW_CL) {
    GBDATA *gb_main = (GBDATA*)cl_gb_main;
    RNA3D_StartApplication(aw->get_root(), gb_main);
}

#else

void ED4_RNA3D_Start(AW_window *, AW_CL, AW_CL) {}

#endif // ARB_OPENGL

