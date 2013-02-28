// ==================================================================== //
//                                                                      //
//   File      : awt_macro.hxx                                          //
//   Purpose   :                                                        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AWT_MACRO_HXX
#define AWT_MACRO_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


class AW_window;
class AW_root;

void awt_popup_macro_window(AW_window *aww, const char *application_id, GBDATA *gb_main);
void awt_execute_macro(GBDATA *gb_main, AW_root *root, const char *macroname);

#else
#error awt_macro.hxx included twice
#endif // AWT_MACRO_HXX

