//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_DEF_H
#define AISC_DEF_H

#ifndef ARB_SIMPLE_ASSERT_H
#include <arb_simple_assert.h>
#endif

#define aisc_assert(cond) arb_assert(cond)

#if defined(DEBUG)
// #define TRACE       // aisc-"debugger"
// #define SHOW_CALLER // show where error was raised
// #define MASK_ERRORS // useful to valgrind via makefile
#endif

#define OPENFILES 16
#define HASHSIZE  1024
#define STACKSIZE 20

enum LookupScope {
    LOOKUP_LIST,
    LOOKUP_BLOCK,
    LOOKUP_BLOCK_REST,
    LOOKUP_LIST_OR_PARENT_LIST,
};

class Command;
class Code;
class hash;
class Token;
class Location;
class Data;
class Interpreter;


#if defined(SHOW_CALLER)
#define CALLER_FILE __FILE__
#define CALLER_LINE __LINE__
#else // !defined(SHOW_CALLER)
#define CALLER_FILE NULL
#define CALLER_LINE 0
#endif

#define print_error(code_or_loc, err)    (code_or_loc)->print_error_internal(err, CALLER_FILE, CALLER_LINE)
#define print_warning(code_or_loc, err)  (code_or_loc)->print_warning_internal(err, CALLER_FILE, CALLER_LINE)

#define printf_error(code_or_loc, format, arg)   print_error(code_or_loc, formatted(format, arg))
#define printf_warning(code_or_loc, format, arg) print_warning(code_or_loc, formatted(format, arg))

#else
#error aisc_def.h included twice
#endif // AISC_DEF_H
