// ============================================================== //
//                                                                //
//   File      : cb.cxx                                           //
//   Purpose   : currently only a test sandbox                    //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2011   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include <cb.h>
#include <string>
#include <stdint.h>

using namespace std;

STATIC_ASSERT(sizeof(int*) == sizeof(AW_CL)); // important for casted db-callback type (GB_CB vs GB_CB_wanted..)

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

// ---------------------------------------------
//      compile time test of type inspection

#define COMPILE_ASSERT_POINTER_TO(ISTYPE,POINTER)               \
    STATIC_ASSERT(TypeT<POINTER>::IsPtrT &&                    \
                   TypeT<CompountT<POINTER>::BaseT>::ISTYPE)

typedef int (*somefun)(const char *);
int myfun(const char *) { return 0; }
static somefun sfun = myfun;

enum someenum { A, B };

class someclass { public : int memfun() { return -1; } };


STATIC_ASSERT(IsFundaT<void>::No);
STATIC_ASSERT(IsFundaT<bool>::Yes);
STATIC_ASSERT(IsFundaT<int>::Yes);
STATIC_ASSERT(IsFundaT<long>::Yes);
STATIC_ASSERT(IsFundaT<double>::Yes);

STATIC_ASSERT(IsFundaT<int*>::No);
COMPILE_ASSERT_POINTER_TO(IsFundaT, int*);
COMPILE_ASSERT_POINTER_TO(IsPtrT, int**);
STATIC_ASSERT(IsFundaT<int&>::No);

STATIC_ASSERT(IsFundaT<somefun>::No);
STATIC_ASSERT(IsFundaT<typeof(myfun)>::No);
STATIC_ASSERT(IsFundaT<someenum>::No);

STATIC_ASSERT(IsFunctionT<typeof(myfun)>::Yes);

STATIC_ASSERT(IsFunctionT<somefun>::No); // somefun is pointer to functiontype
COMPILE_ASSERT_POINTER_TO(IsFuncT, somefun);
STATIC_ASSERT(IsFunctionT<typeof(sfun)>::No); // sfun is pointer to functiontype
COMPILE_ASSERT_POINTER_TO(IsFuncT, typeof(sfun));
STATIC_ASSERT(IsFunctionT<void>::No);
STATIC_ASSERT(IsFunctionT<int>::No);
STATIC_ASSERT(IsFunctionT<bool>::No);

STATIC_ASSERT(CompountT<const int&>::IsRefT);
STATIC_ASSERT(CompountT<int&>::IsRefT);

STATIC_ASSERT(IsEnumT<someenum>::Yes);
COMPILE_ASSERT_POINTER_TO(IsEnumT, someenum*);

STATIC_ASSERT(IsEnumT<int>::No);
STATIC_ASSERT(IsEnumT<int*>::No);
STATIC_ASSERT(IsEnumT<int&>::No);
STATIC_ASSERT(IsEnumT<somefun>::No);
STATIC_ASSERT(IsEnumT<typeof(myfun)>::No);

STATIC_ASSERT(IsClassT<someclass>::Yes);
COMPILE_ASSERT_POINTER_TO(IsClassT, someclass*);

STATIC_ASSERT(IsClassT<int>::No);
STATIC_ASSERT(IsClassT<int*>::No);
STATIC_ASSERT(IsClassT<int&>::No);
STATIC_ASSERT(IsClassT<someenum>::No);
STATIC_ASSERT(IsClassT<somefun>::No);
STATIC_ASSERT(IsClassT<typeof(myfun)>::No);

STATIC_ASSERT(TypeT<int>::IsFundaT);
STATIC_ASSERT(TypeT<int*>::IsPtrT);
STATIC_ASSERT(TypeT<int&>::IsRefT);
STATIC_ASSERT(TypeT<int[]>::IsArrayT);
STATIC_ASSERT(TypeT<int[7]>::IsArrayT);
STATIC_ASSERT(TypeT<typeof(myfun)>::IsFuncT);
STATIC_ASSERT(TypeT<typeof(&someclass::memfun)>::IsPtrMemT);
STATIC_ASSERT(TypeT<someenum>::IsEnumT);
STATIC_ASSERT(TypeT<someclass>::IsClassT);

// -----------------------
//      test callbacks

#define TRACE

static uint32_t traceChecksum;
const int            BUFFERSIZE = 100;
char                 traceBuffer[BUFFERSIZE];

__ATTR__FORMAT(1) static void tracef(const char *format, ...) {
    va_list parg;
    va_start(parg, format);
    int printed = vsnprintf(traceBuffer, BUFFERSIZE, format, parg);
    va_end(parg);

#if defined(TRACE)
    fputs(traceBuffer, stdout);
#endif
    if (printed >= BUFFERSIZE) {
        printf("\nprinted=%i\n", printed);
    }
    TEST_EXPECT(printed<BUFFERSIZE);

    traceChecksum = 0;
    for (int p = 0; p<printed; ++p) {
        traceChecksum = (traceChecksum<<1)^(traceChecksum>>31)^traceBuffer[p];
    }
}

