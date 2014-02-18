// ============================================================= //
//                                                               //
//   File      : BI_basepos.cxx                                  //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "BI_basepos.hxx"

#include <arbdbt.h>


static bool is_Gap(char c) { return c == '-' || c == '.'; }

// ---------------------
//      BasePosition

void BasePosition::initialize(const char *seq, int size) {
    static CharPredicate pred_is_gap(is_Gap);
    initialize(seq, size, pred_is_gap);
}

void BasePosition::initialize(const char *seq, int size, const CharPredicate& is_gap) {
    cleanup();

    bi_assert(size >= 0);

    absLen  = size;
    abs2rel = new int[absLen+1];

    int i;
    for (i = 0; i<size && seq[i]; ++i) {
        abs2rel[i] = baseCount;
        if (!is_gap.applies(seq[i])) ++baseCount;
    }
    bi_assert(baseCount >= 0);

    for (; i <= size; ++i) {
        abs2rel[i] = baseCount;
    }

    rel2abs = new int[baseCount+1];
    for (i = size; i>0; --i) {
        int rel = abs2rel[i];
        if (rel) {
            rel2abs[rel-1] = i-1;
        }
    }
}

// ---------------------
//      BI_ecoli_ref

GB_ERROR BI_ecoli_ref::init(GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    char     *ref = GBT_get_default_ref(gb_main);
    char     *use = GBT_get_default_alignment(gb_main);
    GB_ERROR  err = init(gb_main, use, ref);

    free(ref);
    free(use);

    return err;
}

GB_ERROR BI_ecoli_ref::init(GBDATA *gb_main, char *alignment_name, char *ref_name) {
    GB_transaction ta(gb_main);

    GB_ERROR err  = 0;
    long     size = GBT_get_alignment_len(gb_main, alignment_name);

    if (size<=0) err = GB_await_error();
    else {
        GBDATA *gb_ref_con   = GBT_find_SAI(gb_main, ref_name);
        if (!gb_ref_con) err = GBS_global_string("I cannot find the SAI '%s'", ref_name);
        else {
            GBDATA *gb_ref   = GBT_read_sequence(gb_ref_con, alignment_name);
            if (!gb_ref) err = GBS_global_string("Your SAI '%s' has no sequence '%s/data'", ref_name, alignment_name);
            else {
                const char *data = GB_read_char_pntr(gb_ref);
                if (!data) {
                    err = GB_await_error();
                }
                else {
                    init(data, size);
                }
            }
        }
    }
    return err;
}

