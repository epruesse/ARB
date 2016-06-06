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
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

enum awt_reorder_mode {
    ARM_TOP, 
    ARM_UP, 
    ARM_DOWN, 
    ARM_BOTTOM, 
};

enum awt_collect_mode {
    ACM_ADD, 
    ACM_FILL, // aka "add all"
    ACM_REMOVE, 
    ACM_EMPTY, 
};

namespace UNTYPED {
    typedef void(*awt_orderfun)  (AW_window *aww, awt_reorder_mode pos,  AW_CL cl_user);
    typedef void(*awt_collectfun)(AW_window *aww, awt_collect_mode what, AW_CL cl_user);

    void awt_create_order_buttons(AW_window *aws, awt_orderfun reorder_cb, AW_CL cl_user);
    void awt_create_collect_buttons(AW_window *aws, bool collect_rightwards, awt_collectfun collect_cb, AW_CL cl_user);
};

template<typename T>
inline void awt_create_order_buttons(AW_window *aws, void (*reorder_cb)(AW_window*, awt_reorder_mode, T), T t) {
    disallow_type<T, AW_CL>::here();
    UNTYPED::awt_create_order_buttons(aws, (UNTYPED::awt_orderfun)reorder_cb, AW_CL(t));
}
inline void awt_create_order_buttons(AW_window *aws, void (*reorder_cb)(AW_window*, awt_reorder_mode)) {
    UNTYPED::awt_create_order_buttons(aws, (UNTYPED::awt_orderfun)reorder_cb, 0);
}


template<typename T>
inline void awt_create_collect_buttons(AW_window *aws, bool collect_rightwards, void (*collect_cb)(AW_window*, awt_collect_mode, T), T t) {
    disallow_type<T, AW_CL>::here();
    UNTYPED::awt_create_collect_buttons(aws, collect_rightwards, (UNTYPED::awt_collectfun)collect_cb, AW_CL(t));
}
inline void awt_create_collect_buttons(AW_window *aws, bool collect_rightwards, void (*collect_cb)(AW_window*, awt_collect_mode)) {
    UNTYPED::awt_create_collect_buttons(aws, collect_rightwards, (UNTYPED::awt_collectfun)collect_cb, 0);
}


#else
#error awt_modules.hxx included twice
#endif // AWT_MODULES_HXX
