#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <vector>
#include "ps_node.hxx"
#include "ps_bitset.hxx"
#include "ps_bitmap.hxx"


//  ----------------------------------------------------
//      void PS_make_stats( PS_NodePtr ps_node, unsigned long int depth )
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
void PS_make_stats( PS_NodePtr ps_node, unsigned long int depth ) {

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
  if (ps_node->hasProbes()) {
    size_t count = ps_node->countProbes();
    total_probe_counter     += count;
    probes_per_depth[depth] += count;
  } else {
    ++no_probes_node_counter;
  }

  // child(ren)
  if (ps_node->hasChildren()) {
    for (PS_NodeMapIterator i=ps_node->getChildrenBegin(); i!=ps_node->getChildrenEnd(); ++i) {
      ++total_child_counter;
      PS_make_stats( i->second, depth+1 );
    }
  } else {
    ++no_children_node_counter;
  }
}


//  ----------------------------------------------------
//      void PS_detect_weak_differences( const PS_NodePtr ps_root_node )
//  ----------------------------------------------------
//  recursively walk through database and make a triangle bool-matrix
//  of SpeciesID's where true means that the 2 species can be distinguished
//  by a probe
//
typedef vector<SpeciesID> IDvector;

void PS_detect_weak_differences_stepdown( const PS_NodePtr ps_node, PS_BitMap *the_map, IDvector &upper_nodes, SpeciesID &max_ID ) {
    
    //
    // store maximum SpeciesID
    //
    SpeciesID nodeNum = ps_node->getNum();
    if (nodeNum > max_ID) max_ID = nodeNum;

    //
    // append node to upper-nodes-list
    //
    if (!ps_node->hasProbes()) nodeNum = -nodeNum;               // negative nodeNums indicate an empty probes-list in a node
    upper_nodes.push_back( nodeNum );

    //
    // step down the children
    //
    for (PS_NodeMapConstIterator i = ps_node->getChildrenBegin(); i != ps_node->getChildrenEnd(); ++i) {
        PS_detect_weak_differences_stepdown( i->second, the_map, upper_nodes, max_ID );
    }

    //
    // remove node from upper
    //
    upper_nodes.pop_back();

    //
    // set value in the map : true for all (upper-nodes-id,nodeNum) pairs
    //
    // rbegin() <-> walk from end to start
    IDvector::reverse_iterator upperIndex = upper_nodes.rbegin();
    // skip empty nodes at the end of the upper_nodes - list
    // because they have no probe to distinguish them from us
    for ( ; upperIndex != upper_nodes.rend(); ++upperIndex ) {
        if (*upperIndex >= 0) break;
    }
    // continue with the rest of the list
    if (nodeNum < 0) nodeNum = -nodeNum;
    for ( ; upperIndex != upper_nodes.rend(); ++upperIndex) {
        the_map->triangle_set( nodeNum, abs(*upperIndex), true );
        //printf( "(%i|%i) ", nodeNum, *upperIndex );
    }
    //if (nodeNum % 200 == 0) printf( "%i\n",nodeNum );
}

void PS_detect_weak_differences( const PS_NodePtr ps_root_node ) {
    PS_BitMap *theMap = new PS_BitMap( false );
    SpeciesID  maxID  = 0;
    IDvector   upperNodes;
    
    for (PS_NodeMapConstIterator i = ps_root_node->getChildrenBegin(); i != ps_root_node->getChildrenEnd(); ++i ) {
        printf( "PS_detect_weak_differences_stepdown( %i, theMap, upperNodes, %i )\n",i->first,maxID );
        PS_detect_weak_differences_stepdown( i->second, theMap, upperNodes, maxID );
        if (!upperNodes.empty()) {
            printf( "unclean ids :" );
            for (IDvector::iterator id = upperNodes.begin(); id != upperNodes.end(); ++id ) {
                printf( " %i",*id );
            }
            printf( "\n" );
            upperNodes.clear();
        }
    }
    printf( "max ID = %i\n(enter to continue)",maxID );
    //getchar();

    theMap->print();
    printf( "(enter to continue)\n" );
    //getchar();

    delete theMap;
    printf( "(enter to continue)\n" );
    //getchar();
}


//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    GB_ERROR error;

    printf( "(enter to continue)\n" );
    //getchar();

    // open probe-set-database
    if (argc < 2) {
        printf("Missing arguments\n Usage %s <database name> [output filename]\n",argv[0]);
        exit(1);
    }

    const char *input_DB_name = argv[1];

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    int ps_db = open( input_DB_name, O_RDONLY );
    if (ps_db == -1) {
        if (!error) error = GB_export_error( "Can't open database '%s'", input_DB_name );
        exit(1);
    }
    PS_NodePtr root(new PS_Node(-1));
    if (!root->load( ps_db )) {
        if (!error) error = GB_export_error( "error while reading database '%s'", input_DB_name );
        exit(2);
    }
    printf( "loaded database (enter to continue)\n" );
    //getchar();


    if (argc < 3) { // no output filename specified => making statistics
        printf( "making statistics...\n" );
        PS_make_stats( root,0 );
        printf( "database : %s\n",argv[1] );
        printf( "total_node_counter       %15lu\n", total_node_counter );
        printf( "total_child_counter      %15lu => %lu 1st level nodes\n", total_child_counter, total_node_counter-total_child_counter );
        printf( "no_probes_node_counter   %15lu => %lu nodes with probes\n", no_probes_node_counter, total_node_counter-no_probes_node_counter );
        printf( "no_children_node_counter %15lu (leaves)\n", no_children_node_counter );
        printf( "total_probe_counter      %15lu => %5.2f probes/node with probes\n", total_probe_counter,((double) total_probe_counter) / (total_node_counter-no_probes_node_counter)  );
        printf( "max_depth                %15lu\n", max_depth );
        printf( "  depth    nodes/depth probes/depth\n" );
        printf( "(enter to continue)\n" );
        getchar();
        for ( IntIntMap::iterator i=nodes_per_depth.begin(); i!=nodes_per_depth.end(); ++i) {
            printf( "[ %6lu ] %6lu      %6lu\n", i->first, i->second,probes_per_depth[i->first] );
        }
        printf( "(enter to continue)\n" );
        getchar();
    }

    if (argc >= 3) { // output and input filenames given => merge databases
        PS_detect_weak_differences( root );
        printf( "T O D O\n" );
        printf( "(enter to continue)\n" );
        //getchar();
    }


    root.SetNull();
    printf( "root should be destroyed now\n" );
    printf( "(enter to continue)\n" );
    //getchar();

    nodes_per_depth.clear();
    probes_per_depth.clear();

#ifdef TEST_SMART_COUNTERS
    printf("PS_Node::smartPtrCounter = %li\n", Counted<PS_Node>::smartPtrCounter);
    printf("PS_Probe::smartPtrCounter = %li\n", Counted<PS_Probe>::smartPtrCounter);
#endif
    return 0;
}

#ifdef TEST_SMART_COUNTERS
long Counted<PS_Node>::smartPtrCounter = 0;
long Counted<PS_Probe>::smartPtrCounter = 0;
#endif
