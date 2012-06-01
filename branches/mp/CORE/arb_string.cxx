// ================================================================ //
//                                                                  //
//   File      : arb_string.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "arb_string.h"

#include <arb_assert.h>

#include <cstring>
#include <cstdlib>


char *GB_strduplen(const char *p, unsigned len) {
    // fast replacement for strdup, if len is known
    if (p) {
        char *neu;

        arb_assert(strlen(p) == len);
        // Note: Common reason for failure: a zero-char was manually printed by a GBS_global_string...-function

        neu = (char*)malloc(len+1);
        memcpy(neu, p, len+1);
        return neu;
    }
    return 0;
}

char *GB_strpartdup(const char *start, const char *end) {
    /* strdup of a part of a string (including 'start' and 'end')
     * 'end' may point behind end of string -> copy only till zero byte
     * if 'end'=('start'-1) -> return ""
     * if 'end'<('start'-1) -> return 0
     * if 'end' == NULL -> copy whole string
     */

    char *result;
    if (end) {
        int len = end-start+1;

        if (len >= 0) {
            const char *eos = (const char *)memchr(start, 0, len);

            if (eos) len = eos-start;
            result = (char*)malloc(len+1);
            memcpy(result, start, len);
            result[len] = 0;
        }
        else {
            result = 0;
        }
    }
    else { // end = 0 -> return copy of complete string
        result = nulldup(start);
    }

    return result;
}

char *GB_strndup(const char *start, int len) {
    return GB_strpartdup(start, start+len-1);
}


// --------------------------------------------------------------------------------


#ifdef UNIT_TESTS

#include <string>
#include <climits>

#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

using namespace std;

// ----------------------------------------------
//      some tests for unit-test-code itself

#define TEST_ASSERT_HEAPCOPY_EQUAL(copy,expected) do {  \
        char *theCopy = (copy);                         \
        TEST_ASSERT_EQUAL(theCopy, expected);           \
        free(theCopy);                                  \
    } while(0)

void TEST_arbtest_strf() {
    // tests string formatter from test_unit.h
    using namespace arb_test;
    TEST_ASSERT_HEAPCOPY_EQUAL(StaticCode::strf("<%i>", 7), "<7>");
    TEST_ASSERT_HEAPCOPY_EQUAL(StaticCode::strf("<%0*i>", 3, 7), "<007>");
}

void TEST_arbtest_readable() {
    using namespace arb_test;

    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy('x')), "'x'");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(static_cast<unsigned char>('x'))), "'x'");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(static_cast<signed char>('x'))), "'x'");

    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(true)), "true");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(false)), "false");
    
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(1)), "1");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2hex(make_copy(2)), "0x2");

    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(3L)), "3");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2hex(make_copy(4L)), "0x4");

    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(5U)), "5");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2hex(make_copy(6U)), "0x6");
    
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy("some\ntext\twhich\"special\\chars")), "\"some\\ntext\\twhich\\\"special\\\\chars\"");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy("a\1\2\x1a\x7e\x7f\x80\xfe\xff")), "\"a\\1\\2\\x1a~\\x7f\\x80\\xfe\\xff\"");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy((const char *)NULL)), "(null)");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy((const unsigned char *)NULL)), "(null)");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy((const signed char *)NULL)), "(null)");

    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(1.7)), "1.700000");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(177.0e20)),  "17699999999999998951424.000000");
    TEST_ASSERT_HEAPCOPY_EQUAL(val2readable(make_copy(177.0e20F)), "17699999967695435988992.000000");
}

void TEST_arbtest_copyable() {
    using namespace arb_test;

    int         i = 7;
    const char *s = "servas";

    TEST_ASSERT(make_copy(i) == make_copy(7));

    TEST_ASSERT(strcmp(make_copy(s), make_copy("servas")) == 0);
}

#define TEST_DESCRIPTIONS(d, tt, tf, ft, ff) do {               \
        TEST_ASSERT_EQUAL((d).make(true, true), (tt));   \
        TEST_ASSERT_EQUAL((d).make(true, false), (tf));  \
        TEST_ASSERT_EQUAL((d).make(false, true), (ft));  \
        TEST_ASSERT_EQUAL((d).make(false, false), (ff)); \
    } while(0)

#define TEST_SIMPLE_DESCRIPTIONS(d, ae, nae) TEST_DESCRIPTIONS(d, ae, nae, nae, ae)

void TEST_arbtest_predicate_description() {
    TEST_SIMPLE_DESCRIPTIONS(predicate_description("similar"), "is similar", "isnt similar");
    TEST_SIMPLE_DESCRIPTIONS(predicate_description("repairs"), "repairs", "doesnt repair");

    TEST_DESCRIPTIONS(predicate_description("equals", "differs"),
                      "equals", "doesnt equal",
                      "doesnt differ", "differs");

    TEST_DESCRIPTIONS(predicate_description("less_than", "more_than"),
                      "is less_than", "isnt less_than",
                      "isnt more_than", "is more_than");
}

