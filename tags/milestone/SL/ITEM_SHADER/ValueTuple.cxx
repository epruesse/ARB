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

        ShadedValue val1 = ValueTuple::make(1.0);
        TEST_EXPECT_EQUAL(val1->inspect(), "(1.000)");

        // LinearTuple (mix):
        ShadedValue half = val0->mix(0.50, *val1);
        TEST_EXPECT_EQUAL(half->inspect(), "(0.500)");

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

        // PlanarTuple (mixing):
        ShadedValue px = ValueTuple::make(1, 0);
        ShadedValue py = ValueTuple::make(0, 1);

        TEST_EXPECT_EQUAL(px->mix(INFINITY, *undef)->inspect(), "(1.000,0.000)");
        TEST_EXPECT_EQUAL(undef->mix(INFINITY, *px)->inspect(), "(1.000,0.000)");
        TEST_EXPECT_EQUAL(px->mix(0.5, *py)->inspect(), "(0.500,0.500)");

        // PlanarTuple (partially defined):
        ShadedValue PonlyX = ValueTuple::make(0.5, NAN);
        ShadedValue PonlyY = ValueTuple::make(NAN, 0.5);

        TEST_EXPECT(PonlyX->is_defined());
        TEST_EXPECT(PonlyY->is_defined());

        TEST_EXPECT_EQUAL(PonlyX->inspect(), "(0.500,nan)");
        TEST_EXPECT_EQUAL(PonlyY->inspect(), "(nan,0.500)");

        TEST_EXPECT_EQUAL(PonlyX->mix(INFINITY, *PonlyY)->inspect(), "(0.500,0.500)"); // mixed without using ratio
        TEST_EXPECT_EQUAL(PonlyX->mix(0.5, *px)->inspect(), "(0.750,0.000)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.5, *py)->inspect(), "(0.000,0.750)");
        TEST_EXPECT_EQUAL(PonlyX->mix(0.5, *py)->inspect(), "(0.250,1.000)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.5, *px)->inspect(), "(1.000,0.250)");
        TEST_EXPECT_EQUAL(PonlyY->mix(0.25,*px)->inspect(), "(1.000,0.125)"); // ratio only affects y-coord (x-coord is undef in PonlyY!)
        TEST_EXPECT_EQUAL(PonlyY->mix(0.75,*px)->inspect(), "(1.000,0.375)");
    }

    // ------------------------------------
    //      SpatialTuple (basic test):

    {
        ShadedValue sval0 = ValueTuple::make(0, 0, 0);
        TEST_EXPECT(sval0->is_defined());
        TEST_EXPECT(sval0->clone()->is_defined());
        TEST_EXPECT_EQUAL(sval0->inspect(), "(0.000,0.000,0.000)");

        // SpatialTuple (mixing):
        ShadedValue px = ValueTuple::make(1, 0, 0);
        ShadedValue py = ValueTuple::make(0, 1, 0);
        ShadedValue pz = ValueTuple::make(0, 0, 1);

        TEST_EXPECT_EQUAL(px->mix(INFINITY, *undef)->inspect(), "(1.000,0.000,0.000)");
        TEST_EXPECT_EQUAL(undef->mix(INFINITY, *px)->inspect(), "(1.000,0.000,0.000)");
        TEST_EXPECT_EQUAL(px->mix(0.5, *py)->inspect(), "(0.500,0.500,0.000)");
        TEST_EXPECT_EQUAL(px->mix(0.5, *py)->mix(2/3.0, *pz)->inspect(), "(0.333,0.333,0.333)");

        // SpatialTuple (partially defined):
        ShadedValue PonlyX = ValueTuple::make(0.5, NAN, NAN);
        ShadedValue PonlyY = ValueTuple::make(NAN, 0.5, NAN);
        ShadedValue PonlyZ = ValueTuple::make(NAN, NAN, 0.5);

        TEST_EXPECT(PonlyX->is_defined());
        TEST_EXPECT(PonlyY->is_defined());
        TEST_EXPECT(PonlyZ->is_defined());

        TEST_EXPECT_EQUAL(PonlyX->inspect(), "(0.500,nan,nan)");
        TEST_EXPECT_EQUAL(PonlyY->inspect(), "(nan,0.500,nan)");
        TEST_EXPECT_EQUAL(PonlyZ->inspect(), "(nan,nan,0.500)");

        ShadedValue pxy = PonlyX->mix(INFINITY, *PonlyY); // mixed without using ratio
        ShadedValue pyz = PonlyY->mix(INFINITY, *PonlyZ);
        ShadedValue pzx = PonlyZ->mix(INFINITY, *PonlyX);

        TEST_EXPECT_EQUAL(pxy->inspect(), "(0.500,0.500,nan)");
        TEST_EXPECT_EQUAL(pyz->inspect(), "(nan,0.500,0.500)");
        TEST_EXPECT_EQUAL(pzx->inspect(), "(0.500,nan,0.500)");

        TEST_EXPECT_EQUAL(pxy->mix(INFINITY, *PonlyZ)->inspect(), "(0.500,0.500,0.500)");
        TEST_EXPECT_EQUAL(pyz->mix(INFINITY, *PonlyX)->inspect(), "(0.500,0.500,0.500)");
        TEST_EXPECT_EQUAL(pzx->mix(INFINITY, *PonlyY)->inspect(), "(0.500,0.500,0.500)");

        TEST_EXPECT_EQUAL(pxy->mix(0.5, *pyz)->inspect(), "(0.500,0.500,0.500)");
        TEST_EXPECT_EQUAL(pyz->mix(0.5, *pzx)->inspect(), "(0.500,0.500,0.500)");
        TEST_EXPECT_EQUAL(pzx->mix(0.5, *pxy)->inspect(), "(0.500,0.500,0.500)");

        TEST_EXPECT_EQUAL(pxy->mix(0.5, *px)->inspect(), "(0.750,0.250,0.000)");
        TEST_EXPECT_EQUAL(pyz->mix(0.5, *px)->inspect(), "(1.000,0.250,0.250)");
        TEST_EXPECT_EQUAL(pzx->mix(0.5, *px)->inspect(), "(0.750,0.000,0.250)");

        TEST_EXPECT_EQUAL(pxy->mix(0.5, *py)->inspect(), "(0.250,0.750,0.000)");
        TEST_EXPECT_EQUAL(pyz->mix(0.5, *py)->inspect(), "(0.000,0.750,0.250)");
        TEST_EXPECT_EQUAL(pzx->mix(0.5, *py)->inspect(), "(0.250,1.000,0.250)");

        TEST_EXPECT_EQUAL(pxy->mix(0.5, *pz)->inspect(), "(0.250,0.250,1.000)");
        TEST_EXPECT_EQUAL(pyz->mix(0.5, *pz)->inspect(), "(0.000,0.250,0.750)");
        TEST_EXPECT_EQUAL(pzx->mix(0.5, *pz)->inspect(), "(0.250,0.000,0.750)");

        TEST_EXPECT_EQUAL(pxy->mix(0.25, *pz)->inspect(), "(0.125,0.125,1.000)"); // ratio does not affect z-coord (only defined in pz)
        TEST_EXPECT_EQUAL(pyz->mix(0.25, *pz)->inspect(), "(0.000,0.125,0.875)");
        TEST_EXPECT_EQUAL(pzx->mix(0.25, *pz)->inspect(), "(0.125,0.000,0.875)");
    }

    // --------------------------------------
    //      test NAN leads to undefined:

    ShadedValue novalue = ValueTuple::make(NAN);
    TEST_REJECT(novalue->is_defined());

    ShadedValue noValuePair = ValueTuple::make(NAN, NAN);
    TEST_REJECT(noValuePair->is_defined());

    ShadedValue noValueTriple = ValueTuple::make(NAN, NAN, NAN);
    TEST_REJECT(noValueTriple->is_defined());
}

