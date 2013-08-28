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
#include <arb_assert.h>

// AISC_MKPT_PROMOTE:#ifndef _GLIBCXX_CSTDLIB
// AISC_MKPT_PROMOTE:#include <cstdlib>
// AISC_MKPT_PROMOTE:#endif

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

const char *GBS_readable_timediff(size_t seconds) {
    size_t mins  = seconds/60; seconds -= mins  * 60;
    size_t hours = mins/60;    mins    -= hours * 60;
    size_t days  = hours/24;   hours   -= days  * 24;

    const int   MAXPRINT = 40;
    int         printed  = 0;
    static char buffer[MAXPRINT+1];

    if (days>0)               printed += sprintf(buffer+printed, "%zud", days);
    if (printed || hours>0)   printed += sprintf(buffer+printed, "%zuh", hours);
    if (printed || mins>0)    printed += sprintf(buffer+printed, "%zum", mins);

    printed += sprintf(buffer+printed, "%zus", seconds);

    arb_assert(printed>0 && printed<MAXPRINT);

    return buffer;
}


