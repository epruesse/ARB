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
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#ifndef bi_assert
#define bi_assert(bed) arb_assert(bed)
#endif

typedef bool (*char_predicate_fun)(char);

class CharPredicate { // general predicate for char
    bool isTrue[256];
public:
    explicit CharPredicate(char_predicate_fun is_true) {
        for (int i = 0; i<256; ++i) {
            isTrue[i] = is_true(safeCharIndex((unsigned char)i));
        }
    }

    bool applies(char c) const { return isTrue[safeCharIndex(c)]; }
    bool applies(unsigned char c) const { return isTrue[c]; }
    bool applies(int i) const { bi_assert(i == (unsigned char)i); return isTrue[i]; }
};

typedef char_predicate_fun is_gap_fun;

class BasePosition : virtual Noncopyable {
    int absLen;
    int baseCount;

    int *abs2rel;
    int *rel2abs;

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
    void initialize(const char *seq, int size);
    void initialize(const char *seq, int size, const CharPredicate& is_gap);

    BasePosition() { setup(); }
    BasePosition(const char *seq, int size) { setup(); initialize(seq, size); }
    BasePosition(const char *seq, int size, const CharPredicate& is_gap) { setup(); initialize(seq, size, is_gap); }
    ~BasePosition() { cleanup(); }

    bool gotData() const { return abs2rel != 0; }

    int abs_2_rel(int abs) const {
        // returns the number of base characters in range [0..abs-1]!
        // (i.e. result is in [0..bases])
        // 'abs' has to be in range [0..abs_count()]

        bi_assert(gotData());
        if (abs<0) return 0;
        if (abs > absLen) abs = absLen;
        return abs2rel[abs];
    }

    int rel_2_abs(int rel) const {
        // 'rel' is [0..N-1] (0 = 1st base, 1 = 2nd base)
        // returns abs. position of that base

        bi_assert(gotData());
        if (rel >= baseCount) rel = baseCount-1;
        if (rel<0) return 0;
        return rel2abs[rel];
    }

    int first_base_abspos() const { return rel_2_abs(0); }
    int last_base_abspos() const { return rel_2_abs(baseCount-1); }

    int base_count() const { return baseCount; }
    int abs_count() const { return absLen; }
};



struct BI_ecoli_ref : public BasePosition {
    BI_ecoli_ref() {}

    void init(const char *seq, int size) { initialize(seq, size); }
    GB_ERROR init(GBDATA *gb_main);
    GB_ERROR init(GBDATA *gb_main, char *alignment_name, char *ref_name);
};


#else
#error BI_basepos.hxx included twice
#endif // BI_BASEPOS_HXX
