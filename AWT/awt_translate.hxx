// =============================================================== //
//                                                                 //
//   File      : awt_translate.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2006      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef AWT_TRANSLATE_HXX
#define AWT_TRANSLATE_HXX

GB_ERROR AWT_getTranslationInfo(GBDATA *gb_species, int& arb_transl_table, int &codon_start);
GB_ERROR AWT_saveTranslationInfo(GBDATA *gb_species, int arb_transl_table, int codon_start);
    
int AWT_pro_a_nucs_convert(int arb_code_nr, char *data, int size, int pos, bool translate_all, bool create_start_codon, bool append_stop_codon, int *translatedSize);

#else
#error awt_translate.hxx included twice
#endif // AWT_TRANSLATE_HXX

