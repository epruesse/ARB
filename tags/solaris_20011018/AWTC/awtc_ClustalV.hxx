GB_ERROR AWTC_ClustalV_align(int is_dna, int weighted,
			     const char *seq1, int length1, const char *seq2, int length2,
			     const int *gapsBefore1, 
			     int max_seq_length,
			     char **result1, char **result2, int *result_len, int *score);
void AWTC_ClustalV_align_exit(void);

int AWTC_baseMatch(char c1, char c2);
