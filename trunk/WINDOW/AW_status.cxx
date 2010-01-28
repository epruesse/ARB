// =============================================================== //
//                                                                 //
//   File      : AW_status.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_global.hxx"
#include <aw_awars.hxx>

#include <awt_www.hxx>
#include <awt.hxx>
#include <arbdbt.h>
#include <SigHandler.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

#define FD_SET_TYPE

// Globals
#define AW_GAUGE_SIZE        40 // length of gauge display (in characters)
#define AW_GAUGE_GRANULARITY 1000 // how fine the gauge is transported to status (no of steps) [old value = 255]

#define AW_GAUGE_PERCENT(pc) ((pc)*AW_GAUGE_GRANULARITY/100)

#define AW_STATUS_KILL_DELAY       4000             // in ms
#define AW_STATUS_LISTEN_DELAY     300              // in ms
#define AW_STATUS_HIDE_DELAY       60               // in sec
#define AW_STATUS_PIPE_CHECK_DELAY 1000*2           // in ms (a pipe check every 2 seconds)

#define AWAR_STATUS         "tmp/status/"
#define AWAR_STATUS_TITLE   AWAR_STATUS "title"
#define AWAR_STATUS_TEXT    AWAR_STATUS "text"
#define AWAR_STATUS_GAUGE   AWAR_STATUS "gauge"
#define AWAR_STATUS_ELAPSED AWAR_STATUS "elapsed"

#define AWAR_QUESTION "tmp/question"

#define AWAR_HELP       "tmp/help/"
#define AWAR_HELPFILE   AWAR_HELP "file"
#define AWAR_HELPTEXT   AWAR_HELP "text"
#define AWAR_HELPSEARCH AWAR_HELP "search"

#define AW_MESSAGE_LISTEN_DELAY 500 // look in ms whether a father died
#define AW_MESSAGE_LINES        500

#if defined(DEBUG)

// ARB_LOGGING should always be undefined in SVN version!
// #define ARB_LOGGING
// #define TRACE_STATUS // // // enable debug output for status window (which runs forked!)
// #define TRACE_STATUS_MORE // // enable more debug output
// #define PIPE_DEBUGGING // // enable debug output for pipes (for write commands)

#endif // DEBUG

enum {
    // messages send from status-process to main-process :
    AW_STATUS_OK    = 0,
    AW_STATUS_ABORT = 1,
    // messages send from main-process to status-process :
    AW_STATUS_CMD_INIT,
    AW_STATUS_CMD_OPEN,
    AW_STATUS_CMD_CLOSE,
    AW_STATUS_CMD_TEXT,
    AW_STATUS_CMD_GAUGE,
    AW_STATUS_CMD_MESSAGE
};

#define AW_EST_BUFFER 5

struct aw_stg_struct {
    int        fd_to[2];
    int        fd_from[2];
    int        mode;
    int        hide;
    int        hide_delay;                          // in seconds
    pid_t      pid;
    bool       is_child;                            // true in status window process
    int        pipe_broken;
    int        err_no;
    AW_window *aws;
    AW_window *awm;
    bool       status_initialized;
    char      *lines[AW_MESSAGE_LINES];
    bool       need_refresh;                        // if true -> message list needs to refresh
    time_t     last_refresh_time;
    time_t     last_message_time;
    int        local_message;
    time_t     last_start;                          // time of last status start
    long       last_est_count;
    long       last_estimation[AW_EST_BUFFER];
    long       last_used_est;
} aw_stg = {
    { 0, 0 },                                       // fd_to
    { 0, 0 },                                       // fd_from
    AW_STATUS_OK,                                   // mode
    0,                                              // hide
    0,                                              // hide_delay
    0,                                              // pid
    false,                                          // is_child
    0,                                              // pipe_broken
    0,                                              // errno
    0,                                              // aws
    0,                                              // awm
    false,                                          // status_initialized
    { 0, 0, 0 },                                    // lines
    false,                                          // need_refresh
    0,                                              // last_refresh_time
    0,                                              // last_message_time
    0,                                              // local_message
    0,                                              // last_start
    0,                                              // last_est_count
    { 0 },                                          // last_estimation
    -1,                                             // last_used_est
};

// timeouts :

#define POLL_TIMEOUT 0         // idle wait POLL_TIMEOUT microseconds before returning EOF when polling

#if defined(DEBUG)
#define WRITE_TIMEOUT 1000      // 1 second for debug version (short because it always reaches timeout inside debugger)
#else
#define WRITE_TIMEOUT 10000     // 10 seconds for release
#endif // DEBUG

static void mark_pipe_broken(int err_no) {
#if defined(PIPE_DEBUGGING)
    if (aw_stg.pipe_broken != 0) {
        fprintf(stderr,
                "Pipe already broken in mark_pipe_broken(); pipe_broken=%i aw_stg.errno=%i errno=%i\n",
                aw_stg.pipe_broken, aw_stg.err_no, err_no);
    }

    fprintf(stderr, "Marking pipe as broken (errno=%i)\n", err_no);
#endif // PIPE_DEBUGGING

    aw_stg.err_no       = err_no;
    aw_stg.pipe_broken = 1;

    static bool error_shown = false;
    if (!error_shown) {
        fprintf(stderr,
                "******************************************************************\n"
                "The connection to the status window was blocked unexpectedly!\n"
                "This happens if you run the program from inside the debugger\n"
                "or when the process is blocked longer than %5.2f seconds.\n"
                "Further communication with the status window is suppressed.\n"
                "******************************************************************\n"
                , WRITE_TIMEOUT/1000.0);
    }
}

static ssize_t safe_write(int fd, const char *buf, int count) {
    if (aw_stg.pipe_broken != 0) {
#if defined(PIPE_DEBUGGING)
        fprintf(stderr, "pipe is broken -- avoiding write of %i bytes\n", count);
#endif // PIPE_DEBUGGING
        return -1;
    }

    aw_assert(count>0); // write nothing - bad idea

    ssize_t result = -1;
    {
        fd_set         set;
        struct timeval timeout;
        timeout.tv_sec  = WRITE_TIMEOUT/1000;
        timeout.tv_usec = WRITE_TIMEOUT%1000;

        FD_ZERO(&set);
        FD_SET(fd, &set);

        int sel_res = select(fd+1, 0, &set, 0, &timeout);

        if (sel_res == -1) {
            fprintf(stderr, "select (before write) returned error (errno=%i)\n", errno);
            exit(EXIT_FAILURE);
        }

        bool pipe_would_block = !FD_ISSET(fd, &set);

#if defined(PIPE_DEBUGGING)
        fprintf(stderr, "select returned %i, pipe_would_block=%i (errno=%i)\n",
                sel_res, int(pipe_would_block), errno);

        if (pipe_would_block) {
            fprintf(stderr, "  Avoiding to write to pipe (because it would block!)\n");
        }
        else {
            fprintf(stderr, "  Write %i bytes to pipe.\n", count);
        }
#endif // PIPE_DEBUGGING

        if (!pipe_would_block) {
            result = write(fd, buf, count);
        }
    }

    if (result<0) {
        mark_pipe_broken(errno);
    }
    else if (result != count) {
#if defined(PIPE_DEBUGGING)
        fprintf(stderr, "write wrote %i bytes instead of %i as requested.\n", result, count);
#endif // PIPE_DEBUGGING
        mark_pipe_broken(0);
    }

    return result;
}

static void aw_status_write(int fd, int cmd) {
    char buf = cmd;
    safe_write(fd, &buf, 1);
}

static int aw_status_read_byte(int fd, int poll_flag)
    /* read one byte from the pipe,
     * if poll ==1 then don't wait for any data, but return EOF */
{
    int erg;
    unsigned char buffer[2];

    if (poll_flag) {
        fd_set         set;
        struct timeval timeout;
        timeout.tv_sec  = POLL_TIMEOUT/1000;
        timeout.tv_usec = POLL_TIMEOUT%1000;

        FD_ZERO (&set);
        FD_SET (fd, &set);

        erg = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, NULL, &timeout);
        if (erg == 0) return EOF;
    }
    erg = read(fd, (char *)&(buffer[0]), 1);
    if (erg<=0) {
        //      process died
        fprintf(stderr, "father died, now i kill myself\n");
        exit(EXIT_FAILURE);
    }
    return buffer[0];
}

static int aw_status_read_int(int fd, int poll_flag) {
    /* read one integer from the pipe,
     * if poll ==1 then don't wait for any data, but return EOF */

    int erg;
    unsigned char buffer[sizeof(int)+1];

    if (poll_flag) {
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec  = POLL_TIMEOUT/1000;
        timeout.tv_usec = POLL_TIMEOUT%1000;

        FD_ZERO (&set);
        FD_SET (fd, &set);

        erg = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, NULL, &timeout);
        if (erg == 0) return EOF;
    }
    erg = read(fd, (char *)buffer, sizeof(int));
    if (erg<=0) {
        //      process died
        fprintf(stderr, "father died, now i kill myself\n");
        exit(EXIT_FAILURE);
    }
    return *(int*)buffer;
}

