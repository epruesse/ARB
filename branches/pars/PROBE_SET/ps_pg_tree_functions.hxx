#ifndef PS_PG_TREE_FUNCTIONS_HXX
#define PS_PG_TREE_FUNCTIONS_HXX

#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif
#ifndef PS_ASSERT_HXX
#include "ps_assert.hxx"
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif

// API for Probe-Group-Database format

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



inline GBDATA *PG_get_first_probe(GBDATA *pb_group) {
    return GB_entry(pb_group, "probe");
}

inline GBDATA *PG_get_next_probe(GBDATA *pb_probe) {
    ps_assert(GB_has_key(pb_probe, "probe"));
    return GB_nextEntry(pb_probe);
}

inline const char *PG_read_probe(GBDATA *pb_probe) {
    return GB_read_char_pntr(pb_probe);
}

inline GBDATA *PS_get_first_node(GBDATA *pb_nodecontainer) {
    return GB_entry(pb_nodecontainer, "node");
}

inline GBDATA *PS_get_next_node(GBDATA *pb_node) {
    ps_assert(GB_has_key(pb_node, "node"));
    return GB_nextEntry(pb_node);
}

#else
#error ps_pg_tree_functions.hxx included twice
#endif // PS_PG_TREE_FUNCTIONS_HXX
