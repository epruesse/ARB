// ============================================================= //
//                                                               //
//   File      : BI_basepos.hxx                                  //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef BI_BASEPOS_HXX
#define BI_BASEPOS_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif


#ifndef bi_assert
#define bi_assert(bed) arb_assert(bed)
#endif

class BI_ecoli_ref {
    size_t absLen;
    size_t relLen;

    size_t *abs2rel;
    size_t *rel2abs;

    void bi_exit();

public:
    BI_ecoli_ref();
    ~BI_ecoli_ref();

    const char *init(GBDATA *gb_main);
    const char *init(GBDATA *gb_main, char *alignment_name, char *ref_name);
    const char *init(const char *seq, size_t size);

    bool gotData() const { return abs2rel != 0; }

    size_t abs_2_rel(size_t abs) const {
        // returns the number of base characters in range [0..abs-1]!
        bi_assert(gotData());
        if (abs >= absLen) abs = absLen-1;
        return abs2rel[abs];
    }

    size_t rel_2_abs(size_t rel) const {
        bi_assert(gotData());
        if (rel >= relLen) rel = relLen-1;
        return rel2abs[rel];
    }

    size_t base_count() const { return relLen; }
    size_t abs_count() const { return absLen; }
};

#else
#error BI_basepos.hxx included twice
#endif // BI_BASEPOS_HXX
