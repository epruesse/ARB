#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <map>
#ifndef PS_NODE_HXX
#include "ps_node.hxx"
#endif
#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif
#ifndef PS_PG_TREE_FUNCTIONS
#include "ps_pg_tree_functions.cxx"
#endif

//  ----------------------------------------------------
//      void print_indented_str( int depth, int newline, int id, const char *str )
//  ----------------------------------------------------
void print_indented_str( int depth, int newline, int id, const char *str ) {
    printf( "%6u (%6u) ",depth,id );
    for ( int i = 0; i < depth; ++i ) printf( "  " );
    printf( (newline) ? "%s\n" : "%s", str );
}

//  ----------------------------------------------------
//      void PS_print_node( GBDATA *pb_node, int depth )
//  ----------------------------------------------------
//  recursively print the data of the node and its children
//
void PS_print_node( GBDATA *pb_node, int depth ) {

    // number and species name
    GBDATA     *data   = GB_find( pb_node, "num", 0, down_level );
    const char *buffer = GB_read_char_pntr( data );
    SpeciesID   id     = atoi( buffer );
  
    print_indented_str( depth, 0, id, PG_SpeciesID2SpeciesName( id ).c_str() );

    // probe(s)
    GBDATA *pb_group = GB_find( pb_node, "group", 0, down_level );
    if (pb_group) {
        printf( "\n" );
        data = PG_get_first_probe( pb_group );
        while (data) {
            print_indented_str( depth, 1, id, PG_read_probe( data ) );
            data = PG_get_next_probe( data );
        }
    } else {
        printf( " no probes\n" );
    }

    // child(ren)
    if (depth > 1) return;
    GBDATA *pb_child = PS_get_first_node( pb_node );
    while (pb_child) {
        PS_print_node( pb_child, depth+1 );
        pb_child = PS_get_next_node( pb_child );
    }
}


//  ----------------------------------------------------
//      void PS_make_stats( GBDATA *pb_node, unsigned long int depth )
//  ----------------------------------------------------
//  recursively walk through database and count
//
unsigned long int total_node_counter       = 0;
unsigned long int total_child_counter      = 0;
unsigned long int no_probes_node_counter   = 0;
unsigned long int no_children_node_counter = 0;
unsigned long int total_probe_counter      = 0;
unsigned long int max_depth                = 0;
typedef map<unsigned long int,unsigned long int> IntIntMap;
IntIntMap nodes_per_depth;
IntIntMap probes_per_depth;
void PS_make_stats( GBDATA *pb_node, unsigned long int depth ) {
    // depth
    if (depth > max_depth) max_depth = depth;

    // count this node
    ++total_node_counter;
    if (nodes_per_depth.find(depth) == nodes_per_depth.end()) {
        nodes_per_depth[depth] = 1;
    } else {
        ++nodes_per_depth[depth];
    }
  
    // probe(s)
    if (probes_per_depth.find(depth) == probes_per_depth.end()) {
        probes_per_depth[depth] = 0;
    }
    GBDATA *pb_group = GB_find( pb_node, "group", 0, down_level );
    if (pb_group) {
        GBDATA *data = PG_get_first_probe( pb_group );
        while (data) {
            ++total_probe_counter;
            ++probes_per_depth[depth];
            data = PG_get_next_probe( data );
        }
    } else {
        ++no_probes_node_counter;
    }
  
    // child(ren)
    GBDATA *pb_child = PS_get_first_node( pb_node );
    if (!pb_child) ++no_children_node_counter;
    while (pb_child) {
        ++total_child_counter;
        PS_make_stats( pb_child, depth+1 );
        pb_child = PS_get_next_node( pb_child );
    }
}


//  ----------------------------------------------------
//      void PS_detect_probe_length( GBDATA *pb_node )
//  ----------------------------------------------------
//  recursively walk through database to first probe and get its length
//
int PS_detect_probe_length( GBDATA *pb_node ) {
    int probe_length = -1;

    // search for a probe
    while (probe_length < 0) {
        GBDATA *pb_group = GB_find( pb_node, "group", 0, down_level );
        if (pb_group) {                                     // ps_node has probes
            GBDATA *probe = PG_get_first_probe( pb_group );
            probe_length = strlen(PG_read_probe(probe));
        } else {                                            // ps_node has no probes .. check its children
            GBDATA *pb_child = PS_get_first_node( pb_node );
            while (pb_child && (probe_length < 0)) {
                probe_length = PS_detect_probe_length( pb_child );
                pb_child = PS_get_next_node( pb_child );
            }
        }
    }
  
    return probe_length;
}


