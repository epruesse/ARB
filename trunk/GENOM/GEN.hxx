#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>

// --------------------------------------------------------------------------------
// this header is visible ARB-wide (so here are only things needed somewhere else)
// see GEN_local.hxx for local stuff
// --------------------------------------------------------------------------------

#define GENOM_DB_TYPE  "genom_db" // main flag (true=genom db, false/missing=normal db)

// --------------------------------------------------------------------------------
// awars:

#define AWAR_GENE_NAME "tmp/gene/name"

void GEN_create_awars(AW_root *aw_root, AW_default aw_def);

// --------------------------------------------------------------------------------
// import:

GB_ERROR GEN_read(GBDATA *gb_main, const char *filename, const char *ali_name);

// --------------------------------------------------------------------------------
// windows/menus:

AW_window *GEN_create_gene_window(AW_root *aw_root);
AW_window *GEN_create_gene_query_window(AW_root *aw_root);
AW_window *GEN_map(AW_root *aw_root);

void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE);
    
// --------------------------------------------------------------------------------
// genes:

inline GBDATA* GEN_get_gene_data(GBDATA *gb_species) { return GB_search(gb_species, "gene_data", GB_CREATE_CONTAINER); }

GBDATA* GEN_get_current_species(GBDATA *gb_main, AW_root *aw_root);
GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root);
GBDATA* GEN_get_current_gene(GBDATA *gb_main, AW_root *aw_root); // searches the current gene (using AWAR_SPECIES_NAME and AWAR_GENE_NAME)


GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name); // find existing gene
GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name); // create or find existing gene
GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); // find existing gene
GBDATA* GEN_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); // create or find existing gene

GBDATA* GEN_first_gene(GBDATA *gb_species);
GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data);
GBDATA* GEN_next_gene(GBDATA *gb_gene);

// --------------------------------------------------------------------------------
// annotations:

GBDATA* GEN_find_annotation(GBDATA *gb_gene, const char *name); // find existing annotation
GBDATA* GEN_create_annotation(GBDATA *gb_gene, const char *name); // create or find existing annotation

GBDATA* GEN_first_annotation(GBDATA *gb_gene);
GBDATA* GEN_next_annotation(GBDATA *gb_gene);

// substitute functions for equivalent species functions:


#endif // GEN_HXX
