// ================================================================= //
//                                                                   //
//   File      : ptclean.cxx                                         //
//   Purpose   : prepare db for ptserver/ptpan                       //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2011   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "ptclean.h"
#include <arbdb.h>

// using namespace std;

GB_ERROR clean_ptserver_database(GBDATA *gb_main, Servertype type) {
    fprintf(stderr, "********* clean_ptserver_database does not delete anything yet\n");
    // return "test-error";
    return NULL;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_ptclean() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_pt_src.arb", "rw");

    TEST_ASSERT(gb_main);
    TEST_ASSERT_NO_ERROR(clean_ptserver_database(gb_main, PTSERVER));
    TEST_ASSERT_NO_ERROR(GB_save_as(gb_main, "TEST_pt_cleaned.arb", "a"));
    GB_close(gb_main);

#define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
    TEST_COPY_FILE("TEST_pt_cleaned.arb", "TEST_pt_cleaned_expected.arb");
#else
    TEST_ASSERT_FILES_EQUAL("TEST_pt_cleaned.arb", "TEST_pt_cleaned_expected.arb");
#endif
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

