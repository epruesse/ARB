#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <set>
#include <vector>
//#define TEST_SMART_COUNTERS
#include "ps_node.hxx"
// #include "ps_pg_tree_functions.cxx"

//  ############################################################
//  # PS_BitSet
//  ############################################################
class PS_BitSet {
private:

    char *data;
    long  size;
    long  capacity;
    bool  bias;

public:
    bool get( const long index );
    void set( const long index, const bool value );

    bool reserve( const long _capacity );

    PS_BitSet();
    PS_BitSet( bool _bias ) {
        data     = 0;
        size     = -1;
        capacity = -1;
        bias     = _bias;
    };
    PS_BitSet( const PS_BitSet& );

    ~PS_BitSet() {
        if (data) free( data );
    }
};

void PS_BitSet::set( const long index, const bool value ) {
    reserve( index );
    char byte   = data[index / 8];
    char offset = index % 8;
    char mask   = 1 << offset;
    if (value == true) {
        data[index / 8] |= mask; 
    } else {
        data[index / 8] &= ~mask;
    }
}

bool PS_BitSet::get( const long index ) {
    reserve( index );
    char byte   = data[index / 8];
    char offset = index % 8;
    byte = (byte >> offset);
    return (byte & 1 == 1);
}

bool PS_BitSet::reserve( const long _capacity ) {
    char *new_data;
    if (_capacity < capacity) return false;                       // smaller size requested ?
    if (_capacity == capacity) return true;                       // same size requested ?
    new_data = (char *)malloc((_capacity >> 3)+1);                // get new memory
    if (new_data == 0) return false;
    memset(new_data,bias ? 0xFF : 0,_capacity);                   // set memory to bias value
    memcpy(new_data,data,capacity);                               // copy old values
    capacity = _capacity;                                         // store new capacity
    return true;
}


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
typedef map<SpeciesID,bit_vector> IDmap;
typedef vector<SpeciesID>         IDvector;

void PS_detect_weak_differences_stepdown( const PS_NodePtr ps_node, IDmap &theMap, IDvector &upper, IDvector &lower, SpeciesID &maxID ) {
    
    // store maximum SpeciesID
    if (ps_node->getNum() > maxID) maxID = ps_node->getNum();

    // append node to upper-list
    upper.push_back( ps_node->getNum() );

    // step down the children
    for (PS_NodeMapConstIterator i = ps_node->getChildrenBegin(); i != ps_node->getChildrenEnd(); ++i) {
        PS_detect_weak_differences_stepdown( i->second, theMap, upper, lower, maxID );
    }

    // move node from upper to lower
    upper.pop_back();
    lower.push_back( ps_node->getNum() );

    // set value in the map if node has probes : true for all upper-lower pairs
    if (ps_node->hasProbes()) {
        for (IDvector::iterator upper_it = upper.begin(); upper_it != upper.end(); ++upper_it) {
            // reserve space in the vector up to the highest index in lower, which is always the first element
            theMap[*upper_it].reserve( lower.front() );
            for (IDvector::iterator lower_it = lower.begin(); lower_it != lower.end(); ++lower_it) {
                theMap[*upper_it][*lower_it] = true;
            }
        }
    }
}

void PS_detect_weak_differences( const PS_NodePtr ps_root_node ) {
    IDmap     theMap;
    IDvector  upper;
    IDvector  lower;
    SpeciesID maxID = 0;
    
    for (PS_NodeMapConstIterator i = ps_root_node->getChildrenBegin(); i != ps_root_node->getChildrenEnd(); ++i ) {
        PS_detect_weak_differences_stepdown( i->second, theMap, upper, lower, maxID );
        upper.clear();
        lower.clear();
    }
    printf( "max ID = %i\n (enter to continue)",maxID );
    getchar();

    for (SpeciesID i = 0; i <= maxID; ++i) {
        printf( "[%6i] : ",i );
        IDmap::iterator m = theMap.find(i);
        if (m != theMap.end()) {
            int c = 0;
            for (bit_vector::iterator bit = m->second.begin(); bit != m->second.end(); ++bit, ++c) {
                printf( *bit ? "+" : "_" );
            }
            printf( " %i",c );
        } else {
            printf( "---" );
        }
        printf( "\n" );
    }
}


//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    GB_ERROR error;

    printf( "(enter to continue)\n" );
    getchar();

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
    getchar();


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
        getchar();
    }


    root.SetNull();
    printf( "root should be destroyed now\n" );
    printf( "(enter to continue)\n" );
    getchar();

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
