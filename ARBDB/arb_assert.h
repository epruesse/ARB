/*  ====================================================================

    File      : arb_assert.h
    Purpose   : Global assert macro
    Time-stamp: <Fri Aug/23/2002 23:07 MET Coder@ReallySoft.de>


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
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
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
# define arb_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
#endif

#ifdef ASSERT_ERROR
# define arb_assert(bed) do { if (!(bed)) { printf("assertion '%s' failed in %s #%i\n", #bed, __FILE__, __LINE__); exit(EXIT_FAILURE); } } while (0)
#endif

#ifdef ASSERT_PRINT
# define arb_assert(bed) do { printf("at %s #%i\n", __FILE__, __LINE__); if (!(bed)) { printf("assertion '%s' failed!\n", #bed); } } while (0)
#endif

#ifdef ASSERT_NONE
# undef ASSERTION_USED
# define arb_assert(bed)
#endif

#undef ASSERT_CRASH
#undef ASSERT_ERROR
#undef ASSERT_PRINT
#undef ASSERT_NONE

#else
#error arb_assert.h included twice
#endif /* ARB_ASSERT_H */

