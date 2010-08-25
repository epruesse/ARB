// =============================================================== //
//                                                                 //
//   File      : constructSequence.hxx                             //
//   Purpose   : simple sequence assembler (unfinished)            //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 1998           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef CONSTRUCTSEQUENCE_HXX
#define CONSTRUCTSEQUENCE_HXX


char *constructSequence(int parts, const char **seqs, int minBasesMatching, char **refSeq);        // parameter list should be terminated by NULL

#ifdef DEBUG
char *testConstructSequence(const char *testWithSequence);
#endif


#else
#error constructSequence.hxx included twice
#endif // CONSTRUCTSEQUENCE_HXX
