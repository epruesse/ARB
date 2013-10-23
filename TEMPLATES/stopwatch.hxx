/********************************************************************************
 * Copyright (c) 2013, Elmar Pruesse <elmar@pruesse.net> 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

/** \file stopwatch.h
 * A simple stopwatch for quick-and-dirty printf-style profiling.
 */
#pragma once

#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <time.h>



struct timestamp : private timespec {
    /*
    struct timespec {
        time_t   tv_sec; 
        long     tv_nsec;
    };
    */
    
    timestamp(int) {
       tv_sec = 0;
       tv_nsec = 0;
    }

    void get() {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, this);
    }

    timestamp() {
        get();
    }

    static const long NANO = 1000000000;
    
    timestamp operator-(const timestamp& rval) const {
        timestamp result(0);
        if (tv_nsec < rval.tv_nsec) {
            result.tv_sec = tv_sec - rval.tv_sec - 1;
            result.tv_nsec = NANO + tv_nsec - rval.tv_nsec;
        } else {
            result.tv_sec =  tv_sec - rval.tv_sec;
            result.tv_nsec = tv_nsec - rval.tv_nsec;
        }
        return result;
    }

    timestamp operator+(const timestamp& rval) const {
        timestamp result(0);
        if (tv_nsec + rval.tv_nsec >= NANO) {
            result.tv_sec = tv_sec + rval.tv_sec + 1;
            result.tv_nsec = tv_nsec + rval.tv_nsec - NANO;
        } else {
            result.tv_sec = tv_sec + rval.tv_sec;
            result.tv_nsec = tv_nsec +  rval.tv_nsec;
        }
        return result;
    }
    
    timestamp& operator+=(const timestamp& rval) {
        if (tv_nsec + rval.tv_nsec >= NANO) {
            tv_sec += rval.tv_sec + 1;
            tv_nsec = tv_nsec + rval.tv_nsec - NANO;
        } else {
            tv_sec += rval.tv_sec;
            tv_nsec += rval.tv_nsec;
        }
        return *this;
    }

    timestamp operator/(long div) const {
        timestamp result(0);
        result.tv_sec = tv_sec / div;
        result.tv_nsec = ((tv_sec % div) * NANO + tv_nsec) / div;
        return result;
    }

    double operator/(const timestamp& rval) const {
        return ((double)tv_sec * NANO + tv_nsec)
            / ((double)rval.tv_sec * NANO + rval.tv_nsec);
    }

    friend std::ostream& operator<<(std::ostream& out, const timestamp& t) {
        return out << t.tv_sec << "." << std::setfill('0') << std::setw(6) << t.tv_nsec / 1000;
    }
};


/**
 * Stopwatch can be used to manually instrument code in printf-debugging
 * style. Each "run" of a piece of code is begun by calling start(), 
 * the segment to be timed separated by calls to time() and the run
 * terminated by calling stop(). Measurements from successive runs
 * are added. The results can be obtained by <<operator on a stream.
 * Also, the stopwatch can print itself on destruct, or regularly
 * after a number of runs.
 *
 * usage:
 * stopwatch sw(stopwatch::PRINT_AT_EXIT, 10000);
 * ...
 * void func() {
 * sw.start("text rendering");
 * do_work();
 * sw.time("now do other work");
 * do_other_work();
 * sw.time("and so on");
 * and_so_on();
 * sw.stop("end");
 * ...
 * }
 * ...
 * std::cerr << sw << std::endl;
 * 
 * results in: 
 * stopwatch text rendering (1240 runs)
 *     0.4905
 *   now do other work
 *     4.53
 *   and so on
 *     0.1
 *   end
 */

class stopwatch {
public:
    enum type {
        PLAIN = 0,
        PRINT_AT_EXIT = 1,
    };

private:
    size_t runs, total_runs;
    std::vector<timestamp> times, total_times;
    std::vector<const char*> labels;
    size_t timer_idx;
    timestamp last;
    size_t print_every;
    type tp;
public:

    /**
     * Use t to configure:
     * @param t           PRINT_AT_EXIT => automatically print on destruct
     * @param print_every automatically print every n rounds
     */
    stopwatch(type t = PLAIN,  size_t print_every_ = 0)
        : runs(0), total_runs(0), timer_idx(0), 
          print_every(print_every_), 
          tp(t)
    {
        if (t & PRINT_AT_EXIT) {
            //on_exit(&stopwatch::print_to_stderr,this);
        }
    }
    
    ~stopwatch() {
        if (tp & PRINT_AT_EXIT) {
            std::cerr << *this << std::endl;
        }
    }

    static void print_to_stderr(int /*exitcode*/, void* arg) {
        stopwatch* sw = (stopwatch*) arg;
        std::cerr << *sw;
    }

    
    /**
     * begin a measuring run
     * @param label give this stopwatch a name
     */
    void start(const char* label="default stopwatch") {
        if (!total_runs) labels.push_back(label);
        timer_idx = 0;
        last.get();
    }

    /**
     * stop a time in the run
     * @param label give this stop a name
     */
    void time(const char* label="") {
        timestamp now;
        if (!total_runs) { 
            labels.push_back(label);
            times.push_back(timestamp(0));
            total_times.push_back(timestamp(0));
        }
        times[timer_idx++] += now - last;
        last.get();
    }

    /**
     * finish a measuring run
     * @param label print last
     */
    void stop(const char* label="end of default stopwatch") {
        time(label);
        runs++; 
        total_runs++;
        if (print_every && (runs % print_every) == 0) {
            std::cerr << *this << std::endl;

            runs=0;
            std::vector<timestamp>::iterator it = times.begin(), jt = total_times.begin();
            for (; it != times.end(); ++it, ++jt) {
                *jt += *it;
                *it = 0;
            }
        }
    }
    
    friend std::ostream& operator<<(std::ostream& out , const stopwatch& t) {
        std::vector<const char*>::const_iterator label_it = t.labels.begin();
        if (label_it == t.labels.end()) return out;
        timestamp sum(0), total_sum(0);
        std::vector<timestamp>::const_iterator it = t.times.begin(), jt = t.total_times.begin();
        for (; it != t.times.end(); ++it, ++jt) {
            sum+=*it;
            total_sum+=*jt;
        }
        out << "stopwatch " << *label_it++ 
            << " (" << t.total_runs << " runs)" << std::endl;
        it = t.times.begin();
        jt = t.total_times.begin();
        for (; it != t.times.end(); ++it, ++jt, ++label_it) {
            out << "    " 
                << *it  << " (" << std::setw(2) << (int)(*it/sum * 100) << "%)"  
                << "    "
                << *jt  << " (" << std::setw(2) << (int)(*jt/total_sum * 100) << "%)" 
                << std::endl;
            out << "  " << *label_it << std::endl;
        }

        out << "total" << sum << " " << total_sum << std::endl;
        
        return out;
    }
};
