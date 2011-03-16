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

typedef bool (*is_gap_fun)(char);

class BasePosition {
    size_t absLen;
    size_t baseCount;

    size_t *abs2rel;
    size_t *rel2abs;

    void setup() {
        absLen    = 0;
        baseCount = 0;
        abs2rel   = 0;
        rel2abs   = 0;
    }
    void cleanup() {
        delete [] abs2rel;
        delete [] rel2abs;
        setup();
    }

public:
    void initialize(const char *seq, size_t size);
    void initialize(const char *seq, size_t size, is_gap_fun is_gap);

    BasePosition() { setup(); }
    BasePosition(const char *seq, size_t size) { setup(); initialize(seq, size); }
    ~BasePosition() { cleanup(); }

    bool gotData() const { return abs2rel != 0; }

    size_t abs_2_rel(size_t abs) const {
        // returns the number of base characters in range [0..abs-1]!
        // (i.e. result is in [0..bases])
        // 'abs' has to be in range [0..abs_count()]

        bi_assert(gotData());
        if (abs > absLen) abs = absLen;
        return abs2rel[abs];
    }

    size_t rel_2_abs(size_t rel) const {
        // 'rel' is [0..N-1] (0 = 1st base, 1 = 2nd base)
        // returns abs. position of that base

        bi_assert(gotData());
        if (rel >= baseCount) rel = baseCount-1;
        return rel2abs[rel];
    }

    size_t first_base_abspos() const { return rel_2_abs(0); }
    size_t last_base_abspos() const { return rel_2_abs(baseCount-1); }

    size_t base_count() const { return baseCount; }
    size_t abs_count() const { return absLen; }
};



class BI_ecoli_ref : public BasePosition {
public: 
    BI_ecoli_ref() {}

    void init(const char *seq, size_t size) { initialize(seq, size); }
    GB_ERROR init(GBDATA *gb_main);
    GB_ERROR init(GBDATA *gb_main, char *alignment_name, char *ref_name);
};


#else
#error BI_basepos.hxx included twice
#endif // BI_BASEPOS_HXX
