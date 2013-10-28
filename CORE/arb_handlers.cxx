// ================================================================ //
//                                                                  //
//   File      : arb_handlers.cxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arb_handlers.h>
#include <arb_msg.h>
#include <arb_assert.h>
#include "arb_misc.h"
#include <smartptr.h>
#include <unistd.h>
#include <time.h>

// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include <arb_core.h>
// AISC_MKPT_PROMOTE:#endif

static FILE *arberr = stderr;
static FILE *arbout = stdout;

static void to_arberr(const char *msg) {
    fflush(arbout);
    fprintf(arberr, "%s\n", msg);
    fflush(arberr);
}
static void to_arbout(const char *msg) {
    fprintf(arbout, "%s\n", msg);
}

// -------------------------
//      ARB_arbout_status

const int  HEIGHT = 12; // 12 can be devided by 2, 3, 4 (so subtitles tend to be at start of line)
const int  WIDTH  = 70;
const char CHAR   = '.';

class BasicStatus : virtual Noncopyable {
    int         openCount;
    char       *subtitle;
    int         printed;
    const char *cursor;

    time_t start;

    void reset() {
        printed  = 0;
        cursor   = NULL;
        subtitle = NULL;
        time(&start);
    }

public:
    BasicStatus() : openCount(0) { reset(); }
    ~BasicStatus() { if (openCount) close(); }

    void open(const char *title) {
        arb_assert(++openCount <= 1);

        fprintf(arbout, "Progress: %s\n", title);
        reset();
    }
    void close() {
        freenull(subtitle);
        fprintf(arbout, "[done]\n");
        arb_assert(--openCount >= 0);
        fflush(arbout);
    }

    int next_LF() const { return printed-printed%WIDTH+WIDTH; }

    void set_subtitle(const char *stitle) {
        arb_assert(openCount == 1);
        if (!cursor) {
            freeset(subtitle, GBS_global_string_copy("{%s}", stitle));
            cursor = subtitle;
        }
    }
    void set_gauge(double gauge) {
        arb_assert(openCount == 1);

        int wanted = int(gauge*WIDTH*HEIGHT);
        int nextLF = next_LF();

        if (printed<wanted) {
            while (printed<wanted) {
                if (cursor && cursor[0]) {
                    fputc(*cursor++, arbout);
                }
                else {
                    cursor = 0;
                    fputc(CHAR, arbout);
                }
                printed++;
                if (printed == nextLF) {
                    time_t now;
                    time(&now);

                    fprintf(arbout, " [%5.1f%%]", printed*100.0/(WIDTH*HEIGHT));
                    bool   done    = (printed == (WIDTH*HEIGHT));
                    double seconds = difftime(now, start);

                    const char *whatshown = done ? "used" : "left";
                    fprintf(arbout, " %s: ", whatshown);

                    long show_sec = done ? seconds : long(seconds*(1.0-gauge)/gauge+0.5);
                    fputs(GBS_readable_timediff(show_sec), arbout);
                    fputc('\n', arbout);

                    nextLF = next_LF();
                }
            }
            fflush(arbout);
#if defined(DEBUG)
            // GB_sleep(25, MS); // uncomment to see slow status
#endif
        }
    }
};

static BasicStatus status;

static void basic_openstatus(const char *title) { status.open(title); }
static void basic_closestatus() { status.close(); }
static bool basic_set_title(const char */*title*/) { return false; }
static bool basic_set_subtitle(const char *stitle) { status.set_subtitle(stitle); return false; }
static bool basic_set_gauge(double gauge) { status.set_gauge(gauge); return false; }
static bool basic_user_abort() { return false; }

static arb_status_implementation ARB_arbout_status = {
    AST_FORWARD, 
    basic_openstatus,
    basic_closestatus,
    basic_set_title,
    basic_set_subtitle,
    basic_set_gauge,
    basic_user_abort
};

static arb_handlers arbout_handlers = {
    to_arberr,
    to_arbout,
    to_arbout,
    ARB_arbout_status,
};

arb_handlers *active_arb_handlers = &arbout_handlers; // default-handlers

void ARB_install_handlers(arb_handlers& handlers) { active_arb_handlers = &handlers; }

void ARB_redirect_handlers_to(FILE *errStream, FILE *outStream) {
    arberr = errStream;
    arbout = outStream;
}
