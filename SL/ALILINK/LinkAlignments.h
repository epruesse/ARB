// ============================================================== //
//                                                                //
//   File      : LinkAlignments.h                                 //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef LINKALIGNMENTS_H
#define LINKALIGNMENTS_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

const char *ALI_is_linked(GBDATA *gb_main, const char *ali);
bool        ALI_are_linked(GBDATA *gb_main, const char *ali1, const char *ali2);

GB_ERROR ALI_link(GBDATA *gb_main, const char *ali1, const char *ali2);
GB_ERROR ALI_unlink(GBDATA *gb_main, const char *ali1, const char *ali2);

#else
#error LinkAlignments.h included twice
#endif // LINKALIGNMENTS_H