void TEST_value_color_mapping() {
    // tests mapping from ShadedValue into range_offset (with different phasing)

    const float NOSHIFT = 0.0;

    Phaser dontPhase;

    Phaser halfPhase           (0.5, false, NOSHIFT, NOSHIFT);
    Phaser halfPhasePreShiftPos(0.5, false, +0.25,   NOSHIFT);
    Phaser halfPhasePreShiftNeg(0.5, false, +0.75,   NOSHIFT); // +75% should act like -25%
    Phaser halfPhasePstShiftPos(0.5, false, NOSHIFT, +0.25);
    Phaser halfPhasePstShiftNeg(0.5, false, NOSHIFT, +0.75);

    Phaser twoPhase (2.0, false, NOSHIFT, NOSHIFT);
    Phaser alt3Phase(3.0, true,  NOSHIFT, NOSHIFT);
    Phaser alt4Phase(4.0, true,  NOSHIFT, NOSHIFT);

    {
        ShadedValue val0 = ValueTuple::make(0.0);
        ShadedValue half = ValueTuple::make(0.5);
        ShadedValue val1 = ValueTuple::make(1.0);

        TEST_EXPECT_EQUAL(val0->range_offset(dontPhase),            0);
        TEST_EXPECT_EQUAL(val0->range_offset(halfPhase),            0);
        TEST_EXPECT_EQUAL(val0->range_offset(halfPhasePreShiftPos), AW_RANGE_COLORS*3/8);
        TEST_EXPECT_EQUAL(val0->range_offset(halfPhasePreShiftNeg), AW_RANGE_COLORS*1/8);
        TEST_EXPECT_EQUAL(val0->range_offset(halfPhasePstShiftPos), AW_RANGE_COLORS*1/4);
        TEST_EXPECT_EQUAL(val0->range_offset(halfPhasePstShiftNeg), AW_RANGE_COLORS*3/4);

        TEST_EXPECT_EQUAL(val1->range_offset(dontPhase),            AW_RANGE_COLORS-1);
        TEST_EXPECT_EQUAL(val1->range_offset(halfPhase),            AW_RANGE_COLORS/2);
        TEST_EXPECT_EQUAL(val1->range_offset(halfPhasePreShiftPos), AW_RANGE_COLORS*3/8);
        TEST_EXPECT_EQUAL(val1->range_offset(halfPhasePreShiftNeg), AW_RANGE_COLORS*1/8);
        TEST_EXPECT_EQUAL(val1->range_offset(halfPhasePstShiftPos), AW_RANGE_COLORS*3/4);
        TEST_EXPECT_EQUAL(val1->range_offset(halfPhasePstShiftNeg), AW_RANGE_COLORS*1/4);

        TEST_EXPECT_EQUAL(half->range_offset(dontPhase),            AW_RANGE_COLORS/2);
        TEST_EXPECT_EQUAL(half->range_offset(twoPhase),             AW_RANGE_COLORS-1);
        TEST_EXPECT_EQUAL(half->range_offset(alt3Phase),            AW_RANGE_COLORS/2);
        TEST_EXPECT_EQUAL(half->range_offset(alt4Phase),            0);
        TEST_EXPECT_EQUAL(half->range_offset(halfPhase),            AW_RANGE_COLORS/4);
        TEST_EXPECT_EQUAL(half->range_offset(halfPhasePreShiftPos), AW_RANGE_COLORS*1/8);
        TEST_EXPECT_EQUAL(half->range_offset(halfPhasePreShiftNeg), AW_RANGE_COLORS*3/8);
        TEST_EXPECT_EQUAL(half->range_offset(halfPhasePstShiftPos), AW_RANGE_COLORS*2/4);
        TEST_EXPECT_EQUAL(half->range_offset(halfPhasePstShiftNeg), AW_RANGE_COLORS*4/4 - 1);
    }

#define PQS(c)                    (AW_PLANAR_COLORS*(c)/4)
#define PLANAR_QUARTER_SHIFT(x,y) (PQS(x)*AW_PLANAR_COLORS + PQS(y))

#if 0
    // document results of PLANAR_QUARTER_SHIFT:
#define DOCQS(x,y,res) TEST_EXPECT_EQUAL(PLANAR_QUARTER_SHIFT(x,y), res)

    DOCQS(0,0,0);  DOCQS(1,0,1024); DOCQS(2,0,2048); DOCQS(3,0,3072); DOCQS(4,0,4096);
    DOCQS(0,1,16); DOCQS(1,1,1040); DOCQS(2,1,2064); DOCQS(3,1,3088); DOCQS(4,1,4112);
    DOCQS(0,2,32); DOCQS(1,2,1056); DOCQS(2,2,2080); DOCQS(3,2,3104); DOCQS(4,2,4128);
    DOCQS(0,3,48); DOCQS(1,3,1072); DOCQS(2,3,2096); DOCQS(3,3,3120); DOCQS(4,3,4144);
    DOCQS(0,4,64); DOCQS(1,4,1088); DOCQS(2,4,2112); DOCQS(3,4,3136); DOCQS(4,4,4160);

#endif

    {
        ShadedValue pval0  = ValueTuple::make(0, 0);
        ShadedValue px     = ValueTuple::make(1, 0);
        ShadedValue py     = ValueTuple::make(0, 1);
        ShadedValue PonlyX = ValueTuple::make(0.5, NAN);
        ShadedValue PonlyY = ValueTuple::make(NAN, 0.5);

        TEST_EXPECT_EQUAL(pval0->range_offset(dontPhase), 0);
        TEST_EXPECT_EQUAL(pval0->range_offset(twoPhase),  0);
        TEST_EXPECT_EQUAL(pval0->range_offset(alt3Phase), 0);
        TEST_EXPECT_EQUAL(pval0->range_offset(alt4Phase), 0);

        TEST_EXPECT_EQUAL(px->range_offset(dontPhase), (AW_PLANAR_COLORS-1)*AW_PLANAR_COLORS);
        TEST_EXPECT_EQUAL(py->range_offset(dontPhase),  AW_PLANAR_COLORS-1);

        // partly undefined values produce a valid range-offset:
        TEST_EXPECT_EQUAL(PonlyX->range_offset(dontPhase), (AW_PLANAR_COLORS*AW_PLANAR_COLORS)/2);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(dontPhase), AW_PLANAR_COLORS/2);

        // test 2-dim-phase-scale and -shift:
        TEST_EXPECT_EQUAL(px->range_offset(twoPhase),             PLANAR_QUARTER_SHIFT(4, 0)  - AW_PLANAR_COLORS); // -1*AW_PLANAR_COLORS due to limited range!
        TEST_EXPECT_EQUAL(px->range_offset(halfPhase),            PLANAR_QUARTER_SHIFT(2, 0));
        TEST_EXPECT_EQUAL(px->range_offset(halfPhasePreShiftPos), PLANAR_QUARTER_SHIFT(3, 3)/2);
        TEST_EXPECT_EQUAL(px->range_offset(halfPhasePreShiftNeg), PLANAR_QUARTER_SHIFT(1, 1)/2);
        TEST_EXPECT_EQUAL(px->range_offset(halfPhasePstShiftPos), PLANAR_QUARTER_SHIFT(3, 1));
        TEST_EXPECT_EQUAL(px->range_offset(halfPhasePstShiftNeg), PLANAR_QUARTER_SHIFT(1, 3));

        TEST_EXPECT_EQUAL(py->range_offset(twoPhase),             PLANAR_QUARTER_SHIFT(0, 4)  - 1); // -1 due to limited range!
        TEST_EXPECT_EQUAL(py->range_offset(halfPhase),            PLANAR_QUARTER_SHIFT(0, 2));
        TEST_EXPECT_EQUAL(py->range_offset(halfPhasePreShiftPos), PLANAR_QUARTER_SHIFT(3, 3)/2);
        TEST_EXPECT_EQUAL(py->range_offset(halfPhasePreShiftNeg), PLANAR_QUARTER_SHIFT(1, 1)/2);
        TEST_EXPECT_EQUAL(py->range_offset(halfPhasePstShiftPos), PLANAR_QUARTER_SHIFT(1, 3));
        TEST_EXPECT_EQUAL(py->range_offset(halfPhasePstShiftNeg), PLANAR_QUARTER_SHIFT(3, 1));

        // test 2-dim-phase-scale and -shift (nan-content):
        TEST_EXPECT_EQUAL(PonlyX->range_offset(twoPhase),             PLANAR_QUARTER_SHIFT(4,-4)); // = PLANAR_QUARTER_SHIFT(4, 0) - AW_PLANAR_COLORS
        TEST_EXPECT_EQUAL(PonlyX->range_offset(halfPhase),            PLANAR_QUARTER_SHIFT(1, 0));
        TEST_EXPECT_EQUAL(PonlyX->range_offset(halfPhasePreShiftPos), PLANAR_QUARTER_SHIFT(1, 0)/2);
        TEST_EXPECT_EQUAL(PonlyX->range_offset(halfPhasePreShiftNeg), PLANAR_QUARTER_SHIFT(3, 0)/2);
        TEST_EXPECT_EQUAL(PonlyX->range_offset(halfPhasePstShiftPos), PLANAR_QUARTER_SHIFT(2, 0));
        TEST_EXPECT_EQUAL(PonlyX->range_offset(halfPhasePstShiftNeg), PLANAR_QUARTER_SHIFT(4,-4));

        TEST_EXPECT_EQUAL(PonlyY->range_offset(twoPhase),             PLANAR_QUARTER_SHIFT(0, 4) - 1);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(halfPhase),            PLANAR_QUARTER_SHIFT(0, 1));
        TEST_EXPECT_EQUAL(PonlyY->range_offset(halfPhasePreShiftPos), PLANAR_QUARTER_SHIFT(0, 1)/2);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(halfPhasePreShiftNeg), PLANAR_QUARTER_SHIFT(0, 3)/2);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(halfPhasePstShiftPos), PLANAR_QUARTER_SHIFT(0, 2));
        TEST_EXPECT_EQUAL(PonlyY->range_offset(halfPhasePstShiftNeg), PLANAR_QUARTER_SHIFT(0, 4) - 1);
    }

    {
        ShadedValue sval0 = ValueTuple::make(0, 0, 0);

        ShadedValue px = ValueTuple::make(1, 0, 0);
        ShadedValue py = ValueTuple::make(0, 1, 0);
        ShadedValue pz = ValueTuple::make(0, 0, 1);

        ShadedValue PonlyX = ValueTuple::make(0.5, NAN, NAN);
        ShadedValue PonlyY = ValueTuple::make(NAN, 0.5, NAN);
        ShadedValue PonlyZ = ValueTuple::make(NAN, NAN, 0.5);

        TEST_EXPECT_EQUAL(sval0->range_offset(dontPhase), 0);

        TEST_EXPECT_EQUAL(px->range_offset(dontPhase), (AW_SPATIAL_COLORS-1)*AW_SPATIAL_COLORS*AW_SPATIAL_COLORS);
        TEST_EXPECT_EQUAL(py->range_offset(dontPhase), (AW_SPATIAL_COLORS-1)*AW_SPATIAL_COLORS);
        TEST_EXPECT_EQUAL(pz->range_offset(dontPhase),  AW_SPATIAL_COLORS-1);

        // partly undefined values produce a valid range-offset:
        TEST_EXPECT_EQUAL(PonlyX->range_offset(dontPhase), (AW_SPATIAL_COLORS*AW_SPATIAL_COLORS*AW_SPATIAL_COLORS)/2);
        TEST_EXPECT_EQUAL(PonlyY->range_offset(dontPhase), (AW_SPATIAL_COLORS*AW_SPATIAL_COLORS)/2);
        TEST_EXPECT_EQUAL(PonlyZ->range_offset(dontPhase), AW_SPATIAL_COLORS/2);

        // skipping tests for phasing SpatialTuple (hardly can understand test-results for PlanarTuple)
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

inline int CHECKED_RANGE_OFFSET(int off) {
    is_assert(off>=0 && off<AW_RANGE_COLORS);
    return off;
}

template<int RANGE_SIZE>
inline int fixed_range_offset(float val) {
    is_assert(val>=0.0 && val<=1.0); // val is output of Phaser
    int off = val>=1.0 ? RANGE_SIZE-1 : val*RANGE_SIZE;
    is_assert(off>=0 && off<RANGE_SIZE);
    return off;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_range_mapping() {
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(1.00), 3);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.75), 3);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.74), 2);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.50), 2);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.49), 1);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.25), 1);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.24), 0);
    TEST_EXPECT_EQUAL(fixed_range_offset<4>(0.00), 0);

    TEST_EXPECT_EQUAL(fixed_range_offset<3>(1.0),  2);
    TEST_EXPECT_EQUAL(fixed_range_offset<3>(0.67), 2);
    TEST_EXPECT_EQUAL(fixed_range_offset<3>(0.66), 1);
    TEST_EXPECT_EQUAL(fixed_range_offset<3>(0.34), 1);
    TEST_EXPECT_EQUAL(fixed_range_offset<3>(0.33), 0);
    TEST_EXPECT_EQUAL(fixed_range_offset<3>(0.0),  0);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


