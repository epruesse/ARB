// ================================================================ //
//                                                                  //
//   File      : arb_handlers.cxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "arb_handlers.h"

// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include <arb_core.h>
// AISC_MKPT_PROMOTE:#endif

static void gb_error_to_stderr(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

gb_error_handler_type     gb_error_handler = gb_error_to_stderr;
gb_warning_func_type      gb_warning_func;
gb_information_func_type  gb_information_func;
gb_status_gauge_func_type gb_status_gauge_func;
gb_status_msg_func_type   gb_status_msg_func;


void GB_install_error_handler(gb_error_handler_type aw_message_handler) {
    gb_error_handler = aw_message_handler;
}

void GB_install_warning(gb_warning_func_type warn) {
    gb_warning_func = warn;
}

void GB_install_information(gb_information_func_type info) {
    gb_information_func = info;
}

void GB_install_status(gb_status_gauge_func_type gauge_fun, gb_status_msg_func_type msg_fun) {
    gb_status_gauge_func = gauge_fun;
    gb_status_msg_func   = msg_fun;
}