static int aw_status_read_command(int fd, int poll_flag, char*& str, int *gaugePtr = 0)
{
    char buffer[1024];
    int  cmd = aw_status_read_byte(fd, poll_flag);

    if (cmd == AW_STATUS_CMD_TEXT ||
            cmd == AW_STATUS_CMD_OPEN ||
            cmd == AW_STATUS_CMD_MESSAGE) {
        char *p = buffer;
        int c;
        for (
             c = aw_status_read_byte(fd, 0);
             c;
             c = aw_status_read_byte(fd, 0)) {
            *(p++) = c;
        }
        *(p++) = c;

        str = strdup(buffer);
    }
    else if (cmd == AW_STATUS_CMD_GAUGE) {
        int gauge = aw_status_read_int(fd, 0);
        if (gaugePtr) *gaugePtr = gauge;

        char *p = buffer;
        int   i = 0;

        int rough_gauge = (gauge*AW_GAUGE_SIZE)/AW_GAUGE_GRANULARITY;
        for (; i<rough_gauge && i<AW_GAUGE_SIZE; ++i) *p++ = '*';
        for (; i<AW_GAUGE_SIZE; ++i) *p++ = '-';

        if (rough_gauge<AW_GAUGE_SIZE) {
            int fine_gauge = (gauge*AW_GAUGE_SIZE*4)/AW_GAUGE_GRANULARITY;
            buffer[rough_gauge]  = "-\\|/"[fine_gauge%4];
        }

        *p  = 0;
        str = strdup(buffer);
    }
    else {
        str = 0;
    }
    return cmd;
}

static void aw_status_check_pipe()
{
    if (getppid() <= 1) {
#if defined(TRACE_STATUS)
        fprintf(stderr, "Terminating status process.\n");
#endif // TRACE_STATUS
        exit(EXIT_FAILURE);
    }
}

static void aw_status_wait_for_open(int fd)
{
    char *str = 0;
    int cmd;
    int erg;

    for (cmd = 0; cmd != AW_STATUS_CMD_INIT;) {
        for (erg = 0; !erg;) {
            struct timeval timeout;
            timeout.tv_sec  = AW_STATUS_PIPE_CHECK_DELAY / 1000;
            timeout.tv_usec = AW_STATUS_PIPE_CHECK_DELAY % 1000;

            fd_set set;

            FD_ZERO (&set);
            FD_SET (fd, &set);

#if defined(TRACE_STATUS)
            fprintf(stderr, "Waiting for status open command..\n"); fflush(stderr);
#endif // TRACE_STATUS
            erg = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, NULL, &timeout);
            if (!erg) aw_status_check_pipe();   // time out
        }
        free(str);
        cmd = aw_status_read_command(fd, 0, str);
    }
    aw_stg.mode = AW_STATUS_OK;
    free(str);

#if defined(TRACE_STATUS)
    fprintf(stderr, "OK got status open command!\n"); fflush(stderr);
#endif // TRACE_STATUS
}


static void aw_status_timer_hide_event(AW_root *, AW_CL, AW_CL) {
    if (aw_stg.hide) {
        aw_stg.aws->show();
        aw_stg.hide = 0;
    }
}

static void aw_status_hide(AW_window *aws)
{
    aw_stg.hide = 1;
    aws->hide();

    // install timer event
    aws->get_root()->add_timed_callback(aw_stg.hide_delay*1000, aw_status_timer_hide_event, 0, 0);

    // increase hide delay for next press of hide button
    if (aw_stg.hide_delay < (60*60)) { // max hide delay is 1 hour
        aw_stg.hide_delay *= 3; // initial: 60sec -> 3min -> 9min -> 27min -> 60min (max)
    }
    else {
        aw_stg.hide_delay = 60*60;
    }
}


static void aw_status_timer_event(AW_root *awr, AW_CL, AW_CL)
{
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_status_timer_event\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE
    if (aw_stg.mode == AW_STATUS_ABORT) {
        int action = aw_question("Couldn't quit properly in time.\n"
                                 "Either wait again for the abortion,\n"
                                 "kill the calculating process or\n"
                                 "continue the calculation.",
                                 "Wait again,Kill,Continue");

        switch (action) {
            case 0:
                return;
            case 1: {
                char buf[255];
                sprintf(buf, "kill -9 %i", aw_stg.pid);
                system(buf);
                exit(0);
            }
            case 2: {
                char *title    = awr->awar(AWAR_STATUS_TITLE)->read_string();
                char *subtitle = awr->awar(AWAR_STATUS_TEXT)->read_string();

                aw_message(GBS_global_string("If you think the process should be made abortable,\n"
                                             "please send the following information to devel@arb-home.de:\n"
                                             "\n"
                                             "Calculation not abortable from status window.\n"
                                             "Title:    %s\n"
                                             "Subtitle: %s\n",
                                             title, subtitle));
                aw_stg.mode = AW_STATUS_OK;

                free(subtitle);
                free(title);
                break;
            }
        }
    }
}

static void aw_status_kill(AW_window *aws)
{
    if (aw_stg.mode == AW_STATUS_ABORT) {
        aw_status_timer_event(aws->get_root(), 0, 0);
        if (aw_stg.mode == AW_STATUS_OK) { // continue
            return;
        }
    }
    else {
        if (!aw_ask_sure("Are you sure to abort running calculation?")) {
            return; // don't abort
        }
        aw_stg.mode = AW_STATUS_ABORT;
    }
    aw_status_write(aw_stg.fd_from[1], aw_stg.mode);

    if (aw_stg.mode == AW_STATUS_ABORT) {
#if defined(TRACE_STATUS_MORE)
        fprintf(stderr, "add aw_status_timer_event with delay = %i\n", AW_STATUS_KILL_DELAY); fflush(stdout);
#endif // TRACE_STATUS_MORE
        aws->get_root()->add_timed_callback(AW_STATUS_KILL_DELAY, aw_status_timer_event, 0, 0); // install timer event
    }
}

static void aw_refresh_tmp_message_display(AW_root *awr) {
    GBS_strstruct *stru = GBS_stropen(AW_MESSAGE_LINES*60); // guessed size

    for (int i = AW_MESSAGE_LINES-1; i>=0; i--) {
        if (aw_stg.lines[i]) {
            GBS_strcat(stru, aw_stg.lines[i]);
            GBS_chrcat(stru, '\n');
        }
    };

    char *str = GBS_strclose(stru);
    awr->awar(AWAR_ERROR_MESSAGES)->write_string(str);
    free(str);

    aw_stg.need_refresh      = false;
    aw_stg.last_refresh_time = aw_stg.last_message_time;
}

static void aw_insert_message_in_tmp_message_delayed(const char *message) {
    free(aw_stg.lines[0]);
    for (int i = 1; i< AW_MESSAGE_LINES; i++) {
        aw_stg.lines[i-1] = aw_stg.lines[i];
    };

    time_t      t    = time(0);
    struct tm  *lt   = localtime(&t);
    const char *lf   = strchr(message, '\n');
    char       *copy = 0;

    if (lf) { // contains linefeeds
        const int indentation = 10;
        int       count       = 1;

        while (lf) { lf = strchr(lf+1, '\n'); ++count; }

        int newsize = strlen(message)+count*indentation+1;
        copy        = (char*)malloc(newsize);

        lf       = strchr(message, '\n');
        char *cp = copy;
        while (lf) {
            int len  = lf-message;
            memcpy(cp, message, len+1);
            cp      += len+1;
            memset(cp, ' ', indentation);
            cp      += indentation;

            message = lf+1;
            lf      = strchr(message, '\n');
        }

        strcpy(cp, message);

        message = copy;
    }

    aw_stg.lines[AW_MESSAGE_LINES-1] = GBS_global_string_copy("%02i:%02i.%02i  %s",
                                                              lt->tm_hour, lt->tm_min, lt->tm_sec,
                                                              message);
    aw_stg.last_message_time         = t;
    free(copy);

    aw_stg.need_refresh = true;
}

static void aw_insert_message_in_tmp_message(AW_root *awr, const char *message) {
    aw_insert_message_in_tmp_message_delayed(message);
    aw_refresh_tmp_message_display(awr);
}

inline const char *sec2disp(long seconds) {
    static char buffer[50];

    if (seconds<0) seconds = 0;
    if (seconds<100) {
        sprintf(buffer, "%li sec", seconds);
    }
    else {
        long minutes = (seconds+30)/60;

        if (minutes<60) {
            sprintf(buffer, "%li min", minutes);
        }
        else {
            long hours = minutes/60;
            minutes    = minutes%60;

            sprintf(buffer, "%lih:%02li min", hours, minutes);
        }
    }
    return buffer;
}