//  ----------------------------------------------------
//      unsigned long int PS_extract_probe_data( GBDATA *pb_node, int probe_length, PS_NodePtr ps_current_node )
//  ----------------------------------------------------
//  recursively walk through database and extract probe-data
//
typedef pair<unsigned long int,unsigned long int> PairUL;
PairUL PS_extract_probe_data( GBDATA *pb_node, const int probe_length, PS_NodePtr ps_current_node ) {
    unsigned long int double_probes   = 0;
    unsigned long int double_children = 0;

    //
    // number
    //
    GBDATA     *data   = GB_find( pb_node, "num", 0, down_level );
    const char *buffer = GB_read_char_pntr( data );
    SpeciesID   id     = atoi( buffer );

    PS_NodePtr new_node(new PS_Node(id));                             // make new node and set its id

    //
    // probe(s)
    //
    GBDATA *pb_group = GB_find( pb_node, "group", 0, down_level );    // access probe-group
    if (pb_group) {
        data = PG_get_first_probe( pb_group );                        // get first probe if exists

        while (data) {
            buffer                = PG_read_probe( data );            // get probe string
            PS_ProbePtr new_probe(new PS_Probe);                      // make new probe
            new_probe->length     = probe_length;                     // set probe length
            new_probe->quality    = 100;                              // set probe quality    
            new_probe->GC_content = 0;                                // eval probe for GC-content
            for (int i=0; i < probe_length; ++i) {
                if ((buffer[i] == 'C') || (buffer[i] == 'G')) ++(new_probe->GC_content);
            }
            if (!new_node->addProbe( new_probe )) ++double_probes;    // append probe to new node
            data = PG_get_next_probe( data );                         // get next probe
        }
    }

    if (!ps_current_node->addChild( new_node )) ++double_children;    // append new node to current node

    //
    // child(ren)
    //
    GBDATA *pb_child = PS_get_first_node( pb_node );                  // get first child if exists
    while (pb_child) {
        PairUL doubles(PS_extract_probe_data( pb_child, probe_length, new_node ));
        double_probes   += doubles.first;
        double_children += doubles.second;
        pb_child = PS_get_next_node( pb_child );
    }

    return PairUL(double_probes,double_children);
}


