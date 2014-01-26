/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef ARB_MSG_H
#define ARB_MSG_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* arb_msg.cxx */

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif
#ifndef ARB_CORE_H
#include "arb_core.h"
#endif

// return error and ensure none is exported
#define RETURN_ERROR(err)  arb_assert(!GB_have_error()); return (err)


const char *GBS_vglobal_string(const char *templat, va_list parg) __ATTR__VFORMAT(1);
char *GBS_vglobal_string_copy(const char *templat, va_list parg) __ATTR__VFORMAT(1);
const char *GBS_global_string_to_buffer(char *buffer, size_t bufsize, const char *templat, ...) __ATTR__FORMAT(3);
char *GBS_global_string_copy(const char *templat, ...) __ATTR__FORMAT(1);
const char *GBS_global_string(const char *templat, ...) __ATTR__FORMAT(1);
const char *GBS_static_string(const char *str);
GB_ERROR GBK_assert_msg(const char *assertion, const char *file, int linenr);
GB_ERROR GB_export_error(const char *error);
GB_ERROR GB_export_errorf(const char *templat, ...) __ATTR__FORMAT(1) __ATTR__DEPRECATED_LATER("use GB_export_error(GBS_global_string(...))");
GB_ERROR GB_IO_error(const char *action, const char *filename);
GB_ERROR GB_export_IO_error(const char *action, const char *filename) __ATTR__DEPRECATED_TODO("use GB_export_error(GB_IO_error(...))");
GB_ERROR GB_print_error(void) __ATTR__DEPRECATED_TODO("will be removed completely");
GB_ERROR GB_get_error(void) __ATTR__DEPRECATED_TODO("consider using either GB_have_error() or GB_await_error()");
bool GB_have_error(void);
GB_ERROR GB_await_error(void);
void GB_clear_error(void);
GB_ERROR GB_failedTo_error(const char *do_something, const char *special, GB_ERROR error);
GB_ERROR GB_append_exportedError(GB_ERROR error);
class BackTraceInfo *GBK_get_backtrace(size_t skipFramesAtBottom);
void GBK_dump_former_backtrace(class BackTraceInfo *trace, FILE *out, const char *message);
void GBK_free_backtrace(class BackTraceInfo *trace);
void GBK_dump_backtrace(FILE *out, const char *message);
void GB_internal_error(const char *message);
void GB_internal_errorf(const char *templat, ...) __ATTR__FORMAT(1);
void GBK_terminate(const char *error) __ATTR__NORETURN;
void GBK_terminatef(const char *templat, ...) __ATTR__FORMAT(1) __ATTR__NORETURN;

inline void GBK_terminate_on_error(const char *error) { if (error) GBK_terminatef("Fatal error: %s", error); }

void GB_warning(const char *message);
void GB_warningf(const char *templat, ...) __ATTR__FORMAT(1);
void GB_information(const char *message);
void GB_informationf(const char *templat, ...) __ATTR__FORMAT(1);
void GBS_reuse_buffer(const char *global_buffer);
GB_ERROR GBK_system(const char *system_command) __ATTR__USERESULT;

#else
#error arb_msg.h included twice
#endif /* ARB_MSG_H */
