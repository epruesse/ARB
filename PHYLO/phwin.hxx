#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif


class PH_used_windows {
public:
        PH_used_windows(void);                  // constructor
        static PH_used_windows  *windowList;    // List of all global needed windows and items
        AW_window *phylo_main_window;           // control window

};

void display_status(AW_window *dummy,AW_CL awroot,AW_CL cd2);
void expose_callb(AW_window *aw,AW_CL cd1,AW_CL cd2) ;