#ifdef ARB_LOGGING
static void aw_status_append_to_log(const char* str)
{
    static const char *logname = 0;
    if (!logname) {
        logname = GBS_global_string_copy("%s/arb.log", GB_getenvHOME());
    }

    int fd           = open(logname, O_WRONLY|O_APPEND);
    if (fd == -1) fd = open(logname, O_CREAT|O_WRONLY|O_APPEND, S_IWUSR | S_IRUSR);
    if (fd == -1) return;

    write(fd, str, strlen(str));
    write(fd, "\n", 1);
    close(fd);
}
#endif


static void aw_status_timer_listen_event(AW_root *awr, AW_CL, AW_CL)
{
    static int  delay      = AW_STATUS_LISTEN_DELAY;
    int         cmd;
    char       *str        = 0;
    int         gaugeValue = 0;

#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_status_timer_listen_event (aw_stg.is_child=%i)\n", (int)aw_stg.is_child); fflush(stdout);
#endif // TRACE_STATUS_MORE

    if (aw_stg.need_refresh && aw_stg.last_refresh_time != aw_stg.last_message_time) {
        aw_refresh_tmp_message_display(awr); // force refresh each second
    }

    cmd = aw_status_read_command(aw_stg.fd_to[0], 1, str, &gaugeValue);
    if (cmd == EOF) {
        aw_status_check_pipe();
        delay = delay*3/2+1;      // wait a longer time
        if (aw_stg.need_refresh) aw_refresh_tmp_message_display(awr); // and do the refresh here
    }
    else {
        delay = delay*2/3+1;       // shorten time  (was *2/3+1)
    }
    char *gauge = 0;
    while (cmd != EOF) {
        switch (cmd) {
            case AW_STATUS_CMD_OPEN:
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_OPEN\n"); fflush(stdout);
#endif // TRACE_STATUS
                aw_stg.mode           = AW_STATUS_OK;
                aw_stg.last_start     = time(0);
                aw_stg.last_est_count = 0;
                aw_stg.last_used_est  = -1;
                aw_stg.aws->show();
                aw_stg.hide           = 0;
                aw_stg.hide_delay     = AW_STATUS_HIDE_DELAY;

#if defined(ARB_LOGGING)
                aw_status_append_to_log("----------------------------");
#endif // ARB_LOGGING
                awr->awar(AWAR_STATUS_TITLE)->write_string(str);
                awr->awar(AWAR_STATUS_GAUGE)->write_string("----------------------------");
                awr->awar(AWAR_STATUS_TEXT)->write_string("");
                awr->awar(AWAR_STATUS_ELAPSED)->write_string("");
                cmd = EOF;
                free(str);
                continue;       // break while loop

            case AW_STATUS_CMD_CLOSE:
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_CLOSE\n"); fflush(stdout);
#endif // TRACE_STATUS
                aw_stg.mode = AW_STATUS_OK;
                aw_stg.aws->hide();
                break;

            case AW_STATUS_CMD_TEXT:
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_TEXT\n"); fflush(stdout);
#endif // TRACE_STATUS
#if defined(ARB_LOGGING)
                aw_status_append_to_log(str);
#endif // ARB_LOGGING
                awr->awar(AWAR_STATUS_TEXT)->write_string(str);
                break;

            case AW_STATUS_CMD_GAUGE: {
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_GAUGE\n"); fflush(stdout);
#endif // TRACE_STATUS

                reassign(gauge, str);

                if (gaugeValue>0) {
                    long sec_elapsed   = time(0)-aw_stg.last_start;
                    long sec_estimated = (sec_elapsed*AW_GAUGE_GRANULARITY)/gaugeValue; // guess overall time

#define PRINT_MIN_GAUGE AW_GAUGE_PERCENT(2)

                    char buffer[200];
                    int  off  = 0;
                    off      += sprintf(buffer+off, "%i%%  ", gaugeValue*100/AW_GAUGE_GRANULARITY);
                    off      += sprintf(buffer+off, "Elapsed: %s  ", sec2disp(sec_elapsed));

                    // rotate estimations
                    memmove(aw_stg.last_estimation, aw_stg.last_estimation+1, sizeof(aw_stg.last_estimation[0])*(AW_EST_BUFFER-1));
                    aw_stg.last_estimation[AW_EST_BUFFER-1] = sec_estimated;

                    if (aw_stg.last_est_count == AW_EST_BUFFER) { // now we can estimate!
                        long used_estimation = 0;
                        int  i;

                        for (i = 0; i<AW_EST_BUFFER; ++i) {
                            used_estimation += aw_stg.last_estimation[i];
                        }
                        used_estimation /= AW_EST_BUFFER;

                        if (aw_stg.last_used_est != -1) {
                            long diff = labs(aw_stg.last_used_est-used_estimation);
                            if (diff <= 1) { used_estimation = aw_stg.last_used_est; } // fake away low frequency changes
                        }

                        long sec_rest         = used_estimation-sec_elapsed;
                        off                  += sprintf(buffer+off, "Rest: %s", sec2disp(sec_rest));
                        aw_stg.last_used_est  = used_estimation;
                    }
                    else {
                        aw_stg.last_est_count++;
                        off += sprintf(buffer+off, "Rest: ???");
                    }

                    awr->awar(AWAR_STATUS_ELAPSED)->write_string(buffer);

#if defined(TRACE_STATUS)
                    fprintf(stderr, "gauge=%i\n", gaugeValue); fflush(stdout);
#endif // TRACE_STATUS
                }
                else if (gaugeValue == 0) { // restart timer
                    aw_stg.last_start     = time(0);
                    aw_stg.last_est_count = 0;
                    aw_stg.last_used_est  = 0;
#if defined(TRACE_STATUS)
                    fprintf(stderr, "reset status timer (gauge=0)\n"); fflush(stdout);
#endif // TRACE_STATUS
                }
                break;
            }
            case AW_STATUS_CMD_MESSAGE:
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_MESSAGE\n"); fflush(stdout);
#endif // TRACE_STATUS
#if defined(ARB_LOGGING)
                aw_status_append_to_log(str);
#endif // ARB_LOGGING
                aw_stg.awm->activate();
                aw_insert_message_in_tmp_message_delayed(str);
                break;

            default:
                break;
        }
        free(str);
        cmd = aw_status_read_command(aw_stg.fd_to[0], 1, str, &gaugeValue);
    }

#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "exited while loop\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE

    if (gauge) {
        awr->awar(AWAR_STATUS_GAUGE)->write_string(gauge);
        free(gauge);
    }
    if (delay>AW_STATUS_LISTEN_DELAY) delay = AW_STATUS_LISTEN_DELAY;
    else if (delay<0) delay                 = 0;
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "add aw_status_timer_listen_event with delay = %i\n", delay); fflush(stdout);
#endif // TRACE_STATUS_MORE
    awr->add_timed_callback_never_disabled(delay, aw_status_timer_listen_event, 0, 0);
}

void aw_clear_message_cb(AW_window *aww)
{
    int i;
    AW_root *awr = aww->get_root();
    for (i = 0; i< AW_MESSAGE_LINES; i++) freenull(aw_stg.lines[i]);
    awr->awar(AWAR_ERROR_MESSAGES)->write_string("");
}

static void aw_clear_and_hide_message_cb(AW_window *aww) {
    aw_clear_message_cb(aww);
    AW_POPDOWN(aww);
}

