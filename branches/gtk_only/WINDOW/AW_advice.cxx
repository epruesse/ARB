// ============================================================= //
//                                                               //
//   File      : AW_advice.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_advice.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"

#define AWAR_ADVICE_TMP "/tmp/advices/"

#define AWAR_ADVICE_TEXT       AWAR_ADVICE_TMP "text"
#define AWAR_ADVICE_UNDERSTOOD AWAR_ADVICE_TMP "understood"

// -------------------------
//      internal data :

static bool     initialized = false;
static AW_root *advice_root = 0;

void init_Advisor(AW_root *awr) {
    if (!initialized) {
        advice_root  = awr;

        advice_root->awar_string(AWAR_ADVICE_TEXT, "<no advice>");
        advice_root->awar_int(AWAR_ADVICE_UNDERSTOOD, 0);

        initialized = true;
    }
}

void AW_advice(const char */*message*/,
               int type /*= AW_ADVICE_SIMPLE*/,
               const char */*title*/ /*= 0*/,
               const char */*corresponding_help*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#if 1
#warning please reactivate tests below
#else
void TEST_advice_id_awar_handling() {
    GB_shell  shell;
    AW_root  root("min_ascii.arb");
    init_Advisor(&root);

    const char *one = "one";
    const char *two = "second";

    TEST_REJECT(advice_disabled(one));
    TEST_REJECT(advice_disabled(two));

    disable_advice(one);
    TEST_EXPECT(advice_disabled(one));
    TEST_REJECT(advice_disabled(two));

    disable_advice(two);
    TEST_EXPECT(advice_disabled(one));
    TEST_EXPECT(advice_disabled(two));


    TEST_REJECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));

    toggle_advice_shown(two);
    TEST_REJECT(advice_currently_shown(one));
    TEST_EXPECT(advice_currently_shown(two));

    toggle_advice_shown(one);
    TEST_EXPECT(advice_currently_shown(one));
    TEST_EXPECT(advice_currently_shown(two));

    toggle_advice_shown(two);
    TEST_EXPECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));

    toggle_advice_shown(one);
    TEST_REJECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));
}

void TEST_another_AW_root() {
    GB_shell  shell;
    AW_root("min_ascii.arb");
}

#endif

#endif // UNIT_TESTS
