#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif
#ifndef PS_SPECIESMAP_HXX
#include "ps_speciesmap.hxx"
#endif
#ifndef PS_NODE_HXX
#include "ps_node.hxx"
#endif

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    //
    // check arguments
    //
    if (argc < 3) {
        printf("Missing arguments\n Usage %s <input database name> [input mapfile name] <output file name>\n",argv[0]);
        exit(1);
    }
    const char *input_DB_name  = argv[1];
    const char *input_MAP_name = argc > 3 ? argv[2] : "";
    const char *output_DB_name = argv[ argc == 3 ? 2 : 3 ];

    //
    // open probe-set files
    //
    PS_Node       *root = new PS_Node(-1);
    PS_FileBuffer *fb   = new PS_FileBuffer( input_MAP_name, PS_FileBuffer::READONLY );
    // load map
    printf( "Opening input probe-map-file '%s'..\n", input_MAP_name );
    PS_SpeciesMap speciesMap( fb );
    printf( "mapsize = %li\n",speciesMap.size()+1 );
    
    // load database
    printf( "Opening input probe-set-database '%s'..\n", input_DB_name );
    fb->reinit( input_DB_name, PS_FileBuffer::READONLY );
    root->load( fb );
    printf( "loaded database (enter to continue)\n" );
//     getchar();

    //
    // write as ASCII
    //
    fb->reinit( output_DB_name, PS_FileBuffer::WRITEONLY );
    char *buffer = (char *)malloc( 512 );
    int   count;
    // write map
    printf( "writing species-map to %s\n", output_DB_name );
    count = sprintf( buffer, "--------------------\nSPECIES MAP :\n--------------------\n%s\n--------------------\n", input_MAP_name );
    fb->put( buffer, count );
    speciesMap.saveASCII( fb, buffer );
    
    // write database
    printf( "writing probe-data to %s\n", output_DB_name );
    count = sprintf( buffer, "--------------------\nPROBE SET DATABASE :\n--------------------\n%s\n--------------------\n", input_DB_name );
    fb->put( buffer, count );
    root->saveASCII( fb, buffer );
    printf( "(enter to continue)\n" );
//     getchar();

    //
    // clean up
    //
    free( buffer );
    delete fb;
    printf( "(enter to continue)\n" );
//     getchar();

    return 0;
}
