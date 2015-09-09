// ============================================================ //
//                                                              //
//   File      : awt_misc.hxx                                   //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AWT_MISC_HXX
#define AWT_MISC_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

AW_window *AWT_create_IUPAC_info_window(AW_root *aw_root);

#else
#error awt_misc.hxx included twice
#endif // AWT_MISC_HXX