void aw_initstatus()
{
    int error;

    aw_assert(aw_stg.pid == 0); // do not init status twice!

    error = pipe(aw_stg.fd_to);
    if (error) {
        printf("Cannot create socketpair\n");
        exit(-1);
    }
    error = pipe(aw_stg.fd_from);
    if (error) {
        printf("Cannot create socketpair\n");
        exit(-1);
    }

    aw_stg.pid = getpid();
    GB_install_pid(1);
    pid_t clientid = fork();

    if (clientid) { /* i am the father */
#if defined(TRACE_STATUS)
        fprintf(stderr, "Forked status! (i am the father)\n"); fflush(stderr);
#endif // TRACE_STATUS
        return;
    }
    else {
        GB_install_pid(1);

#if defined(TRACE_STATUS)
        fprintf(stderr, "Forked status! (i am the child)\n"); fflush(stderr);
#endif // TRACE_STATUS

        aw_stg.is_child = true; // mark as child

        AW_root *aw_root;
        aw_root = new AW_root;


        AW_default aw_default = aw_root->open_default(".arb_prop/status.arb");
        aw_root->init_variables(aw_default);
        aw_root->awar_string(AWAR_STATUS_TITLE, "------------------------------------", aw_default);
        aw_root->awar_string(AWAR_STATUS_TEXT, "", aw_default);
        aw_root->awar_string(AWAR_STATUS_GAUGE, "------------------------------------", aw_default);
        aw_root->awar_string(AWAR_STATUS_ELAPSED, "", aw_default);
        aw_root->awar_string(AWAR_ERROR_MESSAGES, "", aw_default);
        aw_root->init_root("ARB_STATUS", true);

        AW_window_simple *aws = new AW_window_simple;
        aws->init(aw_root, "STATUS_BOX", "STATUS BOX");
        aws->load_xfig("status.fig");

        aws->button_length(AW_GAUGE_SIZE+4);
        aws->at("Titel");
        aws->create_button(0, AWAR_STATUS_TITLE);

        aws->at("Text");
        aws->create_button(0, AWAR_STATUS_TEXT);

        aws->at("Gauge");
        aws->create_button(0, AWAR_STATUS_GAUGE);

        aws->at("elapsed");
        aws->create_button(0, AWAR_STATUS_ELAPSED);

        aws->at("Hide");
        aws->callback(aw_status_hide);
        aws->create_button("HIDE", "Hide", "h");

        aws->at("Kill");
        aws->callback(aw_status_kill);
        aws->create_button("ABORT", "Abort", "k");

        aw_stg.hide = 0;
        aw_stg.aws = aws;

        AW_window_simple *awm = new AW_window_simple;
        awm->init(aw_root, "MESSAGE_BOX", "MESSAGE BOX");
        awm->load_xfig("message.fig");

        awm->at("Message");
        awm->create_text_field(AWAR_ERROR_MESSAGES, 10, 2);

        awm->at("Hide");
        awm->callback(AW_POPDOWN);
        awm->create_button("HIDE", "Hide", "h");

        awm->at("Clear");
        awm->callback(aw_clear_message_cb);
        awm->create_button("CLEAR", "Clear", "C");

        awm->at("HideNClear");
        awm->callback(aw_clear_and_hide_message_cb);
        awm->create_button("HIDE_CLEAR", "Ok", "O");

        aw_stg.awm = awm;

#if defined(TRACE_STATUS)
        fprintf(stderr, "Created status window!\n"); fflush(stderr);
#endif // TRACE_STATUS

        aw_status_wait_for_open(aw_stg.fd_to[0]);

        // install callback
        aws->get_root()->add_timed_callback_never_disabled(30, aw_status_timer_listen_event, 0, 0); // use short delay for first callback

        // do NOT AWT_install_cb_guards here!
        aw_root->main_loop();                       // never returns
    }
}




void aw_openstatus(const char *title)
{
    aw_stg.mode = AW_STATUS_OK;
    if (!aw_stg.status_initialized) {
        aw_stg.status_initialized = true;
        aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
    }
    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_OPEN);
    safe_write(aw_stg.fd_to[1], title, strlen(title)+1);
}

void aw_closestatus()
{
    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_CLOSE);
}

int aw_status(const char *text) {
    if (!text) text = "";

    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_TEXT);
    int len = strlen(text)+1;

    safe_write(aw_stg.fd_to[1], text, len);

    return aw_status();
}

int aw_status(double gauge) {
    static int last_val = -1;
    int        val      = (int)(gauge*AW_GAUGE_GRANULARITY);

    if (val != last_val) {
        if (val>0 || gauge == 0.0) {            // only write 0 if gauge really is 0 (cause 0 resets the timer)
            aw_assert(gauge <= 1.0);            // please fix the gauge calculation in caller!
            aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_GAUGE);
            safe_write(aw_stg.fd_to[1], (char*)&val, sizeof(int));
        }
        last_val = val;
    }
    return aw_status();
}

int aw_status() {
    char *str = 0;
    int cmd;
    if (aw_stg.mode == AW_STATUS_ABORT) return AW_STATUS_ABORT;
    for (cmd = 0;
            cmd != EOF;
            cmd = aw_status_read_command(aw_stg.fd_from[0], 1, str)) {
        delete str;
        if (cmd == AW_STATUS_ABORT)     aw_stg.mode = AW_STATUS_ABORT;
    }

    return aw_stg.mode;
}

// -------------------
//      aw_message

int aw_message_cb_result;

void message_cb(AW_window *aw, AW_CL cd1) {
    AWUSE(aw);
    long result = (long)cd1;
    if (result == -1) { /* exit */
        exit(EXIT_FAILURE);
    }
    aw_message_cb_result = ((int)result);
    return;
}



static void aw_message_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2)
{
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_message_timer_listen_event\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE

    AW_window *aww = ((AW_window *)cl1);
    if (aww->is_shown()) { // if still shown, then auto-activate to avoid that user minimizes prompter
        aww->activate();
        awr->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, cl1, cl2);
    }
}

void aw_set_local_message() {
    aw_stg.local_message = 1;
}

bool aw_ask_sure(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    return aw_question(msg, "Yes,No", fixedSizeButtons, helpfile) == 0;
}
void aw_popup_ok(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    aw_question(msg, "Ok", fixedSizeButtons, helpfile);
}
void aw_popup_exit(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    aw_question(msg, "EXIT", fixedSizeButtons, helpfile);
    aw_assert(0); // should not be reached
    exit(EXIT_FAILURE);
}

int aw_question(const char *question, const char *buttons, bool fixedSizeButtons, const char *helpfile) {
    // return 0 for first button, 1 for second button, 2 for third button, ...
    //
    // the single buttons are separated by commas (e.g. "YES,NO")
    // If the button-name starts with ^ it starts a new row of buttons
    // (if a button named 'EXIT' is pressed the program terminates using exit(EXIT_FAILURE))
    //
    // The remaining arguments only apply if 'buttons' is non-zero:
    //
    // If fixedSizeButtons is true all buttons have the same size
    // otherwise the size for every button is set depending on the text length
    //
    // If 'helpfile' is non-zero a HELP button is added.

    aw_assert(buttons);

    AW_root *root = AW_root::THIS;

    AW_window_message *aw_msg;
    char              *button_list  = strdup(buttons ? buttons : "OK");

    if (button_list[0] == 0) {
        freedup(button_list, "Maybe ok,EXIT");
        GBK_dump_backtrace(stderr, "Empty buttonlist");
        question = GBS_global_string_copy("%s\n"
                                          "(Program error - Unsure what happens when you click ok\n"
                                          " Check console for backtrace and report error)",
                                          question);
    }

    AW_awar *awar_quest     = root->awar_string(AWAR_QUESTION);
    if (!question) question = "<oops - no question?!>";
    awar_quest->write_string(question);

    size_t question_length, question_lines;
    aw_detect_text_size(question, question_length, question_lines);

    // hash key to find matching window
    char *hindex = GBS_global_string_copy("%s$%zu$%zu$%i$%s",
                                          button_list,
                                          question_length, question_lines, int(fixedSizeButtons),
                                          helpfile ? helpfile : "");

    static GB_HASH *hash_windows    = 0;
    if (!hash_windows) hash_windows = GBS_create_hash(256, GB_MIND_CASE);
    aw_msg                          = (AW_window_message *)GBS_read_hash(hash_windows, hindex);

#if defined(DEBUG)
    printf("question_length=%zu question_lines=%zu\n", question_length, question_lines);
    printf("hindex='%s'\n", hindex);
    if (aw_msg) printf("[Reusing existing aw_question-window]\n");
#endif // DEBUG

    if (!aw_msg) {
        aw_msg = new AW_window_message;
        GBS_write_hash(hash_windows, hindex, (long)aw_msg);

        aw_msg->init(root, "QUESTION BOX", false);
        aw_msg->recalc_size_atShow(AW_RESIZE_DEFAULT); // force size recalc (ignores user size)

        aw_msg->label_length(10);

        aw_msg->at(10, 10);
        aw_msg->auto_space(10, 10);

        aw_msg->button_length(question_length+1);
        aw_msg->button_height(question_lines);

        aw_msg->create_button(0, AWAR_QUESTION);

        aw_msg->button_height(0);

        aw_msg->at_newline();

        if (fixedSizeButtons) {
            size_t  max_button_length = helpfile ? 4 : 0;
            char   *pos               = button_list;

            while (1) {
                char *comma       = strchr(pos, ',');
                if (!comma) comma = strchr(pos, 0);

                size_t len                                   = comma-pos;
                if (len>max_button_length) max_button_length = len;

                if (!comma[0]) break;
                pos = comma+1;
            }

            aw_msg->button_length(max_button_length+1);
        }
        else {
            aw_msg->button_length(0);
        }

        // insert the buttons:
        char *ret              = strtok(button_list, ",");
        bool  help_button_done = false;
        int   counter          = 0;

        while (ret) {
            if (ret[0] == '^') {
                if (helpfile && !help_button_done) {
                    aw_msg->callback(AW_POPUP_HELP, (AW_CL)helpfile);
                    aw_msg->create_button("HELP", "HELP", "H");
                    help_button_done = true;
                }
                aw_msg->at_newline();
                ++ret;
            }
            if (strcmp(ret, "EXIT") == 0) {
                aw_msg->callback(message_cb, -1);
            }
            else {
                aw_msg->callback(message_cb, (AW_CL)counter++);
            }

            if (fixedSizeButtons) {
                aw_msg->create_button(0, ret);
            }
            else {
                aw_msg->create_autosize_button(0, ret);
            }
            ret = strtok(NULL, ",");
        }

        if (helpfile && !help_button_done) { // if not done above
            aw_msg->callback(AW_POPUP_HELP, (AW_CL)helpfile);
            aw_msg->create_button("HELP", "HELP", "H");
            help_button_done = true;
        }

        aw_msg->window_fit();
    }
    free(hindex);
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();

    free(button_list);
    aw_message_cb_result = -13;

#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "add aw_message_timer_listen_event with delay = %i\n", AW_MESSAGE_LISTEN_DELAY); fflush(stdout);
#endif // TRACE_STATUS_MORE
    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_message_cb_result == -13) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    switch (aw_message_cb_result) {
        case -1:                /* exit with core */
            fprintf(stderr, "Core dump requested\n");
            ARB_SIGSEGV(1);
            break;
        case -2:                /* exit without core */
            exit(-1);
            break;
    }
    return aw_message_cb_result;
}

