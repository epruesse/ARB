
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif

// common globals
SpeciesID          __MAX_ID;
SpeciesID          __MIN_ID;
PS_BitMap_Counted *__MAP;
IDVector          *__PATH;
IDVector          *__INVERSE_PATH;
IDSet             *__PATHSET;

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

   // open probe-set-database
    if (argc < 3) {
        printf( "Missing argument\n Usage %s <database name> <result filename>\n ", argv[0] );
        exit( 1 );
    }

    const char *input_DB_name   = argv[1];
    const char *result_filename = argv[2];

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
    __MIN_ID = db->getMinID();
    printf( "loaded database (enter to continue)\n" );
//    getchar();

    printf( "Opening result file %s\n", result_filename );
    PS_FileBuffer *result_file = new PS_FileBuffer( result_filename, PS_FileBuffer::READONLY );
    long size;
    SpeciesID id1,id2;
    printf( "\nloading no matches : " );
    file->get_long( size );
    printf( "%li", size );
    for (long i=0; i < size; ++i) {
        if (i % 5 == 0) printf( "\n" );
        file->get_int( id1 );
        file->get_int( id2 );
        printf( "%5i %-5i   ", id1, id2 );
    }
    printf( "\n\nloading one matches : " );
    file->get_long( size );
    printf( "%li\n", size );
    long path_length;
    SpeciesID path_id;
    for (long i=0; i < size; ++i) {
        file->get_int( id1 );
        file->get_int( id2 );
        file->get_long( path_length );
        printf( "%5i %-5i path(%6li): ", id1, id2, path_length );
        while (path_length-- > 0) {
            file->get_int( path_id );
            printf( "%i ", path_id );
        }
        printf( "\n" );
    }
    printf( "loading preset bitmap...\n" );
    __MAP = new PS_BitMap_Counted( result_file );
    printf( "loaded result file (enter to continue)\n" );
//    getchar();

    printf( "(enter to continue)\n" );
//    getchar();
    
    printf( "cleaning up...\n" );
    delete __MAP;
    delete db;
    printf( "(enter to continue)\n" );
//    getchar();

    return 0;
}
