double DI_dna_shamos::make_dist(di_mldist *helical,di_mldist *nonhelical,
                                long *helix_filter, char *seq1, char *seq2){
    /* seq are e [0..max_dna], all seqs are already filtered
     */




}


double DI_dna_shamos::main(GDATA *gb_main,DI_MATRIX *matrix,AP_filter *ap_filter){
    GB_transaction transaction_scope(gb_main);
    BI_helix *bi_helix = new BI_helix(gb_main);

    long *helix = matrix->create_helix_filter(bi_helix,ap_filter);
