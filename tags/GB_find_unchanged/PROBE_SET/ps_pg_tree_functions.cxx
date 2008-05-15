#ifndef PS_PG_TREE_FUNCTIONS_CXX
#define PS_PG_TREE_FUNCTIONS_CXX

#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif

using namespace std;

// API for Probe-Group-Database format

// --------------------------------------------------------------------------------
// mapping shortname <-> SpeciesID

static Name2IDMap __NAME2ID_MAP;
static ID2NameMap __ID2NAME_MAP;
static bool       __MAPS_INITIALIZED = false;

//  ----------------------------------------------------------------------
//      GB_ERROR PG_initSpeciesMaps(GBDATA *pb_main)
//  ----------------------------------------------------------------------
GB_ERROR PG_initSpeciesMaps(GBDATA *pb_main) {

  GB_transaction pb_dummy(pb_main);

  ps_assert(!__MAPS_INITIALIZED);

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

      __NAME2ID_MAP[name] = id;
      __ID2NAME_MAP[id]   = name;

      mapping = semicolon+1;
    }
  }

  __MAPS_INITIALIZED = true;
  return 0;
}

//  --------------------------------------------------------------------
//      SpeciesID PG_SpeciesName2SpeciesID(const string& shortname)
//  --------------------------------------------------------------------
SpeciesID PG_SpeciesName2SpeciesID(const string& shortname) {
  ps_assert(__MAPS_INITIALIZED); // you didn't call PG_initSpeciesMaps
  return __NAME2ID_MAP[shortname];
}

//  --------------------------------------------------------------
//      const string& PG_SpeciesID2SpeciesName(SpeciesID num)
//  --------------------------------------------------------------
const string& PG_SpeciesID2SpeciesName(SpeciesID num) {
  ps_assert(__MAPS_INITIALIZED); // you didn't call PG_initSpeciesMaps
  return __ID2NAME_MAP[num];
}

int PG_NumberSpecies(){
    return __ID2NAME_MAP.size();
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
