// =============================================================== //
//                                                                 //
//   File      : AW_status.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awars.hxx>
#include <aw_window.hxx>
#include "aw_msg.hxx"
#include "aw_status.hxx"

#include <arbdbt.h>
#include <arb_strbuf.h>
#include <SigHandler.h>

#include <cerrno>
#include <cstdarg>
#include <ctime>
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

#define AW_MESSAGE_LINES        500

#if defined(DEBUG)

// ARB_LOGGING should always be undefined in SVN version!
// #define ARB_LOGGING
// #define TRACE_STATUS // // // enable debug output for status window (which runs forked!)
// #define TRACE_STATUS_MORE // // enable more debug output
// #define PIPE_DEBUGGING // // enable debug output for pipes (for write commands)

#endif // DEBUG

enum StatusCommand {
    // messages send from status-process to main-process :
    AW_STATUS_OK    = 0,
    AW_STATUS_ABORT = 1,
    // messages send from main-process to status-process :
    AW_STATUS_CMD_INIT,
    AW_STATUS_CMD_OPEN,
    AW_STATUS_CMD_CLOSE,
    AW_STATUS_CMD_NEW_TITLE,
    AW_STATUS_CMD_TEXT,
    AW_STATUS_CMD_GAUGE,
    AW_STATUS_CMD_MESSAGE
};

#define AW_EST_BUFFER 5

struct aw_stg_struct {
    int        fd_to[2];
    int        fd_from[2];
    bool       mode;
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
};

static aw_stg_struct aw_stg = {
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
    union {
        unsigned char buffer[sizeof(int)+1];
        int as_int;
    } input;

    erg = read(fd, input.buffer, sizeof(int));
    if (erg<=0) { // process died
        fprintf(stderr, "father died, now i kill myself\n");
        exit(EXIT_FAILURE);
    }
    return input.as_int;
}

