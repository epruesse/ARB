#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <set>
#include "ps_node.hxx"
#include "ps_pg_tree_functions.cxx"


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
    for ( IntIntMap::iterator i=nodes_per_depth.begin(); i!=nodes_per_depth.end(); ++i) {
      printf( "[ %6lu ] %6lu      %6lu\n", i->first, i->second,probes_per_depth[i->first] );
    }
    printf( "(enter to continue)\n" );
    getchar();
  }

  if (argc >= 3) { // output and input filenames given => merge databases
    printf( "T O D O\n" );
    printf( "(enter to continue)\n" );
    getchar();
  }


  root.SetNull();
  printf( "root should be destroyed now\n" );
  printf( "(enter to continue)\n" );
  getchar();
  return 0;
}


