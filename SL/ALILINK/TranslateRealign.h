// ============================================================== //
//                                                                //
//   File      : TranslateRealign.h                               //
//   Purpose   : Translate and realign interface                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef TRANSLATEREALIGN_H
#define TRANSLATEREALIGN_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

GB_ERROR arb_r2a(GBDATA *gb_main, bool use_entries, bool save_entries, int selected_startpos, bool translate_all, const char *ali_source, const char *ali_dest); // @@@ rename
GB_ERROR realign_marked(GBDATA *gb_main, const char *ali_source, const char *ali_dest, size_t& neededLength, bool unmark_succeeded, bool cutoff_dna); // @@@ rename

#else
#error TranslateRealign.h included twice
#endif // TRANSLATEREALIGN_H