void aw_message(const char *msg) {
#if defined(DEBUG)
    printf("aw_message: '%s'\n", msg);
#endif // DEBUG
    if (aw_stg.local_message) {
        aw_insert_message_in_tmp_message(AW_root::THIS, msg);
    }
    else {
        if (!aw_stg.status_initialized) {
            aw_stg.status_initialized = true;
            aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
        }
        aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_MESSAGE);
        int len = strlen(msg)+1;
        safe_write(aw_stg.fd_to[1], msg, len);
    }
}

#if defined(DEVEL_RALF)
#warning remove AW_ERROR_BUFFER
#endif // DEVEL_RALF
char AW_ERROR_BUFFER[1024];

void aw_message() { aw_message(AW_ERROR_BUFFER); }

void aw_error(const char *text, const char *text2) {
    char    buffer[1024];
    sprintf(buffer, "An internal error occur:\n\n%s %s\n\nYou may:", text, text2);
    aw_question(buffer, "Continue,EXIT");
}

// -----------------
//      aw_input

char       *aw_input_cb_result        = 0;
static int  aw_string_selected_button = -2;

int aw_string_selection_button() {
    aw_assert(aw_string_selected_button != -2);
    return aw_string_selected_button;
}

#define AW_INPUT_AWAR       "tmp/input/string"
#define AW_INPUT_TITLE_AWAR "tmp/input/title"

#define AW_FILE_SELECT_BASE        "tmp/file_select"
#define AW_FILE_SELECT_DIR_AWAR    AW_FILE_SELECT_BASE "/directory"
#define AW_FILE_SELECT_FILE_AWAR   AW_FILE_SELECT_BASE "/file_name"
#define AW_FILE_SELECT_FILTER_AWAR AW_FILE_SELECT_BASE "/filter"
#define AW_FILE_SELECT_TITLE_AWAR  AW_FILE_SELECT_BASE "/title"

static void create_input_awars(AW_root *aw_root) {
    aw_root->awar_string(AW_INPUT_TITLE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_INPUT_AWAR,       "", AW_ROOT_DEFAULT);
}

static void create_fileSelection_awars(AW_root *aw_root) {
    aw_root->awar_string(AW_FILE_SELECT_TITLE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_DIR_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_FILE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_FILTER_AWAR, "", AW_ROOT_DEFAULT);
}

// -------------------------
//      aw_input history

static deque<string> input_history; // front contains newest entries

#if defined(DEBUG)
// # define TRACE_HISTORY
#endif // DEBUG


#if defined(TRACE_HISTORY)
static void dumpHistory(const char *where) {
    printf("History [%s]:\n", where);
    for (deque<string>::iterator h = input_history.begin(); h != input_history.end(); ++h) {
        printf("'%s'\n", h->c_str());
    }
}
#endif // TRACE_HISTORY

static void input_history_insert(const char *str, bool front) {
    string s(str);

    if (input_history.empty()) {
        input_history.push_front(""); // insert an empty string into history
    }
    else {
        deque<string>::iterator found = find(input_history.begin(), input_history.end(), s);
        if (found != input_history.end()) {
            input_history.erase(found);
        }
    }
    if (front) {
        input_history.push_front(s);
    }
    else {
        input_history.push_back(s);
    }

#if defined(TRACE_HISTORY)
    dumpHistory(GBS_global_string("input_history_insert('%s', front=%i)", str, front));
#endif // TRACE_HISTORY
}

void input_history_cb(AW_window *aw, AW_CL cl_mode) {
    int      mode    = (int)cl_mode;                // -1 = '<' +1 = '>'
    AW_root *aw_root = aw->get_root();
    AW_awar *awar    = aw_root->awar(AW_INPUT_AWAR);
    char    *content = awar->read_string();

    if (content) input_history_insert(content, mode == 1);

    if (!input_history.empty()) {
        if (mode == -1) {
            string s = input_history.front();
            awar->write_string(s.c_str());
            input_history.pop_front();
            input_history.push_back(s);
        }
        else {
            string s = input_history.back();
            awar->write_string(s.c_str());
            input_history.pop_back();
            input_history.push_front(s);
        }
    }

#if defined(TRACE_HISTORY)
    dumpHistory(GBS_global_string("input_history_cb(mode=%i)", mode));
#endif // TRACE_HISTORY

    free(content);
}

void input_cb(AW_window *aw, AW_CL cd1) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result        = 0;
    aw_string_selected_button = int(cd1);

    if (cd1 >= 0) {              // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_INPUT_AWAR)->read_as_string();
    }
}

void file_selection_cb(AW_window *aw, AW_CL cd1) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result        = 0;
    aw_string_selected_button = int(cd1);

    if (cd1 >= 0) {              // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_FILE_SELECT_FILE_AWAR)->read_as_string();
    }
}

#define INPUT_SIZE 50           // size of input prompts in aw_input and aw_string_selection

static AW_window_message *new_input_window(AW_root *root, const char *title, const char *buttons) {
    // helper for aw_input and aw_string_selection
    //
    // 'buttons' comma separated list of button names (buttons starting with \n force a newline)

    AW_window_message *aw_msg = new AW_window_message;

    aw_msg->init(root, title, false);

    aw_msg->label_length(0);
    aw_msg->auto_space(10, 10);

    aw_msg->at(10, 10);
    aw_msg->button_length(INPUT_SIZE+1);
    aw_msg->create_button(0, AW_INPUT_TITLE_AWAR);

    aw_msg->at_newline();
    aw_msg->create_input_field(AW_INPUT_AWAR, INPUT_SIZE);

    int    butCount     = 2;    // ok and cancel
    char **button_names = 0;
    int    maxlen       = 6;    // use as min.length for buttons (for 'CANCEL')

    if (buttons) {
        button_names = GBT_split_string(buttons, ',', &butCount);

        for (int b = 0; b<butCount; b++) {
            int len = strlen(button_names[b]);
            if (len>maxlen) maxlen = len;
        }

    }

    aw_msg->button_length(maxlen+1);

#define MAXBUTTONSPERLINE 5

    aw_msg->at_newline();
    aw_msg->callback(input_history_cb, -1); aw_msg->create_button("bwd", "<<", 0);
    aw_msg->callback(input_history_cb,  1); aw_msg->create_button("fwd", ">>", 0);
    int thisLine = 2;

    // @@@ add a history button (opening a window with elements from history)

    if (butCount>(MAXBUTTONSPERLINE-thisLine) && butCount <= MAXBUTTONSPERLINE) { // approx. 5 buttons (2+3) fit into one line
        aw_msg->at_newline();
        thisLine = 0;
    }

    if (buttons) {
        for (int b = 0; b<butCount; b++) {
            const char *name    = button_names[b];
            bool        forceLF = name[0] == '\n';

            if (thisLine >= MAXBUTTONSPERLINE || forceLF) {
                aw_msg->at_newline();
                thisLine = 0;
                if (forceLF) name++;
            }
            aw_msg->callback(input_cb, b);          // use b == 0 as result for 1st button, 1 for 2nd button, etc.
            aw_msg->create_button(name, name, "");
            thisLine++;
        }
        GBT_free_names(button_names);
        button_names = 0;
    }
    else {
        aw_msg->callback(input_cb,  0); aw_msg->create_button("OK", "OK", "O");
        aw_msg->callback(input_cb, -1); aw_msg->create_button("CANCEL", "CANCEL", "C");
    }

    return aw_msg;
}

