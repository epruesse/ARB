// =============================================================== //
//                                                                 //
//   File      : ptcommon.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ptcommon.h"
#include <arb_assert.h>


bool index_exists_for(Servertype type) {
    arb_assert(0);
    return false;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static void check_ptpan_impl() { index_exists_for(PTPAN); }
static void check_ptserver_impl() { index_exists_for(PTSERVER); }

void TEST_not_impl() {
    // TEST_ASSERT(0);
    MISSING_TEST(none);
    TEST_ASSERT_CODE_ASSERTION_FAILS(check_ptpan_impl);
    TEST_ASSERT_CODE_ASSERTION_FAILS(check_ptserver_impl);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



