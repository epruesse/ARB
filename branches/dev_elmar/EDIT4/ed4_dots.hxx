// ================================================================ //
//                                                                  //
//   File      : ed4_dots.hxx                                       //
//   Purpose   : Insert dots where bases may be missing             //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2008   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ED4_DOTS_HXX
#define ED4_DOTS_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

void ED4_create_dot_missing_bases_awars(AW_root *aw_root, AW_default aw_def);
void ED4_popup_dot_missing_bases_window(AW_window *editor_window, AW_CL, AW_CL);

#else
#error ed4_dots.hxx included twice
#endif // ED4_DOTS_HXX
