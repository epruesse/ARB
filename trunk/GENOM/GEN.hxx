#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>

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

inline GBDATA* GEN_get_gene_data(GBDATA *gb_species) { return GB_search(gb_species, "gene_data", GB_CREATE_CONTAINER); }

GBDATA* GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME
GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME
GBDATA* GEN_get_current_gene(GBDATA *gb_main, AW_root *aw_root); // searches the current gene (using AWAR_ORGANISM_NAME and AWAR_GENE_NAME)

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name); // find existing gene
GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name); // create or find existing gene
GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); // find existing gene
GBDATA* GEN_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); // create or find existing gene

GBDATA* GEN_first_gene(GBDATA *gb_species);
GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data);
GBDATA* GEN_next_gene(GBDATA *gb_gene);

// --------------------------------------------------------------------------------
// pseudo gene-species:

bool GEN_is_pseudo_gene_species(GBDATA *gb_species);
bool GEN_is_organism(GBDATA *gb_species);

const char *GEN_origin_organism(GBDATA *gb_pseudo);
const char *GEN_origin_gene(GBDATA *gb_pseudo);
GBDATA     *GEN_find_origin_organism(GBDATA *gb_pseudo);
GBDATA     *GEN_find_origin_gene(GBDATA *gb_pseudo);
GBDATA     *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name);

GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo);

// @@@ FIXME: missing: GEN_gene_not_found (like GEN_organism_not_found)

GBDATA *GEN_find_pseudo(GBDATA *gb_organism, GBDATA *gb_gene);

GBDATA* GEN_first_pseudo_species(GBDATA *gb_main);
GBDATA* GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data);
GBDATA* GEN_next_pseudo_species(GBDATA *gb_species);

GBDATA *GEN_first_pseudo_species(GBDATA *gb_main);
GBDATA *GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data);
GBDATA *GEN_next_pseudo_species(GBDATA *gb_species);

GBDATA *GEN_first_marked_pseudo_species(GBDATA *gb_main);
GBDATA *GEN_next_marked_pseudo_species(GBDATA *gb_species);

GBDATA *GEN_first_organism(GBDATA *gb_main);
GBDATA *GEN_next_organism(GBDATA *gb_organism);

GBDATA *GEN_first_marked_organism(GBDATA *gb_main);
GBDATA *GEN_next_marked_organism(GBDATA *gb_organism);

// --------------------------------------------------------------------------------
// toolkit:

// in adtools.c:
// char *GBT_read_gene_sequence P_((GBDATA *gb_gene));

#endif // GEN_HXX
