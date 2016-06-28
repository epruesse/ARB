// ============================================================ //
//                                                              //
//   File      : ValueTuple.cxx                                 //
//   Purpose   : Abstract value usable for shading items        //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "item_shader.h"

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_shaded_values() {
    // -------------------------------
    //      NoTuple (basic test):

    ShadedValue undef = ValueTuple::undefined();
    TEST_REJECT(undef->is_defined());
    TEST_REJECT(undef->clone()->is_defined());
    TEST_EXPECT_EQUAL(undef->inspect(), "<undef>");

    // -----------------------------------
    //      LinearTuple (basic test):

    {
        ShadedValue val0 = ValueTuple::make(0.0);
        TEST_EXPECT(val0->is_defined());
        TEST_EXPECT(val0->clone()->is_defined());
        TEST_EXPECT_EQUAL(val0->inspect(), "(0.000)");
        TEST_EXPECT_EQUAL(val0->range_offset(), 0);

        ShadedValue val1 = ValueTuple::make(1.0);
        TEST_EXPECT_EQUAL(val1->inspect(), "(1.000)");
        TEST_EXPECT_EQUAL(val1->range_offset(), AW_RANGE_COLORS-1);

        // LinearTuple (mix):
        ShadedValue half = val0->mix(0.50, *val1);
        TEST_EXPECT_EQUAL(half->inspect(), "(0.500)");
        TEST_EXPECT_EQUAL(half->range_offset(), AW_RANGE_COLORS/2);

        TEST_EXPECT_EQUAL(val0->mix(0.25, *val1)->inspect(), "(0.750)");
        TEST_EXPECT_EQUAL(val0->mix(0.60, *val1)->inspect(), "(0.400)");

        TEST_EXPECT_EQUAL(half->mix(0.25, *val1)->inspect(), "(0.875)");
        TEST_EXPECT_EQUAL(half->mix(0.50, *val1)->inspect(), "(0.750)");
        TEST_EXPECT_EQUAL(half->mix(0.75, *val1)->inspect(), "(0.625)");

        // mix LinearTuple with NoTuple:
        TEST_EXPECT_EQUAL(undef->mix(INFINITY, *half)->inspect(), "(0.500)");
        TEST_EXPECT_EQUAL(undef->mix(INFINITY, *half)->inspect(), "(0.500)");
        TEST_EXPECT_EQUAL(half->mix(INFINITY, *undef)->inspect(), "(0.500)");
    }

    // -----------------------------------
    //      PlanarTuple (basic test):

    {
        ShadedValue pval0 = ValueTuple::make(0, 0);
        TEST_EXPECT(pval0->is_defined());
        TEST_EXPECT(pval0->clone()->is_defined());
        TEST_EXPECT_EQUAL(pval0->inspect(), "(0.000,0.000)");
        TEST_EXPECT_EQUAL(pval0->range_offset(), 0);

        // PlanarTuple (mixing):
        ShadedValue px = ValueTuple::make(1, 0);
        ShadedValue py = ValueTuple::make(0, 1);

        TEST_EXPECT_EQUAL(px->mix(INFINITY, *undef)->inspect(), "(1.000,0.000)");
        TEST_EXPECT_EQUAL(undef->mix(INFINITY, *px)->inspect(), "(1.000,0.000)");
        TEST_EXPECT_EQUAL(px->mix(0.5, *py)->inspect(), "(0.500,0.500)");
        TEST_EXPECT_EQUAL(px->range_offset(), AW_PLANAR_COLORS*(AW_PLANAR_COLORS-1));
        TEST_EXPECT_EQUAL(py->range_offset(), AW_PLANAR_COLORS-1);

        // PlanarTuple (partially defined):
        ShadedValue PonlyX = ValueTuple::make(0.5, NAN);
        ShadedValue PonlyY = ValueTuple::make(NAN, 0.5);

        TEST_EXPECT(PonlyX->is_defined());
        TEST_EXPECT(PonlyY->is_defined());

        TEST_EXPECT_EQUAL(PonlyX->inspect(), "(0.500,nan)");
        TEST_EXPECT_EQUAL(PonlyY->inspect(), "(nan,0.500)");

        // partly undefined values produce a valid range-offset:
        TEST_EXPECT_EQUAL(PonlyX->range_offset(), (AW_PLANAR_COLORS*AW_PLANAR_COLORS)/2);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(), AW_PLANAR_COLORS/2);

        TEST_EXPECT_EQUAL(PonlyX->mix(INFINITY, *PonlyY)->inspect(), "(0.500,0.500)"); // mixed without using ratio
        TEST_EXPECT_EQUAL(PonlyX->mix(0.5, *px)->inspect(), "(0.750,0.000)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.5, *py)->inspect(), "(0.000,0.750)");
        TEST_EXPECT_EQUAL(PonlyX->mix(0.5, *py)->inspect(), "(0.250,1.000)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.5, *px)->inspect(), "(1.000,0.250)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.25,*px)->inspect(), "(1.000,0.125)"); // ratio only affects y-coord (x-coord is undef in PonlyY!)
        TEST_EXPECT_EQUAL(PonlyY->mix(0.75,*px)->inspect(), "(1.000,0.375)");
    }

    // @@@ tdd SpatialTuple

    // --------------------------------------
    //      test NAN leads to undefined:

    ShadedValue novalue = ValueTuple::make(NAN);
    TEST_REJECT(novalue->is_defined());

    ShadedValue noValuePair = ValueTuple::make(NAN, NAN);
    TEST_REJECT(noValuePair->is_defined());

}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

