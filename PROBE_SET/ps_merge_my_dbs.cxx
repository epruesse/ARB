#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif
#ifndef PS_NODE_HXX
#include "ps_node.hxx"
#endif

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    if (argc < 3) {
        printf("Missing arguments\n Usage %s <output database name> <input database name> <input database name2> [[input3]...]\n",argv[0]);
        exit(1);
    }

    //
    // open probe-set-database
    //
    const char    *input_DB_name = argv[2];
    printf( "Opening 1st input-probe-set-database '%s'..\n", input_DB_name );
    PS_Node       *root          = new PS_Node(-1);
    PS_FileBuffer *ps_db_fb      = new PS_FileBuffer( input_DB_name, PS_FileBuffer::READONLY );
    root->load( ps_db_fb );
    for (int i = 3; i < argc; ++i) {
        input_DB_name = argv[i];
        printf( "Appending input-probe-set-database '%s'..\n", input_DB_name );
        ps_db_fb->reinit( input_DB_name,PS_FileBuffer::READONLY );
        root->append( ps_db_fb );
    }
    printf( "loaded databases (enter to continue)\n" );
//     getchar();

    //
    // write one big whole tree to file
    //
    const char *output_DB_name = argv[1];
    printf( "Writing output-probe-set-database '%s'..\n",output_DB_name );
    ps_db_fb->reinit( output_DB_name, PS_FileBuffer::WRITEONLY );
    root->save( ps_db_fb );
    printf( "(enter to continue)\n" );
//     getchar();

//     if (argc < 3) { // no output filename specified => making statistics
//         printf( "making statistics...\n" );
//         PS_make_stats( root,0 );
//         printf( "database : %s\n",argv[1] );
//         printf( "total_node_counter       %15lu\n", total_node_counter );
//         printf( "total_child_counter      %15lu => %lu 1st level nodes\n", total_child_counter, total_node_counter-total_child_counter );
//         printf( "no_probes_node_counter   %15lu => %lu nodes with probes\n", no_probes_node_counter, total_node_counter-no_probes_node_counter );
//         printf( "no_children_node_counter %15lu (leaves)\n", no_children_node_counter );
//         printf( "total_probe_counter      %15lu => %5.2f probes/node with probes\n", total_probe_counter,((double) total_probe_counter) / (total_node_counter-no_probes_node_counter)  );
//         printf( "max_depth                %15lu\n", max_depth );
//         printf( "  depth    nodes/depth probes/depth\n" );
//         printf( "(enter to continue)\n" );
//         getchar();
//         for ( IntIntMap::iterator i=nodes_per_depth.begin(); i!=nodes_per_depth.end(); ++i) {
//             printf( "[ %6lu ] %6lu      %6lu\n", i->first, i->second,probes_per_depth[i->first] );
//         }
//         printf( "(enter to continue)\n" );
//         getchar();
//     }


    delete ps_db_fb;
//     printf( "root should be destroyed now\n" );
//     printf( "(enter to continue)\n" );
//     getchar();

//     nodes_per_depth.clear();
//     probes_per_depth.clear();

    return 0;
}
