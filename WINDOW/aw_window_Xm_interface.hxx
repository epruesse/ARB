// ============================================================= //
//                                                               //
//   File      : aw_window_Xm_interface.hxx                      //
//   Purpose   : Provide some X11 interna for applications       //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2009   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AW_WINDOW_XM_INTERFACE_HXX
#define AW_WINDOW_XM_INTERFACE_HXX

#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif
#ifndef _Xm_h
#include <Xm/Xm.h>
#endif

XtAppContext AW_get_XtAppContext(AW_root *aw_root);
Widget       AW_get_AreaWidget(AW_window *aww, AW_area area);
GC           AW_map_AreaGC(AW_window *aww, AW_area area, int gc);


#else
#error aw_window_Xm_interface.hxx included twice
#endif // AW_WINDOW_XM_INTERFACE_HXX
