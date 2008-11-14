// =============================================================== //
//                                                                 //
//   File      : AWT_translate.h                                   //
//   Purpose   :                                                   //
//   Time-stamp: <Fri Nov/14/2008 12:23 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2006      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef AWT_TRANSLATE_H
#define AWT_TRANSLATE_H

GB_ERROR AWT_findTranslationTable(GBDATA *gb_species, int& arb_transl_table);
int AWT_pro_a_nucs_convert(int code_nr, char *data, long size, int pos, bool translate_all);

#else
#error AWT_translate.h included twice
#endif // AWT_TRANSLATE_H

