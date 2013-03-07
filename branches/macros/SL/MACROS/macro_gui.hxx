// ============================================================= //
//                                                               //
//   File      : macro_gui.hxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2005     //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MACRO_GUI_HXX
#define MACRO_GUI_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


class AW_window;
class AW_root;

void awt_popup_macro_window(AW_window *aww, const char *application_id, GBDATA *gb_main);
void awt_execute_macro(GBDATA *gb_main, AW_root *root, const char *macroname);

#else
#error macro_gui.hxx included twice
#endif // MACRO_GUI_HXX