static int aw_status_read_command(int fd, int poll_flag, char*& str, int *gaugePtr = 0)
{
    char buffer[1024];
    int  cmd = aw_status_read_byte(fd, poll_flag);

    if (cmd == AW_STATUS_CMD_TEXT ||
            cmd == AW_STATUS_CMD_OPEN ||
            cmd == AW_STATUS_CMD_NEW_TITLE ||
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


static unsigned aw_status_timer_hide_event(AW_root *) {
    if (aw_stg.hide) {
        aw_stg.aws->show();
        aw_stg.hide = 0;
    }
    return 0; // do not recall
}

static void aw_status_hide(AW_window *aws)
{
    aw_stg.hide = 1;
    aws->hide();

    // install timer event
    aws->get_root()->add_timed_callback(aw_stg.hide_delay*1000, makeTimedCallback(aw_status_timer_hide_event));

    // increase hide delay for next press of hide button
    if (aw_stg.hide_delay < (60*60)) { // max hide delay is 1 hour
        aw_stg.hide_delay *= 3; // initial: 60sec -> 3min -> 9min -> 27min -> 60min (max)
    }
    else {
        aw_stg.hide_delay = 60*60;
    }
}


static unsigned aw_status_timer_event(AW_root *awr)
{
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_status_timer_event\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE
    if (aw_stg.mode == AW_STATUS_ABORT) {
        int action = aw_question(NULL,
                                 "Couldn't quit properly in time.\n"
                                 "Now you can either\n"
                                 "- wait again (recommended),\n"
                                 "- kill the whole application(!) or\n"
                                 "- continue.",
                                 "Wait again,Kill application!,Continue");

        switch (action) {
            case 0:
                break;
            case 1: {
                char buf[255];
                sprintf(buf, "kill -9 %i", aw_stg.pid);
                aw_message_if(GBK_system(buf));
                exit(EXIT_SUCCESS);
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
    return 0; // do not recall
}

static void aw_status_kill(AW_window *aws)
{
    if (aw_stg.mode == AW_STATUS_ABORT) {
        aw_status_timer_event(aws->get_root());
        if (aw_stg.mode == AW_STATUS_OK) { // continue
            return;
        }
    }
    else {
        if (!aw_ask_sure("aw_status_kill", "Are you sure to abort running calculation?")) {
            return; // don't abort
        }
        aw_stg.mode = AW_STATUS_ABORT;
    }
    aw_status_write(aw_stg.fd_from[1], aw_stg.mode);

    if (aw_stg.mode == AW_STATUS_ABORT) {
#if defined(TRACE_STATUS_MORE)
        fprintf(stderr, "add aw_status_timer_event with delay = %i\n", AW_STATUS_KILL_DELAY); fflush(stdout);
#endif // TRACE_STATUS_MORE
        aws->get_root()->add_timed_callback(AW_STATUS_KILL_DELAY, makeTimedCallback(aw_status_timer_event)); // install timer event
    }
}

static void aw_refresh_tmp_message_display(AW_root *awr) {
    GBS_strstruct msgs(AW_MESSAGE_LINES*60);

    for (int i = AW_MESSAGE_LINES-1; i>=0; i--) {
        if (aw_stg.lines[i]) {
            msgs.cat(aw_stg.lines[i]);
            msgs.put('\n');
        }
    };

    awr->awar(AWAR_ERROR_MESSAGES)->write_string(msgs.get_data());

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
        ARB_alloc(copy, newsize);

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


static unsigned aw_status_timer_listen_event(AW_root *awr)
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

            case AW_STATUS_CMD_NEW_TITLE:
#if defined(TRACE_STATUS)
                fprintf(stderr, "received AW_STATUS_CMD_NEW_TITLE\n"); fflush(stdout);
#endif // TRACE_STATUS
#if defined(ARB_LOGGING)
                aw_status_append_to_log(str);
#endif // ARB_LOGGING
                awr->awar(AWAR_STATUS_TITLE)->write_string(str);
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
    aw_assert(delay>0);
    return delay;
}

void aw_clear_message_cb(AW_window *aww) {
    AW_root *awr = aww->get_root();
    for (int i = 0; i< AW_MESSAGE_LINES; i++) freenull(aw_stg.lines[i]);
    awr->awar(AWAR_ERROR_MESSAGES)->write_string("");
}

static void aw_clear_and_hide_message_cb(AW_window *aww) {
    aw_clear_message_cb(aww);
    AW_POPDOWN(aww);
}

static void create_status_awars(AW_root *aw_root) {
    aw_root->awar_string(AWAR_STATUS_TITLE,   "------------------------------------");
    aw_root->awar_string(AWAR_STATUS_TEXT,    "");
    aw_root->awar_string(AWAR_STATUS_GAUGE,   "------------------------------------");
    aw_root->awar_string(AWAR_STATUS_ELAPSED, "");
    aw_root->awar_string(AWAR_ERROR_MESSAGES, "");
}

void aw_initstatus() {
    // fork status window.
    // Note: call this function once as early as possible
    
    aw_assert(aw_stg.pid == 0);                     // do not init status twice!
    aw_assert(!AW_root::SINGLETON);                 // aw_initstatus has to be called before constructing AW_root
    aw_assert(GB_open_DBs() == 0);                  // aw_initstatus has to be called before opening the first ARB-DB

    int error = pipe(aw_stg.fd_to);
    if (error) GBK_terminate("Cannot create socketpair [1]");
    error = pipe(aw_stg.fd_from);
    if (error) GBK_terminate("Cannot create socketpair [2]");

    aw_stg.pid = getpid();
    GB_install_pid(1);
    pid_t clientid = fork();

    if (clientid) { // i am the father
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

        GB_shell shell;
        AW_root *aw_root = new AW_root("status.arb", "ARB_STATUS", true, new NullTracker, 0, NULL); // uses_pix_res("icons/ARB_STATUS.xpm"); see ../SOURCE_TOOLS/check_resources.pl@uses_pix_res
        create_status_awars(aw_root);

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
        aws->get_root()->add_timed_callback_never_disabled(30, makeTimedCallback(aw_status_timer_listen_event)); // use short delay for first callback

        // do NOT AWT_install_cb_guards here!
        aw_root->main_loop();                       // never returns
    }
}

static void status_write_cmd_and_text(StatusCommand cmd, const char *text, int textlen) {
    aw_status_write(aw_stg.fd_to[1], cmd);
    safe_write(aw_stg.fd_to[1], text, textlen+1);
}
static void status_write_cmd_and_text(StatusCommand cmd, const char *text) {
    if (!text) text = "";
    status_write_cmd_and_text(cmd, text, strlen(text));
}

void aw_openstatus(const char *title)
{
    aw_stg.mode = AW_STATUS_OK;
    if (!aw_stg.status_initialized) {
        aw_stg.status_initialized = true;
        aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
    }
    status_write_cmd_and_text(AW_STATUS_CMD_OPEN, title);
}

void aw_closestatus() {
    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_CLOSE);
}

bool AW_status(const char *text) {
    status_write_cmd_and_text(AW_STATUS_CMD_TEXT, text);
    return AW_status();
}

bool aw_status_title(const char *new_title) {
    status_write_cmd_and_text(AW_STATUS_CMD_NEW_TITLE, new_title);
    return AW_status();
}

bool AW_status(double gauge) {
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
    return AW_status();
}

bool AW_status() {
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

void aw_set_local_message() {
    aw_stg.local_message = 1;
}


void aw_message(const char *msg) {
    aw_assert(!RUNNING_TEST()); // aw_message hangs when called from nightly builds
#if defined(DEBUG)
    printf("aw_message: '%s'\n", msg);
#endif // DEBUG
    if (aw_stg.local_message) {
        aw_insert_message_in_tmp_message(AW_root::SINGLETON, msg);
    }
    else {
        if (!aw_stg.status_initialized) {
            aw_stg.status_initialized = true;
            aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
        }
        status_write_cmd_and_text(AW_STATUS_CMD_MESSAGE, msg);
    }
}

