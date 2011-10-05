// =============================================================== //
//                                                                 //
//   File      : ptwrapper.cxx                                     //
//   Purpose   : wrapper around ptserver and ptpan                 //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arb_strbuf.h>
#include <ut_valgrinded.h>

int main(int argc, char *argv[]) {
    GBS_strstruct cmdline(1000);

    cmdline.cat("arb_ptserver");
    for (int a = 1; a<argc; a++) {
        cmdline.put(' ');
        cmdline.cat(argv[a]);
    }

    char *cmd = cmdline.release();
    fprintf(stderr, "ptwrapper calls '%s'\n", cmd);

    make_valgrinded_call(cmd);
    int pt_exit = system(cmd);
    free(cmd);
    return pt_exit;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_nothing() {
    // TEST_ASSERT(0);
    MISSING_TEST(none);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

