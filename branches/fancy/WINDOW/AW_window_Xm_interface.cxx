// ============================================================= //
//                                                               //
//   File      : AW_window_Xm_interface.cxx                      //
//   Purpose   : Provide some X11 interna for applications       //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2009   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_window_Xm_interface.hxx"
#include <aw_window.hxx>
#include <aw_window_Xm.hxx>
#include "aw_common_xm.hxx"
#include <aw_root.hxx>

XtAppContext AW_get_XtAppContext(AW_root *aw_root) {
    return aw_root->prvt->context;
}

Widget AW_get_AreaWidget(AW_window *aww, AW_area area) {
    return aww->p_w->areas[area]->get_area();
}

GC AW_map_AreaGC(AW_window *aww, AW_area area, int gc) {
    return aww->p_w->areas[area]->get_common()->get_GC(gc);
}

