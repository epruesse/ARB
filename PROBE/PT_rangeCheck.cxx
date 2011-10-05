// ============================================================= //
//                                                               //
//   File      : PT_rangeCheck.cxx                               //
//   Purpose   : Check whether probe is inside region            //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "PT_rangeCheck.h"
#include "pt_prototypes.h"

using namespace std;

int Range::calc_max_abs_pos() const {
    // expensive!
    pt_assert(end != -1);

    int   size;
    char *data = probe_read_alignment(curr_match->name, &size);
    int   pos  = min(end, size-1);
    int   skip = probe_len; // bases to skip

    pt_assert(skip);

    while (pos >= start) {
        if (data[pos] != '-') {
            if (!--skip) break;
        }
        --pos;
    }
    pt_assert(!skip || pos<start);

    free(data);

    if (skip) pos = -1; // not enough bases in range

    return pos;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_nothing() {
    // TEST_ASSERT(0);
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver"); 
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