void TEST_arbtest_expectations() {
    // used to TDD expectations
    using namespace arb_test;

    string apple       = "Apfel";
    string pear        = "Birne";
    string boskop      = apple;
    string pomegranate = "Granatapfel";

    TEST_EXPECT(that(apple).is_equal_to("Apfel"));
    
    TEST_EXPECT(that(apple).does_differ_from(pear));
    TEST_EXPECT(that(apple).is_equal_to(boskop));
    TEST_EXPECT(wrong(that(pomegranate).is_equal_to(apple)));

    match_expectation ff1 = that(1.0).is_equal_to(2-1);
    match_expectation ff2 = that(boskop).is_equal_to(apple);
    match_expectation ff3 = that(apple).is_equal_to(apple);

    match_expectation nf1 = that(apple).is_equal_to(pear);
    match_expectation nf2 = that(pomegranate).is_equal_to(apple);
    match_expectation nf3 = that(apple).does_differ_from(boskop);

    match_expectation a1 = all().of(ff1);
    match_expectation a2 = all().of(ff1, ff2);
    match_expectation a3 = all().of(ff1, ff2, ff3);

    TEST_EXPECT(a1);
    TEST_EXPECT(a2);
    TEST_EXPECT(a3);

    match_expectation n1 = none().of(ff1);
    match_expectation n2 = none().of(ff1, ff2);
    match_expectation n3 = none().of(ff1, ff2, ff3);

    TEST_EXPECT(wrong(none().of(that(boskop).is_equal_to(apple))));
    TEST_EXPECT(wrong(n1));
    TEST_EXPECT(wrong(n2));
    TEST_EXPECT(wrong(n3));

    TEST_EXPECT(atleast(1).of(a1));
    TEST_EXPECT(atleast(1).of(a1, n1));
    TEST_EXPECT(atleast(1).of(n2, a1, n1));

    TEST_EXPECT(wrong(atleast(2).of(a1, n1, n2)));
    TEST_EXPECT(wrong(atleast(2).of(a1, n1)));
    TEST_EXPECT(wrong(atleast(2).of(a1))); // impossible

    TEST_EXPECT(atmost(2).of(a1));
    TEST_EXPECT(atmost(2).of(a1, a2));
    TEST_EXPECT(atmost(2).of(a1, a2, n1));
    TEST_EXPECT(atmost(2).of(a1, n1, n2));
    TEST_EXPECT(atmost(2).of(n1, n2));
    TEST_EXPECT(wrong(atmost(2).of(a1, a2, a3)));

    TEST_EXPECT(exactly(1).of(ff1, nf1, nf2));
    TEST_EXPECT(wrong(exactly(1).of(nf1, nf2)));
    TEST_EXPECT(wrong(exactly(1).of(nf1, nf2, nf3)));
    TEST_EXPECT(wrong(exactly(1).of(ff1, ff2, nf2)));
    TEST_EXPECT(wrong(exactly(1).of(ff1, ff2, ff3)));

}

void TEST_expectation_groups() {
    using namespace arb_test;

    expectation_group no_expectations;
    TEST_EXPECT(all().ofgroup(no_expectations));
    TEST_EXPECT(none().ofgroup(no_expectations));

    expectation_group fulfilled_expectation  (that(1).is_equal_to(1));
    expectation_group unfulfilled_expectation(that(1).is_equal_to(0));
    expectation_group some_fulfilled_expectations(that(1).is_equal_to(0), that(1).is_equal_to(1));

    TEST_EXPECT(all().ofgroup(fulfilled_expectation));
    TEST_EXPECT(none().ofgroup(unfulfilled_expectation));

    TEST_ASSERT(none().ofgroup(fulfilled_expectation).unfulfilled());
    TEST_ASSERT(all().ofgroup(unfulfilled_expectation).unfulfilled());
    
    TEST_ASSERT(all().ofgroup(some_fulfilled_expectations).unfulfilled());
    TEST_ASSERT(none().ofgroup(some_fulfilled_expectations).unfulfilled());
}

