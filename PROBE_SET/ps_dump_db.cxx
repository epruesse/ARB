#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <map>
#include "pg_search.hxx"
#include <smartptr.h>

using namespace std;

//***********************************************
//* PS_Probe
//***********************************************
typedef struct {
  unsigned int quality;
  unsigned int length;
  unsigned int GC_content;

} PS_Probe;

typedef SmartPtr<PS_Probe> PS_ProbePtr;

void PS_printProbe( PS_Probe *p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

void PS_printProbe( PS_ProbePtr p ) {
  printf("%u_%u_%u",p->quality,p->length,p->GC_content);
}

struct lt_probe
{
  bool operator()(PS_ProbePtr p1, PS_ProbePtr p2) const
  {
    //printf("\t");PS_printProbe(p1);printf(" < ");PS_printProbe(p2);printf(" ?\n");
    if (p1->quality == p2->quality) {
      if (p1->length == p2->length) {
        return (p1->GC_content < p2->GC_content); // equal quality & length => sort by GC-content
      } else {
        return (p1->length < p2->length);         // equal quality => sort by length
      }
    } else {
      return (p1->quality < p2->quality);         // sort by quality
    }
  }
};



//***********************************************
//* PS_ProbeSet
//***********************************************
typedef set<PS_ProbePtr,lt_probe>   PS_ProbeSet;
typedef PS_ProbeSet::const_iterator PS_ProbeSetIterator;

//***********************************************
//* PS_Node
//***********************************************
class PS_Node;
typedef SmartPtr<PS_Node>         PS_NodePtr;
typedef map<SpeciesID,PS_NodePtr> PS_NodeMap;
typedef PS_NodeMap::iterator      PS_NodeMapIterator;
typedef PS_NodeMap::const_iterator PS_NodeMapConstIterator;

class PS_Node {
private:

  SpeciesID   num;
  PS_NodeMap  children;
  PS_ProbeSet probes;

public:
  
  // *** num ***
  void      setNum( SpeciesID id ) { num = id;   }
  SpeciesID getNum() const { return num; }
  
  // *** children ***
  void addChild( PS_NodePtr& child ) {
    //printf("addChild[%d]\n",child->getNum());
    //PS_NodePtr p = child;
    //printf("inserting child...\n");
    children[child->getNum()] = child;
    //printf("...done\n");
  }

 PS_NodePtr getChild( SpeciesID id ) { 
   PS_NodeMapIterator it = children.find(id); 
   if (it!=children.end()) return it->second;
   return 0;
}
 const PS_NodePtr getChild( SpeciesID id ) const { 
   PS_NodeMapConstIterator it = children.find(id); 
   if (it!=children.end()) return it->second;
   return 0;
 }

  PS_NodeMapIterator getChildren() { return children.begin(); }
  PS_NodeMapConstIterator getChildren() const { return children.begin(); }

  // *** probes ***
  void addProbe( const PS_ProbePtr& probe ) {
    //printf("addProbe(");PS_printProbe(probe);printf(")\n");
    //PS_ProbePtr p = probe;
    //printf("inserting probe...\n");
    probes.insert(probe);
    //printf("...done\n");
  }

  PS_ProbeSetIterator getProbes() {
    return probes.begin();
  }

  // *** output **
  void print() {
    printf( "\nN[%d] P[ ", num );
    for (PS_ProbeSetIterator i=probes.begin(); i!=probes.end(); ++i) {
      PS_printProbe(*i);
      printf(" ");
    }
    printf( "] C[" );
    for (PS_NodeMap::iterator i=children.begin(); i!=children.end(); ++i) {
      i->second->print();
    }
    printf( "]" );
  }

  // *** constructors ***
  PS_Node( SpeciesID id ) { num = id; } //printf(" constructor PS_Node() num=%d\n",num); }

  // *** destructor ***
  ~PS_Node() {
    printf("destroying node #%d\n",num);
    probes.clear();
    children.clear(); 
  }

private:
  PS_Node()               { pg_assert(0); } //printf(" constructor PS_Node() num=%d\n",num); }
  PS_Node(const PS_Node&); // forbidden
};


// API for Probe-Group-Database format

// --------------------------------------------------------------------------------
// mapping shortname <-> SpeciesID

typedef map<string, int>           String2Int;
typedef String2Int::const_iterator String2IntIter;
typedef map<int,string>            Int2String;
typedef Int2String::const_iterator Int2StringIter;

static String2Int species2num_map;
static Int2String num2species_map;
static bool       species_maps_initialized = false;

//  ----------------------------------------------------------------------
//      GB_ERROR PG_initSpeciesMaps(GBDATA *pb_main)
//  ----------------------------------------------------------------------
GB_ERROR PG_initSpeciesMaps(GBDATA *pb_main) {

  GB_transaction pb_dummy(pb_main);

  pg_assert(!species_maps_initialized);

  // look for existing mapping in pb-db:
  GBDATA *pb_mapping = GB_find(pb_main, "species_mapping", 0, down_level);
  if (!pb_mapping) {  // error
    GB_export_error("No species mapping");
  }  else {
    // retrieve mapping from string
    const char *mapping = GB_read_char_pntr(pb_mapping);
    if (!mapping) return GB_export_error("Can't read mapping");
    
    while (mapping[0]) {
      const char *komma     = strchr(mapping, ',');   if (!komma) break;
      const char *semicolon = strchr(komma, ';');     if (!semicolon) break;
      string      name(mapping, komma-mapping);
      komma+=1;
      string idnum(komma,semicolon-komma);
      SpeciesID   id        = atoi(idnum.c_str());

      species2num_map[name] = id;
      num2species_map[id]   = name;

      mapping = semicolon+1;
    }
  }

  species_maps_initialized = true;
  return 0;
}

//  --------------------------------------------------------------------
//      SpeciesID PG_SpeciesName2SpeciesID(const string& shortname)
//  --------------------------------------------------------------------
SpeciesID PG_SpeciesName2SpeciesID(const string& shortname) {
  pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
  return species2num_map[shortname];
}

//  --------------------------------------------------------------
//      const string& PG_SpeciesID2SpeciesName(SpeciesID num)
//  --------------------------------------------------------------
const string& PG_SpeciesID2SpeciesName(SpeciesID num) {
  pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
  return num2species_map[num];
}

int PG_NumberSpecies(){
    return num2species_map.size();
}

// db-structure of group_tree:
//
//                  <root>
//                  |
//                  |
//                  "group_tree"
//                  |
//                  |
//                  "node" <more nodes...>
//                  | | |
//                  | | |
//                  | | "group" (contains all probes for this group; may be missing)
//                  | |
//                  | "num" (contains species-number (created by PG_SpeciesName2SpeciesID))
//                  |
//                  "node" <more nodes...>
//  
//  Notes:  - the "node"s contained in the path from "group_tree" to any "group"
//            describes the members of the group



//  ---------------------------------------------------
//      static GBDATA *group_tree(GBDATA *pb_main)
//  ---------------------------------------------------
// search or create "group_tree"-entry
// static GBDATA *group_tree(GBDATA *pb_main) {
//     return GB_search(pb_main, "group_tree", GB_CREATE_CONTAINER);
// }


//  -----------------------------------------------------
//      GBDATA *PG_get_first_probe(GBDATA *pb_group)
//  -----------------------------------------------------
GBDATA *PG_get_first_probe(GBDATA *pb_group) {
  return GB_find(pb_group, "probe", 0, down_level);
}

//  ----------------------------------------------------
//      GBDATA *PG_get_next_probe(GBDATA *pb_probe)
//  ----------------------------------------------------
GBDATA *PG_get_next_probe(GBDATA *pb_probe) {
  return GB_find(pb_probe, "probe", 0, this_level|search_next);
}

//  ----------------------------------------------------
//      const char *PG_read_probe(GBDATA *pb_probe)
//  ----------------------------------------------------
const char *PG_read_probe(GBDATA *pb_probe) {
  return GB_read_char_pntr(pb_probe);
}


//  -----------------------------------------------------
//      GBDATA *PS_get_first_node(GBDATA *pb_nodecontainer)
//  -----------------------------------------------------
GBDATA *PS_get_first_node(GBDATA *pb_nodecontainer) {
  return GB_find(pb_nodecontainer, "node", 0, down_level);
}


//  ----------------------------------------------------
//      GBDATA *PS_get_next_node(GBDATA *pb_node)
//  ----------------------------------------------------
GBDATA *PS_get_next_node(GBDATA *pb_node) {
  return GB_find(pb_node, "node", 0, this_level|search_next);
}


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
//      void PS_extract_probe_data( GBDATA *pb_node, int probe_length, PS_NodePtr ps_current_node )
//  ----------------------------------------------------
//  recursively walk through database and extract probe-data
//
void PS_extract_probe_data( GBDATA *pb_node, int probe_length, PS_NodePtr ps_current_node ) {
  //
  // number
  //
  GBDATA     *data   = GB_find( pb_node, "num", 0, down_level );
  const char *buffer = GB_read_char_pntr( data );
  SpeciesID   id     = atoi( buffer );

  PS_NodePtr new_node(new PS_Node(id));                                // make new node and set its id

  //
  // probe(s)
  //
  GBDATA *pb_group = GB_find( pb_node, "group", 0, down_level );        // access probe-group
  if (pb_group) {
    data = PG_get_first_probe( pb_group );                              // get first probe if exists

    while (data) {
      buffer                = PG_read_probe( data );                    // get probe string
      PS_ProbePtr new_probe(new PS_Probe); //(PS_Probe *)malloc(sizeof(PS_Probe));     // make new probe
      new_probe->length     = probe_length;                             // set probe length
      new_probe->quality    = 100;                                      // set probe quality    
      new_probe->GC_content = 0;                                        // eval probe for GC-content
      for (int i=0; i < probe_length; ++i) {
	if ((buffer[i] == 'C') || (buffer[i] == 'G')) ++(new_probe->GC_content);
      }
      new_node->addProbe( new_probe );                                  // append probe to new node
      data = PG_get_next_probe( data );                                 // get next probe
    }
  }

  ps_current_node->addChild( new_node );                                // append new node to current node

  //
  // child(ren)
  //
  GBDATA *pb_child = PS_get_first_node( pb_node );                      // get first child if exists
  while (pb_child) {
    PS_extract_probe_data( pb_child, probe_length, new_node );
    pb_child = PS_get_next_node( pb_child );
  }

}


//  ====================================================
//  ====================================================
int main( int argc,  char *argv[] ) {

  GBDATA   *pb_main = 0;
  GB_ERROR  error   = 0;

  // open probe-group-database
  // printf("argc=%i\n",argc);
  if (argc < 2) {
    printf("Missing arguments\n Usage %s <database name> [output filename]\n",argv[0]);
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
  getchar();

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
  getchar();

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
    printf( "total_node_counter       %20lu\n", total_node_counter );
    printf( "total_child_counter      %20lu\n", total_child_counter );
    printf( "no_probes_node_counter   %20lu\n", no_probes_node_counter );
    printf( "no_children_node_counter %20lu\n", no_children_node_counter );
    printf( "total_probe_counter      %20lu\n", total_probe_counter );
    printf( "max_depth                %20lu\n", max_depth );
    printf( "  depth    nodes/depth probes/depth\n" );
    for ( IntIntMap::iterator i=nodes_per_depth.begin(); i!=nodes_per_depth.end(); ++i) {
      printf( "[ %6lu ] %6lu      %6lu\n", i->first, i->second,probes_per_depth[i->first] );
    }
    printf( "(enter to continue)\n" );
    getchar();
  }

  if (argc == 3) { // output filename given => extract probe-data and save to file
    //
    // stage 1 : extract data
    //
    printf( "extracting probe-data...\n" );
    PS_NodePtr root         = new PS_Node(-1);
    int        probe_length = PS_detect_probe_length( group_tree );
    printf( "probe_length = %d\n",probe_length );
    
    first_level_node = PS_get_first_node( group_tree );
    unsigned int first_level_node_counter = 0;
    do {
      ++first_level_node_counter;
      if (first_level_node_counter % 100 == 0) printf( "1st level node #%u\n", first_level_node_counter );
      PS_extract_probe_data( first_level_node, probe_length, root );
      first_level_node = PS_get_next_node( first_level_node );
    } while (first_level_node);
    printf( "(enter to continue)\n" );
    getchar();

    //
    // stage 2 : write tree to file
    //
    printf( "writing probe-data to %s\n",argv[2] );
    printf( "TODO\n" );
    printf( "(enter to continue)\n" );
    getchar();
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


