#ifndef dist_hxx_included
#define dist_hxx_included

#ifndef awt_seq_dna_hxx_included
#include <awt_seq_dna.hxx>
#endif
#ifndef awt_seq_simple_pro_hxx_included
#include <awt_seq_simple_pro.hxx>
#endif

#define di_assert(cond) arb_assert(cond)

extern GBDATA *GLOBAL_gb_main;
GBT_TREE *neighbourjoining(char **names, AP_FLOAT **m, long size, size_t structure_size);

extern AW_CL dist_filter_cl;


#endif
