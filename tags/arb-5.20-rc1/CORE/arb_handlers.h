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
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

enum arb_status_type {
    AST_FORWARD, // gauge may only increment (not decrement)
    AST_RANDOM,  // random gauge allowed
};

struct arb_status_implementation {
    arb_status_type type;
    void (*openstatus)(const char *title);   // opens the status window and sets title
    void (*closestatus)();                   // close the status window
    bool (*set_title)(const char *title);    // set the title (return true on user abort)
    bool (*set_subtitle)(const char *title); // set the subtitle (return true on user abort)
    bool (*set_gauge)(double gauge);         // set the gauge (=percent) (return true on user abort)
    bool (*user_abort)();                    // return true on user abort
};

struct arb_handlers {
    gb_error_handler_type     show_error;
    gb_warning_func_type      show_warning;
    gb_information_func_type  show_message;
    arb_status_implementation status;
};

extern arb_handlers *active_arb_handlers;

void ARB_install_handlers(arb_handlers& handlers);
void ARB_redirect_handlers_to(FILE *use_as_stderr, FILE *use_as_stdout);

#else
#error arb_handlers.h included twice
#endif // ARB_HANDLERS_H