static AW_root    *fake_root   = (AW_root*)1;
static AW_window  *fake_win    = (AW_window*)2;
static GBDATA     *fake_gbd    = (GBDATA*)3;
static GB_CB_TYPE  fake_gbtype = GB_CB_CHANGED;

static void rcb0(AW_root *r) {
    TEST_EXPECT(r == fake_root);
    tracef("rcb0()\n");
}
static void rcb1(AW_root *r, const char *name) {
    TEST_EXPECT(r == fake_root);
    tracef("rcb1(%s)\n", name);
}
static void rcb2(AW_root *r, const char *name, int val) {
    TEST_EXPECT(r == fake_root);
    tracef("rcb2(%s=%i) [int]\n", name, val);
}
static void rcb2(AW_root *r, const char *name, long val) {
    TEST_EXPECT(r == fake_root);
    tracef("rcb2(%s=%li) [long]\n", name, val);
}

static void wcb0(AW_window *w) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb0()\n");
}
static void wcb1(AW_window *w, const char *name) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb1(%s)\n", name);
}
static void wcb1(AW_window *w, string *name) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb1(%s) [string]\n", name->c_str());
}
static void wcb2(AW_window *w, const char *name, const char *val) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb2(%s=%s) [const char/const char]\n", name, val);
}
static void wcb2(AW_window *w, const string *name, const string *val) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb2(%s=%s) [string/string]\n", name->c_str(), val->c_str());
}
static void wcb2(AW_window *w, const char *name, int val) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb2(%s=%i) [int]\n", name, val);
}
static void wcb2(AW_window *w, const char *name, long val) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb2(%s=%li) [long]\n", name, val);
}
static void wcb2(AW_window *w, char c, long long val) {
    TEST_EXPECT(w == fake_win);
    tracef("wcb2(%c=%lli) [long long]\n", c, val);
}

static AW_window *wccb0(AW_root *r) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb0()\n");
    return fake_win;
}
static AW_window *wccb1(AW_root *r, int x) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%i)\n", x);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const long *lp) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%li) [long ptr]\n", *lp);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const int *x) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%i) [const ptr]\n", *x);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, int *x) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%i) [mutable ptr]\n", *x);
    *x *= 2; // modify callback argument!
    return fake_win;
}
static AW_window *wccb1(AW_root *r, char* s) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%s) [mutable]\n", s);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const char* s) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb1(%s) [const]\n", s);
    return fake_win;
}
static AW_window *wccb2(AW_root *r, const char *s1, const char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb2(%s=%s) [const const]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb2(AW_root *r, const char *s1, char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb2(%s=%s) [const mutable]\n", s1, s2);
    return fake_win;
}

static void dbcb01(GBDATA *gbd) {
    TEST_EXPECT(gbd == fake_gbd);
    tracef("dbcb01()\n");
}
static void dbcb012(GBDATA *gbd, GB_CB_TYPE t) {
    TEST_EXPECT(gbd == fake_gbd && t == fake_gbtype);
    tracef("dbcb012()\n");
}
static void dbcb02(GB_CB_TYPE t) {
    tracef("dbcb02(%i)\n", int(t));
}
static void dbcb1(GBDATA *gbd, int x, GB_CB_TYPE t) {
    TEST_EXPECT(gbd == fake_gbd && t == fake_gbtype);
    tracef("dbcb1(%i) [int]\n", x);
}
static void dbcb1(GBDATA *gbd, const char *n, GB_CB_TYPE t) {
    TEST_EXPECT(gbd == fake_gbd && t == fake_gbtype);
    tracef("dbcb1(%s) [const char]\n", n);
}

inline void call(const RootCallback& rcb) { rcb(fake_root); }
inline void call(const WindowCallback& wcb) { wcb(fake_win); }
inline void call(const WindowCreatorCallback& wccb) { TEST_EXPECT(wccb(fake_root) == fake_win); }
inline void call(const DatabaseCallback& dbcb) { dbcb(fake_gbd, fake_gbtype); }

#define TEST_CB(cb,expectedChecksum) do {                               \
        traceChecksum = -666;                                           \
        call(cb);                                                       \
        TEST_EXPECT_EQUAL(traceChecksum, (uint32_t)expectedChecksum);   \
    } while(0)

#define TEST_CB_TRACE(cb,expectedOutput) do {           \
        call(cb);                                       \
        TEST_EXPECT_EQUAL(traceBuffer, expectedOutput); \
    } while(0)

