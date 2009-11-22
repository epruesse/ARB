// =============================================================== //
//                                                                 //
//   File      : awtc_constructSequence.hxx                        //
//   Purpose   : simple sequence assembler (unfinished)            //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 1998           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWTC_CONSTRUCTSEQUENCE_HXX
#define AWTC_CONSTRUCTSEQUENCE_HXX


char *AWTC_constructSequence(int parts, const char **seqs, int minBasesMatching, char **refSeq);        // parameter list should be terminated by NULL

#ifdef DEBUG
char *AWTC_testConstructSequence(const char *testWithSequence);
#endif


#else
#error awtc_constructSequence.hxx included twice
#endif // AWTC_CONSTRUCTSEQUENCE_HXX