char *aw_input(const char *title, const char *prompt, const char *default_input) {
    // prompt user to enter a string
    //
    // title         = title of window
    // prompt        = question
    // default_input = default for answer (NULL -> "")
    //
    // result is NULL, if cancel was pressed
    // otherwise result contains the user input (maybe an empty string)

    static AW_window_message *aw_msg = 0;

    AW_root *root = AW_root::THIS;
    if (!aw_msg) create_input_awars(root); // first call -> create awars

    root->awar(AW_INPUT_TITLE_AWAR)->write_string(prompt);
    aw_assert(strlen(prompt) <= INPUT_SIZE);

    AW_awar *inAwar = root->awar(AW_INPUT_AWAR);
    if (default_input) {
        input_history_insert(default_input, true); // insert default into history
        inAwar->write_string(default_input);
    }
    else {
        inAwar->write_string("");
    }

    aw_assert(GB_get_transaction_level(inAwar->gb_var) <= 0); // otherwise history would not work

    if (!aw_msg) aw_msg = new_input_window(root, title, NULL);
    else aw_msg->set_window_title(title);

    aw_msg->window_fit();
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[]       = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    if (aw_input_cb_result) input_history_insert(aw_input_cb_result, true);
    return aw_input_cb_result;
}

char *aw_input(const char *prompt, const char *default_input) {
    return aw_input("Enter string", prompt, default_input);
}

char *aw_input2awar(const char *title, const char *prompt, const char *awar_name) {
    AW_root *aw_root       = AW_root::THIS;
    AW_awar *awar          = aw_root->awar(awar_name);
    char    *default_value = awar->read_string();
    char    *result        = aw_input(title, prompt, default_value);

    awar->write_string(result);
    free(default_value);

    return result;
}

char *aw_input2awar(const char *prompt, const char *awar_name) {
    return aw_input2awar("Enter string", prompt, awar_name);
}


char *aw_string_selection(const char *title, const char *prompt, const char *default_input, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    // A modal input window. A String may be entered by hand or selected from value_list
    //
    //      title           window title
    //      prompt          prompt at input field
    //      default_input   default value (if NULL => "").
    //      value_list      Existing selections (separated by ';') or NULL if no selection exists
    //      buttons         String containing answer button names separated by ',' (default is "OK,Cancel")
    //                      Use aw_string_selected_button() to detect which has been pressed.
    //      check_fun       function to correct input (or NULL for no check). The function may return NULL to indicate no correction
    //
    // returns the value of the inputfield

    static GB_HASH *str_sels = 0; // for each 'buttons' store window + selection list

    if (!str_sels) str_sels = GBS_create_hash(20, GB_MIND_CASE);

    struct str_sel_data {
        AW_window_message *aw_msg;
        AW_selection_list *sel;
    };

    const char   *bkey = buttons ? buttons : ",default,";
    str_sel_data *sd      = (str_sel_data*)GBS_read_hash(str_sels, bkey);
    if (!sd) {
        sd         = new str_sel_data;
        sd->aw_msg = 0;
        sd->sel    = 0;

        GBS_write_hash(str_sels, bkey, (long)sd);
    }

    AW_window_message *& aw_msg = sd->aw_msg;
    AW_selection_list *& sel    = sd->sel;

    AW_root *root = AW_root::THIS;
    if (!aw_msg) create_input_awars(root); // first call -> create awars

    root->awar(AW_INPUT_TITLE_AWAR)->write_string(prompt);
    aw_assert(strlen(prompt) <= INPUT_SIZE);

    AW_awar *inAwar = root->awar(AW_INPUT_AWAR);
    if (default_input) {
        input_history_insert(default_input, true); // insert default into history
        inAwar->write_string(default_input);
    }
    else {
        inAwar->write_string("");
    }

    aw_assert(GB_get_transaction_level(inAwar->gb_var) <= 0); // otherwise history would not work

    if (!aw_msg) {
        aw_msg = new_input_window(root, title, buttons);

        aw_msg->at_newline();
        sel = aw_msg->create_selection_list(AW_INPUT_AWAR, 0, 0, INPUT_SIZE, 10);
        aw_msg->insert_default_selection(sel, "", "");
        aw_msg->update_selection_list(sel);
    }
    else {
        aw_msg->set_window_title(title);
    }
    aw_msg->window_fit();

    // update the selection box :
    aw_assert(sel);
    aw_msg->clear_selection_list(sel);
    if (value_list) {
        char *values = strdup(value_list);
        char *word;

        for (word = strtok(values, ";"); word; word = strtok(0, ";")) {
            aw_msg->insert_selection(sel, word, word);
        }
        free(values);
    }
    aw_msg->insert_default_selection(sel, "<new>", "");
    aw_msg->update_selection_list(sel);

    // do modal loop :
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;

    char *last_input = root->awar(AW_INPUT_AWAR)->read_string();
    while (aw_input_cb_result == dummy) {
        root->process_events();

        char *this_input = root->awar(AW_INPUT_AWAR)->read_string();
        if (strcmp(this_input, last_input) != 0) {
            if (check_fun) {
                char *corrected_input = check_fun(this_input);
                if (corrected_input) {
                    if (strcmp(corrected_input, this_input) != 0) {
                        root->awar(AW_INPUT_AWAR)->write_string(corrected_input);
                    }
                    free(corrected_input);
                }
            }
            reassign(last_input, this_input);
        }
        free(this_input);

        if (!aw_msg->is_shown()) { // somebody hided/closed the window
            input_cb(aw_msg, (AW_CL)-1); // CANCEL
            break;
        }
    }

    free(last_input);

    root->disable_callbacks = false;
    aw_msg->hide();

    return aw_input_cb_result;
}

char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    // params see aw_string_selection
    // default_value is taken from and result is written back to awar 'awar_name'

    AW_root *aw_root       = AW_root::THIS;
    AW_awar *awar          = aw_root->awar(awar_name);
    char    *default_value = awar->read_string();
    char    *result        = aw_string_selection(title, prompt, default_value, value_list, buttons, check_fun);

    awar->write_string(result);
    free(default_value);

    return result;
}

// --------------------------
//      aw_file_selection

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    AW_root *root = AW_root::THIS;

    static AW_window_simple *aw_msg = 0;
    if (!aw_msg) create_fileSelection_awars(root);

    {
        char *edir      = GBS_eval_env(dir);
        char *edef_name = GBS_eval_env(def_name);

        root->awar(AW_FILE_SELECT_TITLE_AWAR) ->write_string(title);
        root->awar(AW_FILE_SELECT_DIR_AWAR)   ->write_string(edir);
        root->awar(AW_FILE_SELECT_FILE_AWAR)  ->write_string(edef_name);
        root->awar(AW_FILE_SELECT_FILTER_AWAR)->write_string(suffix);

        free(edef_name);
        free(edir);
    }

    if (!aw_msg) {
        aw_msg = new AW_window_simple;

        aw_msg->init(root, "AW_FILE_SELECTION", "File selection");
        aw_msg->allow_delete_window(false); // disable closing the window

        aw_msg->load_xfig("fileselect.fig");

        aw_msg->at("title");
        aw_msg->create_button(0, AW_FILE_SELECT_TITLE_AWAR);

        awt_create_selection_box(aw_msg, AW_FILE_SELECT_BASE);

        aw_msg->button_length(7);

        aw_msg->at("ok");
        aw_msg->callback     (file_selection_cb, 0);
        aw_msg->create_button("OK", "OK", "O");

        aw_msg->at("cancel");
        aw_msg->callback     (file_selection_cb, -1);
        aw_msg->create_button("CANCEL", "CANCEL", "C");

        aw_msg->window_fit();
    }

    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    return aw_input_cb_result;
}

// --------------------
//      help window

struct aw_help_global_struct {
    AW_window *aww;
    AW_selection_list   *upid;
    AW_selection_list   *downid;
    char    *history;
}   aw_help_global;

