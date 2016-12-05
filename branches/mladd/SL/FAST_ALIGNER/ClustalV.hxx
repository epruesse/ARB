// =============================================================== //
//                                                                 //
//   File      : ClustalV.hxx                                      //
//   Purpose   : Clustal V                                         //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2008      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef CLUSTALV_HXX
#define CLUSTALV_HXX

#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif

ARB_ERROR ClustalV_align(int is_dna, int weighted,
                         const char *seq1, int length1, const char *seq2, int length2,
                         const int *gapsBefore1,
                         int max_seq_length,
                         const char*& result1, const char*& result2, int& result_len, int& score);
void ClustalV_exit();

int baseMatch(char c1, char c2);

#else
#error ClustalV.hxx included twice
#endif // CLUSTALV_HXX
