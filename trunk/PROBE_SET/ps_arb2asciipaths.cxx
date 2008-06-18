#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>

#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif
#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif
#ifndef PS_PG_TREE_FUNCTIONS
#include "ps_pg_tree_functions.cxx"
#endif

//  **************************************************
//  GLOBALS
//  **************************************************
IDVector *__PATH = new IDVector;

//  ----------------------------------------------------
//      void PS_print_paths( GBDATA *_pb_node )
//  ----------------------------------------------------
//  recursively print the paths to the leaves
//
void PS_print_paths( GBDATA *_pb_node ) {

    // number and species name
    GBDATA     *data   = GB_entry( _pb_node, "num");
    const char *buffer = GB_read_char_pntr( data );
    SpeciesID   id     = atoi( buffer );
  
    // probe(s)
    GBDATA *pb_group = GB_entry( _pb_node, "group");
    if (!pb_group) {
        id = -id;
    }

    // path
    __PATH->push_back( id );

    // child(ren)
    GBDATA *pb_child = PS_get_first_node( _pb_node );
    if (pb_child) {
        while (pb_child) {
            PS_print_paths( pb_child );
            pb_child = PS_get_next_node( pb_child );
        }
    } else {
        // print path in leaf nodes
        printf( "[%6zu] ",__PATH->size() );
        for (IDVectorCIter i=__PATH->begin(); i != __PATH->end(); ++i ) {
            printf( "%6i ",*i );
        }
        printf( "\n" );
//      getchar();
    }

    // path
    __PATH->pop_back();
}


//  ====================================================
//  ====================================================
int main( int argc,
          char *argv[] ) {

    GBDATA   *pb_main = 0;
    GB_ERROR  error   = 0;

    // open probe-group-database
    if (argc < 2) {
        printf("Missing arguments\n Usage %s <input database name>\n",argv[0]);
        exit(1);
    }

    if (!error) {
        const char *input_DB_name = argv[1];
    
        printf( "Opening probe-group-database '%s'..", input_DB_name );
        pb_main = GB_open( input_DB_name, "rwcN" );//"rwch");
        if (!pb_main) {
            error             = GB_get_error();
            if (!error) error = GB_export_error( "Can't open database '%s'", input_DB_name );
        }
    }
    printf( "loaded database (enter to continue)\n" );
//  getchar();

    GB_transaction dummy(pb_main);
    GBDATA *group_tree = GB_entry( pb_main, "group_tree");
    if (!group_tree) {
        printf( "no 'group_tree' in database\n" );
        error = GB_export_error( "no 'group_tree' in database" );
        exit(1);
    }
    GBDATA *first_level_node = PS_get_first_node( group_tree );
    if (!first_level_node) {
        printf( "no 'node' found in group_tree\n" );
        error = GB_export_error( "no 'node' found in group_tree" );
        exit(1);
    }

    printf( "dumping probes...\n" );
    first_level_node = PS_get_first_node( group_tree );
    if (!first_level_node) {
        printf( "no 'node' found in group_tree\n" );
        error = GB_export_error( "no 'node' found in group_tree" );
    } else {
        printf( "starting with first toplevel nodes\n" );
        // print 1st level nodes (and its subtrees)
        do {
            PS_print_paths( first_level_node );
            first_level_node = PS_get_next_node( first_level_node );
        } while (first_level_node);
    }

    return 0;
}


