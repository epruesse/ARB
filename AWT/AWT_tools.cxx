// ============================================================ //
//                                                              //
//   File      : AWT_tools.cxx                                  //
//   Purpose   : misc tools                                     //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#include <awt.hxx>

#include <time.h>
#include <sys/time.h>

const char * AWT_date_string(void) {
    struct timeval date;
    struct tm *p;

    gettimeofday(&date, 0);

#if defined(DARWIN)
    struct timespec local;
    TIMEVAL_TO_TIMESPEC(&date, &local); // not avail in time.h of Linux gcc 2.95.3
    p = localtime(&local.tv_sec);
#else
    p = localtime(&date.tv_sec);
#endif // DARWIN

    char *readable = asctime(p); // points to a static buffer
    char *cr       = strchr(readable, '\n');
    awt_assert(cr);
    cr[0]          = 0;         // cut of \n

    return readable;
}

