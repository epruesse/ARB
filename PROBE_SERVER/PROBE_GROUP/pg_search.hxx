#ifndef PG_SEARCH_HXX
#define PG_SEARCH_HXX

#ifndef SET
#include <set>
#endif

#ifndef PG_DEF_HXX
#include "pg_def.hxx"
#endif
#ifndef __PG_DB_HXX__
#include "pg_db.hxx"
#endif
#ifndef GLOBAL_DEFS_H
#include "../global_defs.h"
#endif

// DB access (@@@ should better be in module pg_db.xxx)
GBDATA *PG_find_species(GBDATA *node,int id,long gbs);
GBDATA *PG_find_probe_group_for_species(GBDATA *node, const SpeciesBag& species);
GBDATA *PG_find_best_covering_probe_group_for_species(GBDATA *rootNode, const SpeciesBag& species, int min_non_matched, int max_non_matched, int& groupsize);

// initialization (needed for all functions below):
GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername, void (*print_function)(const char *format, ...) FUNCTION_TYPE_ATTR(__ATTR__FORMAT(1)));
void     PG_exit_pt_server(void);

// iterate through all probes:
bool        PG_init_find_probes(int length, GB_ERROR& error);
bool        PG_exit_find_probes();
const char *PG_find_next_probe(GB_ERROR& error);

// probe match:
GB_ERROR PG_probe_match(PG_Group& g, const probe_config_data& para, const char *for_probe);

#else
#error pg_search.hxx included twice
#endif // PG_SEARCH_HXX
