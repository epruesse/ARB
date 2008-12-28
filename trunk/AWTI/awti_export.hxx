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

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define awti_assert(cond) arb_assert(cond)

AW_window *open_AWTC_export_window(AW_root *awr,GBDATA *gb_main);

#else
#error awti_export.hxx included twice
#endif // AWTI_EXPORT_HXX
