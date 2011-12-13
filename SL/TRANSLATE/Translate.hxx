// =============================================================== //
//                                                                 //
//   File      : Translate.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2006      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef TRANSLATE_HXX
#define TRANSLATE_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


GB_ERROR AWT_getTranslationInfo(GBDATA *gb_species, int& arb_transl_table, int &codon_start);
GB_ERROR AWT_saveTranslationInfo(GBDATA *gb_species, int arb_transl_table, int codon_start);
GB_ERROR AWT_removeTranslationInfo(GBDATA *gb_species);

int AWT_pro_a_nucs_convert(int arb_code_nr, char *data, size_t size, size_t pos, bool translate_all, bool create_start_codon, bool append_stop_codon, int *translatedSize);

#else
#error Translate.hxx included twice
#endif // TRANSLATE_HXX
