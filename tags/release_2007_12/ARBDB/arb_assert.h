/*  ====================================================================

    File      : arb_assert.h
    Purpose   : Global assert macro
    Time-stamp: <Fri Aug/10/2007 13:00 MET Coder@ReallySoft.de>


  Coded by Ralf Westram (coder@reallysoft.de) in August 2002
  Copyright Department of Microbiology (Technical University Munich)

  Visit our web site at: http://www.arb-home.de/


  ==================================================================== */

#ifndef ARB_ASSERT_H
#define ARB_ASSERT_H

/* ------------------------------------------------------------
 *
 * ASSERT_CRASH if assert fails debugger stops at assert makro
 * ASSERT_ERROR assert prints an error and ARB exits
 * ASSERT_PRINT assert prints a message (anyway) and ARB continues
 * ASSERT_NONE  assertions inactive
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

#ifdef DEBUG

/* assert that raises SIGSEGV (recommended for DEBUG version!) */
# define ASSERT_CRASH
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

/* ------------------------------------------------------------ */

/* use ASSERTION_USED for code needed for assertions */
#define ASSERTION_USED

#ifdef ASSERT_CRASH
/* this assigns zero to zero-pointer (-> SIGSEGV) */
# define arb_assert(cond) do { if (!(cond)) *(int *)0=0; } while (0)
#endif

#ifdef ASSERT_ERROR
# define arb_assert(cond) do { if (!(cond)) { fprintf(stderr, "assertion '%s' failed in %s #%i\n", #cond, __FILE__, __LINE__); fflush(stderr); exit(EXIT_FAILURE); } } while (0)
#endif

#ifdef ASSERT_PRINT
# define arb_assert(cond) do { fprintf(stderr, "at %s #%i\n", __FILE__, __LINE__); if (!(cond)) { fprintf(stderr, "assertion '%s' failed!\n", #cond); } fflush(stderr); } while (0)
#endif

#ifdef ASSERT_NONE
# undef ASSERTION_USED
# define arb_assert(cond)
#endif

#undef ASSERT_CRASH
#undef ASSERT_ERROR
#undef ASSERT_PRINT
#undef ASSERT_NONE

#ifndef arb_assert
# error arb_assert has not been defined -- check ASSERT_xxx definitions
#endif

#define assert_or_exit(cond)                                            \
do {                                                                    \
    if (!(cond)) {                                                      \
        GB_internal_error(GBS_global_string("Assertion '%s' failed at %s:%i", #cond, __FILE__, __LINE__)); \
    }                                                                   \
 } while(0)

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

#ifdef DEVEL_RELEASE
#ifdef ASSERTION_USED
#error Assertions enabled in release
#endif
#endif

/* ------------------------------------------------------------ */

#else
#error arb_assert.h included twice
#endif /* ARB_ASSERT_H */