class NoTuple: public ValueTuple {
public:
    NoTuple() {}
    ~NoTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return false; }
    ShadedValue clone() const OVERRIDE { return new NoTuple; }
    int range_offset(const Phaser&) const OVERRIDE { is_assert(0); return -9999999; } // defines no range offset

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE { return "<undef>"; }
#endif

    // mixer:
    ShadedValue reverse_mix(float, const LinearTuple& other) const OVERRIDE;
    ShadedValue reverse_mix(float, const PlanarTuple& other) const OVERRIDE;
    ShadedValue reverse_mix(float, const SpatialTuple& other) const OVERRIDE;
    ShadedValue mix(float, const ValueTuple& other) const OVERRIDE { return other.clone(); }
};

class LinearTuple: public ValueTuple {
    float val;

public:
    LinearTuple(float val_) : val(val_) {
        is_assert(!is_nan_or_inf(val));
    }
    ~LinearTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return true; }
    ShadedValue clone() const OVERRIDE { return new LinearTuple(val); }
    int range_offset(const Phaser& phaser) const OVERRIDE {  // returns int-offset into range [0 .. AW_RANGE_COLORS[
        return CHECKED_RANGE_OFFSET(fixed_range_offset<AW_RANGE_COLORS>(phaser.rephase(val)));
    }

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE {
        static SmartCharPtr buf;
        buf = GBS_global_string_copy("(%.3f)", val);
        return &*buf;
    }