static char *get_full_qualified_help_file_name(const char *helpfile, bool path_for_edit = false) {
    GB_CSTR   result             = 0;
    char     *user_doc_path      = strdup(GB_getenvDOCPATH());
    char     *devel_doc_path     = strdup(GB_path_in_ARBHOME("HELP_SOURCE/oldhelp", NULL));
    size_t    user_doc_path_len  = strlen(user_doc_path);
    size_t    devel_doc_path_len = strlen(devel_doc_path);

    const char *rel_path = 0;
    if (strncmp(helpfile, user_doc_path, user_doc_path_len) == 0 && helpfile[user_doc_path_len] == '/') {
        rel_path = helpfile+user_doc_path_len+1;
    }
    else if (strncmp(helpfile, devel_doc_path, devel_doc_path_len) == 0 && helpfile[devel_doc_path_len] == '/') {
        aw_assert(0);            // when does this happen ? never ?
        rel_path = helpfile+devel_doc_path_len+1;
    }

    if (helpfile[0]=='/' && !rel_path) {
        result = GBS_global_string("%s", helpfile);
    }
    else {
        if (!rel_path) rel_path = helpfile;

        if (rel_path[0]) {
            if (path_for_edit) {
#if defined(DEBUG)
                char *gen_doc_path = strdup(GB_path_in_ARBHOME("HELP_SOURCE/genhelp", NULL));

                char *devel_source = GBS_global_string_copy("%s/%s", devel_doc_path, rel_path);
                char *gen_source   = GBS_global_string_copy("%s/%s", gen_doc_path, rel_path);

                int devel_size = GB_size_of_file(devel_source);
                int gen_size   = GB_size_of_file(gen_source);

                aw_assert(devel_size <= 0 || gen_size <= 0); // only one of them shall exist

                if (gen_size>0) {
                    result = GBS_global_string("%s", gen_source); // edit generated doc
                }
                else {
                    result = GBS_global_string("%s", devel_source); // use normal help source (may be non-existing)
                }

                free(gen_source);
                free(devel_source);
                free(gen_doc_path);
#else
                result = GBS_global_string("%s/%s", GB_getenvDOCPATH(), rel_path); // use real help file in RELEASE
#endif // DEBUG
            }
            else {
                result = GBS_global_string("%s/%s", GB_getenvDOCPATH(), rel_path);
            }
        }
        else {
            result = "";
        }
    }

    free(devel_doc_path);
    free(user_doc_path);

    return strdup(result);
}

static char *get_full_qualified_help_file_name(AW_root *awr, bool path_for_edit = false) {
    char *helpfile = awr->awar(AWAR_HELPFILE)->read_string();
    char *qualified = get_full_qualified_help_file_name(helpfile, path_for_edit);
    free(helpfile);
    return qualified;
}

static char *get_local_help_url(AW_root *awr) {
    char   *helpfile          = get_full_qualified_help_file_name(awr, false);
    char   *user_doc_path     = strdup(GB_getenvDOCPATH());
    char   *user_htmldoc_path = strdup(GB_getenvHTMLDOCPATH());
    size_t  user_doc_path_len = strlen(user_doc_path);
    char   *result            = 0;

    if (strncmp(helpfile, user_doc_path, user_doc_path_len) == 0) { // "real" help file
        result            = GBS_global_string_copy("%s%s_", user_htmldoc_path, helpfile+user_doc_path_len);
        size_t result_len = strlen(result);

        aw_assert(result_len > 5);

        if (strcmp(result+result_len-5, ".hlp_") == 0) {
            strcpy(result+result_len-5, ".html");
        }
        else {
            freenull(result);
            GB_export_error("Can't browse that file type.");
        }
    }
    else { // on-the-fly-generated help file (e.g. search help)
        GB_export_error("Can't browse temporary help node");
    }

    free(user_htmldoc_path);
    free(user_doc_path);
    free(helpfile);

    return result;
}

static void aw_help_edit_help(AW_window *aww) {
    char *helpfile = get_full_qualified_help_file_name(aww->get_root(), true);

    if (GB_size_of_file(helpfile)<=0) {
#if defined(DEBUG)
        const char *base = GB_path_in_ARBHOME("HELP_SOURCE/oldhelp", NULL);
#else
        const char *base = GB_path_in_ARBLIB("help", NULL);
#endif // DEBUG

        const char *copy_cmd = GBS_global_string("cp %s/FORM.hlp %s", base, helpfile); // def_hlp_res("FORM.hlp"); (see check_ressources.pl)
        printf("[Executing '%s']\n", copy_cmd);
        system(copy_cmd);
    }

    AWT_edit(helpfile);

    free(helpfile);
}

static char *aw_ref_to_title(char *ref) {
    if (!ref) return 0;

    if (GBS_string_matches(ref, "*.ps", GB_IGNORE_CASE)) {   // Postscript file
        return GBS_global_string_copy("Postscript: %s", ref);
    }

    char *result = 0;
    char *file = 0;
    {
        char *helpfile = get_full_qualified_help_file_name(ref);
        file = GB_read_file(helpfile);
        free(helpfile);
    }

    if (file) {
        result = GBS_string_eval(file, "*\nTITLE*\n*=*2:\t=", 0);
        if (strcmp(file, result)==0) freenull(result);
        free(file);
    }
    else {
        GB_clear_error();
    }

    if (result==0) {
        result = strdup(ref);
    }

    return result;
}

static void aw_help_select_newest_in_history(AW_root *aw_root) {
    char *history = aw_help_global.history;
    if (history) {
        const char *sep      = strchr(history, '#');
        char       *lastHelp = sep ? GB_strpartdup(history, sep-1) : strdup(history);

        aw_root->awar(AWAR_HELPFILE)->write_string(lastHelp);
        free(lastHelp);
    }
}

static void aw_help_back(AW_root *aw_root) {
    char *history = aw_help_global.history;
    if (history) {
        const char *sep = strchr(history, '#');
        if (sep) {
            char *first = GB_strpartdup(history, sep-1);

            freeset(aw_help_global.history, GBS_global_string_copy("%s#%s", sep+1, first)); // wrap first to end
            free(first);
            aw_help_select_newest_in_history(aw_root);
        }
    }
}
static void aw_help_back(AW_window *aww) { aw_help_back(aww->get_root()); }

static GB_ERROR aw_help_show_external_format(const char *help_file, const char *viewer) {
    // Called to show *.ps or *.pdf in external viewer.
    // Can as well show *.suffix.gz (decompresses to temporary *.suffix)

    struct stat st;
    GB_ERROR    error = NULL;
    char        sys[1024];

    sys[0] = 0;

    if (stat(help_file, &st) == 0) { // *.ps exists
        GBS_global_string_to_buffer(sys, sizeof(sys), "%s %s &", viewer, help_file);
    }
    else {
        char *compressed = GBS_global_string_copy("%s.gz", help_file);

        if (stat(compressed, &st) == 0) { // *.ps.gz exists
            char *name_ext;
            GB_split_full_path(compressed, NULL, NULL, &name_ext, NULL);
            // 'name_ext' contains xxx.ps or xxx.pdf
            char *name, *suffix;
            GB_split_full_path(name_ext, NULL, NULL, &name, &suffix);

            char *tempname     = GB_unique_filename(name, suffix);
            char *uncompressed = GB_create_tempfile(tempname);

            GBS_global_string_to_buffer(sys, sizeof(sys),
                                        "(gunzip <%s >%s ; %s %s ; rm %s) &",
                                        compressed, uncompressed,
                                        viewer, uncompressed,
                                        uncompressed);

            free(uncompressed);
            free(tempname);
            free(name);
            free(suffix);
            free(name_ext);
        }
        else {
            error = GBS_global_string("Neither %s nor %s exists", help_file, compressed);
        }
        free(compressed);
    }

    if (sys[0] && !error) error = GB_system(sys);

    return error;
}

