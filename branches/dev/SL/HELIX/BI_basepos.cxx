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



/*!*************************************************************************************
*******         Reference to abs pos                    ********
****************************************************************************************/
void BI_ecoli_ref::bi_exit() {
    delete [] abs2rel;
    delete [] rel2abs;
    abs2rel = 0;
    rel2abs = 0;
}

BI_ecoli_ref::BI_ecoli_ref() {
    memset((char *)this, 0, sizeof(BI_ecoli_ref));
}

BI_ecoli_ref::~BI_ecoli_ref() {
    bi_exit();
}

inline bool isGap(char c) { return c == '-' || c == '.'; }

const char *BI_ecoli_ref::init(const char *seq, size_t size) {
    bi_exit();

    abs2rel = new size_t[size];
    rel2abs = new size_t[size];
    memset((char *)rel2abs, 0, (size_t)(sizeof(*rel2abs)*size));

    relLen = 0;
    absLen = size;
    size_t i;
    size_t sl = strlen(seq);
    for (i=0; i<size; i++) {
        abs2rel[i]      = relLen;
        rel2abs[relLen] = i;
        if (i<sl && !isGap(seq[i])) ++relLen;
    }

    bi_assert(relLen>0); // otherwise BI_ecoli_ref behaves completely wrong

    return 0;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main, char *alignment_name, char *ref_name) {
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
                err = init(GB_read_char_pntr(gb_ref), size);
            }
        }
    }
    return err;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    char     *ref = GBT_get_default_ref(gb_main);
    char     *use = GBT_get_default_alignment(gb_main);
    GB_ERROR  err = init(gb_main, use, ref);

    free(ref);
    free(use);

    return err;
}




