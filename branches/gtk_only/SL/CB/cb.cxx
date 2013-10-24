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
    STATIC_ASSERT(TypeT<POINTER>::IsPtrT &&                     \
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


static void ucb0(UNFIXED) {
    tracef("ucb0()\n");
}
static void ucb1(UNFIXED, const char *name) {
    tracef("ucb1(%s)\n", name);
}
static void ucb2(UNFIXED, const char *name, int val) {
    tracef("ucb2(%s=%i) [int]\n", name, val);
}
static void ucb2(UNFIXED, const char *name, long val) {
    tracef("ucb2(%s=%li) [long]\n", name, val);
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
    tracef("wccb2(%s,%s) [const const]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb2(AW_root *r, const char *s1, char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb2(%s,%s) [const mutable]\n", s1, s2);
    return fake_win;
}

static AW_window *wccb3(AW_root *r, const char *s1, const char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb3(%s,%s) [const const]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb3(AW_root *r, const char *s1, char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb3(%s,%s) [const mutable]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb3(AW_root *r, char *s1, const char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb3(%s,%s) [mutable const]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb3(AW_root *r, char *s1, char *s2) {
    TEST_EXPECT(r == fake_root);
    tracef("wccb3(%s,%s) [mutable mutable]\n", s1, s2);
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

static void plaincb() {
    tracef("plaincb()\n");
}
static AW_window *wccb_plain() {
    tracef("wccb_plain()\n");
    return fake_win;
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
    // MISSING_TEST("please log");
    // for (int i = 0; i<100000; ++i)
    {
        TEST_CB_TRACE(makeRootCallback(plaincb),             "plaincb()\n");
        TEST_CB_TRACE(makeRootCallback(rcb0),                "rcb0()\n");
        TEST_CB_TRACE(makeRootCallback(rcb1,  "dispatched"), "rcb1(dispatched)\n");
        TEST_CB_TRACE(makeRootCallback(rcb2, "age",  46),    "rcb2(age=46) [int]\n");
        TEST_CB_TRACE(makeRootCallback(rcb2, "size", 178L),  "rcb2(size=178) [long]\n");

        TEST_CB_TRACE(makeWindowCallback(plaincb),            "plaincb()\n");
        TEST_CB_TRACE(makeWindowCallback(wcb0),               "wcb0()\n");
        TEST_CB_TRACE(makeWindowCallback(wcb1, "dispatched"), "wcb1(dispatched)\n");
        TEST_CB_TRACE(makeWindowCallback(wcb2, "age",  46),   "wcb2(age=46) [int]\n");
        TEST_CB_TRACE(makeWindowCallback(wcb2, "size", 178L), "wcb2(size=178) [long]\n");

        // declaring a cb with UNFIXED as fixed-argument allows to use callbacks as RootCallback AND as WindowCallback
        // (when we use sigc++ in the future this should be changed to allowing functions w/o the UNFIXED-parameter)

        TEST_CB_TRACE(makeRootCallback(ucb0),                "ucb0()\n");
        TEST_CB_TRACE(makeRootCallback(ucb1,  "dispatched"), "ucb1(dispatched)\n");
        TEST_CB_TRACE(makeRootCallback(ucb2, "age",  46),    "ucb2(age=46) [int]\n");
        TEST_CB_TRACE(makeRootCallback(ucb2, "size", 178L),  "ucb2(size=178) [long]\n");

        TEST_CB_TRACE(makeWindowCallback(ucb0),                "ucb0()\n");
        TEST_CB_TRACE(makeWindowCallback(ucb1,  "dispatched"), "ucb1(dispatched)\n");
        TEST_CB_TRACE(makeWindowCallback(ucb2, "age",  46),    "ucb2(age=46) [int]\n");
        TEST_CB_TRACE(makeWindowCallback(ucb2, "size", 178L),  "ucb2(size=178) [long]\n");

#if defined(ARB_64)
        TEST_CB_TRACE(makeWindowCallback(wcb2, 'l', 49710827735915452LL),  "wcb2(l=49710827735915452) [long long]\n");
#endif

        TEST_CB_TRACE(makeWindowCreatorCallback(wccb_plain), "wccb_plain()\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb0),      "wccb0()\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, 77),  "wccb1(77)\n");

        // TEST_CB(makeDatabaseCallback(plaincb), 44760); // does not work (ambiguous due to 2 fixed arguments of DatabaseCallback)
        TEST_CB_TRACE(makeDatabaseCallback(dbcb01),    "dbcb01()\n");
        TEST_CB_TRACE(makeDatabaseCallback(dbcb012),   "dbcb012()\n");
        TEST_CB_TRACE(makeDatabaseCallback(dbcb02),    "dbcb02(2)\n"); // call dbcb02 with fixed argument (i.e. with argument normally passed by ARBDB)
        TEST_CB_TRACE(makeDatabaseCallback(dbcb1, 77), "dbcb1(77) [int]\n");

        TEST_CB_TRACE(makeDatabaseCallback(dbcb1, freeCharp, strdup("test")), "dbcb1(test) [const char]\n");

        // callbacks with deallocator

        // TEST_CB(makeWindowCallback(wcb1, strdup("leak")), 0x163ac); // leaks
        TEST_CB_TRACE(makeWindowCallback(wcb1, freeCharp, strdup("leak")), "wcb1(leak)\n");
        // TEST_CB(makeWindowCallback(wcb1, freeCharp, "leak"), 0x163ac); // does not compile (can't call freeCharp with const char *)
        TEST_CB_TRACE(makeWindowCallback(wcb1, deleteString, new string("leak")), "wcb1(leak) [string]\n");
        // TEST_CB(makeWindowCallback(wcb2, strdup("hue"), strdup("hott")), 0xe09d7b1); // leaks
        TEST_CB_TRACE(makeWindowCallback(wcb2, freeCharp, strdup("hue"), strdup("hott")), "wcb2(hue=hott) [const char/const char]\n");
        TEST_CB_TRACE(makeWindowCallback(wcb2, deleteString, new string("hue"), new string("hott")), "wcb2(hue=hott) [string/string]\n");

        // test const vs non-const pointers:
        char       *mut = strdup("mut");
        const char *con = "con";

        // if only const/const and const/mutable exists -> shall always select as much mutable as possible
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb2, con, con), "wccb2(con,con) [const const]\n");   // exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb2, mut, con), "wccb2(mut,con) [const const]\n");   // fallback to const/const
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb2, con, mut), "wccb2(con,mut) [const mutable]\n"); // exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb2, mut, mut), "wccb2(mut,mut) [const mutable]\n"); // fallback to const/mutable

        // if all const/nonconst combinations exist -> shall always pick exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb3, con, con), "wccb3(con,con) [const const]\n");     // exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb3, mut, con), "wccb3(mut,con) [mutable const]\n");   // exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb3, con, mut), "wccb3(con,mut) [const mutable]\n");   // exact match
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb3, mut, mut), "wccb3(mut,mut) [mutable mutable]\n"); // exact match

        // test reference arguments
        int       imut = 17;
        const int icon = 23;

        const long lcon = 775L;

        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, imut),  "wccb1(17)\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, icon),  "wccb1(23)\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &lcon), "wccb1(775) [long ptr]\n");

        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &icon), "wccb1(23) [const ptr]\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &icon), "wccb1(23) [const ptr]\n");

        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &imut), "wccb1(17) [mutable ptr]\n"); // modifies 'imut'
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &imut), "wccb1(34) [mutable ptr]\n"); // modifies 'imut'
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, &imut), "wccb1(68) [mutable ptr]\n"); // modifies 'imut'

        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, mut),   "wccb1(mut) [mutable]\n");
        TEST_CB_TRACE(makeWindowCreatorCallback(wccb1, con),   "wccb1(con) [const]\n");

        free(mut);
    }

}

#endif // UNIT_TESTS


