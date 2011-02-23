// ================================================================ //
//                                                                  //
//   File      : core.h                                             //
//   Purpose   : libCORE internals                                  //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef CORE_H
#define CORE_H

#ifndef ARB_CORE_H
#include "arb_core.h"
#endif

extern gb_error_handler_type     gb_error_handler;
extern gb_warning_func_type      gb_warning_func;
extern gb_information_func_type  gb_information_func;
extern gb_status_gauge_func_type gb_status_gauge_func;
extern gb_status_msg_func_type   gb_status_msg_func;


#else
#error core.h included twice
#endif // CORE_H
