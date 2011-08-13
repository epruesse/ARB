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
class AW_window;

// generic callback parameter (used like a void*)
typedef long AW_CL; // client data (casted from pointer or value)

// AW_root callbacks
typedef void (*AW_RCB2)(AW_root*, AW_CL, AW_CL);
typedef void (*AW_RCB1)(AW_root*, AW_CL);
typedef void (*AW_RCB0)(AW_root*);
typedef AW_RCB2 AW_RCB;

// AW_awar callbacks
typedef AW_RCB2 Awar_CB2;
typedef AW_RCB1 Awar_CB1;
typedef AW_RCB0 Awar_CB0;
typedef AW_RCB  Awar_CB;

// AW_window callbacks
typedef void (*AW_CB2)(AW_window*, AW_CL, AW_CL);
typedef void (*AW_CB1)(AW_window*, AW_CL);
typedef void (*AW_CB0)(AW_window*);
typedef AW_CB2 AW_CB;

// AW_window-builder callbacks
typedef AW_window *(*AW_Window_Creator)(AW_root*, AW_CL);

// ---------------------------
//      typesafe callbacks

#ifndef CBTYPES_H
#include "cbtypes.h"
#endif



#else
#error cb.h included twice
#endif // CB_H
