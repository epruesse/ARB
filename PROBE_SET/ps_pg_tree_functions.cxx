#ifndef PS_PG_TREE_FUNCTIONS_CXX
#define PS_PG_TREE_FUNCTIONS_CXX

#ifndef PG_SEARCH_HXX
#include "../PROBE_GROUP/pg_search.hxx"
#endif

using namespace std;

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

#else
#error ps_pg_tree_functions.cxx included twice
#endif
