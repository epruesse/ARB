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

struct PH_used_windows {
        PH_used_windows();                      // constructor
        static PH_used_windows  *windowList;    // List of all global needed windows and items
        AW_window *phylo_main_window;           // control window

};

void display_status_cb();
void expose_cb();

#else
#error phwin.hxx included twice
#endif // PHWIN_HXX
