// ================================================================ //
//                                                                  //
//   File      : PerfMeter.h                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2015   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef PERFMETER_H
#define PERFMETER_H

#ifndef _TIME_H
#include <time.h>
#endif
#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif

struct TimedCombines {
    clock_t ticks;
    long    combines;

    TimedCombines()
        : ticks(clock()),
          combines(AP_sequence::combine_count())
    {}
};

class OptiPerfMeter {
    std::string   what;
    TimedCombines start;
    AP_FLOAT      start_pars;

public:
    OptiPerfMeter(std::string what_, AP_FLOAT start_pars_)
        : what(what_),
          start_pars(start_pars_)
    {}

    void dumpCustom(FILE *out, AP_FLOAT end_pars, const char *label) const {
        TimedCombines end;

        ap_assert(end_pars<=start_pars);

        double   seconds      = double(end.ticks-start.ticks)/CLOCKS_PER_SEC;
        AP_FLOAT pars_improve = start_pars-end_pars;
        long     combines     = end.combines-start.combines;

        double combines_per_second  = combines/seconds;
        double combines_per_improve = combines/pars_improve;
        double improve_per_second   = pars_improve/seconds;

        fprintf(out, "%-27s took %7.2f sec,  improve=%9.1f,  combines=%12li  (comb/sec=%10.2f,  comb/impr=%12.2f,  impr/sec=%10.2f)\n",
                label,
                seconds,
                pars_improve,
                combines,
                combines_per_second,
                combines_per_improve,
                improve_per_second);
    }

    void dump(FILE *out, AP_FLOAT end_pars) const {
        dumpCustom(out, end_pars, what.c_str());
    }
};

class InsertPerfMeter {
    std::string   what;
    TimedCombines start;
    int           inserts;

public:
    InsertPerfMeter(std::string what_, int inserts_)
        : what(what_),
          inserts(inserts_)
    {}

    void dumpCustom(FILE *out, const char *label) const {
        TimedCombines end;

        double seconds  = double(end.ticks-start.ticks)/CLOCKS_PER_SEC;
        long   combines = end.combines-start.combines;

        double combines_per_second = combines/seconds;
        double combines_per_insert = combines/double(inserts);
        double inserts_per_second  = inserts/seconds;

        fprintf(out, "%-27s took %7.2f sec,  inserts=%6i,  combines=%12li  (comb/sec=%10.2f,  comb/ins=%12.2f,  ins/sec=%10.2f)\n",
                label,
                seconds,
                inserts,
                combines,
                combines_per_second,
                combines_per_insert,
                inserts_per_second);
    }

    void dump(FILE *out) const {
        dumpCustom(out, what.c_str());
    }
};

#else
#error PerfMeter.h included twice
#endif // PERFMETER_H
