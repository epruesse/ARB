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
    // NoTuple (basic test):
    ShadedValue undef = ValueTuple::undefined();
    TEST_REJECT(undef->is_defined());
    TEST_REJECT(undef->clone()->is_defined());
    TEST_EXPECT_EQUAL(undef->inspect(), "<undef>");

    // LinearTuple (basic test):
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
    TEST_EXPECT_EQUAL(undef->mix(0.0, *half)->inspect(), "(0.500)");
    TEST_EXPECT_EQUAL(undef->mix(1.0, *half)->inspect(), "(0.500)");
    TEST_EXPECT_EQUAL(half->mix(0.1, *undef)->inspect(), "(0.500)");

    // @@@ tdd PlanarTuple
    // @@@ tdd CubicTuple

    // test pmake
    ShadedValue Pundef = ValueTuple::pmake(NULL);
    TEST_REJECT(Pundef->is_defined());
    TEST_EXPECT_EQUAL(Pundef->inspect(), "<undef>");

    float       f    = 0.75;
    ShadedValue Pdef = ValueTuple::pmake(&f);
    TEST_EXPECT(Pdef->is_defined());
    TEST_EXPECT_EQUAL(Pdef->inspect(), "(0.750)");
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

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
    int range_offset() const OVERRIDE {
        int off =
            val <= 0.0
            ? 0
            : (val >= 1.0
               ? AW_RANGE_COLORS-1
               : val*AW_RANGE_COLORS+0.5);

        is_assert(off>=0 && off<AW_RANGE_COLORS);
        return off;
    }

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE {
        static SmartCharPtr buf;
        buf = GBS_global_string_copy("(%.3f)", val);
        return &*buf;
    }
#endif

    // mixer:
    ValueTuple *reverse_mix(float, const NoTuple&) const OVERRIDE { return clone(); }
    ValueTuple *reverse_mix(float other_ratio, const LinearTuple& other) const OVERRIDE {
        return new LinearTuple(other_ratio*other.val + (1-other_ratio)*val);
    }
    ValueTuple *mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
};

// ---------------------------------
//      mixer (late definition)

ValueTuple *NoTuple::reverse_mix(float, const LinearTuple& other) const { return other.clone(); }

// -----------------
//      factory

ValueTuple *ValueTuple::undefined() {
    return new NoTuple;
}
ValueTuple *ValueTuple::make(float f) {
    return new LinearTuple(f);
}

