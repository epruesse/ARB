#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#ifndef PS_PG_TREE_FUNCTIONS
#include "ps_pg_tree_functions.cxx"
#endif

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

//  **************************************************
//  GLOBALS
//  **************************************************

char     *__ARB_DB_NAME    = 0;
GBDATA   *__ARB_DB         = 0;
GBDATA   *__ARB_GROUP_TREE = 0;
GB_ERROR  __ARB_ERROR      = 0;


//  ----------------------------------------------------
//      bool PS_get_probe_for_path( IDSet        &_path,
//                                  unsigned int  _GC_content,
//                                  unsigned int  _probe_length,
//                                  char         *_probe_data )
//  ----------------------------------------------------
bool PS_get_probe_for_path( IDSet        &_path,
                            unsigned int  _GC_content,
                            unsigned int  _probe_length,
                            char         *_probe_data ) {
    GB_transaction  dummy( __ARB_DB );
    SpeciesID       num;
    GBDATA         *data;
    GBDATA         *arb_node;
    GBDATA         *arb_group;
    IDSetCIter      id;
    unsigned int    gc_content;

    // iterate over path
    arb_node = __ARB_GROUP_TREE;
    for (  id  = _path.begin();
          (id != _path.end()) && arb_node;
         ++id ) {

        // search for arb-node with num == id
        arb_node = PS_get_first_node( arb_node ); if (!arb_node) break;
        data     = GB_find( arb_node, "num", 0, down_level );
        num      = atoi( GB_read_char_pntr( data ) );
        while (num != *id) {
            // get next node
            arb_node = PS_get_next_node( arb_node );
            if (!arb_node) break;
            // get num of arb-node
            data = GB_find( arb_node, "num", 0, down_level );
            num = atoi( GB_read_char_pntr( data ) );
        }
        if (!arb_node) break;
    }
    if (!arb_node) {
        printf( "  ERROR : failed to get node for ID (%i)\n", *id );
        return false;
    }

    // search for probe with GC-content == _GC_content
    arb_group = GB_find( arb_node, "group", 0, down_level );
    if (!arb_group) {
        printf( "  ERROR : failed to get group of node" );
        return false;
    }
    data = PG_get_first_probe( arb_group );
    if (!data) {
        printf( "  ERROR : failed to get first probe of group of node" );
        return false;
    }
    while (_probe_data[0] == '\x00') {
        const char *buffer = PG_read_probe( data );                             // read probe data
        gc_content = 0;
        for ( unsigned int i = 0; i < _probe_length; ++i ) {                    // calc GC-content
            if ((buffer[i] == 'C') || (buffer[i] == 'G')) ++gc_content;
        }
        if (gc_content == _GC_content) {                                        // found matching probe ?
            for ( unsigned int i = 0; i < _probe_length; ++i ) {                // store probe data
                _probe_data[i] = buffer[i];
            }
        } else {
            data = PG_get_next_probe( data );                                   // get next probe
            if (!data) break;
        }
    }
    if (!data) {
        printf( "  ERROR : failed to find probe with GC-content (%u)\n", _GC_content );
        return false;
    }
    
    return true;
}


