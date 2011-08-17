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
#include <attributes.h>
#include <string>


using namespace std;

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TRACE

static unsigned long traceChecksum;
const int            BUFFERSIZE = 100;
char                 traceBuffer[BUFFERSIZE];

STATIC_ATTRIBUTED(__ATTR__FORMAT(1), void tracef(const char *format, ...)) {
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
    TEST_ASSERT(printed<BUFFERSIZE);

    traceChecksum = 0;
    for (int p = 0; p<printed; ++p) {
        traceChecksum = (traceChecksum<<1)^traceBuffer[p];
    }
}

static AW_root   *fake_root = (AW_root*)1;
static AW_window *fake_win  = (AW_window*)2;

static void rcb0(AW_root *r) {
    TEST_ASSERT(r == fake_root);
    tracef("rcb0()\n");
}
static void rcb1(AW_root *r, const char *name) {
    TEST_ASSERT(r == fake_root);
    tracef("rcb1(%s)\n", name);
}
static void rcb2(AW_root *r, const char *name, int val) {
    TEST_ASSERT(r == fake_root);
    tracef("rcb2(%s=%i) [int]\n", name, val);
}
static void rcb2(AW_root *r, const char *name, long val) {
    TEST_ASSERT(r == fake_root);
    tracef("rcb2(%s=%li) [long]\n", name, val);
}

static void wcb0(AW_window *w) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb0()\n");
}
static void wcb1(AW_window *w, const char *name) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb1(%s)\n", name);
}
static void wcb1(AW_window *w, string *name) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb1(%s) [string]\n", name->c_str());
}
static void wcb2(AW_window *w, const char *name, const char *val) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb2(%s=%s) [const char/const char]\n", name, val);
}
static void wcb2(AW_window *w, const string *name, const string *val) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb2(%s=%s) [string/string]\n", name->c_str(), val->c_str());
}
static void wcb2(AW_window *w, const char *name, int val) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb2(%s=%i) [int]\n", name, val);
}
static void wcb2(AW_window *w, const char *name, long val) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb2(%s=%li) [long]\n", name, val);
}
static void wcb2(AW_window *w, char c, long long val) {
    TEST_ASSERT(w == fake_win);
    tracef("wcb2(%c=%lli) [long long]\n", c, val);
}

static AW_window *wccb0(AW_root *r) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb0()\n");
    return fake_win;
}
static AW_window *wccb1(AW_root *r, int x) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%i)\n", x);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const long *lp) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%li) [long ptr]\n", *lp);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const int *x) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%i) [const ptr]\n", *x);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, int *x) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%i) [mutable ptr]\n", *x);
    *x *= 2; // modify callback argument!
    return fake_win;
}
static AW_window *wccb1(AW_root *r, char* s) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%s) [mutable]\n", s);
    return fake_win;
}
static AW_window *wccb1(AW_root *r, const char* s) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb1(%s) [const]\n", s);
    return fake_win;
}
static AW_window *wccb2(AW_root *r, const char *s1, const char *s2) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb2(%s=%s) [const const]\n", s1, s2);
    return fake_win;
}
static AW_window *wccb2(AW_root *r, const char *s1, char *s2) {
    TEST_ASSERT(r == fake_root);
    tracef("wccb2(%s=%s) [const mutable]\n", s1, s2);
    return fake_win;
}

inline void call(const RootCallback& rcb) { rcb(fake_root); }
inline void call(const WindowCallback& wcb) { wcb(fake_win); }
inline void call(const WindowCreatorCallback& wccb) { TEST_ASSERT(wccb(fake_root) == fake_win); }

#define TEST_CB(cb,expectedChecksum) do {                       \
        call(cb);                                               \
        TEST_ASSERT_EQUAL(traceChecksum, (unsigned long)(expectedChecksum)); \
    } while(0)

#define TEST_CB_TRACE(cb,expectedOutput) do {                           \
        call(cb);                                                       \
        TEST_ASSERT_EQUAL(traceBuffer, expectedOutput);                 \
    } while(0)

#define TEST_CB_TRACE__BROKEN(cb,expectedOutput) do {                   \
        call(cb);                                                       \
        TEST_ASSERT_EQUAL__BROKEN(traceBuffer, expectedOutput);         \
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

        TEST_CB(makeWindowCallback(wcb2, 'l', 0xb09bbc04b09bbcLL),  0xb22c830caac);

        TEST_CB(makeWindowCreatorCallback(wccb0),     0x2878);
        TEST_CB(makeWindowCreatorCallback(wccb1, 77), 0xa19c);

        // callbacks with deallocator

        // TEST_CB(makeWindowCallback(wcb1, strdup("leak")), 0x163ac); // leaks
        TEST_CB(makeWindowCallback(wcb1, freeCharp, strdup("leak")), 0x163ac);
        // TEST_CB(makeWindowCallback(wcb1, freeCharp, "leak"), 0x163ac); // does not compile (can't call freeCharp with const char *)
        TEST_CB(makeWindowCallback(wcb1, deleteString, new string("leak")), 0x2c7790c);
        // TEST_CB(makeWindowCallback(wcb2, strdup("hue"), strdup("hott")), 0x5884382750); // leaks
        TEST_CB(makeWindowCallback(wcb2, freeCharp, strdup("hue"), strdup("hott")), 0x16210e09c190);
        TEST_CB(makeWindowCallback(wcb2, deleteString, new string("hue"), new string("hott")), 0x162108df0c);

        // test const vs non-const pointers:
        char       *mut = strdup("mut");
        const char *con = "con";

        TEST_CB(makeWindowCreatorCallback(wccb2, con, con), 0x515078338);  // exact match
        TEST_CB(makeWindowCreatorCallback(wccb2, mut, con), 0x514678338);  // fallback to const/const
        TEST_CB(makeWindowCreatorCallback(wccb2, con, mut), 0x1454460ac4); // exact match
        TEST_CB(makeWindowCreatorCallback(wccb2, mut, mut), 0x514718338);  // fallback to const const

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


