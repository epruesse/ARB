// =============================================================== //
//                                                                 //
//   File      : arb_debug.h                                       //
//   Purpose   : some debugging tools                              //
//   Time-stamp: <Fri May/18/2007 16:12 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef ARB_DEBUG_H
#define ARB_DEBUG_H

#if defined(DEBUG)

// if you get the valgrind warning
// "Conditional jump or move depends on uninitialised value"
// for a complex statement, use the following macro to find out
// which part of the statement is the cause.

#define TEST_INITIALIZED(expr) do {             \
        if (expr) printf("0");                  \
        else printf("1");                       \
    } while(0)

#else

#define TEST_INITIALIZED(expr) { int TEST_INITIALIZED; } // create a warning

#endif // DEBUG

#else
#error arb_debug.h included twice
#endif // ARB_DEBUG_H

