// ================================================================= //
//                                                                   //
//   File      : arb_core.h                                          //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ARB_CORE_H
#define ARB_CORE_H

// this is a wrapper for library ARB_CORE (which has to be created @@@)
// - most or all GBK_...-functions need to be moved to ARB_CORE
// - arb_assert and ARB_ERROR shall be part of ARB_CORE


// for the moment we just continue to depend on library ARBDB :-(
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#else
#error arb_core.h included twice
#endif // ARB_CORE_H