#endif

    // mixer:
    ShadedValue reverse_mix(float other_ratio, const LinearTuple& other) const OVERRIDE {
        return new LinearTuple(other_ratio*other.val + (1-other_ratio)*val);
    }
    ShadedValue mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
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
    ShadedValue clone() const OVERRIDE { return new PlanarTuple(val1, val2); }
    int range_offset(const Phaser& phaser) const OVERRIDE { // returns int-offset into range [0 .. AW_RANGE_COLORS[
        int c1 = is_nan(val1) ? 0 : fixed_range_offset<AW_PLANAR_COLORS>(phaser.rephase(val1));
        int c2 = is_nan(val2) ? 0 : fixed_range_offset<AW_PLANAR_COLORS>(phaser.rephase(val2));
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
    ShadedValue reverse_mix(float other_ratio, const PlanarTuple& other) const OVERRIDE {
        return new PlanarTuple(mix_floats(other.val1, other_ratio, val1),
                               mix_floats(other.val2, other_ratio, val2));
    }
    ShadedValue mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
};

class SpatialTuple: public ValueTuple {
    float val1, val2, val3;

public:
    SpatialTuple(float val1_, float val2_, float val3_) :
        val1(val1_),
        val2(val2_),
        val3(val3_)
    {
        is_assert(!is_inf(val1));
        is_assert(!is_inf(val2));
        is_assert(!is_inf(val3));
        is_assert(!(is_nan(val1) && is_nan(val2) && is_nan(val3))); // only NAN is unwanted
    }
    ~SpatialTuple() OVERRIDE {}

