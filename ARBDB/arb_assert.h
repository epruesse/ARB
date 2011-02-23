/*  ====================================================================

    File      : arb_assert.h
    Purpose   : Global assert macro


  Coded by Ralf Westram (coder@reallysoft.de) in August 2002
  Copyright Department of Microbiology (Technical University Munich)

  Visit our web site at: http://www.arb-home.de/


  ==================================================================== */

#ifndef ARB_ASSERT_H
#define ARB_ASSERT_H

// [WhyIncludeHere]
// need to include all headers needed for unit-tests here [sic]
// if only included inside test_global.h, developing with active unit-tests
// will always break non-unit-test-builds.

#ifndef _STDARG_H
#include <stdarg.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _ERRNO_H
#include <errno.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif

/* ------------------------------------------------------------
 * Define SIMPLE_ARB_ASSERT before including this header
 * to avoid ARBDB dependency!
 *
 * ASSERT_CRASH                 if assert fails debugger stops at assert macro
 * ASSERT_BACKTRACE_AND_CRASH   like ASSERT_CRASH - with backtrace
 * ASSERT_ERROR                 assert prints an error and ARB exits
 * ASSERT_PRINT                 assert prints a message (anyway) and ARB continues
 * ASSERT_NONE                  assertions inactive
 *
 * ------------------------------------------------------------ */

// check correct definition of DEBUG/NDEBUG
#ifndef NDEBUG
# ifndef DEBUG
#  error Neither DEBUG nor NDEBUG is defined!
# endif
#else
# ifdef DEBUG
#  error Both DEBUG and NDEBUG are defined - only one should be!
# endif
#endif

#ifdef arb_assert
#error arb_assert already defined
#endif

// only use ONE of the following ASSERT_xxx defines :

#if defined(DEBUG) && !defined(DEVEL_RELEASE)

// assert that raises SIGSEGV (recommended for DEBUG version!)
// # define ASSERT_CRASH
# define ASSERT_BACKTRACE_AND_CRASH
// test if a bug has to do with assertion code
// # define ASSERT_NONE

#else

// no assert (recommended for final version!)
# define ASSERT_NONE
// assert as error in final version (allows basic debugging of NDEBUG version)
// # define ASSERT_ERROR
// assert as print in final version (allows basic debugging of NDEBUG version)
// # define ASSERT_PRINT

#endif

// ------------------------------------------------------------

// use ASSERTION_USED for code needed for assertions
#define ASSERTION_USED

// ------------------------------------------------------------

#if defined(SIMPLE_ARB_ASSERT)

// code here is independent from ARBDB!

#define ARB_SIGSEGV(backtrace) do {                             \
        *(int *)0 = 0;                                          \
    } while (0)

#ifndef ASSERT_NONE
# define arb_assert(cond)                                               \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "Assertion '%s' failed in '%s' #%i\n",      \
                    #cond, __FILE__, __LINE__);                         \
            *(int *)0 = 0;                                              \
        }                                                               \
    } while (0)
#endif


// ------------------------------------------------------------

#else

/* Provoke a SIGSEGV (which will stop the debugger or terminated the application)
 * Do backtrace manually here and uninstall SIGSEGV-handler
 * (done because automatic backtrace on SIGSEGV lacks name of current function)
 */

#ifndef ARB_CORE_H
#include <arb_core.h>
#endif

#define ARB_SIGSEGV(backtrace) do {                             \
        if (backtrace) GBK_dump_backtrace(NULL, "ARB_SIGSEGV"); \
        GBK_install_SIGSEGV_handler(false);                     \
        *(int *)0 = 0;                                          \
    } while (0)


# define arb_assert_crash(cond)                 \
    do {                                        \
        if (!(cond)) ARB_SIGSEGV(0);            \
    } while (0)