inline int CHECKED_RANGE_OFFSET(int off) {
    is_assert(off>=0 && off<AW_RANGE_COLORS);
    return off;
}

template<int RANGE_SIZE>
inline int fixed_range_offset(float val) {
    int off =
        val <= 0.0
        ? 0
        : (val >= 1.0
           ? RANGE_SIZE-1
           : val*RANGE_SIZE);

    is_assert(off>=0 && off<RANGE_SIZE);
    return off;
}

class NoTuple: public ValueTuple {
public:
    NoTuple() {}
    ~NoTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return false; }
    ValueTuple *clone() const OVERRIDE { return new NoTuple; }
    int range_offset() const OVERRIDE { is_assert(0); return -9999999; } // defines no range offset

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE { return "<undef>"; }
#endif

    // mixer:
    ValueTuple *reverse_mix(float, const LinearTuple& other) const OVERRIDE;
    ValueTuple *reverse_mix(float, const PlanarTuple& other) const OVERRIDE;
    ValueTuple *mix(float, const ValueTuple& other) const OVERRIDE { return other.clone(); }
};

class LinearTuple: public ValueTuple {
    float val;

public:
    LinearTuple(float val_) : val(val_) {
        is_assert(!is_nan_or_inf(val));
    }
    ~LinearTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return true; }
    ValueTuple *clone() const OVERRIDE { return new LinearTuple(val); }
    int range_offset() const OVERRIDE { return CHECKED_RANGE_OFFSET(fixed_range_offset<AW_RANGE_COLORS>(val)); }

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE {
        static SmartCharPtr buf;
        buf = GBS_global_string_copy("(%.3f)", val);
        return &*buf;
    }
#endif

    // mixer:
    ValueTuple *reverse_mix(float other_ratio, const LinearTuple& other) const OVERRIDE {
        return new LinearTuple(other_ratio*other.val + (1-other_ratio)*val);
    }
    ValueTuple *mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
};

inline float mix_floats(float me, float my_ratio, float other) {
    if (is_nan(me)) return other;
    if (is_nan(other)) return me;
    return my_ratio*me + (1-my_ratio)*other;
}

class PlanarTuple: public ValueTuple {
    float val1, val2;

public:
    PlanarTuple(float val1_, float val2_) :
        val1(val1_),
        val2(val2_)
    {
        is_assert(!is_inf(val1));
        is_assert(!is_inf(val2));
        is_assert(!(is_nan(val1) && is_nan(val2))); // only NAN is unwanted
    }
    ~PlanarTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return true; }
    ValueTuple *clone() const OVERRIDE { return new PlanarTuple(val1, val2); }
    int range_offset() const OVERRIDE { // returns int-offset into range [0 .. AW_RANGE_COLORS[
        int c1 = is_nan(val1) ? 0 : fixed_range_offset<AW_PLANAR_COLORS>(val1);
        int c2 = is_nan(val2) ? 0 : fixed_range_offset<AW_PLANAR_COLORS>(val2);
        return CHECKED_RANGE_OFFSET(c1*AW_PLANAR_COLORS + c2);
    }

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE {
        static SmartCharPtr buf;
        buf = GBS_global_string_copy("(%.3f,%.3f)", val1, val2);
        return &*buf;
    }
#endif

    // mixer:
    ValueTuple *reverse_mix(float other_ratio, const PlanarTuple& other) const OVERRIDE {
        return new PlanarTuple(mix_floats(other.val1, other_ratio, val1),
                               mix_floats(other.val2, other_ratio, val2));
    }
    ValueTuple *mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
};


// ---------------------------------
//      mixer (late definition)

ValueTuple *NoTuple::reverse_mix(float, const LinearTuple& other) const { return other.clone(); }
ValueTuple *NoTuple::reverse_mix(float, const PlanarTuple& other) const { return other.clone(); }

// -----------------
//      factory

ValueTuple *ValueTuple::undefined() {
    return new NoTuple;
}
ValueTuple *ValueTuple::make(float f) {
    return is_nan(f) ? undefined() : new LinearTuple(f);
}
ValueTuple *ValueTuple::make(float f1, float f2) {
    return (is_nan(f1) && is_nan(f2)) ? undefined() : new PlanarTuple(f1, f2);
}