#define TEST_CB_TRACE__BROKEN(cb,expectedOutput) do {           \
        call(cb);                                               \
        TEST_EXPECT_EQUAL__BROKEN(traceBuffer, expectedOutput); \
    } while(0)


static void freeCharp(char *str) { free(str); }
static void freeCharp(char *str1, char *str2) { free(str1); free(str2); }
static void deleteString(string *str) { delete str; }
static void deleteString(string *str1, string *str2) { delete str1; delete str2; }

void TEST_cbs() {
    MISSING_TEST("please log");
    // for (int i = 0; i<100000; ++i)
    {
        TEST_CB(makeRootCallback(rcb0),                0x17b8);
        TEST_CB(makeRootCallback(rcb1,  "dispatched"), 0x5d9780);
        TEST_CB(makeRootCallback(rcb2, "age",  46),    0x176c160);
        TEST_CB(makeRootCallback(rcb2, "size", 178L),  0xbaf72ec);

        TEST_CB(makeWindowCallback(wcb0),               0x16f8);
        TEST_CB(makeWindowCallback(wcb1, "dispatched"), 0x589780);
        TEST_CB(makeWindowCallback(wcb2, "age",  46),   0x162c160);
        TEST_CB(makeWindowCallback(wcb2, "size", 178L), 0xb0f72ec);

#if defined(ARB_64)
        TEST_CB(makeWindowCallback(wcb2, 'l', 0xb09bbc04b09bbcLL),  0xc830c18e);
#endif

        TEST_CB(makeWindowCreatorCallback(wccb0),     0x2878);
        TEST_CB(makeWindowCreatorCallback(wccb1, 77), 0xa19c);

        TEST_CB(makeDatabaseCallback(dbcb012), 0x8778);
        TEST_CB(makeDatabaseCallback(dbcb02), 0x87f0);                // call dbcb02 with fixed argument (i.e. with argument normally passed by ARBDB)
        TEST_CB(makeDatabaseCallback(dbcb1, 77), 0x21a260);
        TEST_CB(makeDatabaseCallback(dbcb1, freeCharp, strdup("test")), 0x428ac190);

        // callbacks with deallocator

        // TEST_CB(makeWindowCallback(wcb1, strdup("leak")), 0x163ac); // leaks
        TEST_CB(makeWindowCallback(wcb1, freeCharp, strdup("leak")), 0x163ac);
        // TEST_CB(makeWindowCallback(wcb1, freeCharp, "leak"), 0x163ac); // does not compile (can't call freeCharp with const char *)
        TEST_CB(makeWindowCallback(wcb1, deleteString, new string("leak")), 0x2c7790c);
        // TEST_CB(makeWindowCallback(wcb2, strdup("hue"), strdup("hott")), 0xe09d7b1); // leaks
        TEST_CB(makeWindowCallback(wcb2, freeCharp, strdup("hue"), strdup("hott")), 0xe09d7b1);
        TEST_CB(makeWindowCallback(wcb2, deleteString, new string("hue"), new string("hott")), 0x2108df1a);

        // test const vs non-const pointers:
        char       *mut = strdup("mut");
        const char *con = "con";

        TEST_CB(makeWindowCreatorCallback(wccb2, con, con), 0x1507833d);  // exact match
        TEST_CB(makeWindowCreatorCallback(wccb2, mut, con), 0x1467833d);  // fallback to const/const
        TEST_CB(makeWindowCreatorCallback(wccb2, con, mut), 0x54460ad0); // exact match
        TEST_CB(makeWindowCreatorCallback(wccb2, mut, mut), 0x1471833d);  // fallback to const const

        // test reference arguments
        int       imut = 17;
        const int icon = 23;

        const long lcon = 775L;

        TEST_CB(makeWindowCreatorCallback(wccb1, imut), 0xa1ac);
        TEST_CB(makeWindowCreatorCallback(wccb1, icon), 0xa1a4);
        TEST_CB(makeWindowCreatorCallback(wccb1, &lcon), 0xa188418);

        TEST_CB(makeWindowCreatorCallback(wccb1, &icon), 0xa1b2158);
        TEST_CB(makeWindowCreatorCallback(wccb1, &icon), 0xa1b2158);

        TEST_CB(makeWindowCreatorCallback(wccb1, &imut), 0x286ec698); // modifies 'imut'
        TEST_CB(makeWindowCreatorCallback(wccb1, &imut), 0x2869c698); // modifies 'imut'
        TEST_CB(makeWindowCreatorCallback(wccb1, &imut), 0x286fc698); // modifies 'imut'

        TEST_CB(makeWindowCreatorCallback(wccb1, mut),  0x5169cc4);
        TEST_CB(makeWindowCreatorCallback(wccb1, con),  0x145feb8);

        free(mut);
    }

}

#endif // UNIT_TESTS


