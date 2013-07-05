// =========================================================== //
//                                                             //
//   File      : phwin.hxx                                     //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef PHWIN_HXX
#define PHWIN_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

class PH_used_windows {
public:
        PH_used_windows();                      // constructor
        static PH_used_windows  *windowList;    // List of all global needed windows and items
        AW_window *phylo_main_window;           // control window

};

void display_status(AW_window *dummy, AW_CL awroot, AW_CL cd2);
void expose_callb();

#else
#error phwin.hxx included twice
#endif // PHWIN_HXX