# define arb_assert_backtrace_and_crash(cond)                           \
    do {                                                                \
        if (!(cond)) {                                                  \
            fputs(GBK_assert_msg(#cond, __FILE__, __LINE__), stderr);   \
            fflush(stderr);                                             \
            ARB_SIGSEGV(1);                                             \
        }                                                               \
    } while (0)

#ifdef ASSERT_CRASH
# define arb_assert(cond) arb_assert_crash(cond)
#endif

#ifdef ASSERT_BACKTRACE_AND_CRASH
# define arb_assert(cond) arb_assert_backtrace_and_crash(cond)
#endif

#ifdef ASSERT_ERROR
# define arb_assert(cond) assert_or_exit(cond)
#endif

#ifdef ASSERT_PRINT
# define arb_assert(cond)                                               \
    do {                                                                \
        fprintf(stderr, "at %s #%i\n", __FILE__, __LINE__);             \
        if (!(cond)) fprintf(stderr, "assertion '%s' failed!\n", #cond); \
        fflush(stderr);                                                 \
    } while (0)
#endif

#endif // SIMPLE_ARB_ASSERT

// ------------------------------------------------------------

#ifdef ASSERT_NONE
# undef ASSERTION_USED
# define arb_assert(cond)
#endif

#undef ASSERT_CRASH
#undef ASSERT_BACKTRACE_AND_CRASH
#undef ASSERT_ERROR
#undef ASSERT_PRINT
#undef ASSERT_NONE

#ifndef arb_assert
# error arb_assert has not been defined -- check ASSERT_xxx definitions
#endif

#if !defined(SIMPLE_ARB_ASSERT)
#define assert_or_exit(cond)                                            \
    do {                                                                \
        if (!(cond)) {                                                  \
            GBK_terminate(GBK_assert_msg(#cond, __FILE__, __LINE__));   \
        }                                                               \
    } while (0)
#endif // SIMPLE_ARB_ASSERT

// ------------------------------------------------------------

#if (UNIT_TESTS == 1)
#ifndef TEST_GLOBAL_H
#include <test_global.h> // overrides arb_assert()!
#endif
#else
#define RUNNING_TEST() false
#endif

// ------------------------------------------------------------
// logical operators (mostly used for assertions)

// Note: 'conclusion' is not evaluated if 'hypothesis' is wrong!
#define implicated(hypothesis,conclusion) (!(hypothesis) || !!(conclusion))

#ifdef __cplusplus
inline bool correlated(bool hypo1, bool hypo2) { return implicated(hypo1, hypo2) == implicated(hypo2, hypo1); } // equivalence!
inline bool contradicted(bool hypo1, bool hypo2) { return !correlated(hypo1, hypo2); } // non-equivalence!
#endif

// ------------------------------------------------------------
// use the following macros for parameters etc. only appearing in one version

#ifdef DEBUG
# define IF_DEBUG(x) x
# define IF_NDEBUG(x)
#else
# define IF_DEBUG(x)
# define IF_NDEBUG(x) x
#endif

#ifdef ASSERTION_USED
# define IF_ASSERTION_USED(x) x
#else
# define IF_ASSERTION_USED(x)
#endif

// ------------------------------------------------------------
// Assert specific result in DEBUG and silence __ATTR__USERESULT warnings in NDEBUG.
// 
// The value 'Expected' (or 'Limit') should be side-effect-free (it is only executed in DEBUG mode).
// The given 'Expr' is evaluated under all conditions!
// 
// Important note:
//      When you swap 'Expected' and 'Expr' by mistake,
//      code working in DEBUG, may suddenly stop working in NDEBUG!

#ifdef ASSERTION_USED

# define ASSERT_RESULT(Type, Expected, Expr) do {       \
        Type value = (Expr);                            \
        arb_assert(value == (Expected));                \
    } while (0)

# define ASSERT_RESULT_PREDICATE(Pred, Expr) do {       \
        arb_assert(Pred(Expr));                         \
    } while (0)

#else

# define ASSERT_RESULT(Type, Expected, Expr) do {       \
        (void)Expr;                                     \
    } while(0)

# define ASSERT_RESULT_PREDICATE(Pred, Expr) do {       \
        (void)Expr;                                     \
    } while(0)

#endif

#define ASSERT_NULL_RESULT(ptrExpr) ASSERT_RESULT(const void*, NULL, ptrExpr)
#define ASSERT_NO_ERROR(errorExpr)  ASSERT_RESULT(GB_ERROR, NULL, errorExpr)

#define ASSERT_TRUE(boolExpr)  ASSERT_RESULT(bool, true, boolExpr)
#define ASSERT_FALSE(boolExpr) ASSERT_RESULT(bool, false, boolExpr)

// ------------------------------------------------------------
// UNCOVERED is used to document/test missing test coverage
// see ../INCLUDE/test_global.h@UNCOVERED

#ifndef UNCOVERED
#define UNCOVERED()
#endif

// ------------------------------------------------------------

#ifdef DEVEL_RELEASE
# ifdef ASSERTION_USED
#  error Assertions enabled in release
# endif
#endif

// ------------------------------------------------------------

#if !defined(SIMPLE_ARB_ASSERT)
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif
#endif // SIMPLE_ARB_ASSERT

#else
#error arb_assert.h included twice
#endif // ARB_ASSERT_H

