// =============================================================== //
//                                                                 //
//   File      : awti_export.hxx                                   //
//   Purpose   : Interface to export window                        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2008      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWTI_EXPORT_HXX
#define AWTI_EXPORT_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class AW_window;
class AW_root;

AW_window *create_AWTC_export_window(AW_root *awr, GBDATA *gb_main);

#else
#error awti_export.hxx included twice
#endif // AWTI_EXPORT_HXX
