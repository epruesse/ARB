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
#include <aw_commn.hxx>

XtAppContext AW_get_XtAppContext(AW_root *aw_root) {
    return aw_root->prvt->context;
}

Widget AW_get_AreaWidget(AW_window *aww, AW_area area) {
    return aww->p_w->areas[area]->area;
}

GC AW_map_AreaGC(AW_window *aww, AW_area area, int gc) {
    AW_common *common = aww->p_w->areas[ area ]->common;
    AW_GC_Xm  *gcm    = AW_MAP_GC( gc );

    return gcm->gc;
}

