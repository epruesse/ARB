// =============================================================== //
//                                                                 //
//   File      : awtc_ClustalV.hxx                                 //
//   Purpose   : Clustal V                                         //
//   Time-stamp: <Wed Jun/04/2008 09:46 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2008      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWTC_CLUSTALV_HXX
#define AWTC_CLUSTALV_HXX

GB_ERROR AWTC_ClustalV_align(int is_dna, int weighted,
                             const char *seq1, int length1, const char *seq2, int length2,
                             const int *gapsBefore1,
                             int max_seq_length,
                             char **result1, char **result2, int *result_len, int *score);
void AWTC_ClustalV_align_exit(void);

int AWTC_baseMatch(char c1, char c2);

#else
#error awtc_ClustalV.hxx included twice
#endif // AWTC_CLUSTALV_HXX
