// ============================================================== //
//                                                                //
//   File      : client_types.h                                   //
//   Purpose   : AISC types used by clients                       //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2010   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef CLIENT_TYPES_H
#define CLIENT_TYPES_H

#ifndef __cplusplus
#error AISC clients no longer work in plain C
#endif

#ifndef AISC_GLOBAL_H
#include <aisc_global.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#else
#error client_types.h included twice
#endif // CLIENT_TYPES_H
