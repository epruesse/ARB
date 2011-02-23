// ================================================================ //
//                                                                  //
//   File      : arb_handlers.h                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_HANDLERS_H
#define ARB_HANDLERS_H

#ifndef ARB_CORE_H
#include <arb_core.h>
#endif

void GB_install_error_handler(gb_error_handler_type aw_message_handler);
void GB_install_warning(gb_warning_func_type warn);
void GB_install_information(gb_information_func_type info);
void GB_install_status(gb_status_gauge_func_type gauge_fun, gb_status_msg_func_type msg_fun);

#else
#error arb_handlers.h included twice
#endif // ARB_HANDLERS_H
