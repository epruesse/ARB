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
    // returns the max abs startpos of a specific species,
    // such that a probe of length 'probe_len' hits inside this Range.
    //
    // This check is expensive!

    const probe_input_data& pid = curr_match->get_pid();

    pid.preload_rel2abs();

    int rel_size = pid.get_size();
    int abs_size = pid.get_abspos(rel_size-1)+1;

    int max_wanted_abs = min(end, abs_size-1);

    for (int rel = 0; rel<rel_size; ++rel) { // @@@ brute forced
        if (int(pid.get_abspos(rel))>max_wanted_abs) {
            int max_rel = rel-probe_len;
            return pid.get_abspos(max_rel);
        }
    }

    return -1;
}

