// =============================================================== //
//                                                                 //
//   File      : sec_helix.hxx                                     //
//   Purpose   : helix related things                              //
//   Time-stamp: <Thu Sep/06/2007 10:40 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_HELIX_HXX
#define SEC_HELIX_HXX

// new way to store folded helices

char *SEC_xstring_to_foldedHelixList(const char *x_string, size_t xlength, const BI_helix *helix);
char *SEC_foldedHelixList_to_xstring(const char *foldedHelices, size_t xlength, const BI_helix *helix, GB_ERROR& error);

// support for old way to store folded helices
char *old_decode_xstring_rel_helix(GB_CSTR rel_helix, size_t xlength, const BI_helix *helix, int *no_of_helices_ptr);

#else
#error sec_helix.hxx included twice
#endif // SEC_HELIX_HXX
