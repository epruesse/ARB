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

class AW_window;
class AW_root;

void insert_macro_menu_entry(AW_window *awm, bool prepend_separator);
void awt_execute_macro(AW_root *root, const char *macroname);

#else
#error macro_gui.hxx included twice
#endif // MACRO_GUI_HXX