static void aw_help_helpfile_changed_cb(AW_root *awr) {
    char *help_file = get_full_qualified_help_file_name(awr);

    if (!strlen(help_file)) {
        awr->awar(AWAR_HELPTEXT)->write_string("no help");
    }
    else if (GBS_string_matches(help_file, "*.ps", GB_IGNORE_CASE)) { // Postscript file
        GB_ERROR error = aw_help_show_external_format(help_file, GB_getenvARB_GS());
        if (error) aw_message(error);
        aw_help_select_newest_in_history(awr);
    }
    else if (GBS_string_matches(help_file, "*.pdf", GB_IGNORE_CASE)) { // PDF file
        GB_ERROR error = aw_help_show_external_format(help_file, GB_getenvARB_PDFVIEW());
        if (error) aw_message(error);
        aw_help_select_newest_in_history(awr);
    }
    else {
        if (aw_help_global.history) {
            if (strncmp(help_file, aw_help_global.history, strlen(help_file)) != 0) {
                // remove current help from history (if present) and prefix it to history
                char *comm = GBS_global_string_copy("*#%s*=*1*2:*=%s#*1", help_file, help_file);
                char *h    = GBS_string_eval(aw_help_global.history, comm, 0);

                aw_assert(h);
                freeset(aw_help_global.history, h);
                free(comm);
            }
        }
        else {
            aw_help_global.history = strdup(help_file);
        }

        char *helptext = GB_read_file(help_file);
        if (helptext) {
            char *ptr;
            char *h, *h2, *tok;

            ptr = strdup(helptext);
            aw_help_global.aww->clear_selection_list(aw_help_global.upid);
            h2 = GBS_find_string(ptr, "\nUP", 0);
            while ((h = h2)) {
                h2 = GBS_find_string(h2+1, "\nUP", 0);
                tok = strtok(h+3, " \n\t");  // now I got UP
                char *title = aw_ref_to_title(tok);
                if (tok) aw_help_global.aww->insert_selection(aw_help_global.upid, title, tok);
                free(title);
            }
            free(ptr);
            aw_help_global.aww->insert_default_selection(aw_help_global.upid, "   ", "");
            aw_help_global.aww->update_selection_list(aw_help_global.upid);

            ptr = strdup(helptext);
            aw_help_global.aww->clear_selection_list(aw_help_global.downid);
            h2 = GBS_find_string(ptr, "\nSUB", 0);
            while ((h = h2)) {
                h2 = GBS_find_string(h2+1, "\nSUB", 0);
                tok = strtok(h+4, " \n\t");  // now I got SUB
                char *title = aw_ref_to_title(tok);
                if (tok) aw_help_global.aww->insert_selection(aw_help_global.downid, title, tok);
                free(title);
            }
            free(ptr);
            aw_help_global.aww->insert_default_selection(aw_help_global.downid, "   ", "");
            aw_help_global.aww->update_selection_list(aw_help_global.downid);

            ptr = GBS_find_string(helptext, "TITLE", 0);
            if (!ptr) ptr = helptext;
            ptr = GBS_string_eval(ptr, "{*\\:*}=*2", 0);

            awr->awar(AWAR_HELPTEXT)->write_string(ptr);
            free(ptr);
            free(helptext);
        }
        else {
            sprintf(AW_ERROR_BUFFER, "I cannot find the help file '%s'\n\n"
                    "Please help us to complete the ARB-Help by submitting\n"
                    "this missing helplink via ARB_NT/File/About/SubmitBug\n"
                    "Thank you.\n"
                    "\n"
                    "Details:\n"
                    "%s",
                    help_file, GB_await_error());
            awr->awar(AWAR_HELPTEXT)->write_string(AW_ERROR_BUFFER);
        }
    }
    free(help_file);
}

static void aw_help_browse(AW_window *aww) {
    char *help_url = get_local_help_url(aww->get_root());
    if (help_url) {
        awt_openURL(aww->get_root(), 0, help_url);
        free(help_url);
    }
    else {
        aw_message(GBS_global_string("Can't detect URL of help file\n(Reason: %s)", GB_await_error()));
    }
}

static void aw_help_search(AW_window *aww) {
    GB_ERROR  error      = 0;
    char     *searchtext = aww->get_root()->awar(AWAR_HELPSEARCH)->read_string();

    if (searchtext[0]==0) error = "Empty searchstring";
    else {
        char        *helpfilename = 0;
        static char *last_help; // tempfile containing last search result

        // replace all spaces in 'searchtext' by '.*'
        freeset(searchtext, GBS_string_eval(searchtext, " =.*", 0));

        // grep .hlp for occurrences of 'searchtext'.
        // write filenames of matching files into 'helpfilename'
        {
            {
                char *helpname = GB_unique_filename("arb", "hlp");
                helpfilename   = GB_create_tempfile(helpname);
                free(helpname);
            }

            if (!helpfilename) error = GB_await_error();
            else {
                const char *gen_help_tmpl = "cd %s;grep -i '^[^#]*%s' `find . -name \"*.hlp\"` | sed -e 'sI:.*IIg' -e 'sI^\\./IIg' | sort | uniq > %s";
                char       *gen_help_cmd  = GBS_global_string_copy(gen_help_tmpl, GB_getenvDOCPATH(), searchtext, helpfilename);

                error = GB_system(gen_help_cmd);

                free(gen_help_cmd);
                GB_remove_on_exit(helpfilename);
            }
        }

        if (!error) {
            char *result       = GB_read_file(helpfilename);
            if (!result) error = GB_await_error();
            else {
                // write temporary helpfile containing links to matches as subtopics

                FILE *helpfp       = fopen(helpfilename, "wt");
                if (!helpfp) error = GB_export_IO_error("writing helpfile", helpfilename);
                else {
                    fprintf(helpfp, "\nUP arb.hlp\n");
                    if (last_help) fprintf(helpfp, "UP %s\n", last_help);
                    fputc('\n', helpfp);

                    int   results = 0;
                    char *rp      = result;
                    while (1) {
                        char *eol = strchr(rp, '\n');
                        if (!eol) {
                            eol = rp;
                            while (*eol) ++eol;
                        }
                        if (eol>rp) {
                            char old = eol[0];
                            eol[0] = 0;
                            fprintf(helpfp, "SUB %s\n", rp);
                            results++;
                            eol[0] = old;
                        }
                        if (eol[0]==0) break; // all results inserted
                        rp = eol+1;
                    }

                    fprintf(helpfp, "\nTITLE\t\tResult of search for '%s'\n\n", searchtext);
                    if (results>0)  fprintf(helpfp, "\t\t%i results are shown as subtopics\n",  results);
                    else            fprintf(helpfp, "\t\tThere are no results.\n");

                    if (results>0) freedup(last_help, helpfilename);

                    fclose(helpfp);
                    aww->get_root()->awar(AWAR_HELPFILE)->write_string(helpfilename); // display results in helpwindow
                }
                free(result);
            }
        }
        free(helpfilename);
    }

    if (error) aw_message(error);

    free(searchtext);
}

void AW_POPUP_HELP(AW_window *aw, AW_CL /* char */ helpcd) {
    static AW_window_simple *helpwindow = 0;

    AW_root *awr       = aw->get_root();
    char    *help_file = (char*)helpcd;

    if (!helpwindow) {
        awr->awar_string(AWAR_HELPTEXT,   "", AW_ROOT_DEFAULT);
        awr->awar_string(AWAR_HELPSEARCH, "", AW_ROOT_DEFAULT);
        awr->awar_string(AWAR_HELPFILE,   "", AW_ROOT_DEFAULT);
        awr->awar(AWAR_HELPFILE)->add_callback(aw_help_helpfile_changed_cb);

        helpwindow = new AW_window_simple;
        helpwindow->init(awr, "HELP", "HELP WINDOW");
        helpwindow->load_xfig("help.fig");

        helpwindow->button_length(10);

        helpwindow->at("close");
        helpwindow->callback((AW_CB0)AW_POPDOWN);
        helpwindow->create_button("CLOSE", "CLOSE", "C");

        helpwindow->at("back");
        helpwindow->callback(aw_help_back);
        helpwindow->create_button("BACK", "BACK", "B");


        helpwindow->at("super");
        aw_help_global.upid = helpwindow->create_selection_list(AWAR_HELPFILE, 0, 0, 3, 3);
        helpwindow->insert_default_selection(aw_help_global.upid, "   ", "");
        helpwindow->update_selection_list(aw_help_global.upid);

        helpwindow->at("sub");
        aw_help_global.downid = helpwindow->create_selection_list(AWAR_HELPFILE, 0, 0, 3, 3);
        helpwindow->insert_default_selection(aw_help_global.downid, "   ", "");
        helpwindow->update_selection_list(aw_help_global.downid);
        aw_help_global.aww = helpwindow;
        aw_help_global.history = 0;

        helpwindow->at("text");
        helpwindow->create_text_field(AWAR_HELPTEXT, 3, 3);

        helpwindow->at("browse");
        helpwindow->callback(aw_help_browse);
        helpwindow->create_button("BROWSE", "BROWSE", "B");

        helpwindow->at("expression");
        helpwindow->create_input_field(AWAR_HELPSEARCH);

        helpwindow->at("search");
        helpwindow->callback(aw_help_search);
        helpwindow->create_button("SEARCH", "SEARCH", "S");

        helpwindow->at("edit");
        helpwindow->callback(aw_help_edit_help);
        helpwindow->create_button("EDIT", "EDIT", "E");

    }

    aw_assert(help_file);

    awr->awar(AWAR_HELPFILE)->write_string(help_file);

    if (!GBS_string_matches(help_file, "*.ps", GB_IGNORE_CASE) &&
        !GBS_string_matches(help_file, "*.pdf", GB_IGNORE_CASE))
    { // don't open help if postscript or pdf file
        helpwindow->activate();
    }
}


#if defined(DEVEL_RALF)
#warning Check where AW_ERROR is used and maybe use one of the GB_error/terminate functions
#endif // DEVEL_RALF

void AW_ERROR(const char *templat, ...) {
    char buffer[10000];
    va_list parg;
    char *p;
    sprintf(buffer, "Internal ARB Error [AW]: ");
    p = buffer + strlen(buffer);

    va_start(parg, templat);

    vsprintf(p, templat, parg);
    fprintf(stderr, "%s\n", buffer);

    aw_message(buffer);
    aw_assert(0);
}
