//  ==================================================================== //
//                                                                       //
//    File      : perf_timer.h                                           //
//    Purpose   : Simple performance timer using clock()                 //
//    Time-stamp: <Thu Nov/18/2004 13:55 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in November 2004         //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //
#ifndef PERF_TIMER_H
#define PERF_TIMER_H

#ifndef _CPP_STRING
#include <string>
#endif
#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef _CPP_CTIME
#include <ctime>
#endif

class PerfTimer {
    clock_t       started_at;
    unsigned long loop_counter;
    std::string   message;

public:

    PerfTimer(const std::string& message_)
        : started_at(clock())
        , loop_counter(0)
        , message(message_)
    {}

    ~PerfTimer() {
        clock_t stopped_at = clock();
        clock_t ticks      = stopped_at-started_at;
        double  seconds    = double(ticks)/CLOCKS_PER_SEC;

        printf("Time for '%s': ticks=%lu (= %5.2f seconds)",
               message.c_str(), ticks, seconds);

        if (loop_counter > 0) { // loop timer
            if (loop_counter == 1) {
                printf(" 1 loop");
            }
            else {
                double lticks  = double(ticks)/loop_counter;
                double lseconds = lticks/CLOCKS_PER_SEC;

                printf(" %lu loops. Each took: ticks=%lu",
                       loop_counter, (clock_t)(lticks+0.5));
                if (lseconds >= 0.01) {
                    printf(" (= %5.2f seconds)", lseconds);
                }
                else {
                    printf(" (= %5.2f milliseconds)", lseconds/1000);
                }

                double loopsPerSecond = loop_counter/seconds;
                printf(" = %5.2f loops/second", loopsPerSecond);
            }
        }
        printf("\n");
    }

    void announceLoop() { loop_counter++; }
};



#else
#error perf_timer.h included twice
#endif // PERF_TIMER_H

