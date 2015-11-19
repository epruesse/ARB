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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class AW_root;
class AW_window;
class AW_awar;

// generic callback parameter (used like a void*)
typedef long AW_CL; // client data (casted from pointer or value)

// Note:
// most callbacks below are considered obsolete (as soon as #432 is completed)
// They are mostly used to cast functions; better use make....Callback-wrappers 

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

// simplest callback flavours of each type
typedef AW_window *(*CreateWindowCallbackSimple)(AW_root*); // use makeCreateWindowCallback if you need parameters
typedef AW_RCB0 RootCallbackSimple;                         // use makeRootCallback if you need parameters
typedef AW_CB0  WindowCallbackSimple;                       // use makeWindowCallback if you need parameters

// ---------------------------
//      typesafe callbacks

#ifndef CBTYPES_H
#include <cbtypes.h>
#endif

// @@@ when gtk port is back to trunk, the definition of some of these cb-types may be moved into WINDOW

DECLARE_CBTYPE_FVV_AND_BUILDERS(RootCallback,         void,       AW_root*);   // generates makeRootCallback
DECLARE_CBTYPE_FVV_AND_BUILDERS(TimedCallback,        unsigned,   AW_root*);   // generates makeTimedCallback (return value: 0->do not call again, else: call again after XXX ms)
DECLARE_CBTYPE_FVV_AND_BUILDERS(WindowCallback,       void,       AW_window*); // generates makeWindowCallback
DECLARE_CBTYPE_FVV_AND_BUILDERS(CreateWindowCallback, AW_window*, AW_root*);   // generates makeCreateWindowCallback

DECLARE_CBTYPE_FVF_AND_BUILDERS(DatabaseCallback, void, GBDATA*, GB_CB_TYPE);    // generates makeDatabaseCallback

DECLARE_CBTYPE_FFV_AND_BUILDERS(TreeAwarCallback, void, AW_awar*, bool); // generates makeTreeAwarCallback

#else
#error cb.h included twice
#endif // CB_H
