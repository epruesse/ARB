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

// simplest callback flavours of each type
typedef AW_window *(*CreateWindowCallbackSimple)(AW_root*); // use makeCreateWindowCallback if you need parameters
typedef void (*RootCallbackSimple)(AW_root*);               // use makeRootCallback if you need parameters
typedef void (*WindowCallbackSimple)(AW_window*);           // use makeWindowCallback if you need parameters

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
