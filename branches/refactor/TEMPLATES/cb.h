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

class AW_root;

typedef long AW_CL; // client data (casted from pointer or value)

typedef void (*AW_RCB)(AW_root*, AW_CL, AW_CL);

#else
#error cb.h included twice
#endif // CB_H
