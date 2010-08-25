// =============================================================== //
//                                                                 //
//   File      : ed4_RNA3D.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_RNA3D_HXX
#define ED4_RNA3D_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

void ED4_RNA3D_Start(AW_window *aw, AW_CL cl_gb_main, AW_CL);

#else
#error ed4_RNA3D.hxx included twice
#endif // ED4_RNA3D_HXX