    bool is_defined() const OVERRIDE { return true; }
    ShadedValue clone() const OVERRIDE { return new SpatialTuple(val1, val2, val3); }
    int range_offset(const Phaser& phaser) const OVERRIDE { // returns int-offset into range [0 .. AW_RANGE_COLORS[
        int c1 = is_nan(val1) ? 0 : fixed_range_offset<AW_SPATIAL_COLORS>(phaser.rephase(val1));
        int c2 = is_nan(val2) ? 0 : fixed_range_offset<AW_SPATIAL_COLORS>(phaser.rephase(val2));
        int c3 = is_nan(val3) ? 0 : fixed_range_offset<AW_SPATIAL_COLORS>(phaser.rephase(val3));
        return CHECKED_RANGE_OFFSET((c1*AW_SPATIAL_COLORS + c2)*AW_SPATIAL_COLORS + c3);
    }

#if defined(UNIT_TESTS)
    const char *inspect() const OVERRIDE {
        static SmartCharPtr buf;
        buf = GBS_global_string_copy("(%.3f,%.3f,%.3f)", val1, val2, val3);
        return &*buf;
    }
#endif

    // mixer:
    ShadedValue reverse_mix(float other_ratio, const SpatialTuple& other) const OVERRIDE {
        return new SpatialTuple(mix_floats(other.val1, other_ratio, val1),
                                mix_floats(other.val2, other_ratio, val2),
                                mix_floats(other.val3, other_ratio, val3));
    }
    ShadedValue mix(float my_ratio, const ValueTuple& other) const OVERRIDE { return other.reverse_mix(my_ratio, *this); }
};


// ---------------------------------
//      mixer (late definition)

ShadedValue NoTuple::reverse_mix(float, const LinearTuple&  other) const { return other.clone(); }
ShadedValue NoTuple::reverse_mix(float, const PlanarTuple&  other) const { return other.clone(); }
ShadedValue NoTuple::reverse_mix(float, const SpatialTuple& other) const { return other.clone(); }

// -----------------
//      factory

ShadedValue ValueTuple::undefined() {
    return new NoTuple;
}
ShadedValue ValueTuple::make(float f) {
    return is_nan(f) ? undefined() : new LinearTuple(f);
}
ShadedValue ValueTuple::make(float f1, float f2) {
    return (is_nan(f1) && is_nan(f2)) ? undefined() : new PlanarTuple(f1, f2);
}
ShadedValue ValueTuple::make(float f1, float f2, float f3) {
    return (is_nan(f1) && is_nan(f2) && is_nan(f3)) ? undefined() : new SpatialTuple(f1, f2, f3);
}

