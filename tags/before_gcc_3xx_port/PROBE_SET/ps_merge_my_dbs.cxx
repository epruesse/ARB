#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif

void PS_print_time_diff( const struct tms *_since, const char *_before = 0, const char *_after = 0 ) {
    struct tms now;
    times( &now );
    if (_before) printf( "%s", _before );
    printf( "time used : user (" );
    unsigned int minutes = (now.tms_utime-_since->tms_utime)/CLK_TCK / 60;
    unsigned int hours   = minutes / 60; 
    minutes -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs) system (", (float)(now.tms_utime-_since->tms_utime)/CLK_TCK-(hours*3600)-(minutes*60) );
    minutes  = (now.tms_stime-_since->tms_stime)/CLK_TCK / 60;
    hours    = minutes / 60; 
    minutes -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs)",  (float)(now.tms_stime-_since->tms_stime)/CLK_TCK-(hours*3600)-(minutes*60) );
    if (_after) {
        printf( "%s", _after );
    } else {
        printf( "\n" );
    }
    fflush( stdout );
}

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    if (argc < 3) {
        printf( "Missing arguments\n Usage %s <output database name> <input database name> <input database name2> [[input3]...]\n", argv[0] );
        exit( 1 );
    }

    //
    // init database object
    //
    const char *input_DB_name = argv[2];
    struct tms before;
    times( &before );
    printf( "Opening 1st input-probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    PS_print_time_diff( &before, "(enter to continue)  " );
//    getchar();

    //
    // merge in other databasefiles
    //
    for (int i = 3; i < argc; ++i) {
        input_DB_name = argv[i];
        printf( "Appending input-probe-set-database '%s'..\n", input_DB_name );
        times( &before );
        db->merge( input_DB_name );
        PS_print_time_diff( &before );
    }
    printf( "Merged databases (enter to continue)\n" );
//    getchar();

    //
    // write one big whole tree to file
    //
    const char *output_DB_name = argv[1];
    times( &before );
    printf( "Writing output-probe-set-database '%s'..\n",output_DB_name );
    db->saveTo( output_DB_name );
    PS_print_time_diff( &before, "(enter to continue)  " );
//    getchar();

    printf( "cleaning up...\n" );
    if (db) delete db;
//     printf( "root should be destroyed now\n" );
//     printf( "(enter to continue)\n" );
//     getchar();

    return 0;
}
