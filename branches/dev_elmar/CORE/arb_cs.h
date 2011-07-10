// ============================================================= //
//                                                               //
//   File      : arb_cs.h                                        //
//   Purpose   : Basics for client/server communication          //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef ARB_CS_H
#define ARB_CS_H

#ifndef ARB_CORE_H
#include "arb_core.h"
#endif

void arb_gethostbyname(const char *name, struct hostent *& he, GB_ERROR& err);
const char *arb_gethostname();

#else
#error arb_cs.h included twice
#endif // ARB_CS_H
