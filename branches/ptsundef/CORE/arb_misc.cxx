// =============================================================== //
//                                                                 //
//   File      : arb_misc.cxx                                      //
//   Purpose   : misc that doesnt fit elsewhere                    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "arb_misc.h"
#include "arb_msg.h"

const char *GBS_readable_size(unsigned long long size, const char *unit_suffix) {
    // return human readable size information
    // returned string is maximal 6+strlen(unit) characters long
    // (using "b" as 'unit_suffix' produces '### b', '### Mb' etc)

    if (size<1000) return GBS_global_string("%llu %s", size, unit_suffix);

    const char *units = "kMGTPEZY"; // kilo, Mega, Giga, Tera, ... should be enough forever
    int i;

    for (i = 0; units[i]; ++i) {
        char unit = units[i];
        if (size<1000*1024) {
            double amount = size/(double)1024;
            if (amount<10.0)  return GBS_global_string("%4.2f %c%s", amount+0.005, unit, unit_suffix);
            if (amount<100.0) return GBS_global_string("%4.1f %c%s", amount+0.05, unit, unit_suffix);
            return GBS_global_string("%i %c%s", (int)(amount+0.5), unit, unit_suffix);
        }
        size /= 1024; // next unit
    }
    return GBS_global_string("MUCH %s", unit_suffix);
}

