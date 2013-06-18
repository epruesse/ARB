// ================================================================ //
//                                                                  //
//   File      : arb_core.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_CORE_H
#define ARB_CORE_H

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef DUPSTR_H
#include <dupstr.h>
#endif

typedef const char *GB_ERROR; // memory managed by CORE

typedef void (*gb_error_handler_type)(const char *msg);
typedef void (*gb_warning_func_type)(const char *msg);
typedef void (*gb_information_func_type)(const char *msg);
typedef int (*gb_status_gauge_func_type)(double val);
typedef int (*gb_status_msg_func_type)(const char *val);


bool GBK_running_on_valgrind(void);

bool GBK_raises_SIGSEGV(void (*cb)(void), bool result_in_valgrind);
void GBK_install_SIGSEGV_handler(bool dump_backtrace);

GB_ERROR GBK_assert_msg(const char *assertion, const char *file, int linenr);

void GBK_dump_backtrace(FILE *out, const char *message);
void GBK_terminate(const char *error);

#else
#error arb_core.h included twice
#endif // ARB_CORE_H
