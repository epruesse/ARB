//  ==================================================================== //
//                                                                       //
//    File      : ps_tools.cxx                                           //
//    Purpose   : remove duplicated code                                 //
//    Time-stamp: <Mon Oct/04/2004 17:50 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2004          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include "ps_tools.hxx"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/times.h>

// void PS_print_time_diff( const struct tms *_since ) {
//     struct tms now;
//     times( &now );
//     printf( "time used : user (" );
    
//     unsigned int minutes  = (now.tms_utime-_since->tms_utime)/CLOCKS_PER_SEC / 60;
//     unsigned int hours    = minutes / 60;
//     minutes              -= hours * 60;
//     if (hours > 0) printf( "%uh ", hours );
//     if (minutes > 0) printf( "%um ", minutes );
//     printf( "%.3fs) system (", (float)(now.tms_utime-_since->tms_utime)/CLOCKS_PER_SEC-(hours*3600)-(minutes*60) );

//     minutes  = (now.tms_stime-_since->tms_stime)/CLOCKS_PER_SEC / 60;
//     hours    = minutes / 60;
//     minutes -= hours * 60;
//     if (hours > 0) printf( "%uh ", hours );
//     if (minutes > 0) printf( "%um ", minutes );
//     printf( "%.3fs)\n",  (float)(now.tms_stime-_since->tms_stime)/CLOCKS_PER_SEC-(hours*3600)-(minutes*60) );

//     fflush(stdout);
// }

void PS_print_time_diff( const struct tms *_since, const char *_before, const char *_after) {
    struct tms now;
    times( &now );
    if (_before) printf( "%s", _before );
    printf( "time used : user (" );
    
    unsigned int minutes  = (now.tms_utime-_since->tms_utime)/CLOCKS_PER_SEC / 60;
    unsigned int hours    = minutes / 60;
    minutes              -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs) system (", (float)(now.tms_utime-_since->tms_utime)/CLOCKS_PER_SEC-(hours*3600)-(minutes*60) );
    minutes               = (now.tms_stime-_since->tms_stime)/CLOCKS_PER_SEC / 60;
    hours                 = minutes / 60;
    minutes              -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs)",  (float)(now.tms_stime-_since->tms_stime)/CLOCKS_PER_SEC-(hours*3600)-(minutes*60) );
    
    if (_after) {
        printf( "%s", _after );
    } else {
        printf( "\n" );
    }
    fflush( stdout );
}



