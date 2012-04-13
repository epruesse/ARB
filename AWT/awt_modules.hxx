// ============================================================= //
//                                                               //
//   File      : awt_modules.hxx                                 //
//   Purpose   : small modules for GUI construction              //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AWT_MODULES_HXX
#define AWT_MODULES_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

enum awt_reorder_mode {
    ARM_TOP, 
    ARM_UP, 
    ARM_DOWN, 
    ARM_BOTTOM, 
};

typedef void (*awt_orderfun)(AW_window *aww, awt_reorder_mode pos, AW_CL cl_user);

void awt_create_order_buttons(AW_window *aws, awt_orderfun reorder_cb, AW_CL cl_user);

#else
#error awt_modules.hxx included twice
#endif // AWT_MODULES_HXX
