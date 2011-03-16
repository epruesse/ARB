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
#include <smartptr.h>
#include <unistd.h>


// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include <arb_core.h>
// AISC_MKPT_PROMOTE:#endif

static void to_stderr(const char *msg) {
    fflush(stdout);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}
static void to_stdout(const char *msg) {
    fprintf(stdout, "%s\n", msg);
}

// -------------------------
//      ARB_stdout_status

const int  HEIGHT = 12; // 12 can be devided by 2, 3, 4 (so subtitles tend to be at start of line)
const int  WIDTH  = 70;
const char CHAR   = '.';

class BasicStatus {
    static int openCount;

    char       *subtitle;
    int         printed;
    const char *cursor;

public:
    void open(const char *title) {
        arb_assert(++openCount <= 1);
        
        printf("Progress: %s\n", title);
        printed  = 0;
        cursor   = NULL;
        subtitle = NULL;
    }
    void close() {
        freenull(subtitle);
        printf("[done]\n");
        arb_assert(--openCount >= 0);
        fflush(stdout);
    }

    ~BasicStatus() {
        if (openCount) close();
    }

    int next_LF() const { return printed-printed%WIDTH+WIDTH; }

    void set_subtitle(const char *stitle) {
        arb_assert(openCount == 1);
        if (cursor) {
            int offset = cursor - subtitle;
            subtitle[strlen(subtitle)-1] = 0;
            freeset(subtitle, GBS_global_string_copy("%s/%s}", subtitle, stitle));
            cursor = subtitle+offset;
        }
        else {
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
                    fputc(*cursor++, stdout);
                }
                else {
                    cursor = 0;
                    fputc(CHAR, stdout);
                }
                printed++;
                if (printed == nextLF) {
                    printf(" [%5.1f%%]\n", printed*100.0/(WIDTH*HEIGHT));
                    nextLF = next_LF();
                }
            }
            fflush(stdout);
#if defined(DEBUG)
            // usleep(25000); // uncomment to see slow status
#endif
        }
    }
};
int BasicStatus::openCount = 0;

static BasicStatus status;

static void basic_openstatus(const char *title) { status.open(title); }
static void basic_closestatus() { status.close(); }
static bool basic_set_title(const char */*title*/) { return false; }
static bool basic_set_subtitle(const char *stitle) { status.set_subtitle(stitle); return false; }
static bool basic_set_gauge(double gauge) { status.set_gauge(gauge); return false; }
static bool basic_user_abort() { return false; }

static arb_status_implementation ARB_stdout_status = {
    AST_FORWARD, 
    basic_openstatus,
    basic_closestatus,
    basic_set_title,
    basic_set_subtitle,
    basic_set_gauge,
    basic_user_abort
};

static arb_handlers stderr_handlers = {
    to_stderr,
    to_stdout,
    to_stdout,
    ARB_stdout_status, 
};

arb_handlers *active_arb_handlers = &stderr_handlers;
void ARB_install_handlers(arb_handlers& handlers) { active_arb_handlers = &handlers; }

