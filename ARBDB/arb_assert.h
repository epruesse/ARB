/*  ====================================================================

    File      : arb_assert.h
    Purpose   : Global assert macro


  Coded by Ralf Westram (coder@reallysoft.de) in August 2002
  Copyright Department of Microbiology (Technical University Munich)

  Visit our web site at: http://www.arb-home.de/


  ==================================================================== */

#ifndef ARB_ASSERT_H
#define ARB_ASSERT_H

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

/* check correct definition of DEBUG/NDEBUG */
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


/* only use ONE of the following ASSERT_xxx defines : */

#if defined(DEBUG) && !defined(DEVEL_RELEASE)

/* assert that raises SIGSEGV (recommended for DEBUG version!) */
// # define ASSERT_CRASH
# define ASSERT_BACKTRACE_AND_CRASH
/* test if a bug has to do with assertion code */
/* # define ASSERT_NONE */

#else

/* no assert (recommended for final version!) */
# define ASSERT_NONE
/* assert as error in final version (allows basic debugging of NDEBUG version) */
/* # define ASSERT_ERROR */
/* assert as print in final version (allows basic debugging of NDEBUG version) */
/* # define ASSERT_PRINT */

#endif

/* -------------------------------------------------------------------------------
 * Provoke a SIGSEGV (which will stop the debugger or terminated the application)
 * Do backtrace manually here and uninstall SIGSEGV-handler
 * (done because automatic backtrace on SIGSEGV lacks name of current function) 
 */

#define ARB_SIGSEGV(backtrace) do {                             \
        if (backtrace) GBK_dump_backtrace(NULL, "ARB_SIGSEGV"); \
        GBK_install_SIGSEGV_handler(GB_FALSE);                  \
        *(int *)0 = 0;                                          \
    } while(0)

/* ------------------------------------------------------------ */

/* use ASSERTION_USED for code needed for assertions */
#define ASSERTION_USED

/* ------------------------------------------------------------ */

#if defined(SIMPLE_ARB_ASSERT)

#ifndef ASSERT_NONE
# define arb_assert(cond)                                               \
    do  {                                                               \
        if (!(cond)) {                                                  \
                fprintf(stderr, "Assertion '%s' failed in '%s' #%i\n",  \
                        #cond, __FILE__, __LINE__);                     \
            *(int *)0 = 0;                                              \
        }                                                               \
    } while (0)
#endif

/* ------------------------------------------------------------ */
#else

#ifdef ASSERT_CRASH
# define arb_assert(cond)                       \
    do {                                        \
        if (!(cond)) ARB_SIGSEGV(0);            \
    } while (0)
#endif

#ifdef ASSERT_BACKTRACE_AND_CRASH
# define arb_assert(cond)                                               \
    do {                                                                \
        if (!(cond)) {                                                  \
            fputs(GBK_assert_msg(#cond, __FILE__, __LINE__), stderr);   \
            fflush(stderr);                                             \
            ARB_SIGSEGV(1);                                             \
        }                                                               \
    } while (0)
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
    } while(0)
#endif // SIMPLE_ARB_ASSERT

/* ------------------------------------------------------------ */
/* use the following macros for parameters etc. only appearing in one version */

#ifdef DEBUG
# define IF_DEBUG(x) x
# define IF_NDEBUG(x)
#else
# define IF_DEBUG(x)
# define IF_NDEBUG(x) x
#endif

/* ------------------------------------------------------------ */
/* assert specific result (DEBUG only) and silence __ATTR__USERESULT warnings */

#define ASSERT_RESULT(Type, Expected, Expr) do {        \
        Type value = (Expr);                            \
        arb_assert(value == (Expected));                \
        value             = value;                      \
    } while(0)

#define ASSERT_NULL_RESULT(ptrExpr) ASSERT_RESULT(const void*, NULL, ptrExpr)
#define ASSERT_NO_ERROR(error)      ASSERT_RESULT(GB_ERROR, NULL, error)

/* ------------------------------------------------------------ */

#ifdef DEVEL_RELEASE
#ifdef ASSERTION_USED
#error Assertions enabled in release
#endif
#endif

/* ------------------------------------------------------------ */

#if !defined(SIMPLE_ARB_ASSERT)
#ifndef ADLOCAL_H
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#endif
#endif // SIMPLE_ARB_ASSERT

#else
#error arb_assert.h included twice
#endif /* ARB_ASSERT_H */

