// =============================================================== //
//                                                                 //
//   File      : ed4_secedit.hxx                                   //
//   Purpose   : start secondary editor                            //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_SECEDIT_HXX
#define ED4_SECEDIT_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL);

#else
#error ed4_secedit.hxx included twice
#endif // ED4_SECEDIT_HXX