//  ====================================================
//  ====================================================
int main( int   argc,
          char *argv[] ) {
    //
    // check arguments
    //
    if (argc < 3) {
        printf( "Missing argument\n Usage %s <final candidates paths filename> <prefix to arb-databases>\n", argv[0] );
        printf( "Example:\n %s ~/data/850.final_candidates.paths ~/data/ssjun03_Eucarya_850.pg_\n", argv[0] );
        exit( 1 );
    }

    char *final_candidates_paths_filename = argv[1];
    char *arb_db_name_prefix              = argv[2];
    char *temp_filename = (char *)malloc( strlen(final_candidates_paths_filename)+1+5 );
    strcpy( temp_filename, final_candidates_paths_filename );
    strcat( temp_filename, ".temp" );
    unlink( temp_filename );
    printf( "Opening temp-file '%s'..\n", temp_filename );
    PS_FileBuffer *temp__file = new PS_FileBuffer( temp_filename, PS_FileBuffer::WRITEONLY );

    //
    // candidates
    //
    printf( "Opening candidates-paths-file '%s'..\n", final_candidates_paths_filename );
    PS_FileBuffer *paths_file          = new PS_FileBuffer( final_candidates_paths_filename, PS_FileBuffer::READONLY );
    unsigned long  paths_todo;
    unsigned int   probe_length_todo   = 0;
    unsigned int   probe_buffer_length = 100;
    char          *probe_buffer        = 0;

    // read count of paths
    paths_file->get_ulong( paths_todo );
    temp__file->put_ulong( paths_todo );

    // read used probe lengths
    unsigned int      count;
    set<unsigned int> probe_lengths;
    paths_file->get_uint( count );
    temp__file->put_uint( count );
    printf( "probe lengths :" );
    for ( unsigned int i = 0; i < count; ++i ) {
        unsigned int length;
        paths_file->get_uint( length );
        temp__file->put_uint( length );
        probe_lengths.insert( length );
        printf( " %u", length );
    }
    printf( "\n" );

    // read candidates
    while (paths_todo) {
        printf( "\npaths todo (%lu)\n", paths_todo-- );
        unsigned int probe_length;
        unsigned int probe_GC_content;
        unsigned int path_length;
        IDSet        path;
        SpeciesID    id;

        // read one candidate
        paths_file->get_uint( probe_length );
        temp__file->put_uint( probe_length );
        paths_file->get_uint( probe_GC_content );
        temp__file->put_uint( probe_GC_content );
        printf( "  probe length (%u) GC (%u)\n", probe_length, probe_GC_content );
        paths_file->get_uint( path_length );
        temp__file->put_uint( path_length );
        printf( "  path size (%u) ( ", path_length );
        for ( unsigned int i = 0; i < path_length; ++i ) {
            paths_file->get_int( id );
            temp__file->put_int( id );
            path.insert( id );
            printf( "%i ", id );
        }
        printf( ")\n" );
        if (!probe_buffer || (probe_length > probe_buffer_length)) {
            if (probe_buffer) { // adjust buffer size
                free( probe_buffer );
                probe_buffer_length = 2 * probe_length;
            }
            probe_buffer = (char*)calloc( probe_buffer_length,sizeof(char) );
        }
        paths_file->get( probe_buffer, probe_length );
        probe_buffer[ probe_length ] = '\x00';

        // handle probe
        if (probe_buffer[0] == '\x00') {
            if (!probe_length_todo) {
                printf( "handling probes of length %u this time\n", probe_length );
                probe_length_todo = probe_length;
                // init. arb-db-filename
                // +1 for \0, +7 for 'tmp.arb', +5 for max number of digits of unsigned int
                __ARB_DB_NAME = (char*)malloc( strlen(arb_db_name_prefix)+1+7+5 );
                sprintf( __ARB_DB_NAME, "%s%utmp.arb", arb_db_name_prefix, probe_length );
                printf( "Opening ARB-Database '%s'..\n  ", __ARB_DB_NAME );
                __ARB_DB    = GB_open( __ARB_DB_NAME, "rN" );
                __ARB_ERROR = GB_get_error();
                if (__ARB_ERROR) {
                    printf( "%s\n", __ARB_ERROR );
                    return 1;
                }
                GB_transaction dummy( __ARB_DB );
                __ARB_GROUP_TREE = GB_find( __ARB_DB, "group_tree", 0, down_level );
                if (!__ARB_GROUP_TREE) {
                    printf( "no 'group_tree' in database\n" );
                    return 1;
                }
                GBDATA *first_level_node = PS_get_first_node( __ARB_GROUP_TREE );
                if (!first_level_node) {
                    printf( "no 'node' found in group_tree\n" );
                    return 1;
                }
            }
            if (probe_length_todo == probe_length) {
                if (!PS_get_probe_for_path( path, probe_GC_content, probe_length, probe_buffer )) {
                    delete temp__file;
                    return 1;
                }
                printf( "  probe data (%s)  ==> updated\n", probe_buffer );
            } else {
                printf( "  probe data (%s)  --> skipped\n", probe_buffer );
            }
        } else {
            printf( "  probe data (%s)  --> finished\n", probe_buffer );
        }
        temp__file->put( probe_buffer, probe_length );
    }
    probe_lengths.clear();
    paths_file->get_uint( count );
    for ( unsigned int i = 0; i < count; ++i ) {
        unsigned int length;
        paths_file->get_uint( length );
        if (length != probe_length_todo) probe_lengths.insert( length );
    }
    printf( "remaining probe lengths :" );
    temp__file->put_uint( probe_lengths.size() );
    for ( set<unsigned int>::iterator length = probe_lengths.begin();
          length != probe_lengths.end();
          ++length ) {
        temp__file->put_uint( *length );
        printf( " %u", *length );
    }
    printf( "\n" );
    if (probe_buffer) free( probe_buffer );
    if (__ARB_DB_NAME) free( __ARB_DB_NAME );

    printf( "cleaning up... temp-file\n" ); fflush( stdout );
    delete temp__file;
    printf( "cleaning up... candidates-paths-file\n" ); fflush( stdout );
    delete paths_file;
    printf( "moving temp-file to candiates-paths-file\n" ); fflush( stdout );
    rename( temp_filename, final_candidates_paths_filename );

    // exit code == 0 if all probe lengths handled
    // exit code == 1 if failure
    // exit code >= 2 else
    return probe_lengths.size()*2;
}
