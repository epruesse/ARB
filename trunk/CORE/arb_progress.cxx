// ================================================================ //
//                                                                  //
//   File      : arb_progress.cxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "arb_progress.h"
#include <cstdio>


// -------------------------
//      ARB_basic_status

const int  HEIGHT = 10;
const int  WIDTH = 70;
const char CHAR  = '.';

static int printed = 0;

static void basic_openstatus(const char *title) {
    printf("Progress: %s\n", title);
    printed = 0;
}
static void basic_closestatus() {
    printf("[done]\n");
}
static bool basic_set_subtitle(const char *) { return false; }
static bool basic_set_gauge(double gauge) {
    int wanted = int(gauge*WIDTH*HEIGHT);
    int nextLF = printed-printed%WIDTH+WIDTH;
    while (printed<wanted) {
        fputc(CHAR, stdout);
        printed++;
        if (printed == nextLF) {
            printf(" [%5.1f%%]\n", printed*100.0/(WIDTH*HEIGHT));
            nextLF = printed-printed%WIDTH+WIDTH;
        }
    }
    return false;
}
static bool basic_user_abort() { return false; }

arb_status_implementation ARB_basic_status = {
    basic_openstatus, 
    basic_closestatus, 
    basic_set_subtitle, 
    basic_set_gauge, 
    basic_user_abort
};

// ------------------------
//      ARB_null_status

static void null_openstatus(const char *) {}
static void null_closestatus() {}
static bool null_set_subtitle(const char *) { return false; }
static bool null_set_gauge(double ) { return false; }
static bool null_user_abort() { return false; }

arb_status_implementation ARB_null_status = {
    null_openstatus, 
    null_closestatus, 
    null_set_subtitle,
    null_set_gauge,
    null_user_abort
};

// ---------------------
//      arb_progress

arb_status_implementation  *arb_progress::impl = &ARB_basic_status;
arb_progress::arb_progress *opened             = NULL;

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <test_unit.h>

void TEST_arb_progress() {
    arb_progress outer("outer", 100);
    for (int i = 0; i<100; ++i) {
        ++outer;
    }
}

#endif // UNIT_TESTS


