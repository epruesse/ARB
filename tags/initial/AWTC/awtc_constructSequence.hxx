#ifndef awtc_constructSequence_hxx_included
#define awtc_constructSequence_hxx_included


char *AWTC_constructSequence(int parts, const char **seqs, int minBasesMatching, char **refSeq);	// parameter list should be terminated by NULL

#ifdef DEBUG
char *AWTC_testConstructSequence(const char *testWithSequence);
#endif


#endif
