#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>
#include <adGene.h>

class AWT_canvas;

// --------------------------------------------------------------------------------
// this header is visible ARB-wide (so here are only things needed somewhere else)
// see GEN_local.hxx for local stuff
// --------------------------------------------------------------------------------

#define GENOM_DB_TYPE "genom_db" // main flag (true=genom db, false/missing=normal db)

bool GEN_is_genom_db(GBDATA *gb_main, int default_value = -1);

// --------------------------------------------------------------------------------
// awars:

#define AWAR_GENE_NAME  "tmp/gene/name"
#define GENOM_ALIGNMENT "ali_genom"

void GEN_create_awars(AW_root *aw_root, AW_default aw_def);

// --------------------------------------------------------------------------------
// import:

GB_ERROR GEN_read_genbank(GBDATA *gb_main, const char *filename, const char *ali_name);
GB_ERROR GEN_read_embl(GBDATA *gb_main, const char *filename, const char *ali_name);

// --------------------------------------------------------------------------------
// windows/menus:

AW_window *GEN_create_gene_window(AW_root *aw_root);
AW_window *GEN_create_gene_query_window(AW_root *aw_root);
AW_window *GEN_map(AW_root *aw_root, AW_CL cl_nt_canvas);

void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE, AWT_canvas *ntree_canvas);

// --------------------------------------------------------------------------------
// genes:

GBDATA* GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME
GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME
GBDATA* GEN_get_current_gene(GBDATA *gb_main, AW_root *aw_root); // searches the current gene (using AWAR_ORGANISM_NAME and AWAR_GENE_NAME)

// --------------------------------------------------------------------------------
// toolkit:

// in adtools.c:
// char *GBT_read_gene_sequence P_((GBDATA *gb_gene));

#endif // GEN_HXX