void TEST_replace_old_TEST_ASSERTS_by_expectations() {
    // test various string-types are matchable (w/o casts)
    {
        const char *car_ccp = "Alfa";
        char       *car_cp  = strdup("Alfa");
        string      car_str("Alfa");

        TEST_ASSERT_EQUAL(car_ccp, "Alfa");
        TEST_ASSERT_EQUAL(car_cp, "Alfa");
        TEST_ASSERT_EQUAL(car_str, "Alfa");

        TEST_ASSERT_EQUAL("Alfa", car_ccp);
        TEST_ASSERT_EQUAL("Alfa", car_cp);
        TEST_ASSERT_EQUAL("Alfa", car_str);

        TEST_ASSERT_EQUAL(car_cp, car_ccp);
        TEST_ASSERT_EQUAL(car_cp, car_str);
        TEST_ASSERT_EQUAL(car_ccp, car_cp);
        TEST_ASSERT_EQUAL(car_ccp, car_str);
        TEST_ASSERT_EQUAL(car_str, car_cp);
        TEST_ASSERT_EQUAL(car_str, car_ccp);

        char *null = NULL;
        TEST_ASSERT_NULL((void*)NULL);
        TEST_ASSERT_NULL(null);

        TEST_ASSERT_CONTAINS(car_ccp, "lf");
        TEST_ASSERT_CONTAINS(car_cp, "fa");
        TEST_ASSERT_CONTAINS(car_str, "Al");

        free(car_cp);
    }

    // test various numeric types are matchable

    {
        short unsigned su = 7;
        short          s  = -su;

        unsigned iu = su;
        int      i  = -iu;

        long unsigned lu = (long unsigned)INT_MAX+3;
        long          l  = -lu;

        float  f = s;
        double d = i;

        TEST_ASSERT_EQUAL(s, -7);
        TEST_ASSERT_EQUAL(i, -7);

        TEST_ASSERT_EQUAL(su, 7);  TEST_ASSERT_EQUAL(iu, 7);
        TEST_ASSERT_EQUAL(su, 7U); TEST_ASSERT_EQUAL(iu, 7U);
        TEST_ASSERT_EQUAL(su, 7L); TEST_ASSERT_EQUAL(iu, 7L);

        TEST_ASSERT_EQUAL(s, -su); TEST_ASSERT_EQUAL(s, -iu);
        TEST_ASSERT_EQUAL(i, -iu); TEST_ASSERT_EQUAL(i, -su);
        TEST_ASSERT_EQUAL(l, -lu);

        TEST_ASSERT_EQUAL(f, d);
        TEST_ASSERT_EQUAL(d, f);
    }

    TEST_ASSERT_ZERO(7-7);
}

// --- simulate user_type (which may be defined anywhere) ---
class user_type {
    int x, y;
public:
    user_type(int X, int Y) : x(X), y(Y) {}

    int get_x() const { return x; }
    int get_y() const { return y; }

    user_type flipped() const { return user_type(y,x); }

    int quadrant() const {
        if (x == 0 || y == 0) return 0; // on axis
        if (y>0) return x<0 ? 2 : 1;
        return x<0 ? 3 : 4;
    }
};
// --- end of user_type ---

// helpers needed for tests:
inline bool operator == (const user_type& u1, const user_type& u2) { return u1.get_x() == u2.get_x() && u1.get_y() == u2.get_y(); }
inline char *val2readable(const user_type& u) { return arb_test::StaticCode::strf("user_type(%i,%i)", u.get_x(), u.get_y()); }
inline bool in_same_quadrant(const user_type& u1, const user_type& u2) { return u1.quadrant() == u2.quadrant(); }

void TEST_user_type_with_expectations() {
    user_type ut1(3, 4);
    user_type ut12(4, 4);
    user_type ut2(-4, 4);
    user_type ut3(-4, -8);
    user_type ut4(4, -8);

    TEST_EXPECT(that(ut1).does_differ_from(ut12));
    TEST_EXPECT(that(ut12).is_equal_to(ut12.flipped()));
    TEST_EXPECT(that(ut1).does_differ_from(ut1.flipped()));

    TEST_EXPECT(that(ut1).is(in_same_quadrant, ut12));
    TEST_EXPECT(none().of(that(ut1).is(in_same_quadrant, ut2),
                          that(ut2).is(in_same_quadrant, ut3),
                          that(ut3).is(in_same_quadrant, ut4)));
}

void TEST_similarity() {
    double d1      = 0.7531;
    double epsilon = 0.00001;
    double d2      = d1-epsilon*0.6;
    double d3      = d1+epsilon*0.6;

    TEST_EXPECT(that(d1).is(epsilon_similar(epsilon), d2));
    TEST_EXPECT(that(d1).is(epsilon_similar(epsilon), d3));
    TEST_EXPECT(that(d2).is_not(epsilon_similar(epsilon), d3));

    TEST_ASSERT_SIMILAR(d1, d2, epsilon);
    TEST_ASSERT_SIMILAR(d1, d3, epsilon);
}

void TEST_less_equal() {
    int x = 7;
    int y = 8;
    int z = 9;

    // less/more etc

    TEST_EXPECT(that(x).less_than(y));
    TEST_EXPECT(that(x).less_or_equal(y));
    TEST_EXPECT(that(x).less_or_equal(x));
    
    TEST_EXPECT(that(y).more_than(x));
    TEST_EXPECT(that(y).more_or_equal(x));
    TEST_EXPECT(that(y).more_or_equal(y));

    TEST_ASSERT_LOWER_EQUAL(x, y);
    TEST_ASSERT_LOWER_EQUAL(x, x);
    TEST_ASSERT_LOWER(x, y);
    TEST_ASSERT_IN_RANGE(y, x, z);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