//  ====================================================
//  ====================================================
int main( int argc,  char *argv[] ) {

    GBDATA   *pb_main = 0;
    GB_ERROR  error   = 0;

    // open probe-group-database
    if (argc < 2) {
        printf("Missing arguments\n Usage %s <input database name> [<output database name> [output mapping-file name]]\n",argv[0]);
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
//    getchar();

    printf( "init Species <-> ID - Map\n" );
    PG_initSpeciesMaps(pb_main);
    int species_count = PG_NumberSpecies();
    printf( "%i species in the map ", species_count );
    if (species_count >= 10) {
        printf( "\nhere are the first 10 :\n" );
        int count = 0;
        for (Int2StringIter i=num2species_map.begin(); count<10; ++i,++count) {
            printf( "[ %2i ] %s\n", i->first, i->second.c_str() );
        }
    }
    printf( "(enter to continue)\n" );
//    getchar();

    GB_transaction dummy(pb_main);
    GBDATA *group_tree = GB_find( pb_main, "group_tree", 0, down_level );
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

    if (argc < 3) { // no output filename specified => making statistics
        printf( "making statistics...\n" );
        first_level_node = PS_get_first_node( group_tree );
        do {
            PS_make_stats( first_level_node,0 );
            first_level_node = PS_get_next_node( first_level_node );
        } while (first_level_node);
        printf( "database : %s\n",argv[1] );
        printf( "total_node_counter       %15lu\n", total_node_counter );
        printf( "total_child_counter      %15lu => %lu 1st level nodes\n", total_child_counter, total_node_counter-total_child_counter );
        printf( "no_probes_node_counter   %15lu => %lu nodes with probes\n", no_probes_node_counter, total_node_counter-no_probes_node_counter );
        printf( "no_children_node_counter %15lu (leaves)\n", no_children_node_counter );
        printf( "total_probe_counter      %15lu => %5.2f probes/node with probes\n", total_probe_counter,((double) total_probe_counter) / (total_node_counter-no_probes_node_counter)  );
        printf( "max_depth                %15lu\n", max_depth );
        printf( "  depth    nodes/depth probes/depth\n" );
        for ( IntIntMap::iterator i=nodes_per_depth.begin(); i!=nodes_per_depth.end(); ++i) {
            printf( "[ %6lu ] %6lu      %6lu\n", i->first, i->second,probes_per_depth[i->first] );
        }
        printf( "(enter to continue)\n" );
        getchar();
    }

    if (argc >= 3) { // output filenames given => extract probe-data and save to file
        //
        // stage 1 : write ID<->Name Map
        //
        if (argc > 3) {
            printf( "writing map file %s\n", argv[3] );
            int fdMap = open( argv[3], O_WRONLY | O_CREAT | O_EXCL , S_IRUSR | S_IWUSR );
            if (fdMap == -1) {
                printf( "error : %s already exists or can't be created\n",argv[3] );
            } else {
                ssize_t written;
                int     size;
                for (Int2StringIter i = num2species_map.begin(); i != num2species_map.end(); ++i) {
                    // write id
                    written = write( fdMap, &(i->first), sizeof(SpeciesID) );
                    if (written != sizeof(SpeciesID)) exit(1);
                    // write length of name
                    size = i->second.size();
                    written = write( fdMap, &size, sizeof(size) );
                    if (written != sizeof(size)) exit(1);
                    // write name
                    written = write( fdMap, i->second.c_str(), size);
                    if (written != size) exit(1);
                }
            }
            close( fdMap );
            printf( "(enter to continue)\n" );
            //        getchar();
        }
        //
        // stage 2 : extract data
        //
        printf( "extracting probe-data...\n" );
        int      probe_length = PS_detect_probe_length( group_tree );
        printf( "probe_length = %d\n",probe_length );
    
        PS_NodePtr  root = new PS_Node(-1);
        first_level_node = PS_get_first_node( group_tree );
        unsigned int first_level_node_counter = 0;
        unsigned long int double_probes   = 0;
        unsigned long int double_children = 0;
        do {
            if (first_level_node_counter % 200 == 0) printf( "1st level node #%u\n", first_level_node_counter );
            PairUL doubles   = PS_extract_probe_data( first_level_node, probe_length, root );
            double_probes   += doubles.first;
            double_children += doubles.second;
            ++first_level_node_counter;
            first_level_node = PS_get_next_node( first_level_node );
        } while (first_level_node);
        printf( "done after %u 1st level nodes\n",first_level_node_counter );
        printf( "there have been %lu probes and %lu children inserted double\n", double_probes, double_children );
        printf( "(enter to continue)\n" );
//        getchar();

        //
        // stage 3 : write tree to file
        //
        printf( "writing probe-data to %s\n",argv[2] );
        PS_FileBuffer *fb = new PS_FileBuffer( argv[2], PS_FileBuffer::WRITEONLY );
        root->save( fb );
        delete fb;

        root.Discard();
        printf( "(enter to continue)\n" );
//        getchar();

    }


    //   printf( "dumping probes...\n" );
    //   first_level_node = PS_get_first_node( group_tree );
    //   if (!first_level_node) {
    //     printf( "no 'node' found in group_tree\n" );
    //     error = GB_export_error( "no 'node' found in group_tree" );
    //   } else {
    //     printf( "starting with first toplevel nodes\n" );
    //     // print 1st level nodes (and its subtrees)
    //     do {
    //       PS_print_node( first_level_node,0 );
    //       first_level_node = PS_get_next_node( first_level_node );
    //     } while (first_level_node);
    //   }

    return 0;
}


