// ================================================================= //
//                                                                   //
//   File      : cb.h                                                //
//   Purpose   : callback                                            //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef CB_H
#define CB_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

typedef void (*AW_RCB)(AW_root*, AW_CL, AW_CL);

#else
#error cb.h included twice
#endif // CB_H
