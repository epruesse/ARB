#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>

// this header is visible ARB-wide (so here are only things needed somewhere else)
// see GEN_local.hxx for local stuff

#define GENOM_DB_TYPE  "genom_db" // main flag (true=genom db, false/missing=normal db)

GB_ERROR GEN_read(GBDATA *gb_main, const char *filename, const char *ali_name);

AW_window *GEN_create_gene_window(AW_root *aw_root);
AW_window *GEN_create_gene_query_window(AW_root *aw_root);

#endif // GEN_HXX
