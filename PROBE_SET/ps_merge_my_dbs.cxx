#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif

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
    printf( "Opening 1st input-probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );

    //
    // merge in other databasefiles
    //
    db->load();
    for (int i = 3; i < argc; ++i) {
        input_DB_name = argv[i];
        printf( "Appending input-probe-set-database '%s'..\n", input_DB_name );
        db->merge( input_DB_name );
    }
    printf( "Merged databases (enter to continue)\n" );
//     getchar();

    //
    // write one big whole tree to file
    //
    const char *output_DB_name = argv[1];
    printf( "Writing output-probe-set-database '%s'..\n",output_DB_name );
    db->saveTo( output_DB_name );
    printf( "(enter to continue)\n" );
//     getchar();


    if (db) delete db;
//     printf( "root should be destroyed now\n" );
//     printf( "(enter to continue)\n" );
//     getchar();

    return 0;
}
